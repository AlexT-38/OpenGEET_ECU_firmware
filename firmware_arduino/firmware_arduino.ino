
/*
 * OpenGEET Engine Control Unit firmware (Arduino)
 * Created by A Timiney, 13/08/2022
 * GNU GPL v3
 * 
 * 
 * Open GEET Engine Control Unit firmware
 *
 * Reads sensor values (rpm, egt, map) and user inputs, control valve positions. 
 * Optionally drives fuel injection and ignition, reads magnetic fields. Logs all values to sd card.
 * Initially, control inputs will be mapped directly to valve positions. 
 * Once the behaviour of the reactor and engine are better understood, 
 *  control input can be reduced to rpm control via PID loops. 
 * A 3-phase BLDC motor can then be added to provide engine start and power generation 
 *  using an off the shelf motor controller. Control input can then be tied to output power draw.
 * Hardware will initially be an Arduino Uno with a datalogger sheild. 
 * Upgrade to an STM10x or 40x device can be made later if required.
 */

 /* ram usage has rapidly become an issue
  *  for the SD, SPI, GD2, MAX6675 and Servo libraries, 830 bytes are used
  *  instanciating MAX6675 and GD2 objects uses an additional 101 bytes,
  *  total ram usage for libraries alone is 931 bytes,
  *  leaving 1117 bytes for everything else, including string literals.
  *  
  *  to sample the analog pins at 20 samples per second over 4 channels, plus 75 rpm ticks
  *  DATA_RECORD is aprox 375 bytes per record
  *  two records is 750 bytes, which leaves only 367 bytes for the stack and string literals.
  *  
  *  to avoid the need for a double buffer, we must be able to write the data record to SD card 
  *  and serial port in under 13.3 ms , or else drop data from the rpm counter.
  *  
  *  SD card access is taking approximately 30ms, so double buffering is required.
  *  
  *  to avoid string literals, strings must be placed into progmem
  *  and read out into a temporary buffer as per https://www.arduino.cc/reference/en/language/variables/utilities/progmem/
  */
#include <EEPROM.h>
#include <SD.h>         //sd card library
#include <SPI.h>        //needed for gameduino
#include <GD2.h>        //Gameduino (FT810 plus micro sdcard
#include <GyverMAX6675_SPI.h>    //digital thermocouple interface using SPI port

#include <Servo.h>      //servo control disables analog write on pins 9 and 10


//settings for the SoftI2CMaster library, see https://github.com/felias-fogg/SoftI2CMaster
#define I2C_HARDWARE 1      
#define SDA_PORT PORTC
#define SDA_PIN 4
#define SCL_PORT PORTC
#define SCL_PIN 5
//#define I2C_FASTMODE 1

#include<SoftI2CMaster.h>

//#include <SoftWire.h> //hopefully this will transparently replace Wire.. no it didnt
//#include <Wire.h>   //consumes too much ram, replacing with SoftI2cMaster
//#include "RTClib.h" //required for DateTime definition
//DS1307 rtc;

// struct for storiong date time info
typedef struct datetime{
  byte  year, month, day, hour, minute, second;  
}DateTime;

#define DEBUG

#ifdef DEBUG            //enable debugging readouts
//#define DEBUG_LOOP    //mprint out loop markers and update time
#define DEBUG_UPDATE    //measure how long an update takes
#define DEBUG_SDCARD    //measure how long SD car access takes
//#define DEBUG_ANALOG  //measure how long analog read and processing takes
#endif


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
// eeprom addresses
/* as there is no way of incrementing a macro using the preprocessesor,
 * an altternative method of assigning eeprom addresses is required
 * my best guess just now is to define a struct and use some built in 
 * function to get the offsets
 * indeed, the following concludes with the same:
 * https://forum.arduino.cc/t/solved-how-to-structure-data-in-eeprom-variable-address/196538
 * the function is __builtin_offsetof also defined as offsetof (type, member | .member | [expr])
 */

/* flags for controlling log destination and format */

#define GET_FLAG(flag)  ((flags&(1<<flag)) != 0)
#define SET_FLAG(flag)  (flags |= (1<<flag))
#define CLR_FLAG(flag)  (flags &= ~(1<<flag))
#define TOG_FLAG(flag)  (flags ^= (1<<flag))
//byte flags;
//#define FLAG_...       1

//alternatively, use bitfields
//some of these flags need to be stored in EEPROM, some do not
typedef struct {
    byte do_serial_write:1;
    byte do_serial_write_hex:1;
    byte do_sdcard_write:1;
    byte do_sdcard_write_hex:1;
    byte sd_card_available:1;
}FLAGS;

FLAGS flags;

