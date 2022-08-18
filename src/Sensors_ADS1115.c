/* Data Sheet: https://www.ti.com/lit/ds/symlink/ads1114.pdf?ts=1609357468599&ref_url=https%253A%252F%252Fwww.google.com%252F */

#include <wiringPiI2C.h>
#include <CLibrary.h>


/*=========================================================================
  POINTER REGISTER
  ADS111x have 4 registers that are accessible through the I2C interface using
  the Address Pointer register.
  -----------------------------------------------------------------------*/
#define ADS1115_REG_POINTER_CONVERT   (0x00)   /* Conversion */
#define ADS1115_REG_POINTER_CONFIG    (0x01)    /* Configuration */
#define ADS1115_REG_POINTER_LOWTHRESH (0x02) /* Low threshold */
#define ADS1115_REG_POINTER_HITHRESH  (0x03)  /* High threshold */
/*=========================================================================*/

/*=========================================================================
  CONFIG REGISTER

  config & ADS1115_OS_MASK will return only the OS settings, this applies to all
  other settings
  -----------------------------------------------------------------------*/
/* Bit 15 (first bit): Operational status or single-shot conversion start */
#define ADS1115_OS_MASK    (0x8000) /* OS Mask */
#define ADS1115_OS_SINGLE  (0x8000) /* Write: Set to start a single-conversion */
#define ADS1115_OS_BUSY    (0x0000) /* Read: Bit = 0 when conversion is in progress */
#define ADS1115_OS_NOTBUSY (0x8000) /* Read: Bit = 1 when device is not performing a conversion */

/* Bit 14:12: Input multiplexer configuration */
#define ADS1115_MUX_MASK     (0x7000) /* Mux Mask */
#define ADS1115_MUX_DIFF_0_1 (0x0000) /* Differential P = AIN0, N = AIN1 (default) */
#define ADS1115_MUX_DIFF_0_3 (0x1000) /* Differential P = AIN0, N = AIN3 */
#define ADS1115_MUX_DIFF_1_3 (0x2000) /* Differential P = AIN1, N = AIN3 */
#define ADS1115_MUX_DIFF_2_3 (0x3000) /* Differential P = AIN2, N = AIN3 */
#define ADS1115_MUX_SINGLE_0 (0x4000) /* Single-ended AIN0 */
#define ADS1115_MUX_SINGLE_1 (0x5000) /* Single-ended AIN1 */
#define ADS1115_MUX_SINGLE_2 (0x6000) /* Single-ended AIN2 */
#define ADS1115_MUX_SINGLE_3 (0x7000) /* Single-ended AIN3 */

/* Bit 11:9: Programmable gain amplifier configuration */
#define ADS1115_PGA_MASK   (0x0E00) /* PGA Mask */
#define ADS1115_PGA_6_144V (0x0000) /* +/-6.144V */
#define ADS1115_PGA_4_096V (0x0200) /* +/-4.096V */
#define ADS1115_PGA_2_048V (0x0400) /* +/-2.048V (default) */
#define ADS1115_PGA_1_024V (0x0600) /* +/-1.024V */
#define ADS1115_PGA_0_512V (0x0800) /* +/-0.512V */
#define ADS1115_PGA_0_256V (0x0A00) /* +/-0.256V */

/* Bit 8: Device operating mode */
#define ADS1115_MODE_MASK   (0x0100) /* Mode Mask */
#define ADS1115_MODE_CONTIN (0x0000) /* Continuous conversion mode */
#define ADS1115_MODE_SINGLE (0x0100) /* Power-down single-shot mode (default) */

