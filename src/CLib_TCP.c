#include "CLibrary.h"


/************ Static Variables Available in and only in this file ************/
/* IP setting for this TCP instance */
static char* server_ipaddr_; /* TCP server IP address            */
static int port_;            /* TCP port number                  */

/* All clients should have the same first 3 octents, example: 192.168.1.XXX */
static int min_client_addr_; /* The smallest 4th octents of all clients */
static int max_client_addr_; /* The largest  4th octents of all clients */
static int num_clients_;     /* Total number of clients                 */

static double update_freq_; /* Update freqeuncy of TCP */


/* server socket */
static int server_socket_;
/* epoll stuff */
static int server_epoll_fd_;
static struct epoll_event * server_events_monitored_ptr_;

/* server address, used by the client */
struct sockaddr_in serv_addr_;

/* client socket */
static int client_socket_;
/* epoll stuff */
static int client_epoll_fd_;
static struct epoll_event client_events_monitored_;


/* message queues for the server */
static tcpmessagering_t server_message_in_ring_, server_message_out_ring_;
/* message queues for the client */
static tcpmessagering_t client_message_in_ring_, client_message_out_ring_;


/* Counter for number of connected clients */
static int connected_client_couter_;


/************ Static Functions Limited to Access within this File ************/
static void tcp_ring_init( tcpmessagering_t *ring_ptr );
static void tcp_clear_message( tcpmessage_t *message_ptr );
static void tcp_increment_ring_ptr_processing( tcpmessagering_t *ring_ptr );
static void tcp_increment_ring_ptr_new( tcpmessagering_t *ring_ptr );
static void tcp_add_message( tcpmessagering_t *ring_ptr, char* message_ptr, char* source_ip_ptr);
static void tcp_process_message( tcpmessagering_t *ring_ptr, \
  void (*processing_func_ptr)(tcpmessage_t *), void (*emptyring_func_ptr)(void) );



/*
 * Initialize the libraray with all the settings needed
 * Arguments:
 *   server_addr:     [Input] string for server address
 *   port:            [Input] TCP port number
 *   update_freq:     [Input] Update freqeuncy of TCP
 * Return None
 */
void tcp_lib_init(char* server_addr, int port, double update_freq ) {

  /* Set the static paramerters */
  server_ipaddr_ = server_addr;
  port_          = port;
  update_freq_   = update_freq;

  /* Initalize the message rings for the server */
  tcp_ring_init( &server_message_in_ring_  );
  tcp_ring_init( &server_message_out_ring_ );
  /* Initalize the message rings for the client */
  tcp_ring_init( &client_message_in_ring_  );
  tcp_ring_init( &client_message_out_ring_ );

  return;
}


/*
 * Setup server side for TCP
 * Arguments:
 *   min_client_addr: [Input] The smallest 4th octents of all clients
 *   max_client_addr: [Input] The largest  4th octents of all clients
 * Return:
 *    0: if setup successful
 *   -1: if setup failed
 */
