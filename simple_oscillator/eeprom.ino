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
byte flag_update_eeprom = false;



void reset_config()
{
  //                prescale_n, pwm, pwm_osc, pwm_min, pwm_max, pwm_ramp, pwm_bits, pwm_invert, pwm_negate, oscillate, enable, lut, interval
  config = (CONFIG) {1,         0,   PWM_MAX, PWM_MIN, PWM_MAX,        1,        8,      false,       true,     false,  false, true,     500};
  config_update();
}
//update registers to match config
void config_update()
{
  set_pwm_invert(config.pwm_negate, config.pwm_invert);
  set_pwm_prescale(config.pwm_prescale);
  set_pwm_bits(config.pwm_bits);
  write_pwm(config.pwm);
}

/* Load config from eeprom, check the crc and if a match, store in ram 
 * Returns: true if loaded correctly
 */
char load_eeprom()
{
  #ifdef DEBUG_EEPROM_SYSTEM
  Serial.println(F("load_eep"));
  #endif
  
  CONFIG config_t;
  
  EEP_GET(config,config_t);
  
  byte crc = EEP_DEFAULT_CRC; 
  //crc = get_crc(crc,servo_cal_t,sizeof(servo_cal_t));
  //crc = get_crc(crc,flags_config_t,sizeof(flags_config_t));
  byte eep_crc;
  EEP_GET(eep_crc,eep_crc);
  if(eep_crc == crc)
  {
    
    config = config_t;
    config_update();

    export_eeprom(&Serial);
    return true;
  }
  #ifdef DEBUG_EEPROM_SYSTEM
  Serial.print(F("eep CRC fail: "));
  Serial.print(eep_crc);
  Serial.print(' ');
  Serial.println(crc);
  #else
  Serial.println(F("eep CRC fail"));
  #endif
  return false;
}

/* immediately calculate config register crc and write to eeprom  */
void save_eeprom()
{
  #ifdef DEBUG_EEPROM_SYSTEM
  Serial.println(F("save_eep"));
  #endif
  byte eep_crc = EEP_DEFAULT_CRC; 
  //eep_crc = get_crc(eep_crc,config,sizeof(config));
  EEP_PUT(config,config);
  EEP_PUT(eep_crc,eep_crc);
  
}

/* check for the update flag to be set, write eeprom only once 1 second has passed since the last write */
void check_eeprom_update()
{
  
  int time_now = millis();
  int elapsed_time = time_now - eeprom_timestamp_ms;

  //prevent stale timer / loopback
  if (elapsed_time > EEPROM_TIMER_MAX_ELAPSED || elapsed_time < 0) eeprom_timestamp_ms = time_now - EEPROM_WRITE_INTERVAL_ms;

  // check for the update flag and update when ready
  if(flag_update_eeprom)
  {
    #ifdef DEBUG_EEPROM_SYSTEM
    Serial.print(F("check_eep: "));
    Serial.print(time_now);
    Serial.print('-');
    Serial.print(eeprom_timestamp_ms);
    Serial.print('=');
    Serial.print(elapsed_time);
    Serial.print('/');
    Serial.println(EEPROM_WRITE_INTERVAL_ms);
    #endif
    if( elapsed_time >  EEPROM_WRITE_INTERVAL_ms)
    {
      eeprom_timestamp_ms = time_now;
      flag_update_eeprom = false;
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
  flag_update_eeprom = true;
}

/* at startup, check if the eeprom version has changed.
 *  if so, we cannot load configuration values
 *  instead, we write the defualt config to eeprom
 *  if loading eeprom fails, we also write the defaults
 *  
 */
void initialise_eeprom()
{
    
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

/*dump basic eeprom config to the specified stream
 * UNO doesn't have enough flash left for a full descriptive dump
 * so only servo cal, rpm pid and torque cal are dumped, and without per-field descriptors
 */
void export_eeprom(Stream *dst)
{
  /* 
    byte pwm_prescale;
    byte pwm;
    byte pwm_min;
    byte pwm_max;
    int pwm_ramp;
    byte pwm_invert:1;
    byte pwm_negate:1;
    byte enable:1;  
    long int interval;
   */
  dst->println(FS(S_BAR));
  dst->println(F("eeprom:"));

  byte eep_version;
  EEP_GET(eep_version,eep_version);

  dst->print(F("version:"));
  dst->println(eep_version);

  byte eep_crc;
  EEP_GET(eep_crc,eep_crc);

  dst->print(F("crc:"));
  dst->println(eep_crc);

  CONFIG config_t;
  EEP_GET(config,config_t);
  
  export_config(dst, &config_t);
}

void export_config(Stream *dst, CONFIG *config_t)
{
  dst->println(FS(S_BAR));
  dst->println(F("config:"));
  
  dst->print(F("pwm_prescale: "));
  dst->println(config_t->pwm_prescale);

  dst->print(F("pwm: "));
  dst->println(config_t->pwm);

  dst->print(F("pwm_osc: "));
  dst->println(config_t->pwm_osc);
  
  dst->print(F("pwm_min: "));
  dst->println(config_t->pwm_min);

  dst->print(F("pwm_max: "));
  dst->println(config_t->pwm_max);

  dst->print(F("pwm_ramp(ms): "));
  dst->println(get_pwm_ramp_ms());

  dst->print(F("pwm_bits: "));
  dst->println(config_t->pwm_bits);

  dst->print(F("pwm_negate: "));
  dst->println(config_t->pwm_negate);

  dst->print(F("pwm_invert: "));
  dst->println(config_t->pwm_invert);

  dst->print(F("enable oscillation: "));
  dst->println(config_t->oscillate);
  
  dst->print(F("enable LFO output: "));
  dst->println(config_t->enable);

  dst->print(F("use look up table: "));
  dst->println(config_t->lut);

  dst->print(F("interval: "));
  dst->println(config_t->interval);
  
  dst->println(FS(S_BAR));

  
}



void import_eeprom(Stream *dst)
{
}
