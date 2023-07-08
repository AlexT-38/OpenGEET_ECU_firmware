/*
 * engine simulator
 * reads a servo input for throttle,
 * generates rpm counter pulses
 * rate of change of pulse rate
 * detirmined by integrating throttle position
 * and net torque wrt pulses
 * 
 */
#define PIN_SERVO_IN 2
#define PIN_PULSE_OUT A0


#define SERVO_MIN_us               944     //default minimum servo value
#define SERVO_MAX_us               1860    //default maximum servo value
#define SERVO_RANGE_us             (SERVO_MAX_us-SERVO_MIN_us)



#define RPM_TO_US(rpm)                (60000000/(rpm))
#define US_TO_RPM(us)                 RPM_TO_US(us)   //same formula, included for clarity

#define RADS_TO_US(rads)              RPM_TO_US(RADS_TO_RPM(rads))
#define US_TO_RADS(us)                RPM_TO_RADS(US_TO_RPM(us))

#define RPM_TO_MS(rpm)                (60000/(rpm))
#define MS_TO_RPM(ms)                 RPM_TO_MS(ms)   //same formula, included for clarity

#define RPM_TO_RADS(rpm)              ((rpm) * 0.10471975511965977461542144610932)
#define RADS_TO_RPM(rads)             ((rads) * 9.5492965855137201461330258023506)

#define US_TO_S(us)                   ((us)*0.000001)
#define S_TO_US(s)                    ((s)*1000000)

#define SIM_RPM_MAX_us                RPM_TO_US(6000)
#define SIM_RPM_MIN_us                RPM_TO_US(600)

#define SIM_MASS_kgm2                 0.01    //rotational inertia in kg.m2 - manual states maximum is 250 kg.cm² to comply with EN standards
#define SIM_SHAFT_DRAG_Nms_rad        0.1     //1st order drag in Nm.sec/rad, caused by mechanical losses
#define SIM_IMPULSE_MAX               0.01    //maximum energy per power stroke in Joules, imp = torque@rpm/(2*rads)
#define SIM_IMPULSE_MIN (SIM_IMPULSE_MAX*0.001)  //minimum energy per ps - max calc: 7Nm at 3600 rpm, = 0.00928
#define SIM_INTAKE_DRAG               0.1     //reduction in pow torque with speed, caused by intake pressure drop at speed, 
                                              //need to research this a bit.

#define NO_SIM_LAG  0.01  //when !do_sim, rate at which engine speed changes per rotation
/* intake drag
 *  the intake has apeture area A
 *  there's a pressure drop from intake to cyclinder of P
 *  the flow velocity into A can be estimated by A*(1-e^-k|P|)
 *  the flow velocity out of A is detirmined by the piston speed and cylinder capacity
 *  but the relationship of Vout to P is A*(e^m|P|)
 *  to accurately simulate intake dynamics, we would have to solve for P given Vout
 *  then solve for Vin, and then multiply by A to get the volume of air intaken,
 *  and then multiply by the density of air, to get the total moles of air intaken
 *  P = ln(|Vout|/A)/m
 *  Vin = A*(1-e^k( ln(Vout/A)/m ))
 *  this is wrong. it gives a peak at zero Vout and zeros at Vout=A
 *  
 *  instead we will assume P is directly proportional to engine speed
 *  A sets the maximum air charge intake
 *  k set the rate at which A is approached wrt to engine speed
 *  
 *  so max_power_torque is multiplied by throttle_position to get A,
 *  engine speed (in rad/s) is multiplied by -intake_drag to get -k|P|, KP
 *  actual torque is A*(1-POW(e,KP))
 *  
 *  
 *  
 */

volatile int servo_time_us;

// since nothing else is happening, we can afford to use floats
float servo_pos;

float engine_speed_rpm;
float engine_speed_rads;
long pulse_time_us = 0;

bool do_log = true;
bool do_sim = false;