/* Bit 7:5: Data rates */
#define ADS1115_RATE_MASK   (0x00E0) /* Data Rate Mask */
#define ADS1115_RATE_8SPS   (0x0000) /*   8 samples per second */
#define ADS1115_RATE_16SPS  (0x0020) /*  16 samples per second */
#define ADS1115_RATE_32SPS  (0x0040) /*  32 samples per second */
#define ADS1115_RATE_64SPS  (0x0060) /*  64 samples per second */
#define ADS1115_RATE_128SPS (0x0080) /* 128 samples per second (default) */
#define ADS1115_RATE_250SPS (0x00A0) /* 250 samples per second */
#define ADS1115_RATE_475SPS (0x00C0) /* 475 samples per second */
#define ADS1115_RATE_860SPS (0x00E0) /* 860 samples per second */

/* Bit 4: Comparator mode */
#define ADS1115_CMODE_MASK   (0x0010) /* CMode Mask */
#define ADS1115_CMODE_TRAD   (0x0000) /* Traditional comparator with hysteresis (default) */
#define ADS1115_CMODE_WINDOW (0x0010) /* Window comparator */

/* Bit 3: Comparator polarity */
#define ADS1115_CPOL_MASK    (0x0008) /* CPol Mask */
#define ADS1115_CPOL_ACTVLOW (0x0000) /* ALERT/RDY pin is low when active (default) */
#define ADS1115_CPOL_ACTVHI  (0x0008) /* ALERT/RDY pin is high when active */

/* Bit 2: Latching comparator */
#define ADS1115_CLAT_MASK   (0x0004) /* CLatch Mask */
#define ADS1115_CLAT_NONLAT (0x0000) /* Non-latching comparator (default) */
#define ADS1115_CLAT_LATCH  (0x0004) /* Latching comparator */

/* Bit 1:0: Comparator queue and disable */
#define ADS1115_CQUE_MASK  (0x0003) /* CQue Mask */
#define ADS1115_CQUE_1CONV (0x0000) /* Assert ALERT/RDY after one conversions */
#define ADS1115_CQUE_2CONV (0x0001) /* Assert ALERT/RDY after two conversions */
#define ADS1115_CQUE_4CONV (0x0002) /* Assert ALERT/RDY after four conversions */
#define ADS1115_CQUE_NONE  (0x0003) /* Disable the comparator and put ALERT/RDY in high state (default) */
/*=========================================================================*/


/*
 * This function generates a configuration integer for ADS 1115 in a
 * single-ended read mode (not differential voltage measurement)
 *
 * Arguments:
 *   VRange:   [Input] the voltage range, can be 0.256, 0.512, 1.024, 2.048, 4.096, or 6.144
 *                     Input voltage on the analoge pins should not exceed Vcc regardless of this setting
 *   DataRate: [Input] the data rate, can be 8, 16, 32, 64, 128, 250, 475 or 860
 *
 * Return:
 *   configuration integer on success or -1 on failure
 */
uint16_t ADS1115_SingleEnded_Config( double VRange, int DateRate ) {
  uint16_t  config, m_gain, m_dataRate;


  /* Set the voltage range */
  if (VRange == 6.144) {
    m_gain = ADS1115_PGA_6_144V;
  }
  else if (VRange == 4.096) {
    m_gain = ADS1115_PGA_4_096V;
  }
  else if (VRange == 2.048) {
    m_gain = ADS1115_PGA_2_048V;
  }
  else if (VRange == 1.024) {
    m_gain = ADS1115_PGA_1_024V;
  }
  else if (VRange == 0.512) {
    m_gain = ADS1115_PGA_0_512V;
  }
  else if (VRange == 0.256) {
    m_gain = ADS1115_PGA_0_256V;
  }
  else {
    return -1;
  }

  /* set the data rate */
  switch (DateRate) {
  case 8:
    m_dataRate = ADS1115_RATE_8SPS;
    break;
  case 16:
    m_dataRate = ADS1115_RATE_16SPS;
    break;
  case 32:
    m_dataRate = ADS1115_RATE_32SPS;
    break;
  case 64:
    m_dataRate = ADS1115_RATE_64SPS;
    break;
  case 128:
    m_dataRate = ADS1115_RATE_128SPS;
    break;
  case 250:
    m_dataRate = ADS1115_RATE_250SPS;
    break;
  case 475:
    m_dataRate = ADS1115_RATE_475SPS;
    break;
  case 860:
    m_dataRate = ADS1115_RATE_860SPS;
    break;
  default:
    return -1;
    break;
  }

  /* generate a configuration int for single-ended operation */
  config =
    ADS1115_CQUE_NONE    | /* Disable the comparator and put ALERT/RDY in high state (default) */
    ADS1115_CLAT_NONLAT  | /* Non-latching comparator                                (default) */
    ADS1115_CPOL_ACTVLOW | /* ALERT/RDY pin is low when active                       (default) */
    ADS1115_CMODE_TRAD   | /* Traditional comparator with hysteresis                 (default) */
    ADS1115_MODE_SINGLE  | /* Power-down single-shot mode                            (default) */
    m_gain               | /* Set PGA/voltage range                                            */
    m_dataRate           | /* Set data rate                                                    */
    0x0000               | /* Do not set channel information yet                               */
    ADS1115_OS_SINGLE;     /* Set 'start single-conversion' bit                                */

  return config;
}


