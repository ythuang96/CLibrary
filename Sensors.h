#ifndef SENSORS_H
#define SENSORS_H


/***************************** Sensors_ADS1115.c *****************************/
uint16_t ADS1115_SingleEnded_Config( double VRange, int DateRate );
double ADS1115_SingleEnded_Read( int FD, uint16_t config, int channel );

/***************************** Sensors_PCA9685.c *****************************/
void PCA9685_setPWMFreq(int FD, double freq);
void PCA9685_setOutputMode(int FD, bool totempole);
void PCA9685_setPWM(int FD, uint8_t PinNum, uint16_t on, uint16_t off);
void PCA9685_setPinPWM(int FD, uint8_t PinNum, uint16_t PWMval);


#endif