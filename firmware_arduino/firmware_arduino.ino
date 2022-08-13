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
#include <max6675.h>    //digital thermocouple interface

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
  #define PIN_SPI_EGT_1_CS          9      //abitrary, change as needed
  #define SENSOR_MODEL_EGT_1        MAX6675
  #define SENSOR_EGT_1_NAME         "Reactor Exhaust Inlet Temperature"
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
  // put your setup code here, to run once:

}

void loop() {
  // put your main code here, to run repeatedly:

}
