#define PIN_SAMPLE_GAIN_1 5   //lowest value resistor
#define PIN_SAMPLE_GAIN_2 4
#define PIN_SAMPLE_GAIN_3 3
#define PIN_SAMPLE_GAIN_4 2   //highest value resistor

#define TEST_CH_INPUT            A0
#define TEST_CH_OUTPUT           A1

typedef enum test_type
{
  TT_NONE = 0,
  TT_CALIBRATE,           //a single measurement. provide a custom voltage and check that the output value is correct, enables debug messages
  TT_FULL_SWEEP,          //full sweep through all bit ranges and from 0 to max pwm with decremental step ups, starting at 21
  TT_MEASURE,             //performs a single measurement
  NO_OF_TEST_TYPES
} TEST_TYPE;

//shouldn't be needed outside of test.c, but placed here for tidyness' sake
typedef enum test_stage
{
  TS_IDLE = 0,
  TS_SETUP,          //configure the output as required for the test
  TS_SAMPLE,         //take adc readings of input and output until they are stable
  TS_REPORT,         //report the collected data
  TS_NEXT,           //select the next configuration 
//  TS_DONE,           //final report, test stats elapsed time etc.
  NO_OF_TEST_STAGES
} TEST_STAGE;
