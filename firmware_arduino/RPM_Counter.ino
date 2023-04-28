/* rpm counter functions */


/* time of last tick, stored as microseconds for the benefit of PID */
unsigned int rpm_last_tick_time_ms = 0;

//#define RPM_CALC_SIMPLE
#define RPM_SCALE_BITS 2
#define RPM_SCALE_ROUNDING (1<<(RPM_SCALE_BITS-1))

/* returns the average rpm for a given record */
unsigned int get_rpm(DATA_RECORD * data_record)
{
      /* for rpm, we can approximate by the number of ticks divided by the update interval
       * but for a more acurate reading, we must divide the number of ticks
       * by the sum all tick intervals
       * 
       * lets see if we can remove the long division...
       *  
       *  on average, the total tick time cannot be greater than the update interval
       * eg if rpm is 3000, that is 50 ticks 20ms of per update 
       * rpm avg total will be 1000ms +/- change in rpm of 1st tick vs last
       * if rpm is 750, that is 12 to 13 ticks of 83 to 77ms per update
       * rpm avg total could be from 924 to 1080ms
       * 
       * we need about 11 bits for the denominator and up to 23bits for the numerator
       * we'd need to loose 7 bits of precision to fit this into 16 bit division
       * giving a final precision of approx 3bits (0-8)
       * 
       * on the other hand,
       * since rpm avg tot. will always be approx 1000 +/- 100ms
       * could we divide 60000 and retain precision?
       * 60k/1000 = 60, 6 bits, twice as good as the above.
       * 
       * can we shift some more bits about?
       * if we divide time by two
       * val = 60000/(rpm_avg_tot/2)
       * avg = (ticks * val)/2
       * 
       * eg
       * rpm_tot = 1026 (11 bit)
       * rpm_tot_reduced = 1026>>1 = 513 (10 bit)
       * val = 60000/513 = 116 (7bit)
       * 
       * rpm_tot = 1026 (11 bit)
       * rpm_tot_reduced = 1026>>2 = 256 (9 bit)
       * val = 60000/256 = 234 (8bit)
       * dividing by 4 reduces the input precision to 8/9 bits, but increases the output precision to 8bits
       * 
       * 
       */
  unsigned int val = 0;
  #ifdef RPM_CALC_SIMPLE
  val = (60000 / UPDATE_INTERVAL_ms) * data_record->RPM_no_of_ticks;
  #else
  if (data_record->RPM_avg > 0)
  {
    val = 60000 / ((data_record->RPM_avg + RPM_SCALE_ROUNDING) >> RPM_SCALE_BITS);
    val = ((data_record->RPM_no_of_ticks * val) + RPM_SCALE_ROUNDING) >> RPM_SCALE_BITS;
  }
  #endif
  #ifdef DEBUG_RPM_COUNTER
  Serial.print(F("RPM Debug: "));
  Serial.print(data_record->RPM_avg);
  Serial.print(F(", "));
  Serial.print(data_record->RPM_no_of_ticks);
  Serial.print(F(" -> "));
  Serial.print(val);
  Serial.println();
  #endif
  return val;
}

/* check for excessivley long intervals and clip time to reasonable values 
  we could expand on this to sustain the last non zero rpm until this event */
void rpm_clip_time()
{
    unsigned int timenow_ms = millis();
    unsigned int elapsed_time = timenow_ms - rpm_last_tick_time_ms;
    if(elapsed_time > RPM_MAX_TICK_INTERVAL_ms) elapsed_time = timenow_ms - (elapsed_time>>1);
}

/* gives a time in ms for the given rpm */
unsigned int get_rpm_time_ms(unsigned int rpm)
{
  return 60000/rpm;
}
/* gives a time in us for the given rpm */
unsigned long get_rpm_time_us(unsigned int rpm)
{
  return 60000000/rpm;
}

/* ISR for rpm counter input */
void rpm_count(void)
{
  if (CURRENT_RECORD.RPM_no_of_ticks < RPM_MAX_TICKS_PER_UPDATE)
  {
    /* get time now and time elapsed since last tick */
    unsigned int timenow_ms = millis();
    unsigned int elapsed_time = timenow_ms - rpm_last_tick_time_ms;
    /* update last tick time to current tick */
    rpm_last_tick_time_ms = timenow_ms;

    /* only count ticks if last tick time was not zero or elapsed time is reasonable */
    if (/*rpm_last_tick_time_ms != 0 || */(elapsed_time > RPM_MIN_TICK_INTERVAL_ms))
    {
      if(elapsed_time > RPM_MAX_TICK_INTERVAL_ms) elapsed_time = RPM_MAX_TICK_INTERVAL_ms;
      
      /* increment total time for final average */
      CURRENT_RECORD.RPM_avg += elapsed_time;
      /* clamp time to fit storage datatype */
      if (elapsed_time > 255) elapsed_time = 255;  
      /* record the tick time */
      CURRENT_RECORD.RPM_tick_times_ms[CURRENT_RECORD.RPM_no_of_ticks++] = elapsed_time;
    }
    
  }
}

/* set up the rpm counter input pin and ISR */
void configiure_rpm_counter(void)
{
  pinMode(PIN_RPM_COUNTER_INPUT, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_RPM_COUNTER_INPUT), rpm_count, RISING );
}