int tcp_server_setup( int min_client_addr, int max_client_addr ) {
  int opt = 1;
  int i;
  struct sockaddr_in address;


  /* min and max client address, and the total number of clients */
  min_client_addr_ = min_client_addr;
  max_client_addr_ = max_client_addr;
  num_clients_     = max_client_addr_ - min_client_addr_ + 1;


  /* allocate server_events_monitored_ptr_ */
  server_events_monitored_ptr_ = (struct epoll_event*) calloc(num_clients_+1, sizeof(struct epoll_event));
  if (server_events_monitored_ptr_ == NULL) {
    fprintf(stderr, "Out of memory!\n");
    exit(1);
  }

  /* Type of socket created */
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons( port_ );

  /* Create a master socket
   * AF_INET for IPV4, SOCK_STREAM for TCP, 0 for default protocol
   * Creates a socket descriptor: server_socket_ */
  if ( (server_socket_ = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
    free( server_events_monitored_ptr_ );
    return -1;
  }

  /* Set master socket to allow multiple connections */
  if ( setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR, (char *)&opt,
        sizeof(opt)) < 0 ) {
    free( server_events_monitored_ptr_ );
    return -1;
  }

  /* Bind the socket to localhost port 8888
   * Bind the socket to the address and port number specified in address */
  if (bind(server_socket_, (struct sockaddr *)&address, sizeof(address)) < 0) {
    free( server_events_monitored_ptr_ );
    return -1;
  }

  /* Listen
   * puts the server socket in a passive mode, where it waits for the client to
   * approach the server to make a connection.
   * num_clients_ backlog, defines the maximum length of pending connections for
   * the master socket */
  if (listen(server_socket_, num_clients_) < 0) {
    free( server_events_monitored_ptr_ );
    return -1;
  }


  /* Create epoll file descriptor */
  if ( (server_epoll_fd_ = epoll_create1(0)) == -1) {
    free( server_events_monitored_ptr_ );
    return -1;
  }

  /* Add server_socket_ to epoll monitor */
  server_events_monitored_ptr_->events = EPOLLIN; /* watch for input events */
  server_events_monitored_ptr_->data.fd = server_socket_;
  if(epoll_ctl(server_epoll_fd_, EPOLL_CTL_ADD, server_socket_, server_events_monitored_ptr_)) {
    close(server_epoll_fd_);
    free( server_events_monitored_ptr_ );
    return -1;
  }


  /* Initialise all client sockets to -1 */
  for ( i = 1 ; i < num_clients_+1 ; i++) {
    (server_events_monitored_ptr_ + i)->data.fd = -1;
  }

  /* Initialize connected client counter to 0 */
  connected_client_couter_ = 0;

  return 0;
}


/*
 * Montior TCP comm from the server side, run this continuously in a loop
 * Arguments: None
 * Return:
 *   on success: number of connected clients
 *   on failure: -1
 */
int tcp_server_monitor( void ) {
  int event_count, i;
  /* message length */
  int bytes_read;
  /* data buffer */
  char buffer[TCPBUFFERSIZE];
  /* temp socket descripters */
  int sd, new_socket;

  /* array_postion in server_events_monitored_ptr_ */
  int array_position;
  /* array to store the active_events */
  struct epoll_event * active_events_ptr;

  /* Holds address to new socketets */
  struct sockaddr_in address;
  int addrlen;
  addrlen = sizeof(address);


  /* allocate active_events_ptr */
  active_events_ptr = (struct epoll_event*) calloc(num_clients_+1, sizeof(struct epoll_event));
  if (active_events_ptr == NULL) {
    fprintf(stderr, "Out of memory!\n");
    exit(-1);
  }

  /**************************** Monitor Activity ******************************/
  /* Wait for an activity on one of the sockets, timeout is set to update at
   * update_freq_ */
  event_count = epoll_wait(server_epoll_fd_, active_events_ptr, num_clients_+1, round(1000.0/update_freq_));
  /* Whenever a new client connects, server_socket_ will be activated and a new
   * fd will be open for that client. We will store its fd in the array
   * server_events_monitored_ptr_ and in the add it to epoll to monitor for
   * activity from this client.
   * Similarly, if an old client sends some data, the file descrpitor will
   * activated. */

  /********************* Loop over all the active events *********************/
  for (i = 0; i < event_count; i++) {

    if ( (active_events_ptr + i)->data.fd == server_socket_ ) {
      /*************************** New Connection *****************************/
      /* If the master socket is active, then it is an new connection. */

      /* accept new connection */
      new_socket = accept( server_socket_, (struct sockaddr *)&address, \
        (socklen_t*)&addrlen );
      if ( new_socket < 0 ) {
        free( active_events_ptr );
        return -1;
      }

      /* Print new connection information */
      printf("New connection , socket fd is %d , ip is : %s , port : %d\n" , \
        new_socket , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));

      /* Add new socket to be monitors */
      array_position = last_ip_digit(inet_ntoa(address.sin_addr))-min_client_addr_+1;
      (server_events_monitored_ptr_ + array_position)->events = EPOLLIN; /* watch for input events */
      (server_events_monitored_ptr_ + array_position)->data.fd = new_socket;
      epoll_ctl(server_epoll_fd_, EPOLL_CTL_ADD, new_socket, (server_events_monitored_ptr_ + array_position));

      /* Increment connected client counter */
      connected_client_couter_ += 1;
    }
    else {
      /**************************** IO By Client ******************************/
      /* else it is some IO operation on some client socket */
      sd = (active_events_ptr + i)->data.fd;

      /* Clear buffer first */
      memset( buffer, '\0', sizeof(buffer) );

      /* Read incomming message and store in buffer,
       * and return the number of bytes read to bytes_read */
      bytes_read = read( sd , buffer, TCPBUFFERSIZE );
      /* Ensure NULL Terminate */
      buffer[TCPBUFFERSIZE-1] = '\0';

      if ( bytes_read == 0) {
        /* If valread is 0, then the client disconnected, get details and print */
        getpeername(sd , (struct sockaddr*)&address , (socklen_t*)&addrlen);
        printf("Client disconnected , ip %s , port %d \n" ,
              inet_ntoa(address.sin_addr) , ntohs(address.sin_port));

        /* Remove the client socket from server_events_monitored_ptr_, and close the socket */
        array_position = last_ip_digit(inet_ntoa(address.sin_addr))-min_client_addr_+1;
        (server_events_monitored_ptr_ + array_position)->data.fd = -1;
        epoll_ctl(server_epoll_fd_, EPOLL_CTL_DEL, sd, (server_events_monitored_ptr_ + array_position));
        close( sd );

        /* Decrement connected client counter */
        connected_client_couter_ -= 1;
      }

      /* Else, a message is sent from the clinet */
      else {
        /* Get client detail */
        getpeername(sd , (struct sockaddr*)&address , (socklen_t*)&addrlen);

        /* Add message and sender IP to the TCP message ring.
         * Note that Buffer is already NULL terminated */
        tcp_add_message( &server_message_in_ring_, buffer, inet_ntoa(address.sin_addr) );
      }
    }
  }

  free( active_events_ptr );
  return connected_client_couter_;
}


