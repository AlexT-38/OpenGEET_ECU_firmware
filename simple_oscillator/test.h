#define PIN_SAMPLE_GAIN_1  5   //lowest value resistor (regardless of net name)
#define PIN_SAMPLE_GAIN_2  4
#define PIN_SAMPLE_GAIN_3  3
#define PIN_SAMPLE_GAIN_4  2   //highest value resistor

#define PIN_SOURCE_GAIN_1  A5   //lowest value resistor
#define PIN_SOURCE_GAIN_2  A4   //highest value resistor

#define TEST_CH_INPUT      A0   //boost sweep test, connect to Vsupply
#define TEST_CH_OUTPUT     A1   //boost sweep test, connect to Vbus

#define TEST_CH_DAC        A2   //comp thresh tests, connect to Vphase

#define PIN_INPUT_1        8      //comp low
#define PIN_INPUT_2        7      //comp hi
#define PIN_INPUT_3        6      //comp hi @ -20V

#define PIN_DAC_GAIN       A7


#define TEST_RANGE_800mV   LOW
#define TEST_RANGE_80mV    HIGH


#define NO_OF_CALIBRATION_POINTS 3

#define TEST_SAMPLES             16    //number of samples to take in total
#define TEST_SAMPLE_TOLERANCE_lsb 8   //total min max variance allowable
#define TEST_SPREAD_RETRY_MAX     128
    

typedef enum test_type
{
  TT_NONE = 0,
  TT_CALIBRATE,           //a single measurement. provide a custom voltage and check that the output value is correct, enables debug messages
  TT_FULL_SWEEP,          //full sweep through all bit ranges and from 0 to max pwm with decremental step ups, starting at 21
  TT_MEASURE,             //performs a single measurement
  TT_CAL_DAC,             //calibrates the DAC and its monitor ADC channel
  TT_COMPARATOR_LO,       //tests the UUT's upper and lower comparator thresholds at 0v, using J9 Test Connector
  TT_COMPARATOR_HI,       //tests the UUT's upper comparator thresholds at -20V
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

typedef struct calibration_points
{
  unsigned int dac[NO_OF_CALIBRATION_POINTS];      //dac setting for the following measurments
  unsigned int adc[NO_OF_CALIBRATION_POINTS];      //adc measurement of the DAC output
  float mV[NO_OF_CALIBRATION_POINTS];              //DMM measurement of the DAC output
} CALIBRATION_POINTS;

typedef struct calibration
{
  byte points;
  CALIBRATION_POINTS range[2]; //calibration points for DAC, high and low ranges
} CALIBRATION;

CALIBRATION calibration;

typedef struct sample
{
  unsigned int in, out;
  unsigned int out_min, out_max;
}SAMPLE;
