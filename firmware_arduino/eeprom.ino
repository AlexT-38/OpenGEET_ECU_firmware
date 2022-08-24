/*    
    byte         eep_version;
    unsigned int servo0_min_us, servo0_max_us;
    unsigned int servo1_min_us, servo1_max_us;
    unsigned int servo2_min_us, servo2_max_us;
    unsigned int map_cal_zero, map_cal_min;
    unsigned int knob_0_min, knob_0_max;
    unsigned int knob_1_min, knob_1_max;
    unsigned int knob_2_min, knob_2_max;
*/

void reset_eeprom()
{
  byte eep_version;
  EEP_GET(eep_version,eep_version);
  
  if(eep_version != EEP_VERSION)
  {
    EEP_PUT(eep_version, EEP_VERSION);
    EEP_PUT(servo0_min_us, SERVO_0_MIN);
    EEP_PUT(servo0_max_us, SERVO_0_MAX);
    EEP_PUT(servo1_min_us, SERVO_1_MIN);
    EEP_PUT(servo1_max_us, SERVO_1_MAX);
    EEP_PUT(servo2_min_us, SERVO_2_MIN);
    EEP_PUT(servo2_max_us, SERVO_2_MAX);
    EEP_PUT(map_cal_zero, EEP_VERSION);
    EEP_PUT(map_cal_min, EEP_VERSION);
    EEP_PUT(knob_0_min, KNOB_0_MIN);
    EEP_PUT(knob_0_max, KNOB_0_MAX);
    EEP_PUT(knob_1_min, KNOB_1_MIN);
    EEP_PUT(knob_1_max, KNOB_1_MAX);
    EEP_PUT(knob_2_min, KNOB_2_MIN);
    EEP_PUT(knob_2_max, KNOB_2_MAX);
    EEP_PUT(flags, flags);
    
    
  }
}
