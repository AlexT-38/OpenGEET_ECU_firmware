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
typedef struct {
    byte do_serial_write:1;
    byte do_serial_write_hex:1;
    byte do_sdcard_write:1;
    byte do_sdcard_write_hex:1;
}FLAGS_CONFIG;

typedef struct {
    byte sd_card_available:1;
    byte logging_active:1;
    byte redraw_pending:1;
    byte update_active:1;
    byte engine_running:1;    //true if rpm is above minimum
    byte engine_starting:1;   //true when motor is being commanded to to turn the engine over
    byte reactor_running:1;   //once we have a foolproof way of checking the functioning of the reactor, it will be stored here
    byte reactor_fault:1;     //true if any reactor parameter is out of spec
    byte generator_running:1; //true when motor is being set for regenerative braking
    byte generator_fault:1;    //true if anypart of the generation system is not working, eg battery full, unbalanced, overloaded
    
}FLAGS_STATUS;

FLAGS_CONFIG flags_config;
FLAGS_STATUS flags_status;

#endif //__FLAGS_H__
