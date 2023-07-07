#ifndef __RPM_COUNTER_H__
#define __RPM_COUNTER_H__


#define RPM_INT_EN()                  TIMSK4 &= ~_BV(ICIE4)
#define RPM_INT_DIS()                 TIMSK4 |= _BV(ICIE4)
  
#define RPM_TO_MS(rpm)                (60000/(rpm))
#define MS_TO_RPM(ms)                 RPM_TO_MS(ms)   //same formula, included for clarity

#define TICK_us_BITS                  4
#define TICK_us                       _BV(TICK_us_BITS)

#define US_TO_RPM(us)                 (60000000L/(us))
#define TICK_TO_US(tk)                (((long)tk)<<TICK_us_BITS)
#define TICK_TO_RPM(tk)               ((60000000L/TICK_us)/((long)tk))
#define MS_TO_TICK(ms)                ((((long)ms)*1000)>>TICK_us_BITS)
#define RPM_TO_TK(rpm)                TICK_TO_RPM(rpm)

#define RPM_MAX_rpm                   6000
#define RPM_MIN_TICK_INTERVAL_ms      RPM_TO_MS(RPM_MAX_rpm)
#define RPM_MAX_TICKS_PER_UPDATE      (1+(UPDATE_INTERVAL_ms/RPM_MIN_TICK_INTERVAL_ms))
#define RPM_MIN                       1
#define RPM_MAX_TICK_INTERVAL_ms      RPM_TO_MS(RPM_MIN)

#define RPM_MIN_SET_rpm                   1000
#define RPM_MAX_SET_rpm                   4500
#define RPM_MIN_SET_ms                RPM_TO_MS(RPM_MIN_SET_rpm)
#define RPM_MAX_SET_ms                RPM_TO_MS(RPM_MAX_SET_rpm)
#define RPM_MIN_SET_tk                RPM_TO_TK(RPM_MIN_SET_rpm)
#define RPM_MAX_SET_tk                RPM_TO_TK(RPM_MAX_SET_rpm)



#define RPM_counter                   ICR4



#endif //__RPM_COUNTER_H__
