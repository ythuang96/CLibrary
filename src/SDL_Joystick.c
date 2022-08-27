#include <CLibrary.h>
#include <SDL.h>


static SDL_Joystick *joystick_ = NULL;
static SDL_Event event_;

static int n_axis_, n_button_;
static double long_press_sec_;

static int *axis_name_, *axis_max_, *axis_min_, *axis_dz_, *axis_inv_;
static int *button_name_;
static clock_t *button_press_time_;


static double axis_value_2_double( int axis_value, int axis_min, int axis_max, \
  int axis_dz, int axis_inv );




/*
 * This function initializes the joystick
 *
 * Arguments
 *   device_num: [Input] joystick device number
 *   n_axis:     [Input] number of axis to use/monitor
 *   n_button:   [Input] number of buttons to use/monitor
 * Return
 *   0 on success, -1 on failure
 */
int Joystick_Init( int device_num, int n_axis, int n_button ) {
  n_axis_ = n_axis;
  n_button_ = n_button;

  /* Initialize SDL */
  SDL_Init( SDL_INIT_JOYSTICK );

  /* Check if joystick is connected */
  if (SDL_NumJoysticks() == 0) {
    print_time();
    fprintf(error_log_, "No joystick found. Exiting.\n");
    return -1;
  }

  /* Open joystick */
  joystick_ = SDL_JoystickOpen( device_num );
  /* Display joystick information */
  if (joystick_) {
    printf("Joystick opened: %s\n", SDL_JoystickName(joystick_));
    printf("Total number of    axes: %d\n", SDL_JoystickNumAxes(joystick_));
    printf("Total number of buttons: %d\n\n", SDL_JoystickNumButtons(joystick_));
    printf("Initializing %d axis, %d buttons\n", n_axis_, n_button_ );
  }
  else {
    print_time();
    fprintf(error_log_, "Could not open joystick %d.\n", device_num);
    return -1;
  }

  return 0;
}


/*
 * Intialize joytick axes
 * Arguments
 *   axis_name: [Input, array size n_axis]
 *              the integer name for each of axes to use
 *   axis_min:  [Input, array size n_axis]
 *              the minimum value for each axis, should be in the same order axis_name
 *   axis_max:  [Input, array size n_axis]
 *              the maximum value for each axis, should be in the same order axis_name
 *   axis_dz:   [Input, array size n_axis]
 *              the deadzone value for each axis, should be in the same order axis_name
 *   axis_inv:  [Input, array size n_axis]
 *              the inversion value for each axis, should be in the same order axis_name
 *              1 for no inversion and -1 for invert axis
 * Return
 *   None
 */
void Joystick_Init_Axis( const int *axis_name, const int *axis_min, const int *axis_max, \
  const int *axis_dz, const int* axis_inv){
  int ii;

  /* allocate static array */
  axis_name_ = (int *) calloc(n_axis_, sizeof(int));
  axis_max_  = (int *) calloc(n_axis_, sizeof(int));
  axis_min_  = (int *) calloc(n_axis_, sizeof(int));
  axis_dz_   = (int *) calloc(n_axis_, sizeof(int));
  axis_inv_  = (int *) calloc(n_axis_, sizeof(int));

  /* copy the values to the static arrays */
  for ( ii = 0; ii < n_axis_; ii++ ) {
    *(axis_name_ + ii) = *(axis_name + ii);
    *(axis_max_  + ii) = *(axis_max  + ii);
    *(axis_min_  + ii) = *(axis_min  + ii);
    *(axis_dz_   + ii) = *(axis_dz   + ii);
    *(axis_inv_  + ii) = *(axis_inv  + ii);
  }

  return;
}


/*
 * Intialize joytick buttons
 * Arguments
 *   button_name:      [Input, array size n_button]
 *                     the integer name for each of button to use
 *   long_press_sec:   [Input]
 *                     the number of seconds for long press, pressing longer than this will register as a long press
 * Return
 *   None
 */
void Joystick_Init_Button( const int *button_name, double long_press_sec){
  int ii;


  long_press_sec_ = long_press_sec;
  printf("Long press length: %5.1f sec\n", long_press_sec_);

  /* allocate static array */
  button_name_ = (int *) calloc(n_button_, sizeof(int));
  /* allocate press time array */
  button_press_time_ = (clock_t *) calloc(n_button_, sizeof(clock_t));

  /* copy the values to the static array */
  for ( ii = 0; ii < n_button_; ii++ ) {
    *(button_name_ + ii) = *(button_name + ii);
  }
  return;
}


/*
 * This function displays the joystick motion
 * including which axis is moved and its raw values, and which button is pressed
 * This function should be run in a while loop checking for the EXITING state
 *
 * This function is useful for ditermining the integer name and range of the
 * axis and button
 */
