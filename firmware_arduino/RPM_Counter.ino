/*
 * MEGA 2560 Input Capture mode RPM counter.
 * This method is not compatible with the Servo library
 * 
 * Input capture pins are:
 * ICPn,    Port Pin,   Dig. Pin
 * ICP1     PD4         -
 * ICP3     PE7         -
 * ICP4     PL0         D49
 * ICP5     PL1         D48
 * 
 * Only timers 4 & 5 can be used for ICP as the others have no pin.
 * 
 * 
 * Base clock is 16Mhz, 
 * DIV    Clk       Tick    Max Time    Lowest RPM    T@6k rpm  drpm@6k rpm               T@1k rpm    drpm@1k rpm        
 * 1      16M       62.5n   4.09600m    14648         -         -                         -           -
 * 8      2M        500 n   32.7680m    1831.0        20000     -0.300  +0.300            -           -
 * 64     250k      4.0 u   262.144m    228.88        2500      -2.399  +2.401            15000       -0.0667 +0.0667
 * 256    62.5k     16.0u   1.048576    57.220        625       -9.585  +9.615            3750        -0.267  +0.267
 * 1024   15.625k   64.0u   4.194304    14.305        156(.25)  -38.28 (+9.615) +38.77    937(.5)     -1.064 (-0.533) +1.067
 * 
 * The ICP interrupt needs to collect the new timer value and subtract the old timer values to get an accurate interval,
 * reseting the timer during the interrupt would introduce latency when other interrupts are occuring.
 * The overflow interrupt is also needed to detect when multiple overflows have occured. 
 * There's no need to add the number of overflows to the time, just discard any intervals 
 * that span multiple overflows and consider the engine stopped.
 * 
 * We'll set the clock to 62.5k initially, and consider increasing it later if needed.
 */

#define OVF_LIMIT 2           //timer overflow threshold
#define OVF_COUNT_LIMIT 254   //timer overflow limit





volatile byte rpm_tmr_ovf = 2;        //increment on overflow, reset on input capture, discard time if >1
volatile unsigned int rpm_last_time_tk;      //last ICP value

 
/* time of last tick */

/* total ticks and ms since last pid update */
volatile unsigned int rpm_total_tk;
volatile byte rpm_total_count;




//#define RPM_CALC_SIMPLE
#define RPM_SCALE_BITS 2
#define RPM_SCALE_ROUNDING (1<<(RPM_SCALE_BITS-1))




/* returns the average rpm for a given record */
unsigned int get_rpm(DATA_RECORD * data_record)
{
 
  unsigned int val = 0;
  unsigned int RPM_elapsed = 0;
  #ifdef RPM_CALC_SIMPLE
  val = (60000 / UPDATE_INTERVAL_ms) * data_record->RPM_no_of_ticks;
  #else
  if (data_record->RPM_no_of_ticks > 0)
  {
    //sum the tick times
    for(byte n=0; n<data_record->RPM_no_of_ticks; n++)
    {
      RPM_elapsed += data_record->RPM_tick_times_tk[n];
    }
    if(RPM_elapsed > 0)
    {
      val = 60000 / ((RPM_elapsed + RPM_SCALE_ROUNDING) >> RPM_SCALE_BITS);
      val = ((data_record->RPM_no_of_ticks * val) + RPM_SCALE_ROUNDING) >> RPM_SCALE_BITS;
    }
    else
    {
      //shouldn't occur, but just in case, use the previous RPM avg - this will hopefully smooth over any glitches
      val = Data_Averages.RPM;
    }
  }
  else
  {
    //decay average rpm, 25% decay rate
    val = (Data_Averages.RPM*3)>>2;
  }
  
  #ifdef DEBUG_RPM_COUNTER
  Serial.print(F("RPM Debug: "));
  Serial.print(RPM_elapsed);
  Serial.print(F(", "));
  Serial.print(data_record->RPM_no_of_ticks);
  Serial.print(F(" -> "));
  Serial.print(val);
  Serial.println();
  #endif //DEBUG_RPM_COUNTER
  #endif //RPM_CALC_SIMPLE
  return val;
}

/* calculate average rpm since the last call of this function
 *  this should be called in the PID uppdate, even if not being used
 *  
 *  will return ticks if flags_config.pid_rpm_use_time is set
 */
