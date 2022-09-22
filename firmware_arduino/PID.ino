/* pid routine for controlling throttle positions etc 
 *
 * parameters are in 8.8 format
 * calculations are in 16.16
 *
 *
 */



typedef struct pid
{
  unsigned int target;
  int kp, ki, kd;
  int p, i, d;
} PID;

PID RPM_control;

void configure_PID()
{
  RPM_control.target = rpm_last_tick_time_us;
}
void update_PID()
{
  long result = 0;
  int err = RPM_control.target - rpm_last_tick_time_us;

  RPM_control.d = err - RPM_control.p;
  RPM_control.p = err;
  RPM_control.i += err;

  result =  ((long)RPM_control.d * RPM_control.kd) +
            ((long)RPM_control.p * RPM_control.kp) +
            ((long)RPM_control.i * RPM_control.ki);
}

#define SERVO_RANGE_BITS  10
#define MAP_SERVO(input, out_min, out_max)    ((input *  (out_max - out_min))>>SERVO_RANGE_BITS)

void process_pid_loop()
{
  #ifdef DEBUG_PID_TIME
  unsigned int timestamp_us = micros();
  #endif
  
  // process invloves somthing like:
  // opening the air managemnt valve to increase rpm, closing to reduce rpm
  // opening reactor inlet valve to reduce vacuum, closing to increace vacuum
  // opening RIV and closing AMV to reduce exhaust temperature

  //this will be detirmined after collecting some data under manual control
  // which is the point in datalogging

  // test initially with rpm control of unmodified engine, direct servo control of throttle

  // buffers for the eeprom min max values

  unsigned int sv_targets[NO_OF_SERVOS] = {0};

  //all servo inputs will be scaled to 0-1024 prior to scaling to the configured min max values

  // copy the dial inputs to the servo outputs
  sv_targets[0] = KNOB_value_0 << (SERVO_RANGE_BITS - KNOB_RANGE_BITS);
  #if NO_OF_SERVOS >1
    sv_targets[1] = KNOB_value_1 << (SERVO_RANGE_BITS - KNOB_RANGE_BITS);
    #if NO_OF_SERVOS >2
      sv_targets[2] = KNOB_value_2 << (SERVO_RANGE_BITS - KNOB_RANGE_BITS);
    #endif
  #endif

  //overwrite those values with PID generated values
#ifdef PID_ENABLED
  #ifdef PID_MODE_RPM_ONLY
  #else
  #endif
#endif
  

  // go through each servo, and map inputs to outputs
  for (byte n=0; n<NO_OF_SERVOS; n++)
  {
    // fetch the servo times from eeprom
    unsigned int servo_min_us, servo_max_us;
    EEP_GET_N(servo_min_us,n, servo_min_us);
    EEP_GET_N(servo_max_us,n, servo_max_us);
    //map input to output range
    unsigned int servo_pos_us = amap(sv_targets[n], servo_min_us, servo_max_us);
    //write output
    servo[n].writeMicroseconds(servo_pos_us);
    #ifdef DEBUG_SERVO
    Serial.print(F("sv["));Serial.write('0'+n);Serial.print(F("]: ")); Serial.println(servo_pos_us);
    #endif
  }
  


//this is taking about 180us, which seems far to much.
// perhaps it because the map function uses long ints?
// yep, using short ints reduces the time to 52us
  #ifdef DEBUG_PID_TIME
  timestamp_us = micros() - timestamp_us;
  Serial.print(F("t_pid us: "));
  Serial.println(timestamp_us);
  #endif
}