#define EEP_VERSION 0
struct eeprom
{
    byte         eep_version;
    unsigned int servo0_min_us, servo0_max_us;
    unsigned int servo1_min_us, servo1_max_us;
    unsigned int servo2_min_us, servo2_max_us;
    unsigned int map_cal_zero, map_cal_min;
    unsigned int knob_0_min, knob_0_max;
    unsigned int knob_1_min, knob_1_max;
    unsigned int knob_2_min, knob_2_max;
    FLAGS        flags;
    
};
#define EEP_ADDR(member)      offsetof(struct eeprom, member)
#define EEP_GET(member,dst)   EEPROM.get(EEP_ADDR(member), dst);
#define EEP_PUT(member,src)   EEPROM.put(EEP_ADDR(member), src);

// sketching out some constants for IO, with future upgradability in mind

//datalogger SD card CS pin
#define PIN_LOG_SDCARD_CS         10

//rpm counter interrupt pin
#define PIN_RPM_COUNTER_INPUT     2 //INT0

// control input pins
#define PIN_CONTROL_INPUT_1       A0
#define PIN_CONTROL_INPUT_2       A1

//#define CONTROL_INPUT_1_NAME      "Reactor Inlet Valve Target"
//#define CONTROL_INPUT_2_NAME      "Engine Inlet Valve Target"

// sensor types - probably ought to be an enum
#define ST_ANALOG                 0
#define ST_I2C                    1
#define ST_SPI                    2

// pressure sensor configuration
#define NO_OF_MAP_SENSORS         1

// pressure sensor 1
#define SENSOR_TYPE_MAP_1         ST_ANALOG
#define PIN_INPUT_MAP_1           A2
//#define SENSOR_MAP_1_NAME         "Reactor Fuel Inlet Pressure"
#define SENSOR_MAP_1_MIN          1000    //analog sample value for absolute zero pressure
#define SENSOR_MAP_1_ZERO         80      //analog sample value for 1 atm pressure

// temperature sensor configuration
#define NO_OF_EGT_SENSORS         1

#define MAX6675                   1

// temperature sensor
#if NO_OF_EGT_SENSORS > 0
  #define SENSOR_TYPE_EGT_1         ST_SPI
  #define PIN_SPI_EGT_1_CS          7
  #define SENSOR_MODEL_EGT_1        MAX6675
//  #define SENSOR_EGT_1_NAME         "Reactor Exhaust Inlet Temperature"

  #if SENSOR_MODEL_EGT_1 == MAX6675
GyverMAX6675_SPI<PIN_SPI_EGT_1_CS> EGTSensor1;
  #endif
#endif

//min max values for the control knobs
#define KNOB_0_MIN 90
#define KNOB_0_MAX 930
#define KNOB_1_MIN 10
#define KNOB_1_MAX 1014
#define KNOB_2_MIN 10
#define KNOB_2_MAX 1014

// servo output configuration
#define NO_OF_SERVOS              3

// servo 1
#if NO_OF_SERVOS > 0
  #define PIN_SERVO_0               3
  #define SERVO_0_MAX               750     //signal to move valve to 'closed' position
  #define SERVO_0_MIN               2300    //signal to move valve to 'open' position
//  #define SERVO_0_NAME              "Reactor Inlet Valve Servo"
Servo servo0;
#endif

// servo 2
#if NO_OF_SERVOS > 1
  #define PIN_SERVO_1               5
  #define SERVO_1_MAX               750     //signal to move valve to 'closed' position
  #define SERVO_1_MIN               2300    //signal to move valve to 'open' position
//  #define SERVO_1_NAME              "Engine Inlet Valve Servo"
Servo servo1;
#endif

// servo 3
#if NO_OF_SERVOS > 2
  #define PIN_SERVO_2               6       //'stop/freewheel' is assumed to be centre
  #define SERVO_2_MAX               2300    //signal to set BLDC controller to full 'drive' 
  #define SERVO_2_MIN               750     //signal to set BLDC controller to full 'brake'
//  #define SERVO_2_NAME              "Starter/Generator Motor Control"
Servo servo2;
#endif



// update rates

// screen/log file update
#define UPDATE_INTERVAL_ms          1000
#define UPDATE_START_ms             (UPDATE_INTERVAL_ms + 0)      //setting update loops to start slightly offset so they aren't tying to execute at the same time

// digital themocouple update rate
#define EGT_SAMPLE_INTERVAL_ms      250 //max update rate
#define EGT_SAMPLES_PER_UPDATE      (UPDATE_INTERVAL_ms/EGT_SAMPLE_INTERVAL_ms)
#define EGT_UPDATE_START_ms         (EGT_SAMPLE_INTERVAL_ms - 30)

//analog input read rate
#define ANALOG_SAMPLE_INTERVAL_ms   100   //record analog values this often
#define ANALOG_SAMPLES_PER_UPDATE   (UPDATE_INTERVAL_ms/ANALOG_SAMPLE_INTERVAL_ms)
#define ANALOG_UPDATE_START_ms      (ANALOG_SAMPLE_INTERVAL_ms - 20)

