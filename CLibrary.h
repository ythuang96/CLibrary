#ifndef CLIBARAY_H
#define CLIBARAY_H

#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
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

#define IPADDRSIZE 16      /* Size of IP Address String        */
#define TCPBUFFERSIZE 256  /* Data buffer size for TCP message */
#define TCPRINGSIZE 8      /* Size of TCP message ring         */


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

/******************************* CLib_Strings.c *******************************/
void init_string(string_t *s);
size_t writefunc(void *ptr, size_t size, size_t nmemb, string_t *s);
int last_ip_digit( char ip_add[] );

/******************************** CLib_Time.c ********************************/
void print_time(void);
void nsleep(uint64_t ns);
int current_time(void);

/********************************* CLib_TCP.c *********************************/
void tcp_lib_init(char* server_addr, int port, double update_freq );

int tcp_server_setup( int min_client_addr, int max_client_addr );
int tcp_server_monitor( void );
void tcp_server_cleanup( void );

void tcp_server_process_message( void (*processing_func_ptr)(tcpmessage_t *), void (*emptyring_func_ptr)(void) );
void tcp_server_send_message( void );
void tcp_server_add_message_sendqueue( char message[TCPBUFFERSIZE], char destination_ip[IPADDRSIZE] );

int tcp_client_setup( void );
int tcp_client_reconnect( void );
int tcp_client_monitor( void );
void tcp_client_cleanup( void );

void tcp_client_process_message( void (*processing_func_ptr)(tcpmessage_t *), void (*emptyring_func_ptr)(void) );
void tcp_client_send_message( void );
void tcp_client_add_message_sendqueue( char message[TCPBUFFERSIZE] );


#endif