/*
 * basic readout screen
 */

/* draw an integer value and a label from progmem */
void draw_readout_int(const int pos_x, const int pos_y, int opts, const int value, const char *label_pgm)
{
  MAKE_STRING(label_pgm);
  GD.cmd_text(pos_x, pos_y+20, 28, opts, label_pgm_str);
  GD.cmd_number(pos_x, pos_y-8, 31, opts, value);
}

/* draw an integer coded decimal value, with a given number decimal places, and label */
void draw_readout_decimal(const int pos_x, const int pos_y, int opts, const int value, const byte dps, const char *label_pgm)
{
}

#define MAX_SIG_FIGS  4
/* draw a fixed point integer value, with the given number of fractional bits and significant digits */
void draw_readout_fixed(const int pos_x, const int pos_y, int opts, const int value, const byte frac_bits, byte extra_sf, const char *label_pgm)
{
  MAKE_STRING(LIST_SEPARATOR);
  
  MAKE_STRING(label_pgm);
  GD.cmd_text(pos_x, pos_y+20, 28, opts, label_pgm_str);

  char value_str[18];
  char *str_ptr = &value_str[18];

  unsigned int base = (1<<frac_bits);          //numeric base for the fractional part
  int whole = labs(value) >> frac_bits;             //whole part
  unsigned int frac = labs(value) & (base-1); //fractional part

  if (extra_sf > MAX_SIG_FIGS) extra_sf = MAX_SIG_FIGS; //clamp maximum sig figs

  //look up tables
  const unsigned long int pow10[] = { 1, 10, 100, 1000, 10000, 100000};
  const byte dps[] = { 0, 1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5 };

  //total number of decimal places to draw
  byte total_dps = dps[frac_bits] + extra_sf;
  //corresponding power factor
  unsigned long int factor = pow10[dps[frac_bits]] * pow10[extra_sf];
  // decimal integer representation of the fractional part, with rounding
  unsigned long int decimal = ((frac * factor * 2)+base) / (base*2);

  //dispense the digits into the string buffer
  *str_ptr-- = '\0';
  
  while(total_dps-- > 0 && str_ptr > value_str)
  {
    *str_ptr-- = DEC_CHAR(decimal);
    decimal /= 10;
  }
  if(str_ptr <= value_str)
  {
    Serial.println(F("ERROR: ran out of digits parsing fixed point decimal part"));
    return;
  }
  *str_ptr-- = '.';
  if(whole == 0) *str_ptr-- = '0'; 
  while(whole > 0 && str_ptr > value_str)
  {
    *str_ptr-- = DEC_CHAR(whole);
    whole /= 10;
  }
  if(str_ptr <= value_str)
  {
    Serial.println(F("ERROR: ran out of digits parsing fixed point integer part"));
    return;
  }
  if(value < 0)
  {
    *str_ptr = '-';
  }
  else
  {
    *str_ptr = ' ';
  }
  byte font_size = 31;
  if(strlen(str_ptr) > 8) font_size = 30;
  if(strlen(str_ptr) > 12) font_size = 29;
  if(strlen(str_ptr) > 14) font_size = 28;
  if(strlen(str_ptr) > 16) font_size = 27;
  
  GD.cmd_text(pos_x, pos_y-8, font_size, opts, str_ptr);
}


void draw_basic()
{
  int colour_bg = RGB(0x00,0x20,0x40);

  int pos_y;
  int pos_x;

  /* data record to read */
  DATA_RECORD *data_record = &Data_Record[1-Data_Record_write_idx];
 
  GD.ClearColorRGB(colour_bg);
  GD.Clear();

  pos_x = GD.w - 16;
  pos_y = 32;
  draw_readout_fixed(pos_x, pos_y, OPT_RIGHTX | OPT_CENTERY, data_record->EGT_avg, 2,1, EGT1_DEGC);

  pos_y += 64;
  draw_readout_int(pos_x, pos_y, OPT_RIGHTX | OPT_CENTERY, data_record->RPM_avg, RPM);
  
  pos_y += 64;

  pos_y += 64;
  {
    DateTime t_now = DS1307_now();
    char datetime_string[11];
    date_to_string(t_now, datetime_string);
    GD.cmd_text(pos_x,  pos_y+12, 26, OPT_RIGHTX | OPT_CENTERY, datetime_string);
    time_to_string(t_now, datetime_string);
    GD.cmd_text(pos_x,  pos_y-12, 26, OPT_RIGHTX | OPT_CENTERY, datetime_string);
  }

  pos_x = 16;
  pos_y = 32;
  draw_readout_int(pos_x, pos_y, OPT_CENTERY, data_record->A0_avg, A0_VALUE);
  
  pos_y += 64;
  draw_readout_int(pos_x, pos_y, OPT_CENTERY, data_record->A1_avg, A1_VALUE);
  
  pos_y += 64;
  draw_readout_int(pos_x, pos_y, OPT_CENTERY, data_record->A2_avg, A2_VALUE);
  
  pos_y += 64;
  draw_readout_int(pos_x, pos_y, OPT_CENTERY, data_record->A3_avg, A3_VALUE);
  

}
