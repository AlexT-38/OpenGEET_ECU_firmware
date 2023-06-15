 /*
 * OpenGEET Engine Control Unit firmware (Arduino)
 * Created by A Timiney, 13/08/2022
 * GNU GPL v3
 * 
 * 
 * Open GEET Engine Control Unit firmware
 *
 * Reads sensor values (rpm, egt, map, torque) and user inputs, control valve positions. 
 * Optionally drives fuel injection and ignition, reads magnetic fields. Logs all values to sd card.
 * Initially, control inputs will be mapped directly to valve positions. 
 * Once the behaviour of the reactor and engine are better understood, 
 *  control input can be reduced to rpm control via PID loops. 
 * A 3-phase BLDC motor can then be added to provide engine start and power generation 
 *  using an off the shelf motor controller. Control input can then be tied to output power draw.
 * Hardware will initially be an Arduino Uno with a datalogger sheild. 
 * Upgrade to an STM10x or 40x device can be made later if required.
 */


/* modifications to the deek-robot data logger sheild:
 *  
 *  link the following pins on the underside of the arduino Mega:
 *  11-51
 *  12-50
 *  13-52
 *  
 *  pins 11-13 will not be usable.
 *  
 *  cut tracks between breakout pad for pins 4 & 5 and the adjacent SDA and SCL breakout pads
 *  connect the SDA and SCL pads to the SDA and SCL pads adjecent to AREF pin
 *  
 *  run a 1 ohm resistor and a ferrite bead (eg BLM18HG102SN1, 100mA 1000ohm @100MHz) from 5V to AREF
 *  add 47uF 6.3v MLCC to GND at each end and power all analog inputs from AREF
 *  to eliminate noise from the supply when using a buck convertor to provide sufficient current for 
 *  the servos from a 12-15v 1A supply
 *  MAP sensor draws ~7.5mA, and each pot draws 0.5mA
 */
/* MEGA PIN MAP / Circuit description:
 *  MEGA Pin    I/O     Function/name
 *  0           I       USB Serial Rx
 *  1           O       USB Serial Tx
 *  2           I       RPM counter
 *  3           O       Servo 1
 *  4           I       DATA: HX711 Load Cell
 *  5           O       Servo 2
 *  6           O       Servo 3
 *  7           O       CS: EGT 1
 *  8           O       CS: Gameduino FT810
 *  9           O       CS: Gameduino SD Card / CLK: HX711 Load Cell
 *  10          O       CS: SD Card
 *  11          I       Unusable / MOSI
 *  12          I       Unusable / MISO
 *  13          I       Unusable / SCK
 *  14  A0      I       MAP 1
 *  15  A1      I       User Input 1
 *  16  A2      I       User Input 2
 *  17  A3      I       User Input 3
 *  21          O       SCL / RTC
 *  20          I/O     SDA / RTC
 *  50          I       MISO: Display, EGT, SD Card
 *  51          O       MOSI: Display, EGT, SD Card  
 *  52          O       SCK:  Display, EGT, SD Card
 *  
 */

  /* latest on ram usage:
   *  adding a PID struct and it's output to the record struct
   *  has brought free ram down to 443 bytes.
   *  it may be necessary to increase the update rate to 2 per second
   *  we might be able to scrape a few bytes using bitfields for the numbers of samples
   *  
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
  *  SD card access delay can be larger than this and is dependant on the card's buffer size.
  *  
  *  to avoid string literals, strings must be placed into progmem
  *  and read out into a temporary buffer as per https://www.arduino.cc/reference/en/language/variables/utilities/progmem/
  */
#include <EEPROM.h>
#include <SD.h>         //sd card library
#include <SPI.h>        //needed for gameduino
#include <GD2.h>        //Gameduino (FT810 plus micro sdcard

#include <Servo.h>      //servo control disables analog write on pins 9 and 10


#include "tiny_rtc.h"
#include "hx711.h"


#define DEBUG

//keep debug strings short to limit time spent sending them

