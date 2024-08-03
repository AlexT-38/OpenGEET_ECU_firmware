//#define DEBUG_TEST
//#define DEBUG_TEST_DELAY

#define TEST_CSV_OUTPUT

//ramp test time now uses the configured ramp time.
//#define TEST_RAMP_TIME_ms       1000     //time take to ramp between 0-255
#define TEST_RAMP_TIME_FACTOR     5     //nmultiplier for TEST_RAMP_TIME_ms
#define TEST_DELAY_TIME_ms      2000    //time after setting value before making reading
#define TEST_GAIN_TIME_ms        50    //time for gain change to settle
#define TEST_RESET_TIME_ms       10    //time between checks for Vin to match Vout when resetting pwm to 0
#define TEST_SAMPLE_OVERLOAD      (1022L*TEST_SAMPLES)
#define TEST_SAMPLE_MAX           (1023L*TEST_SAMPLES) //might be 1024?

#define TEST_NO_OF_OUTPUT_GAIN_SETTINGS  6
#define TEST_OUTPUT_UNITY_GAIN_IDX       (TEST_NO_OF_OUTPUT_GAIN_SETTINGS-1)
#define TEST_NO_OF_INPUT_GAIN_SETTINGS  4
#define TEST_INPUT_UNITY_GAIN_IDX       (TEST_NO_OF_INPUT_GAIN_SETTINGS-1)

#define TEST_PWM_INC_INIT        21
#define TEST_PWM_INC_MIN          2

#define TEST_DB(x)      if(test_debug) Serial.print(x);
#define TEST_DBLN(x)    if(test_debug) Serial.println(x);



//measure each resistor and enter the values here
#define TEST_RO_TOP      9.97              // 10.0
#define TEST_RO_4        9.97              // 10.0
#define TEST_RO_3        3.304             // 3.30
#define TEST_RO_2        1.301             // 1.50
#define TEST_RO_1        0.685             // 0.680
#define TEST_RO_ALL      (1.0/((1.0/TEST_RO_1)+(1.0/TEST_RO_2)+(1.0/TEST_RO_3)+(1.0/TEST_RO_4)))

#define TEST_RI_TOP      10.0
#define TEST_RI_2        5.107 
#define TEST_RI_1        1.306 
#define TEST_RI_ALL      (1.0/((1.0/TEST_RI_1)+(1.0/TEST_RI_2)))

//input measured 4.34 (tenma) 4.358 (DM6000) 4.97 (this, input) 4.36 (this, output)
//input measured 6.21 (tenma) 6.23 (DM6000) 6.85 (this, input) 6.23 (this, output)
//actual input to the ADC for input channel 3.427, times the measured ratio = 6.86
//ADC is reading right, but divider is not the expected value? No, it is infact correct.
//DMMs were measuring output, and there's upto a diode drop depending on comparator states.



#define TEST_PD_RATIO(rt, rb) (( (rb) + (rt)) / (rb))

#define TEST_RI_RATIO(rb)     TEST_PD_RATIO(TEST_RI_TOP, rb)
#define TEST_GI_RATIO(n0, n1)  (TEST_RI_RATIO(n1) / TEST_RI_RATIO(n0)) // next range gain relative to current: smaller range / larger range
#define TEST_GI_RATIO_FINAL(n0)  (1.0 / TEST_RI_RATIO(n0))             //final relative range gain

#define TEST_RO_RATIO(rb) TEST_PD_RATIO(TEST_RO_TOP, rb)          // range gain: 1 / (divider ratio)
#define TEST_GO_RATIO(n0, n1)  (TEST_RO_RATIO(n1) / TEST_RO_RATIO(n0)) // next range gain relative to current: smaller range / larger range
#define TEST_GO_RATIO_FINAL(n0)  (1.0 / TEST_RO_RATIO(n0))             //final relative range gain
//gain divider resistors of 10 / [ 10 | 3.3 | 1.5 | 0.68 ], see test_set_output_gain()
//float test_output_gains[] = {26.40285,15.70588,7.66667,4.03030,2.00000,1.00000}; //range gain, multiply sample by this value to get actual value relative to Aref
//float test_output_gain_steps[] = {1.68108, 2.04859, 1.90226, 2.01515, 2.00000};
const float test_output_gains[] =      {TEST_RO_RATIO(TEST_RO_ALL),           TEST_RO_RATIO(TEST_RO_1),
                                        TEST_RO_RATIO(TEST_RO_2),             TEST_RO_RATIO(TEST_RO_3),
                                        TEST_RO_RATIO(TEST_RO_4),             1.00000};
