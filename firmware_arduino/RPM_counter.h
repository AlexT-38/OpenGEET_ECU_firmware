#ifndef __RPM_COUNTER_H__
#define __RPM_COUNTER_H__

#define RPM_TO_MS(rpm)                (60000/(rpm))
#define MS_TO_RPM(ms)                 RPM_TO_MS(ms)   //same formula, included for clarity

#define RPM_MAX                       6000
#define RPM_MIN_TICK_INTERVAL_ms      RPM_TO_MS(RPM_MAX)
#define RPM_MAX_TICKS_PER_UPDATE      (UPDATE_INTERVAL_ms/RPM_MIN_TICK_INTERVAL_ms)
#define RPM_MIN                       1
#define RPM_MAX_TICK_INTERVAL_ms      RPM_TO_MS(RPM_MIN)


#define RPM_MIN_SET                   1000
#define RPM_MAX_SET                   4500
#define RPM_MIN_SET_ms                RPM_TO_MS(RPM_MIN_SET)
#define RPM_MAX_SET_ms                RPM_TO_MS(RPM_MAX_SET)

extern volatile unsigned long rpm_last_tick_time_ms;

extern volatile unsigned int rpm_total_ms;
extern volatile unsigned int rpm_total_tk;

#endif //__RPM_COUNTER_H__