#ifdef DEBUG                      //enable debugging readouts
//#define DEBUG_LOOP              //mprint out loop markers and update time                 min(us)       typ(us)       max(us)
//#define DEBUG_UPDATE_TIME       //measure how long an update takes                    t:  40000
//#define DEBUG_DISPLAY_TIME      //measure how long draw screen takes                  t:  10000
//#define DEBUG_SDCARD_TIME       //measure how long SD card log write takes            t:  30000
//#define DEBUG_SERIAL_TIME       //measure how long serial log write takes             t:
//#define DEBUG_PID_TIME          //measure how long the pid loop takes                 t:     48(direct) 76(1ch)      
//#define DEBUG_ANALOG_TIME       //measure how long analog read and processing takes   t:
//#define DEBUG_DIGITAL_TIME      //measure how long reading from digital sensors takes t:  
//#define DEBUG_TOUCH_TIME        //measure how long reading and processing touch input t:                125           300
//#define DEBUG_RECORD  // n/a
//#define DEBUG_ADC
//#define DEBUG_SERVO
//#define DEBUG_EEP_RESET
//#define DEBUG_EEP_CONTENT
//#define DEBUG_RPM_COUNTER
//#define DEBUG_MAP_CAL
//#define DEBUG_MAP  // n/a
//#define DEBUG_TMP_CAL
//#define DEBUG_SCREEN_RES        //print screen resolution during startup (should be WQVGA: 480 x 272)
//#define DEBUG_TOUCH_INPUT
//#define DEBUG_TOUCH_CAL
//#define DEBUG_TORQUE_SENSOR
//#define DEBUG_HX711
//#define DEBUG_TORQUE_RAW        //record raw values
//#define DEBUG_TORQUE_CAL        //print calibration values on set
//#define DEBUG_TORQUE_NO_TIMEOUT   //no timeout for reading torque sensor - this saves 40 or so bytes of flash, but will hang if comms lost
//#define DEBUG_TORQUE_CAL_NO_TIMEOUT //no timeout reading torque sensor calibration values, saves about 16 bytes of flash with the above, but without the above, saves 40 bytes (or, conversly CONSUMES 56 bytes)
//#define DEBUG_DISABLE_DIGITAL
//#define DEBUG_TOUCH_REG_DUMP
//#define DEBUG_CAL_TOUCH
//#define DEBUG_RTC
//#define DEBUG_RTC_FORCE_UPDATE
//#define DEBUG_SDCARD_TEST_FILE_OPEN
//#define DEBUG_SDCARD_TEST_FILE_NAME "23061200.txt"

#endif

//#define OVERWRITE_TOUCH_CAL


// I/O counts by type
#define NO_OF_USER_INPUTS         4     //physical knobs
#define NO_OF_MAP_SENSORS         1     //pressure sensors
#define NO_OF_TMP_SENSORS         0     // analog low temperature NTC / PTC sensors, manifold inlet temperatures etc
#define NO_OF_EGT_SENSORS         1     // digital thermocouple sensors, MAX6675
#define NO_OF_SERVOS              3

#include "torque_sensor.h"
#include "PID.h"
#include "strings.h"
#include "screens.h"
#include "RPM_counter.h"
#include "servos.h"
#include "flags.h"
#include "eeprom.h"


//front panel status LED
#define PIN_LED 22
#define LED_ON LOW
#define LED_OFF HIGH


//datalogger SD card CS pin
#define PIN_LOG_SDCARD_CS         10

//rpm counter interrupt pin
#define PIN_RPM_COUNTER_INPUT     2 //INT0 -this clashes with the gameduino interupt pin


//calibration values for Lemark LMS184 1 bar MAP sensor
//specifying them as Long type to ensure correct evaluation
#define SENSOR_MAP_CAL_HIGH_LSb     39L    //analog sample value for the high cal point
#define SENSOR_MAP_CAL_LOW_LSb     969L    //analog sample value for the low cal point

#define SENSOR_MAP_CAL_HIGH_mbar  1050L    //absolute pressure at the high cal point
#define SENSOR_MAP_CAL_LOW_mbar    200L    //absolute pressure at the low cal point

#define SENSOR_MAP_CAL_MAX_LSb    1024L    //adc value for maximum possible pressure value
#define SENSOR_MAP_CAL_MIN_LSb       0L    //adc value for minimum possible pressure value

/* while using these points to map the analog input is technically correct
 *  we can get better performance if the input range is a power of two,
 *  (which it is, being 10bit adc)
 *  so we can rescale HIGH_mbar and LOW_mbar to fit the range
 */