const float test_output_gain_steps[] = {TEST_GO_RATIO(TEST_RO_ALL,TEST_RO_1), TEST_GO_RATIO(TEST_RO_1,TEST_RO_2),
                                        TEST_GO_RATIO(TEST_RO_2,TEST_RO_3),   TEST_GO_RATIO(TEST_RO_3,TEST_RO_4), 
                                        TEST_GO_RATIO_FINAL(TEST_RO_4),       1.00000};

const float test_input_gains[] =      {TEST_RI_RATIO(TEST_RI_ALL),           TEST_RI_RATIO(TEST_RI_1),
                                       TEST_RI_RATIO(TEST_RI_2),             1.00000};
const float test_input_gain_steps[] = {TEST_GI_RATIO(TEST_RI_ALL,TEST_RI_1), TEST_GI_RATIO(TEST_RI_1,TEST_RI_2),
                                       TEST_GI_RATIO_FINAL(TEST_RI_2),       1.00000};



static TEST_TYPE test_type = TT_NONE;
static TEST_STAGE test_stage = TS_IDLE;

static unsigned long TEST_RAMP_TIME_ms;

static byte test_debug = false;

static byte test_output_gain_idx;
static byte test_input_gain_idx;
static byte test_output_overload;
static byte test_input_overload;

static int test_pwm;
static byte test_pwm_inc; //amount to increase pwm by each step, reduce by one each time

static byte test_bits;

unsigned int input_sample;
unsigned int output_sample;

//count of configurations tested
unsigned int test_setup_idx;

unsigned long test_elapsed_time_ms;

#ifdef DEBUG_TEST

const char PROGMEM S_TEST_IDLE[] = "TEST IDLE";
const char PROGMEM S_TEST_SETUP[] = "TEST SETUP";
const char PROGMEM S_TEST_SAMPLE[] = "TEST SAMPLE";
const char PROGMEM S_TEST_REPORT[] = "TEST REPORT";
const char PROGMEM S_TEST_NEXT[] = "TEST NEXT";
const char PROGMEM S_TEST_INVALID[] = "TEST STATE INVALID";

const char * const test_stages_str[] PROGMEM = { //this should prabably be an indexed intialiser
  S_TEST_IDLE,
  S_TEST_SETUP,
  S_TEST_SAMPLE,
  S_TEST_REPORT,
  S_TEST_NEXT
};

#endif
/*** test_is_running()
 *  returns true if a test is running
 */
bool test_is_running(){
  return    test_type != TT_NONE;
}

/*** print_test_time()
 *  prints the time since the a test was last started
 */
void print_test_time(){
  test_elapsed_time_ms = millis() - test_elapsed_time_ms;
  Serial.print(F("Elapsed test time (ms): "));
  Serial.println(test_elapsed_time_ms);
  Serial.println(FS(S_BAR));
}

/*** test_stop()
 *  sets the test state to none/idle and resets gain settings
 */
void test_stop(){
  test_set_output_gain(0);
  test_set_input_gain(0);
  test_type = TT_NONE;
  test_stage = TS_IDLE;
  test_debug = false;
}

/*** test_reset()
 *  stops the test and sets pwm to 0
 */
void test_reset(){
  test_stop();
  set_pwm(0);
}

/*** start_test(TEST_TYPE)
 * command call to start test.
 * params - test type to start
 */
void start_test(TEST_TYPE test_no)
{
  test_type = test_no;
  switch(test_no)
  {
    case TT_CAL_DAC:
      start_dac_cal();
      break;
    case TT_MEASURE:
      test_stage = TS_SETUP;
      test_setup_idx = 0;
      break;
    case TT_CALIBRATE:
      test_stage = TS_SETUP;
      test_pwm = 0;
      test_bits = 8;
      //set_pwm_ramp(TEST_RAMP_TIME_ms);
      TEST_RAMP_TIME_ms = get_pwm_ramp_ms();
      test_setup_idx = 0;
      config.lut = false;
      test_pwm_inc = 0;
      break;
      
    case TT_FULL_SWEEP:
      #ifdef DEBUG_TEST
      Serial.println(F("Full Sweep Test"));
      #endif
      #ifdef TEST_CSV_OUTPUT
      Serial.println(FS(S_CSV_HEADER));
      #endif
      test_stage = TS_SETUP;
      test_pwm = 0;
      test_bits = PWM_BITS_MIN;
      //set_pwm_ramp(TEST_RAMP_TIME_ms);
      TEST_RAMP_TIME_ms = get_pwm_ramp_ms();
      test_setup_idx = 0;
      config.lut = false;
      test_pwm_inc = TEST_PWM_INC_INIT;
      break;
      
    default:
      test_stop();
      break;
  }

  test_elapsed_time_ms = millis();
  Serial.flush();
}

