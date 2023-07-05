#ifndef __HX711_H__
#define __HX711_H__

#define HX711_16BITS


#define HX711_CLK  9
#define HX711_DATA 4

#define HX711_CLK_bit 6
#define HX711_DATA_bit 5
#define HX711_CLK_port PORTH
#define HX711_DATA_port PORTG
#define HX711_DATA_pin PING
#define HX711_CLK_ddr DDRH
#define HX711_DATA_ddr DDRG


//generic bit access macros
#define setBIT(port, bit_) port|=_BV(bit_)
#define clrBIT(port, bit_) port&=~_BV(bit_)
#define togBIT(port, bit_) port^=_BV(bit_)

#define HX711_setCLK()  setBIT(HX711_CLK_port,HX711_CLK_bit)
#define HX711_clrCLK()  clrBIT(HX711_CLK_port,HX711_CLK_bit)

#define HX711_getDATA() ((HX711_DATA_pin&_BV(HX711_DATA_bit))!=0)



#endif //__HX711_H__
