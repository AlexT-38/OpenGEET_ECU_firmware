#define TEST_RAMP_TIME_ms       100     //time take to ramp between 0-255
#define TEST_DELAY_TIME_ms      500    //time after setting value before making reading
#define TEST_GAIN_TIME_ms         2    //time for gain change to settle
#define TEST_SAMPLES             16    //number of samples to take in total
#define TEST_SAMPLE_TOLERANCE_lsb 8   //total min max variance allowable
#define TEST_SAMPLE_OVERLOAD      (1022*TEST_SAMPLES)
#define TEST_SAMPLE_MAX           (1023*TEST_SAMPLES) //might be 1024?

#define TEST_NO_OF_GAIN_SETTINGS  6
#define TEST_UNITY_GAIN_IDX       (TEST_NO_OF_GAIN_SETTINGS-1)
#define TEST_PWM_INC_INIT        21

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
      set_pwm_ramp(TEST_RAMP_TIME_ms);
      test_setup_idx = 0;
      config.lut = false;
      test_pwm_inc = 0;
      break;
      
    case TT_FULL_SWEEP:
      test_stage = TS_SETUP;
      test_pwm = 0;
      test_bits = PWM_BITS_MIN;
      set_pwm_ramp(TEST_RAMP_TIME_ms);
      test_setup_idx = 0;
      config.lut = false;
      test_pwm_inc = TEST_PWM_INC_INIT;
      break;
      
    default:
      test_reset();
      break;
  }

  test_elapsed_time_ms = millis();
  
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
      going = run_sweep_test();
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
    default:
      test_reset();
      break;
  }
  return (test_stage != TS_IDLE);
}
void test_setup()
{
  set_pwm_bits(test_bits);
  set_pwm(test_pwm);

  Serial.println(FS(S_BAR));
  Serial.print(F("Setup no.: "));
  Serial.println(test_setup_idx);
  
  Serial.print(F("\nTEST SETUP (bits, pwm): "));
  Serial.print(test_bits);
  Serial.print(' ');
  Serial.println(test_pwm);
  
  delay(TEST_RAMP_TIME_ms + TEST_DELAY_TIME_ms);
  test_stage = TS_SAMPLE;
  test_setup_idx++;
}

void test_report()
{
  //calculate some useful values (could be processed externally, but what the hey)
  float scaled_output = (float)output_sample * test_gains[test_gain_idx];
  float output_ratio = scaled_output / (float)input_sample;
  
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

  test_stage = TS_NEXT;
}

void test_print_voltage(float value, byte gain_idx)
{
  float voltage = 5.0 * test_gains[gain_idx] * (float)value / (float)TEST_SAMPLE_MAX;
  float err = 5.0 * test_gains[gain_idx] * (float)TEST_SAMPLES*2 /  (float)TEST_SAMPLE_MAX;   //base ADC accuracy is +/- 2 LSB
  Serial.print(F("Calc. volts: "));
  Serial.print(voltage);
  Serial.print(F("V +/-"));
  Serial.println(err);
}
void test_sweep_next()
{
  //select the next setup

  //check if we can increase PWM
  if(test_pwm < config.pwm_max)
  {
    //increase the pwm
    test_pwm += test_pwm_inc;
    //decrease the increment
    if (test_pwm_inc > 1) test_pwm_inc--;
  }
  //otherwise reset the pwm back to 0 and change the resolution/freq
  else if (test_bits < PWM_BITS_MAX)
  {
    test_bits++;
    test_pwm = 0;
    test_pwm_inc = TEST_PWM_INC_INIT;
  }
  else
  {
    Serial.println(F("Test Complete"));
    test_stage = TS_IDLE;
  }
}

void test_sample()
{
  //testing for stability on the output side only.
  //it will be the least stable, and therfore easier to detect
  
  
  
  test_gain_idx = 0;                //global, for Report
  test_set_gain(test_gain_idx);
  
  byte go = true;
  test_sample_overload = false;     //global, for Report

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
    if (spread < TEST_SAMPLE_TOLERANCE_lsb)
    {
      TEST_DBLN(F("Sample spread in tolerance"));
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
    }//range tolerance
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