/*** run_test()
 *  called from main loop, runs the currently selected test and stops if serial input detected
 *  returns true if a test is running
 */
byte run_test()
{
  byte going = false;
  switch(test_type)
  {
    case TT_NONE:
      break;
    case TT_CAL_DAC:
      going = run_dac_cal();
      break;
    case TT_MEASURE:
      run_measure_test();
      break;
    case TT_CALIBRATE:
      going = run_calib_test();
      if(!going) 
      {
        print_test_time();
        test_reset();
      }
      break;
    case TT_FULL_SWEEP:
      #ifdef DEBUG_TEST
      {
        Serial.print(F("Running Test: "));
        char buf[50];
        if (test_stage > NO_OF_TEST_STAGES) test_stage = NO_OF_TEST_STAGES;
        READ_STRING_FROM(test_stages_str,test_stage,buf);
        Serial.println(buf);
      }
      #endif
      going = run_sweep_test();
      #ifdef DEBUG_TEST
      {
        Serial.print(F("Going: "));
        Serial.println(going);
      }
      #endif
      if(!going) 
      {
        print_test_time();
        test_reset();
      }
      break;
    default:
      test_stop();
      break;
  }
  
  if(going && test_type != TT_CAL_DAC && Serial.available()>0)
  {
    Serial.println(F("Interrupting test."));
    Serial.flush();
    if(test_type == TT_MEASURE)
      test_stop();
    else
      test_reset();
    going = false;
  }
  
  return going;
}

//------------------------------------------------------------ TEST TYPES
/*** run_measure_test()
 * run a single sample and report
 */
byte run_measure_test()
{
  switch(test_stage)
  {
    case TS_IDLE:
      break;
    case TS_SETUP:
      #ifdef TEST_CSV_OUTPUT
      Serial.println(FS(S_CSV_HEADER));
      #endif
      test_stage = TS_SAMPLE;
      break;
    case TS_SAMPLE:
      test_sample();
      break;
    case TS_REPORT:
      test_report();
      break;
    default:
      test_stop();
      break;
  }
  return (test_stage != TS_IDLE);
}

/*** run_calib_test()
 * print all gains and ratios, then run a single sample with debug messages enabled
 */
byte run_calib_test()
{
  test_debug = true;
  switch(test_stage)
  {
    case TS_IDLE:
      break;
    case TS_SETUP:
      test_print_ratios();
      test_setup();
      break;
    case TS_SAMPLE:
      test_sample();
      break;
    case TS_REPORT:
      test_report();
      break;
    case TS_NEXT:
//      test_sweep_next();
    default:
      test_reset();
      break;
  }
  return (test_stage != TS_IDLE);
}

/*** run_sweep_test()
 *  sweep through all pwm-bitage and pwm-value settings and report reuslts for each
 */
byte run_sweep_test()
{
  switch(test_stage)
  {
    case TS_IDLE:
      break;
    case TS_SETUP:
      test_setup();
      break;
    case TS_SAMPLE:
      test_sample();
      break;
    case TS_REPORT:
      test_report();
      break;
    case TS_NEXT:
      test_sweep_next();
      break;
    default:
      #ifdef DEBUG_TEST
      Serial.println(F("Undefined test state: resetting"));
      #endif
      test_reset();
      break;
  }
  return (test_stage != TS_IDLE);
}

/*** test_ramp_delay()
 *  Delay after changing pwm to allow values to settle
 */
void test_ramp_delay()
{
  unsigned long delay_ms = ((TEST_RAMP_TIME_ms * TEST_RAMP_TIME_FACTOR * (unsigned long)test_pwm_inc)>>8) + TEST_DELAY_TIME_ms;
  #ifdef DEBUG_TEST_DELAY
  {
    Serial.print(F("Delay: "));
    Serial.println(delay_ms);
    Serial.print(F("TEST_RAMP_TIME_ms: "));
    Serial.println(TEST_RAMP_TIME_ms);
    Serial.print(F("TEST_RAMP_TIME_FACTOR: "));
    Serial.println(TEST_RAMP_TIME_FACTOR);
    Serial.print(F("test_pwm_inc: "));
    Serial.println(test_pwm_inc);
    Serial.print(F("product of above: "));
    Serial.println((TEST_RAMP_TIME_ms * TEST_RAMP_TIME_FACTOR * (unsigned long)test_pwm_inc));
    Serial.print(F("div by 256: "));
    Serial.println((TEST_RAMP_TIME_ms * TEST_RAMP_TIME_FACTOR * (unsigned long)test_pwm_inc)>>8);
    Serial.print(F("TEST_DELAY_TIME_ms: "));
    Serial.println(TEST_DELAY_TIME_ms);
  }
  #endif
  delay(delay_ms);
}

