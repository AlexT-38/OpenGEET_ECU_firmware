
static unsigned int rpm_last_tick_time = 0;

void rpm_count(void)
{
  if (CURRENT_RECORD.RPM_no_of_ticks < MAX_RPM_TICKS_PER_UPDATE)
  {
    unsigned int timenow = millis();
    unsigned int elapsed_time = timenow - rpm_last_tick_time;
    rpm_last_tick_time = timenow;
    if (elapsed_time > 255) elapsed_time = 255;
    CURRENT_RECORD.RPM_tick_times_ms[CURRENT_RECORD.RPM_no_of_ticks++] = elapsed_time;
  }
}
void configiure_rpm_counter(void)
{
  pinMode(PIN_RPM_COUNTER_INPUT, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_RPM_COUNTER_INPUT), rpm_count, RISING );
}
