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

#define STRING_BUFFER(STRING_NAME)  char string[strlen_P(STRING_NAME)+1]
#define MAKE_STRING(STRING_NAME)  char STRING_NAME ## _str[strlen_P(STRING_NAME)+1]; strcpy_P(STRING_NAME ## _str,STRING_NAME)
#define PUT_STRING(STRING_NAME, buffer) strcpy_P(buffer, STRING_NAME);
#define GET_STRING(STRING_NAME) strcpy_P(string, STRING_NAME);

// define them as consts here and delete the original 
const char DATE[] PROGMEM = __DATE__;
const char TIME[] PROGMEM = __TIME__;
const char FIRMWARE_NAME[]  PROGMEM = "OpenGEET Reactor Controller";
const char FIRMWARE_VERSION[]  PROGMEM = "v0.1";
const char LIST_SEPARATOR[] PROGMEM = ", ";
const char CARD_FAILED_OR_NOT_PRESENT[] PROGMEM = "SD Card failed, or not present.";
const char CARD_INITIALISED[] PROGMEM = "SD Card initialised.";
const char RECORD_VER_C[] PROGMEM = "Record Ver.: ";
const char TIMESTAMP_C[] PROGMEM = "Timestamp: ";
const char A0_C[] PROGMEM = "A0: ";
const char A1_C[] PROGMEM = "A1: ";
const char A2_C[] PROGMEM = "A2: ";
const char A3_C[] PROGMEM = "A3: ";
const char EGT1_C[] PROGMEM = "EGT1: ";
const char RPM_AVG[] PROGMEM = "RPM avg: ";
const char RPM_NO_OF_TICKS[] PROGMEM = "RPM no of ticks: ";
const char RPM_TICK_TIMES[] PROGMEM = "RPM tick times (ms): ";
const char OUTPUT_FILE_NAME_C[] PROGMEM = "Output File Name: ";
const char DEFAULT_FILENAME[] PROGMEM = "datalog";
const char FILE_EXT_TXT[] PROGMEM = ".txt";
const char FILE_EXT_RAW[] PROGMEM = ".raw";

const char EGT1_DEGC[] PROGMEM = "EGT1 degC";
const char RPM[] PROGMEM = "RPM";
const char A0_VALUE[] PROGMEM = "A0 value";
const char A1_VALUE[] PROGMEM = "A1 value";
const char A2_VALUE[] PROGMEM = "A2 value";
const char A3_VALUE[] PROGMEM = "A3 value";
const char TIMESTAMP_MS[] PROGMEM = "Timestamp (ms)";
/*


const char NAME[] PROGMEM = "";
const char NAME[] PROGMEM = "";
const char NAME[] PROGMEM = "";
const char NAME[] PROGMEM = "";
const char NAME[] PROGMEM = "";
const char NAME[] PROGMEM = "";
*/

// int to char conversion for hex and decimal
const char PROGMEM hex_char[] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
#define HEX_CHAR(value)   pgm_read_byte_near(hex_char + (value&0xf))
#define DEC_CHAR(value)   ('0' + (value%10))


#endif // __STRINGS_H__
