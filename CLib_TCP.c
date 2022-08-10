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
int tcp_server_setup( int *master_socket_ptr, int *epoll_fd_ptr, struct epoll_event events_monitored[MAXCLIENTS+1] ) {
  int opt = 1;
  int i;
  struct sockaddr_in address;

  /* Type of socket created */
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons( PORT );

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
   * MAXCLIENTS backlog, defines the maximum length of pending connections for
   * the master socket */
  if (listen(*master_socket_ptr, MAXCLIENTS) < 0) return -1;


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
  for ( i = 1 ; i < MAXCLIENTS+1 ; i++) {
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
int tcp_server_monitor( int master_socket, int epoll_fd, struct epoll_event events_monitored[MAXCLIENTS+1] ) {
  int event_count, i;
  /* message length */
  int bytes_read;
  /* data buffer */
  char buffer[BUFFERSIZE];
  /* temp socket descripters */
  int sd, new_socket;

  /* array_postion in events_monitored */
  int array_position;
  /* array to store the active_events */
  struct epoll_event active_events[MAXCLIENTS];

  /* Holds address to new socketets */
  struct sockaddr_in address;
  int addrlen;
  addrlen = sizeof(address);


  /**************************** Monitor Activity ******************************/
  /* Wait for an activity on one of the sockets, timeout is set to update at
   * TCPUPDATEFREQ */
  event_count = epoll_wait(epoll_fd, active_events, MAXCLIENTS, 1000/TCPUPDATEFREQ);
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
      array_position = last_ip_digit(inet_ntoa(address.sin_addr))-MINCLIENTADD+1;
      events_monitored[array_position].events = EPOLLIN; /* watch for input events */
      events_monitored[array_position].data.fd = new_socket;
      epoll_ctl(epoll_fd, EPOLL_CTL_ADD, new_socket, &events_monitored[array_position]);
    }
    else {
      /**************************** IO By Client ******************************/
      /* else it is some IO operation on some client socket */
      sd = active_events[i].data.fd;

      /* Read incomming message and store in buffer,
       * and return the number of bytes read to bytes_read */
      bytes_read = read( sd , buffer, BUFFERSIZE);

      if ( bytes_read == 0) {
        /* If valread is 0, then the client disconnected, get details and print */
        getpeername(sd , (struct sockaddr*)&address , \
            (socklen_t*)&addrlen);
        printf("Host disconnected , ip %s , port %d \n" ,
              inet_ntoa(address.sin_addr) , ntohs(address.sin_port));

        /* Remove the client socket from events_monitored, and close the socket */
        array_position = last_ip_digit(inet_ntoa(address.sin_addr))-MINCLIENTADD+1;
        events_monitored[array_position].data.fd = -1;
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, sd, &events_monitored[array_position]);
        close( sd );
      }

      /* Else, a message is sent from the clinet */
      else {
        /* NULL terminate buffer */
        buffer[bytes_read] = '\0';
        printf("Message from client is: %s\n", buffer);

        nsleep(1000000000);
        send(sd , "Hello from server!" , 18 , 0 );
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
void tcp_server_cleanup( int master_socket, int epoll_fd, struct epoll_event events_monitored[MAXCLIENTS+1] ) {
  int i;

  /* close client sockets */
  for ( i = 1; i < MAXCLIENTS+1; i++) {
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