// hopefully this will be evaluated at compile time! //yeah, this seems to be as fast as casting and writing an interger literal, and fractionally slower than a non cast literal
#define SENSOR_CAL_LIMIT_out(in_limit, in_low, in_high, out_low, out_high)   ( ( ((in_limit-in_low) * (out_high-out_low)) / (in_high-in_low)  ) + out_low + 0.5)
//lim=maxlsb=0, in=lim-inlo=-969, outrng=outhi-outlo=850, numr=in*outrng=-823650, inrng=inhi-inlo=-930, outfrac=numr/inrng=885, out=outfrac+outlo=1085
//lim=minlsb=1024, in=lim-inlo=55, outrng=outhi-outlo=850, numr=in*outrng=46750, inrng=inhi-inlo=-930, outfrac=numr/inrng=-50, out=outfrac+outlo=150
#define SENSOR_MAP_CAL_MAX_mbar   (int)SENSOR_CAL_LIMIT_out( (float)SENSOR_MAP_CAL_MAX_LSb, SENSOR_MAP_CAL_LOW_LSb, SENSOR_MAP_CAL_HIGH_LSb, SENSOR_MAP_CAL_LOW_mbar ,SENSOR_MAP_CAL_HIGH_mbar)
#define SENSOR_MAP_CAL_MIN_mbar   (int)SENSOR_CAL_LIMIT_out( (float)SENSOR_MAP_CAL_MIN_LSb, SENSOR_MAP_CAL_LOW_LSb, SENSOR_MAP_CAL_HIGH_LSb, SENSOR_MAP_CAL_LOW_mbar ,SENSOR_MAP_CAL_HIGH_mbar)
/* these should evaluate to 150 and 1086 respectively
 *  and now do after specifying the types of the parameter constants or at least close enoug
 *  they are 150 and 1085, so  some very minor rounding errors that can be disregarded
 *  
 *  Actually they are evaluating as 1085 and 150 for some bizare reason I cannot see
 *  I'm just going to swap MIN/MAX LSb and live with not knowing why
 *  
 *  Rounding fixed by specifiying float literals
 */



// digital thermocouple sensor configuration
#define PIN_SPI_EGT_1_CS          7
byte EGTSensors[NO_OF_EGT_SENSORS] = {PIN_SPI_EGT_1_CS};
  



// Task Schedule

// screen/log file update
#define UPDATE_INTERVAL_ms            500
#define UPDATE_START_ms               (UPDATE_INTERVAL_ms + 0)      //setting update loops to start slightly offset so they aren't tying to execute at the same time

// digital themocouple update rate
#define EGT_SAMPLE_INTERVAL_ms        250 //max update rate
#define EGT_SAMPLES_PER_UPDATE        (UPDATE_INTERVAL_ms/EGT_SAMPLE_INTERVAL_ms)
#define EGT_UPDATE_START_ms           110

//analog input read rate
#define ANALOG_SAMPLE_INTERVAL_ms     100   //record analog values this often
#define ANALOG_SAMPLES_PER_UPDATE     (UPDATE_INTERVAL_ms/ANALOG_SAMPLE_INTERVAL_ms)
#define ANALOG_UPDATE_START_ms        70

//rpm counter params
#define RPM_TO_MS(rpm)                (60000/(rpm))
#define MS_TO_RPM(ms)                 RPM_TO_MS(ms)   //same formula, included for clarity

#define RPM_MAX                       4500
#define RPM_MIN_TICK_INTERVAL_ms      RPM_TO_MS(RPM_MAX)
#define RPM_MAX_TICKS_PER_UPDATE      (UPDATE_INTERVAL_ms/RPM_MIN_TICK_INTERVAL_ms)
#define RPM_MIN                       1
#define RPM_MAX_TICK_INTERVAL_ms      RPM_TO_MS(RPM_MIN)


#define RPM_MIN_SET                   1500
#define RPM_MAX_SET                   4500
#define RPM_MIN_SET_ms                RPM_TO_MS(RPM_MIN_SET)
#define RPM_MAX_SET_ms                RPM_TO_MS(RPM_MAX_SET)

#define PID_UPDATE_INTERVAL_ms        50
#define PID_UPDATE_START_ms           40  
#define PID_LOOPS_PER_UPDATE          (UPDATE_INTERVAL_ms/PID_UPDATE_INTERVAL_ms)

