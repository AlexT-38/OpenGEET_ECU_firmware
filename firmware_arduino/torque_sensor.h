#ifndef __TORQUE_H__
#define __TORQUE_H__

#define TORQUE_PRESCALE   (8-3)     
#define TORQUE_ROUNDING   _BV(TORQUE_PRESCALE-1)


typedef struct torque_cal
{
  long counts_max, counts_min, counts_zero;
}TORQUE_CAL;

extern TORQUE_CAL          torque_cal;

#endif //__TORQUE_H__
