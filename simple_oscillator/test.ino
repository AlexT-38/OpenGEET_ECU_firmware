//#define DEBUG_TEST

#define TEST_CSV_OUTPUT

//ramp test time now uses the configured ramp time.
//#define TEST_RAMP_TIME_ms       1000     //time take to ramp between 0-255
#define TEST_RAMP_TIME_FACTOR     5     //nmultiplier for TEST_RAMP_TIME_ms
#define TEST_DELAY_TIME_ms      2000    //time after setting value before making reading
#define TEST_GAIN_TIME_ms        50    //time for gain change to settle
#define TEST_RESET_TIME_ms       10    //time between checks for Vin to match Vout when resetting pwm to 0
#define TEST_SAMPLES             16    //number of samples to take in total
#define TEST_SAMPLE_TOLERANCE_lsb 8   //total min max variance allowable
#define TEST_SAMPLE_OVERLOAD      (1022*TEST_SAMPLES)
#define TEST_SAMPLE_MAX           (1023*TEST_SAMPLES) //might be 1024?
#define TEST_SPREAD_RETRY_MAX     128

#define TEST_NO_OF_GAIN_SETTINGS  6
#define TEST_UNITY_GAIN_IDX       (TEST_NO_OF_GAIN_SETTINGS-1)
#define TEST_PWM_INC_INIT        21
#define TEST_PWM_INC_MIN          2

#define TEST_DB(x)      if(test_debug) Serial.print(x);
#define TEST_DBLN(x)    if(test_debug) Serial.println(x);



//measure each resistor and enter the values here
#define TEST_R_TOP      9.98              // 10.0
#define TEST_R_4        10.01             // 10.0
#define TEST_R_3        (0.3305 + 3.009)  // 3.30
#define TEST_R_2        (0.997 + 0.5095)  // 1.50
#define TEST_R_1        0.679             // 0.680
#define TEST_R_ALL      (1.0/((1.0/TEST_R_1)+(1.0/TEST_R_2)+(1.0/TEST_R_3)+(1.0/TEST_R_4)))

#define TEST_R_RATIO(rb) (( (rb) + TEST_R_TOP) / (rb))          // range gain: 1 / (divider ratio)
#define TEST_G_RATIO(n0, n1)  (TEST_R_RATIO(n1) / TEST_R_RATIO(n0)) // next range gain relative to current: smaller range / larger range
#define TEST_G_RATIO_FINAL(n0)  (1.0 / TEST_R_RATIO(n0))             //final relative range gain
//gain divider resistors of 10 / [ 10 | 3.3 | 1.5 | 0.68 ], see test_set_gain()
//float test_gains[] = {26.40285,15.70588,7.66667,4.03030,2.00000,1.00000}; //range gain, multiply sample by this value to get actual value relative to Aref
//float test_gain_steps[] = {1.68108, 2.04859, 1.90226, 2.01515, 2.00000};
float test_gains[] =      {TEST_R_RATIO(TEST_R_ALL),          TEST_R_RATIO(TEST_R_1),
                           TEST_R_RATIO(TEST_R_2),            TEST_R_RATIO(TEST_R_3),
                           TEST_R_RATIO(TEST_R_4),            1.00000};
float test_gain_steps[] = {TEST_G_RATIO(TEST_R_ALL,TEST_R_1), TEST_G_RATIO(TEST_R_1,TEST_R_2),
                           TEST_G_RATIO(TEST_R_2,TEST_R_3),   TEST_G_RATIO(TEST_R_3,TEST_R_4), 
                           TEST_G_RATIO_FINAL(TEST_R_4),      1.00000};


static TEST_TYPE test_type = TT_NONE;
static TEST_STAGE test_stage = TS_IDLE;

static unsigned long TEST_RAMP_TIME_ms;

static byte test_debug = false;

static byte test_gain_idx;
static byte test_sample_overload;

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

