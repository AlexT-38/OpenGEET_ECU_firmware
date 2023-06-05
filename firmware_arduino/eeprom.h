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
 
#define EEP_VERSION 7
struct eeprom
{
    byte                touch_calibrated; //is set to 0x7C when calibrated. we can force recalibration by setting this byte to anything else
    long int            touch_transform[6];  //GD library uses the start of eeprom to store touch calibration.
    byte                eep_version;
    byte                eep_crc;
    SV_CAL              servo_cal[NO_OF_SERVOS];
    FLAGS_CONFIG        flags_config;
    PID_K               pid_k_rpm, pid_k_vac;
    TORQUE_CAL          torque_cal;
};
/* offsetof doesn not accept array indices, so getting the offset for an array member
 *  requires manually adding the index x the member size.
 *  however, sizeof cannot give the size of a member of a struct, so we must 
 *  fake a struct at a void address to get the member sizes 
 */
#define EEP_SIZEOF(member)        sizeof(((struct eeprom *)0)->member)
#define EEP_SIZEOF_ARR(member)    sizeof(((struct eeprom *)0)->member[0])
#define EEP_ADDR(member)          offsetof(struct eeprom, member)
#define EEP_GET(member,dst)       EEPROM.get(EEP_ADDR(member), dst);
#define EEP_GET_N(member,n,dst)   EEPROM.get(EEP_ADDR(member)+(EEP_SIZEOF_ARR(member)*n), dst);
#define EEP_PUT(member,src)       EEPROM.put(EEP_ADDR(member), src);
#define EEP_PUT_N(member,n,src)   EEPROM.put(EEP_ADDR(member)+(EEP_SIZEOF_ARR(member)*n), src);



#endif //__EEPROM_H_