//rpm counter params
#define MAX_RPM                     4500
#define MIN_RPM_TICK_INTERVAL_ms    (60000/MAX_RPM)
#define MAX_RPM_TICKS_PER_UPDATE    (UPDATE_INTERVAL_ms/MIN_RPM_TICK_INTERVAL_ms)
#define MAX_RPM_TICK_INTERVAL_ms    250
#define MIN_RPM                     (60000/MAX_RPM_TICK_INTERVAL_ms)

#define PID_UPDATE_INTERVAL_ms      50
#define PID_UPDATE_START_ms         (PID_UPDATE_INTERVAL_ms - 10)

typedef void(*SCREEN_DRAW_FUNC)(void);

/* pointer to the func to use to draw the screen */
SCREEN_DRAW_FUNC draw_screen_func;

/* struct for storing sensor samples over the update period */
#define DATA_RECORD_VERSION     1

typedef struct data_record
{
  long int timestamp;                   // TODO: use the RTC for this
  unsigned int A0[ANALOG_SAMPLES_PER_UPDATE];    // TODO: replace these with meaningful values, consider seperate update rates, lower resolution
  unsigned int A1[ANALOG_SAMPLES_PER_UPDATE];
  unsigned int A2[ANALOG_SAMPLES_PER_UPDATE];
  unsigned int A3[ANALOG_SAMPLES_PER_UPDATE];
  unsigned int A0_avg;
  unsigned int A1_avg;
  unsigned int A2_avg;
  unsigned int A3_avg;
  byte ANA_no_of_samples;
  int EGT[EGT_SAMPLES_PER_UPDATE];
  int EGT_avg;
  byte EGT_no_of_samples;
  int RPM_avg;                          //average rpm over this time period, can be caluclated from number of ticks and update rate
  byte RPM_no_of_ticks;                 //so we can use bytes for storage here and have 235RPM as the slowest measurable rpm by tick time.
  byte RPM_tick_times_ms[MAX_RPM_TICKS_PER_UPDATE];           //rpm is between 1500 and 4500, giving tick times in ms of 40 and 13.3, 
} DATA_RECORD; //... bytes per record with current settings

/* struct for wrapping around any data */ //this is unneccesarily general
typedef struct data_storage
{
  unsigned int bytes_stored;
  byte hash;
  byte *data;
}DATA_STORAGE;


/* the records to write to */
DATA_RECORD Data_Record[2];
/* the index of the currently written record */
byte Data_Record_write_idx = 0;

#define CURRENT_RECORD    Data_Record[Data_Record_write_idx]




char output_filename[13] = ""; //8+3 format

/* generate a hash for the given data storage struct */
void hash_data(DATA_STORAGE *data)
{
  byte hash = 0;
  for (unsigned int next_byte = 0; next_byte < data->bytes_stored; next_byte++)
  {
    byte index = next_byte;
    index += data->data[next_byte];
    hash = hash ^ index;
  }
  data->hash = hash;
}

void serial_print_int_array(int *array_data, unsigned int array_size, const char *title_str)
{
  MAKE_STRING(LIST_SEPARATOR);
  Serial.print(title_str);
  for(int idx = 0; idx < array_size; idx++)
  {
    Serial.print(array_data[idx]);
    Serial.print(LIST_SEPARATOR_str);
  }
  Serial.println();
}
void serial_print_byte_array(byte *array_data, unsigned int array_size, const char *title_str)
{
  MAKE_STRING(LIST_SEPARATOR);
  Serial.print(title_str);
  for(int idx = 0; idx < array_size; idx++)
  {
    Serial.print(array_data[idx]);
    Serial.print(LIST_SEPARATOR_str);
  }
  Serial.println();
}
void file_print_int_array(File *file, int *array_data, unsigned int array_size, const char *title_str)
{
  MAKE_STRING(LIST_SEPARATOR);
  file->print(title_str);
  for(int idx = 0; idx < array_size; idx++)
  {
    file->print(array_data[idx]);
    file->print(LIST_SEPARATOR_str);
  }
  file->println();
}
void file_print_byte_array(File *file, byte *array_data, unsigned int array_size, const char *title_str)
{
  MAKE_STRING(LIST_SEPARATOR);
  file->print(title_str);
  for(int idx = 0; idx < array_size; idx++)
  {
    file->print(array_data[idx]);
    file->print(LIST_SEPARATOR_str);
  }
  file->println();
}

