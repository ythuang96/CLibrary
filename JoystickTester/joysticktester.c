#include <CLibrary.h>
#include <MySDL.h>

typedef enum state_e {
  RUNNING, EXITING
} state_e;

void SigHandler(int dummy);


/* pointer for error log file */
FILE *error_log_;

/* state */
state_e state_;


int main( void ) {
  /* Set state */
  state_ = RUNNING;
  /* Open error log file */
  error_log_ = stdout;

  /* Setup signal handler for SIGINT */
  signal(SIGINT, SigHandler);

  /* Initialize Joystick */
  if ( Joystick_Init( 0, 0, 0, 0 ) < 0) return -1;

  /* Loop until state changes to EXITING */
  while ( state_ != EXITING ) {
    Joystick_Display_Action(  );
  }

  /* Clean up joystick */
  Joystick_Cleanup( );

  return 0;
}


void SigHandler(int dummy) {
  state_ = EXITING;
  printf("\n");
  return;
}



