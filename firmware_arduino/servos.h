#ifndef __SERVOS_H__
#define __SERVOS_H__



Servo servo[NO_OF_SERVOS];

// servo pins 
#define PIN_SERVO_0               3     //uses INT1, making INT0 the only one available
#define PIN_SERVO_1               5
#define PIN_SERVO_2               6

#define SERVO_MIN               750     //default minimum servo value
#define SERVO_MAX               2300    //default maximum servo value
#define SERVO_RANGE             (SERVO_MAX-SERVO_MIN)
typedef struct servo_cal
{
  unsigned int upper, lower;
}SV_CAL;

extern SV_CAL servo_cal[NO_OF_SERVOS];

#endif  //__SERVOS_H__
