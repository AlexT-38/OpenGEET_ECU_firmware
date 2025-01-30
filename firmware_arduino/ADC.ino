
 /* New ADC system
  * 
  * finalise_record() waits for any fast conversion to complete for all fast channels,
  * then starts the ADC trigger timer and resets sample/channel counts
  * process_adc() waits for any ongoing conversion to finish as above
  * process_adc() disables the ADC interrupt and timer trigger
  * process_adc() reads slow channels
  * process_adc() reenables ADC interrupt and timer trigger
  * ADC ISR stops the ADC trigger timer once sufficient samples have been recieved
  */


//config for timer 0 to trigger adc
#define ADC_TMR_PRESCALE_CFG          (3<<CS10)       // div by 64, clk 250kHz, 4us ticks
#define ADC_TMR_PRESCALE              64

//stop start and clear the ADC timer
#define ADC_TMR_STOP()                TCCR1B &= ~(_BV(CS10)|_BV(CS11)|_BV(CS12))
#define ADC_TMR_START()               TCCR1B |= ADC_TMR_PRESCALE_CFG
#define ADC_TMR_CLR()                 TCNT1 = 0

//configure the ADC timer
#define ADC_TMR_CFG()                 TCCR1A = 0; TCCR1B = _BV(WGM12);    //CTC, TOP = OCR1A

//TIMER TOP and ADC trigger values    //OCR1A = (T(OC1A) * f(clk_IO) / 2N) -1
#define ADC_TMR_TOP                   (unsigned int)(((ADC_SAMPLE_INTERVAL_us * (F_CPU/1000) / ADC_TMR_PRESCALE) / 1000) -1) //4.5ms = 1124, 5ms = 1249, 4.99ms = 1246
#define ADC_TMR_TRIG                  1
#define ADC_TMR_SET()                 OCR1A = ADC_TMR_TOP; OCR1B = ADC_TMR_TRIG;

//select the trigger source for ADC
#define ADC_SET_TRIG_ADC()            ADCSRB &= ~0x7   //free running
#define ADC_SET_TRIG_TMR()            ADCSRB |= 0x5   //trig on OC1B.

//clear/get the ADC timer trigger flag
#define ADC_TMR_CLRF()                TIFR1 |= _BV(OCF1B)
#define ADC_TMR_GETF()                ((TIFR1 & _BV(OCF1B)) != 0)

//enable and disable ADC auto triggering
#define ADC_TRIG_DIS()                ADCSRA &= ~_BV(ADATE)
#define ADC_TRIG_EN()                 ADCSRA |= _BV(ADATE)

//defined in ADC.h
//#define ADC_INT_DIS()                 ADCSRA &= ~(_BV(ADIE) | _BV(ADIF))
//#define ADC_INT_EN()                  ADCSRA &= ~_BV(ADIF); ADCSRA |= _BV(ADIE)

#define ADC_START()                   ADCSRA |= _BV(ADSC)

#define ADC_COMPLETE()                (ADCSRA & _BV(ADIF))
#define ADC_CLR_FLAG()                ADCSRA |= _BV(ADIF)


  /* old */
//configuration

#define ADC_CHANNELS              (ADC_CHN_TMP_END+1)        // number of ADC inputs to scan ---- this isn't used

// sensor channel mapping
//user inputs will be mapped to analog channels starting from                  ---- is this assuming that usr/map/tmp are contiguous?
#define ADC_CHN_USR_START             0                                     // ---- yes and no - only per speed it seems, and any physical pin can be mapped.
#define ADC_CHN_USR_END               (ADC_CHN_USR_START+NO_OF_USER_INPUTS) // ---- this isn't used
//MAP sensors will start from --map sensors now use fast sampling
#define ADC_CHN_MAP_START             (0)
#define ADC_CHN_MAP_END               (ADC_CHN_MAP_START+NO_OF_MAP_SENSORS)
//TMP sensors will start from
#define ADC_CHN_TMP_START             (ADC_CHN_USR_END)//(ADC_CHN_MAP_END)  // ---- this isn't right, should be ADC_CHN_USR_END, as USR and TMP are slow
#define ADC_CHN_TMP_END               (ADC_CHN_TMP_START+NO_OF_TMP_SENSORS) // ---- this is used for iterating over slow channels when getting calibrated readigns
                                                                            // ---- thermistor inputs haven't been tested yet
