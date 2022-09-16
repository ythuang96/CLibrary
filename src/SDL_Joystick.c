#include <CLibrary.h>
#include <SDL.h>


static SDL_Joystick *joystick_ = NULL;
static SDL_Event event_;

static int n_axis_, n_button_sl_, n_button_sh_;
static double long_press_sec_, hold_sec_;

static int *axis_name_, *axis_max_, *axis_min_, *axis_dz_, *axis_inv_;
static int *button_name_sl_, *button_name_sh_;
static clock_t *button_press_time_sl_, *button_press_time_sh_;


static double axis_value_2_double( int axis_value, int axis_min, int axis_max, \
  int axis_dz, int axis_inv );




/*
 * This function initializes the joystick
 *
 * Arguments
 *   device_num:  [Input] joystick device number
 *   n_axis:      [Input] number of axis to use/monitor
 *   n_button_sl: [Input] number of buttons to use/monitor for short/long press
 *   n_button_sh: [Input] number of buttons to use/monitor for short press/hold
 * Return
 *   0 on success, -1 on failure
 */
int Joystick_Init( int device_num, int n_axis, int n_button_sl, int n_button_sh ) {
  n_axis_ = n_axis;
  n_button_sl_ = n_button_sl;
  n_button_sh_ = n_button_sh;

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
    printf("Initializing %d axis, %d buttons with short/long press, %d buttons wiht short press/hold\n", \
      n_axis_, n_button_sl_, n_button_sh_ );
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
 * Intialize joytick buttons in single/long press mode
 * Arguments
 *   button_name:      [Input, array size n_button]
 *                     the integer name for each of button to use
 *   long_press_sec:   [Input]
 *                     the number of seconds for long press, pressing longer than this will register as a long press
 * Return
 *   None
 */
void Joystick_Init_Button_SL( const int *button_name, double long_press_sec){
  int ii;


  long_press_sec_ = long_press_sec;
  printf("Long press length: %5.1f sec\n", long_press_sec_);

  /* allocate static array */
  button_name_sl_ = (int *) calloc(n_button_sl_, sizeof(int));
  /* allocate press time array */
  button_press_time_sl_ = (clock_t *) calloc(n_button_sl_, sizeof(clock_t));

  /* copy the values to the static array */
  for ( ii = 0; ii < n_button_sl_; ii++ ) {
    *(button_name_sl_ + ii) = *(button_name + ii);
  }
  return;
}


/*
 * Intialize joytick buttons in single press/hold mode
 * Arguments
 *   button_name: [Input, array size n_button]
 *                the integer name for each of button to use
 *   hold_sec:    [Input]
 *                the minimum number of seconds for hold,
 *                pressing longer than this will register as a hold
 *                pressing shorter than this will register as a single press
 * Return
 *   None
 */
void Joystick_Init_Button_SH( const int *button_name, double hold_sec){
  int ii;


  hold_sec_ = hold_sec;
  printf("Hold button length at least: %5.1f sec\n", hold_sec_);

  /* allocate static array */
  button_name_sh_ = (int *) calloc(n_button_sh_, sizeof(int));
  /* allocate press time array */
  button_press_time_sh_ = (clock_t *) calloc(n_button_sh_, sizeof(clock_t));

  /* copy the values to the static array */
  for ( ii = 0; ii < n_button_sh_; ii++ ) {
    *(button_name_sh_ + ii) = *(button_name + ii);
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
    /* If joystick button release */
    else if ( event_.type == SDL_JOYBUTTONUP ) {
      printf("Button released on number: %d\n", event_.jbutton.button );
    }
  }
  return;
}


/*
 * This function monitors the motion of the joystick
 * This function should be run in a while loop checking for the EXITING state
 *
 * Arguments
 *   axis_value:      [Input/Output, array size n_axis]
 *                    An array that stores the position value for each axis,
 *                    ranging from -1 to 1
 *                    This array gets updated every time there is axis motion
 *   button_value_sl: [Input/Output, array size n_button_sl]
 *                    An array that stores the condition for each button that is
 *                    in short/long press mode.
 *                    0 for no action, 1 for short press and 2 for long press
 *                    This array gets updated every time there is button motion,
 *                    and the motion gets registered when the button is released
 *   button_value_sh: [Input/Output, array size n_button_sh]
 *                    An array that stores the condition for each button that is
 *                    in short press/hold mode.
 *                    0 for no action, -1 for pressed down, 1 for short press and
 *                    2 for currently being held down
 *                    This array gets updated every time there is button motion.
 *                    When pressed down, value first changes to -1. If button is
 *                    released within hold_sec_, then the value changes to
 *                    1 indicating a short press. If hold_sec_ expired and
 *                    button is still not released, value changes to 2, indicating
 *                    the button is being held down. When the held down button is
 *                    released, the value changes to 0.
 * Return
 *   None
 */
void Joystick_Monitor( double *axis_value, int *button_value_sl, int *button_value_sh ) {
  int ii;
  double elapsedtime;

  /* loop over each of the initialized buttons for short press/hold mode
   * to check if the hold_sec_ has expired */
  for (ii = 0; ii < n_button_sh_; ii++) {

    /* if the button is pressed down but has not gone into hold state */
    if ( *(button_value_sh + ii) == -1 ) {
      /* compute the elaspsed time */
      elapsedtime = (double)(clock() - *(button_press_time_sh_ + ii)) / CLOCKS_PER_SEC;
      /* if hold_sec_ expired */
      if (elapsedtime >= hold_sec_) {
        /* register this button as being held down */
        *(button_value_sh + ii) = 2;
      }
    }
  }


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

      /* loop over each of the initialized buttons for short/long press mode */
      for (ii = 0; ii < n_button_sl_; ii++) {
        /* if the button press is the current button */
        if ( event_.jbutton.button == *(button_name_sl_ + ii) ){
          /* record press time */
          *(button_press_time_sl_ + ii) = clock();
        }
      }

      /* loop over each of the initialized buttons for short press/hold mode */
      for (ii = 0; ii < n_button_sh_; ii++) {
        /* if the button press is the current button */
        if ( event_.jbutton.button == *(button_name_sh_ + ii) ){
          /* record press time */
          *(button_press_time_sh_ + ii) = clock();
          /* change state to -1  to indicate a press down */
          *(button_value_sh + ii) = -1;
        }
      }

    }

    /* If joystick button release */
    else if ( event_.type == SDL_JOYBUTTONUP ) {

      /* loop over each of the initialized buttons for short/long press mode */
      for (ii = 0; ii < n_button_sl_; ii++) {
        /* if the button released is the current button */
        if ( event_.jbutton.button == *(button_name_sl_ + ii) ){
          /* compute the elaspsed time */
          elapsedtime = (double)(clock() - *(button_press_time_sl_ + ii)) / CLOCKS_PER_SEC;

          if ( elapsedtime >= long_press_sec_ ) {
            /* register this button as a long press */
            *(button_value_sl + ii) = 2;
          }
          else if ( elapsedtime >= 0 ) {
            /* register this button as a short press */
            *(button_value_sl + ii) = 1;
          }
        }
      }

      /* loop over each of the initialized buttons for short press/hold mode */
      for (ii = 0; ii < n_button_sh_; ii++) {
        /* if the button released is the current button */
        if ( event_.jbutton.button == *(button_name_sh_ + ii) ){

          /* if the button has not gone into hold state, it is a short press */
          if ( *(button_value_sh + ii) == -1 ) {
            /* register this button as a short press */
            *(button_value_sh + ii) = 1;
          }
          /* if the button already gone into hold state, it is released from hold */
          else if ( *(button_value_sh + ii) == 2 ) {
            /* register this button as a released from a hold */
            *(button_value_sh + ii) = 0;
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
  free(button_name_sl_);
  free(button_press_time_sl_);
  free(button_name_sh_);
  free(button_press_time_sh_);

  /* Close joystick if opened */
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