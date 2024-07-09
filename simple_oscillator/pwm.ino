// set when changing bit depth
static unsigned int update_top = 0;
//set true during ISR so functions called from ISR know not to print debug msg
#if defined(DEBUG_TRAP) || defined(DEBUG_PWM_BITS) || defined(DEBUG_ISR)
static byte in_isr = false;
#endif
static char bit_shift = 0;


/*set_pwm()
 * set the target pwm, without limits or look up tables
 * the ISR will ramp to this value
 * the value will be used by the LFO if oscillate is enabled
 */
void set_pwm(byte pwm)
{
  #ifdef DEBUG_TRAP
  Serial.println(F("set_pwm start"));
  #endif
  
  //write the value to the pwm config
  config.pwm = pwm;

  //if not oscillating, or oscilation state is 0, also change the current target value
  if(!config.oscillate || !osc_state)
  {
    set_target(pwm);
    //Serial.println(pwm);
  }
  
  
  #ifdef DEBUG_TRAP
  Serial.println(F("set_pwm end"));
  #endif
}
unsigned int pwm_bitshift(byte pwm)
{
  #if PWM_BITS_MIN < 8
  return (bit_shift>0)? pwm<<bit_shift : pwm>>-bit_shift;
  #else
  return pwm<<bit_shift;
  #endif
}
/* force_pwm(pwm)
 * writes a value to OCR1A after negating if required
 * disables oscillation
 * ignores limits and look up tables
 */
void force_pwm(byte pwm)
{
  #ifdef DEBUG_TRAP
  Serial.println(F("force_pwm start"));
  #endif
  
  forced = true;
  forced_value = pwm;
  config.oscillate = false;

  #ifdef DEBUG_PWM_BITS
  Serial.print(F("F, pwm:"));
  Serial.println((int)pwm);
  #endif
  
  //shift the bits before negating
  pwm = pwm_bitshift(pwm);
  //pwm = (bit_shift>0)? pwm<<bit_shift : pwm>>-bit_shift;
    
  #ifdef DEBUG_PWM_BITS
  Serial.print(F("F, shifted:"));
  Serial.println((int)pwm);
  #endif
  
  //use TOP (ICR1) to negate
  if(config.pwm_negate)
  {
    pwm=ICR1-pwm;
  }

  OCR1A = pwm;

  #ifdef DEBUG_PWM_BITS
  Serial.print(F("ICR1:  "));
  Serial.println((int)ICR1);
  Serial.print(F("OCR1A: "));
  Serial.println((int)OCR1A);
  #endif

  #ifdef DEBUG_TRAP
  Serial.println(F("force_pwm end"));
  #endif
}

/* write_pwm(pwm)
 * writes a value to OCR1A after applying lookup tables, limiting and negating as required
 * 
 * this function is called by the ISR and should be heavily optimised
 */
void write_pwm(byte pwm)
{
  unsigned int ocr1a;// = pwm;
  #ifdef DEBUG_TRAP
  //no prints insde isr
  if(!in_isr)
    Serial.println(F("write_pwm start"));
  #endif

  //TODO: high resolution LUT
  //apply look up table
  if(config.lut)
    pwm = LUT[pwm];
  //apply limits        this should account for pwm_bits, and be applied after bit shifting
  pwm = limit_pwm(pwm);

  #ifdef DEBUG_PWM_BITS
  #ifndef DEBUG_PRINT_IN_ISR
  if(!in_isr )
  #endif
  {
  
    Serial.print(F("W, pwm:"));
    Serial.println(pwm);
  }
  #endif
  
  //shift the bits before negating, we should also ignore this if using a 16bit LUT
//  pwm = pwm_bitshift(pwm);
  #if PWM_BITS_MIN < 8
  ocr1a = (bit_shift>0)? pwm<<bit_shift : pwm>>-bit_shift;
  #else
  ocr1a = pwm<<bit_shift;
  #endif
  
  #ifdef DEBUG_PWM_BITS
  #ifndef DEBUG_PRINT_IN_ISR
  if(!in_isr)
  #endif
  {
    Serial.print(F("F, shifted:"));
    Serial.println(ocr1a);
  }
  #endif
  
  //negate
  if(config.pwm_negate)
  {
    ocr1a=ICR1-ocr1a;
  }
  
  //write the final value to OCR1A  
  OCR1A = ocr1a;



  #ifdef DEBUG_TRAP
  if(!in_isr)
  {
    Serial.println(F("write_pwm end"));
  }
  #endif
}