/* reset the back record after use */
void reset_record()
{
  Data_Record[1-Data_Record_write_idx] = (DATA_RECORD){0};
  Data_Record[1-Data_Record_write_idx].timestamp = millis();
}
/* swap records and calculate averages */
void finalise_record()
{
  /* switch the records */
  Data_Record_write_idx = 1-Data_Record_write_idx; 
  
  /* data record to read */
  DATA_RECORD *data_record = &Data_Record[1-Data_Record_write_idx];
  
  
  /* calculate averages */
  if(data_record->ANA_no_of_samples > 0)
  {
    int rounding = data_record->ANA_no_of_samples/2;
    data_record->A0_avg += rounding;
    data_record->A1_avg += rounding;
    data_record->A2_avg += rounding;
    data_record->A3_avg += rounding;
    
    data_record->A0_avg /= data_record->ANA_no_of_samples;
    data_record->A1_avg /= data_record->ANA_no_of_samples;
    data_record->A2_avg /= data_record->ANA_no_of_samples;
    data_record->A3_avg /= data_record->ANA_no_of_samples;
  }
  if(data_record->EGT_no_of_samples > 0)
  {
    unsigned int rounding = data_record->EGT_no_of_samples/2;
    data_record->EGT_avg += rounding;
    data_record->EGT_avg /= data_record->EGT_no_of_samples;
  }
//  if(data_record->RPM_no_of_samples > 0)
//  {
    // for rpm, we can approximate by the number of ticks
    data_record->RPM_avg = data_record->RPM_no_of_ticks * 60000/UPDATE_INTERVAL_ms;
    // for a more acurate reading, we must sum all tick intervals
    // and divide by number of ticks
    // but this should be sufficient
    
    //unsigned int rounding = data_record->RPM_no_of_samples/2;
    //data_record->RPM_avg += rounding;
    //data_record->RPM_avg = (unsigned int)(60000*(unsigned long int)data_record->RPM_no_of_samples/data_record->RPM_avg);  
//  }
}
/* writes the current data record to serial port and SD card as required */
void write_data_record()
{
  /* data record to read */
  DATA_RECORD *data_record = &Data_Record[1-Data_Record_write_idx];

  /* we can break this down into subsections if the overall process time
   * is greater than the shortest snesor read interval */
   
  /* hash the data for hex storage*/
  DATA_STORAGE data_store = {data_record, sizeof(DATA_STORAGE), 0};
  hash_data(&data_store);

  /* send the data to the serial port */
  if(flags.do_serial_write)
  {
    if(flags.do_serial_write_hex)
    {
      
      Serial.write((byte*)data_store.bytes_stored, sizeof(data_store.bytes_stored));    //write the number of bytes sent
      Serial.write(data_store.hash);                                              //write the hash value
      Serial.write(data_store.data, sizeof(data_store.bytes_stored));            //write the data
    }
    else
    {
      Serial.println(F("----------"));
      MAKE_STRING(RECORD_VER_C);    Serial.print(RECORD_VER_C_str);     Serial.println(DATA_RECORD_VERSION);
      MAKE_STRING(TIMESTAMP_C);     Serial.print(TIMESTAMP_C_str);      Serial.println(data_record->timestamp);

      
      Serial.print(F("A0_avg: "));      Serial.println(data_record->A0_avg);
      Serial.print(F("A1_avg: "));      Serial.println(data_record->A1_avg);
      Serial.print(F("A2_avg: "));      Serial.println(data_record->A2_avg);
      Serial.print(F("A3_avg: "));      Serial.println(data_record->A3_avg);

      Serial.print(F("ANA samples: "));      Serial.println(data_record->ANA_no_of_samples);
      STRING_BUFFER(A0_C);
      GET_STRING(A0_C);             serial_print_int_array(data_record->A0, data_record->ANA_no_of_samples, string);
      GET_STRING(A1_C);             serial_print_int_array(data_record->A1, data_record->ANA_no_of_samples, string);
      GET_STRING(A2_C);             serial_print_int_array(data_record->A2, data_record->ANA_no_of_samples, string);
      GET_STRING(A3_C);             serial_print_int_array(data_record->A3, data_record->ANA_no_of_samples, string);
      Serial.print(F("EGT_avg"));      Serial.println(data_record->EGT_avg);
      Serial.print(F("EGT samples: "));      Serial.println(data_record->EGT_no_of_samples);
      MAKE_STRING(EGT1_C);          serial_print_int_array(data_record->EGT, data_record->EGT_no_of_samples, EGT1_C_str);
      MAKE_STRING(RPM_AVG);         Serial.print(RPM_AVG_str);          Serial.println(data_record->RPM_avg);
      MAKE_STRING(RPM_NO_OF_TICKS); Serial.print(RPM_NO_OF_TICKS_str);  Serial.println(data_record->RPM_no_of_ticks);
      MAKE_STRING(RPM_TICK_TIMES);  serial_print_byte_array(data_record->RPM_tick_times_ms, data_record->RPM_no_of_ticks, RPM_TICK_TIMES_str);
      Serial.println(F("----------"));
    }
  }

/* send the data to the SD card, if available */
  if(flags.do_sdcard_write && flags.sd_card_available)
  {
#ifdef DEBUG_SDCARD    
    unsigned int timestamp_ms = millis();
#endif
    
    File log_data_file = SD.open(output_filename, O_WRITE | O_APPEND);
    if (log_data_file)
    {
      /* replace all these serial calls with SD card file calls */
      if(flags.do_serial_write_hex)
      {
        log_data_file.write((byte*)data_store.bytes_stored, sizeof(data_store.bytes_stored));    //write the number of bytes sent
        log_data_file.write(data_store.hash);                                              //write the hash value
        log_data_file.write(data_store.data, sizeof(data_store.bytes_stored));            //write the data
      }
      else
      {
        //MAKE_STRING(RECORD_VER_C);    log_data_file.print(RECORD_VER_C_str);    log_data_file.println(DATA_RECORD_VERSION);
        MAKE_STRING(TIMESTAMP_C);     log_data_file.print(TIMESTAMP_C_str);     log_data_file.println(data_record->timestamp);
        STRING_BUFFER(A0_C);
        GET_STRING(A0_C);             file_print_int_array(&log_data_file, data_record->A0, ANALOG_SAMPLES_PER_UPDATE, string);
        GET_STRING(A1_C);             file_print_int_array(&log_data_file, data_record->A1, ANALOG_SAMPLES_PER_UPDATE, string);
        GET_STRING(A2_C);             file_print_int_array(&log_data_file, data_record->A2, ANALOG_SAMPLES_PER_UPDATE, string);
        GET_STRING(A3_C);             file_print_int_array(&log_data_file, data_record->A3, ANALOG_SAMPLES_PER_UPDATE, string);
        MAKE_STRING(EGT1_C);          file_print_int_array(&log_data_file, data_record->EGT, EGT_SAMPLES_PER_UPDATE, EGT1_C_str);
        MAKE_STRING(RPM_AVG);         log_data_file.print(RPM_AVG_str);         log_data_file.println(data_record->RPM_avg);
        MAKE_STRING(RPM_NO_OF_TICKS); log_data_file.print(RPM_NO_OF_TICKS_str); log_data_file.println(data_record->RPM_no_of_ticks);
        MAKE_STRING(RPM_TICK_TIMES);  file_print_byte_array(&log_data_file, data_record->RPM_tick_times_ms, data_record->RPM_no_of_ticks, RPM_TICK_TIMES_str);
      }
      log_data_file.close();

#ifdef DEBUG_SDCARD
      timestamp_ms = millis() - timestamp_ms;
      Serial.print(F("SDCard Access Time (ms): "));
      Serial.println(timestamp_ms);
    }
    else
    {
      Serial.println(F("Fail: open output file"));
#endif
    }
  }
  
}

