#ifndef __SERVOS_H__
#define __SERVOS_H__


#define SERVO_TIMER_CLK     _BV(CS31)
#define SERVO_US_FRAC_BITS    1

#define US_TO_SERVO(us)     (us<<SERVO_US_FRAC_BITS)
#define SERVO_TO_US(us)     (us>>SERVO_US_FRAC_BITS)

#define SERVO_MIN_us               750     //default minimum servo value
#define SERVO_MAX_us               2300    //default maximum servo value
#define SERVO_RANGE             (SERVO_MAX_us-SERVO_MIN_us)


#define SERVO_MIN_tk               US_TO_SERVO(SERVO_MIN_us)     //default minimum servo value
#define SERVO_MAX_tk               US_TO_SERVO(SERVO_MAX_us)    //default maximum servo value

typedef struct servo_cal
{
  unsigned int upper, lower;
}SV_CAL;

extern SV_CAL servo_cal[NO_OF_SERVOS];

#endif  //__SERVOS_H__
