/*in an effort to save some flash, read the data as an in and forget the last TORQUE_PRESCALE bits */
#ifdef SQUEESE_HX711
#define HX711_A128  1
#define HX711_B32  2
#define HX711_A64  3
#else
#define HX711_A128  (1+TORQUE_PRESCALE)
#define HX711_B32  (2+TORQUE_PRESCALE)
#define HX711_A64  (3+TORQUE_PRESCALE)
#endif

byte HX711_mode = HX711_A128;

void HX711_configure()
{
  //pinMode(HX711_CLK, OUTPUT);
  //pinMode(HX711_DATA, INPUT_PULLUP);
  setBIT(HX711_CLK_ddr, HX711_CLK_bit);
  setBIT(HX711_DATA_port, HX711_CLK_bit);
  HX711_clrCLK();
}





#ifdef SQUEESE_HX711
/* this doesnt work reliably, and has no real impact on flash size */
int HX711_read_value()
{
  //read directly as an int
  byte count = 24-TORQUE_PRESCALE-1;
  unsigned int value = 0;
  //collect data
  //get MSB
  HX711_setCLK();
  HX711_clrCLK();
  byte msb = HX711_getDATA();
  while(count--)
  {
    HX711_setCLK();
    HX711_clrCLK();
    value <<= 1;
    value |= HX711_getDATA(); //data;
  }
  count = HX711_mode;
  //trigger next sample
  while(count--)
  {
    HX711_setCLK();
    HX711_clrCLK();
  }
  /* we are taking the middle 16 bits of a 24 bit signed int
   * how do we account for the negative?
   * 24 bit signed range 
   * 0x800000 - 0xFFFFFF, 0x000000 - 0x7FFFFF
   * 16 bit signed range
   * 0x8000 - 0xFFFF, 0x0000 - 0x7FFF
   * our range
   * 0x0000 - 0xFFFF, 0x0000 - 0x7FFF
   * we need the MSb to determine sign, replace MSb of read value
   * our corrected range
   * 0x8000 - 0xFFFF, 0x0000 
   */
  //if(value > (unsigned int)(0x7FFFFF>>TORQUE_PRESCALE) value |= (unsigned int)(0xFF000000>>TORQUE_PRESCALE);
  value |= msb<<16;

#else
long HX711_read_value()
{
  byte count = 24;
  long int value = 0;
  //collect data
  while(count--)
  {
    HX711_setCLK();
    HX711_clrCLK();
    value <<= 1;
    value |= HX711_getDATA(); //data;
  }
  count = HX711_mode;
  //trigger next sample
  while(count--)
  {
    HX711_setCLK();
    HX711_clrCLK();
  }
  //convert 24 bit signed to 32 bit signed
  if(value > 0x7FFFFF) value |= 0xFF000000;
#endif
  return value;
}
