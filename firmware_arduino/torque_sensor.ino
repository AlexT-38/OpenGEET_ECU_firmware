/* gx120 peak torque: 7.2 N.m = 713.8013 gf.m, 7200 mN.m
 * Load Cell capacity is 10kg, 98.0665N
 * Prony Brake Load Arm is 0.1m
 * Torque capacity is 9806 mN.m
 * Presumably we can safely use it as a 100N load cell
 * for calibration purposes, we will use 1kg.m
 * but to keep values in the int16 range, we will use mN.m as calibration units
 * 
 * how long does it take to read this?
 * fastest possible is 0.1+25*0.4 us = 10.1 us
 * typical is 2*25 = 50us, 
 * either way is more than the time taken to read one ADC, but less than less than half the time is takes to read all 16 inputs
 */

#define TORQUE_TIMEOUT_ms 1
#define TORQUE_CAL_MAX_mNm    4903 //9806
#define TORQUE_CAL_MIN_mNm    -TORQUE_CAL_MAX_mNm





#define TORQUE_CAL_DEFAULT    (300000>>TORQUE_PRESCALE) //default counts for 1kg.m

#define TORQUE_CAL_OVERSAMPLE 7
#define TORQUE_CAL_READINGS   _BV(TORQUE_CAL_OVERSAMPLE) //number of readings to take when calibrating


TORQUE_CAL          torque_cal;
static long         torque_LSB;
static int          torque_mNm;

void print_torque_cal(Stream &file)
{
  file.print(F("Torque Cal (Mn,Mx,Zr): "));
  file.print(torque_cal.counts_min); file.print(FS(S_COMMA));
  file.print(torque_cal.counts_max); file.print(FS(S_COMMA));
  file.print(torque_cal.counts_zero); file.print(FS(S_COMMA));
  
  file.print(F("\nTorque (LSB,mNm): "));
  file.print(torque_LSB); file.print(FS(S_COMMA));
  file.println(torque_mNm);  
  
}

void configure_torque_sensor()
{
  //Serial.println(F("tqc"));
  //configure the io pins
  HX711_configure();
  //configure the default calibration
  if(torque_cal.counts_zero == 0xFFFF)
  {
    torque_cal.counts_zero = 0;
    torque_cal.counts_max = TORQUE_CAL_DEFAULT;   //approximate 16bit counts for 10kg, based on 3kg cal data (~100000 raw counts)
    torque_cal.counts_min = -TORQUE_CAL_DEFAULT;  // these values are too low - losing about 5 bits of accuracy
  }
  //trigger a read to ensure the next read is the correct channel/gain, if not default.
  torqueRead(); 
}

int torqueRead()
{
#ifdef DEBUG_TORQUE_SENSOR
  //Serial.println(F("tqr"));
#endif
  //wait for valid data
#ifdef DEBUG_TORQUE_NO_TIMEOUT
  while(HX711_getDATA());
#else
  int timeout_ms = millis() + TORQUE_TIMEOUT_ms;
  while(HX711_getDATA() && millis() < timeout_ms);
#endif
  
  
  torque_mNm = 0;
#ifndef DEBUG_TORQUE_NO_TIMEOUT
  if(!HX711_getDATA())
  {
#endif
    //round down to int before applying the calibration
    torque_LSB = (HX711_read_value() + TORQUE_ROUNDING) >> TORQUE_PRESCALE;;
    torque_mNm = tq_counts_to_mNm(torque_LSB);

#ifdef DEBUG_TORQUE_SENSOR
    Serial.println(torque_mNm,HEX);
#ifndef DEBUG_TORQUE_NO_TIMEOUT
  }
  else
  {
    Serial.println(F("tqr FAIL"));
  }
#endif // no timeout
#else // dbug
#ifndef DEBUG_TORQUE_NO_TIMEOUT
  }
#endif 
#endif

  return torque_mNm;
}

/* applies the calibration to the given raw counts
 *  
 */
int tq_counts_to_mNm(int value_counts)
{
  #ifdef DEBUG_TORQUE_RAW
  return value_counts;
  #else
  int cal_value;
  int a, b,c, d;
  if(value_counts < torque_cal.counts_zero)
  {
    //cal_value = lmap (value_counts, torque_cal.counts_min, torque_cal.counts_zero, TORQUE_CAL_MIN_mNm,0);
    a= torque_cal.counts_min;
    b= torque_cal.counts_zero;
    c= TORQUE_CAL_MIN_mNm;
    d= 0;
  }
  else 
  {
    //32284
    //cal_value = lmap (value_counts, torque_cal.counts_zero, torque_cal.counts_max, 0,TORQUE_CAL_MAX_mNm);
    a= torque_cal.counts_zero;
    b= torque_cal.counts_max;
    c= 0;
    d= TORQUE_CAL_MAX_mNm;
  }
  //32272 - saves us 12 bytes
  cal_value = lmap (value_counts, a,b,c,d);
  return cal_value;
  #endif
}

/* this function will block for several seconds, and should not be used when logging, pid is active, or engine is running
 * if a read takes too long, INT16_MAX is returned
 */
int tq_get_cal_value()
{
  long avg_count = 0;
#ifndef DEBUG_TORQUE_CAL_NO_TIMEOUT
  int timeout_ms = millis() + 100 + TORQUE_TIMEOUT_ms;
#endif

  for(int n = 0; n < TORQUE_CAL_READINGS; n++)
  {
    //wait for data
#ifdef DEBUG_TORQUE_CAL_NO_TIMEOUT
    while(HX711_getDATA());// && (millis() < timeout_ms));
#else 
    while(HX711_getDATA() && (millis() < timeout_ms));
    //check for timeout
    if(HX711_getDATA()) 
    {
#ifdef DEBUG_TORQUE_CAL
      Serial.println(F("tq cal FAIL"));
#endif 
      return INT16_MAX;
    }
#endif
    //add the new reading
    avg_count += HX711_read_value();
  }
#define TORQUE_CAL_SCALE (TORQUE_PRESCALE + TORQUE_CAL_OVERSAMPLE)
#ifdef DEBUG_TORQUE_CAL
  Serial.println((avg_count+_BV(TORQUE_CAL_SCALE-1))>>TORQUE_CAL_SCALE,HEX);
#endif 

  return (avg_count+_BV(TORQUE_CAL_SCALE-1))>>TORQUE_CAL_SCALE;

}


void tq_set_zero()
{
  // get the cal value
  int cal_value = tq_get_cal_value();
  // set the value if valid
  //if(cal_value != INT16_MAX){  
  torque_cal.counts_zero = cal_value;
  //}
  //otherwise use the default
 // else { torque_cal.counts_zero = 0;}
}
void tq_set_high()
{
  // get the cal value
  int cal_value = tq_get_cal_value();
  // set the value if valid
  //if(cal_value != INT16_MAX){  
  torque_cal.counts_max = cal_value;
  //}
  //otherwise use the default
  //else { torque_cal.counts_max = TORQUE_CAL_DEFAULT;}
}
void tq_set_low()
{
  // get the cal value
  int cal_value = tq_get_cal_value();
  // set the value if valid
  //if(cal_value != INT16_MAX){  
  torque_cal.counts_min = cal_value;
  //}
  //otherwise use the default
  //else { torque_cal.counts_min = -TORQUE_CAL_DEFAULT;}
}