/*limit_pwm(pwm)
 * applies absolute pwm range limits to the given value
 * used only when writing to OCR1A after LUT and before Negate
 */
byte limit_pwm(byte pwm)
{
  if (pwm <config.pwm_min) pwm = (config.pwm_negate) ? 0 : config.pwm_min;         //jump to zero only if negate is enabled
  else if (pwm > config.pwm_max) pwm = (config.pwm_negate) ? config.pwm_max : 255;  //jump to max only if negate is NOT enabled 
  return pwm;
}

/*set_target(pwm)
 * sets the ramp target for ISR updates
 */
void set_target(byte pwm)
{
  byte old_target = ramp_target;
  //enforce limits, incase they changed since setting the pwm/pwm_osc values
  ramp_target = pwm;
  //check if the pwm value was forced, in which set the current ramp value to match
  //the forced value. if look up table is enabled, the nearest value will be used
  if(forced)
  {
    ramp_value = find_in_table(forced_value);
    forced = false;
    //enable_isr();
  }
  //if the target has changed, enable the isr
  //else if (old_target != ramp_target)

  //actually, just enable it whenever a target is set.
  //if the target has already been reached, the ISR will turn itself off
  //enable_isr();
}
/*set_osc(pwm)
 * set the alternate value for LFO oscillation mode, stored in config.pwm_osc
 * if oscillation is enabled and the state is high, the ramp target will be set also
 */
void set_osc(byte pwm)
{
  //write the value to the pwm config
  config.pwm_osc = pwm;

  //if oscillating and oscilation state is 1, also change the current target value
  if(config.oscillate && osc_state)
  {
    set_target(pwm);
  }
  
  //Serial.println(pwm);
}

// ISR -------------------------------------------------------------------------------

inline void enable_isr()
{
  TIMSK1 = bit(TOIE1);
}
inline void disable_isr()
{
  TIMSK1 = 0;
}

// OCR1A updates are now executed on the TMR2 overflow ISR, TMR1 OVF is only for ICR1 updates
// this could probably be a naked ISR, but KISS

/* update ICR1 and then disable this interrupt */
ISR(TIMER1_OVF_vect)
{
  ICR1 = update_top;
  disable_isr();
}

/* pwm ramp, update OCR1A. runs constantly but could be disabled when not in use (again, KISS). */
ISR(TIMER2_OVF_vect)
{
  #ifdef DEBUG_ISR_LED
  SET_ISR_LED();
  //TOG_ISR_LED();
  #endif
  
  #if defined(DEBUG_TRAP) || defined(DEBUG_PWM_BITS) || defined(DEBUG_ISR)
  in_isr = true;
  #endif
  
  #ifdef DEBUG_ISR
  static int count;
  isr_interval_time_last = isr_interval_time;
  isr_interval_time = micros();
  if (count==0)
  {
    isr_time_start = isr_interval_time;
  }
  #endif
  
  //process ramping
  if(ramp_value != ramp_target)
  {
    //jump directly to target when ramp is 0
    if(config.pwm_ramp == 0)
    {      
      ramp_value = ramp_target;
      //keep the ramp counter reset and disable the isr
      ramp_counter = 0;
    }
    //ramp using the ramp counter
    else if(ramp_counter++ >= config.pwm_ramp)
    {
      ramp_counter = 0;
      if(ramp_value < ramp_target)
      {
        ramp_value++;
      }
      else
      {
        ramp_value--;
      }
    }
    write_pwm(ramp_value);
  }
  else
  {
    //keep ramp counter reset when target reached
    ramp_counter = 0;
  }

  #ifdef DEBUG_ISR
  if (count==0)
  {
    isr_time_stop = micros();
  }
  count++;
  #endif
  
  #if defined(DEBUG_TRAP) || defined(DEBUG_PWM_BITS) || defined(DEBUG_ISR)
  in_isr = false;
  #endif
  #ifdef DEBUG_ISR_LED
  CLR_ISR_LED();
  #endif
}


// PWM configuration routines --------------------------------------------------------

