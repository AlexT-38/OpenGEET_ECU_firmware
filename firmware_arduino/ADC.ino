
/* ADC handling.
 *  After the first sample, it takes 13 Tclk(adc) ticks to complete a conversion
 *  at 16Mhz with 128 prescaler, Tclk(adc) = 104 us     measured: 108us
 *  to sample all sixteen channels takes 1664 us        measured: 1892 (analogRead)
 *  to sample all sixteen channels twice takes 3328 us  measured: 3692 (analogRead)
 *  we can read external digital adcs (ie torque gauge) while this is happenening
 *  process_adc has 10ms to complete before the next scheduled task
 */

//configuration

#define ADC_CHANNELS                  16//(ADC_CHN_TMP_END+1)        // number of ADC inputs to scan
#define ADC_MAP                   // enable remapping of inputs - only required if using a sheild that blocks access to some ADC pins, or not using an R3 board with dedicated SDA/SCL pins
#define ADC_MULTISAMPLE           //enable sampling the ADC multiple times and taking the average. 
                                  //this feature needs some thought, as it may be better to sample over time using a timer to trigger a sampling run
                                  //or it may be better to retain the extra bits from oversampling instead of rounding back down to 10bit
                                  //or it could just be a complete waste of time.


// sensor channel mapping
//user inputs will be mapped to analog channels starting from
#define ADC_CHN_USR_START             0
#define ADC_CHN_USR_END               (ADC_CHN_USR_START+NO_OF_USER_INPUTS-1)
//MAP sensors will start from
#define ADC_CHN_MAP_START             (ADC_CHN_USR_END+1)
#define ADC_CHN_MAP_END               (ADC_CHN_MAP_START+NO_OF_MAP_SENSORS-1)
//TMP sensors will start from
#define ADC_CHN_TMP_START             (ADC_CHN_MAP_END+1)     
#define ADC_CHN_TMP_END               (ADC_CHN_TMP_START+NO_OF_TMP_SENSORS-1)



int ADC_results[ADC_CHANNELS];
byte ADC_channel;
volatile bool ADC_complete;

#ifdef ADC_MULTISAMPLE 
#define ADC_EXTRA_BITS  1
#define ADC_SAMPLES _BV(ADC_EXTRA_BITS)
byte ADC_sample;
#endif

#ifdef ADC_MAP
static byte ADC_channel_map[ADC_CHANNELS] = {1,2,0};
#define ADC_SELECT  ADC_channel_map[chn]
#else
#define ADC_SELECT  chn
#endif

void configureADC()
{
  //disable the digital inputs
  DIDR0 = 0xFF;
  DIDR1 = 0xFF;
  PORTF=0x0;
  PORTK=0x0;
  DDRF=0;
  DDRK=0;

  analogRead(A0);
}

void process_analog_inputs()
{
  #ifdef DEBUG_ANALOG_TIME
  unsigned int timestamp_us = micros();
  #endif

  //read the torque sensor while we wait for the ADC to catch complete.
  int torque = torqueRead(); //while technically a 'digital' sensor (in terms of interface), the update rate is 10Hz, which matches with the analog update rate, and not the MAX6675 update rate
                             //the torque data is always one sample behind, since the conversion takes 100ms


  
  #ifndef ADC_MULTISAMPLE
  for(byte chn = 0; chn < ADC_CHANNELS; chn++)
  {
    ADC_results[chn] = analogRead(A0+ADC_SELECT);
  }
  #else
  //clear the results table
  memset(ADC_results,0,sizeof(ADC_results));
  
  for(byte smp = 0; smp < ADC_SAMPLES; smp++)
  {
    for(byte chn = 0; chn < ADC_CHANNELS; chn++)
    {
      ADC_results[chn] += analogRead(A0+ADC_SELECT);
    }
  }
  for (int chn = 0; chn < ADC_CHANNELS; chn++)
  {
    ADC_results[chn] = (ADC_results[chn]+_BV(ADC_EXTRA_BITS-1)) >> ADC_EXTRA_BITS;
  }
  #endif
  


  #ifdef DEBUG_ADC
  Serial.println(F("ADC results:"));
  for (int chn = 0; chn < ADC_CHANNELS; chn++)
  {
    Serial.print(chn);
    Serial.print(FS(S_COLON));
    Serial.println(ADC_results[chn]);
  }
  Serial.println();
  #endif

  //copy user inputs to knob values todo: make this make sense - PID's should probably take their target values direct from ADC_results
  for(byte knob=0; knob<NO_OF_KNOBS; knob++)
  {
    KNOB_values[knob] = ADC_results[ADC_CHN_USR_START+knob];
  }


  // get calibrated MAP sensor reading
  for(byte idx =ADC_CHN_MAP_START; idx<(ADC_CHN_MAP_END); idx++)
  {
    #ifdef DEBUG_MAP_CAL
    int pressure_LSBs = ADC_results[idx];
    #endif
    
    //todo: make this have independant calibrations for each sensor, so we can have 0-1 bar on intake, 0-2/3 bar for exhaust pressure, etc
    ADC_results[idx] = amap(ADC_results[idx], SENSOR_MAP_CAL_MIN_mbar, SENSOR_MAP_CAL_MAX_mbar); 
    
    #ifdef DEBUG_MAP_CAL
    Serial.print(idx-ADC_CHN_MAP);
    Serial.print(F(" map cal; in, min, max, out: "));
    Serial.print(pressure_LSBs); Serial.print(F(", "));
    Serial.print(SENSOR_MAP_CAL_MIN_mbar); Serial.print(F(", "));
    Serial.print(SENSOR_MAP_CAL_MAX_mbar); Serial.print(F(", "));
    Serial.print(ADC_results[idx]); Serial.print(F(" mbar"));
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
    int temperature_LSBs = ADC_results[idx];
    #endif
    
    //todo: make this have independant calibrations for each sensor, so we can have 0-1 bar on intake, 0-2/3 bar for exhaust pressure, etc
    ADC_results[idx] = amap(ADC_results[idx], SENSOR_TMP_CAL_MIN_degC, SENSOR_TMP_CAL_MAX_degC); 
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
  if (CURRENT_RECORD.ANA_no_of_samples < ANALOG_SAMPLES_PER_UPDATE)
  {
    //log calibrated data
    unsigned int analog_index = CURRENT_RECORD.ANA_no_of_samples++;

    CURRENT_RECORD.TRQ[analog_index] = torque;

    for(byte idx = 0; idx < Data_Config.USR_no; idx++) {CURRENT_RECORD.USR[idx][analog_index] = ADC_results[ADC_CHN_USR_START+idx];}
    for(byte idx = 0; idx < Data_Config.MAP_no; idx++) {CURRENT_RECORD.MAP[idx][analog_index] = ADC_results[ADC_CHN_MAP_START+idx];}
    for(byte idx = 0; idx < Data_Config.TMP_no; idx++) {CURRENT_RECORD.USR[idx][analog_index] = ADC_results[ADC_CHN_TMP_START+idx];}

  }

  #ifdef DEBUG_ANALOG_TIME
  timestamp_us = micros() - timestamp_us;
  Serial.print(F("t_ana us: "));
  Serial.println(timestamp_us);
  #endif
}
