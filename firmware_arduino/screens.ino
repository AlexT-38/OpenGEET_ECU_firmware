/* screen resolution is WXVGA, 480,272 
 *   so we can use Vertex2II exclusively 
 */


typedef void(*SCREEN_DRAW_FUNC)(void);

/* pointer to the func to use to draw the screen */
SCREEN_DRAW_FUNC draw_screen_funcs[NO_OF_SCREENS] = {screen_draw_basic, NULL, NULL, NULL, screen_draw_config};

SCREEN_EN current_screen = SCREEN_1;


/* opens comms to display chip and calls the current screen's draw function */
static byte draw_step = 0;

byte draw_screen()
{
  flags_status.redraw_pending = true;
  
  // check if enough time has passed since the last redraw
  int time_now = millis();
  if ((time_now - display_timestamp) > 0)
  {
    display_timestamp = time_now + SCREEN_REDRAW_INTERVAL_MIN_ms;
    flags_status.redraw_pending = false;
    
    if (draw_screen_funcs[current_screen] != NULL)
    {
      #ifdef DEBUG_DISPLAY_TIME
      unsigned int timestamp_us = micros();
      #endif
    
      /* restart comms with the FT810 */
      GD.resume();
      /* draw the current screen */
      draw_screen_funcs[current_screen]();
      /* display the screen when done */
      GD.swap();
      /* stop comms to FT810, so other devices (ie MAX6675, SD CARD) can use the bus */
      GD.__end();
  
      #ifdef DEBUG_DISPLAY_TIME
      timestamp_us = micros() - timestamp_us;
      Serial.print(F("t_dsp us: "));
      Serial.println(timestamp_us);
      #endif
    }

  }
  return flags_status.redraw_pending;
}

/* touch handling - avoid heavy lifting in this funtion */
static byte touch_tag = TAG_NONE;
static byte touch_event = TOUCH_NONE;
void read_touch()
{

  #ifdef DEBUG_TOUCH_TIME
  unsigned int timestamp_us = micros();
  #endif
  
  // fetch the current tag
  byte tag = GD.rd(REG_TOUCH_TAG);

  //check for change in tag
  if (tag != touch_tag)
  {
    //detirmine the event type - for simplicities' sake, we have only on and off events, maybe cancel also
    if(touch_tag == 0) touch_event = TOUCH_ON;
    else if(tag == 0) touch_event = TOUCH_OFF;
    touch_tag = tag;

    bool redraw = true;
    
    switch(tag)
    {
      case TAG_SCREEN_1:
        if(touch_event == TOUCH_OFF)  current_screen = SCREEN_1;
        break;
      case TAG_SCREEN_2:
        if(touch_event == TOUCH_OFF)  current_screen = SCREEN_2;
        break;
      case TAG_SCREEN_3:
        if(touch_event == TOUCH_OFF)  current_screen = SCREEN_3;
        break;
      case TAG_SCREEN_4:
        if(touch_event == TOUCH_OFF)  current_screen = SCREEN_4;
        break;
      case TAG_SCREEN_5:
        if(touch_event == TOUCH_OFF)  current_screen = SCREEN_5;
        break;
      case TAG_LOG_START:
        //if not previously logging and an sd card is available, generate the filename for logging
        if(!flags_status.logging_active && flags_status.sd_card_available) generate_file_name();
        flags_status.logging_active = true;
        break;
      case TAG_LOG_STOP:
        flags_status.logging_active = false;
        break;
      case TAG_CAL_SV0:
        //calibrate_servo(0);
        break;
      case TAG_CAL_SV1:
        //calibrate_servo(1);
        break;
      case TAG_CAL_SV2:
        //calibrate_servo(2);
        break;
      case TAG_ENGINE_START:
        break;
      case TAG_ENGINE_STOP:
        break;
      default:
        redraw = false;
    }
  }

  #ifdef DEBUG_TOUCH_TIME
  timestamp_us = micros() - timestamp_us;
  Serial.print(F("t_dsp us: "));
  Serial.println(timestamp_us);
  #endif
}

/* drawing utility functions */