#define SCREEN_REDRAW_INTERVAL_MIN_ms 100
#define TOUCH_READ_INTERVAL_ms        100
#define TOUCH_READ_START_ms           80  


/*          event         duration  
 *                      typ.      max.    log sdcard    log serial                                                      min gap to next, max if different
 *      U - update                                                      000                                     500     40    80ms till next spi use
 *      T - touch                                                           080     180     280     380     480         10
 *      P - pid                                                         040 090 140 190 240 290 340 390 440 490 540     10, 30
 *      E - egt                                                             110                 360                     10, 30
 *      A - analog                                                      070     170     270     370     470     570     10
 
 *      
 *      3.33ms per char
 *         010   030   050   070   090   110   130   150   170   190   210   230   250   270   290   310   330   350   370   390   410   430   450   470   490
 *      000   020   040   060   080   100   120   140   160   180   200   220   240   260   280   300   320   340   360   380   400   420   440   460   480   500
 *      U           P        A  T  P     E        P        A  T  P              P        A  T  P              P     E  A  T  P              P        A  T  P  U
 *                                    
 *      500   520   540   560   580   600   620   640   660   680   700   720   740   760   780   800   820   840   860   880   900   920   940   960   980  1000
 *      U           P        A  T  P     E        P        A  T  P              P        A  T  P              P     E  A  T  P              P        A  T  P  U
 * 
 */


#include "records.h"



/*PID references KNOB values to set Servo values,
 * so the number of knobs must be at least the number of servos,
 * even if there are no knobs to set the value
 */
#if NO_OF_USER_INPUTS < NO_OF_SERVOS
#define NO_OF_KNOBS NO_OF_SERVOS
#else
#define NO_OF_KNOBS NO_OF_USER_INPUTS
#endif
unsigned int KNOB_values[NO_OF_KNOBS];




static int update_timestamp = 0; //not much I can do easily about these timestamps
static int analog_timestamp = 0; //could be optimised by using a timer and a progmem table of func calls
static int egt_timestamp = 0;    //more optimal if all intervals have large common root
static int pid_timestamp = 0;    //they probably dont need to be long ints though.
static int display_timestamp = 0;//short ints can handle an interval of 32 seconds
static int touch_timestamp = 0;

                                 

/* some alternate, possibly faster mapping functions 
 *  map : 39673   - built in function
 *  imap :14914   - copy of map, using 16bit integers only. bit range of input and output limited to 16 total
 *  lmap : 38538 - copy of map, inputs and output are 16bit, cast to long where needed
 *  ib2map: 643   - copy of map, but input range is a power of 2. eliminates division, thus much faster
 *  ob2map: 39675 - copy of ib2map, but output range is power of 2. no benefit
 */
  
/* map(), but with all inputs and outputs as 16bit integers: (out_range * in_range) must be less than 32k */
int imap(int x, int in_min, int in_max, int out_min, int out_max)
{
  return ((x - in_min) * (out_max - out_min) / (in_max - in_min)) + out_min;
}
/* map(), but with all inputs and outputs as 16bit integers: (out_range * in_range) can exceed 32k */
int lmap( int x,  int in_min,  int in_max,  int out_min,  int out_max)
{
  return (int)((( long)(x - in_min) * (out_max - out_min)) / (in_max - in_min)) + out_min;
}
/* map(), but with long inputs SLOW, but needed for 24bit sensors */
long longmap(long x, long in_min, long in_max, long out_min, long out_max) 
{
  return (long)((long long)(x - in_min) * (out_max - out_min) / (in_max - in_min)) + out_min;
}


/* if input range is a power of two, this function is much faster */
int ib2map(int x, int in_min, byte in_range_log2, int out_min, int out_max)
{
  return ((int)(((long)(x - in_min) * (long)(out_max - out_min)) >> in_range_log2 )) + out_min;
}

/* for mapping ADC values, we can remove some extraneous parameters and add rounding */
int amap(int x, int out_min, int out_max)
{
  long result = ((long)x * (out_max - out_min)) + (1 << 9);
  return (int)(result >> 10) + out_min;
}

