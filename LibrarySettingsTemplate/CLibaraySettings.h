#ifndef CLIB_SETTING_H
#define CLIB_SETTING_H

/*
 * To use the CLibrary, create a file called "CLibrarySettings.h" with the
 * following definitions.
 * These definitions can be changed, but must exist.
 * The ENUM state_e can include mutliple different states, but the "EXITING"
 * state must be present
 */


/******************************** TCP Settings ********************************/
#define PORT 8888                   /* TCP port number                  */
#define MAXCLIENTS 16               /* Maximum number of TCP clients    */
#define BUFFERSIZE 256              /* Data buffer size for TCP message */
#define SERVERADD "192.168.1.61"    /* TCP server address               */

/* Update frequency of TCP in HZ
 * tcp_server_monitor is a blocking function and will block for 1/TCPUPDATEFREQ
 * seconds before timing out */
#define TCPUPDATEFREQ 50

/************************************ ENUM ************************************/
typedef enum state_e {
  RUNNING, EXITING
} state_e;


#endif