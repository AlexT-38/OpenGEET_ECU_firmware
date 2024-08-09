/*
 * Comparator Resistor Network Balancing
 * 
 * Resistor networks are balance by measuring the threshold
 * voltages and entering the results into a spreadsheet calculator.
 * 
 * Presently only the switch-off threshold voltage has a calculator.
 * Both on and off thresholds will be measured.
 * 
 * The FET driver should be replaced with two links to hold the FETs off.
 * Only the comparators need to be connected.
 * The test must be run twice, once with Vbus and V0 shorted to GND,
 * and the second time with V0 connected to -20V (or more)
 * The nescessary connection are provided on two headers.
 * Once the first test is complete, power down and connect to the 
 * second header.
 * 
 * The procedure is essentially a binary search.
 * The pwm output drives a simple DAC to bias the comparator inputs
 * Once a threshold has been passed, the output must be set past the
 * other threshold to reset.
 * 
 * ADC feedback from the the DAC allows the DAC to be calibrated and 
 * results verified.
 * The DAC can be higher resolution than the ADC, so the DAC value should 
 * be taken as authorative.
 * 
 * Calibration requires only that the DAC output and ADC input be 
 * shorted together, then run the calibration process. The DAC will be set
 * to +,0,- FSD (and maybe 50% points also) and the ADC read. 
 * The DAC should be monitored with a DMM, and the resulting values 
 * should be entered via serial comms. 
 * The DAC and ADC have individual calibrations from the same 16 bit pwm value.
 * 
 * Note that the calibration is only valid for a single invert/negate setting.
 * 
 * 
 * DAC Calibration process:
 * 1) connect the following pairs of pins on the test headers
 *    J9 or J10 : pin 7 & 10
 * 2) connect a good DMM set to Vdc to J9/10 pin7 and ground (J8-3, J9/10-3/8)
 * 3) run k1
 * 4) enter values read from the DMM when requested
 * 
 * DAC Verification:
 * 1) use the same hardware setup as above
 * 2) clear the terminal and run k3, copy the results to the analysis spreadsheet
 * 3) clear and run k4, copy the results to the analysis spreadsheet
 * 4) run k2 to toggle the range and repeat steps (2) and (3)
 * 
 * 
 * Comp threshold test:
 * 1) take a board with the FET driver chip removed or disabled 
 *     (this can be implemented as part of the test harness by looping GND to SD on the UIT connector,
 *      or by shorting pins 8&9, 12&14  Board top, corner opposite PHASE  top row left is 14, L;:;::;;   )
 * 2) connect the test harness to the UIT, and then to the 0V or -V test headers
 * 3) ensure range is 800mV with command k2
 * 4) run k3 and check the threshold results, 
 *      these should be switch-on states at around 450mV, ignore switch-offs
 * 5) run k4 and check the threshold results
 * 6) use k2 to switch to 80mV range
 * 7) run k3 and check the threshold results, 
 *      these should be switch-off states at around 0.0mV, switch-ons shouldn't occur
 * 8) it may be nescessary to switch ranges again to trip the comparator into the on state
 * 9) run k4 and check the threshold results
 * 10) enter the results into the divider adjustment calculator spreadsheet,
 *       use both the DAC and ADC values and check if the resulting shunt resistors are the same,
 * 11) if not, use the d command to manually set the comp state 
 * 12) check the comp state with the m2 command (TODO: implement m2 command)
 *     
 * 
 */
//#define DEBUG_DAC_CAL
//#define DEBUG_DAC_CAL_VERIFY
//#define DEBUG_DAC_MAP



#define DAC_BITS 12
#define DAC_STEP (1<<(16-DAC_BITS))

#define DAC_VERIFY_READ_COUNT_MAX 128

//number of ADC LSB's to offset from end of range, in 16bit
#define DAC_CAL_END_OFFSET (UINT16_MAX - (8 * (1<<16-10)))
#define DAC_CAL_START_OFFSET (4544)

#define DAC_GAIN_CHANGE_TIME_ms   50
#define DAC_VALUE_CHANGE_TIME_us  500 //time per unit dac change
#define DAC_VALUE_CHANGE_TIME_ms  100 //fixed time

byte dac_range = TEST_RANGE_80mV;

