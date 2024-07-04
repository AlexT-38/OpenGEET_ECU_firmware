/*
 * 
 * this was simple, but now less so
 * 
 * soft generate a square wave up to 25Hz
 * generate a PWM signal with optional min and max values and ramp rate
 * 
 * this could do with a command to invert the PWM output
 * and another to change the base PWM frequency prescale
 * and eeprom saving of all these params
 * 
 * just now I need to be able to set, or clear or bypass the min and max values
 * that works.
 * 
 * now working on eeprom, copied and adapted from openGeet firmware
 * needs commands to save and load. 
 * suggest 'W' for "Write eeprom" and 'E' for "load Eeprom"
 * commands should alter control register as needed
 * set commands should heed config
 * 
 * 
 * pwm prescale, limit and invert commands implemented
 * eeprom load and save commands implemented
 * pwm registers updated when loading eeprom
 * pwm invert in respected by set_ and force_pwm
 * 
 * it's not clear if pwm input inversion should occur befor or after min/max limit
 * given that max is an absolute limit, and min jumps to zero,
 * this will have an impact on behavior
 * since PWM is byte ranged and cannot go to full max,
 * I believe the inversion should occur first
 * otherwise jump to zero would occur at the high end where 'inverted zero' does not occur
 * 
 * time interval and enable references point to config now
 * 
 * successful eeprom load will dump eeprom to serial
 * 
 * fixed set invert function and command to use two parameters
 * 
 * ramp control implemented
 * LFO control over PWM
 * 
 * trying to imlpement transfer functions but I need to rationalise inversion first
 * 
 * we have the following:
 * OCR1A
 * config.pwm
 * write_pwm()
 * force_pwm()
 * set_pwm()
 * negate
 * invert
 * pwm_min and _max
 * oscillate
 * 
 * a value of OCR1A = 0 produces a 1bit on waveform
 * this is a stupid design flaw. 0 should be FULL OFF but it isnt
 * for most applications full off is a requirement, including boost conversion
 * for boost confversion, full on is not desirable
 * to achieve full off, the output must be inverted (or uninverted) and the value of OCR1A must be negated
 * only by negating the PWM before writing to OCR1A can an input value of 0 produce a 0 waveform
 * this negation must occur at the very last moment before writing to OCR1A as it is part of the PWM output characteristic
 * 
 * this should be performed by write_pwm()
 * 
 * However, when applying a minimum and maximum value to the PWM, it may be required that beyond the limit
 * the PWM is maxed out in that direction.
 * This can only be achieved at the high end of OCR1A values, so the negation of OCR1A should be accounted for 
 * when jumping from a limit to a 0/255 value
 * ie, if negated, value will jump to 0 below min_value and clamp to max value
 *     if not negated, value will be clamped at min_value, but jump to 255 above max_value
 *     
 * force_pwm should set maybe set config.pwm and write that value directly using write_pwm, ignoring limits
 * set_pwm should only set config.pwm after applying limits, allowing the ISR to ramp to config.pwm
 * 
 * the ISR needs a target_pwm value instead of using config.pwm, so that it can oscillate without changing config.pwm
 * 
 * oscillate currently uses min and max values as targets
 * when selecting a target, the value is written to config.pwm
 * it would be better to have a set min and max for oscillation
 * config.pwm should be ideally only be altered by set_pwm(), and maybe force_pwm()
 * oscillate command should probably take a parameter that is the value to switch to from the current pwm value
 * 
 * force_pwm should probably disable oscillation, since it is forceing pwm to a particular value
 * while set pwm would only change the config.pwm, allowing changes to oscilate values while oscillating
 * 
 * meanwhile, any transfer function would also be applied by write_pwm()
 * transfer functions apply to the input prior to inversion and negation, which are output configurations
 * 
 * testing under load (51 Ohm) from a 2A 3A psu, efficiency drops off above ~75% pwm
 * this is due to the low inductance vs resistance, ie the RL time constant is small compared to the time base of the pwm
 * this could be compensated for to some extent by reducing the number of bits at high ratios
 * this may cuase problems with the comparators, since it would increase the proportion of their switching time 
 * relative to the pwm time base
 * 
 * a quick test of efficiency shows that running at 7.9kHz is more efficient
 * than 62.5kHz which is slightly more efficient than 125kHz
 * 
 * this loss in efficiency must be due to switching losses
 * 
 * at 977Hz, results were erratic and extremely inefficient.
 * 
 * therefore it would be desirable to run PWM at full speed
 * and vary the number of bits to optimise efficiency
 * ideally, control would continue to be 8bit
 * and that 8 bits is converted to n bits when writing the value to OCR1A
 * the value perhaps should be scaled such that a value of 255 is always TOP?
 * we can also make the LUT 16bit
 * 
 * step 1: change the B command to accept a number of bits from 6-14
 * step 2: change the force and write_pwm commands to scale input to this range
 *         by bit shifting only. Since we generally don't want full on PWM
 *         there's no point accounting for this edge case
 * step 3: create a step sequencer that progresses through each pwm input value    
 *         measure the input and output voltage, calculates the boost and the efficiency
 *         and then prints the data to serial
 *         Vin is to be measured directly
 *         Vout should be measured with a potential divider
 *         with the bottom leg switchable between different values
 * step 4: plot the data in a spreadsheet
 * step 5: create a table that selects the best bit depth for a given pwm
 * step 6: update the boost LUT to use the highest number of bits in the above table 
 *         and scale down as needed
 * 
 */
