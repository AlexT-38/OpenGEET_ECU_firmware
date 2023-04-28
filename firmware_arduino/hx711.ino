#define HX711_A128  1
#define HX711_B32  2
#define HX711_A64  3

byte HX711_mode = HX711_A128;

void HX711_configure()
{
  pinMode(HX711_CLK, OUTPUT);
  pinMode(HX711_DATA, INPUT_PULLUP);
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
