#ifndef __STRINGS_H__
#define __STRINGS_H__

// PROGMEM STRING MACROS
// see the docs: https://www.nongnu.org/avr-libc/user-manual/group__avr__pgmspace.html
// string literals consume ram unless they are declared as PROGMEM variables
// for stricly one off strings being passed to Serial.print(), use F("string literal")
// otherwise use the following macros to fetch and or initialse a buffer for a given PROGMEM string
// (further reading of the docs may reveal a less verbose method)


// buffers should come off the stack after last usage and once it's reached the top of the stack,
// we can reuse buffer variable names by placing all calls to init and read that buffer in separate {} blocks
// keep buffer declarations close to where they are used

#define FS(s) ((__FlashStringHelper *)(s))  //for functions that accept it, use in place of F() for globally defined PSTRings

#define STRING_BUFFER(STRING_NAME)  char string[strlen_P(STRING_NAME)+1]
#define MAKE_STRING(STRING_NAME)  char STRING_NAME ## _str[strlen_P(STRING_NAME)+1]; strcpy_P(STRING_NAME ## _str,STRING_NAME)
#define READ_STRING(STRING_NAME, buffer) strcpy_P(buffer, STRING_NAME);
#define GET_STRING(STRING_NAME) strcpy_P(string, STRING_NAME);

#define READ_STRING_FROM(ARRAY_NAME, INDEX, buffer) strcpy_P(buffer, (char *)pgm_read_word(&(ARRAY_NAME[INDEX])));  // Necessary casts and dereferencing, just copy.


// int to char conversion for hex and decimal
const char PROGMEM hex_char[] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
#define HEX_CHAR(value)   pgm_read_byte_near(hex_char + (value&0xf))
#define DEC_CHAR(value)   ('0' + (value%10))



const char S_DATE[] PROGMEM = __DATE__;
const char S_TIME[] PROGMEM = __TIME__;
const char S_FIRMWARE_NAME[]  PROGMEM = "Simple Oscilator++";
const char S_COMMA[] PROGMEM = ", ";
const char S_COLON[] PROGMEM = ": ";
const char S_ARROW[] PROGMEM = " -> ";
const char S_BAR[] PROGMEM = "------------------------";

const char S_DONE[] PROGMEM = "Done";



const char PROGMEM S_STOP_ALL[] = "STOP_ALL";            //S sets PWM to 0 and disables LFO
const char PROGMEM S_SET_INTERVAL[] = "SET_INTERVAL <0...4.3M>ms";    //set LFO total interval
const char PROGMEM S_SET_PWM[] = "SET_PWM <0...255>";              //V set PWM, observing min and max
const char PROGMEM S_FORCE_PWM[] = "FORCE_PWM <0...255>";          //F set PWM, ignoring min and max
const char PROGMEM S_HELP[] = "HELP";                    //H prints available commands

const char PROGMEM  S_SET_PRESCALE[] = "SET_PRESCALE <1...5>";           //C set clock rate for PWM
const char PROGMEM  S_SET_PWM_LIMIT[] = "SET_PWM_LIMIT [<0...-254>|<1...255>]";          //L set minimum (-ve) and maximum (+ve) pwm values
const char PROGMEM  S_SET_PWM_INVERT[] = "SET_PWM_INVERT <in> <out>";         //I set PWM inversion bit0:in (255-pwm) bit1:out (hi/lo)
const char PROGMEM  S_SET_PWM_RAMP[] = "SET_PWM_RAMP <0...65536>";         //I set PWM inversion bit0:in (255-pwm) bit1:out (hi/lo)
const char PROGMEM  S_SET_PWM_BITS[] = "SET_PWM_BITS";         //B reduce PWM bits from 8 to 7 above 192
const char PROGMEM  S_OSCILLATE[] = "OSCILLATE";         //O send LFO to pwm
const char PROGMEM S_USE_LUT[] = "USE LUT";
const char PROGMEM  S_SAVE_EEP[] = "SAVE_EEP";               //W
const char PROGMEM  S_LOAD_EEP[] = "LOAD_EEP";               //E
const char PROGMEM S_RESET_CONFIG[] = "RESET_CONFIG";           //X
const char PROGMEM S_PRINT_CONFIG[] = "PRINT_CONFIG";           //Z
const char PROGMEM S_PRINT_STATE[] = "PRINT_STATE";           //A

const char PROGMEM S_TEST_FULL_SWEEP[] = "TEST_FULL_SWEEP";
const char PROGMEM S_TEST_CALIBRATE[] = "TEST_CALIBRATE";

const char PROGMEM S_STOP[] = "STOP";
const char PROGMEM S_START[] = "START";
const char PROGMEM S_LFO_OUT_ENABLE[] = "LFO OUT ENABLE";
const char PROGMEM S_LFO_OUT_DISABLE[] = "LFO OUT DISABLE";




const char * const command_str[] PROGMEM = { //this should prabably be an indexed intialiser
  S_STOP_ALL,                   
  S_SET_INTERVAL,               
  S_SET_PWM,              
  S_FORCE_PWM,
  S_SET_PRESCALE,
  S_SET_PWM_LIMIT,
  S_SET_PWM_INVERT,
  S_SET_PWM_RAMP,
  S_SET_PWM_BITS,
  S_OSCILLATE,
  S_USE_LUT,
  S_SAVE_EEP,
  S_LOAD_EEP,
  S_RESET_CONFIG,
  S_PRINT_CONFIG,
  S_PRINT_STATE,
  S_TEST_FULL_SWEEP,
  S_TEST_CALIBRATE,
  S_HELP,                   
};
  


#endif // __STRINGS_H__