#define ADC_NO_SLOW                  (NO_OF_USER_INPUTS+NO_OF_TMP_SENSORS)
#define ADC_NO_FAST                   (NO_OF_MAP_SENSORS)
/* new ADC channel mapping */
byte channels_slow[ADC_NO_SLOW] = {1,2,3};                //channels to sample at low rate (USR and TMP sensors)
byte channels_fast[ADC_NO_FAST] = {0,10};                 //channels to sample at high rate (MAP sensors)
byte adc_chn;                                             //channel index, increments each adc interrupt
byte adc_smp;                                             //sample index, increments each adc interrupt when adc_chn overflows

int adc_readings_slow[ADC_NO_SLOW];

//averaged map/fast reading for pid sampling
int pid_total;
byte pid_count;
char pid_chn = -1;  //fast channel to read, -1 for none
#define ADC_PID_COUNT_MAX   64  //maximum number of samples to acumulate

#ifdef DEBUG_ADC_FAST
unsigned int isr_duration_tk;
bool isr_print = false;
#endif

/* old ADC */
void configureADC()
{
  //disable the digital inputs
  DIDR0 = 0xFF;
  DIDR1 = 0xFF;
  PORTF=0x0;
  PORTK=0x0;
  DDRF=0;
  DDRK=0;
  
  //enable the ADC and set prescaler to slowest rate
  analogReference(EXTERNAL);
  //ADCSRA = _BV(ADEN) | 0x7;

  // set timer 0 to CTC (mode 2, no output compare) (also clears default arduino config for TMR1)
  ADC_TMR_CFG();
  // clear timer 0
  ADC_TMR_CLR();
  // setup timer 0 to reset every 5ms
  ADC_TMR_SET();
}

/* call to stop the interrupt driven ADC process */
void ADC_stop_fast()
{
  //wait for ongoing fast sampling to complete all channels
  while(ADC_TMR_GETF()) {}
  
  ADC_TMR_STOP();
  ADC_TRIG_DIS();
  ADC_INT_DIS();
  
}

/* call to start the interrupt driven ADC process */
void ADC_start_fast()
{
  adc_chn = 0;
  //select the first fast channel
  ADC_set_channel(channels_fast[adc_chn]);
  //set the timer as the trigger source
  ADC_SET_TRIG_TMR();
  //clear the timer and interrupt flag
  ADC_TMR_CLR();
  ADC_TMR_CLRF();
  //enable the ADC interrupt
  ADC_INT_EN();
  //enable the trigger
  ADC_TRIG_EN();
  //start the timer
  ADC_TMR_START();
}

/* select the adc channel to sample next time */
void ADC_set_channel(byte chn)
{
  //ADC is in two banks of 8, ADMUX set the bank's channel and voltage reference (set to ext AREF)
  ADMUX = (chn & 0x7);
  //select the bank
  if((chn & 0x8)==0)
    ADCSRB &= ~_BV(MUX5);
  else
    ADCSRB |= _BV(MUX5);
}

/* get the average map reading since the last call of this function (up to a maximum of 64 samples) */
int ADC_pid_pop_map()
{
  static int last = 0;
  if (pid_count == 0) return last;
  if (pid_chn < 0 || pid_chn > NO_OF_MAP_SENSORS) return -1;
  
  ADC_INT_DIS();
  int count = pid_count;
  int value = pid_total;
  pid_total = 0;
  pid_count = 0;
  ADC_INT_EN();
  
  value /= count;
  value = ADC_apply_map_calibration(pid_chn, value);
  last = value;
  return value;
}