float mapf(float x, float in_min, float in_max, float out_min, float out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
float lerp(float a, float b, float t)
{
  return a+((b-a)*t);
}

void setup() {
  // put your setup code here, to run once:
  pinMode(PIN_SERVO_IN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_SERVO_IN), servo_input, CHANGE );

  pinMode(PIN_PULSE_OUT, OUTPUT);

  engine_speed_rpm = 1500.0;
  engine_speed_rads = RADS_TO_RPM(engine_speed_rpm);
  pulse_time_us = (long)RPM_TO_US(engine_speed_rpm);

  Serial.begin(1000000);
  Serial.println(F("Engine Simulator v0.1"));
}

void loop() {
  process_input();
  simulate_engine();
}

void simulate_engine()
{
  //generate pulses at the current engine speed
  static long pulse_timestamp_us = 10000000; //start after 10 seconds
  static bool is_power_stroke = true;
  long timenow_us = micros();
  long RPM_interval = timenow_us - pulse_timestamp_us;

  if(RPM_interval >= 0)
  {
    //pulse the output
    digitalWrite(PIN_PULSE_OUT, HIGH);
    delayMicroseconds(100);
    digitalWrite(PIN_PULSE_OUT, LOW);

    if(do_sim)
    {
      //simulation
      float total_torque_Nm = 0;
      float time_s = US_TO_S((float)pulse_time_us);
      //effect of drag over the last rotation
      total_torque_Nm -= SIM_SHAFT_DRAG_Nms_rad * engine_speed_rads * time_s;
  
      if(is_power_stroke)
      {
        is_power_stroke = false;
        
        //apply power
        float power = ((SIM_IMPULSE_MAX-SIM_IMPULSE_MIN)* servo_pos) + SIM_IMPULSE_MIN;
        power *=  1 - exp(-SIM_INTAKE_DRAG * engine_speed_rads);

        total_torque_Nm += power;
        
      }
  
      //apply the torque to the flywheel
      engine_speed_rads += total_torque_Nm * (1/SIM_MASS_kgm2) * time_s;
    }
    else
    {
      float new_speed = US_TO_RADS(lerp(SIM_RPM_MIN_us, SIM_RPM_MAX_us, servo_pos));
      engine_speed_rads = lerp(engine_speed_rads, new_speed, NO_SIM_LAG);
      
    }
     

    //conver rads to rpm
    engine_speed_rpm = RADS_TO_RPM(engine_speed_rads);

    //convert to microseconds and clamp
    pulse_time_us = (long)RPM_TO_US(engine_speed_rpm);
    pulse_time_us = constrain(pulse_time_us, SIM_RPM_MAX_us, SIM_RPM_MIN_us);
    
    //set the time of the next pulse
    pulse_timestamp_us += pulse_time_us;

    timenow_us = micros()-timenow_us;
    if(do_log)
    {
      Serial.print("t: "); Serial.println(timenow_us);
      Serial.print("dt: "); Serial.println(pulse_time_us);
      Serial.println();
    }
  }
}


void process_input()
{
  //collect pulse time, convert to 10bit and then to float
  if (servo_time_us)
  {
    int amount = servo_time_us;
    servo_time_us = 0; //reset the pulse time so we dont keep re calculating it

    amount = constrain(amount, SERVO_MIN_us, SERVO_MAX_us);
    amount = map(amount, SERVO_MIN_us, SERVO_MAX_us, 0, 1024);
    
    servo_pos = float(amount)/1024.0;
    
    if(do_log)
    {
      Serial.print("a: "); Serial.println(amount);
      Serial.print("s: "); Serial.println(servo_pos);
      Serial.print("r: "); Serial.println(engine_speed_rpm); 
      Serial.println();
    }
    
  }
}

/* set up the rpm counter input pin and ISR */
void servo_input(void)
{
  static int last_us = 0;
  int now_us = micros();
  if( digitalRead(PIN_SERVO_IN) )
    last_us = now_us;
  else
    servo_time_us = now_us - last_us;
}