void print_test_time()
{
  test_elapsed_time_ms = millis() - test_elapsed_time_ms;
  Serial.print(F("Elapsed test time (ms): "));
  Serial.println(test_elapsed_time_ms);
  Serial.println(FS(S_BAR));
}
//print all the gains and ratios
void test_print_ratios()
{
  Serial.println(FS(S_BAR));
  /* //debugging the ratio function
  Serial.println(F("TEST_R_RATIO(R):\n"));
  for(float f = 0.1; f<= 16.0; f*=1.189207115)
  {
    Serial.print(f);
    Serial.print(FS(S_COLON));
    Serial.println(TEST_R_RATIO(f));
  }
  */
  Serial.println(F("RESISTORS:\n"));
  Serial.print(F("Rtop: "));
  Serial.println(TEST_R_TOP);
  Serial.print(F("Rall: "));
  Serial.println(TEST_R_ALL);
  Serial.print(F("R1  : "));
  Serial.println(TEST_R_1);
  Serial.print(F("R2  : "));
  Serial.println(TEST_R_2);
  Serial.print(F("R3  : "));
  Serial.println(TEST_R_3);
  Serial.print(F("R4  : "));
  Serial.println(TEST_R_4);
  Serial.println(F("\nRANGE GAINS / RATIOS:\n"));
  for(byte n; n < TEST_NO_OF_GAIN_SETTINGS; n++)
  {
    //this deosnt work?
//    static const char PROGMEM fmt[] = "%d1 : %f2.3,  %f2.3\n";
//    #define STR_BUF_SIZE 255
//    char string[STR_BUF_SIZE];
//    int count = snprintf(string, STR_BUF_SIZE, fmt, n, test_gains[n], test_gain_steps[n]);
//    if(count < STR_BUF_SIZE)
//      Serial.println(string);

    Serial.print(n);
    Serial.print(FS(S_COLON));
    Serial.print(test_gains[n]);
    Serial.print(FS(S_COMMA));
    Serial.println(test_gain_steps[n]);
  }
  Serial.println(FS(S_BAR));
}

void test_reset()
{
  set_pwm(0);
  test_set_gain(0);
  test_type = TT_NONE;
  test_stage = TS_IDLE;
  test_debug = false;
}
//command call to start test.
void start_test(TEST_TYPE test_no)
{
  test_type = test_no;
  switch(test_no)
  {
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
      Serial.println(F("IDX, BITS, PWM, IN, OUT, IN(v), OUT(v), IN(tol%), OUT(tol%), OUT(rng), RATIO, TARG, LOSS(%)"));
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
      test_reset();
      break;
  }

  test_elapsed_time_ms = millis();
  Serial.flush();
}

//called from main loop, could be made non blocking... but why bother?
//none the less, call from main loop and use the return value to decide what other tasks to perform
byte run_test()
{
  byte going = false;

  
  
  switch(test_type)
  {
    case TT_NONE:
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
      Serial.print(F("Running Test: "));
      char buf[50];
      if (test_stage > NO_OF_TEST_STAGES) test_stage = NO_OF_TEST_STAGES;
      READ_STRING_FROM(test_stages_str,test_stage,buf);
      Serial.println(buf);
      #endif
      going = run_sweep_test();
      #ifdef DEBUG_TEST
      Serial.print(F("Going: "));
      Serial.println(going);
      #endif
      if(!going) 
      {
        print_test_time();
        test_reset();
      }
      break;
    default:
      test_reset();
      break;
  }
  
  if(going && Serial.available()>0)
  {
    Serial.println(F("Interrupting test."));
    Serial.flush();
    test_reset();
    going = false;
    //for some reason this is causing the kalibration test to run
  }
  
  return going;
}

