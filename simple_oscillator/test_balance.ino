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
 * The DAC and ADC are calibrated independantly.
 */

#define DAC_BITS 12
#define DAC_STEP (1<<(16-DAC_BITS))

#define DAC_GAIN_CHANGE_TIME_ms   50
#define DAC_VALUE_CHANGE_TIME_us  15

byte dac_range = TEST_RANGE_800mV;

unsigned int dac_sample;  //value read from the adc
unsigned int dac_value;   //value set with pwm
float dac_voltage;        //value entered via serial

void dac_read_sample()
{
  byte dac_sample_idx = 0;
  dac_sample = 0;

  unsigned int adc_min = UINT16_MAX;
  unsigned int adc_max = 0;
  unsigned int adc_spread = adc_min;
  byte attempts = 0;
  
  while (attempts < TEST_SPREAD_RETRY_MAX && adc_spread > TEST_SAMPLE_TOLERANCE_lsb)
  {
    while(dac_sample_idx++ < TEST_SAMPLES)
    {
      unsigned int new_sample = analogRead(TEST_CH_DAC);
      dac_sample += new_sample;
      adc_min = min(adc_min, new_sample);
      adc_max = max(adc_max, new_sample);
    }
    adc_spread = adc_max - adc_min;
    attempts++;
  }
}
void set_dac(unsigned int new_dac_value)
{
  force_pwm_w(new_dac_value);
  if(new_dac_value != dac_value)
  {
    unsigned int change = (dac_value > new_dac_value) ? (dac_value - new_dac_value) : (new_dac_value - dac_value);
    delayMicroseconds((unsigned long)change * DAC_VALUE_CHANGE_TIME_us);
    dac_value = new_dac_value;
  }
}

void set_dac_gain(byte range)
{
  if(range != dac_range)
  {
    digitalWrite(PIN_DAC_GAIN, range);
    delay(DAC_GAIN_CHANGE_TIME_ms);
    dac_range = range;
  }
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
  dst->print(FS(S_DAC_HEADER));
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
  unsigned int cal_range = test_setup_idx / NO_OF_CALIBRATION_POINTS;
  unsigned int new_dac_value = test_setup_idx % NO_OF_CALIBRATION_POINTS;
  new_dac_value = (DAC_STEP*2) + (new_dac_value * ((UINT16_MAX - DAC_STEP*4) / NO_OF_CALIBRATION_POINTS));

  //change the gain range if test_idx is greater than number of points
  if(cal_range) {
    set_dac_gain(TEST_RANGE_80mV);
  }
  set_dac(new_dac_value);

}
void cal_stop()
{
  test_type = TT_NONE;
  test_stage = TS_IDLE;
}
void cal_dac_sample()
{
  bool is_correct = true;
  dac_read_sample();
  do
  {
    Serial.println(F("Enter measured DAC voltage"));
    while(Serial.available()==0);
    dac_voltage = Serial.parseFloat();
    Serial.flush();
    Serial.println(F("Correct Value? (Y/n/c)"));
    is_correct = true;
    while(Serial.available()==0);
    char user_input = Serial.read();
    if(user_input == 'n') { is_correct = false; }
    if(user_input == 'c') { cal_stop(); return; }
  }while(!is_correct);

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
  Serial.print(F("Cal Report, range: "));
  if(dac_range == TEST_RANGE_800mV)  Serial.println(FS(S_800MV));
  else Serial.println(FS(S_80MV));

  unsigned int cal_range = test_setup_idx / NO_OF_CALIBRATION_POINTS;
  export_dac_cal_set(&Serial, &calibration.range[cal_range], NO_OF_CALIBRATION_POINTS);

  test_setup_idx++;

  if(test_setup_idx >= NO_OF_CALIBRATION_POINTS*2)
  {
    test_stage = TS_NEXT;
  }
}

void cal_dac_complete()
{
  calibration.points = NO_OF_CALIBRATION_POINTS;
  cal_stop();
}
 