/*
 * Cleanup TCP comm from the server side, closes the master socket, client
 * sockets and the epoll file descriptor
 * Arguments: None
 * Return: None
 */
void tcp_server_cleanup( void ) {
  int i;

  /* Free dynamic allocation */
  free( server_events_monitored_ptr_ );

  /* close client sockets */
  for ( i = 1; i < num_clients_+1; i++) {
    if ( (server_events_monitored_ptr_ + i)->data.fd > 0 ) {
      close( (server_events_monitored_ptr_ + i)->data.fd );
    }
  }

  /* close server_socket_ */
  close( server_socket_ );

  /* close epoll file descriptor */
  close( server_epoll_fd_ );

  return;
}


/*
 * Process one message in the input message ring of the server
 *
 * Arguments
 *   processing_func_ptr: [Input]
 *                        pointer to the function that is used for processsing,
 *                        this function should take (tcpmessage_t *) as an input
 *                        this function will be executed once with the first element in the ring as the input
 *   emptyring_func_ptr:  [Input]
 *                        pointer to the function that is used if the ring is empty
 *                        this pointer can be NULL if you don't want anything done on a empty ring
 *
 * Return: None
 */
void tcp_server_process_message( void (*processing_func_ptr)(tcpmessage_t *), void (*emptyring_func_ptr)(void) ) {
  tcp_process_message( &server_message_in_ring_, processing_func_ptr, emptyring_func_ptr );
  return;
}


/*
 * Send all messages in the outbound message queue of the server
 * Arguments: None
 * Return   : None
 */
void tcp_server_send_message( void ) {
  int array_position;
  /* temp socket descriptor */
  int sd;

  /* keep sending message as long as the ring is not empty */
  while ( server_message_out_ring_.ptr_processing != server_message_out_ring_.ptr_new ) {
    /* find the array position based on the last IP digit of the destination IP address */
    array_position = last_ip_digit(server_message_out_ring_.ptr_processing->source_ip)-min_client_addr_+1;
    /* get the socket descriptor from the monitored array */
    sd = (server_events_monitored_ptr_ + array_position)->data.fd;
    if ( sd == -1 ) {
      /* if the desgination IP address is not a connected client, print error message and ignore this message */
      print_time();
      fprintf(error_log_, "Message sending failure, IP Address: %s is not connected, message is: %s\n", \
        server_message_out_ring_.ptr_processing->source_ip, server_message_out_ring_.ptr_processing->message);
    }
    else{
      /* otherwise, send the message to the client */
      send(sd , server_message_out_ring_.ptr_processing->message, TCPBUFFERSIZE, 0 );
    }

    /* clear the proccessed message */
    tcp_clear_message( server_message_out_ring_.ptr_processing );
    /* increment the proccessing pointer */
    tcp_increment_ring_ptr_processing( &server_message_out_ring_ );
  }
  return;
}