/* main ADC processing task */
void process_analog_inputs()
{
  #ifdef DEBUG_ANALOG_TIME
  unsigned int timestamp_us = micros();
  #endif

  #ifdef DEBUG_ADC_ISR
  Serial.print(F("\nADC ISR time tk: ")); Serial.println(isr_duration_tk);
  #endif

  //read the torque sensor while we wait for the ADC to catch complete.
  int torque = torqueRead(); //while technically a 'digital' sensor (in terms of interface), the update rate is 10Hz, which matches with the analog update rate, and not the MAX6675 update rate
                             //the torque data is always one sample behind, since the conversion takes 100ms
  // Note also that th HX711 can sample at 80SPS. If the fast sample rate were set to 6.25ms (160SPS), a torque reading could be taken every other sample.
  // alternatively, keeping fast at 5ms (200SPS) the HX711 could be sampled every third fast sample for a rate of 66SPS.

  //wait for ongoing fast sampling to complete all channels
  while(ADC_TMR_GETF()){
    #ifdef DEBUG_ADC
    Serial.print(F("ADC wait, "));Serial.println(ADC_TMR_GETF());
    #endif
  }

  //disable the trigger source and interrupts
  ADC_TRIG_DIS();
  ADC_INT_DIS();

  //read slow channels
  for(byte n = 0; n < ADC_NO_SLOW; n++)
  {
    //set channel
    ADC_set_channel(channels_slow[n]);
    //start ADC
    ADC_START();
    //wait for conversion by checking interrupt flag
    while(! ADC_COMPLETE()); //{ Serial.println(ADCSRA,HEX); }
    //read value
    adc_readings_slow[n] = ADC;
    //clear the interrupt flag
    ADC_CLR_FLAG();
  }


  //re-enable the trigger source and interrupts
  ADC_TRIG_EN();
  ADC_INT_EN();



  #ifdef DEBUG_ADC
  Serial.println(F("\nADC slow results:"));
  for (int chn = 0; chn < ADC_NO_SLOW; chn++)
  {
    Serial.print(chn);
    Serial.print(FS(S_COLON));
    Serial.println(adc_readings_slow[chn]);
  }
  Serial.println();
  #endif

  //copy user inputs to knob values todo: make this make sense - PID's should probably take their target values direct from ADC_results
  for(byte knob=0; knob<NO_OF_KNOBS; knob++)
  {
    KNOB_values[knob] = adc_readings_slow[ADC_CHN_USR_START+knob];
  }


  
  // get calibrated MAP sensor reading
  //this should be done by the finalise record process, along with min-maxing
  for(byte idx = 0; idx<(0); idx++)
  {
    #ifdef DEBUG_MAP_CAL
    int pressure_LSBs = 0;
    #endif
    
    //todo: make this have independant calibrations for each sensor, so we can have 0-1 bar on intake, 0-2/3 bar for exhaust pressure, etc
//    ADC_results[idx] = amap(ADC_results[idx], SENSOR_MAP_CAL_MIN_mbar, SENSOR_MAP_CAL_MAX_mbar); 
    
    #ifdef DEBUG_MAP_CAL
    Serial.print(idx-ADC_CHN_MAP_START);
    Serial.print(F(" map cal; in, min, max, out: "));
    Serial.print(pressure_LSBs); Serial.print(F(", "));
    Serial.print(SENSOR_MAP_CAL_MIN_mbar); Serial.print(F(", "));
    Serial.print(SENSOR_MAP_CAL_MAX_mbar); Serial.print(F(", "));
//    Serial.print(ADC_results[idx]); Serial.print(F(" mbar"));
    Serial.println();
    #endif
  }

  //stand ins for thermistor calibration
  #define SENSOR_TMP_CAL_MAX_degC   0
  #define SENSOR_TMP_CAL_MIN_degC   100

  // get calibrated TMP sensor reading
  for(byte idx=ADC_CHN_TMP_START; idx<(ADC_CHN_TMP_END); idx++)
  {
    #ifdef DEBUG_TMP_CAL
    int temperature_LSBs = adc_readings_slow[idx];
    #endif
    
    //todo: make this have independant calibrations for each sensor, so we can have 0-1 bar on intake, 0-2/3 bar for exhaust pressure, etc
    adc_readings_slow[idx] = amap(adc_readings_slow[idx], SENSOR_TMP_CAL_MIN_degC, SENSOR_TMP_CAL_MAX_degC); 
    //temperature sensors are typically non linear (unless a constant current source is used) and will need a lokup table for conversion
    
    #ifdef DEBUG_TMP_CAL
    Serial.print(idx-ADC_CHN_TMP);
    Serial.print(F(" tmp cal; in, min, max, out: "));
    Serial.print(temperature_LSBs); Serial.print(F(", "));
    Serial.print(SENSOR_MAP_CAL_MIN_degC); Serial.print(F(", "));
    Serial.print(SENSOR_MAP_CAL_MAX_degC); Serial.print(F(", "));
    Serial.print(ADC_results[idx]); Serial.print(F(" mbar"));
    Serial.println();
    #endif
  }

  
  //log the analog values, if there's space in the current record
  if (CURRENT_RECORD.ANA_no_of_slow_samples < ANALOG_SAMPLES_PER_UPDATE)
  {
    //log calibrated data
    unsigned int analog_index = CURRENT_RECORD.ANA_no_of_slow_samples++;

    CURRENT_RECORD.TRQ[analog_index] = torque;

    for(byte idx = 0; idx < Data_Config.USR_no; idx++) {CURRENT_RECORD.USR[idx][analog_index] = adc_readings_slow[ADC_CHN_USR_START+idx];}
    for(byte idx = 0; idx < Data_Config.TMP_no; idx++) {CURRENT_RECORD.USR[idx][analog_index] = adc_readings_slow[ADC_CHN_TMP_START+idx];}

  }

  #ifdef DEBUG_ANALOG_TIME
  timestamp_us = micros() - timestamp_us;
  Serial.print(F("t_ana us: "));
  Serial.println(timestamp_us);
  #endif
}

