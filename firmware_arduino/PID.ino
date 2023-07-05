/* pid routine for controlling throttle positions etc 
 *
 * parameters are in 10.6 format
 * calculations are in 26.6
 * output is unsiged int
 *
 *
 */

//#define DEBUG_PID_LOG_FUNC
//#define DEBUG_PID_EXP_FUNC
//#define DEBUG_PID_FEEDBACK

#define DEBUG_PID_FEEDBACK_VALUE  RPM_TO_MS((RPM_MIN_SET + RPM_MAX_SET)>>1)

#define PID_RPM_DECAY_SCALE 2 //scale of 1 or 2 or gives a similar-ish curve to adding interval_ms
#define PID_DEFAULT_THROTTLE 200  //pid throttle position when engine is not running (time to last pulse > max pulse time)

/* 
 *  RPM PID can track rpm directly, or via rpm intervals. A division is required either way.
 *  By tracking RPM, we may get better resolution at high speeds because we can convert the
 *  accumulated times to rpm first and then multiply by the number of ticks. 
 *  This minimises truncation.
 *  On the other hand, tracking tick times reduces the size of error terms at high speeds
 *  and may (or may not) help to stabilise the response under some load conditions.
 *  If this is at all needed, it may be better to have a lookup table for pid coefs that 
 *  adapt for engine speed (available torque/power), and shaft load.
 *  
 *  Either way, quantisation from the low time resolution may cause problems with the loop, 
 *  see issue #11 about using input capture to get a more precise, acurate and latency free pulse time.
 */

PID PIDs[NO_OF_PIDS];

void configure_PID()
{
  // set default coefficients
  RPM_control.k = (PID_K){PID_FL_TO_FP(1.0),  PID_FL_TO_FP(0.25),  PID_FL_TO_FP(0.05)}; 
}

void reset_PID(struct pid *pid)
{
  pid->i = 0;
  if (!pid->i_max)  pid->i_max = INT16_MAX;
}
/* reset the pid and reduce by the given power of two 
 * this should be the least bits needed to store the largest error value likely to be seen
 */
void reset_PID(struct pid *pid, byte i_max_rsh)
{
  pid->i = 0;
  pid->i_max = INT32_MAX>>i_max_rsh; 
}

unsigned int update_PID(struct pid *pid, int feedback)
{
#ifdef  DEBUG_PID_FEEDBACK
  feedback = DEBUG_PID_FEEDBACK_VALUE;
#endif
  
  int result = 0;
  pid->actual = feedback;
  if(!pid->invert) pid->err = pid->target - feedback;
  else pid->err = feedback - pid->target;

  pid->d = pid->err - pid->p;
  pid->p = pid->err; //this is a bit redundant, unless we store the scaled p and d here
  
  char clip_integral;
  char block_integration;
  if(pid->k.i)
  {
    //only integrate if integral is not saturated
    if (( (pid->err > 0) && (pid->i >=  (pid->i_max - pid->err)) )  || ( (pid->err < 0) && (pid->i <= ( -pid->i_max - pid->err))  ))
      clip_integral = true;
    else clip_integral = false;
  
    //do we also pause integration when the output is saturated?
    if ( ((pid->err > 0) && (pid->output >= PID_OUTPUT_MAX)) && ((pid->err < 0) && (pid->output <= PID_OUTPUT_MIN)) )
      block_integration = true;
    else block_integration = 0;
    
  
    //stop integrating when the output is saturated
    if(!block_integration)
    {
      //prevent pid->i from overflowing
      if( !clip_integral ) 
      {
        pid->i += pid->err;
        #ifdef DEBUG_PID
        Serial.println(F("pid intgt"));
        #endif
      }
      else 
      {
        if(pid->err > 0)
          pid->i = pid->i_max;
        else
          pid->i = -pid->i_max;
        #ifdef DEBUG_PID
        Serial.println(F("pid ovfl"));
        #endif
      }
    }
    else
    {
      #ifdef DEBUG_PID
      Serial.println(F("pid clip"));
      #endif
    }
  }
  else
  {
    #ifdef DEBUG_PID
    Serial.println(F("pid no i"));
    #endif
  }

  //calculate demand (int * fp = fp)
  long p = (long)pid->p * pid->k.p;
  long i = (long)pid->i * pid->k.i;
  long d = (long)pid->d * pid->k.d;
  long calc =  p+i+d;
  
  //convert fp to int
  int raw_out = (int) (calc>>PID_FP_FRAC_BITS);
  result = PID_OUTPUT_CENTRE + raw_out;

  //store the total impact on output
  pid->p = (int)(p>>PID_FP_FRAC_BITS);
  pid->d = (int)(d>>PID_FP_FRAC_BITS);
  pid->ik = (int)(i>>PID_FP_FRAC_BITS);
  
  // clamp to range
  if (result < PID_OUTPUT_MIN)
  {
    result = 0;
    #ifdef DEBUG_PID
    Serial.println(F("pid zero"));
    #endif
  }
  else if (result > PID_OUTPUT_MAX)
  {
    result = PID_OUTPUT_MAX;
    #ifdef DEBUG_PID
    Serial.println(F("pid max"));
    #endif
  }
  pid->output = result;

  
  #ifdef DEBUG_PID
  
  Serial.print(F("PID kp, ki, kd:\n   "));
  Serial.print(String((float)pid->k.p/(float)PID_FP_ONE));
  Serial.print(FS(S_COMMA));
  Serial.print(String((float)pid->k.i/(float)PID_FP_ONE));
  Serial.print(FS(S_COMMA));
  Serial.print(String((float)pid->k.d/(float)PID_FP_ONE));
  Serial.println();

  Serial.print(F("PID    t,     a,     e,     p,     i,     d,       int,   raw,   out:\n   "));

  char string[80];
  snprintf(string,80, "% 5d, % 5d, % 5d, % 5d, % 5d, % 5d, % 9ld, %5d, %5d\n", 
                      pid->target, pid->actual, pid->err, pid->p, pid->ik, pid->d, pid->i, raw_out, pid->output);
  Serial.print(string);
  #endif
  
  return result;
}

