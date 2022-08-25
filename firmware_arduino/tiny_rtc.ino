/*
 * lightweight DS1307 lib that doesnt consume ram
 * uses SoftI2CMaster - a lightweight, zero ram consumption I2C library
 * 
 * DS1307 runs at 100kHz, which means each byte takes 0.1ms
 * a single write is 0.3ms
 * a single read is 0.4ms
 */
 
#define DS1307_ADDRESS        0x68                  //7bit address
#define DS1307_WRITE          (DS1307_ADDRESS<<1)   //8bit address
#define DS1307_READ           (DS1307_WRITE|1)

static uint8_t t_bcd2bin (uint8_t val) { return val - 6 * (val >> 4); }
static uint8_t t_bin2bcd (uint8_t val) { return val + 6 * (val / 10); }

/* print the date and time to the serial port */
void serial_print_date_time(const DateTime now) 
{
    Serial.print(2000+now.year, DEC);
    Serial.print('/');
    Serial.print(now.month, DEC);
    Serial.print('/');
    Serial.print(now.day, DEC);
    Serial.print(' ');
    Serial.print(now.hour, DEC);
    Serial.print(':');
    Serial.print(now.minute, DEC);
    Serial.print(':');
    Serial.print(now.second, DEC);
    Serial.println();
}
void date_to_string(DateTime now, char *string)
{
  string += 10;
  *string-- = '\0';
  *string-- = '0' + now.day%10;
  *string-- = '0' + now.day/10;
  *string-- = ':';
  *string-- = '0' + now.month%10;
  *string-- = '0' + now.month/10;
  *string-- = ':';
  *string-- = '0' + now.year%10;
  *string-- = '0' + now.year/10;
  *string-- = '0';
  *string-- = '2';
}
void time_to_string(const DateTime now, char *string)
{
  string += 8;
  *string-- = '\0';
  *string-- = '0' + now.second%10;
  *string-- = '0' + now.second/10;
  *string-- = '/';
  *string-- = '0' + now.minute%10;
  *string-- = '0' + now.minute/10;
  *string-- = '/';
  *string-- = '0' + now.hour%10;
  *string-- = '0' + now.hour/10;
}

#define NVREAD(addr)        DS1307_read(8 + addr)   //nvram starts at address 8
#define NVWRITE(addr, val)  DS1307_write(8 + addr, val)
/* read a string of registers */
uint8_t DS1307_read_bytes(const uint8_t addr, uint8_t *dst, const uint8_t bytes) 
{
  uint8_t count = 0;
  if (i2c_start(DS1307_WRITE))
  {
    i2c_write(addr);
    if(i2c_rep_start(DS1307_READ))
    {
      while(++count < bytes) *dst++ = i2c_read(false);
      *dst++ = i2c_read(true);
    }
  }
  i2c_stop();
  return count;
}
/* read the next register - approx 0.3ms */
uint8_t DS1307_read_next() 
{
  unsigned char value = 0;
  if(i2c_rep_start(DS1307_READ))
  {
    value = i2c_read(true);
  }
  i2c_stop();
  return value;
}
/* read a specific register - approx 0.4ms */
uint8_t DS1307_read(const uint8_t addr) 
{
  unsigned char value = 0;
  if (i2c_start(DS1307_WRITE))
  {
    i2c_write(addr);
    if(i2c_rep_start(DS1307_READ))
    {
      value = i2c_read(true);
    }
  }
  i2c_stop();
  return value;
}
/* write a specific register - approx 0.3ms */
void DS1307_write(const uint8_t addr, const uint8_t val) 
{
  if (i2c_start(DS1307_WRITE))
  {
    i2c_write(addr);
    i2c_write(val);
  }
  i2c_stop();
}

uint8_t DS1307_isrunning(void) 
{
  uint8_t ss = DS1307_read(0);
  return !(ss>>7);
}

