#include "CLibrary.h"

/*
 * Setup server side for TCP
 * Arguments:
 *   master_socket_ptr: [int *, Output] the master socket descriptor
 * Return:
 *    0: if setup successful
 *   -1: if setup failed
 */
int tcp_server_setup( int *master_socket_ptr ) {
  int opt = 1;
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
   * 10 backlog, defines the maximum length of pending connections for the
   * master socket */
  if (listen(*master_socket_ptr, 10) < 0) {
    return -1;
  }

  return 0;
}


/*
 * Montior TCP comm from the server side, run this continuously in a loop
 * Arguments:
 *   master_socket:  [int, Input]
 *                   file ID for master socket
 *   client_sockets: [int, size MAXCLIENTS, Input/Output]
 *                   array of file IDs for the client socket, gets updated by
 *                   this function if new clients connects to the server
 * Return:
 *    0: on success
 *   -1: on failure
 */
int tcp_server_monitor( int master_socket, int client_sockets[MAXCLIENTS] ) {
  int i;
  /* message length */
  int valread;
  /* data buffer */
  char buffer[BUFFERSIZE];
  /* Set of socket descriptors
   * which will monitor all the active file descriptors of all the clients
   * plus that of the main server listening socket.*/
  fd_set readfds;
  /* temp socket descripters */
  int sd, max_sd, new_socket;
  /* time interval for select */
  struct timeval tv;
  /* activity from select */
  int activity;

  /* Holds address to new socketets */
  struct sockaddr_in address;
  int addrlen;
  addrlen = sizeof(address);

  /* select time out time */
  tv.tv_sec = 0;
  tv.tv_usec = 1000000/TCPUPDATEFREQ;

  /************************** Build the socket set ****************************/
  FD_ZERO(&readfds);                /* clear the socket set               */
  FD_SET(master_socket, &readfds);  /* add master socket to set           */
  max_sd = master_socket;           /* set highest file descriptor number */
                                    /* need it for the select function    */

  /* Add each client sockets to set */
  for ( i = 0 ; i < MAXCLIENTS ; i++) {
    sd = client_sockets[i];
    /* If valid socket descriptor, then add to read list */
    if(sd > 0) FD_SET( sd , &readfds);
    /* Set highest file descriptor number */
    if(sd > max_sd) max_sd = sd;
  }


  /**************************** Monitor Activity ******************************/
  /* Wait for an activity on one of the sockets, timeout is tv ,
   * set to update at TCPUPDATEFREQ
   * After select() has returned, readfds will be cleared of all file
   * descriptors except for those that are ready for reading.
   * We will need to rebuild readfds on next loop. */
  activity = select( max_sd + 1 , &readfds , NULL , NULL , &tv);
  /* return if select timed out */
  if (activity == 0) return 0;

  /* Whenever a new client connects, master_socket will be activated and a new
   * fd will be open for that client. We will store its fd in the array
   * client_sockets and in the next iteration we will add it to the readfds to
   * monitor for activity from this client.
   * Similarly, if an old client sends some data, readfds will be activated and
   * we will check from the list of existing client to see which client has
   * send the data. */


  /***************************** New Connection *******************************/
  /* If something happened on the master socket, then its an new connection.
   * Check with FD_ISSET if master_socket is in the active list */
  if (FD_ISSET(master_socket, &readfds)) {
    /* accept new connection */
    new_socket = accept( master_socket, (struct sockaddr *)&address, \
      (socklen_t*)&addrlen );

    if ( new_socket < 0 ) return -1;

    /* Print new connection information */
    printf("New connection , socket fd is %d , ip is : %s , port : %d\n" , \
      new_socket , inet_ntoa(address.sin_addr) , ntohs
          (address.sin_port));

    /* Add new socket to array of sockets */
    for (i = 0; i < MAXCLIENTS; i++) {
      /* if position is empty */
      if( client_sockets[i] == 0 ) {
        client_sockets[i] = new_socket;
        break;
      }
    }
  }


  /****************************** IO By Client ********************************/
  /* else its some IO operation on some other socket */
  /* loop over all client sockets */
  for (i = 0; i < MAXCLIENTS; i++) {
    sd = client_sockets[i];

    /* Check if this clinet is active */
    if (FD_ISSET( sd , &readfds)) {
      /* Read incomming message and store in buffer,
       * and return the number of bytes read to valread */
      valread = read( sd , buffer, BUFFERSIZE);

      if ( valread == 0) {
        /* If valread is 0, then someone disconnected, get details and print */
        getpeername(sd , (struct sockaddr*)&address , \
            (socklen_t*)&addrlen);
        printf("Host disconnected , ip %s , port %d \n" ,
              inet_ntoa(address.sin_addr) , ntohs(address.sin_port));

        /* Close the socket, and remove from client list */
        close( sd );
        client_sockets[i] = 0;
      }

      /* Else, a message is sent from the clinet */
      else {
        /* NULL terminate buffer */
        buffer[valread] = '\0';
        printf("Message from client is: %s\n", buffer);

        nsleep(1000000000);
        send(sd , "Hello from server!" , 18 , 0 );
      }
    }
  }

  return 0;
}




void tcp_server_cleanup( int master_socket , int client_sockets[MAXCLIENTS] ) {
  int i;

  /* close master_socket */
  close( master_socket );
  /* close client sockets */
  for ( i = 0 ; i < MAXCLIENTS ; i++) {
    if ( client_sockets[i] > 0 ) {
      close( client_sockets[i] );
    }
  }

  return;
}

