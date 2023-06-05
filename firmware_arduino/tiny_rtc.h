#ifndef __TINY_RTC_H__
#define __TINY_RTC_H__

//settings for the SoftI2CMaster library, see https://github.com/felias-fogg/SoftI2CMaster
#define I2C_HARDWARE 1      
#define SDA_PORT PORTD
#define SDA_PIN 1
#define SCL_PORT PORTD
#define SCL_PIN 0
//#define I2C_FASTMODE 1

#include<SoftI2CMaster.h>

// struct for storiong date time info
typedef struct datetime{
  byte  year, month, day, hour, minute, second;  
}DateTime;

#endif //__TINY_RTC_H__
