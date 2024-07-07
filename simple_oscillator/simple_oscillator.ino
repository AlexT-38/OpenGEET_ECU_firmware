/*
 * Not so much a simple oscillator as a halfbridge waveform generator
 * and systematic tester.
 *
 * Control via serial interface, simple single letter command codes
 * Soft generated LFO square wave up to 25Hz
 * PWM output with optional min and max values and ramp rate
 * Configurable PWM prescale, resolution, inversion and negation
 * Negation accounts for the inability Atmega to generate zero pwm.
 * PWM value input specified as 8-bit values, regardless of resolution
 * LFO modulation of PWM
 * Configurable look up table for boost conversion application
 * Configuration stored in EEPROM
 * Override command to instantly set PWM to any value and disable modulation
 * 
 * 
 * Test and Dev notes:
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
 * DONE
 * step 2: change the force and write_pwm commands to scale input to this range
 *         by bit shifting only. Since we generally don't want full on PWM
 *         there's no point accounting for this edge case
 * DONE
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
 * 
 * #notes - Realistically, faster switching rate than 8bit are not feasible without seriously optimising the ISR.
 *          At 6bit resolution, interupts occur every 4 micro seconds, but ISR currently takes at least 12 us.
 *          Even at 8 bits, ISR is every 16us, so ISR is 75% of available CPU time
 *          Infact, with context switching included, the ISR is 16us.
 *          Actually, more like 20ms since constext switching also needs to occur after exiting he ISR.
 *          
 *          we could disable the isr when no ramping action is needed.
 *          this doesn't really solve the problem
 *          
 *          the only way to make this work with an arbitrary bit depth is to use another timer
 *          that runs at 30kHz or less to process ramping.
 *          this has the advantage of making ramp time independent of pwm frequency
 *          
 *          the pwm isr is still needed to syncronise ICR1 updates.
 *          however, if we use OCR1A as TOP, it will be double buffered and no cpu overhead is required.
 *          that being said, it requires that I change the circuit and do a whole bunch of refactoring
 *          and since ICR1 does not change as part of waveform generation, 
 *          but rather is a configurable fixed value, there's no real benefit.
 *          
 *          all that being said, I've been measuing the time of the ISR _WITH_ debug code.
 *          it may be more spritely with all that taken out.
 *          
 *          indeed, the ISR runtime is 5.6us (not including context switching at around 8us total)
 *          so 13.6us out of the available 16us @8bit clk/1
 */


#define ARD_LED 13
#define ARD_PWM 9
#define ARD_ISR_LED 7
#define SET_ISR_LED()   PORTD |= bit(7)
#define CLR_ISR_LED()   PORTD &= ~bit(7)
#define TOG_ISR_LED()   PORTD ^= bit(7)

#define INTERVAL_MIN  20
//defaults
#define PWM_MIN 10    //below this, pwm is off (if PWM is negated)
#define PWM_MAX 245   //pwm can never be higher than this (unless PWM is negated)
#define PWM_BITS_MIN  6
#define PWM_BITS_MAX  14

//#define DEBUG_PWM_BITS
//#define DEBUG_TRAP
//#define DEBUG_ISR
#define DEBUG_ISR_LED
//#define DEBUG_PRINT_IN_ISR

//#define DEBUG_COMMAND_TIME

#include <EEPROM.h>
#include "eeprom.h"
#include "strings.h"
#include "commands.h"
#include "test.h"



//LFO oscillator control
static long int osc_time_stamp;
static byte osc_state = 0;

//PWM ramp control
static unsigned int ramp_counter;
static byte ramp_value;
static byte ramp_target;
static byte forced = false;
static byte forced_value;


#ifdef DEBUG_ISR
static unsigned long isr_interval_time;
static unsigned long isr_interval_time_last;
static unsigned int isr_time_start;
static unsigned int isr_time_stop;

static unsigned int last_ramp_value;
#endif






//default config

static CONFIG config;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(1000000);
  Serial.setTimeout(1);
  Serial.println(FS(S_BAR));
  
  // set up pins
  init_sample_pins();
  pinMode(ARD_LED,OUTPUT);
  digitalWrite(ARD_LED,LOW);
  pinMode(ARD_PWM,OUTPUT);
  digitalWrite(ARD_PWM,LOW);

  //setup PWM (TMR1)
  //WGM = 0b101 Fast PWM, 8-bit
  //inverted output (PWM is always at least 1 bit on. We want to go completely off, and never completey on).
  //actualy, TMR1 is 16 bit, and we could probably have full on by writing 256 to OCR1A
  //no prescaling

  //changing to  WGM = 0b1110 Fast PWM, TOP=ICR1

  //only set non changing reg bits here. the others will be set by load_eeprom() via their respective setters
  TCCR1A = (bit(COM1A1) |  bit(WGM11)); //bit(WGM10) |
  TCCR1B = (bit(WGM13) | bit(WGM12));

  ICR1=255;


  //setup RAMP timer (TMR2)
  //WGM, COM2A, COM2B = 0; - we just want the timer to run and trigger overflows
  TCCR2A = 0;
  TCCR2B = 2; // clk/8
  TIMSK2 = bit(TOIE2); // always trigger the interrupt for the sake of simplicity
  


  reset_config();
  //initialise and load config from eeprom 
  initialise_eeprom();


  //set up LFO
  osc_time_stamp = millis()+config.interval;

  
  

  Serial.println(FS(S_FIRMWARE_NAME));
  Serial.println(FS(S_DATE));
  Serial.println(FS(S_TIME));

  
  
}


void loop() {
  #ifdef DEBUG_TRAP
  static int count;
  if(count==0)
  {
    Serial.println();
    Serial.println(F("Loop"));
  }
  count++;
  #endif

  #ifdef DEBUG_ISR
  if(last_ramp_value != ramp_value)
  {
    last_ramp_value = ramp_value;
    Serial.println(F("---"));
    Serial.print(F("Ramp: "));
    Serial.println(last_ramp_value);
    Serial.print(F("OCR1A: "));
    Serial.println(OCR1A);
    Serial.print(F("ICR1: "));
    Serial.println(ICR1);
    Serial.println(F("---"));
  }
  if(isr_time_start && isr_time_stop )
  {
    cli();
    unsigned long isr_interval_time_copy = isr_interval_time;
    unsigned long isr_interval_time_last_copy = isr_interval_time_last;
    unsigned int isr_time_start_copy = isr_time_start;
    unsigned int isr_time_stop_copy = isr_time_stop;
    isr_time_start = 0;
    isr_time_stop = 0;
    sei();

    unsigned long isr_interval = isr_interval_time_copy - isr_interval_time_last_copy;
    Serial.print(F("ISR Interval: "));
    Serial.println(isr_interval);

    unsigned int isr_duration = isr_time_stop_copy - isr_time_start_copy;
    Serial.print(F("ISR Duration: "));
    Serial.println(isr_duration);
  }
  #endif

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

  //check to see if a test is running, if so only run the eeprom update
  if(run_test()) func_slot = false;
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
