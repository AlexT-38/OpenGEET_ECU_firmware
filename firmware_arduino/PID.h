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


/* PWL approximation of LOG2 and POW2 */
#define LOG_PAR_MIN_BITS      (4)
#define LOG_PAR_BITS          (16)
#define LOG_PAR_LIM           (1<<LOG_PAR_BITS)
#if LOG_PAR_BITS == (16)
#define LOG_PAR_MAX           (UINT16_MAX)
#else
#define LOG_PAR_MAX           (LOG_PAR_LIM-1)
#endif
#define LOG_SCR_BITS          (8)
#define LOG_SCR_LIM           (1<<LOG_SCR_BITS)
#define LOG_SCR_MAX           (LOG_SCR_LIM-1)
#define LOG_STEP_BITS         (LOG_SCR_BITS-4)
#define LOG_STEP              (1<<LOG_STEP_BITS)
#define LOG_STEPS             (LOG_SCR_MAX/LOG_STEP)
#define LOG_SCR_MAX           (LOG_SCR_LIM-1)
#define LOG_SCR_MIN           ((LOG_SCR_LIM * LOG_PAR_MIN_BITS) / LOG_PAR_BITS)
#define LOG_PAR_MIN           (1<<LOG_PAR_MIN_BITS)
#define LOG_WH_OFFSET         (LOG_STEP_BITS-2)

typedef struct pid_k
{
  int p,i,d;
}PID_K;

typedef struct pid
{
  int target;
  PID_K k;
  int p;
  long i;
  int d;
  byte invert;
}PID;

extern PID RPM_control; 
extern PID VAC_control;


#endif //__PID_H__