/* opens comms to display chip and calls the current screen's draw function */
void draw_screen()
{
  if (draw_screen_func != NULL)
  {
    /* restart comms with the FT810 */
    GD.resume();
    /* draw the current screen */
    draw_screen_func();
    /* display the screen */
    GD.swap();
    /* stop comms to FT810, so other devices (ie MAX6675, SD CARD) can use the bus */
    GD.__end();
  }
}


const char PROGMEM hex_char[] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
#define HEX_CHAR(value)   pgm_read_byte_near(hex_char + (value&0xf))
#define DEC_CHAR(value)   pgm_read_byte_near(hex_char + (value%10))

/* generate a filename from the rtc date/time
 *  if more than 255 files are required in a day, we'll
 *  have to make use of sub folders (or increase the base of NN to 36)
 *  eg /001/YY_MM_DD.txt
 *  or /YYYMMDD/LOG_0001.txt
 *  meanwhile, we'll just overwrite older files
 */

void generate_file_name()
{
  char *str_ptr;
  
  int value;
  byte nn = 0;

  // set the file extension
  str_ptr = output_filename + 8;
  if(flags.do_sdcard_write_hex) {strcpy_P(str_ptr,FILE_EXT_RAW);}
  else                          {strcpy_P(str_ptr,FILE_EXT_TXT);}

  //set the base name from the year month and day
  DateTime now = DS1307_date();
  //fill in the digits backwards
  str_ptr = output_filename + 5;
  
  value = now.day;
  *str_ptr-- = DEC_CHAR(value);
  value /=10;
  *str_ptr-- = DEC_CHAR(value);

  value = now.month;
  *str_ptr-- = DEC_CHAR(value);
  value /=10;
  *str_ptr-- = DEC_CHAR(value);
  
  value = now.year;
  *str_ptr-- = DEC_CHAR(value);
  value /=10;
  *str_ptr-- = DEC_CHAR(value);

  bool do_loop = true;
  while (do_loop)
  {
    //set the iterator number
    str_ptr = output_filename + 7;

    value = nn++;
    *str_ptr-- = HEX_CHAR(value);
    value >>=1;
    *str_ptr-- = HEX_CHAR(value);

    //break the loop if the target file doenst exist, or we have run out of indices
    if (!SD.exists(output_filename) || nn==0 )
    {
      do_loop = false;
    }
  }
  //report the selected filename
  MAKE_STRING(OUTPUT_FILE_NAME_C);
  Serial.print(OUTPUT_FILE_NAME_C_str);
  Serial.println(output_filename);
  
  //check that the file can be opened
  File dataFile = SD.open(output_filename, FILE_WRITE);
  if(dataFile)
  {
    MAKE_STRING(RECORD_VER_C);    dataFile.print(RECORD_VER_C_str);    dataFile.println(DATA_RECORD_VERSION);
    dataFile.close();
    
    Serial.println(F("Opened OK"));
    flags.do_sdcard_write = true;

    if(nn == 0)
    {
      Serial.println(F("Overwriting older file"));
    }
  }
  else
  {
    Serial.println(F("Open FAILED"));
    flags.do_sdcard_write = false;
  }

  
}

