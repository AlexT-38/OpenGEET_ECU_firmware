/* rpm counter functions */


/* time of last tick, stored as microseconds for the benefit of PID */
static unsigned int rpm_last_tick_time_us = 0;

/* ISR for rpm counter input */
void rpm_count(void)
{
  if (CURRENT_RECORD.RPM_no_of_ticks < MAX_RPM_TICKS_PER_UPDATE)
  {
    /* get time now and time elapsed since last tick */
    unsigned int timenow_us = micros();
    unsigned int elapsed_time = timenow_us - rpm_last_tick_time_us;
    /* update last tick time to current tick */
    rpm_last_tick_time_us = timenow_us;
    /* convert to ms */
    elapsed_time = (elapsed_time + 500) / 1000;
    /* clamp time to fit datatype */
    if (elapsed_time > 255) elapsed_time = 255;
    /* increment total time for final average */
    CURRENT_RECORD.RPM_avg += elapsed_time;
    /* record the tick time */
    CURRENT_RECORD.RPM_tick_times_ms[CURRENT_RECORD.RPM_no_of_ticks++] = elapsed_time;
  }
}

/* set up the rpm counter input pin and ISR */
void configiure_rpm_counter(void)
{
  pinMode(PIN_RPM_COUNTER_INPUT, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_RPM_COUNTER_INPUT), rpm_count, RISING );
}