void Joystick_Display_Action( void ) {
  /* Check if there are any events, SDL_PollEvent return 1 if there are events, 0 if no events */
  if ( SDL_PollEvent(&event_) != 0) {

    /* If joystick axis motion */
    if ( event_.type == SDL_JOYAXISMOTION ) {
      printf("Motion on axes number: %d, with raw value: %d\n", event_.jaxis.axis, event_.jaxis.value );
    }
    /* If joystick button press down */
    else if ( event_.type == SDL_JOYBUTTONDOWN) {
      printf("Button pressed on number: %d\n", event_.jbutton.button );
    }

  }
  return;
}


/*
 * This function monitors the motion of the joystick
 * This function should be run in a while loop checking for the EXITING state
 *
 * Arguments
 *   axis_value:   [Input/Output, array size n_axis]
 *                 An array that stores the position value for each axis,
 *                 ranging from -1 to 1
 *                 This array gets updated every time there is axis motion
 *   button_value: [Input/Output, array size n_button]
 *                 An array that stores the condition for each button,
 *                 0 for no action, 1 for sigle press and 2 for long press
 *                 This array gets updated every time there is button motion
 * Return
 *   None
 */
void Joystick_Monitor( double *axis_value, int *button_value ) {
  int ii;
  double elapsedtime;


  /* Check if there are any events, SDL_PollEvent return 1 if there are events, 0 if no events */
  if ( SDL_PollEvent(&event_) != 0) {

    /* If joystick axis motion */
    if ( event_.type == SDL_JOYAXISMOTION ) {
      /* loop over each of the initialized axis */
      for (ii = 0; ii < n_axis_; ii++) {
        /* if the motion is on the current axis */
        if ( event_.jaxis.axis == *(axis_name_ + ii) ) {
          /* record the motion value */
          *(axis_value + ii) = \
            axis_value_2_double( event_.jaxis.value, \
            *(axis_min_ + ii), *(axis_max_ + ii), *(axis_dz_ + ii), *(axis_inv_ + ii) );
        }
      }
    }

    /* If joystick button press down */
    else if ( event_.type == SDL_JOYBUTTONDOWN) {
      /* loop over each of the initialized buttons */
      for (ii = 0; ii < n_button_; ii++) {
        /* if the button press is the current button */
        if ( event_.jbutton.button == *(button_name_ + ii) ){
          /* record press time */
          *(button_press_time_ + ii) = clock();
        }
      }
    }

    /* If joystick button release */
    else if ( event_.type == SDL_JOYBUTTONUP ) {
      /* loop over each of the initialized buttons */
      for (ii = 0; ii < n_button_; ii++) {
        /* if the button released is the current button */
        if ( event_.jbutton.button == *(button_name_ + ii) ){
          /* compute the elaspsed time */
          elapsedtime = (double)(clock() - *(button_press_time_ + ii)) / CLOCKS_PER_SEC;

          if ( elapsedtime >= long_press_sec_ ) {
            /* register this button as a long press */
            *(button_value + ii) = 2;
          }
          else if ( elapsedtime >= 0 ) {
            /* register this button as a short press */
            *(button_value + ii) = 1;
          }
        }
      }
    }

  } /* end if event */
  return;
}


/*
 * Cleanup: release memory, close joystick and exit SDL
 * Arguments: None
 * Return: None
 */
void Joystick_Cleanup( void ) {
  /* Free dynamically allocated arrays */
  free(axis_name_  );
  free(axis_max_   );
  free(axis_min_   );
  free(axis_dz_    );
  free(axis_inv_   );
  free(button_name_);
  free(button_press_time_);

  /* Close joystick if oopened */
  if (SDL_JoystickGetAttached(joystick_)) {
    SDL_JoystickClose(joystick_);
  }
  /* Exit SDL */
  SDL_Quit();
}


/*
 * Convert the axis value to a double number between -1 and 1
 *
 * Arguments
 *   axis_value: [Input] integer axis value from the joystick
 *   axis_min  : [Input] minimum integer axis value from the joystick
 *   axis_max  : [Input] maximum integer axis value from the joystick
 *   axix_dz   : [Input] integer axis value for the deadzone
 *   axix_inv  : [Input] integer axis value for the inversion, +1 for no inversion and -1 for invert axis
 * Return
 *   double number between -1 and 1
 */
double axis_value_2_double( int axis_value, int axis_min, int axis_max, int axis_dz, int axis_inv ) {
  double result;

  /* Within dead zone */
  if ( (axis_value <= axis_dz) && (axis_value >= -axis_dz)) {
    result = 0.0;
  }

  /* Positive values */
  else if ( axis_value > axis_dz ) {
    if (axis_value >= axis_max) {
      result = 1.0;
    }
    else {
      result = ((double) (axis_value - axis_dz))/((double) (axis_max - axis_dz));
    }
  }

  /* Negative values */
  else if ( axis_value < -axis_dz ) {
    if (axis_value <= axis_min) {
      result = -1.0;
    }
    else {
      result = - ((double) (axis_value + axis_dz))/((double) (axis_min + axis_dz));
    }
  }

  return ((double) axis_inv)*result;
}