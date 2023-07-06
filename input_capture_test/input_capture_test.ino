//#include <Servo.h> //The standard servo library uses ALL THE TIMERS

/*
 * MEGA 2560
 * Input capture pins are:
 * ICPn,    Port Pin,   Dig. Pin
 * ICP1     PD4         -
 * ICP3     PE7         -
 * ICP4     PL0         D49
 * ICP5     PL1         D48
 * 
 * Only timers 4 & 5 can be used for ICP as the others have no pin.
 * 
 * 
 * Base clock is 16Mhz, 
 * DIV    Clk       Tick    Max Time    Lowest RPM    T@6k rpm  drpm@6k rpm               T@1k rpm    drpm@1k rpm        
 * 1      16M       62.5n   4.09600m    14648         -         -                         -           -
 * 8      2M        500 n   32.7680m    1831.0        20000     -0.300  +0.300            -           -
 * 64     250k      4.0 u   262.144m    228.88        2500      -2.399  +2.401            15000       -0.0667 +0.0667
 * 256    62.5k     16.0u   1.048576    57.220        625       -9.585  +9.615            3750        -0.267  +0.267
 * 1024   15.625k   64.0u   4.194304    14.305        156(.25)  -38.28 (+9.615) +38.77    937(.5)     -1.064 (-0.533) +1.067
 * 
 * The ICP interrupt needs to collect the new timer value and subtract the old timer values to get an accurate interval,
 * reseting the timer during the interrupt would introduce latency when other interrupts are occuring.
 * The overflow interrupt is also needed to detect when multiple overflows have occured. 
 * There's no need to add the number of overflows to the time, just discard any intervals 
 * that span multiple overflows and consider the engine stopped.
 * 
 * We'll set the clock to 62.5k initially, and consider increasing it later if needed.
 */

/*  It might be worth writing a custom using OCA,OCB and OCC to generate pulses, and ICP to set rate.
 *  No ISR would be needed then (assuming there's a timer with all the OC pins connected)
 *  
 *  Timer Chan Port Pin
 *  TMR2  OC2A PB4  D10       D10 is currently SD Card CS, but could probably be remapped 
 *        OC2B PH6  D9
 *        OC2C   -            Timer 2 only has outputs for channels A and B
 *        
 *  TMR3  OC3A PE3  D5        D5 is already a servo output
 *        OC3B PE4  D2        D2 was the RPM counter
 *        OC3C PE5  D3        D3 is already a servo output
 *       
 *  TMR4  OC4A PH3  D6        D6 is already a servo output
 *        OC4B PH4  D7        D7 is currently EGT CS, but can be remapped
 *        OC4C PH5  D8        D8 is used for Gameduino CS pins. Cannot be remapped
 *        
 *  TMR5  OC5A PL3  D46       Unused
 *        OC5B PL4  D45
 *        OC5C PL5  D44
 *        
 *  Using Timer 3 for servo outputs would be most convenient,
 *  since 2 of those pins are already servos, and the third is 
 *  already connected as part of the same connector group.
 *  D6 can then be left unused and shorted to PL0 or PL1
 *  for ICP on timers 4/5
 *  
 *  Using Timer4 for input capture leaves Timer 5 free for expansion if needed.
 */

#define PIN_SERVO               3
#define SERVO_MIN_us               750     //default minimum servo value
#define SERVO_MAX_us               2300    //default maximum servo value
#define SERVO_MIN_tk               (SERVO_MIN_us<<1)
#define SERVO_MAX_tk               (SERVO_MAX_us<<1)

#define SERVO_RANGE             (SERVO_MAX-SERVO_MIN)

volatile byte rpm_tmr_ovf = 2;        //increment on overflow, reset on input capture, discard time if >1
volatile unsigned int last_time;      //last ICP value
volatile byte idx = 0;                //index of current sample
volatile unsigned int intervals[256]; //samples

volatile unsigned int total;
volatile byte total_count;

#define OVF_LIMIT 2
#define OVF_COUNT_LIMIT 254

