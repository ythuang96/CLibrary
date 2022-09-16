#ifndef MYSDL_H
#define MYSDL_H


int Joystick_Init( int device_num, int n_axis, int n_button_sl, int n_button_sh );
void Joystick_Init_Axis( const int *axis_name, const int *axis_min, const int *axis_max, \
  const int *axis_dz, const int* axis_inv );
void Joystick_Init_Button_SL( const int *button_name, double long_press_sec);
void Joystick_Init_Button_SH( const int *button_name, double hold_sec);
void Joystick_Display_Action( void );
void Joystick_Monitor( double *axis_value, int *button_value_sl, int *button_value_sh );
void Joystick_Cleanup( void );


#endif