#define ARD_LED 13
#define ARD_PWM 9

#define INTERVAL_MIN  20
//defaults
#define PWM_MIN 10    //below this, pwm is off (if PWM is negated)
#define PWM_MAX 245   //pwm can never be higher than this (unless PWM is negated)
#define PWM_BITS_MIN  6
#define PWM_BITS_MAX  14

#include <EEPROM.h>
#include "eeprom.h"
#include "strings.h"
#include "commands.h"


//look up tables
#define LUT lut_boost_01x_8bit
byte lut_boost_01x_8bit[] = {0,23,43,59,73,85,96,105,113,121,128,134,139,144,149,153,157,161,164,167,170,173,175,178,180,182,184,186,188,190,191,193,194,196,197,198,200,201,202,203,204,205,206,207,208,209,209,210,211,212,213,213,214,215,215,216,216,217,218,218,219,219,220,220,221,221,221,222,222,223,223,224,224,224,225,225,225,226,226,226,227,227,227,228,228,228,228,229,229,229,230,230,230,230,230,231,231,231,231,232,232,232,232,232,233,233,233,233,233,234,234,234,234,234,234,235,235,235,235,235,235,236,236,236,236,236,236,236,237,237,237,237,237,237,237,237,238,238,238,238,238,238,238,238,238,239,239,239,239,239,239,239,239,239,239,240,240,240,240,240,240,240,240,240,240,240,241,241,241,241,241,241,241,241,241,241,241,241,241,242,242,242,242,242,242,242,242,242,242,242,242,242,242,242,243,243,243,243,243,243,243,243,243,243,243,243,243,243,243,243,243,243,244,244,244,244,244,244,244,244,244,244,244,244,244,244,244,244,244,244,244,244,244,245,245,245,245,245,245,245,245,245,245,245,245,245,245,245,245,245,245,245,245,245,245,245};

//LFO oscillator control
static long int osc_time_stamp;
static byte osc_state = 0;

//PWM ramp control
static unsigned int ramp_counter;
static byte ramp_value;
static byte ramp_target;
static byte forced = false;
static byte forced_value;




//default config