/* //////// OVERFLOW COUNT considerations
 *  
 * A tick occuring at MAX/2 followed by a tick MAX+n, where n is < MAX/2,
 * would overflow before the timer overflows a second time.
 * To get around this, we need to increment the overflow count at the half way mark.
 * We can do this using OCA match interrupt
 */

//input capture interrupt
ISR(TIMER5_CAPT_vect)
{
  unsigned int this_time = ICR5;
  //check for overflows
  if(rpm_tmr_ovf < OVF_LIMIT)// && ~idx) //(stop when out of samples, wait for buffer to clear)
  {
    intervals[idx] = this_time - last_time;
    total += intervals[idx++];
    total_count++;
  }
  rpm_tmr_ovf = 0;
  last_time = this_time;
}

/* see https://forum.arduino.cc/t/how-can-one-view-the-assembly-code-output-from-the-arduino-ide/50302/8
 * for how to get the assembly output from the elf file 
 * and thus check if this isr can be optimised with inline asm.
 * (this is an old post, and there may be a better way than this now)
 */
//timer overflow interrupt
ISR(TIMER4_OVF_vect)
{
  //increment overflow count
   rpm_tmr_ovf++;
}
//OCA match interrupt is the same as the overflow interrupt
ISR(TIMER4_COMPA_vect, ISR_ALIASOF(TIMER4_OVF_vect));

void setup() {
  // set data logger SD card CS high
  pinMode(10, OUTPUT);
  digitalWrite(10, HIGH);
  // set Gameduino CS pins high
  pinMode(9, OUTPUT);
  digitalWrite(9, HIGH);
  pinMode(8, OUTPUT);
  digitalWrite(8, HIGH);
  // set egt sensor CS high
  pinMode(7, OUTPUT);
  digitalWrite(7, HIGH);


  Serial.begin(1000000);
  Serial.println(F("input capture test"));

  //configure servo

  //configure timer 3 for PWM on A B and C using ICR3 to store a top value of 50k-1, giving a 25ms rate (up to 32.768ms)
  //we need WGN mode 14, must be set before ICR can be written
  //clock source 3 (clk/8), COM mode 2 (non inverting)

  //TCCR3A    COM-A1 COM-A0 COM-B1 COM-B0 COM-C1 COM-C0 WGM-1  WGM-0
  //          1      0      1      0      1      0      1      0
  //TCCR3B    ICNC   ICES   -      WGM3   WGM2   CS2    CS1    CS0
  //          0      0      0      1      1      0      1      1
  //TCCR3C    FOC-A  FOC-B  FOC-C  -      -      -      -      -
  //TIMSK3    -      -      ICIE   -      OCIE-C OCIE-B OCIE-A TOIE
  //          0      0      0      0      0      0      0      0 

  //first set the mode
  TCCR3B = _BV(WGM33) | _BV(WGM32);
  TCCR3A = _BV(WGM31);
  //set the refresh rate to 25ms
  ICR3 = 49999; //-1 to compensate for Fast frequency error
  //set the output configuration
  TCCR3A |= _BV(COM3A1) | _BV(COM3B1) | _BV(COM3C1);
  //set the minimum pulse
  OCR3A = SERVO_MIN_tk;
  OCR3B = SERVO_MIN_tk;
  OCR3C = SERVO_MIN_tk;
  //start the timer
  TCCR3B |= _BV(CS31) | _BV(CS30);

  
  //configure ICP pin as input with pullup

  pinMode(49, INPUT_PULLUP); //D49 is PL0, ICP4

  //configure timer 4, enable ovfl and icp isr's for timer 4
  //TCCR4A    COM-A1 COM-A0 COM-B1 COM-B0 COM-C1 COM-C0 WGM-1  WGM-0
  //          0      0      0      0      0      0      0      0
  //TCCR4B    ICNC   ICES   -      WGM3   WGM2   CS2    CS1    CS0
  //          0      0      0      0      0      1      0      0
  //TCCR4C    FOC-A  FOC-B  FOC-C  -      -      -      -      -
  //TIMSK4    -      -      ICIE   -      OCIE-C OCIE-B OCIE-A TOIE
  //          0      0      1      0      0      0      1      1 

  OCR4A  = INT16_MAX;
  TCCR4A = 0;
  //set the clock to 62.5kHz
  TCCR4B = _BV(CS42);
  //enable the intterupts
  TIMSK4 = _BV(ICIE4) | _BV(TOIE4) | _BV(OCIE4A);
  /* The timer and interrupts should not be enabled until
   *  the main loop is ready to start clamping rpm_tmr_ovf.
   * That being said, we have 264 seconds (at 62.5kHz)
   *  before clamping is required.
   */
}