static int update_timestamp = 0; //not much I can do easily about these timestamps
static int analog_timestamp = 0; //could be optimised by using a timer and a progmem table of func calls
static int egt_timestamp = 0;    //more optimal if all intervals have large common root
static int pid_timestamp = 0;    //they probably dont need to be long ints though.
                                 //short ints can handle an interval of 32 seconds

void setup() {

  //set datalogger SDCard CS pin high, 
  pinMode(PIN_LOG_SDCARD_CS, OUTPUT);
  digitalWrite(PIN_LOG_SDCARD_CS, HIGH);
  // set Gameduino CS pins high
  pinMode(9, OUTPUT);
  digitalWrite(9, HIGH);
  pinMode(8, OUTPUT);
  digitalWrite(8, HIGH);
 
  //configure the RPM input
  configiure_rpm_counter();
  
  //initialise servos
  #if NO_OF_SERVOS > 0
  servo0.attach(PIN_SERVO_0);
  #endif
  #if NO_OF_SERVOS > 1
  servo1.attach(PIN_SERVO_1);
  #endif
  #if NO_OF_SERVOS > 2
  servo2.attach(PIN_SERVO_2);
  #endif

  
  //fetch the firmware fersion string
  
  char firwmare_string[strlen_P(FIRMWARE_NAME) + strlen_P(FIRMWARE_VERSION) + strlen_P(LIST_SEPARATOR) + 1];
  char *string_ptr = firwmare_string;
  PUT_STRING(FIRMWARE_NAME, string_ptr);
  string_ptr += strlen_P(FIRMWARE_NAME);
  PUT_STRING(LIST_SEPARATOR, string_ptr);
  string_ptr += strlen_P(LIST_SEPARATOR);
  PUT_STRING(FIRMWARE_VERSION, string_ptr);
  
  //initialise the serial port:
  Serial.begin(115200);
  Serial.println();
  Serial.println(firwmare_string);
  Serial.println();

  //initialise RTC
  i2c_init();

  //431 bytes left useing Wire.h
  //667 bytes left using SoftI2CMaster and tiny_rtc
  if (!DS1307_isrunning())
  {
    Serial.println(F("Setting RTC to compile date/time"));
    DS1307_adjust(CompileDateTime());
  }
  
  DateTime now = DS1307_now();
  serial_print_date_time(now);

  
  //set default flags
  flags.do_serial_write = true;
  flags.do_sdcard_write = false;

  //reset eeprom if a different version
  reset_eeprom();

  //read flag configuration from eeprom
  EEP_GET(flags,flags);


  //attempt to initialse the logging SD card. this will include creating a new file from the RTC date for immediate logging

  //Serial.print("Initializing SD card...");
  // see if the card is present and can be initialized:
  STRING_BUFFER(CARD_FAILED_OR_NOT_PRESENT);
  
  
  if (!SD.begin(PIN_LOG_SDCARD_CS)) {
    GET_STRING(CARD_FAILED_OR_NOT_PRESENT);
    Serial.println(string);
    flags.sd_card_available = false;
  }
  else
  {
    GET_STRING(CARD_INITIALISED);
    Serial.println(string);
    flags.sd_card_available = true;

    generate_file_name();
  }

  
  //initialise the display
  GD.begin(0);
  // draw splash screen
  GD.Clear();
  GD.cmd_text(GD.w/2, GD.h/2-32, 29, OPT_CENTER, firwmare_string);  //firmware name and version
  GD.cmd_text(GD.w/2, GD.h/2+32, 27, OPT_CENTER, string);           //sd card status
  if (flags.sd_card_available && flags.do_sdcard_write)             //output file name, if valid
  {
    MAKE_STRING(OUTPUT_FILE_NAME_C);
    GD.cmd_text(GD.w/2, GD.h/2+64, 26, OPT_CENTERY | OPT_RIGHTX, OUTPUT_FILE_NAME_C_str);
    GD.cmd_text(GD.w/2, GD.h/2+64, 26, OPT_CENTERY, output_filename);
  }
  //date and time
  {
    char datetime_string[11];
    date_to_string(now, string);
    GD.cmd_text(GD.w/2-96,  GD.h-64, 26, OPT_CENTER, datetime_string);
    time_to_string(now, string);
    GD.cmd_text(GD.w/2+96,  GD.h-64, 26, OPT_CENTER, datetime_string);
  }
  
  GD.swap();
  GD.__end();

  //set the first screen to be drawn
  draw_screen_func = draw_basic;

  
  
  //start the ADC
  analogRead(A0);
  analogRead(A1);
  analogRead(A2);
  analogRead(A3);


  //set initial servo conditions
  servo0.writeMicroseconds(map(1,0,2,SERVO_0_MIN, SERVO_0_MAX));
  servo1.writeMicroseconds(map(1,0,2,SERVO_1_MIN, SERVO_1_MAX));
  servo2.writeMicroseconds(map(1,0,2,SERVO_2_MIN, SERVO_2_MAX));
  
  // wait for MAX chip to stabilize, and to see the splash screen
  delay(4000);

  int timenow = millis();

  //set the initial times for the loop events
  update_timestamp = timenow + UPDATE_START_ms;
  analog_timestamp = timenow + ANALOG_UPDATE_START_ms;
  egt_timestamp = timenow + EGT_UPDATE_START_ms;
  pid_timestamp = timenow + PID_UPDATE_START_ms;
}


