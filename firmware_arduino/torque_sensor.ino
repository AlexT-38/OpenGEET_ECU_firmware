/* gx120 peak torque: 7.2 N.m = 713.8013 gf.m, 7200 mN.m
 * Load Cell capacity is 10kg, 98.0665N
 * Prony Brake Load Arm is 0.1m
 * Torque capacity is 9806 mN.m
 * Presumably we can safely use it as a 100N load cell
 * for calibration purposes, we will use 1kg.m
 * but to keep values in the int16 range, we will use mN.m as calibration units
 */

#define TORQUE_TIMEOUT_ms 2
#define TORQUE_CAL_MAX_mNm    9806
#define TORQUE_CAL_MIN_mNm    -TORQUE_CAL_MAX_mNm



#define TORQUE_PRESCALE   4     //divide by 16 to get values in the INT16 range
#define TORQUE_ROUNDING   _BV(TORQUE_PRESCALE-1)

#define TORQUE_CAL_DEFAULT    (300000>>TORQUE_PRESCALE) //default counts for 1kg.m

#define TORQUE_CAL_OVERSAMPLE 7
#define TORQUE_CAL_READINGS   _BV(TORQUE_CAL_OVERSAMPLE) //number of readings to take when calibrating


TORQUE_CAL          torque_cal;

void configure_torque_sensor()
{
  //configure the io pins
  HX711_configure();
  //configure the default calibration
  torque_cal.counts_zero = 0;
  torque_cal.counts_max = TORQUE_CAL_DEFAULT;   //approximate 16bit counts for 10kg, based on 3kg cal data (~100000 raw counts)
  torque_cal.counts_min = -TORQUE_CAL_DEFAULT;  // these values are too low - losing about 5 bits of accuracy
  //trigger a read to ensure the next read is the correct channel/gain, if not default.
  torqueRead(); 
}

int torqueRead()
{
  //wait for valid data
  int timeout_ms = millis() + TORQUE_TIMEOUT_ms;
  while(HX711_getDATA() && millis() < timeout_ms);
  //round down to int before applying the calibration
  int torque_mNm = 0;
  if(!HX711_getDATA())
  { 
    int torque_mNm = tq_counts_to_mNm((HX711_read_value()+TORQUE_ROUNDING)>>TORQUE_PRESCALE);
  }
  return torque_mNm;
}

/* applies the calibration to the given raw counts
 *  
 */
int tq_counts_to_mNm(int value_counts)
{
  int cal_value;
  if(value_counts < torque_cal.counts_zero)
  {
    cal_value = lmap (value_counts, torque_cal.counts_min, torque_cal.counts_zero, TORQUE_CAL_MIN_mNm,0);
  }
  else 
  {
    cal_value = lmap (value_counts, torque_cal.counts_zero, torque_cal.counts_max, 0,TORQUE_CAL_MAX_mNm);
  }
  return cal_value;
}

/* this function will block for several seconds, and should not be used when logging, pid is active, or engine is running
 * if a read takes too long, INT16_MAX is returned
 */
int tq_get_cal_value()
{
  long avg_count = 0;
  int timeout_ms = millis() + 100 + TORQUE_TIMEOUT_ms;
  for(int n = 0; n < TORQUE_CAL_READINGS; n++)
  {
    //wait for data
    while(HX711_getDATA() && (millis() < timeout_ms));
    //check for timeout
    if(HX711_getDATA()) return INT16_MAX;
    //add the new reading
    avg_count += HX711_read_value();
  }
  return ((avg_count+(TORQUE_CAL_READINGS>>1))>>TORQUE_CAL_OVERSAMPLE);
}

void tq_set_zero()
{
  // get the cal value
  int cal_value = tq_get_cal_value();
  // set the value if valid
  if(cal_value != INT16_MAX){  torque_cal.counts_zero = cal_value;}
  //otherwise use the default
  else { torque_cal.counts_zero = 0;}
}
void tq_set_high()
{
  // get the cal value
  int cal_value = tq_get_cal_value();
  // set the value if valid
  if(cal_value != INT16_MAX){  torque_cal.counts_max = cal_value;}
  //otherwise use the default
  else { torque_cal.counts_max = TORQUE_CAL_DEFAULT;}
}
void tq_set_low()
{
  // get the cal value
  int cal_value = tq_get_cal_value();
  // set the value if valid
  if(cal_value != INT16_MAX){  torque_cal.counts_min = cal_value;}
  //otherwise use the default
  else { torque_cal.counts_min = -TORQUE_CAL_DEFAULT;}
}
