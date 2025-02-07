/* K-type thermocouple ADC module
 *  12-bit, 0.25 °C resolution
 *  8-bit accuracy from 0 to 700 °C
 */
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

  #ifdef DEBUG_MAX6675
  Serial.print(F("MAX6675: 0x")); Serial.print(MAX6675_data,HEX);
  #endif

  if ((MAX6675_data == 0xFFFF) || (MAX6675_data & _BV(2))) return false;
  MAX6675_data >>= 3;
  return true;
}
unsigned int MAX6675_GetData()
{
  return MAX6675_data;
}