static CONFIG config;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(1000000);
  Serial.setTimeout(1);
  Serial.println(FS(S_BAR));
  
  // set up pins
  pinMode(ARD_LED,OUTPUT);
  digitalWrite(ARD_LED,LOW);
  pinMode(ARD_PWM,OUTPUT);
  digitalWrite(ARD_PWM,LOW);

  //setup PWM
  //WGM = 0b101 Fast PWM, 8-bit
  //inverted output (PWM is always at least 1 bit on. We want to go completely off, and never completey on).
  //actualy, TMR1 is 16 bit, and we could probably have full on by writing 256 to OCR1A
  //no prescaling

  //changing to  WGM = 0b1110 Fast PWM, TOP=ICR1

  //only set non changing reg bits here. the others will be set by load_eeprom() via their respective setters
  TCCR1A = (bit(COM1A1) |  bit(WGM11)); //bit(WGM10) |
  TCCR1B = (bit(WGM13) | bit(WGM12));

  ICR1=255;
  


  reset_config();
  //initialise and load config from eeprom 
  initialise_eeprom();


  //set up LFO
  osc_time_stamp = millis()+config.interval;

  
  

  Serial.println(FS(S_FIRMWARE_NAME));
  Serial.println(FS(S_DATE));
  Serial.println(FS(S_TIME));

//enable ramp interrupt
  TIMSK1 = bit(TOIE1);
  
}
static byte update_top = 0;
static byte update_ocr1a = 0;

void set_pwm_bits(byte bits)
{
  //limit bit range
  if(bits < PWM_BITS_MIN) bits = PWM_BITS_MIN;
  else if(bits > PWM_BITS_MAX) bits = PWM_BITS_MAX;
  
  //new top value
  unsigned int top = bit(bits)-1;
  
  //difference between old bits and new bits
  char bit_change = bits-config.pwm_bits;

  //modify OCR1A to reflect the new bitage
  unsigned long ocr1a = OCR1A;
  //note that this is the non negated value, which means that we may need to unnegate, shift, renegate
  if(config.pwm_negate)
  {
    ocr1a = ICR1 - ocr1a; //unnegate using the old top
    ocr1a = (ocr1a<<bit_change); //not sure if bit shifting negative values is allowed?
    ocr1a = top - ocr1a; //re negate using the new top
  }
  else
  {
    ocr1a = (ocr1a<<bit_change);
  }

  //writing directly to the registers here may cause problems if top < OCR1A, in which case update ICR1/OCR1A in the ISR
  //not also that if OCR1A has been written prior to this, but hasnt buffered through yet,
  //this may cause problems if the readback of OCR1A is the actual value of OCR1A and not the BUFFERED value. Check the data sheet
  //data sheet say CPU has access only to the buffered value when in PWM mode so no problem there.
  if(top<OCR1A)
  {
    update_top = top;
    update_ocr1a = ocr1a;
  }
  else
  {
    OCR1A = ocr1a;
    ICR1 = top;  
  }
  config.pwm_bits = bits;
}

void loop() {
  //Serial.println(F("Loop"));

  long int time_now = millis();

  long int time_remaining = time_now - osc_time_stamp;

  if(time_remaining >= 0)
  {
    osc_time_stamp = time_now + config.interval + (time_remaining%config.interval);
    osc_state = 1-osc_state;

    if (config.enable || config.oscillate)
    {
      Serial.print(F("LFO: "));
      Serial.println(osc_state);
    }
    
    if (config.enable)
    {
      digitalWrite(ARD_LED,osc_state);
    }
    if(config.oscillate)
    {
      if(osc_state)
        set_target(config.pwm_osc);
      else
        set_target(config.pwm);
    }
  }

  //to reduce latency in the LFO, other tasks will be performed in sequence here
  static byte func_slot = false;
  //Serial.print(F("func slot: "));
  //Serial.println(func_slot);
  if(func_slot)
    Process_Commands();
  else
    check_eeprom_update();
    
  func_slot = !func_slot;
}

void print_state(Stream *dst)
{
  dst->println(FS(S_BAR));
  dst->println(F("state:"));

  dst->print(F("ramp_target: "));
  dst->println(ramp_target);

  dst->print(F("ramp_value: "));
  dst->println(ramp_value);

  dst->print(F("osc_state: "));
  dst->println(osc_state);

  dst->print(F("forced: "));
  dst->println(forced);

  dst->print(F("forced_value: "));
  dst->println(forced_value);

  dst->println(FS(S_BAR));
}

