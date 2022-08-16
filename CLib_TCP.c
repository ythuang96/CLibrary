#include "CLibrary.h"

/*
 * Setup server side for TCP
 * Arguments:
 *   master_socket_ptr: [Output] pointer to the master socket descriptor
 *   epoll_fd_ptr:      [Output] pointer to the epoll file descriptor
 *   events_monitored:  [Output] array of events that are monitored, with first
 *                               one being the master socket and the client
 *                               socket arranged base on client ip address
 * Return:
 *    0: if setup successful
 *   -1: if setup failed
 */
int tcp_server_setup( int *master_socket_ptr, int *epoll_fd_ptr, \
  struct epoll_event events_monitored[TCPMAXCLIENTS+1] ) {
  int opt = 1;
  int i;
  struct sockaddr_in address;

  /* Type of socket created */
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons( TCPPORT );

  /* Create a master socket
   * AF_INET for IPV4, SOCK_STREAM for TCP, 0 for default protocol
   * Creates a socket descriptor: *master_socket_ptr */
  if ( (*master_socket_ptr = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
    return -1;
  }

  /* Set master socket to allow multiple connections */
  if ( setsockopt(*master_socket_ptr, SOL_SOCKET, SO_REUSEADDR, (char *)&opt,
        sizeof(opt)) < 0 ) {
    return -1;
  }

  /* Bind the socket to localhost port 8888
   * Bind the socket to the address and port number specified in address */
  if (bind(*master_socket_ptr, (struct sockaddr *)&address, sizeof(address)) < 0) {
    return -1;
  }

  /* Listen
   * puts the server socket in a passive mode, where it waits for the client to
   * approach the server to make a connection.
   * TCPMAXCLIENTS backlog, defines the maximum length of pending connections for
   * the master socket */
  if (listen(*master_socket_ptr, TCPMAXCLIENTS) < 0) return -1;


  /* Create epoll file descriptor */
  if ( (*epoll_fd_ptr = epoll_create1(0)) == -1) return -1;

  /* Add master_socket to epoll monitor */
  events_monitored[0].events = EPOLLIN; /* watch for input events */
  events_monitored[0].data.fd = *master_socket_ptr;
  if(epoll_ctl(*epoll_fd_ptr, EPOLL_CTL_ADD, *master_socket_ptr, &events_monitored[0])) {
    close(*epoll_fd_ptr);
    return -1;
  }


  /* Initialise all client sockets to -1 */
  for ( i = 1 ; i < TCPMAXCLIENTS+1 ; i++) {
    events_monitored[i].data.fd = -1;
  }

  return 0;
}


/*
 * Montior TCP comm from the server side, run this continuously in a loop
 * Arguments:
 *   master_socket:    [Input]
 *                     file ID for master socket
 *   epoll_fd:         [Input]
 *                     epoll file descriptor
 *   events_monitored: [Input/Output]
 *                     array of file IDs for the client socket, gets updated by
 *                     this function if new clients connects to the server
 * Return:
 *    0: on success
 *   -1: on failure
 */
int tcp_server_monitor( int master_socket, int epoll_fd, \
    struct epoll_event events_monitored[TCPMAXCLIENTS+1], tcpmessagering_t *ring_ptr) {
  int event_count, i;
  /* message length */
  int bytes_read;
  /* data buffer */
  char buffer[TCPBUFFERSIZE];
  /* temp socket descripters */
  int sd, new_socket;

  /* array_postion in events_monitored */
  int array_position;
  /* array to store the active_events */
  struct epoll_event active_events[TCPMAXCLIENTS];

  /* Holds address to new socketets */
  struct sockaddr_in address;
  int addrlen;
  addrlen = sizeof(address);


  /**************************** Monitor Activity ******************************/
  /* Wait for an activity on one of the sockets, timeout is set to update at
   * TCPUPDATEFREQ */
  event_count = epoll_wait(epoll_fd, active_events, TCPMAXCLIENTS, 1000/TCPUPDATEFREQ);
  /* Whenever a new client connects, master_socket will be activated and a new
   * fd will be open for that client. We will store its fd in the array
   * events_monitored and in the add it to epoll to monitor for activity from
   * this client.
   * Similarly, if an old client sends some data, the file descrpitor will
   * activated. */

  /********************* Loop over all the active events *********************/
  for (i = 0; i < event_count; i++) {

    if ( active_events[i].data.fd == master_socket ) {
      /*************************** New Connection *****************************/
      /* If the master socket is active, then it is an new connection. */

      /* accept new connection */
      new_socket = accept( master_socket, (struct sockaddr *)&address, \
        (socklen_t*)&addrlen );

      if ( new_socket < 0 ) return -1;

      /* Print new connection information */
      printf("New connection , socket fd is %d , ip is : %s , port : %d\n" , \
        new_socket , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));

      /* Add new socket to be monitors */
      array_position = last_ip_digit(inet_ntoa(address.sin_addr))-TCPMINCLIENTADD+1;
      events_monitored[array_position].events = EPOLLIN; /* watch for input events */
      events_monitored[array_position].data.fd = new_socket;
      epoll_ctl(epoll_fd, EPOLL_CTL_ADD, new_socket, &events_monitored[array_position]);
    }
    else {
      /**************************** IO By Client ******************************/
      /* else it is some IO operation on some client socket */
      sd = active_events[i].data.fd;

      /* Clear buffer first */
      memset( buffer, '\0', sizeof(buffer) );

      /* Read incomming message and store in buffer,
       * and return the number of bytes read to bytes_read,
       * the TCPBUFFERSIZE-1 is there to leave one extra space for NULL termination */
      bytes_read = read( sd , buffer, TCPBUFFERSIZE-1 );

      if ( bytes_read == 0) {
        /* If valread is 0, then the client disconnected, get details and print */
        getpeername(sd , (struct sockaddr*)&address , (socklen_t*)&addrlen);
        printf("Host disconnected , ip %s , port %d \n" ,
              inet_ntoa(address.sin_addr) , ntohs(address.sin_port));

        /* Remove the client socket from events_monitored, and close the socket */
        array_position = last_ip_digit(inet_ntoa(address.sin_addr))-TCPMINCLIENTADD+1;
        events_monitored[array_position].data.fd = -1;
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, sd, &events_monitored[array_position]);
        close( sd );
      }

      /* Else, a message is sent from the clinet */
      else {
        /* Get client detail */
        getpeername(sd , (struct sockaddr*)&address , (socklen_t*)&addrlen);

        /* Add message and sender IP to the TCP message ring.
         * Note that Buffer is already NULL terminated,
         * due to the memset and TCPBUFFERSIZE-1 in previous code */
        tcp_add_message( ring_ptr, buffer, inet_ntoa(address.sin_addr) );
      }
    }
  }

  return 0;
}


