/*  Custom using OCA,OCB and OCC to generate pulses, and ICP to set rate.
 *  No ISR is needed.
 *  
 *  Timers available are:
 *  
 *  Timer Chan Port Pin       Comment
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
 *        OC4C PH5  D8        D8 is used for Gameduino GPU CS pin. Cannot be remapped
 *        
 *  TMR5  OC5A PL3  D46       Unused
 *        OC5B PL4  D45
 *        OC5C PL5  D44
 *        
 *  Using Timer 3 for servo outputs is most convenient,
 *  since 2 of those pins are already servos, and the third is 
 *  already connected as part of the same connector group.
 *  D6 can then be left unused and shorted to PL0 or PL1
 *  for ICP on timers 4/5
 *  
 *  Using Timer4 for input capture leaves Timer 5 free for expansion if needed.
 *  Timer 2 could also be used if even more channels are needed.
 */

 /* digital connectors remapping for breakoutboard
  *  brd con, old pin,old func, new func,wire con,(remapped)
  *  1        2       RPM       SVb      3        2
  *  2        3       SV0       SVc      4        3
  *  3        5       SV1       SVa      2        4
  *  4        6       SV2       RPM      1        1
  *  
  *  Servos could be remapped to maintain order (ie a->2 b->0 c->1)
  *  0 - b
  *  1 - c
  *  2 - a
  */
  

//we can't use pointers to registers, so assign regs to servo with defines
#define SV0_REG OCR3B
#define SV1_REG OCR3C
#define SV2_REG OCR3A

// servo pins 
#define PIN_SERVO_0               2
#define PIN_SERVO_1               3
#define PIN_SERVO_2               5



SV_CAL servo_cal[NO_OF_SERVOS];

//this could be handled better
//now that tags are working, sliders should use FT81x's touch trackers
#ifdef XN
#undef XN
#endif
#define XN 5
#define PX 2

void set_servo_min(byte sv)
{
  servo_cal[sv].lower = get_slider_value_horz(GRID_XL(PX,XN), SCREEN_W, SERVO_MIN_us, SERVO_MAX_us);
}
void set_servo_max(byte sv)
{
  servo_cal[sv].upper = get_slider_value_horz(GRID_XL(PX,XN), SCREEN_W, SERVO_MIN_us, SERVO_MAX_us);
}
void initialise_servos()
{
  //pin order is irrelevent
  const byte servo_pins[] = {PIN_SERVO_0,PIN_SERVO_1,PIN_SERVO_2};
  
  for(byte n=0; n<NO_OF_SERVOS; n++) 
  {
    servo_cal[n] = (SV_CAL){SERVO_MIN_us,SERVO_MAX_us};
    pinMode(servo_pins[n], OUTPUT);
  }
    
  //configure timer 3 for PWM on A B and C using ICR3 to store a top value of 50k-1, giving a 25ms rate (up to 32.768ms)
  //we need WGN mode 14, must be set before ICR can be written
  //clock source 2 (clk/8), COM mode 2 (non inverting)


  //TCCR3A    COM-A1 COM-A0 COM-B1 COM-B0 COM-C1 COM-C0 WGM-1  WGM-0
  //          1      0      1      0      1      0      1      0
  //TCCR3B    ICNC   ICES   -      WGM3   WGM2   CS2    CS1    CS0
  //          0      0      0      1      1      0      1      0
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
  TCCR3B |= SERVO_TIMER_CLK;
  
}

/* move all servos to their configured minimum position */
void reset_servos()
{
  for(byte n=0;n<NO_OF_SERVOS;n++)
  {
    set_servo_position(n,0);
  }
}



/* set the position of servo n as a ratio of min to max on a 10bit scale */
void set_servo_position(byte n, unsigned int ratio)
{
    if(n<NO_OF_SERVOS)
    {
      //map to ticks
      unsigned int servo_pos = amap(ratio, US_TO_SERVO(servo_cal[n].lower), US_TO_SERVO(servo_cal[n].upper));
      //write output
      switch(n)
      {
        case 0:
          SV0_REG = servo_pos;
          break;
        case 1:
          SV1_REG = servo_pos;
          break;
        case 2:
          SV2_REG = servo_pos;
          break;
      }
      

      #ifdef DEBUG_SERVO
      Serial.print(F("sv["));Serial.write('0'+n);Serial.print(F("]: ")); 
      Serial.print(ratio);
      Serial.print(F(" (")); 
      Serial.print(servo_cal[n].lower);
      Serial.print(F("-")); 
      Serial.print(servo_cal[n].upper);
      Serial.print(F(") -> ")); 
      Serial.print(SERVO_TO_US(servo_pos));
      Serial.println();
      #endif
    }
}
