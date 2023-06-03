
#define MAX6675_SPI_SPEED 4000000

static SPISettings MAX6675_SETTINGS(MAX6675_SPI_SPEED, MSBFIRST, SPI_MODE0);

unsigned int MAX6675_data;
void ConfigureMAX6675(byte CS_Pin)
{
  pinMode(CS_Pin, OUTPUT);
  digitalWrite(CS_Pin, HIGH);
}

bool MAX6675_ReadTemp(byte CS_Pin)
{
  digitalWrite(CS_Pin, LOW);
  SPI.beginTransaction(MAX6675_SETTINGS);
  MAX6675_data = SPI.transfer16(0);
  SPI.endTransaction();
  digitalWrite(CS_Pin, HIGH);

  if ((MAX6675_data == 0xFFFF) || (MAX6675_data & 0x8)) return false;
  MAX6675_data >>= 3;
  return true;
}
unsigned int MAX6675_GetData()
{
  return MAX6675_data;
}