//run a single sample
byte run_calib_test()
{
  
  
  //Serial.print(F("test_debug: "));
  //Serial.println(test_debug);

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
void test_ramp_delay()
{
  delay((TEST_RAMP_TIME_ms * TEST_RAMP_TIME_FACTOR * (unsigned long)test_pwm_inc)>>8 + TEST_DELAY_TIME_ms);
}
void test_setup()
{
  
  

  #ifndef TEST_CSV_OUTPUT
  Serial.println(FS(S_BAR));
  Serial.print(F("Setup no.: "));
  Serial.println(test_setup_idx);
  
  Serial.print(F("\nTEST SETUP (bits, pwm): "));
  Serial.print(test_bits);
  Serial.print(' ');
  Serial.println(test_pwm);
  #endif

  set_pwm(test_pwm);
  test_ramp_delay();
  set_pwm_bits(test_bits);
  
  test_stage = TS_SAMPLE;
  test_setup_idx++;
}

void test_report()
{
  //calculate some useful values (could be processed externally, but what the hey)
  float scaled_output = (float)output_sample * test_gains[test_gain_idx];
  float output_ratio = scaled_output / (float)input_sample;
  float target_ratio = 1/(1-((float)test_pwm/256.0));
  float loss = 100*(1.0-(output_ratio/target_ratio));

  #ifdef TEST_CSV_OUTPUT
  //Serial.println(F("\tIDX, \tBITS, \tPWM, \tIN(lsb), \tOUT(lsb), \tIN(v), \tOUT(v), \tIN(tol), \tOUT(tol), \tOUT(gain), \tRATIO, \rTARG, \tLOSS%"));
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
  Serial.print(get_voltage(input_sample,TEST_UNITY_GAIN_IDX));
  Serial.print(S_COMMA_str);
  Serial.print(get_voltage(output_sample,test_gain_idx));
  Serial.print(S_COMMA_str);
  Serial.print(get_voltage_tol(TEST_UNITY_GAIN_IDX)*100);
  Serial.print(S_COMMA_str);
  Serial.print(get_voltage_tol(test_gain_idx)*100);
  Serial.print(S_COMMA_str);
  Serial.print(test_gains[test_gain_idx]);
  Serial.print(S_COMMA_str);
  Serial.print(output_ratio);
  Serial.print(S_COMMA_str);
  Serial.print(target_ratio);
  Serial.print(S_COMMA_str);
  Serial.println(loss);
  #else
  Serial.print(F("\nInput: "));
  Serial.println(input_sample);
  Serial.print(F("Output: "));
  Serial.println(output_sample);
  Serial.print(F("Range Gain: "));
  Serial.println(test_gains[test_gain_idx]);
  Serial.print(F("Scaled Output: "));
  Serial.println(scaled_output);
  Serial.print(F("Ratio: "));
  Serial.println(output_ratio);
  test_print_voltage(output_sample,test_gain_idx);
  Serial.println(FS(S_BAR));
  #endif

  test_stage = TS_NEXT;
}

float get_voltage(float value, byte gain_idx)
{
  return 5.0 * test_gains[gain_idx] * (float)value / (float)TEST_SAMPLE_MAX;
}
float get_voltage_tol(byte gain_idx)
{
  return 5.0 * test_gains[gain_idx] * (float)TEST_SAMPLES*2 /  (float)TEST_SAMPLE_MAX;   //base ADC accuracy is +/- 2 LSB
}
void test_print_voltage(float value, byte gain_idx)
{
  float voltage = get_voltage(value, gain_idx);
  float tol = get_voltage_tol(gain_idx);
  Serial.print(F("Calc. volts: "));
  Serial.print(voltage);
  Serial.print(F("V +/-"));
  Serial.println(tol);
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
    unsigned int input, output, count;
    do
    {
      delay(TEST_RESET_TIME_ms);
      input = analogRead(TEST_CH_INPUT);
      output = analogRead(TEST_CH_OUTPUT);
      #ifdef DEBUG_TEST
      if (count&0x3FF)
        Serial.println(F("Waiting on Vin to match Vout"));
      #endif
    }while(input < output && ++count != 0);
    #ifdef DEBUG_TEST
    if(count == 0)
      Serial.println(F("Gave up waiting"));
    #endif

    #ifdef TEST_CSV_OUTPUT
    Serial.println();
    #endif
    
  }
  else
  {
    Serial.println(F("Test Complete"));
    test_stage = TS_IDLE;
    set_pwm(0);
  }
}

