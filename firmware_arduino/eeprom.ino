/*    
struct eeprom
{
    byte         eep_version;
    unsigned int servo_min_us[NO_OF_SERVOS], servo_max_us[NO_OF_SERVOS];
    FLAGS        flags;
};
*/

void reset_eeprom()
{
  byte eep_version;
  EEP_GET(eep_version,eep_version);
  
  if(eep_version != EEP_VERSION)
  {
    EEP_PUT(eep_version, EEP_VERSION);

    for(byte n=0; n<NO_OF_SERVOS; n++)
    {
      EEP_PUT_N(servo_min_us, n, SERVO_MIN);
      EEP_PUT_N(servo_max_us, n, SERVO_MAX);
    }

    EEP_PUT(flags, flags);
    
    
  }
}