/*** test_setup()
 *  Set the initial conditions for a test run
 */
void test_setup()
{
  #ifndef TEST_CSV_OUTPUT
  {
    Serial.println(FS(S_BAR));
    Serial.print(F("Setup no.: "));
    Serial.println(test_setup_idx);
    
    Serial.print(F("\nTEST SETUP (bits, pwm): "));
    Serial.print(test_bits);
    Serial.print(' ');
    Serial.println(test_pwm);
  }
  #endif

  set_pwm(test_pwm);
  test_ramp_delay();
  set_pwm_bits(test_bits);
  
  test_stage = TS_SAMPLE;
  test_setup_idx++;
}
/*** test_report()
 *  print the latest test report to Serial
 */
void test_report()
{
  //calculate some useful values
  float scaled_output = (float)output_sample * test_output_gains[test_output_gain_idx];
  float scaled_input = test_input_gains[test_input_gain_idx] * (float)input_sample;
  float output_ratio = scaled_output / scaled_input;
  float target_ratio = 1/(1-((float)test_pwm/256.0));
  float loss = 100*(1.0-(output_ratio/target_ratio));

  #ifdef TEST_CSV_OUTPUT
  //Serial.println(F("\tIDX, \tBITS, \tPWM, \tIN(lsb), \tOUT(lsb), \tIN(v), \tOUT(v), \tIN(tol), \tOUT(tol), \tOUT(gain), \tRATIO, \rTARG, \tLOSS%"));
  {
    MAKE_STRING(S_COMMA);
    Serial.print(test_setup_idx);
    Serial.print(S_COMMA_str);
    Serial.print(test_bits);
    Serial.print(S_COMMA_str);
    Serial.print(test_pwm);
    Serial.print(S_COMMA_str);
    Serial.print(input_sample);
    Serial.print(S_COMMA_str);
    Serial.print(output_sample);
    Serial.print(S_COMMA_str);
    Serial.print(get_input_voltage());
    Serial.print(S_COMMA_str);
    Serial.print(get_output_voltage());
    Serial.print(S_COMMA_str);
    Serial.print(get_input_voltage_tol());
    Serial.print(S_COMMA_str);
    Serial.print(get_output_voltage_tol());
    Serial.print(S_COMMA_str);
    Serial.print(test_output_gains[test_output_gain_idx]); //I'd like to add input gain here, but that would upset my spreadsheets. It can be infered from tolerance
    Serial.print(S_COMMA_str);
    Serial.print(output_ratio);
    Serial.print(S_COMMA_str);
    Serial.print(target_ratio);
    Serial.print(S_COMMA_str);
    Serial.println(loss);
  }
  #else
  {
    Serial.print(F("\nInput: "));
    Serial.println(input_sample);
    Serial.print(F("Scaled Input: "));
    Serial.println(scaled_input);
    Serial.print(F("Input Range Gain: "));
    Serial.println(test_input_gains[test_input_gain_idx]);
    Serial.print(F("\nOutput: "));
    Serial.println(output_sample);
    Serial.print(F("Output Range Gain: "));
    Serial.println(test_output_gains[test_output_gain_idx]);
    Serial.print(F("Scaled Output: "));
    Serial.println(scaled_output);
    Serial.print(F("Ratio: "));
    Serial.println(output_ratio);
    test_print_voltage(output_sample,test_output_gain_idx);
    Serial.println(FS(S_BAR));
  }
  #endif

  test_stage = TS_NEXT;
}


