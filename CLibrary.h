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


/*********************************** STRUCT ***********************************/
typedef struct string_t {
  char *ptr;
  size_t len;
} string_t;


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
int tcp_server_setup( void );
int tcp_server_monitor( int master_socket, int client_sockets[MAXCLIENTS] );
void tcp_server_cleanup( int master_socket , int client_sockets[MAXCLIENTS] );


#endif