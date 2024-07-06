#define TEST_RAMP_TIME_ms       100     //time take to ramp between 0-255
#define TEST_DELAY_TIME_ms      500    //time after setting value before making reading
#define TEST_GAIN_TIME_ms         2    //time for gain change to settle
#define TEST_SAMPLES             16    //number of samples to take in total
#define TEST_SAMPLE_TOLERANCE_lsb 8   //total min max variance allowable
#define TEST_SAMPLE_OVERLOAD      (1022*TEST_SAMPLES)

#define TEST_NO_OF_GAIN_SETTINGS  6
#define TEST_PWM_INC_INIT        21

//gain divider resistors of 10 / [ 10 | 3.3 | 1.5 | 0.68 ], see test_set_gain()
float test_gains[] = {26.40285,15.70588,7.66667,4.03030,2.00000,1.00000}; //range gain, multiply sample by this value to get actual value relative to Aref
float test_gain_steps[] = {1.68108, 2.04859, 1.90226, 2.01515, 2.00000};

//am I making this more complicated than I need to?
typedef enum test_stage
{
  TS_IDLE = 0,
  TS_SETUP,          //configure the output as required for the test
  TS_SAMPLE,         //take adc readings of input and output until they are stable
  TS_REPORT          //report the collected data
} TEST_STAGE;

//probably won't need this
typedef enum sample_stage
{
  SS_WAIT = 0,
  SS_SAMPLE
} SAMPLE_STAGE;


static TEST_STAGE test_stage = TS_IDLE;

static byte test_gain_idx;
static byte test_sample_overload;



static int test_pwm;
static byte test_pwm_inc; //amount to increase pwm by each step, reduce by one each time

static byte test_bits;

//static int test_samples_input[TEST_SAMPLES];
//static int test_samples_output[TEST_SAMPLES];
//static byte test_sample_idx;

unsigned int input_sample;
unsigned int output_sample;

//count of configurations tested
unsigned int test_setup_idx;


//command call to start test.
void start_test(byte test_no)
{
  test_stage = TS_SETUP;
  test_pwm = 0;
  test_bits = PWM_BITS_MIN;
  set_pwm_ramp(TEST_RAMP_TIME_ms);
  test_setup_idx = 0;
  config.lut = false;
  test_pwm_inc = TEST_PWM_INC_INIT;
}

//called from main loop, could be made non blocking... but why bother?
byte run_test()
{
  switch(test_stage)
  {
    case TS_SETUP:
      test_setup();
      break;
    case TS_SAMPLE:
      test_sample();
      break;
    case TS_REPORT:
      test_report();
      break;
    default:
      test_stage = TS_IDLE;
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
  
  Serial.print(F("\nTEST SETUP: "));
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
  
  Serial.print(F("Input: "));
  Serial.println(input_sample);
  Serial.print(F("Output: "));
  Serial.println(output_sample);
  Serial.print(F("Range Gain: "));
  Serial.println(test_gains[test_gain_idx]);
  Serial.print(F("Scaled Output: "));
  Serial.println(scaled_output);
  Serial.print(F("Ratio: "));
  Serial.println(output_ratio);
  Serial.println(FS(S_BAR));

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
    unsigned int min_sample = UINT16_MAX;
    unsigned int max_sample = 0;

    input_sample = 0;               //globals, for Report
    output_sample = 0;
    
    byte test_sample_idx = 0;
    while(test_sample_idx++ < TEST_SAMPLES)
    {
      //to reduce channel crosstalk, we'll take two samples and scap the first
      analogRead(TEST_CH_INPUT); 
      input_sample += analogRead(TEST_CH_INPUT); 
      analogRead(TEST_CH_OUTPUT);
      output_sample += analogRead(TEST_CH_OUTPUT);
      
      min_sample = min(min_sample, output_sample);//test_sample_output[test_sample_idx]);
      max_sample = max(max_sample, output_sample);//test_sample_output[test_sample_idx]);
    }

    // check if sample range is within specified tolerance
    unsigned int range = max_sample - min_sample;
    if (range < TEST_SAMPLE_TOLERANCE_lsb)
    {
      //check for overload, reduce 
      if(output_sample > TEST_SAMPLE_OVERLOAD)
      {
        if(test_gain_idx)
        { 
          test_gain_idx--;
          test_sample_overload = true;
          test_set_gain(test_gain_idx);
        }
        else 
        {
          //stop sampling if gain cannot be reduced after an overflow
          go = false;
        }
      }
      //not an overload, but a previous sample run was, so stop here
      else if (test_sample_overload)
      {
        go = false;
      }
      //check if sample is below the gain change threshold, if there are more gain settings to try
      else if (test_gain_idx < (TEST_NO_OF_GAIN_SETTINGS-1))
      {
        unsigned int threshold = lroundf((float)(1023*TEST_SAMPLES) / test_gain_steps[test_gain_idx]);
        if(output_sample < threshold)
        {
            test_gain_idx++;
            test_set_gain(test_gain_idx);
        }
        else
        {
          //stop sampling when gain sufficient
          go = false;
        }
      }
      else
      {
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