void test_map_implementations()
{
  Serial.println(F("Testing mmap:"));

  unsigned long tt = 0;
  unsigned int total = 0;
  for (byte in_max =4; in_max < 15; in_max += 2)
  {
    for (byte out_max =4; out_max < 15; out_max += 2)
    {
      for (byte in_val =0; in_val < in_max; in_val++)
      {
        unsigned int iv = (1<<in_val);//in_val;//
        unsigned int im = (1<<in_max);//in_max;//
        unsigned int om = (1<<out_max);//out_max;//
        unsigned int t;
        
        volatile unsigned int out;
        int n = 1000;

        t = micros();
        while( n--)  
        //{ out = ib2map(iv, 0, im, 0, om );}
        { out = lmap(iv, 0, im, 0, om );}
        //{out = SENSOR_MAP_CAL_MAX_mbar;} //760 val = 170 - both these values are wrong
        //{out = SENSOR_MAP_CAL_MIN_mbar;} //760   val = 220
        //{out = (const int)(1.0);}  //760
        //{out = (const int)(1);} //760
         //{out = 0;} //628
        t = micros()-t;
        
        tt += t;
        total++;

        iv = (1<<in_val);
        im = (1<<in_max);
        om = (1<<out_max);        
        Serial.print(F("dt, o, om, im, i: "));
        Serial.print(t);
        Serial.print(FS(S_COMMA));
        Serial.print(out);
        Serial.print(FS(S_COMMA));
        Serial.print(om);
        Serial.print(FS(S_COMMA));
        Serial.print(im);
        Serial.print(FS(S_COMMA));
        Serial.print(iv);
        Serial.println();
      }
    }
  }
  Serial.print(F("mean time: "));
  Serial.println(tt/total);

  // mmap :14914
  // map : 39673
  // ulmap : 38538
  // ib2map: 643
  // ob2map: 39675
}

void setup() {

  //configure front panel LED
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, LED_ON);
  

  //set datalogger SDCard CS pin high, 
  pinMode(PIN_LOG_SDCARD_CS, OUTPUT);
  digitalWrite(PIN_LOG_SDCARD_CS, HIGH);
  // set Gameduino CS pins high
  pinMode(9, OUTPUT);
  digitalWrite(9, HIGH);
  pinMode(8, OUTPUT);
  digitalWrite(8, HIGH);

  //set EGT CS pins high
  for(byte idx = 0; idx < NO_OF_EGT_SENSORS; idx++) { ConfigureMAX6675(EGTSensors[idx]); }
 
  //configure the RPM input
  configiure_rpm_counter();
  
  //initialise servos
  initialise_servos();
  

  //fetch the firmware fersion string

  
  //initialise the serial port:
  Serial.begin(1000000);  //for usb coms, no reason not to use fastest available baud rate - this turns out to be the biggest time usage during update/report
  Serial.println();

  MAKE_STRING(S_FIRMWARE_NAME);           //we use make here so we can pass the string to screen_splash
  Serial.println(S_FIRMWARE_NAME_str);
  
  DateTime t_compile;
  t_compile = CompileDateTime();
  stream_print_date_time(t_compile, Serial);
//  Serial.println();


  //initialise RTC
  i2c_init();

  if (!DS1307_isrunning())
  {
    Serial.println(F("RTC On"));//    Serial.println(F("Starting RTC"));
    DS1307_adjust(t_compile);
  }
  else
  {
    DateTime t_now = DS1307_now();
    #ifndef DEBUG_RTC_FORCE_UPDATE
    if (is_after(t_compile, t_now))
    #endif
    {
      Serial.println(F("Set RTC"));//Serial.println(F("Updating RTC"));
      DS1307_adjust(t_compile);
    }
    #ifndef DEBUG_RTC_FORCE_UPDATE
    else
    {
      //Serial.print(F("Time: "));//Serial.print(F("Time now: "));
      stream_print_date_time(t_now, Serial);
    }
    #endif
  }
  
  //set default flags
  flags_config.do_serial_write = true;
  flags_config.do_sdcard_write = true;

  

  //load eeprom, write defualts if a different version
  initialise_eeprom();

  //attempt to initialse the logging SD card. 
  //this will include creating a new file from the RTC date for immediate logging
  
  //Serial.print(FS(S_SDCARD));//Serial.print(F("Initializing SD card..."));
  STRING_BUFFER(S_CARD_FAILED_OR_NOT_PRESENT);//S_NO_SD_CARD);//
  
  if (!SD.begin(PIN_LOG_SDCARD_CS)) {
    GET_STRING(S_CARD_FAILED_OR_NOT_PRESENT);
    flags_status.sdcard_available = false;
    Serial.println(F("SD Card failed to init."));
  }
  else
  {
    GET_STRING(S_CARD_INITIALISED); //S_NO_SD_CARD);//
    flags_status.sdcard_available = true;
    Serial.println(F("SD Card init'ed succesfully."));
#ifdef DEBUG_SDCARD_TEST_FILE_OPEN
    File dataFile = SD.open(DEBUG_SDCARD_TEST_FILE_NAME, FILE_WRITE);
    if(dataFile)
    {
      //print the header
      dataFile.println(F("Test data"));
      dataFile.close();
      
      Serial.println(F("Test file opened OK"));
    }
    else
    {
      //open failed on first attempt, do not attempt to save to this file
      Serial.println(F("Test file open FAILED"));
    }
#endif
  }
