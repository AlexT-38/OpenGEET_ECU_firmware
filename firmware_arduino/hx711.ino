/*in an effort to save some flash, read the data as an in and forget the last TORQUE_PRESCALE bits */

#define HX711_A128  (1+TORQUE_PRESCALE)
#define HX711_B32  (2+TORQUE_PRESCALE)
#define HX711_A64  (3+TORQUE_PRESCALE)


byte HX711_mode = HX711_A128;

void HX711_configure()
{
  //pinMode(HX711_CLK, OUTPUT);
  //pinMode(HX711_DATA, INPUT_PULLUP);
  setBIT(HX711_CLK_ddr, HX711_CLK_bit);
  setBIT(HX711_DATA_port, HX711_CLK_bit);
  HX711_clrCLK();
}


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
  return value;
}