unsigned int get_rpm_for_pid()
{
  //clamp the overflow counter
  if(rpm_tmr_ovf > OVF_COUNT_LIMIT) rpm_tmr_ovf = OVF_COUNT_LIMIT;

  //lock out the ICP interrupt while we grab data
  RPM_INT_DIS();
  unsigned int total_t = rpm_total_tk;
  byte total_count_t = rpm_total_count;
  //reset the counts
  rpm_total_tk = 0; 
  rpm_total_count = 0;
  //reenable the interrupt
  RPM_INT_EN();

  static unsigned int rpm_avg_since_last_pid;
  static byte no_tick_count = 0;

  //check for ticks since last update
  if(total_t)
  {
    /* expected operation: 
     *  tick range (rpm max)625   3750(rpm min) 
     *  tick count          5     1
     *  tick total          3125  3750
     */

    if(!flags_config.pid_rpm_use_time)
    {
      //calculate average rpm over last 50ms
      rpm_avg_since_last_pid = (unsigned int)(((60000000L/TICK_us) * total_count_t)/(long)total_t);
    }
    else
    {
      rpm_avg_since_last_pid = TICK_TO_US(total_t)/(total_count_t);
    }
    //reset no tick count
    no_tick_count = 0;
      
  }
  else
  {
    /* if rpm is lower than 1200, a tick may not occur within this update interval
     * if this is the first time without tick, we can use the last rpm figure
     * otherwise, we count the number of cycles with no tick up until rpm_tmr_ovf 
     * reaches it's limit, OVF_LIMIT
     * At that point we call rpm zero
     */

    //check that the timer hasn't overflowed
    if(rpm_tmr_ovf < OVF_LIMIT)
    {
      //check if this is the first time no ticks were counted
      if(no_tick_count++)
      {
        //if not, estimate the rpm from the number of missed ticks
        if(!flags_config.pid_rpm_use_time)
          rpm_avg_since_last_pid = MS_TO_RPM( UPDATE_INTERVAL_ms * no_tick_count );
        else
          rpm_avg_since_last_pid = MS_TO_TICK( UPDATE_INTERVAL_ms * no_tick_count );
        #ifdef DEBUG_RPM_COUNTER
        Serial.print(no_tick_count);Serial.println("th no tick");
      }
      else
      {
        Serial.println("1st no tick");
        #endif
      }
    }
    else
    {
      #ifdef DEBUG_RPM_COUNTER
      if(rpm_avg_since_last_pid)
      {
        Serial.println("RPM: 0");
      }
      #endif
      //set rpm to zero
      rpm_avg_since_last_pid = 0; //even if using ticks, zero means engine stopped, do not perform PID
      //reset no tick count so that the first tick does not use a stale no_tick_count to estimate rpm
      no_tick_count = 0;
    }
  }

  #ifdef DEBUG_RPM_COUNTER
  //report the rpm
  if(rpm_avg_since_last_pid)
  {
    Serial.print("RPM: "); Serial.println(rpm_avg_since_last_pid); 
  }
  #endif

  return rpm_avg_since_last_pid;
  
}




/* set up the rpm counter input pin and ISR */
void configiure_rpm_counter(void)
{
  //configure ICP pin as input with pullup

  pinMode(PIN_RPM_COUNTER_INPUT, INPUT_PULLUP); 

  //configure timer 4, enable ovfl and icp isr's for timer 4
  //TCCR4A    COM-A1 COM-A0 COM-B1 COM-B0 COM-C1 COM-C0 WGM-1  WGM-0
  //          0      0      0      0      0      0      0      0
  //TCCR4B    ICNC   ICES   -      WGM3   WGM2   CS2    CS1    CS0
  //          0      0      0      0      0      1      0      0
  //TCCR4C    FOC-A  FOC-B  FOC-C  -      -      -      -      -
  //TIMSK4    -      -      ICIE   -      OCIE-C OCIE-B OCIE-A TOIE
  //          0      0      1      0      0      0      1      1 

  //set OCRA to 50% for overflow detection
  OCR4A  = INT16_MAX;
  TCCR4A = 0;
  //set the clock to 62.5kHz
  TCCR4B = _BV(CS42);
  //enable the intterupts
  TIMSK4 = _BV(ICIE4) | _BV(TOIE4) | _BV(OCIE4A);
}


/* //////// OVERFLOW COUNT considerations
 *  
 * A tick occuring at MAX/2 followed by a tick MAX+n, where n is < MAX/2,
 * would overflow before the timer overflows a second time.
 * To get around this, we need to increment the overflow count at the half way mark.
 * We can do this using OCA match interrupt
 */
//input capture interrupt
ISR(TIMER4_CAPT_vect)
{
  unsigned int this_time = RPM_counter;

  /* record timestamp of first tick each record */
  if (CURRENT_RECORD.RPM_no_of_ticks == 0)
  {
    //since we are now using ticks instead of ms, when the record updates
    //we need to record the ICP value in ticks and subtract it from this ICP
    CURRENT_RECORD.RPM_tick_offset_tk = this_time - CURRENT_RECORD.RPM_tick_offset_tk; 
  }
  
  //check for overflows
  if(rpm_tmr_ovf < OVF_LIMIT)// && ~idx) //(stop when out of samples, wait for buffer to clear)
  {
    unsigned int interval = this_time - rpm_last_time_tk;
    //check for space in current record
    if (CURRENT_RECORD.RPM_no_of_ticks < RPM_MAX_TICKS_PER_UPDATE)
    {
      CURRENT_RECORD.RPM_tick_times_tk[CURRENT_RECORD.RPM_no_of_ticks++] = interval;
    }
    //tally for PID
    rpm_total_tk += interval;
    rpm_total_count++;
  }
  //reset overlow counter and store current tick time
  rpm_tmr_ovf = 0;
  rpm_last_time_tk = this_time;
}
//timer overflow interrupt
ISR(TIMER4_OVF_vect)
{
  //increment overflow count
   rpm_tmr_ovf++;
}
//OCA match interrupt is the same as the overflow interrupt
ISR(TIMER4_COMPA_vect, ISR_ALIASOF(TIMER4_OVF_vect));