//  Serial.println(string);
  screen_draw_splash(S_FIRMWARE_NAME_str, string);


  //start the ADC
  configureADC();
  configure_torque_sensor();


  //move all servos to minimum
  reset_servos();


  // test the map functions
//  test_map_implementations();



#ifdef DEBUG_MAP_CAL
// SENSOR_CAL_LIMIT_out(in_limit, in_low, in_high, out_low, out_high)   ( ( ((in_limit-in_low) * (out_high-out_low)) / (in_high-in_low)  ) + out_low + 0.5)
// SENSOR_MAP_CAL_MAX_mbar   (int)SENSOR_CAL_LIMIT_out( SENSOR_MAP_CAL_MAX_LSb, SENSOR_MAP_CAL_LOW_LSb, SENSOR_MAP_CAL_HIGH_LSb, SENSOR_MAP_CAL_LOW_mbar ,SENSOR_MAP_CAL_HIGH_mbar)
// SENSOR_MAP_CAL_MIN_mbar   (int)SENSOR_CAL_LIMIT_out( SENSOR_MAP_CAL_MIN_LSb, SENSOR_MAP_CAL_LOW_LSb, SENSOR_MAP_CAL_HIGH_LSb, SENSOR_MAP_CAL_LOW_mbar ,SENSOR_MAP_CAL_HIGH_mbar)

  Serial.print(F("map cal LOW LSb  : "));
  Serial.println((int)SENSOR_MAP_CAL_LOW_LSb);
  Serial.print(F("map cal HIGH LSb : "));
  Serial.println((int)SENSOR_MAP_CAL_HIGH_LSb);

  Serial.print(F("map cal LOW mbar : "));
  Serial.println((int)SENSOR_MAP_CAL_LOW_mbar);
  Serial.print(F("map cal HIGH mbar: "));
  Serial.println((int)SENSOR_MAP_CAL_HIGH_mbar);

  Serial.print(F("map cal MIN LSb  : "));
  Serial.println((int)SENSOR_MAP_CAL_MIN_LSb);
  Serial.print(F("map cal MAX LSb  : "));
  Serial.println((int)SENSOR_MAP_CAL_MAX_LSb);
  
  Serial.print(F("map cal MIN mbar : "));
  Serial.println((int)SENSOR_MAP_CAL_MIN_mbar);
  Serial.print(F("map cal MAX mbar : "));
  Serial.println((int)SENSOR_MAP_CAL_MAX_mbar);
  
  Serial.println();
#endif

  Serial.println(F("Init Complete"));
  // wait for MAX chip to stabilize, and to see the splash screen
  delay(4000);


#ifdef DEBUG_TOUCH_REG_DUMP
//302104h REG_TOUCH_MODE : 2
//302108h REG_TOUCH_ADC_MODE : 1
//30210Ch REG_TOUCH_CHARGE : 16
//30211Ch REG_TOUCH_RAW_XY :32
//302120h REG_TOUCH_RZ : 16
//302124h REG_TOUCH_SCREEN_XY :32
//302128h REG_TOUCH_TAG_XY :32
//30212Ch REG_TOUCH_TAG :8
//302168h REG_TOUCH_CONFIG :16        0x8381
//30218Ch REG_TOUCH_DIRECT_XY :32
//302190h REG_TOUCH_DIRECT_Z1Z2 :32

