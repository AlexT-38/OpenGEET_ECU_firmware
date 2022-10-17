/*    
struct eeprom
{
    byte         eep_version;
    unsigned int servo_min_us[NO_OF_SERVOS], servo_max_us[NO_OF_SERVOS];
    FLAGS_CONFIG        flags_config;
};
*/


void reset_eeprom()
{
  #ifdef DEBUG_TOUCH_INPUT
  //print the calibration values
  Serial.println(F("Touch Calibration:"));

  //read the values out from eeprom
  long int transform;
  for(byte n=0; n< 6; n++)
  {
    EEPROM.get(n*sizeof( transform ), transform);
    Serial.print((char)('A'+n));
    Serial.print(F(": "));
    Serial.println(transform,HEX);
/*
 * sign + 15.16 fixed point
 * x' = x*A + y*B + C
 * y' = x*D + y*E + F
 * 
 * A 0x FF7E 527C
 * B 0x 0003 DEFF
 * C 0x E50C B600
 * 
 * D 0x FFFF E901
 * E 0x 004B 3AFF
 * F 0x F195 FE00
 * 
 */
    
  }
  /* transform F was 1024 higher than it was supposed to be.
   *  corrected here, but this has had no effect on #issue 1 (tag always zero)
   */
  //EEPROM.put(5*sizeof( transform ), 0xED95FE00);
    
   delay(1000);
  #endif
  
  int debug = 0;
  byte eep_version_r;
  EEP_GET(eep_version,eep_version_r);
#ifdef DEBUG_EEP_RESET
  Serial.print(F("eep old(new): "));
  Serial.print(eep_version_r);
  Serial.print(F(", ("));
  Serial.print(EEP_VERSION);
  Serial.println(')');
#endif
  
  if(eep_version_r != EEP_VERSION)
  {
    //trying to figure out why this isnt working
    delay(1000);

    for(byte n=0; n<NO_OF_SERVOS; n++)
    {
      EEP_PUT_N(servo_min_us, n, SERVO_MIN);
      EEP_PUT_N(servo_max_us, n, SERVO_MAX);
#ifdef DEBUG_EEP_RESET
      // show the values being written
      Serial.print(F("eep write, min: "));
      Serial.print(SERVO_MIN);
      Serial.print(F(", max:  "));
      Serial.print(SERVO_MAX);
#endif
    }

    EEP_PUT(flags_config, flags_config);
    //don't update eep version till all eep writes complete
    EEP_PUT(eep_version, EEP_VERSION);

#ifdef DEBUG_EEP_RESET
    // read back servo values to ensure write/read process is correct
    for(byte n=0; n<NO_OF_SERVOS; n++)
    {
      Serial.print(F("eep read, min: "));
      EEP_GET_N(servo_min_us, n, debug);
      Serial.print(debug);
      Serial.print(F(", max:  "));
      EEP_GET_N(servo_max_us, n, debug);
      Serial.print(debug);
      Serial.println();
    }
#endif    
    
    
  }
}
