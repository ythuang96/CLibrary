#ifndef MYSDL_H
#define MYSDL_H


int Joystick_Init( int device_num, int n_axis, int n_button );
void Joystick_Init_Axis( const int *axis_name, const int *axis_min, const int *axis_max, const int *axis_dz );
void Joystick_Init_Button( const int *button_name, double long_press_sec);
void Joystick_Display_Action( void );
void Joystick_Monitor( double *axis_value, int *button_value );
void Joystick_Cleanup( void );


#endif