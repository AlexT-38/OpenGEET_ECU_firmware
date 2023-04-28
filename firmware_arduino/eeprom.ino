/*    
struct eeprom
{
    byte         eep_version;
    unsigned int servo_min_us[NO_OF_SERVOS], servo_max_us[NO_OF_SERVOS];
    FLAGS_CONFIG        flags_config;
};
*/
#define EEPROM_WRITE_INTERVAL_ms  1000
#define EEPROM_TIMER_MAX_ELAPSED  0x4000

//#define DEBUG_EEPROM_SYSTEM

int eeprom_timestamp_ms;

/* Load config from eeprom, check the crc and if a match, store in ram 
 * Returns: true if loaded correctly
 */
char load_eeprom()
{
  #ifdef DEBUG_EEPROM_SYSTEM
  Serial.println(F("load_eep"));
  #endif
  SV_CAL servo_cal_t[NO_OF_SERVOS];
  FLAGS_CONFIG flags_config_t;
  PID_K pid_k_rpm_t, pid_k_vac_t;
  TORQUE_CAL torque_cal_t;
  
  EEP_GET(servo_cal,servo_cal_t);
  EEP_GET(flags_config,flags_config_t);
  EEP_GET(flags_config,flags_config_t);
  EEP_GET(pid_k_rpm,pid_k_rpm_t);
  EEP_GET(pid_k_vac,pid_k_vac_t);
  EEP_GET(torque_cal,torque_cal_t);

  byte crc = 0xFF; 
  //crc = get_crc(crc,servo_cal_t,sizeof(servo_cal_t));
  //crc = get_crc(crc,flags_config_t,sizeof(flags_config_t));
  byte eep_crc;
  EEP_GET(eep_crc,eep_crc);
  if(eep_crc == crc)
  {
    memcpy(servo_cal, servo_cal_t,sizeof(servo_cal));
    flags_config = flags_config_t;
    RPM_control.k = pid_k_rpm_t;
    VAC_control.k = pid_k_vac_t;
    torque_cal = torque_cal_t;
    return true;
  }
  Serial.print(F("eep CRC fail: "));
  Serial.print(eep_crc);
  Serial.print(' ');
  Serial.println(crc);
  return false;
}

/* immediately calculate config register crc and write to eeprom  */
void save_eeprom()
{
  #ifdef DEBUG_EEPROM_SYSTEM
  Serial.println(F("save_eep"));
  #endif
  byte eep_crc = 0xFF; 
  //eep_crc = get_crc(eep_crc,servo_cal,sizeof(servo_cal));
  //eep_crc = get_crc(eep_crc,flags_config,sizeof(flags_config));
  //eep_crc = get_crc(eep_crc,RPM_control.k,sizeof(RPM_control.k));
  //eep_crc = get_crc(eep_crc,VAC_control.k,sizeof(VAC_control.k));
  
  EEP_PUT(servo_cal,servo_cal);
  EEP_PUT(flags_config,flags_config);
  EEP_PUT(pid_k_rpm,RPM_control.k);
  EEP_PUT(pid_k_vac,VAC_control.k);
  EEP_PUT(eep_crc,eep_crc);
}

/* check for the update flag to be set, write eeprom only once 1 second has passed since the last write */
void check_eeprom_update()
{
  
  static int time_now = millis();
  int elapsed_time = time_now - eeprom_timestamp_ms;

  //prevent stale timer / loopback
  if (elapsed_time > EEPROM_TIMER_MAX_ELAPSED || elapsed_time < 0) eeprom_timestamp_ms = time_now - EEPROM_WRITE_INTERVAL_ms;

  // check for the update flag and update when ready
  if(flags_status.update_eeprom)
  {
    #ifdef DEBUG_EEPROM_SYSTEM
    Serial.println(F("check_eep"));
    #endif
    if( elapsed_time >  EEPROM_WRITE_INTERVAL_ms)
    {
      eeprom_timestamp_ms = time_now;
      flags_status.update_eeprom = false;
      save_eeprom();
    }
  #ifdef DEBUG_EEPROM_SYSTEM
  else  {  Serial.println(F("waiting")); }
  #endif
  }
  
}

/* request to save the config registers to eeprom */
void update_eeprom()
{
  #ifdef DEBUG_EEPROM_SYSTEM
  Serial.println(F("update_eep"));
  #endif
  flags_status.update_eeprom = true;
}

/* at startup, check if the eeprom version has changed.
 *  if so, we cannot load configuration values
 *  instead, we write the defualt config to eeprom
 *  if loading eeprom fails, we also write the defaults
 *  
 */
void initialise_eeprom()
{
  #if defined DEBUG_TOUCH_INPUT || defined DEBUG_TOUCH_CAL
  //print the calibration values
  Serial.println(F("Touch Calibration:"));

  //read the values out from eeprom
  /*
   * sign + 15.16 fixed point
   * x' = x*A + y*B + C
   * y' = x*D + y*E + F
   * 
   * A 0x FFFF 7E31
   * B 0x FFFF FEA5
   * C 0x 01F3 ACCF
   * 
   * D 0x 0000 0043
   * E 0x 0000 4F2B
   * F 0x FFE9 3A88 - should be 0x 03ED 3A88
   * 
   */
  long int transform;
  for(byte n=0; n< 6; n++)
  {
    EEP_GET_N(touch_transform, n, transform);
    Serial.print((char)('A'+n));
    Serial.print(F(": "));
    Serial.println(transform,HEX);
   
  }
  /* transform F was 1024 higher than it was supposed to be.
   *  corrected here, but this has had no effect on #issue 1 (tag always zero)
   *  on rerunning the calibration process, the value came out 1028 too low
   */
//  EEPROM.put(0, 0xFF);                  //reset the calibration flag
//  EEP_PUT_N(touch_transform, 4, 0x4F2B); //write a value to touch transform e
//  EEP_PUT_N(touch_transform, 5, 0x03ED3A88); //write a value to touch transform f
  
  delay(1000);
  #endif

  #ifdef DEBUG_EEP_CONTENT
  for(byte b=0; b< sizeof(struct eeprom); b++)
  {
    byte content;
    EEPROM.get(b,content);
    Serial.print(b);
    Serial.print(F(": 0x"));
    Serial.println(content,HEX);
  }
  #endif

  // get the stored eeprom data format version and compare to the code's version
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
    //wait for succesive resets before making changes to eeprom
    #ifdef DEBUG_EEP_RESET
    Serial.println(F("Wrong eep version: writing default eeprom"));
    #endif
    delay(1000);
    EEP_PUT(eep_version, EEP_VERSION);
    save_eeprom();
    
  }
  else
  {
    if(!load_eeprom())  
    {
      update_eeprom();
      #ifdef DEBUG_EEP_RESET
      Serial.println(F("Eep load failed: writing default eeprom"));
      #endif
    }
  }
  eeprom_timestamp_ms = millis();
}
