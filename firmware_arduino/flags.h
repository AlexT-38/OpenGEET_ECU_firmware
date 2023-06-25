#ifndef __FLAGS_H__
#define __FLAGS_H__


/* configuration and status flags as bitfeilds */

//these macros arent used
#define GET_FLAG(reg, flag)  ((reg&(1<<flag)) != 0)
#define SET_FLAG(reg, flag)  (reg |= (1<<flag))
#define CLR_FLAG(reg, flag)  (reg &= ~(1<<flag))
#define TOG_FLAG(reg, flag)  (reg ^= (1<<flag))

#define GET_FLAG_C(flag)  GET_FLAG(flags_config,flag)
#define SET_FLAG_C(flag)  SET_FLAG(flags_config,flag)
#define CLR_FLAG_C(flag)  CLR_FLAG(flags_config,flag)
#define TOG_FLAG_C(flag)  TOG_FLAG(flags_config,flag)

#define GET_FLAG_S(flag)  GET_FLAG(flags_status,flag)
#define SET_FLAG_S(flag)  SET_FLAG(flags_status,flag)
#define CLR_FLAG_S(flag)  CLR_FLAG(flags_status,flag)
#define TOG_FLAG_S(flag)  TOG_FLAG(flags_status,flag)

//#define FLAG_...       1

//alternatively, use bitfields
//some of these flags need to be stored in EEPROM, some do not
//values must be exlicitly saved to eeprom to avoid burnout
typedef struct {
    byte do_serial_write:1;
    byte do_serial_write_hex:1;
    byte do_sdcard_write:1;
    byte do_sdcard_write_hex:1;
}FLAGS_CONFIG;

typedef union flags_config_union{
  FLAGS_CONFIG flags;
  byte data;
}FLAGS_CONFIG_UNION;

//these flags indicate the state of the system
typedef struct {
    byte update_eeprom:1;
    byte sdcard_available:1;
    byte file_openable:1;
    byte logging_requested:1;
    byte logging_state:2;
    byte redraw_pending:1;
    byte update_active:1;
    byte engine_running:1;    //true if rpm is above minimum
    byte engine_starting:1;   //true when motor is being commanded to to turn the engine over
    byte reactor_running:1;   //once we have a foolproof way of checking the functioning of the reactor, it will be stored here. this is essentially for running reactor startup process
    byte reactor_fault:1;     //true if any reactor parameter is out of spec
    byte generator_running:1; //true when motor is being set for regenerative braking
    byte generator_fault:1;    //true if anypart of the generation system is not working, eg battery full, unbalanced, overloaded
    byte hold_direct_input:1;  //if true, user inputs will be collected, but not passed to the servos
    
}FLAGS_STATUS;

//these are the various modes of operation we might want to use
typedef enum sys_mode {
  MODE_DIRECT,            //each servo is control by a single analog input
  MODE_PID_RPM_CARB,      //regulate rpm by controlling a single throttle servo, set rpm from a single analog input, regen braking set from A3
  MODE_PID_RPM_GEET_1,    //as above plus control vacuum with second servo
  MODE_PID_RPM_GEET_2,    //as above plus control ratio by exhaust temperature
  MODE_PID_POW_CARB,      //regulate power output through regen braking and rpm control using some scheme to be determined, power demand set by a single analog input
  MODE_PID_POW_GEET,      //as above, but with GEET PID instead of CARB PID
  NO_OF_SYS_MODES
}SYS_MODE;

FLAGS_CONFIG flags_config;
FLAGS_STATUS flags_status;
SYS_MODE     sys_mode;

#endif //__FLAGS_H__
