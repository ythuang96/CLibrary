#ifndef SENSORS_H
#define SENSORS_H

/****************************** GLOBAL VARIABLES ******************************/
/* pointer for error log file */
extern FILE *error_log_;

/***************************** Sensors_ADS1115.c *****************************/
int ADS1115_Init( int i2c_addr, double VRange, int DateRate );
double ADS1115_SingleEnded_Read( int channel );

/***************************** Sensors_PCA9685.c *****************************/
int PCA9685_Init( int i2c_addr, double freq, bool totempole );
void PCA9685_setPinPWM( uint8_t PinNum, uint16_t PWMval);
void PCA9685_cleanup( void );


#endif