void set_pwm_bits(byte bits)
{
  #ifdef DEBUG_TRAP
  Serial.println(F("set_bits start"));
  #endif
  
  //limit bit range
  if(bits < PWM_BITS_MIN) bits = PWM_BITS_MIN;
  else if(bits > PWM_BITS_MAX) bits = PWM_BITS_MAX;
  
  //new top value
  unsigned int top = bit(bits)-1;
  
  //difference between old bits and new bits
  char bit_change = bits-config.pwm_bits;

  //modify OCR1A to reflect the new bitage
  unsigned int ocr1a = OCR1A;

  #ifdef DEBUG_PWM_BITS
  Serial.print(F("new bits: "));
  Serial.println(bits);
  Serial.print(F("new top: "));
  Serial.println(top);
  Serial.print(F("bit change: "));
  Serial.println(bit_change);
  Serial.print(F("old OCR1A:"));
  Serial.println(ocr1a);
  #endif //DEBUG_PWM_BITS

  //note that this is the non negated value, which means that we may need to unnegate, shift, renegate
  if(config.pwm_negate)
  {
    ocr1a = ICR1 - ocr1a; //unnegate using the old top
    ocr1a = bit_change>0?(ocr1a<<bit_change):(ocr1a>>-bit_change); //not sure if bit shifting negative values is allowed?
    ocr1a = top - ocr1a; //re negate using the new top
  }
  else
  {
    ocr1a = bit_change>0?(ocr1a<<bit_change):(ocr1a>>-bit_change);
  }

  #ifdef DEBUG_PWM_BITS
  Serial.print(F("new ocr1a: "));
  Serial.println((int)ocr1a);
  #endif
  //writing directly to the registers here may cause problems if top < OCR1A, in which case update ICR1/OCR1A in the ISR
  //not also that if OCR1A has been written prior to this, but hasnt buffered through yet,
  //this may cause problems if the readback of OCR1A is the actual value of OCR1A and not the BUFFERED value. Check the data sheet
  //data sheet say CPU has access only to the buffered value when in PWM mode so no problem there.

  //for simplicitie's sake:
  //    ICR1 will always be updated in the ISR, 
  //    OCR1A will always be updated immediately (since it's buffered)
  
  config.pwm_bits = bits;
  
  //new bitshift value
  bit_shift = config.pwm_bits - 8;
  
  OCR1A = ocr1a;
  if(top != ICR1)
  {
    update_top = top;  
    #ifdef DEBUG_PWM_BITS
    Serial.print(F("update_top set: "));
    Serial.println(update_top);
    #endif
    enable_isr();
    
  }

  #ifdef DEBUG_TRAP
  Serial.println(F("set_bits end"));
  #endif
}

void set_pwm_prescale(byte prescale)
{
  //1 8 64 256 1024
  if(prescale >0 && prescale < 6)
  {
    config.pwm_prescale = prescale;
    byte reg = TCCR1B;
    reg &= ~0b111;
    reg |= prescale;
    TCCR1B = reg;
    byte scale[] = {0,1,8,64,256,1024};
    unsigned int clk[] = {0, 16000, 2000, 250, 63, 16};
    unsigned int frq[] = {0, 62500, 7813, 977, 244, 61};
    /* move this to commands, so that only a command will illicit a response
    Serial.print(F("PWM pre: "));
    Serial.println(prescale);
    Serial.print(F("PWM clk: "));
    Serial.println(clk[prescale]);
    Serial.print(F("PWM frq: "));
    Serial.println(frq[prescale]);
    */
  }
}
void set_pwm_invert(byte negate, byte invert)
{
  byte do_invert = (config.pwm_invert != invert);
  config.pwm_invert = (invert != 0);
  config.pwm_negate = (negate != 0);

  if(config.pwm_invert)
    TCCR1A |= bit(COM1A0);
  else
    TCCR1A &= ~bit(COM1A0);

  // if negate has changed, we need to update the pwm register
  if(do_invert)
  {
    if(forced) //if previously forced, update with force+pwm()
    {
      force_pwm(forced_value);
    }
    else //otherwise write the current ramp value
    {
      write_pwm(ramp_value);
    }
  }
  /* move this to commands, so that only a command will illicit a response
  Serial.print("PWM inv in: ");
  Serial.println(config.pwm_negate);
  Serial.print("PWM inv out: ");
  Serial.println(config.pwm_invert);
  */
}
#define MS_PER_RAMP_UPDATE 33 //actually 32.768
void set_pwm_ramp(unsigned int ramp_ms)
{
  unsigned long ramp_time = (unsigned long)ramp_ms / MS_PER_RAMP_UPDATE;
  config.pwm_ramp = ramp_time;
}

unsigned long get_pwm_ramp_ms()
{
  unsigned long ramp_ms = config.pwm_ramp * MS_PER_RAMP_UPDATE;
  return ramp_ms;
}
