/* Data Sheet: https://www.nxp.com/docs/en/data-sheet/PCA9685.pdf */

#include <wiringPiI2C.h>
#include <CLibrary.h>

/* REGISTER ADDRESSES */
#define PCA9685_MODE1      0x00 /* Mode Register 1              */
#define PCA9685_MODE2      0x01 /* Mode Register 2              */
#define PCA9685_SUBADR1    0x02 /* I2C-bus subaddress 1         */
#define PCA9685_SUBADR2    0x03 /* I2C-bus subaddress 2         */
#define PCA9685_SUBADR3    0x04 /* I2C-bus subaddress 3         */
#define PCA9685_ALLCALLADR 0x05 /* LED All Call I2C-bus address */
#define PCA9685_LED0_ON_L  0x06 /* LED0 on tick, low byte       */
#define PCA9685_LED0_ON_H  0x07 /* LED0 on tick, high byte      */
#define PCA9685_LED0_OFF_L 0x08 /* LED0 off tick, low byte      */
#define PCA9685_LED0_OFF_H 0x09 /* LED0 off tick, high byte     */

/* etc all 16:  LED15_OFF_H 0x45 */
#define PCA9685_ALLLED_ON_L  0xFA /* load all the LEDn_ON registers, low  */
#define PCA9685_ALLLED_ON_H  0xFB /* load all the LEDn_ON registers, high */
#define PCA9685_ALLLED_OFF_L 0xFC /* load all the LEDn_OFF registers, low */
#define PCA9685_ALLLED_OFF_H 0xFD /* load all the LEDn_OFF registers,high */
#define PCA9685_PRESCALE     0xFE /* Prescaler for PWM output frequency   */
#define PCA9685_TESTMODE     0xFF /* defines the test mode to be entered  */

/* MODE1 bits */
#define PCA9685_MODE1_ALLCAL  0x01 /* respond to LED All Call I2C-bus address */
#define PCA9685_MODE1_SUB3    0x02 /* respond to I2C-bus subaddress 3         */
#define PCA9685_MODE1_SUB2    0x04 /* respond to I2C-bus subaddress 2         */
#define PCA9685_MODE1_SUB1    0x08 /* respond to I2C-bus subaddress 1         */
#define PCA9685_MODE1_SLEEP   0x10 /* Low power mode. Oscillator off          */
#define PCA9685_MODE1_AI      0x20 /* Auto-Increment enabled                  */
#define PCA9685_MODE1_EXTCLK  0x40 /* Use EXTCLK pin clock                    */
#define PCA9685_MODE1_RESTART 0x80 /* Restart enabled                         */
/* MODE2 bits */
#define PCA9685_MODE2_OUTNE_0 0x01 /* Active LOW output enable input                   */
#define PCA9685_MODE2_OUTNE_1 0x02 /* Active LOW output enable input - high impedience */
#define PCA9685_MODE2_OUTDRV  0x04 /* totem pole structure vs open-drain               */
#define PCA9685_MODE2_OCH     0x08 /* Outputs change on ACK vs STOP                    */
#define PCA9685_MODE2_INVRT   0x10 /* Output logic state inverted                      */

#define PCA9685_I2C_ADDRESS 0x40      /* Default PCA9685 I2C Slave Address */
#define PCA9685_FREQUENCY_OSCILLATOR 25000000 /* Int. osc. frequency in datasheet */

#define PCA9685_PRESCALE_MIN 3   /* minimum prescale value */
#define PCA9685_PRESCALE_MAX 255 /* maximum prescale value */


/**************** Static Global Variables and Static Function ****************/
static int i2c_fd_;

static void PCA9685_setPWMFreq( double freq);
static void PCA9685_setOutputMode( bool totempole);
static void PCA9685_setPWM( uint8_t PinNum, uint16_t on, uint16_t off);


/*
 * This function initializes PCA9685
 *
 * Arguments:
 *   i2c_addr:  [Input] the I2C address for ADS 1115, default should be 0x48
 *   freq:      [Input] PWM fequency, should be between 23 and 1600 Hz
 *   totempole: [Input] Totempole if true, open drain if false.
 *                      LEDs with integrated zener diodes should only be driven in open drain mode.
 *
 * Return:
 *   0 on success or -1 on failure
 */
int PCA9685_Init( int i2c_addr, double freq, bool totempole ) {
  i2c_fd_ = wiringPiI2CSetup(i2c_addr);
  if (i2c_fd_ == -1) {
    printf("I2C Initialization failed. Most likely you are not root\n");
    printf("Please remember to run as root.\n");
    print_time();
    fprintf(error_log_, "I2C Initialization failed. Most likely you are not root\n");
    fprintf(error_log_, "Please remember to run as root.\n");
    fflush(error_log_);
    return -1 ;
  }
  PCA9685_setPWMFreq( freq );
  PCA9685_setOutputMode( totempole );
  return 0;
}