#define REG_TOUCH_CONFIG 0x302168

  GD.resume();
  GD.ClearColorRGB(96,192,256);
  GD.ClearTag(0x23);
  GD.Clear(1,1,1);
  GD.ColorRGB(256,192,96);
  GD.TagMask(1);
  GD.Tag(0x17);
  draw_box(0,0,200,300,0,0);
  GD.ClearTag(0x42);
  /* display the screen when done */
  GD.swap();
  
  while(1)
  {
    Serial.print(F("REG_TOUCH_MODE        0x")); Serial.println(GD.rd(REG_TOUCH_MODE),HEX);
    Serial.print(F("REG_TOUCH_ADC_MODE    0x")); Serial.println(GD.rd(REG_TOUCH_ADC_MODE),HEX);
    Serial.print(F("REG_TOUCH_CHARGE      0x")); Serial.println(GD.rd16(REG_TOUCH_CHARGE),HEX);
    Serial.print(F("REG_TOUCH_RAW_XY      0x")); Serial.println(GD.rd32(REG_TOUCH_RAW_XY),HEX);
    Serial.print(F("REG_TOUCH_RZ          0x")); Serial.println(GD.rd16(REG_TOUCH_RZ),HEX);
    Serial.print(F("REG_TOUCH_SCREEN_XY   0x")); Serial.println(GD.rd32(REG_TOUCH_SCREEN_XY),HEX);
    Serial.print(F("REG_TOUCH_TAG_XY      0x")); Serial.println(GD.rd32(REG_TOUCH_TAG_XY),HEX);
    Serial.print(F("REG_TOUCH_TAG         0x")); Serial.println(GD.rd(REG_TOUCH_TAG),HEX);
    Serial.print(F("REG_TOUCH_CONFIG      0x")); Serial.println(GD.rd16(REG_TOUCH_CONFIG),HEX);
    Serial.print(F("REG_TOUCH_DIRECT_XY   0x")); Serial.println(GD.rd32(REG_TOUCH_DIRECT_XY),HEX);
    Serial.print(F("REG_TOUCH_DIRECT_Z1Z2 0x")); Serial.println(GD.rd32(REG_TOUCH_DIRECT_Z1Z2),HEX);
    Serial.print(F("REG_TAG               0x")); Serial.println(GD.rd(REG_TAG),HEX);
    Serial.println();

    Serial.println();
      delay(1000);
  }
#endif

  
  int timenow = millis();

  //set the initial times for the loop events
  update_timestamp = timenow + UPDATE_START_ms;
  analog_timestamp = timenow + ANALOG_UPDATE_START_ms;
  egt_timestamp = timenow + EGT_UPDATE_START_ms;
  pid_timestamp = timenow + PID_UPDATE_START_ms;
  touch_timestamp = timenow + TOUCH_READ_START_ms;

  
}





void process_digital_inputs()
{
  #ifdef DEBUG_DISABLE_DIGITAL
  return;
  #endif
  #ifdef DEBUG_DIGITAL_TIME
  unsigned int timestamp_us = micros();
  #endif

  //collect egt data only if there are available sample slots
  if (CURRENT_RECORD.EGT_no_of_samples < EGT_SAMPLES_PER_UPDATE)
  {
    byte inc_samples = 0;
    for(int idx = 0; idx < NO_OF_EGT_SENSORS; idx++)
    {
      if(MAX6675_ReadTemp(EGTSensors[idx]))
      {
        CURRENT_RECORD.EGT[idx][CURRENT_RECORD.EGT_no_of_samples] = MAX6675_GetData();
        inc_samples = 1;
      }
    }
    CURRENT_RECORD.EGT_no_of_samples += inc_samples;
  }
  #ifdef DEBUG_DIGITAL_TIME
  timestamp_us = micros() - timestamp_us;
  Serial.print(F("t_dig us: "));
  Serial.println(timestamp_us);
  #endif

    
}






static byte update_step = 0;

