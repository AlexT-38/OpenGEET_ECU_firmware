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

#define PID_RPM_MAX_FB_TIME_MS  1000
#define PID_RPM_MIN_FB_RPM      60

#define PID_FLAG_INVERT       1

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
  int actual;
  PID_K k;
  int err;
  int output;
  int p;
  long i;
  int d;
  byte invert;
}PID;

typedef struct pid_record
{
  int target[PID_LOOPS_PER_UPDATE];
  int actual[PID_LOOPS_PER_UPDATE];
  int err[PID_LOOPS_PER_UPDATE];
  int output[PID_LOOPS_PER_UPDATE];
  int p[PID_LOOPS_PER_UPDATE];
  long i[PID_LOOPS_PER_UPDATE];
  int d[PID_LOOPS_PER_UPDATE];
}PID_RECORD;

#define RPM_control PIDs[PID_RPM] 
#define VAC_control PIDs[PID_VAC]

typedef enum pids
{
  PID_RPM = 0,
  PID_VAC
}PIDS;

extern PID PIDs[NO_OF_PIDS];


#endif //__PID_H__