/*
 * Set the PWM frequency of the chip
 * Arguments
 *   freq: [Input] PWM fequency, should be between 23 and 1600 Hz
 * Return None
 */
void PCA9685_setPWMFreq( double freq) {
  int prescaleval;
  uint8_t prescale, mode;


  /* Disable all output first for an orderly shutdown, by writting a logic 1 to bit 4 in register ALL_LED_OFF_H */
  wiringPiI2CWriteReg8(i2c_fd_, PCA9685_ALLLED_OFF_H, 0x10);

  /* Limit the frequency between 23 and 1600 Hz for the internal 25MHz oscillator */
  if (freq < 23) freq = 23;
  if (freq > 1600) freq = 1600;

  /* Compute prescale from data sheet equation 1 */
  prescaleval = round((PCA9685_FREQUENCY_OSCILLATOR / (freq * 4096.0))) - 1;

  /* Limit the prescale value to the datasheet upper and lower limit */
  if (prescaleval < PCA9685_PRESCALE_MIN) prescaleval = PCA9685_PRESCALE_MIN;
  if (prescaleval > PCA9685_PRESCALE_MAX) prescaleval = PCA9685_PRESCALE_MAX;
  prescale = (uint8_t) prescaleval;

  /* Set the restart, external clock, SUB1, SUB2, SUB3, ALLCALL to 0
   * set the sleep and auto increment to 1 */
  mode = PCA9685_MODE1_SLEEP | PCA9685_MODE1_AI;
  /* Put the board to sleep */
  wiringPiI2CWriteReg8(i2c_fd_, PCA9685_MODE1, mode);
  /* Set the prescalar */
  wiringPiI2CWriteReg8(i2c_fd_, PCA9685_PRESCALE, prescale);
  /* Set the sleep bit to 0 */
  mode = (mode & ~PCA9685_MODE1_SLEEP);
  /* Wake up the board */
  wiringPiI2CWriteReg8(i2c_fd_, PCA9685_MODE1, mode);
  /* Wait at least 500 us before restart, as required by datasheet */
  nsleep(5000000);

  /* Set the restart bit to 1  */
  mode = (mode | PCA9685_MODE1_RESTART);
  /* Restart */
  wiringPiI2CWriteReg8(i2c_fd_, PCA9685_MODE1, mode);

  return;
}


/*
 * Sets the output mode of the PCA9685 to either open drain or push pull / totempole.
 * Warning: LEDs with integrated zener diodes should only be driven in open drain mode.
 * Parameter:
 *   totempole: [Input] Totempole if true, open drain if false.
 * Return: None
 */
void PCA9685_setOutputMode( bool totempole) {
  uint8_t oldmode, newmode;

  oldmode = wiringPiI2CReadReg8(i2c_fd_, PCA9685_MODE2);
  if (totempole) {
    newmode = oldmode | PCA9685_MODE2_OUTDRV;
  } else {
    newmode = oldmode & ~PCA9685_MODE2_OUTDRV;
  }
  wiringPiI2CWriteReg8(i2c_fd_, PCA9685_MODE2, newmode);
  return;
}


/*
 * Sets the PWM output of one of the PCA9685 pins
 * Parameter:
 *   PinNum: [Input] PWM pin number, from 0 to 15
 *   on:     [Input] At what point in the 4096-part cycle to turn the PWM output ON
 *   off:    [Input] At what point in the 4096-part cycle to turn the PWM output OFF
 * Return: None
 */
void PCA9685_setPWM( uint8_t PinNum, uint16_t on, uint16_t off) {
  wiringPiI2CWriteReg16(i2c_fd_, PCA9685_LED0_ON_L + 4 * PinNum, on);
  wiringPiI2CWriteReg16(i2c_fd_, PCA9685_LED0_ON_L + 4 * PinNum + 2, off);
  return;
}


/*
 * Set pin PWM output. Sets pin without having to deal with on/off tick
 * placement and properly handles a zero value as completely off and 4095 as
 * completely on.

 * Parameter:
 *   PinNum:    [Input] PWM pin number, from 0 to 15
 *   PWMval:    [Input] The number of ticks out of 4096 to be active, should be
 *                      a value from 0 to 4095 inclusive.
 * Return: None
 */
void PCA9685_setPinPWM( uint8_t PinNum, uint16_t PWMval) {
  /* Clamp value between 0 and 4095 inclusive. */
  if (PWMval > 4095) PWMval = 4095;

  if (PWMval == 4095) {
    /* Special value for signal fully on. */
    PCA9685_setPWM( PinNum, 4096, 0);
  } else if (PWMval == 0) {
    /* Special value for signal fully off. */
    PCA9685_setPWM( PinNum, 0, 4096);
  } else {
    PCA9685_setPWM( PinNum, 0, PWMval);
  }
  return;
}


void PCA9685_cleanup( void ) {
  int ii;

  /* set all pins to 0 */
  for ( ii = 0; ii < 16; ii ++ ) {
    PCA9685_setPinPWM( ii, 0);
  }

  return;
}