void draw_box(int px, int py, int sx, int sy, char w, int opt)
{
  
  if(opt&OPT_CENTERX)
  {    px -= sx/2;  }
  else if(opt&OPT_RIGHTX)
  {    px -= sx;    }
  if(opt&OPT_CENTERY)
  {    py -= sy/2;  }
  
  sx = px+sx;
  sy = py+sy;

  if(w>1)
  {
    char wx = (w+1)/2;
    
    if(px>sx) wx = -wx;
    px += wx;
    sx -= wx;

    w = w/2;
    if(py>sy) w = -w;
    py += w;
    sy -= w;
  }
  else
  {
    w=1; //minimum linewidth
  }
  GD.LineWidth(PTSP(w));
  GD.Begin(RECTS);
  GD.Vertex2ii(px,py,0,0);
  GD.Vertex2ii(sx,sy,0,0);
}

/* screen selection */

/* datetime */
void draw_datetime(int pos_x, int pos_y, unsigned int optx)
{
  unsigned int opts = OPT_CENTERY | optx;
  
  GD.ColorA(A_BKG_WINDOW);
  GD.ColorRGB(C_BKG_NORMAL);
  draw_box(pos_x, pos_y, GRID_SX(4)-BORDER, GRID_SY(4)-BORDER, BOX_WIDTH, opts);
  
  DateTime t_now = DS1307_now();
  char datetime_string[11];
  
  date_to_string(t_now, datetime_string);
  GD.ColorA(A_OPAQUE);
  GD.ColorRGB(C_LABEL);
  GD.cmd_text(pos_x-12,  pos_y+10, 26, opts, datetime_string);
  time_to_string(t_now, datetime_string);
  GD.cmd_text(pos_x-12,  pos_y-18, 28,  opts, datetime_string);
  
  GD.cmd_number(pos_x-12,  pos_y+22, 20,  opts, SCREEN_BUILD_ID);
}
/* draw an integer value and a label from progmem */
void draw_readout_int(const int pos_x, const int pos_y, int opts, const int value, const char *label_pgm)
{
  char dx = BORDER, dy =BORDER;
  if(opts&OPT_RIGHTX)
  {    dx = -dx;  }
  else if(opts&OPT_CENTERX)
  {    dx = 0;   }
  if(opts&OPT_CENTERY)
  {    dy = 0;  }
  GD.ColorA(A_BKG_WINDOW);
  GD.ColorRGB(C_BKG_NORMAL);
  draw_box(pos_x, pos_y, GRID_SX(4)-BORDER, GRID_SY(4)-BORDER, BOX_WIDTH, opts);
  
  MAKE_STRING(label_pgm);
  GD.ColorA(A_OPAQUE);
  GD.ColorRGB(C_LABEL);
  GD.cmd_text(pos_x+dx, pos_y+20, 28, opts, label_pgm_str);
  GD.ColorRGB(C_VALUE);
  GD.cmd_number(pos_x+dx, pos_y-8, 31, opts, value);
}

/* draw an integer coded decimal value, with a given number decimal places, and label */
void draw_readout_decimal(const int pos_x, const int pos_y, int opts, const int value, const byte dps, const char *label_pgm)
{
  GD.ColorA(A_BKG_WINDOW);
  GD.ColorRGB(C_BKG_NORMAL);
  //draw_box(pos_x, pos_y, 120,60,4,true);
}

#define MAX_SIG_FIGS  4
/* draw a fixed point integer value, with the given number of fractional bits and significant digits */
void draw_readout_fixed(const int pos_x, const int pos_y, int opts, const int value, const byte frac_bits, byte extra_sf, const char *label_pgm)
{
  char dx = BORDER, dy =BORDER;
  if(opts&OPT_RIGHTX)
  {    dx = -dx;  }
  else if(opts&OPT_CENTERX)
  {    dx = 0;   }
  if(opts&OPT_CENTERY)
  {    dy = 0;  }
  
  GD.ColorA(A_BKG_WINDOW);
  GD.ColorRGB(C_BKG_NORMAL);
  draw_box(pos_x, pos_y, GRID_SX(4)-BORDER, GRID_SY(4)-BORDER, BOX_WIDTH, opts);

  
  MAKE_STRING(S_COMMA);
  MAKE_STRING(label_pgm);

  GD.ColorA(A_OPAQUE);
  GD.ColorRGB(C_LABEL);
  GD.cmd_text(pos_x+dx, pos_y+20, 28, opts, label_pgm_str);

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

  GD.ColorRGB(C_VALUE);
  GD.cmd_text(pos_x+dx, pos_y-8, font_size, opts, str_ptr);
}


void draw_screen_selector(byte width)
{
  
}