bool process_update_loop()
{ 
  #ifdef DEBUG_UPDATE_TIME
  unsigned int timestamp_us = micros();
  Serial.print(F("upd_stp: "));
  Serial.println(update_step);
  #endif
  switch(update_step)
  {
    //todo: make writing to sd card a seperate process
    case 0:   
      reset_record();
      finalise_record();
      update_step -= draw_screen();       //  10032us (basic) 9548us (config) 9528 (pid rpm, in current state)
      update_step++;
      break;
    
    case 1:   // 160us (disabled) approx 30ms when active. see function for breakdown
      while(!write_sdcard_data_record()); //sd card write must be completed before giving up the spi bus
      update_step++;
      break;
    
    case 2:   // 160us (disabled) see function for breakdown when active
      if (write_serial_data_record()) //serial writes can be split up safely
      { update_step = 0;  }
      break; 
    
    default:  update_step = 0; break;
  }
  
  #ifdef DEBUG_UPDATE_TIME
  timestamp_us = micros() - timestamp_us;
  Serial.print(F("t_upd us: "));
  Serial.println(timestamp_us);
  #endif

  return (update_step != 0);
}

void loop() {

  int elapsed_time;
  int timenow = millis();

  #ifdef DEBUG_SDCARD_TEST_FILE_OPEN
  static unsigned int sd_count = 0;
  File dataFile = SD.open(DEBUG_SDCARD_TEST_FILE_NAME, FILE_WRITE);
  if(dataFile)
  {
    //print the header
    //dataFile.println(F("Test data"));
    dataFile.close();
    
    Serial.print(F("SD OK   ")); Serial.println(sd_count);
  }
  else
  {
    //open failed on first attempt, do not attempt to save to this file
    Serial.print(F("SD FAIL ")); Serial.println(sd_count);
  }
  sd_count++;
  timenow = millis();
  #endif  

  /* get time left till next update */
  elapsed_time = update_timestamp - timenow;
  /* execute update if update interval has elapsed */
  if(elapsed_time < 0)
  {
    /* set the timestamp for the next update */
    update_timestamp = timenow + UPDATE_INTERVAL_ms + (elapsed_time%UPDATE_INTERVAL_ms);
    flags_status.update_active = true;
    /* emit the timestamp interval of the current update, if debugging */
    #ifdef DEBUG_LOOP
    Serial.print(F("\nUPD: ")); Serial.println(timenow);
    #endif
  }
  if(flags_status.update_active)
  {
    flags_status.update_active = process_update_loop();
    
    //update the time
    timenow = millis();
  }

  /* get time left till next update */
  elapsed_time = egt_timestamp - timenow;
  /* execute update if update interval has elapsed */
  if(elapsed_time < 0)
  {
    /* set the timestamp for the next update */
    egt_timestamp = timenow + EGT_SAMPLE_INTERVAL_ms + (elapsed_time%EGT_SAMPLE_INTERVAL_ms);

    // debug marker
    #ifdef DEBUG_LOOP
    Serial.print(F("\nDIG: ")); Serial.println(timenow);
    #endif
    
    process_digital_inputs();
    
    //update the time
    timenow = millis();
  }
  
  /* get time left till next update */
  elapsed_time = analog_timestamp - timenow;
  /* execute update if update interval has elapsed */
  if(elapsed_time < 0)
  {
    /* set the timestamp for the next update */
    analog_timestamp = timenow + ANALOG_SAMPLE_INTERVAL_ms + (elapsed_time%ANALOG_SAMPLE_INTERVAL_ms);

    //debug marker
    #ifdef DEBUG_LOOP
    Serial.print(F("\nANA: "));Serial.println(timenow);
    #endif
    
    process_analog_inputs();

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
    #ifdef DEBUG_LOOP
    Serial.print(F("\nPID: ")); Serial.println(timenow);
    #endif  

    process_pid_loop();

    //update the time
    timenow = millis();
  }


  
  /* get time left till next update */
  elapsed_time = touch_timestamp - timenow;
  /* execute update if update interval has elapsed */
  if(elapsed_time < 0)
  {
    /* set the timestamp for the next update */
    int timelag = elapsed_time%TOUCH_READ_INTERVAL_ms;
    touch_timestamp = timenow + TOUCH_READ_INTERVAL_ms + timelag;

    //debug marker
    #ifdef DEBUG_LOOP
    Serial.print(F("\nTOUCH: ")); Serial.println(timenow);
    #endif  

    read_touch();

    //update the time
    timenow = millis();
  }

  /* redraw the screen if a touch occured and an update is not in progress */
  if(!flags_status.update_active && flags_status.redraw_pending)
  {
    draw_screen();
  }


  check_eeprom_update();

  #ifdef DEBUG_LOOP
  //Serial.print('.');
  #endif
}
