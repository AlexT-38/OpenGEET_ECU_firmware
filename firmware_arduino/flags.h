#ifndef __FLAGS_H__
#define __FLAGS_H__


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

#endif //__FLAGS_H__
