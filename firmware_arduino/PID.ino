/* pid routine for controlling throttle positions etc 
 *
 * parameters are in 10.6 format
 * calculations are in 26.6
 * output is unsiged int
 *
 *
 */
//#define DEBUG_PID
//#define DEBUG_PID_LOG_FUNC
//#define DEBUG_PID_EXP_FUNC
#define DEBUG_PID_FEEDBACK
#define DEBUG_PID_FEEDBACK_VALUE  RPM_TO_MS((RPM_MIN_SET + RPM_MAX_SET)>>1)


                 //target,  kp,                ki,                kd,  p,  i, d, invert                                                  
PID RPM_control = {0,       {PID_FL_TO_FP(10),  PID_FL_TO_FP(0.1),  0},  0,  0, 0, 1};  
                  // pid will track tick time instead of rpm, to avoid costly division ops
                  // quantisation from the low resolution may cause problems with the loop, 
PID VAC_control;  // in which case we'd have to keep a 2x or 4x averaged version, or use units of 100us instead of ms
                  // the greatest problem is at the high end of rpm range where 1ms represents a larger change in RPM
                  // at these speeds, there are at least 2 ticks per pid update, so averaging the last two would not
                  // increase the overal loop time

void configure_PID()
{
  RPM_control.target = rpm_last_tick_time_ms;
  RPM_control.p = 0;
  RPM_control.i = 0;
}

unsigned int update_PID(struct pid *pid, int feedback)
{
#ifdef  DEBUG_PID_FEEDBACK
  feedback = DEBUG_PID_FEEDBACK_VALUE;
#endif
  
  int result = 0;
  int err = pid->target - feedback;
  byte do_integrate = ((err > 0) && (pid->i < (PID_FP_MAX-err))) || ((err < 0) && (pid->i > -(PID_FP_MAX+err)));

  pid->d = err - pid->p;
  pid->p = err;
  //prevent pid->i from overflowing
  if( do_integrate ) 
  {
    pid->i += err;
  }
  #ifdef DEBUG_PID
  else 
  {
    Serial.println(F("integral overflow"));
  }
  #endif

  //calculate demand
  long p = (long)pid->p * pid->k.p;
  long i = (long)pid->i * pid->k.i;
  long d = (long)pid->d * pid->k.d;
  long calc =  p+i+d;
  
  //convert fp to int
  int raw_out = (int) (calc>>PID_FP_FRAC_BITS);
  if(pid->invert) raw_out = -raw_out;
  result = PID_OUTPUT_CENTRE + raw_out;
  
  // clamp to range and prevent integral from running away
  if (result < PID_OUTPUT_MIN)
  {
    result = 0;
    if((err < 0) != (pid->invert!=0) ) 
    {
      pid->i -= err; //revert integral to previous value to prevent runaway, if ki*err is contributing to overflow
    }
  }
  else if (result > PID_OUTPUT_MAX)
  {
    result = PID_OUTPUT_MAX;
    if((err > 0) != (pid->invert!=0) ) 
    {
      pid->i -= err;
    }
  }
  #ifdef DEBUG_PID
  
  Serial.print(F("PID kp, ki, kd:   "));
  Serial.print(pid->k.p);
  Serial.print(FS(S_COMMA));
  Serial.print(pid->k.i);
  Serial.print(FS(S_COMMA));
  Serial.print(pid->k.d);
  Serial.println();

  Serial.print(F("PID t, e, p, i, d, raw, out: "));
  Serial.print(pid->target);
  Serial.print(FS(S_COMMA));
  Serial.print(feedback);
  Serial.print(FS(S_COMMA));
  Serial.print(err);
  Serial.print(FS(S_COMMA));
  Serial.print(p);
  Serial.print(FS(S_COMMA));
  Serial.print(i);
  Serial.print(FS(S_COMMA));
  Serial.print(d);
  Serial.print(FS(S_COMMA));
  Serial.print(raw_out);
  Serial.print(FS(S_COMMA));
  Serial.print(result);
  Serial.println();
  #endif
  
  return result;
}

/* make editing pid values with sliders easier by converting to/from log space
 *  use k to px to convert pid parameters to screen slider space
 *  use px to k to convert screen slider values to pid parameter space
 */
 /* to do, if needed */