/*
 * This function read an analog input pin and return the voltage
 *
 * Arguments
 *   FD:      [Input] file descriptor for wiringPiI2CSetup, used for I2C read and write operations
 *   config:  [Input] ADS1115 configuration, generated by ADS1115_SingleEnded_Config
 *   channel: [Input] 0-3 for the channel to read
 *
 * Return
 *   voltage read from the channel, or 0 on failure
 */
double ADS1115_SingleEnded_Read( int FD, uint16_t config, int channel ) {
  uint16_t counts;
  double fsRange;


  /* set the channel */
  switch (channel) {
  case 0:
    config |= ADS1115_MUX_SINGLE_0;
    break;
  case 1:
    config |= ADS1115_MUX_SINGLE_1;
    break;
  case 2:
    config |= ADS1115_MUX_SINGLE_2;
    break;
  case 3:
    config |= ADS1115_MUX_SINGLE_3;
    break;
  default:
    return 0;
  }

  /* byte swap is needed as ADS1115 read and write starts with the Most Significant Bit */
  config = (config>>8) | ((config<<8)&0xffff);
  /* Write config to the config register of the ADC */
  wiringPiI2CWriteReg16(FD, ADS1115_REG_POINTER_CONFIG, config);

  /* Wait for the conversion to complete
   * First read the configuration register, mask with ADS1115_OS_MASK to get only the OS value,
   * then compare with ADS1115_OS_BUSY to see if it is still doing the conversion */
  while ( ((wiringPiI2CReadReg16(FD, ADS1115_REG_POINTER_CONFIG) & ADS1115_OS_MASK ) == ADS1115_OS_BUSY) ) {
    nsleep(1000);
  }

  /* Read the conversion results */
  counts = wiringPiI2CReadReg16( FD, ADS1115_REG_POINTER_CONVERT);
  /* byte swap is needed as ADS1115 read and write starts with the Most Significant Bit */
  counts = (counts>>8) | ((counts<<8)&0xffff);

  /* Compute the fsRange from config */
  switch (ADS1115_PGA_MASK & config) {
  case ADS1115_PGA_6_144V:
    fsRange = 6.144;
    break;
  case ADS1115_PGA_4_096V:
    fsRange = 4.096;
    break;
  case ADS1115_PGA_2_048V:
    fsRange = 2.048;
    break;
  case ADS1115_PGA_1_024V:
    fsRange = 1.024;
    break;
  case ADS1115_PGA_0_512V:
    fsRange = 0.512;
    break;
  case ADS1115_PGA_0_256V:
    fsRange = 0.256;
    break;
  default:
    fsRange = 0.0;
  }

  /* Compute and return the voltage */
  return ( counts * (fsRange / 32768.0) );
}