/* table of calibration values for MAP sensors */
const MAP_CAL ADC_map_cal[NO_OF_MAP_SENSORS] = {{SENSOR_MAP_CAL_MIN_mbar_0,SENSOR_MAP_CAL_MAX_mbar_0},{SENSOR_MAP_CAL_MIN_mbar_1,SENSOR_MAP_CAL_MAX_mbar_1}};

/* applies the calibration for the given map snesor to the given value
 *  returns: calibrated value in mbar 
 */
int ADC_apply_map_calibration(byte map_idx, int value)
{
  #ifdef DEBUG_MAP_CAL
  int pressure_LSBs = value;
  #endif

  //catch out of bounds sensor index
  if(map_idx >= NO_OF_MAP_SENSORS) return -1;
  
  //todo: make this have independant calibrations for each sensor, so we can have 0-1 bar on intake, 0-2/3 bar for exhaust pressure, etc
  value = amap(value, ADC_map_cal[map_idx].min_mbar, ADC_map_cal[map_idx].max_mbar); 
  
  #ifdef DEBUG_MAP_CAL
  Serial.print(idx-ADC_CHN_MAP_START);
  Serial.print(F(" map cal; in, min, max, out: "));
  Serial.print(pressure_LSBs); Serial.print(F(", "));
  Serial.print(ADC_map_cal[map_idx].min_mbar); Serial.print(F(", "));
  Serial.print(ADC_map_cal[map_idx].max_mbar); Serial.print(F(", "));
  Serial.print(value); Serial.print(F(" mbar"));
  Serial.println();
  #endif

  return value;
}

/* ADC ISR for fast sampling */
ISR(ADC_vect)
{
  

#ifdef DEBUG_ADC_ISR
  isr_duration_tk = TCNT1;
  static long tl = micros();
  long t_now = micros();
  long dt = t_now - tl;
  tl = t_now;
  if(isr_print){Serial.print("ISR: ");Serial.print(dt);Serial.print("; ");}
#endif

  int ADC_val = ADC;
  
  //  Accumulate for PID, if this is the selected channel for pid feedback
  if (pid_chn == adc_chn )
  {
    //check for count overflow
    if( pid_count < ADC_PID_COUNT_MAX )
    {
      pid_total += ADC_val;
      pid_count++;
    }
    else //reset the count
    {
      pid_total = ADC_val;
      pid_count = 1;
    }
  }
  
  //store value in record
  if(CURRENT_RECORD.ANA_no_of_fast_samples < NO_OF_ADC_FAST_PER_RECORD)
  {
    CURRENT_RECORD.MAP[adc_chn][CURRENT_RECORD.ANA_no_of_fast_samples] = ADC_val;
  }
  
  //increment channel
  adc_chn++;
  
  //if next channel is valid, start a conversion on that channel
  if(adc_chn < ADC_NO_FAST)
  {
    #ifdef DEBUG_ADC_ISR
    if(isr_print){Serial.print("chn: "); Serial.println(adc_chn);}
    #endif
    
    //start adc
    ADC_set_channel(channels_fast[adc_chn]);
    ADC_START();
  }
  else //otherwise, clear the timer flag, increment the sample and, if last sample, stop the timer
  {
    //clear the timer interrupt flag
    ADC_TMR_CLRF();
    
    //increment ADC process count
    adc_smp++;

    #ifdef DEBUG_ADC_ISR
    if(isr_print){Serial.print("smp: "); Serial.println(adc_smp);}
    #endif
    
    if(CURRENT_RECORD.ANA_no_of_fast_samples < NO_OF_ADC_FAST_PER_RECORD)
    {
      //increment record sample count
      CURRENT_RECORD.ANA_no_of_fast_samples++;
      //reset the channel
      adc_chn = 0;
    }
    else
    {
      //stop the timer
      ADC_TMR_STOP();
    }
    
  }
  #ifdef DEBUG_ADC_ISR
  isr_duration_tk = TCNT1 - isr_duration_tk;
  if(isr_print)Serial.println();
  #endif
}

// end of file