void test_sweep_next()
{
  //select the next setup
  #ifdef DEBUG_TEST
  Serial.println(F("Selecting next test params"));
  #endif
  test_stage = TS_SETUP;
  
  //check if we can increase PWM
  if(test_pwm < config.pwm_max)
  {
    //increase the pwm
    test_pwm += test_pwm_inc;
    //decrease the increment
    if (test_pwm_inc > TEST_PWM_INC_MIN) test_pwm_inc--;

    #ifdef DEBUG_TEST
    Serial.print(F("Test PWM inc. : "));
    Serial.print(test_pwm);
    Serial.print(FS(S_COMMA));
    Serial.println(test_pwm_inc);
    #endif
  }
  //otherwise reset the pwm back to 0 and change the resolution/freq
  else if (test_bits < PWM_BITS_MAX)
  {
    test_bits++;
    test_pwm = 0;
    test_pwm_inc = TEST_PWM_INC_INIT;
    #ifdef DEBUG_TEST
    Serial.println(F("Test Bits inc. : "));
    Serial.print(test_bits);
    Serial.print(FS(S_COMMA));
    Serial.print(test_pwm);
    Serial.print(FS(S_COMMA));
    Serial.println(test_pwm_inc);
    #endif
    
    //immediately set pwm to zero and wait for output voltage to drop back down to input
    set_pwm(0);
    test_ramp_delay();
    
    unsigned int  count;
    float input, output;
    float err_in = get_input_voltage_tol();
    float err_out = get_output_voltage_tol();
    test_set_output_gain(0);//should already be set, but just to make sure
    test_set_input_gain(0);
    do
    {
      delay(TEST_RESET_TIME_ms);

      SAMPLE sample;

      if (!test_sample_read(&sample)) return;
  
      output_sample = sample.out;
      input_sample = sample.in;
      
      input = get_input_voltage() + err_in;
      output = get_output_voltage() - err_out;
      
      #ifdef DEBUG_TEST
      if (count&0x3FF)
        Serial.println(F("Waiting on Vin to match Vout"));
        Serial.println(input);
        Serial.println(output);
      #endif
    }while((input) < output && ++count != 0);
    #ifdef DEBUG_TEST
    if(count == 0)
      Serial.println(F("Gave up waiting"));
    #endif

    #ifdef TEST_CSV_OUTPUT
    Serial.println(FS(S_CSV_HEADER));
    #endif
    
  }
  else
  {
    Serial.println(F("Test Complete"));
    test_stage = TS_IDLE;
    set_pwm(0);
  }
}

//--------------------------------------------------------------------- TEST SAMPLE
/*** test_sample_read(SAMPLE *)
 *   Reads a number of input and output channel samples into a SAMPLE
 *   params - pointer to the output struct
 *   returns - true if completed without interuption from Serial
 */
bool test_sample_read(SAMPLE *sample)
{
  *sample = (SAMPLE){ 0, 0, UINT16_MAX, 0 };
  
  byte test_sample_idx = 0;
  while(test_sample_idx++ < TEST_SAMPLES)
  {
    sample->in += analogRead(TEST_CH_INPUT); 
    unsigned int new_output_sample = analogRead(TEST_CH_OUTPUT);
    sample->out += new_output_sample;
    
    sample->out_min = min(sample->out_min, new_output_sample);
    sample->out_max = max(sample->out_max, new_output_sample);
    if(Serial.available()>0)
    {
      test_stop();
      return false;
    }
  }
  return true;
}
/*** test_sample()
 *  repeatedly takes sample of input and output channels, until the sample is stable,
 *  and adjusting gain settings as required.
 */
void test_sample()
{
  //testing for stability on the output side only.
  //it will be the least stable, and therefore easier to detect
  #ifdef DEBUG_TEST
  Serial.println(F("Test sample... "));
  #endif
  
  
  test_output_gain_idx = 0;                //global, for Report
  test_input_gain_idx = 0;
  test_set_output_gain(test_output_gain_idx);
  test_set_input_gain(test_output_gain_idx);
  
  byte go = true;
  byte go_input = true;
  byte go_output = true;
  test_output_overload = false;     //global, for Report
  test_input_overload = false;

  unsigned long spread_retry_avg = 0;
  unsigned int spread_retry_count = 0;
  
  //keep looping till we have the final value
  while(go)
  {
    TEST_DBLN();
    SAMPLE sample;

    if (!test_sample_read(&sample)) return;

    output_sample = sample.out;
    input_sample = sample.in;
    
    unsigned int spread = sample.out_max - sample.out_min;
    if(test_debug)
    {
      Serial.print(F("\ntest input sample: "));
      Serial.println(sample.in);
      Serial.print(F("test output sample: "));
      Serial.println(sample.out);
      Serial.print(F("test sample min: "));
      Serial.println(sample.out_min);
      Serial.print(F("test sample max: "));
      Serial.println(sample.out_max);
      Serial.print(F("test sample spread: "));
      Serial.println(spread);
      Serial.print(F("test input range gain : "));
      Serial.println(test_input_gains[test_output_gain_idx]);
      Serial.print(F("test output range gain: "));
      Serial.println(test_output_gains[test_output_gain_idx]);
      Serial.print(F("test input voltage "));
      test_print_voltage(get_input_voltage(), get_input_voltage_tol());
      Serial.print(F("test output voltage"));
      test_print_voltage(get_output_voltage(), get_output_voltage_tol());
      Serial.println();
    }
    // check if sample spread is within specified tolerance
    if (spread < TEST_SAMPLE_TOLERANCE_lsb || spread_retry_count >= TEST_SPREAD_RETRY_MAX)
    {
      if(spread_retry_count < TEST_SPREAD_RETRY_MAX)
      {
        TEST_DBLN(F("Sample spread in tolerance"));
      }
      else
      {
        #ifndef TEST_CSV_OUTPUT
        Serial.println(F("Test sample spread persistantly out of tolerance: using average of retries"));
        #endif
        output_sample = spread_retry_avg/spread_retry_count;
        spread_retry_avg = 0;
        spread_retry_count = 0;
      }
      
      if(go_output) go_output = test_sample_adjust_gain(output_sample, true);
      if(go_input)go_input = test_sample_adjust_gain(input_sample, false);
      go = (go_output || go_input);
    }//spread range out of tolerance
    else
    {
      //start averaging samples. 
      //if not within spec after N retrys, use the average
      TEST_DBLN(F("Sample spread out of range, trying again"));
      spread_retry_avg += output_sample;
      spread_retry_count++;
    }
  }//sampling loop

  //done
  test_stage = TS_REPORT;
  test_set_output_gain(0);
}
/*** test_sample_adjust_gain(uint, bool)
 *  adjusts the input or output channel gain settings based on the given sample
 *  params - sample: the value to use for adjustment
 *           is_output: true if the sample is from the output channel
 *  returns - true if an adjustment has been made, and a new sample should be taken
 */
