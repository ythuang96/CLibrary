#ifndef CLIBARAY_H
#define CLIBARAY_H

#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <strings.h>
#include <dirent.h>
#include <time.h>
#include <errno.h>
#include <math.h>
#include <sys/resource.h>
#include <curl/curl.h>

/* TCP Comm */
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <sys/epoll.h>

#include <CLibaraySettings.h>

#define IPADDRSIZE 16


/*********************************** STRUCT ***********************************/
typedef struct string_t {
  char *ptr;
  size_t len;
} string_t;

typedef struct tcpmessage_t {
  char message[TCPBUFFERSIZE];
  char source_ip[IPADDRSIZE];
} tcpmessage_t;

typedef struct tcpmessagering_t {
  tcpmessage_t messages[TCPRINGSIZE];
  /* points to the item to be processed, which is the first one in the squence */
  tcpmessage_t *ptr_processing;
  /* points to the item for new storage, which is the first empty one (the one behind the last in the squence) */
  tcpmessage_t *ptr_new;
  /* start and end address of the entire ring, set during initialization and use for incrementing the ring pointers */
  tcpmessage_t *ptr_start;
  tcpmessage_t *ptr_end;
} tcpmessagering_t;


/****************************** GLOBAL VARIABLES ******************************/
/* pointer for error log file */
extern FILE *error_log_;

/* Code running state */
extern state_e state_;


/******************************* CLib_Strings.c *******************************/
void init_string(string_t *s);
size_t writefunc(void *ptr, size_t size, size_t nmemb, string_t *s);
int last_ip_digit( char ip_add[] );

/******************************** CLib_Time.c ********************************/
void print_time(void);
void nsleep(uint64_t ns);
int current_time(void);

/***************************** CLib_SigHandler.c *****************************/
void SigHandler(int dummy);

/********************************* CLib_TCP.c *********************************/
int tcp_server_setup( int *master_socket_ptr, int *epoll_fd_ptr, struct epoll_event events_monitored[TCPMAXCLIENTS+1] );
int tcp_server_monitor( int master_socket, int epoll_fd, struct epoll_event events_monitored[TCPMAXCLIENTS+1] );
void tcp_server_cleanup( int master_socket, int epoll_fd, struct epoll_event events_monitored[TCPMAXCLIENTS+1] );

void tcp_ring_init( tcpmessagering_t *ring_ptr );
void tcp_clear_message( tcpmessage_t *message_ptr );
void tcp_increment_ring_ptr_processing( tcpmessagering_t *ring_ptr );
void tcp_increment_ring_ptr_new( tcpmessagering_t *ring_ptr );
void tcp_process_message( tcpmessagering_t *ring_ptr, void (*processing_func_ptr)(tcpmessage_t *), void (*emptyring_func_ptr)(void) );
void tcp_add_message( tcpmessagering_t *ring_ptr, char message[TCPBUFFERSIZE], char source_ip[IPADDRSIZE]);


#endif