/* make editing pid values with sliders easier by converting to/from log space
 *  use k to px to convert pid parameters to screen slider space
 *  use px to k to convert screen slider values to pid parameter space
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

  unsigned int sv_targets[NO_OF_SERVOS] = {0};
  byte n=0; //input/servo index
  
  // copy the dial inputs to the servo outputs - we could take these directly from the user input ADC channels now
  for(n=0; n<NO_OF_SERVOS; n++)
  {
    sv_targets[n] = KNOB_values[n];
  }
  n=0; //reset the index

  static unsigned int rpm_avg_since_last_pid;

  // I'm not sure this MODE idea is the rgth way to do this
  // it might be better to use a matrix to set connectivity between inputs(sensors), outputs(servos) and controllers(PID / other logic)
  // this would make experimentation with control methods easier, since the configuration can be set with a function call
  switch(sys_mode)
  {
    default:
    case MODE_DIRECT:
      // disable writes to servos if holding inputs
      if(flags_status.hold_direct_input)
      {
        n=NO_OF_SERVOS;
      }
      //set the number of pids to report
      Data_Config.PID_no = 0;
      //reset the derived pid inputs
      rpm_avg_since_last_pid = RPM_MIN_SET_ms;
      break;
    case MODE_PID_RPM_CARB:
      if(flags_config.pid_rpm_use_ms) /////////////////////////// rotation time control
      {
        // we need to reduce throtte to increase time, so enable inversion
        if(!RPM_control.invert)
        {
          RPM_control.invert = true;
          //clear the pid's integral and set it's maximum
          reset_PID(&RPM_control, 10); //max reduced by 10 bits, the maximum time (ms) err
        }
        
        // get the target as a ms figure
        RPM_control.target = amap(sv_targets[0], RPM_MIN_SET_ms, RPM_MAX_SET_ms);
        
        //get the average tick time since last pid update
        if(rpm_total_tk)
        {
          rpm_avg_since_last_pid = rpm_total_ms / rpm_total_tk;
        }
        //if there haven't been any ticks, add the PID update interval to the previous value, up to a maximum.
        else if(rpm_avg_since_last_pid < PID_RPM_MAX_FB_TIME_MS)
        {
          rpm_avg_since_last_pid += PID_UPDATE_INTERVAL_ms;
        }
        
        // run the PID calculation, only if rpm avg hasn't reached max
        if(rpm_avg_since_last_pid < PID_RPM_MAX_FB_TIME_MS)
        {
          sv_targets[0] = update_PID(&RPM_control, rpm_avg_since_last_pid);
        }
        else  
        {
          //the engine is not running, or running too slowly
          #ifdef DEBUG_PID
          if(RPM_control.i)  Serial.println(F("PID rpm: reset due to low speed"));
          #endif
          //hold the throttle at the default
          sv_targets[0] = PID_DEFAULT_THROTTLE;
          //clear the pid's integral
          reset_PID(&RPM_control);
          
        }
      }
      else    ////////////////////////////////////// rotation rate control
      {
        // we need to increase throtte to increase rate, so disable inversion
        if(RPM_control.invert)
        {
          RPM_control.invert = false;
          //clear the pid's integral
          reset_PID(&RPM_control, 13); //max reduced by 13 bits, the maximum rate (rpm) err
        }
        
        //get the target as an rpm figure
        RPM_control.target = amap(sv_targets[0], RPM_MIN_SET, RPM_MAX_SET);

        //get the average rpm from ticks since last pid update
        if(rpm_total_tk)
        {
          rpm_avg_since_last_pid = MS_TO_RPM(rpm_total_ms) * rpm_total_tk;
        }
        //if there haven't been any ticks, add the PID update interval to the previous value, up to a maximum.
        
        else if(rpm_avg_since_last_pid > PID_RPM_MIN_FB_RPM)
        {
          rpm_avg_since_last_pid = (rpm_avg_since_last_pid*(_BV(PID_RPM_DECAY_SCALE)-1))>>PID_RPM_DECAY_SCALE;
        }

        // run the PID calculation, only if rpm avg hasn't reached max
        if(rpm_avg_since_last_pid > PID_RPM_MIN_FB_RPM)
        {
          sv_targets[0] = update_PID(&RPM_control, rpm_avg_since_last_pid);
        }
        else  
        {
          //the engine is not running, or running too slowly
          #ifdef DEBUG_PID
          if(RPM_control.i)  Serial.println(F("PID rpm: reset due to low speed"));
          #endif
          //hold the throttle at the default
          sv_targets[0] = PID_DEFAULT_THROTTLE;
          //clear the pid's integral
          reset_PID(&RPM_control);
          
        }
      }
      
      //set the number of pids to report
      Data_Config.PID_no = 1;
      break;
  }

  //reset the rpm pid counters
  rpm_total_ms = 0;
  rpm_total_tk = 0;
  

  
  // go through each servo, and map inputs to outputs
  for (; n<NO_OF_SERVOS; n++)
  {
    set_servo_position(n, sv_targets[n]);
  }

  // log the new servo values and PID states
  if (CURRENT_RECORD.SRV_no_of_samples < PID_LOOPS_PER_UPDATE)
  {
    for(byte idx = 0; idx<Data_Config.SRV_no; idx++)
    {
      CURRENT_RECORD.SRV[idx][CURRENT_RECORD.SRV_no_of_samples] = sv_targets[idx];
    }
    for(byte idx = 0; idx<Data_Config.PID_no; idx++)
    {
      CURRENT_RECORD.PIDs[idx].target [CURRENT_RECORD.SRV_no_of_samples] = PIDs[idx].target;
      CURRENT_RECORD.PIDs[idx].actual [CURRENT_RECORD.SRV_no_of_samples] = PIDs[idx].actual;
      CURRENT_RECORD.PIDs[idx].err    [CURRENT_RECORD.SRV_no_of_samples] = PIDs[idx].err;
      CURRENT_RECORD.PIDs[idx].output [CURRENT_RECORD.SRV_no_of_samples] = PIDs[idx].output;
      CURRENT_RECORD.PIDs[idx].p      [CURRENT_RECORD.SRV_no_of_samples] = PIDs[idx].p;
      CURRENT_RECORD.PIDs[idx].i      [CURRENT_RECORD.SRV_no_of_samples] = PIDs[idx].i;
      CURRENT_RECORD.PIDs[idx].d      [CURRENT_RECORD.SRV_no_of_samples] = PIDs[idx].d;
    }
    CURRENT_RECORD.SRV_no_of_samples++;
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
