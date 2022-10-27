#ifndef __PID_H__
#define __PID_H__


#define PID_FL_TO_FP(f_val)      ((int)((f_val)*PID_FP_ONE))

#define PID_FP_FRAC_BITS        8     //use 8.8 fixed point integers for pid calculations
#define PID_FP_WHOLE_BITS       7     //not including sign bit
#define PID_FP_ONE              (1<<PID_FP_FRAC_BITS)
#define PID_FP_WHOLE_MAX        ((1<<PID_FP_WHOLE_BITS)-1)
#define PID_FP_FRAC_MAX         (PID_FP_ONE-1)
#define PID_FP_MAX              (PID_FL_TO_FP(PID_FP_WHOLE_MAX) + PID_FP_FRAC_MAX)

#define PID_OUTPUT_MAX          (1023)
#define PID_OUTPUT_MIN          (0)
#define PID_OUTPUT_CENTRE       ( (PID_OUTPUT_MAX + PID_OUTPUT_MIN) >> 1 )




typedef struct pid
{
  int target;
  int kp, ki, kd;
  int p;
  long i;
  int d;
  byte invert;
}PID;

extern PID RPM_control; 
extern PID VAC_control;


#endif //__PID_H__