#define UPDATE_INTERVAL_ms 50
void loop() {

  //read a user input, set servo, print rpm
  
  static int timestamp_ms = millis() + UPDATE_INTERVAL_ms;
  int timenow_ms = millis();
  int elapsed_ms = timenow_ms - timestamp_ms;

  if(elapsed_ms <= 0)
  {
    //clamp the overflow counter
    if(rpm_tmr_ovf > OVF_COUNT_LIMIT) rpm_tmr_ovf = OVF_COUNT_LIMIT;
    
    //lock out the ICP interrupt while we grab data
    TIMSK4 &= ~_BV(ICIE4);
    unsigned int total_t = total;
    byte total_count_t = total_count;
    //reset the counts
    total = 0; 
    total_count = 0;
    //reenable the interrupt
    TIMSK4 |= _BV(ICIE4);

    //calculate rpm and report
    //units are 16us
    #define TICK_us                       16
    #define US_TO_RPM(us)                 (60000000L/(us))
    #define MS_TO_RPM(ms)                 (60000/(ms))
    #define TICK_TO_US(tk)                (((long)tk)<<4)
    #define TICK_TO_RPM(tk)               ((60000000L/TICK_us)/((long)tk))

    static unsigned int rpm_avg_since_last_pid;
    static byte no_tick_count = 0;

    //check for ticks since last update
    if(total_t)
    {
      /* expected operation: 
       *  tick range (rpm max)625   3750(rpm min) 
       *  tick count          5     1
       *  tick total          3125  3750
       */

      //calculate average rpm over last 50ms
      rpm_avg_since_last_pid = (unsigned int)(((60000000L/TICK_us) * total_count_t)/(long)total_t);
       
      //reset no tick count
      no_tick_count = 0;
        
    }
    else
    {
      /* if rpm is lower than 1200, a tick may not occur within this update interval
       * if this is the first time without tick, we can use the last rpm figure
       * otherwise, we count the number of cycles with no tick up until rpm_tmr_ovf 
       * reaches it's limit, OVF_LIMIT
       * At that point we call rpm zero
       */

      //check that the timer hasn't overflowed
      if(rpm_tmr_ovf < OVF_LIMIT)
      {
        //check if this is the first time no ticks were counted
        if(no_tick_count++)
        {
          //if not, estimate the rpm from the number of missed ticks
          rpm_avg_since_last_pid = MS_TO_RPM( UPDATE_INTERVAL_ms * no_tick_count );
        }
      }
      else
      {
        //set rpm to zero
        rpm_avg_since_last_pid = 0;
        //reset no tick count so that the first tick does not use a stale no_tick_count to estimate rpm
        no_tick_count = 0;
      }
    }

    //report the rpm
    Serial.print("RPM: "); Serial.println(rpm_avg_since_last_pid); 

    //update the servo
    int input = analogRead(A1);
    set_sv(input);
  }
  
}

/* mapping for ADC values */
int amap(int x, int out_min, int out_max)
{
  long result = ((long)x * (out_max - out_min)) + (1 << 9);
  return (int)(result >> 10) + out_min;
}

void set_sv(unsigned int ratio)
{
    //map to ticks
    unsigned int servo_pos = amap(ratio, SERVO_MIN_tk, SERVO_MAX_tk);
    //write the register for servo[0] / D6
    OCR3C = servo_pos;
}