/*
 * Add one message to the outbound message queue of the server
 * Arguments
 *   message:        [Input]
 *                   string to put as the message
 *                   messages with more than TCPBUFFERSIZE-1 characters will have the end discarded
 *   destination_ip: [Input]
 *                   string to put as the destination ip address for this new message
 *                   string with more than IPADDRSIZE-1 characters will have the end discarded
 *
 * Return: None
 */
void tcp_server_add_message_sendqueue( char* message_ptr, char* destination_ip_ptr ) {
  tcp_add_message( &server_message_out_ring_, message_ptr, destination_ip_ptr);
  return;
}




/*
 * Setup client side for TCP
 * Arguments: None
 * Return:
 *    0: if setup successful
 *   -1: if setup failed
 */
int tcp_client_setup( void ) {
  /* setup serv_addr struct with IP and port */
  bzero(&serv_addr_, sizeof(serv_addr_));
  serv_addr_.sin_family = AF_INET;
  serv_addr_.sin_port = htons(port_);
  /* Convert IPv4 addresses from text to binary form */
  serv_addr_.sin_addr.s_addr = inet_addr(server_ipaddr_);

  /* Create a client socket
   * AF_INET for IPV4, SOCK_STREAM for TCP, 0 for default protocol
   * Creates a socket descriptor: client_socket_ */
  if ((client_socket_ = socket(AF_INET, SOCK_STREAM, 0)) < 0) return -1;

  /* Create epoll file descriptor */
  if ( (client_epoll_fd_ = epoll_create1(0)) == -1) return -1;

  return 0;
}


/*
 * Attempt to reconncet to the server, should run within a loop than checks for the state_
 * Arguments: None
 * Return   :  0 on success
 *            -1 if server not yet online
 *            -2 on error
 */
int tcp_client_reconnect( void ) {
  /* Connect the socket descriptor: client_socket_ to the server address
   * returns 0 on success, return -1 if failed, indicating the server is not
   * yet online. We will keep trying to connect to the server until
   * connection is established or the state_ turns to exiting */
  if (connect(client_socket_, (struct sockaddr*)&serv_addr_, sizeof(serv_addr_)) < 0) {
    nsleep(1000000000/update_freq_);
    return -1;
  }
  /* Else the connection was established */
  else {
    print_time();
    fprintf(error_log_, "Connected to server!\n");

    /* Add server_socket_ to epoll monitor */
    client_events_monitored_.events = EPOLLIN; /* watch for input events */
    client_events_monitored_.data.fd = client_socket_;
    if(epoll_ctl(client_epoll_fd_, EPOLL_CTL_ADD, client_socket_, &client_events_monitored_)) {
      close(client_epoll_fd_);
      return -2;
    }
    return 0;
  }
}


/*
 * Montior TCP comm from the client side, run this continuously in a loop
 * Arguments: None
 * Return:
 *    0: on success
 *   -1: if server disconnected
 */
int tcp_client_monitor( void ) {
  struct epoll_event active_events;
  int event_count, bytes_read;

  char buffer[TCPBUFFERSIZE];


  /**************************** Monitor Activity ******************************/
  /* Wait for an activity on one of the sockets, timeout is set to update at
   * update_freq_ */
  event_count = epoll_wait(client_epoll_fd_, &active_events, 1, round(1000.0/update_freq_));
  /* If server disconnects or send message, there will be activity*/


  /*************************** Deal With Activity *****************************/
  if ( event_count == 1 ) {
    /* Clear buffer first */
    memset( buffer, '\0', sizeof(buffer) );

    /* Read incomming message and store in buffer,
     * and return the number of bytes read to bytes_read */
    bytes_read = read( client_socket_ , buffer, TCPBUFFERSIZE );
    /* Ensure NULL Terminate */
    buffer[TCPBUFFERSIZE-1] = '\0';

    if ( bytes_read == 0) {
      /* If valread is 0, then the server disconnected. */
      print_time();
      fprintf(error_log_, "Server disconnected.\n");

      /* Remove the client socket from client_events_monitored_, and do not close the socket */
      client_events_monitored_.data.fd = -1;
      epoll_ctl(client_epoll_fd_, EPOLL_CTL_DEL, client_socket_, &client_events_monitored_);

      /* clean up, close socket (needs to be reset before attempting to reconnect)
       * close epoll */
      tcp_client_cleanup( );

      return -1;
    }

    else {
      /* Else, a message is sent from the server
       * Add message and server IP to the TCP message ring. */
      tcp_add_message( &client_message_in_ring_, buffer, server_ipaddr_ );
    }

  }
  return 0;
}


