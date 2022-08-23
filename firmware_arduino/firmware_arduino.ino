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

#include <SD.h>         //sd card library
#include <SPI.h>        //needed for gameduino
#include <GD2.h>        //Gameduino (FT810 plus micro sdcard
#include <GyverMAX6675_SPI.h>    //digital thermocouple interface using SPI port

#include <Servo.h>      //servo control disables analog write on pins 9 and 10

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

// servo output configuration
#define NO_OF_SERVOS              2

// servo 1
#if NO_OF_SERVOS > 0
  #define PIN_SERVO_1               4
  #define SERVO_1_MAX               2300
  #define SERVO_1_MIN               750
//  #define SERVO_1_NAME              "Reactor Inlet Valve Servo"
#endif

// servo 2
#if NO_OF_SERVOS > 1
  #define PIN_SERVO_2               5
  #define SERVO_2_MAX               2300
  #define SERVO_2_MIN               750
//  #define SERVO_2_NAME              "Engine Inlet Valve Servo"
#endif

// servo 3
#if NO_OF_SERVOS > 2
  #define PIN_SERVO_2               6
  #define SERVO_2_MAX               2300
  #define SERVO_2_MIN               750
//  #define SERVO_2_NAME              "Starter/Generator Motor Control"
#endif

// rpm counter configuration

#define PIN_SENSOR_RPM              3     //ideally should be a timer capture pin


// update rates

// screen/log file update
#define UPDATE_INTERVAL_ms          1000

// digital themocouple update rate
#define EGT_SAMPLE_INTERVAL_ms      250 //max update rate
#define EGT_SAMPLES_PER_UPDATE    (UPDATE_INTERVAL_ms/EGT_SAMPLE_INTERVAL_ms)

//analog input read rate
#define ANALOG_SAMPLE_INTERVAL_ms   100   //record analog values this often
#define ANALOG_SAMPLES_PER_UPDATE   (UPDATE_INTERVAL_ms/ANALOG_SAMPLE_INTERVAL_ms)

//rpm counter params
#define MAX_RPM                     4500
#define MIN_RPM_TICK_INTERVAL_ms    (60000/MAX_RPM)
#define MAX_RPM_TICKS_PER_UPDATE    (UPDATE_INTERVAL_ms/MIN_RPM_TICK_INTERVAL_ms)
#define MAX_RPM_TICK_INTERVAL_ms     250
#define MIN_RPM                     (60000/MAX_RPM_TICK_INTERVAL_ms)

typedef void(*SCREEN_DRAW_FUNC)(void);

/* pointer to the func to use to draw the screen */
SCREEN_DRAW_FUNC draw_screen_func;

/* struct for storing sensor samples over the update period */
#define DATA_RECORD_VERSION     1

