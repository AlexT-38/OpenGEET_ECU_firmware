#ifndef __EEPROM_H__
#define __EEPROM_H__

/* as there is no way of incrementing a macro using the preprocessesor,
 * an altternative method of assigning eeprom addresses is required
 * my best guess just now is to define a struct and use some built in 
 * function to get the offsets
 * indeed, the following concludes with the same:
 * https://forum.arduino.cc/t/solved-how-to-structure-data-in-eeprom-variable-address/196538
 * the function is __builtin_offsetof also defined as offsetof (type, member | .member | [expr])
 */
 
#define EEP_VERSION 4
#define EEP_DEFAULT_CRC 0x5A



typedef struct configuration
{
  byte pwm_prescale;    //TCCR1B 0:3
  byte pwm;             //programmed target value
  byte pwm_osc;         //alternate value for oscilation
  byte pwm_min;         //minimum setable value, will jump to 0 below this if 'negate' is set
  byte pwm_max;         //maximum setable value, will jump to 255 above this if  'negate is NOT set
  unsigned int pwm_ramp; //number of PWM cycles between inc/decrement of the current pwm input value
  byte pwm_bits;    
  byte pwm_invert:1;    //inverts the polarity presented on the output pin 
  byte pwm_negate:1;    //negates the pwm value (255-pwm) prior to writing to OCR1A so that a pwm value of 0 can be achieved
  byte oscillate:1;     //indicates that the LFO should set the target pwm, switching between config.pwm and config.pwm_osc
  byte enable:1;        //enables LFO state output
  byte lut:1;           //enables look up table on pwm values prior to negation and writing to OCR1A
  
  long int interval;    //half cycle time interval for LFO
  
  byte padding[11];     //round up from 13 to 24 bytes
}CONFIG;

struct eeprom
{
    byte          eep_version;
    byte          eep_crc;
    CONFIG        config;
    byte          eep_cal_crc;
    CALIBRATION   calibration;
};

/* offsetof doesn not accept array indices, so getting the offset for an array member
 *  requires manually adding the index x the member size.
 *  however, sizeof cannot give the size of a member of a struct, so we must 
 *  fake a struct at a void address to get the member sizes 
 */
#define EEP_SIZEOF(member)        sizeof(((struct eeprom *)0)->member)
#define EEP_SIZEOF_ARR(member)    sizeof(((struct eeprom *)0)->member[0])
#define EEP_ADDR(member)          offsetof(struct eeprom, member)
#define EEP_GET(member,dst)       EEPROM.get(EEP_ADDR(member), dst)
#define EEP_GET_N(member,n,dst)   EEPROM.get(EEP_ADDR(member)+(EEP_SIZEOF_ARR(member)*n), dst)
#define EEP_PUT(member,src)       EEPROM.put(EEP_ADDR(member), src)
#define EEP_PUT_N(member,n,src)   EEPROM.put(EEP_ADDR(member)+(EEP_SIZEOF_ARR(member)*n), src)



#endif //__EEPROM_H_
