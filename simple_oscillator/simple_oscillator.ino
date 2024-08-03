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
 * Additional function: Calibrating the comparator resistor networks
 *  DAC to bias the FETs
 *  Amp to match the DAC voltage range to the ADC.
 *  Selectable gain, +/- 800mV or +/-90mV
 *  Comparator monitor inputs
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
 * *this seems to be due to the comparator resistor networks not being balanced
 *  the imbalance meant the comparators were not switching off when they should have
 *  after balancing, the efficiency at 922Hz is terrible, but it is not erratic.
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
 * DONE
 * step 4: plot the data in a spreadsheet
 * DONE
 * step 5: create a table that selects the best bit depth for a given pwm
 * N/A, 10 and 11bits are best and too close to be worth switching on the go
 * step 6: update the boost LUT to use the highest number of bits in the above table 
 *         and scale down as needed
 * In progress.
 * 
 * 
 * I'd like to add the following features:
 * 1) test parameters, specifically no of bits and step size, step size reduction
 *    should be configurable using the q command
 * 2) make single measurement with the current config settings at a command
 * 
 * for (2) command 'M' for measure, and there should probably be a Test Typ TT_MEASURE
 * an optional parameter select csv output format as a boolean parameter
 * 
 * OK, measure done.
 * 
 * skipping test params for now.
 * 
 * need to configure  input voltage measurement gain.
 * 
 * 
 * 
 */


#define ARD_LED 13
#define ARD_PWM 9
#define ARD_ISR_LED 7
#define SET_ISR_LED()   PORTD |= bit(7)
#define CLR_ISR_LED()   PORTD &= ~bit(7)
#define TOG_ISR_LED()   PORTD ^= bit(7)

#define ARD_ENABLE  6

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
#include "test.h"
#include "eeprom.h"
#include "strings.h"
#include "commands.h"




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
  pinMode(ARD_ENABLE,OUTPUT);
  digitalWrite(ARD_ENABLE,LOW);

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

  digitalWrite(ARD_ENABLE,HIGH);
  
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