typedef struct data_record
{
  long int timestamp;                   // TODO: use the RTC for this
  unsigned int A0[ANALOG_SAMPLES_PER_UPDATE];    // TODO: replace these with meaningful values, consider seperate update rates
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
  int RPM_avg;                          //average rpm over this time period
  byte RPM_no_of_ticks;                 //so we can use bytes for storage here and have 15.3RPM as the slowest measurable rpm.
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


/* flags for controlling log destination and format */

#define GET_FLAG(flag)  ((flags&(1<<flag)) != 0)
#define SET_FLAG(flag)  (flags |= (1<<flag))
#define CLR_FLAG(flag)  (flags &= ~(1<<flag))
#define TOG_FLAG(flag)  (flags ^= (1<<flag))
//byte flags;
//#define FLAG_...       1

//alternatively, use bitfields
typedef struct {
    byte do_serial_write:1;
    byte do_serial_write_hex:1;
    byte do_sdcard_write:1;
    byte do_sdcard_write_hex:1;
    byte sd_card_available:1;
}FLAGS;

FLAGS flags;

char output_filename[16];

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
  /*
  // data record to reset 
  DATA_RECORD *data_record = &Data_Record[1-Data_Record_write_idx];
  // reset counters 
  data_record->EGT_no_of_samples = 0;
  data_record->ANA_no_of_samples = 0;
  data_record->RPM_no_of_ticks = 0;
  // reset averages 
  data_record->A0_avg;
  data_record->A1_avg;
  data_record->A2_avg;
  data_record->A3_avg;
  data_record->EGT_avg = 0;
  data_record->RPM_avg = 0;  
  */
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

static long int update_timestamp = 0; //not much I can do easily about these timestamps
static long int analog_timestamp = 0; //could be optimised by using a timer and a progmem table of func calls
static long int egt_timestamp = 0;    //more optimal if all intervals have large common root

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
  congiure_rpm_counter();
  
  //initialise RTC


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

  //read configuration from eeprom

  flags.do_serial_write = true;
  flags.do_sdcard_write = false;




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

    // generate a filename
    strcpy_P(output_filename,DEFAULT_FILENAME);
    char *str_ptr = output_filename + strlen_P(DEFAULT_FILENAME);
    if(flags.do_sdcard_write_hex) {strcpy_P(str_ptr,FILE_EXT_RAW);}
    else                          {strcpy_P(str_ptr,FILE_EXT_TXT);}
    
    MAKE_STRING(OUTPUT_FILE_NAME_C);
    Serial.print(OUTPUT_FILE_NAME_C_str);
    Serial.println(output_filename);
    
    File dataFile = SD.open(output_filename, FILE_WRITE);
    if(dataFile)
    {
      MAKE_STRING(RECORD_VER_C);    dataFile.print(RECORD_VER_C_str);    dataFile.println(DATA_RECORD_VERSION);
      dataFile.close();
      
      Serial.println(F("Opened OK"));
      flags.do_sdcard_write = true;
    }
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
  GD.swap();
  GD.__end();

  //set the first screen to be drawn
  draw_screen_func = draw_basic;

  
  
  //start the ADC
  analogRead(A0);
  analogRead(A1);
  analogRead(A2);
  analogRead(A3);

  

  // wait for MAX chip to stabilize, and to see the splash screen
  delay(4000);
}




void loop() {

  long int elapsed_time;
  long int timenow = millis();

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
    Serial.print(F("Update Time: "));
    Serial.println(timenow);
    #endif

    
    // swap the buffers and finalise averages
    finalise_record();
    
    // draw the current screen
    draw_screen();
    
    // write the current data to target media
    write_data_record();
    
    //reset the record ready for next time
    reset_record();
    
    //update the time?
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
    Serial.println(F("EGT Read"));
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
    //update the time?
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
    Serial.println(F("ANA Read"));
    #endif
    
    //collect analog inputs
    if (CURRENT_RECORD.ANA_no_of_samples < ANALOG_SAMPLES_PER_UPDATE)
    {
      #ifdef DEBUG_ANALOG
      unsigned int timestamp_us = micros();
      #endif
      
      unsigned int analog_index = CURRENT_RECORD.ANA_no_of_samples++;
      CURRENT_RECORD.A0[analog_index] = analogRead(A0);
      CURRENT_RECORD.A1[analog_index] = analogRead(A1);
      CURRENT_RECORD.A2[analog_index] = analogRead(A2);
      CURRENT_RECORD.A3[analog_index] = analogRead(A3);
      CURRENT_RECORD.A0_avg += CURRENT_RECORD.A0[analog_index];
      CURRENT_RECORD.A1_avg += CURRENT_RECORD.A1[analog_index];
      CURRENT_RECORD.A2_avg += CURRENT_RECORD.A2[analog_index];
      CURRENT_RECORD.A3_avg += CURRENT_RECORD.A3[analog_index];
      
      #ifdef DEBUG_ANALOG
      timestamp_us = micros() - timestamp_us;
      Serial.print(F("Analog Read Time (us): "));
      Serial.println(timestamp_us);
      #endif
    }  
    //update the time?
    //timenow = millis();
  }
  
}