bool test_sample_adjust_gain(unsigned int sample, bool is_output)
{
  byte * gain_idx =          is_output?   &test_output_gain_idx           : &test_input_gain_idx;
  byte * overload =          is_output?   &test_output_overload           : &test_input_overload;
  void (*set_gain)(byte) =   is_output?   &test_set_output_gain           : &test_set_input_gain;

  const float *gain_steps =        is_output?   &test_output_gain_steps[0]      : &test_input_gain_steps[0];
  byte no_of_gain_settings = is_output?   TEST_NO_OF_OUTPUT_GAIN_SETTINGS : TEST_NO_OF_INPUT_GAIN_SETTINGS;
  no_of_gain_settings--;
  
  TEST_DB(F("gain: "));
  TEST_DB(*gain_idx);
  TEST_DB(F(" / "));
  TEST_DBLN(no_of_gain_settings);
  bool go = true;

   //check for overload, reduce 
  if(sample > TEST_SAMPLE_OVERLOAD)
  {
    TEST_DBLN(F("Sample overloaded"));
    if(*gain_idx)
    { 
      (*gain_idx)--;
      *overload = true;
      (*set_gain)(*gain_idx);
    }
    else 
    {
      TEST_DBLN(F("Stopping this sample"));
      //stop sampling if gain cannot be reduced after an overflow
      go = false;
    }
  }
  //not an overload, but a previous sample run was, so stop here
  else if (*overload)
  {
    TEST_DBLN(F("Sample in range after overload"));
    go = false;
  }
  //check if sample is below the gain change threshold, if there are more gain settings to try
  else if (*gain_idx < (no_of_gain_settings))
  {
    TEST_DBLN(F("Checking against threshold (ratio, lsb): "));
    TEST_DB(gain_steps[*gain_idx]);
    unsigned int threshold = lroundf((float)TEST_SAMPLE_MAX * gain_steps[*gain_idx]);
    TEST_DB(FS(S_COMMA));
    TEST_DBLN(threshold);
    
    if(sample < threshold)
    {
        TEST_DBLN(F("Reducing range"));
        (*gain_idx)++;
        (*set_gain)(*gain_idx);
    }
    else
    {
      TEST_DBLN(F("Accepting this sample"));
      //stop sampling when gain sufficient
      go = false;
    }
  }
  else
  {
    TEST_DBLN(F("Accepting this sample (max gain)"));
    //stop sampling when gain maxed out
    go = false;
  }
  return go;
}

/*** get_sample_voltage(float, float, float)
 *  returns a value in volts, given gain and max value
 */
float get_sample_voltage(float value,  float gain, float max_count){
  return 5.0 * gain * value / max_count;
}

/*** get_output_voltage()
 *  returns output_sample as a voltage
 */
float get_output_voltage(){
  return get_sample_voltage(output_sample, test_output_gains[test_output_gain_idx], (float)TEST_SAMPLE_MAX);
}
/*** get_input_voltage()
 *  returns input_sample as a voltage
 */
float get_input_voltage(){
  return get_sample_voltage(input_sample, test_input_gains[test_input_gain_idx], (float)TEST_SAMPLE_MAX);
}
/*** get_output_voltage_tol()
 *  returns adc accuracy in volts for the current output gain setting
 */