void set_time_interval(long int new_interval)
{
  osc_time_stamp -= config.interval;
  osc_time_stamp += new_interval;
  config.interval = new_interval;
}

ISR(TIMER1_OVF_vect)
{
  //Serial.println(F("ISR"));
  
  //if bits have changed such that top < OCR1A when change requested,
  //we must update OCR1A and ICR1 here
  if (update_top)
  {
    ICR1 = update_top;
    OCR1A = update_ocr1a;
    update_top = 0;
    update_ocr1a = 0;
  }
  
  //process ramping
  if(ramp_value != ramp_target)
  {
    if(config.pwm_ramp != 0)
    {
      //ramp using the ramp counter
      if(ramp_counter++ >= config.pwm_ramp)
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
    }
    else
    {
      //jump directly to target when ramp is 0
      ramp_value = ramp_target;
    }
    write_pwm(ramp_value);
  }
  else
  {
    //keep ramp counter reset when target reached
    ramp_counter = 0;
  }
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
  //enforce limits, incase they changed since setting the pwm/pwm_osc values
  ramp_target = pwm;
  //check if the pwm value was forced, in which set the current ramp value to match
  //the forced value. if look up table is enabled, the nearest value will be used
  if(forced)
  {
    ramp_value = find_in_table(forced_value);
    forced = false;
  }
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

/*find_in_table(val)
 * finds and index in the look up table that is near to the given value 
 * starts in the middle and works outward, selecting the first table value that
 * is closer to the middle or equal to than the given value
 */
byte find_in_table(byte val)
{
  //if lut isnt enabled then do nothing and return the value
  if(!config.lut)
    return val;

  //start in the middle and work outward
  byte idx = 128;
  if(LUT[idx] > val)
  {
    while(LUT[idx] > val && idx > 0)
    {
      idx--;
    }
  }
  else 
  {
    while(LUT[idx] < val && idx < 255)
    {
      idx++;
    }
  }
}
/*set_pwm()
 * set the target pwm, without limits or look up tables
 * the ISR will ramp to this value
 * the value will be used by the LFO if oscillate is enabled
 */
void set_pwm(byte pwm)
{
  //write the value to the pwm config
  config.pwm = pwm;

  //if not oscillating, or oscilation state is 0, also change the current target value
  if(!config.oscillate || !osc_state)
  {
    set_target(pwm);
    Serial.println(pwm);
  }
  
  
  
}

/* force_pwm(pwm)
 * writes a value to OCR1A after negating if required
 * disables oscillation
 * ignores limits and look up tables
 */
void force_pwm(byte pwm)
{
  forced = true;
  forced_value = pwm;
  config.oscillate = false;

  //shift the bits before negating
  char bit_shift = config.pwm_bits - 8;
  if (bit_shift) pwm<<bit_shift;

  //use TOP (ICR1) to negate
  if(config.pwm_negate)
  {
    pwm=ICR1-pwm;
  }

  OCR1A = pwm;

  Serial.print(F("ICR1:  "));
  Serial.println(ICR1);
  Serial.print(F("OCR1A: "));
  Serial.println(OCR1A);
}

/* write_pwm(pwm)
 * writes a value to OCR1A after applying lookup tables, limiting and negating as required
 */
void write_pwm(byte pwm)
{
  //apply look up table
  if(config.lut)
    pwm = LUT[pwm];
  //apply limits        this should account for pwm_bits, and be applied after bit shifting
  pwm = limit_pwm(pwm);


  //shift the bits before negating, we should also ignore this if using a 16bit LUT
  char bit_shift = config.pwm_bits - 8;
  if (bit_shift) pwm<<bit_shift;
  
  //negate
  if(config.pwm_negate)
  {
    pwm=ICR1-pwm;
  }
  
  //write the final value to OCR1A  
  OCR1A = pwm;
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
