#ifndef __COMMANDS_H__
#define __COMMANDS_H__

//would be better to use gcode, no?
typedef enum
{
  //basic commands
  C_STOP_ALL =0,            //S set pwm to 0 and disable the LFO output and pwm oscillator
  C_SET_RATE,               //R set LFO rate, disable/enable LFO when no parameter given
  C_SET_PWM,                //P set PWM ratio, restore value after force_pwm if no new value given
  C_FORCE_PWM,              //F set PWM ratio ignore limits
  C_FORCE_PWM_W,            //G set PWM using 16 bit number
  C_SET_PRESCALE,           //C set clock rate for PWM
  C_SET_PWM_LIMIT,          //L set minimum (-ve) and maximum (+ve) pwm values
  C_SET_PWM_INVERT,         //I set PWM inversion bit0:in (255-pwm) bit1:out (hi/lo)
  C_SET_PWM_RAMP,           //V
  C_SET_PWM_BITS,          //B reduce PWM bits from 8 to 7 above PWM=192
  C_OSCILLATE,              //O send LFO to pwm
  C_USE_LUT,                //T set transfer function/look up table (direct, boost x0.1)
  C_SAVE_EEP,               //W
  C_LOAD_EEP,               //E
  C_RESET_CONFIG,           //X
  C_PRINT_CONFIG,           //Z
  C_PRINT_STATE,            //A
  C_RUN_TEST,               //Q sweep through a range of pwm values and frequencies, measure some voltages
  C_CALIBRATE_TEST,          //K test the electrical set up for the above test
  C_MEASURE_TEST,            //M take a single sample with current settings and report
  C_HELP,                   //H prints available commands
  NO_OF_COMMANDS
}COMMAND;

#endif //__COMMANDS_H__