/* some low resolution mapped analog values */
byte MAP_pressure_abs;
byte KNOB_value_0;
byte KNOB_value_1;
byte KNOB_value_2;

void process_analog_inputs()
{
  
}
void process_digital_inputs()
{
  
}
void process_pid_loop()
{
  
}
void process_display_loop()
{
  
}
void loop() {

  int elapsed_time;
  int timenow = millis();

  /* get time left till next update */
  elapsed_time = update_timestamp - timenow;
  /* execute update if update interval has elapsed */
  if(elapsed_time < 0)
  {
    /* set the timestamp for the next update */
    int timelag = elapsed_time%UPDATE_INTERVAL_ms;
    update_timestamp = timenow + UPDATE_INTERVAL_ms + timelag;

    /* emit the timestamp of the current update, if debugging */
    #ifdef DEBUG_LOOP
    Serial.print(F("Screen Update: ")); Serial.println(timenow);
    #endif

    
    // swap the buffers and finalise averages
    finalise_record();
    
    // draw the current screen
    draw_screen();
    
    // write the current data to target media
    write_data_record();
    
    //reset the record ready for next time
    reset_record();
    
    //update the time
    timenow = millis();
    
  }

  /* get time left till next update */
  elapsed_time = egt_timestamp - timenow;
  /* execute update if update interval has elapsed */
  if(elapsed_time < 0)
  {
    /* set the timestamp for the next update */
    int timelag = elapsed_time%EGT_SAMPLE_INTERVAL_ms;
    egt_timestamp = timenow + EGT_SAMPLE_INTERVAL_ms + timelag;

    // debug marker
    #ifdef DEBUG_LOOP
    Serial.print(F("EGT Read: ")); Serial.println(timenow);
    #endif
    //collect the EGT reading  
    
    if (CURRENT_RECORD.EGT_no_of_samples < EGT_SAMPLES_PER_UPDATE)
    {
      if(EGTSensor1.readTemp())
      {
        int EGT1_value = EGTSensor1.getTempIntRaw();
        CURRENT_RECORD.EGT[CURRENT_RECORD.EGT_no_of_samples++] = EGT1_value;
        CURRENT_RECORD.EGT_avg += EGT1_value;
      }
      
    }
    //update the time
    timenow = millis();
  }
  
  /* get time left till next update */
  elapsed_time = analog_timestamp - timenow;
  /* execute update if update interval has elapsed */
  if(elapsed_time < 0)
  {
    /* set the timestamp for the next update */
    int timelag = elapsed_time%ANALOG_SAMPLE_INTERVAL_ms;
    analog_timestamp = timenow + ANALOG_SAMPLE_INTERVAL_ms + timelag;

    //debug marker
    #ifdef DEBUG_LOOP
    Serial.print(F("ANA Read: "));Serial.println(timenow);
    #endif

    //collect analog inputs
    int a0 = analogRead(A0);
    int a1 = analogRead(A1);
    int a2 = analogRead(A2);
    int a3 = analogRead(A3);

    //do some sort of processing with these values
    //for now we want to use a1-a3 to directly control servos
    // and convert a0 to a vaccuum as a percentile (or roughly kPa)
    // we need to store these values statically, 
    // and reference them in a pid control loop that controls servo output
    int map_min, map_max, val;
    
    EEP_GET(map_cal_min, map_min);
    EEP_GET(map_cal_zero, map_max);
    MAP_pressure_abs = constrain(map(a0, map_min, map_max, 0, 101), 0, 255); 

    EEP_GET(knob_0_min, map_min);
    EEP_GET(knob_0_max, map_max);
    KNOB_value_0 = constrain(map(a1, map_min, map_max, 0, 256), 0, 255); 

    EEP_GET(knob_1_min, map_min);
    EEP_GET(knob_1_max, map_max);
    KNOB_value_1 = constrain(map(a2, map_min, map_max, 0, 256), 0, 255);

    EEP_GET(knob_2_min, map_min);
    EEP_GET(knob_2_max, map_max);
    KNOB_value_2 = constrain(map(a3, map_min, map_max, 0, 256), 0, 255);
    
    
    //log the analog values, if there's space in the current record
    if (CURRENT_RECORD.ANA_no_of_samples < ANALOG_SAMPLES_PER_UPDATE)
    {
      #ifdef DEBUG_ANALOG
      unsigned int timestamp_us = micros();
      #endif

      //we'll continue to log raw values for now
      unsigned int analog_index = CURRENT_RECORD.ANA_no_of_samples++;
      CURRENT_RECORD.A0[analog_index] = a0;
      CURRENT_RECORD.A1[analog_index] = a1;
      CURRENT_RECORD.A2[analog_index] = a2;
      CURRENT_RECORD.A3[analog_index] = a3;
      CURRENT_RECORD.A0_avg += CURRENT_RECORD.A0[analog_index];
      CURRENT_RECORD.A1_avg += CURRENT_RECORD.A1[analog_index];
      CURRENT_RECORD.A2_avg += CURRENT_RECORD.A2[analog_index];
      CURRENT_RECORD.A3_avg += CURRENT_RECORD.A3[analog_index];
    }

    #ifdef DEBUG_ANALOG
    timestamp_us = micros() - timestamp_us;
    Serial.print(F("Analog Read Time (us): "));
    Serial.println(timestamp_us);
    #endif

    //update the time
    timenow = millis();
  }

  /* servo library produces continous signal without needing .write_microseconds to be called repeatedly,
   *  so no independant loop is required - just write the values upon calculation
   *  
   *  however, we want to run a pid loop to calculate servo demand based on operating conditions
   *  and we may want to run the pid loo at a different rate to the sampling rate
   *  eg EGT readings with a MAX6675 update every 250ms, 
   *  analog ports are sampled at 100ms, but could be much higher if long events like sd card writes are broken up
   *  or an interupt is used to stash analog values
   *  update rate is limited by whatever loop call takes the longest
   *  break longer loops into steps for better performance
   */
  /* get time left till next update */
  elapsed_time = pid_timestamp - timenow;
  /* execute update if update interval has elapsed */
  if(elapsed_time < 0)
  {
    /* set the timestamp for the next update */
    int timelag = elapsed_time%PID_UPDATE_INTERVAL_ms;
    pid_timestamp = timenow + PID_UPDATE_INTERVAL_ms + timelag;

    //debug marker
    #ifdef DEBUG_PID
    Serial.print(F("PID Loop: ")); Serial.println(timenow);
    #endif  

    // process invloves somthing like:
    // opening the air managemnt valve to increase rpm, closing to reduce rpm
    // opening reactor inlet valve to reduce vacuum, closing to increace vacuum
    // opening RIV and closing AMV to reduce exhaust temperature

    //this will be detirmined after collecting some data under manual control
    // which is the point in datalogging

    // buffers for the eeprom min max values
    unsigned int servo_min_us, servo_max_us;
    unsigned int servo_pos_us;
    
  #if NO_OF_SERVOS > 0
    // fetch the servo times from eeprom
    EEP_GET(servo0_min_us, servo_min_us);
    EEP_GET(servo0_max_us, servo_max_us);
    servo_pos_us = map(KNOB_value_0, 0, 256, servo_min_us, servo_max_us);
    servo0.writeMicroseconds(servo_pos_us);
    #ifdef DEBUG_SERVO
    Serial.print(F("Servo0: ")); Serial.println(servo_pos_us);
    #endif
  #endif
  #if NO_OF_SERVOS > 1
    EEP_GET(servo1_min_us, servo_min_us);
    EEP_GET(servo1_max_us, servo_max_us);
    servo_pos_us = map(KNOB_value_1, 0, 256, servo_min_us, servo_max_us);
    servo1.writeMicroseconds(servo_pos_us);
    #ifdef DEBUG_SERVO
    Serial.print(F("Servo1: ")); Serial.println(servo_pos_us);
    #endif
  #endif
  #if NO_OF_SERVOS > 2
    EEP_GET(servo2_min_us, servo_min_us);
    EEP_GET(servo2_max_us, servo_max_us);
    servo_pos_us = map(KNOB_value_2, 0, 256, servo_min_us, servo_max_us);
    servo2.writeMicroseconds(servo_pos_us);
    #ifdef DEBUG_SERVO
    Serial.print(F("Servo2: ")); Serial.println(servo_pos_us);
    #endif
  #endif
      
  }
  
}
