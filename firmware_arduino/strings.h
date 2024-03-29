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

// define them as consts here and delete the original 
const char S_DATE[] PROGMEM = __DATE__;
const char S_TIME[] PROGMEM = __TIME__;
const char S_FIRMWARE_NAME[]  PROGMEM = "OpenGEET Reactor Controller (MEGA) v0.7.0";
const char S_COMMA[] PROGMEM = ", ";
const char S_COLON[] PROGMEM = ": ";
const char S_CARD_FAILED_OR_NOT_PRESENT[] PROGMEM = "SD Card failed, or absent"; //if this string is not defined/referenced, or is made shorter than this, an extra 40-60 bytes is consumed for no obvious reason
//#define S_CARD_FAILED_OR_NOT_PRESENT S_NO_SD_CARD
const char S_CARD_INITIALISED[] PROGMEM = "SD Card OK";
const char S_RECORD_VER_C[] PROGMEM = "Record Ver.: ";
const char S_TIMESTAMP_C[] PROGMEM = "Timestamp: ";
const char S_MAP_C[] PROGMEM = "MAP: ";
const char S_USR_C[] PROGMEM = "USR: ";
const char S_TMP_C[] PROGMEM = "TMP: ";
const char S_TRQ_C[] PROGMEM = "TRQ: ";
const char S_EGT_C[] PROGMEM = "EGT: ";
const char S_POW_C[] PROGMEM = "POW: ";
const char S_RPM_AVG[] PROGMEM = "RPM avg: ";

const char S_RPM_NO_OF_TICKS[] PROGMEM = "RPM no.: ";
const char S_RPM_TICK_TIMES[] PROGMEM = "RPM times (ms): ";
const char S_RPM_TICK_OFFSET[] PROGMEM = "RPM tick offset (ms): ";

const char S_OUTPUT_FILE_NAME_C[] PROGMEM = "Output File Name: ";





const char S_MAP_AVG_C[] PROGMEM = "MAP avg: ";

const char S_ANA_SAMPLES_C[] PROGMEM = "ANA no.: ";

const char S_EGT_AVG_C[] PROGMEM = "EGT avg: ";
const char S_EGT_SAMPLES_C[] PROGMEM = "EGT no.: ";

const char S_DOT_TXT[] PROGMEM = ".txt";
const char S_DOT_RAW[] PROGMEM = ".raw";
const char S_DOT_JSN[] PROGMEM = ".jsn";


const char S_EGT_DEGC[] PROGMEM = "EGT degC";
const char S_RPM[] PROGMEM = "RPM";
const char S_MAP_MBAR[] PROGMEM = "MAP mbar";
const char S_INPUT_1[] PROGMEM = "Input 1";
const char S_INPUT_2[] PROGMEM = "Input 2";
const char S_INPUT_3[] PROGMEM = "Input 3";
const char S_TIMESTAMP_MS[] PROGMEM = "Timestamp (ms)";
const char S_TORQUE_MNM[] PROGMEM = "Tq. mN.m";
const char S_TORQUE_LSB[] PROGMEM = "Tq. LSB";
const char S_TORQUE[] PROGMEM = "Torque";
const char S_POWER_W[] PROGMEM = "Power W";
const char S_TEMP_DEGC[] PROGMEM = "Temp degC";

const char S_TARGET[] PROGMEM = "Target";
const char S_OUTPUT[] PROGMEM = "Output";

const char S_PID_SV0[] PROGMEM = "PID SV0";
const char S_PID_SV0_C[] PROGMEM = "PID SV0: ";
const char S_PID_SV0_AVG_C[] PROGMEM = "PID SV0 avg: ";
const char S_PID_SAMPLES_C[] PROGMEM = "PID no.: ";

const char S_ACTIVE[] PROGMEM = "ACTIVE";
const char S_HOLD[] PROGMEM = "HOLD";

const char S_TQ_SET_ZR[] PROGMEM = "TQ SET ZR";
const char S_TQ_SET_MX[] PROGMEM = "TQ SET MX";
const char S_TQ_SET_MN[] PROGMEM = "TQ SET MN";


const char S_TQ_ZR_LSB[] PROGMEM = "TQ ZR LSB";
const char S_TQ_MX_LSB[] PROGMEM = "TQ MX LSB";
const char S_TQ_MN_LSB[] PROGMEM = "TQ MN LSB";

const char S_RECORD_MARKER[] PROGMEM = "----------";


const char S_REC[] PROGMEM = "REC";
const char S_SERIAL[] PROGMEM = "Serial";
const char S_SDCARD[] PROGMEM = "SDCard";
const char S_HEX[] PROGMEM = "HEX";
const char S_TXT[] PROGMEM = "TXT";
const char S_NO_FILE_OPEN[] PROGMEM = "No File Open";
const char S_NO_SD_CARD[] PROGMEM = "No SD Card";

const char S_SAVE[] PROGMEM = "SAVE";
const char S_LOAD[] PROGMEM = "LOAD";


const char S_EXPORT[] PROGMEM = "EXPORT";
const char S_IMPORT[] PROGMEM = "IMPORT";
const char S_PID_USE_MS[] PROGMEM = "PID use ms";


const char S_BASIC[] PROGMEM = "Basic";
const char S_PID_RPM[] PROGMEM = "PID RPM";
const char S_PID_2[] PROGMEM = "PID 2";
const char S_CONFIG[] PROGMEM = "Config";
const char S_NONE[] PROGMEM = "None";


const char S_SERVO_MIN[] PROGMEM = "Servo Min";
const char S_SERVO_MAX[] PROGMEM = "Servo Max";

const char S_SERVO_0[] PROGMEM = "Servo 0";
const char S_SERVO_1[] PROGMEM = "Servo 1";
const char S_SERVO_2[] PROGMEM = "Servo 2";



// int to char conversion for hex and decimal
const char PROGMEM hex_char[] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
#define HEX_CHAR(value)   pgm_read_byte_near(hex_char + (value&0xf))
#define DEC_CHAR(value)   ('0' + (value%10))


#endif // __STRINGS_H__