float get_output_voltage_tol(){
  return  test_output_gains[test_output_gain_idx] * (float)TEST_SAMPLES * 2/(float)TEST_SAMPLE_MAX;   //base ADC accuracy is +/- 2 LSB
}
/*** get_input_voltage_tol()
 *  returns adc accuracy in volts for the current input gain setting
 */
float get_input_voltage_tol(){
  return 5.0 * test_input_gains[test_input_gain_idx] * (float)TEST_SAMPLES * 2/(float)TEST_SAMPLE_MAX;   //base ADC accuracy is +/- 2 LSB
}

/*** test_print_voltage(float, float)
 *  print the given voltage and accuracy values to Serial
 */
void test_print_voltage(float voltage, float tol)
{
  Serial.print(FS(S_COLON));
  Serial.print(voltage);
  Serial.print(F("V +/-"));
  Serial.println(tol);
}
/*** test_print_ratios()
 *    print all the gains and ratios for the input and output channels
 */
void test_print_ratios()
{
  Serial.println(FS(S_BAR));
  /* //debugging the ratio function
  Serial.println(F("TEST_RO_RATIO(R):\n"));
  for(float f = 0.1; f<= 16.0; f*=1.189207115)
  {
    Serial.print(f);
    Serial.print(FS(S_COLON));
    Serial.println(TEST_RO_RATIO(f));
  }
  */

  Serial.println(F("\n INPUT:\n"));

  Serial.println(F("RESISTORS:\n"));
  Serial.print(F("Rtop: "));
  Serial.println(TEST_RI_TOP);
  Serial.print(F("Rall: "));
  Serial.println(TEST_RI_ALL);
  Serial.print(F("R1  : "));
  Serial.println(TEST_RI_1);
  Serial.print(F("R2  : "));
  Serial.println(TEST_RI_2);

  Serial.println(F("\nRANGE GAINS / RATIOS:\n"));
  for(byte n; n < TEST_NO_OF_INPUT_GAIN_SETTINGS; n++)
  {
    Serial.print(n);
    Serial.print(FS(S_COLON));
    Serial.print(test_input_gains[n]);
    Serial.print(FS(S_COMMA));
    Serial.println(test_input_gain_steps[n]);
  }

  Serial.println(F("\n OUTPUT:\n"));
  
  Serial.println(F("RESISTORS:\n"));
  Serial.print(F("Rtop: "));
  Serial.println(TEST_RO_TOP);
  Serial.print(F("Rall: "));
  Serial.println(TEST_RO_ALL);
  Serial.print(F("R1  : "));
  Serial.println(TEST_RO_1);
  Serial.print(F("R2  : "));
  Serial.println(TEST_RO_2);
  Serial.print(F("R3  : "));
  Serial.println(TEST_RO_3);
  Serial.print(F("R4  : "));
  Serial.println(TEST_RO_4);
  Serial.println(F("\nRANGE GAINS / RATIOS:\n"));
  for(byte n; n < TEST_NO_OF_OUTPUT_GAIN_SETTINGS; n++)
  {
    Serial.print(n);
    Serial.print(FS(S_COLON));
    Serial.print(test_output_gains[n]);
    Serial.print(FS(S_COMMA));
    Serial.println(test_output_gain_steps[n]);
  }
  
  Serial.println(FS(S_BAR));
}


