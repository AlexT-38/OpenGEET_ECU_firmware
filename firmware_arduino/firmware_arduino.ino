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

//both the follwing libraries use the SPI interface, 
// and will need to be tested for cross compatibility
#include <SD.h>         //sd card library
#include <SPI.h>        //needed for gameduino
#include <GD2.h>        //Gameduino (FT810 plus micro sdcard
#include <GyverMAX6675_SPI.h>    //digital thermocouple interface using SPI port

#include <Servo.h>      //servo control disables analog write on pins 9 and 10

// sketching out some constants for IO, with future upgradability in mind

// control input pins
#define PIN_CONTROL_INPUT_1       A0
#define PIN_CONTROL_INPUT_2       A1

#define CONTROL_INPUT_1_NAME      "Reactor Inlet Valve Target"
#define CONTROL_INPUT_2_NAME      "Engine Inlet Valve Target"

// sensor types - probably ought to be an enum
#define ST_ANALOG                 0
#define ST_I2C                    1
#define ST_SPI                    2

// pressure sensor configuration
#define NO_OF_MAP_SENSORS         1

// pressure sensor 1
#define SENSOR_TYPE_MAP_1         ST_ANALOG
#define PIN_INPUT_MAP_1           A2
#define SENSOR_MAP_1_NAME         "Reactor Fuel Inlet Pressure"

// temperature sensor configuration
#define NO_OF_EGT_SENSORS         1

#define MAX6675                   1

// temperature sensor
#if NO_OF_EGT_SENSORS > 0
  #define SENSOR_TYPE_EGT_1         ST_SPI
  #define PIN_SPI_EGT_1_CS          7
  #define SENSOR_MODEL_EGT_1        MAX6675
  #define SENSOR_EGT_1_NAME         "Reactor Exhaust Inlet Temperature"

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
  #define SERVO_1_NAME              "Reactor Inlet Valve Servo"
#endif

// servo 2
#if NO_OF_SERVOS > 1
  #define PIN_SERVO_2               5
  #define SERVO_2_MAX               2300
  #define SERVO_2_MIN               750
  #define SERVO_2_NAME              "Engine Inlet Valve Servo"
#endif

// servo 3
#if NO_OF_SERVOS > 2
  #define PIN_SERVO_2               6
  #define SERVO_2_MAX               2300
  #define SERVO_2_MIN               750
  #define SERVO_2_NAME              "Starter/Generator Motor Control"
#endif

// rpm counter configuration

#define PIN_SENSOR_RPM              3     //ideally should be a timer capture pin





void setup() {

  //force CS pins high, attempting to debug FT810 and MAX6675 interop
  pinMode(PIN_SPI_EGT_1_CS, OUTPUT);
  digitalWrite(PIN_SPI_EGT_1_CS, HIGH);

  pinMode(10, OUTPUT);
  digitalWrite(10, HIGH);
  
  // put your setup code here, to run once:
  Serial.begin(115200);

  Serial.println("MAX6675 and GameduinoIII test");

  GD.begin(0);
  
  // wait for MAX chip to stabilize
  delay(500);
}

void loop() {
  static byte EGT_err = 0;
  
  if(EGTSensor1.readTemp())
  {
    Serial.print("EGT1: ");
    Serial.print(EGTSensor1.getTemp());
    Serial.println("°C");
    EGT_err &= ~(1);
  }
  else if ( !(EGT_err & (1)) )
  {
    Serial.println("EGT1 disconnected");
    EGT_err |= (1);
  }
     
/*
  byte cs_max = digitalRead(7);
  byte cs_ft8 = digitalRead(8);
  byte cs_sdm = digitalRead(9);
  byte cs_sd = digitalRead(10);
  byte clk = digitalRead(13);
  byte miso = digitalRead(12);
  byte mosi = digitalRead(11);

  Serial.print("cs_max:");
  Serial.println(cs_max);
  Serial.print("cs_ft8:");
  Serial.println(cs_ft8);
  Serial.print("cs_sdm:");
  Serial.println(cs_sdm);
  Serial.print("cs_sd:");
  Serial.println(cs_sd);

  Serial.print("clk:");
  Serial.println(clk);
  Serial.print("miso:");
  Serial.println(miso);
  Serial.print("mosi:");
  Serial.println(mosi);
    */
  delay(500);
  
  static int colour = 0xFF8000;
  long int timenow = millis()/1000;
  GD.ClearColorRGB(colour);
  GD.Clear();

  colour = ~colour;
  //GD.cmd_text(GD.w / 2, GD.h / 2, 31, OPT_CENTER, "Hello world");
  GD.cmd_number(GD.w / 2, GD.h / 2, 31, OPT_CENTER, timenow);
  GD.swap();
  
  /* stop comms to FT810, so other devices (ie MAX6675) can use the bus */
  GD.__end();

  delay(500);
}
