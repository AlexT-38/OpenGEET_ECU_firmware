#ifndef __TORQUE_H__
#define __TORQUE_H__

#define TORQUE_PRESCALE   4     //divide by 16 to get values in the INT16 range
#define TORQUE_ROUNDING   _BV(TORQUE_PRESCALE-1)


typedef struct torque_cal
{
  long counts_max, counts_min, counts_zero;
}TORQUE_CAL;

extern TORQUE_CAL          torque_cal;

#endif //__TORQUE_H__
