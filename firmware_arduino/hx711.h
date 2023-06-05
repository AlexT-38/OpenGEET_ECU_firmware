#ifndef __HX711_H__
#define __HX711_H__

#define HX711_16BITS

//#if TARGET == UNO
#define HX711_CLK  9
#define HX711_DATA 4
// see "hardware/arduino/variants/standard/pins_arduino.h"

#define HX711_CLK_port PORTB
#define HX711_DATA_port PORTD
#define HX711_DATA_pin PIND
#define HX711_CLK_bit 1
#define HX711_DATA_bit 4
#define HX711_CLK_ddr DDRB
#define HX711_DATA_ddr DDRD
/*
#elif TARGET == MEGA
#define HX711_CLK  9
#define HX711_DATA 4

#define HX711_CLK_port PORTB
#define HX711_DATA_pin PINB
#define HX711_CLK_bit 6
#define HX711_DATA_bit 5

#endif
*/

//generic bit access macros
#define setBIT(port, bit_) port|=_BV(bit_)
#define clrBIT(port, bit_) port&=~_BV(bit_)
#define togBIT(port, bit_) port^=_BV(bit_)

#define HX711_setCLK()  setBIT(HX711_CLK_port,HX711_CLK_bit)
#define HX711_clrCLK()  clrBIT(HX711_CLK_port,HX711_CLK_bit)

#define HX711_getDATA() ((HX711_DATA_pin&_BV(HX711_DATA_bit))!=0)



#endif //__HX711_H__