//-------------------------------------------------------------- I/O
void init_sample_pins()
{
  //default state should be maximum gain range, to minimise risk of over voltaging the analog pin
  digitalWrite(PIN_SAMPLE_GAIN_1,LOW);
  digitalWrite(PIN_SAMPLE_GAIN_2,LOW);
  digitalWrite(PIN_SAMPLE_GAIN_3,LOW);
  digitalWrite(PIN_SAMPLE_GAIN_4,LOW);
  pinMode(PIN_SAMPLE_GAIN_1,OUTPUT);
  pinMode(PIN_SAMPLE_GAIN_2,OUTPUT);
  pinMode(PIN_SAMPLE_GAIN_3,OUTPUT);
  pinMode(PIN_SAMPLE_GAIN_4,OUTPUT);

  digitalWrite(PIN_SOURCE_GAIN_1,LOW);
  digitalWrite(PIN_SOURCE_GAIN_2,LOW);
  pinMode(PIN_SOURCE_GAIN_1,OUTPUT);
  pinMode(PIN_SOURCE_GAIN_2,OUTPUT);

  digitalWrite(PIN_DAC_GAIN, TEST_RANGE_800mV);
  pinMode(PIN_DAC_GAIN, OUTPUT);

  pinMode(PIN_INPUT_1, INPUT);
  pinMode(PIN_INPUT_2, INPUT);
  pinMode(PIN_INPUT_3, INPUT);
  
  analogRead(TEST_CH_INPUT);
  analogRead(TEST_CH_OUTPUT);
  analogRead(TEST_CH_DAC);
}
void test_set_output_gain(byte gain_idx)
{
  /*with a potential divider with the following config
   * 10 / [ 10 | 3.3 | 1.3 | 0.68 ]
   * 
   * has the follwoing useful settings:
   * 
   * setting   gain   change   idx
   * 0 0 0 0   1.00        -   5
   * 1 0 0 0   2.00     2.00   4
   * 0 1 0 0   4.03     2.02   3
   * 0 0 1 0   8.69     2.16   2
   * 0 0 0 1  15.71     1.81   1
   * 1 1 1 1  27.43     1.75   0
   * 
   * we start with the lowest setting, idx 0
   * if the result, when stable, is less than 1/change of full scale,
   * we increment the gain index and repeat the sample
   */
  TEST_DB(F("set_output_gain: "));
  TEST_DBLN(gain_idx);
  //set outputs first so range goes up then down, never down then up
  switch(gain_idx)
  {
  case 0:
    pinMode(PIN_SAMPLE_GAIN_1,OUTPUT);
    pinMode(PIN_SAMPLE_GAIN_2,OUTPUT);
    pinMode(PIN_SAMPLE_GAIN_3,OUTPUT);
    pinMode(PIN_SAMPLE_GAIN_4,OUTPUT);
    break;
  case 1:
    pinMode(PIN_SAMPLE_GAIN_1,OUTPUT);
    pinMode(PIN_SAMPLE_GAIN_2,INPUT);
    pinMode(PIN_SAMPLE_GAIN_3,INPUT);
    pinMode(PIN_SAMPLE_GAIN_4,INPUT);
    break;
  case 2:
    pinMode(PIN_SAMPLE_GAIN_2,OUTPUT);
    pinMode(PIN_SAMPLE_GAIN_1,INPUT);
    pinMode(PIN_SAMPLE_GAIN_3,INPUT);
    pinMode(PIN_SAMPLE_GAIN_4,INPUT);
    break;
  case 3:
    pinMode(PIN_SAMPLE_GAIN_3,OUTPUT);
    pinMode(PIN_SAMPLE_GAIN_1,INPUT);
    pinMode(PIN_SAMPLE_GAIN_2,INPUT);
    pinMode(PIN_SAMPLE_GAIN_4,INPUT);
    break;
  case 4:
    pinMode(PIN_SAMPLE_GAIN_4,OUTPUT);
    pinMode(PIN_SAMPLE_GAIN_1,INPUT);
    pinMode(PIN_SAMPLE_GAIN_2,INPUT);
    pinMode(PIN_SAMPLE_GAIN_3,INPUT);
    break;
  case 5:
    pinMode(PIN_SAMPLE_GAIN_1,INPUT);
    pinMode(PIN_SAMPLE_GAIN_2,INPUT);
    pinMode(PIN_SAMPLE_GAIN_3,INPUT);
    pinMode(PIN_SAMPLE_GAIN_4,INPUT);
    break;
  default:
    return;
  }
  delay(TEST_GAIN_TIME_ms);
}

void test_set_input_gain(byte gain_idx)
{
  /*with a potential divider with the following config
   * 10 / [ 5.1 | 1.3 ]
   * 
   * has the follwoing useful settings:
   * 
   * setting   gain   change   idx
   * 0 0       1.00        -   5
   * 1 0       2.96     2.96   4
   * 0 1       8.69     2.94   3
   * 1 1      10.65     1.23   0
   * 
   * we start with the lowest setting, idx 0
   * if the result, when stable, is less than 1/change of full scale,
   * we increment the gain index and repeat the sample
   */
  TEST_DB(F("set_input_gain: "));
  TEST_DBLN(gain_idx);
  
  switch(gain_idx)
  {
  case 0:
    pinMode(PIN_SOURCE_GAIN_1,OUTPUT);
    pinMode(PIN_SOURCE_GAIN_2,OUTPUT);
    break;
  case 1:
    pinMode(PIN_SOURCE_GAIN_1,OUTPUT);
    pinMode(PIN_SOURCE_GAIN_2,INPUT);
    break;
  case 2:
    pinMode(PIN_SOURCE_GAIN_1,INPUT);
    pinMode(PIN_SOURCE_GAIN_2,OUTPUT);
    break;
  case 3:
    pinMode(PIN_SOURCE_GAIN_1,INPUT);
    pinMode(PIN_SOURCE_GAIN_2,INPUT);
    break;
  default:
    return;
  }
  delay(TEST_GAIN_TIME_ms);
}