/*
 * Cleanup TCP comm from the server side, closes the master socket, client
 * sockets and the epoll file descriptor
 * Arguments:
 *   master_socket:    [Input]
 *                     file ID for master socket
 *   epoll_fd:         [Input]
 *                     epoll file descriptor
 *   events_monitored: [Input]
 *                     array of file IDs for the client socket
 * Return:
 *   None
 */
void tcp_server_cleanup( int master_socket, int epoll_fd, \
  struct epoll_event events_monitored[TCPMAXCLIENTS+1] ) {
  int i;

  /* close client sockets */
  for ( i = 1; i < TCPMAXCLIENTS+1; i++) {
    if ( events_monitored[i].data.fd > 0 ) {
      close( events_monitored[i].data.fd );
    }
  }

  /* close master_socket */
  close( master_socket );

  /* close epoll file descriptor */
  close( epoll_fd );

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
 * Process one message in the message ring
 *
 * Arguments
 *   ring_ptr:            [Input/Output]
 *                        pointer to the message ring
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
void tcp_process_message( tcpmessagering_t *ring_ptr, \
  void (*processing_func_ptr)(tcpmessage_t *), void (*emptyring_func_ptr)(void) ) {
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
void tcp_add_message( tcpmessagering_t *ring_ptr, \
  char message[TCPBUFFERSIZE], char source_ip[IPADDRSIZE]) {
  /* First clear the address */
  tcp_clear_message( ring_ptr->ptr_new );

  /* write to the ring */
  strncpy( ring_ptr->ptr_new->message  , message  , TCPBUFFERSIZE-1 );
  strncpy( ring_ptr->ptr_new->source_ip, source_ip, IPADDRSIZE-1    );
  /* Note that NULL termination is automatic, since message and source_ip are
   * always set with NULL before this operation, and we only copy size-1
   * characters, which will ensure that at least the last character will always
   * be NULL. */

  /* Increment ptr_new */
  tcp_increment_ring_ptr_new( ring_ptr );

  return;
}