/*
 * Cleanup TCP comm from the client side, close the client_socket_ and the
 * epoll file descriptor
 * Arguments: None
 * Return: None
 */
void tcp_client_cleanup( void ) {
  /* closing the client socket */
  close(client_socket_);
  /* closing the epoll file descriptor */
  close(client_epoll_fd_);

  return;
}


/*
 * Process one message in the input message ring of the client
 *
 * Arguments
 *   processing_func_ptr: [Input]
 *                        pointer to the function that is used for processsing,
 *                        this function should take (tcpmessage_t *) as an input
 *                        this function will be executed once with the first element in the ring as the input
 *   emptyring_func_ptr:  [Input]
 *                        pointer to the function that is used if the ring is empty
 *                        this pointer can be NULL if you don't want anything done on a empty ring
 *
 * Return: None
 */
void tcp_client_process_message( void (*processing_func_ptr)(tcpmessage_t *), void (*emptyring_func_ptr)(void) ) {
  tcp_process_message( &client_message_in_ring_, processing_func_ptr, emptyring_func_ptr );
  return;
}


/*
 * Send all messages in the outbound message queue of the server
 * Arguments: None
 * Return   : None
 */
void tcp_client_send_message( void ) {
  /* keep sending message as long as the ring is not empty */
  while ( client_message_out_ring_.ptr_processing != client_message_out_ring_.ptr_new ) {
    /* otherwise, send the message to the client */
    send(client_socket_ , client_message_out_ring_.ptr_processing->message, TCPBUFFERSIZE, 0 );

    /* clear the proccessed message */
    tcp_clear_message( client_message_out_ring_.ptr_processing );
    /* increment the proccessing pointer */
    tcp_increment_ring_ptr_processing( &client_message_out_ring_ );
  }
  return;
}


/*
 * Add one message to the outbound message queue of the client
 * Arguments
 *   message:        [Input]
 *                   string to put as the message
 *                   messages with more than TCPBUFFERSIZE-1 characters will have the end discarded
 *
 * Return: None
 */
void tcp_client_add_message_sendqueue( char* message_ptr ) {
  tcp_add_message( &client_message_out_ring_, message_ptr, server_ipaddr_ );
  return;
}




/*
 * Initialize a tcp message ring
 *
 * Arguments:
 *   ring_ptr: [Input/Output] pointer to a message ring that is type tcpmessagering_t
 *
 * Return: None
 */
void tcp_ring_init( tcpmessagering_t *ring_ptr ) {
  int i;

  for (i = 0; i < TCPRINGSIZE; i++ ) {
    /* loop over all messages in the ring and clear those */
    tcp_clear_message( &(ring_ptr->messages[i]) );
  }

  /* set start and end pointers */
  ring_ptr->ptr_start = &(ring_ptr->messages[0]);
  ring_ptr->ptr_end   = &(ring_ptr->messages[TCPRINGSIZE-1]);

  /* initialize processing and new pointers to the start pointer */
  ring_ptr->ptr_processing = ring_ptr->ptr_start;
  ring_ptr->ptr_new        = ring_ptr->ptr_start;

  return;
}


/*
 * Clear a message
 *
 * Arguments:
 *   message_ptr: [Input/Output] pointer to a message that is type tcpmessage_t
 *
 * Return: None
 */
void tcp_clear_message( tcpmessage_t *message_ptr ) {
  memset( message_ptr->message, '\0', sizeof(message_ptr->message) );
  memset( message_ptr->source_ip, '\0', sizeof(message_ptr->source_ip) );
  return;
}


/*
 * Increment the processing pointer in a message ring
 *
 * Arguments:
 *   ring_ptr: [Input/Output] pointer to a message ring that is type tcpmessagering_t
 *
 * Return: None
 */