unsigned int dac_sample;  //value read from the adc
unsigned int dac_value;   //value set with pwm
float dac_voltage;        //value entered via serial, or interpolated from cal data
float dac_sample_voltage; //interpolated from dac sample
float dac_adc_tol;

bool comp_input[3];
bool comp_input_old[3];
bool comp_changed[3];
float comp_threshold[3];
float comp_threshold_sample[3];
bool dac_verify_up;

void dac_get_adc_tol_mV()
{
  unsigned int a = dac_sample + ADC_TOL;
  unsigned int b = dac_sample - ADC_TOL;
  float a_mV, b_mV;
  if(dac_range == TEST_RANGE_80mV)
  {
    a_mV = dac_interpolate(a, &calibration.range[1].adc[0], &calibration.range[1].mV[0], calibration.points);
    b_mV = dac_interpolate(b, &calibration.range[1].adc[0], &calibration.range[1].mV[0], calibration.points);
  }
  else
  {
    a_mV = dac_interpolate(a, &calibration.range[0].adc[0], &calibration.range[0].mV[0], calibration.points);
    b_mV = dac_interpolate(b, &calibration.range[0].adc[0], &calibration.range[0].mV[0], calibration.points);
  }

  dac_adc_tol = abs(a_mV - b_mV);
  
}
void start_dac_verify(bool up)
{
  Serial.println(F("Starting DAC verify"));
  Serial.println(FS(S_DAC_CAL_VERIFY_CSV_HEADER));
  //temporarily pull up these pins so that if they are not connected, they will hopefully read consistantly
  digitalWrite(PIN_INPUT_1, HIGH);
  digitalWrite(PIN_INPUT_2, HIGH);
  digitalWrite(PIN_INPUT_1, LOW);
  digitalWrite(PIN_INPUT_2, LOW);

  set_pwm_bits(DAC_BITS);
  dac_verify_up = up;
  if(up){
    test_setup_idx = DAC_CAL_START_OFFSET;
    set_dac(0);
  }
  else{
    test_setup_idx = DAC_CAL_END_OFFSET;
    set_dac(UINT16_MAX);
  }
  
  dac_read_sample();
  comp_changed[0] = false;
  comp_changed[1] = false;
  comp_changed[2] = false;
  
  test_stage = TS_SETUP;
}
bool run_dac_verify()
{
  switch(test_stage)
  {
    case TS_IDLE:
      break;
    case TS_SETUP:
      dac_verify_setup();
      break;
    case TS_SAMPLE:
      dac_verify_sample();
      break;
    case TS_REPORT:
      dac_verify_report();
      break;
    case TS_NEXT:
      dac_verify_next();
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
static unsigned int dac_verify_count = 0; 
void dac_verify_setup(){
  set_dac(test_setup_idx);
  test_stage = TS_SAMPLE;
  #ifdef DEBUG_DAC_CAL_VERIFY
    Serial.print(F("DAC Verify Setup: "));
    Serial.println(test_setup_idx);
  #endif
  dac_verify_count = 0;
}

void dac_verify_sample(){
  #ifdef DEBUG_DAC_CAL_VERIFY
  if(dac_verify_count == 0)
    Serial.println(F("DAC Read Sample"));
//  else
//    Serial.println(dac_verify_count);
  #endif
  dac_read_sample();
  dac_calculate_voltages();
  for(byte n = 0; n < 3; n++)
  {
    if(comp_input[n] != comp_input_old[n])
    {
      comp_changed[n] = true;
      comp_threshold[n] = dac_voltage;
      comp_threshold_sample[n] = dac_sample_voltage;
    }
  }
  
  float dac_adc_err = abs(dac_sample_voltage - dac_voltage);
  if(dac_adc_err < dac_adc_tol || abs(dac_voltage) > 600.0  )
    test_stage = TS_REPORT;
  else if (++dac_verify_count > DAC_VERIFY_READ_COUNT_MAX)
  {
    #ifdef DEBUG_DAC_CAL_VERIFY
    Serial.println(F("ADC not within tol"));
    #endif
    test_stage = TS_REPORT;
  }
}
void dac_verify_report(){
  #ifdef DEBUG_DAC_CAL_VERIFY
  Serial.println(F("DAC Report"));
  #endif
  dac_report_csv();
  test_stage = TS_NEXT;
}
void dac_verify_next(){
  test_setup_idx += dac_verify_up?DAC_STEP:-DAC_STEP;
  #ifdef DEBUG_DAC_CAL_VERIFY
  Serial.print(F("DAC Next: "));
  Serial.println(test_setup_idx);
  #endif
  if( ((test_setup_idx > (DAC_CAL_END_OFFSET)) && dac_verify_up) || 
      ((test_setup_idx < (DAC_CAL_START_OFFSET)) && !dac_verify_up) )
  {
    test_stage = TS_IDLE;
    print_comp_thresholds();
    print_test_time();
  }
  else test_stage = TS_SETUP;
}

void print_comp_thresholds()
{
  Serial.println(F("\nComp Thresholds:"));
  for(byte n = 0; n < 3; n++)
  {
    if(comp_changed[n])
    {
      Serial.print(n);
      Serial.print(FS(S_COMMA));
      Serial.print(comp_threshold[n]);
      Serial.print(FS(S_COMMA));
      Serial.println(comp_threshold_sample[n]);
    }
  }
  Serial.println(FS(S_BAR));
}

unsigned int get_dac_value()
{
  return dac_value;
}
/*** dac_interpolate(value, *ref, *cal, points)
 * returns a float interpolated from *cal and *ref using value
 */
float dac_interpolate(unsigned int value, unsigned int *ref, float  *cal, byte points)
{
  //reference values should be ascending, but we can account for otherwise
  bool invert = false;
  byte top = points-1;
  if(ref[0] > ref[top]) invert = true;

  #ifdef DEBUG_DAC_MAP
  Serial.print(F("dac_interpolate: "));
  Serial.println(value);
  #endif

  //find the index of the 1st ref value larger than value
  byte n;
  for (n = 0; n < points; n++)
  {
    byte idx = invert ? top-n : n;
    //break out of the loop once we reach a ref value greater than the target value
    if ( value < ref[idx] ) break; 
  }
  // account for values greater than top value and smaller than bot value (extrapolate)
  if ( n == 0) n = 1;
  if ( n == points) n = top;
  byte m = n-1;

  //account for inversion to get correct array indices
  n = invert ? top-n : n;
  m = invert ? top-m : m;

  //interpolate from the ref and cal data using the above indices
  float result = dac_cal_map((float)value, (float)ref[m], (float)ref[n], cal[m], cal[n]);

  #ifdef DEBUG_DAC_MAP
  Serial.print(m);
  Serial.print(FS(S_ARROW));
  Serial.print(n);
  Serial.print(FS(S_COLON));
  Serial.print(ref[m]);
  Serial.print(FS(S_ARROW));
  Serial.print(ref[n]);
  Serial.print(FS(S_COLON));
  Serial.print(cal[m]);
  Serial.print(FS(S_ARROW));
  Serial.println(cal[n]);
  Serial.print(F("Result: "));
  Serial.println(result);
  #endif

  
  return result;
}
float dac_cal_map(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
void dac_calculate_voltages()
{
  if(dac_range == TEST_RANGE_80mV)
  {
    dac_voltage        = dac_interpolate(dac_value, &calibration.range[1].dac[0], &calibration.range[1].mV[0], calibration.points);
    dac_sample_voltage = dac_interpolate(dac_sample, &calibration.range[1].adc[0], &calibration.range[1].mV[0], calibration.points);
  }
  else
  {
    dac_voltage        = dac_interpolate(dac_value, &calibration.range[0].dac[0], &calibration.range[0].mV[0], calibration.points);
    dac_sample_voltage = dac_interpolate(dac_sample, &calibration.range[0].adc[0], &calibration.range[0].mV[0], calibration.points);
  }
  dac_get_adc_tol_mV();
}

byte dac_read_sample()
{
  #ifdef DEBUG_DAC_CAL
  Serial.println(F("dac_read_sample"));
  #endif
  byte dac_sample_idx = 0;
  dac_sample = 0;

  unsigned int adc_min = UINT16_MAX;
  unsigned int adc_max = 0;
  unsigned int adc_spread = adc_min;
  byte attempts = 0;

  comp_input_old[0] = comp_input[0];
  comp_input_old[1] = comp_input[1];
  comp_input_old[2] = comp_input[2];
  
  while (attempts < TEST_SPREAD_RETRY_MAX && adc_spread > TEST_SAMPLE_TOLERANCE_lsb)
  {
    comp_input[0] = digitalRead(PIN_INPUT_1);
    comp_input[1] = digitalRead(PIN_INPUT_2);
    comp_input[2] = digitalRead(PIN_INPUT_3);
    while(dac_sample_idx++ < TEST_SAMPLES)
    {
      unsigned int new_sample = analogRead(TEST_CH_DAC);
      dac_sample += new_sample;
      adc_min = min(adc_min, new_sample);
      adc_max = max(adc_max, new_sample);
    }
    adc_spread = adc_max - adc_min;
    attempts++;
    #ifdef DEBUG_DAC_CAL
    Serial.print(F("attempts: "));
    Serial.println(attempts);
    #endif
  }
  return attempts;
}
void set_dac(unsigned int new_dac_value)
{
  //truncate to resolution
  new_dac_value >>= (16-DAC_BITS);
  new_dac_value <<= (16-DAC_BITS);
  
  #ifdef DEBUG_DAC_CAL
  Serial.print(F("set_dac ")); Serial.println(new_dac_value);
  #endif

  set_pwm_bits(DAC_BITS);
  force_pwm_w(new_dac_value);
  if(new_dac_value != dac_value)
  {
    unsigned int change = (dac_value > new_dac_value) ? (dac_value - new_dac_value) : (new_dac_value - dac_value);
    delayMicroseconds(((unsigned long)change * DAC_VALUE_CHANGE_TIME_us) + (1000*DAC_VALUE_CHANGE_TIME_ms));
    dac_value = new_dac_value;
  }
}
void tog_dac_gain()
{
  if(dac_range == TEST_RANGE_800mV) {set_dac_gain(TEST_RANGE_80mV);}
  else {set_dac_gain(TEST_RANGE_800mV);}
}
void set_dac_gain(byte range)
{
  #ifdef DEBUG_DAC_CAL
  Serial.print(F("set_dac_gain ")); Serial.println(range);
  #endif
  if(range != dac_range)
  {
    digitalWrite(PIN_DAC_GAIN, range);
    delay(DAC_GAIN_CHANGE_TIME_ms);
    dac_range = range;
  }
}

void dac_report()
{
  Serial.print(dac_value);
  Serial.print(FS(S_COLON));
  Serial.println(dac_voltage);
  Serial.print(dac_sample);
  Serial.print(FS(S_COLON));
  Serial.print(dac_sample_voltage);
  Serial.print(F(" +/- "));
  Serial.println(dac_adc_tol);
  Serial.print(F("Inputs: "));
  Serial.print(FS(S_COMMA));
  Serial.print(comp_input[0]);
  Serial.print(FS(S_COMMA));
  Serial.print(comp_input[1]);
  Serial.print(FS(S_COMMA));
  Serial.println(comp_input[2]);
}
void dac_report_csv()
{
  //export_dac_cal_row(&Serial, get_pwm_w(), dac_sample, 0);
  
  Serial.print(dac_value);
  Serial.print(FS(S_COMMA));
  Serial.print(dac_voltage);
  Serial.print(FS(S_COMMA));
  Serial.print(dac_sample);
  Serial.print(FS(S_COMMA));
  Serial.print(dac_sample_voltage);
  Serial.print(FS(S_COMMA));
  Serial.print(dac_adc_tol);
  Serial.print(FS(S_COMMA));
  Serial.print(comp_input[0]);
  Serial.print(FS(S_COMMA));
  Serial.print(comp_input[1]);
  Serial.print(FS(S_COMMA));
  Serial.println(comp_input[2]);
  
}

void export_dac_cal_row(Stream *dst, unsigned int dac, unsigned int adc, float mV)
{
  dst->print(dac);
  dst->print(FS(S_COLON));
  dst->print(adc);
  dst->print(FS(S_COMMA));
  dst->println(mV);
}

void export_dac_cal_set(Stream *dst, CALIBRATION_POINTS *cal, byte points)
{
  dst->println(FS(S_DAC_HEADER));
  for(byte n = 0; n < points; n++)
  {
    export_dac_cal_row(dst, cal->dac[n] , cal->adc[n],cal->mV[n]);
  }
}

void export_dac_cal(Stream *dst, CALIBRATION *cal)
{
  if((cal->points == 0) || (cal->points == 0xff)){
    dst->println(F("No Cal data"));
  }

  else{
    dst->println(F("800mV"));
    export_dac_cal_set(dst, &cal->range[0], cal->points);
    dst->println(F("80mV"));
    export_dac_cal_set(dst, &cal->range[1], cal->points);
  }
}

void start_dac_cal()
{
  export_dac_cal(&Serial, &calibration);
  calibration.points = 0;
  
  set_pwm_bits(DAC_BITS);
  set_dac(0);
  set_dac_gain(TEST_RANGE_800mV);
  test_setup_idx = 0;
  test_stage = TS_SETUP;
}

bool run_dac_cal()
{
  switch(test_stage)
  {
    case TS_IDLE:
      break;
    case TS_SETUP:
      cal_dac_setup();
      break;
    case TS_SAMPLE:
      cal_dac_sample();
      break;
    case TS_REPORT:
      cal_dac_report();
      break;
    case TS_NEXT:
      cal_dac_complete();
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

void cal_dac_setup()
{
  #ifdef DEBUG_DAC_CAL
  Serial.println(F("cal_dac_setup"));
  #endif
  
  unsigned int cal_range = test_setup_idx / NO_OF_CALIBRATION_POINTS;
  unsigned int new_dac_value = test_setup_idx % NO_OF_CALIBRATION_POINTS;
  new_dac_value = DAC_CAL_START_OFFSET + (new_dac_value * ((DAC_CAL_END_OFFSET - DAC_CAL_START_OFFSET) / (NO_OF_CALIBRATION_POINTS-1)));

  //change the gain range if test_idx is greater than number of points
  if(cal_range) {
    set_dac_gain(TEST_RANGE_80mV);
  }
  set_dac(new_dac_value);
  test_stage = TS_SAMPLE;

}
void cal_stop()
{
  #ifdef DEBUG_DAC_CAL
  Serial.println(F("cal_stop"));
  #endif
  test_type = TT_NONE;
  test_stage = TS_IDLE;
  set_dac_gain(LOW);
}
void cal_dac_sample()
{
  bool is_correct = true;
  
  do
  {
    Serial.println(F("Enter measured DAC voltage"));
    while(Serial.available()==0);
    dac_voltage = Serial.parseFloat();
    clear_serial();
    Serial.print(dac_voltage);
    Serial.print(F(" mV: "));
    Serial.println(F("Correct Value? (Y/n/c)"));
    is_correct = true;
    while(Serial.available()==0);
    char user_input = Serial.read();
    clear_serial();
    if(user_input == 'n') { is_correct = false; }
    if(user_input == 'c') { cal_stop(); return; }
  }while(!is_correct);

  dac_read_sample();

  unsigned int cal_range = test_setup_idx / NO_OF_CALIBRATION_POINTS;
  unsigned int cal_row = test_setup_idx % NO_OF_CALIBRATION_POINTS;

  calibration.range[cal_range].dac[cal_row] = dac_value;
  calibration.range[cal_range].adc[cal_row] = dac_sample;
  calibration.range[cal_range].mV[cal_row] = dac_voltage;

  if((test_setup_idx % NO_OF_CALIBRATION_POINTS) == (NO_OF_CALIBRATION_POINTS-1)) {
    test_stage = TS_REPORT;
  }
  else {
    test_stage = TS_SETUP;
    test_setup_idx++;
  }
  
}

void cal_dac_report()
{
  Serial.print(FS(S_BAR));
  Serial.print(F("\nCal Report, range: "));
  if(dac_range == TEST_RANGE_800mV)  Serial.println(FS(S_800MV));
  else Serial.println(FS(S_80MV));

  unsigned int cal_range = test_setup_idx / NO_OF_CALIBRATION_POINTS;
  export_dac_cal_set(&Serial, &calibration.range[cal_range], NO_OF_CALIBRATION_POINTS);

  Serial.println(FS(S_BAR));
  
  test_setup_idx++;

  if(test_setup_idx >= NO_OF_CALIBRATION_POINTS*2)  {
    test_stage = TS_NEXT;
  }
  else  {
    test_stage = TS_SETUP;
  }
}

void cal_dac_complete()
{
  calibration.points = NO_OF_CALIBRATION_POINTS;
  cal_stop();
}

 