void DS1307_adjust(const DateTime dt) 
{
  if (i2c_start(DS1307_WRITE))
  {
    i2c_write(0);
    i2c_write(t_bin2bcd(dt.second));
    i2c_write(t_bin2bcd(dt.minute));
    i2c_write(t_bin2bcd(dt.hour));
    i2c_write(t_bin2bcd(0));
    i2c_write(t_bin2bcd(dt.day));
    i2c_write(t_bin2bcd(dt.month));
    i2c_write(t_bin2bcd(dt.year));
    i2c_write(0);
    i2c_stop();
  }
}

DateTime DS1307_now() {
  DateTime dt = {0};
  if (i2c_start(DS1307_WRITE))
  {
    i2c_write(0);
    if(i2c_rep_start(DS1307_READ))
    {
      dt.second = t_bcd2bin(i2c_read(false) & 0x7F);
      dt.minute = t_bcd2bin(i2c_read(false));
      dt.hour   = t_bcd2bin(i2c_read(false));
      i2c_read(false);
      dt.day    = t_bcd2bin(i2c_read(false));
      dt.month  = t_bcd2bin(i2c_read(false));
      dt.year   = t_bcd2bin(i2c_read(true));  
      
    }
  }
  i2c_stop();
  return dt;
}

/* incase only date or time are required */
DateTime DS1307_date() {
  DateTime dt = {0};
  if (i2c_start(DS1307_WRITE))
  {
    i2c_write(4);
    if(i2c_rep_start(DS1307_READ))
    {
      dt.day    = t_bcd2bin(i2c_read(false));
      dt.month  = t_bcd2bin(i2c_read(false));
      dt.year   = t_bcd2bin(i2c_read(true));  
      
    }
  }
  i2c_stop();
  return dt;
}

DateTime DS1307_time() {
  DateTime dt = {0};
  if (i2c_start(DS1307_WRITE))
  {
    i2c_write(0);
    if(i2c_rep_start(DS1307_READ))
    {
      dt.second = t_bcd2bin(i2c_read(false) & 0x7F);
      dt.minute = t_bcd2bin(i2c_read(false));
      dt.hour   = t_bcd2bin(i2c_read(false));
    }
  }
  i2c_stop();
  return dt;
}

/* convert the next two characters at the pointer into a decimal integer */
static uint8_t conv2d(const char* p) {
  uint8_t v = 0;
  if ('0' <= *p && *p <= '9')
    v = *p - '0';
  return 10 * v + *++p - '0';
}

/* return a DateTime struct loaded from the compiler date and time macros */
DateTime CompileDateTime()
{
  DateTime dt;
  //(__DATE__, __TIME__)

  MAKE_STRING(DATE);
  //                offset 0123456789A           01234567
  // sample input: date = "Dec 26 2009", time = "12:34:56"
  dt.year = conv2d(DATE_str + 9);
  dt.day  = conv2d(DATE_str + 4);
  
  // Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec 
  switch (DATE_str[0]) {
    case 'J': dt.month = DATE_str[1] == 'a' ? 1 : dt.month = DATE_str[2] == 'n' ? 6 : 7; break;
    case 'F': dt.month = 2; break;
    case 'A': dt.month = DATE_str[2] == 'r' ? 4 : 8; break;
    case 'M': dt.month = DATE_str[2] == 'r' ? 3 : 5; break;
    case 'S': dt.month = 9; break;
    case 'O': dt.month = 10; break;
    case 'N': dt.month = 11; break;
    case 'D': dt.month = 12; break;
    default: // "dd mm yyyy" format
      dt.year  = conv2d(DATE_str + 8);
      dt.day   = conv2d(DATE_str);
      dt.month = conv2d(DATE_str + 3);
      break;
  }
  MAKE_STRING(TIME);
  dt.hour = conv2d(TIME_str);
  dt.minute = conv2d(TIME_str + 3);
  dt.second = conv2d(TIME_str + 6);

}