void test_sample()
{
  //testing for stability on the output side only.
  //it will be the least stable, and therfore easier to detect
  #ifdef DEBUG_TEST
  Serial.println(F("Test sample... "));
  #endif
  
  
  test_gain_idx = 0;                //global, for Report
  test_set_gain(test_gain_idx);
  
  byte go = true;
  test_sample_overload = false;     //global, for Report

  unsigned long spread_retry_avg = 0;
  unsigned int spread_retry_count = 0;
  
  //keep looping till we have the final value
  while(go)
  {
    TEST_DBLN();
    unsigned int min_sample = UINT16_MAX;
    unsigned int max_sample = 0;

    input_sample = 0;               //globals, for Report
    output_sample = 0;
    
    byte test_sample_idx = 0;
    while(test_sample_idx++ < TEST_SAMPLES)
    {
      //to reduce channel crosstalk, we'll take two samples and scrap the first
      analogRead(TEST_CH_INPUT); 
      input_sample += analogRead(TEST_CH_INPUT); 
      analogRead(TEST_CH_OUTPUT);
      unsigned int new_output_sample = analogRead(TEST_CH_OUTPUT);
      output_sample += new_output_sample;
      
      min_sample = min(min_sample, new_output_sample);
      max_sample = max(max_sample, new_output_sample);
      if(Serial.available()>0)
      {
        test_reset();
        return;
      }
    }
    unsigned int spread = max_sample - min_sample;
    if(test_debug)
    {
      Serial.print(F("\ntest input sample: "));
      Serial.println(input_sample);
      Serial.print(F("test output sample: "));
      Serial.println(output_sample);
      Serial.print(F("test sample min: "));
      Serial.println(min_sample);
      Serial.print(F("test sample max: "));
      Serial.println(max_sample);
      Serial.print(F("test sample spread: "));
      Serial.println(spread);
      Serial.print(F("test range gain: "));
      Serial.println(test_gains[test_gain_idx]);

      test_print_voltage(input_sample,TEST_UNITY_GAIN_IDX);
      test_print_voltage(output_sample,test_gain_idx);
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
      
      //check for overload, reduce 
      if(output_sample > TEST_SAMPLE_OVERLOAD)
      {
        TEST_DBLN(F("Sample overloaded"));
        if(test_gain_idx)
        { 
          test_gain_idx--;
          test_sample_overload = true;
          test_set_gain(test_gain_idx);
        }
        else 
        {
          TEST_DBLN(F("Stopping this sample"));
          //stop sampling if gain cannot be reduced after an overflow
          go = false;
        }
      }
      //not an overload, but a previous sample run was, so stop here
      else if (test_sample_overload)
      {
        TEST_DBLN(F("Sample in range after overload"));
        go = false;
      }
      //check if sample is below the gain change threshold, if there are more gain settings to try
      else if (test_gain_idx < (TEST_NO_OF_GAIN_SETTINGS-1))
      {
        TEST_DBLN(F("Checking against threshold (ratio, lsb): "));
        TEST_DB(test_gain_steps[test_gain_idx]);
        unsigned int threshold = lroundf((float)TEST_SAMPLE_MAX * test_gain_steps[test_gain_idx]);
        TEST_DB(FS(S_COMMA));
        TEST_DBLN(threshold);
        
        if(output_sample < threshold)
        {
            TEST_DBLN(F("Reducing range"));
            test_gain_idx++;
            test_set_gain(test_gain_idx);
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
}


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
  analogRead(TEST_CH_INPUT);
  analogRead(TEST_CH_OUTPUT);
}
void test_set_gain(byte gain_idx)
{
  /*with a potential divider with the following config
   * 10 / [ 10 | 3.3 | 1.5 | 0.68 ]
   * 
   * has the follwoing useful settings:
   * 
   * setting   gain   change   idx
   * 0 0 0 0   1.00        -   5
   * 1 0 0 0   2.00     2.00   4
   * 0 1 0 0   4.03     2.02   3
   * 0 0 1 0   7.67     1.90   2
   * 0 0 0 1  15.71     2.05   1
   * 1 1 1 1  26.40     1.68   0
   * 
   * we start with the lowest setting, idx 0
   * if the result, when stable, is less than 1/change of full scale,
   * we increment the gain index and repeat the sample
   */

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
      pinMode(PIN_SAMPLE_GAIN_1,INPUT);
      pinMode(PIN_SAMPLE_GAIN_2,OUTPUT);
      pinMode(PIN_SAMPLE_GAIN_3,INPUT);
      pinMode(PIN_SAMPLE_GAIN_4,INPUT);
      break;
    case 3:
      pinMode(PIN_SAMPLE_GAIN_1,INPUT);
      pinMode(PIN_SAMPLE_GAIN_2,INPUT);
      pinMode(PIN_SAMPLE_GAIN_3,OUTPUT);
      pinMode(PIN_SAMPLE_GAIN_4,INPUT);
      break;
    case 4:
      pinMode(PIN_SAMPLE_GAIN_1,INPUT);
      pinMode(PIN_SAMPLE_GAIN_2,INPUT);
      pinMode(PIN_SAMPLE_GAIN_3,INPUT);
      pinMode(PIN_SAMPLE_GAIN_4,OUTPUT);
      break;
    case 5:
      pinMode(PIN_SAMPLE_GAIN_1,INPUT);
      pinMode(PIN_SAMPLE_GAIN_2,INPUT);
      pinMode(PIN_SAMPLE_GAIN_3,INPUT);
      pinMode(PIN_SAMPLE_GAIN_4,INPUT);
      break;
   }
   delay(TEST_GAIN_TIME_ms);
}