void tcp_increment_ring_ptr_processing( tcpmessagering_t *ring_ptr ) {
  if ( ring_ptr->ptr_processing == ring_ptr->ptr_end ) {
    /* if the pointer is already the end pointer, then set it to the start pointer */
    ring_ptr->ptr_processing = ring_ptr->ptr_start;
  }
  else {
    /* otherwise, increment it by 1 */
    ring_ptr->ptr_processing ++;
  }
  return;
}


/*
 * Increment the new pointer in a message ring
 *
 * Arguments:
 *   ring_ptr: [Input/Output] pointer to a message ring that is type tcpmessagering_t
 *
 * Return: None
 */
void tcp_increment_ring_ptr_new( tcpmessagering_t *ring_ptr ) {
  if ( ring_ptr->ptr_new == ring_ptr->ptr_end ) {
    /* if the pointer is already the end pointer, then set it to the start pointer */
    ring_ptr->ptr_new = ring_ptr->ptr_start;
  }
  else {
    /* otherwise, increment it by 1 */
    ring_ptr->ptr_new ++;
  }

  /* If after incrementing the ptr_new, it reaches the same as ptr_processing
   * (catching up from behind), then it indicates that the ring is full.
   * It will print an error message and pretend the entire ring is empty, which
   * means the entire ring is effectively cleared and all data lost */
  if (ring_ptr->ptr_new == ring_ptr->ptr_processing) {
    print_time();
    fprintf(error_log_, "Ring full, entire ring cleared, data lose occured!\n");
  }

  return;
}


/*
 * Add one message to the message ring
 *
 * Arguments
 *   ring_ptr:  [Input/Output]
 *              pointer to the message ring
 *   message:   [Input]
 *              string to put as the message
 *              messages with more than TCPBUFFERSIZE-1 characters will have the end discarded
 *   source_ip: [Input]
 *              string to put as the source ip address for this new message
 *              string with more than IPADDRSIZE-1 characters will have the end discarded
 *
 * Return: None
 */
void tcp_add_message( tcpmessagering_t *ring_ptr, char* message_ptr, char* source_ip_ptr) {
  /* First clear the address */
  tcp_clear_message( ring_ptr->ptr_new );

  /* write to the ring */
  strncpy( ring_ptr->ptr_new->message  , message_ptr  , TCPBUFFERSIZE );
  strncpy( ring_ptr->ptr_new->source_ip, source_ip_ptr, IPADDRSIZE    );
  /* Ensure Null Terminate */
  ring_ptr->ptr_new->message[TCPBUFFERSIZE - 1] = '\0';
  ring_ptr->ptr_new->source_ip[IPADDRSIZE - 1] = '\0';

  /* Increment ptr_new */
  tcp_increment_ring_ptr_new( ring_ptr );

  return;
}


/*
 * Process one message in the message ring
 *
 * Arguments
 *   ring_ptr:            [Input]
 *                        pointer to the ring to be processed
 *   processing_func_ptr: [Input]
 *                        pointer to the function that is used for processsing,
 *                        this function should take (tcpmessage_t *) as an input
 *                        this function will be executed once with the first element in the ring as the input
 *   emptyring_func_ptr:  [Input]
 *                        pointer to the function that is used if the ring is empty
 *                        this pointer can be NULL if you don't want anything done on a empty ring
 *
 * Return: None
 */
void tcp_process_message( tcpmessagering_t *ring_ptr, void (*processing_func_ptr)(tcpmessage_t *), void (*emptyring_func_ptr)(void) ) {
  if ( ring_ptr->ptr_processing == ring_ptr->ptr_new ) {
    /* if ptr_processing and ptr_new is the same, then the ring is empty and
     * call the function *emptyring_func_ptr if it is not a NULL pointer */
    if (emptyring_func_ptr) (*emptyring_func_ptr)();
    return;
  }
  else { /* if the ring is not empty */
    /* process the first non-empty element in the ring */
    (*processing_func_ptr)( ring_ptr->ptr_processing );
    /* clear the proccessed message */
    tcp_clear_message( ring_ptr->ptr_processing );
    /* increment the proccessing pointer */
    tcp_increment_ring_ptr_processing( ring_ptr );
  }
  return;
}