/* PWL approximation of LOG2 and POW2 
#define LOG_PAR_MIN_BITS      (4)
#define LOG_PAR_BITS          (16)
#define LOG_PAR_LIM           (1<<LOG_PAR_BITS)
#define LOG_PAR_MAX           (1<<LOG_PAR_BITS)
#define LOG_SCR_BITS          (8)
#define LOG_SCR_LIM           (1<<LOG_SCR_BITS)
#define LOG_SCR_MAX           (LOG_SCR_LIM-1)
#define LOG_STEP_BITS         (LOG_SCR_BITS-4)
#define LOG_STEP              (1<<LOG_STEP_BITS)
#define LOG_STEPS             (LOG_SCR_MAX/LOG_STEP)
#define LOG_SCR_MAX           (LOG_SCR_LIM-1)
#define LOG_SCR_MIN           ((LOG_SCR_LIM * LOG_PAR_MIN_BITS) / LOG_PAR_BITS)
#define LOG_PAR_MIN           (1<<LOG_PAR_MIN_BITS)
#define LOG_WH_OFFSET         (LOG_STEP_BITS-2)
*/
/* convert parameter space to screen space */
int pid_convert_k_to_px(unsigned int val_lin)
{
  int val_log = 0;

  #ifdef DEBUG_PID_LOG_FUNC
  Serial.print(F("Log func(x), "));
  Serial.println(val_lin);
  #endif
  
  if (val_lin >= LOG_PAR_MIN)
  {
    unsigned int base = val_lin >> LOG_STEP_BITS;
    char n = 0;
    while(base)
    {
      n++;
      base >>=1;
    }
    unsigned int frac = val_lin >> (n-1);
    unsigned int whole = (LOG_WH_OFFSET + n)*LOG_STEP;
    val_log = whole + frac;
    if (val_log <= LOG_SCR_MIN)
    {
      val_log = 0;
    }
    else
    {
      val_log -= LOG_SCR_MIN;
    }

    #ifdef DEBUG_PID_LOG_FUNC
    Serial.print(F("n: "));
    Serial.println((byte)n);
    Serial.print(F("frac: "));
    Serial.println(frac);
    Serial.print(F("whole: "));
    Serial.println(whole);
    #endif
  }
  #ifdef DEBUG_PID_LOG_FUNC
  Serial.print(F("Result: "));
  Serial.println(val_log);
  Serial.println();
  #endif

  return val_log;
}

/* convert screen space to parameter space */
unsigned int pid_convert_px_to_k(int val_log)
{
  
  int val_lin = 0;
  
  val_log += LOG_SCR_MIN;
  #ifdef DEBUG_PID_EXP_FUNC
  Serial.print(F("Exp func(x), "));
  Serial.print(LOG_SCR_MIN);
  Serial.print(F(" < "));
  Serial.print(val_log);
  Serial.print(F(" < "));
  Serial.println(LOG_SCR_MAX);
  #endif
  
  if (val_log > LOG_SCR_MIN)
  {
    if( val_log > LOG_SCR_MAX )
    {
      val_lin = LOG_PAR_MAX;
    }
    else
    {
      byte whole = val_log/LOG_STEP;
      byte frac = val_log%LOG_STEP;
      byte shift = whole - LOG_STEP_BITS;
      val_lin = (frac + LOG_STEP)<<shift;
      
      #ifdef DEBUG_PID_EXP_FUNC
      Serial.print(F("whole: "));
      Serial.println(whole);
      Serial.print(F("frac: "));
      Serial.println(frac);
      Serial.print(F("shift: "));
      Serial.println(shift);
      #endif
    }
    
  }
  #ifdef DEBUG_PID_EXP_FUNC
  Serial.print(F("Result: "));
  Serial.println(val_lin);
  #endif
  return val_lin;
}



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

  // copy the dial inputs to the servo outputs
  for(byte n=0; n<NO_OF_SERVOS; n++)
  {
    sv_targets[n] = KNOB_values[n];
  }
  switch(sys_mode)
  {
    default:
    case MODE_DIRECT:
      break;
    case MODE_PID_RPM_CARB:
      // convert control input to target rpm 
      RPM_control.target = amap(sv_targets[0], RPM_MIN_SET_ms, RPM_MAX_SET_ms);
      // run the PID calculation
      sv_targets[0] = update_PID(&RPM_control, rpm_last_tick_time_ms);
      // somehow now also log the servo demand value
      if (CURRENT_RECORD.PID_no_of_samples < PID_LOOPS_PER_UPDATE)
      {
        CURRENT_RECORD.PID_SV0[CURRENT_RECORD.PID_no_of_samples] = sv_targets[0];
        CURRENT_RECORD.PID_SV0_avg += sv_targets[0];
      }
      CURRENT_RECORD.PID_no_of_samples++;
      break;
  }
  

  
  // go through each servo, and map inputs to outputs
  for (byte n=0; n<NO_OF_SERVOS; n++)
  {
    set_servo_position(n, sv_targets[n]);
  }
  


//this is taking about 180us, which seems far to much.
// perhaps it because the map function uses long ints?
// yep, using short ints reduces the time to 52us
// odd, I've futher optimised map but performance is still 52us?
// after debugging, is now 72-76us
  #ifdef DEBUG_PID_TIME
//  if(sys_mode != MODE_DIRECT)
//  {
    timestamp_us = micros() - timestamp_us;
    Serial.print(F("t_pid us: "));
    Serial.println(timestamp_us);
//  }
  #endif
}
