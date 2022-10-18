/* screen resolution is WXVGA, 480,272 
 *   so we can use Vertex2II exclusively 
 */


typedef void(*SCREEN_DRAW_FUNC)(void);


/* pointer to the func to use to draw the screen */
const SCREEN_DRAW_FUNC draw_screen_funcs[NO_OF_SCREENS] = {screen_draw_basic, NULL, NULL, NULL, screen_draw_config};
const char * const screen_labels[NO_OF_SCREENS] PROGMEM = {S_BASIC, S_NONE,S_NONE,S_NONE, S_CONFIG};


SCREEN_EN current_screen = SCREEN_1;


#define SCREEN_BUTTON_FONT 26
#define SCREEN_BUTTON_XN  6

/* touch input statics */

//static byte touch_tag;
#ifdef DEBUG_TOUCH_INPUT
//static xy touch_coord;
#endif

#ifdef TAG_BYPASS
/* for some reason I cant get tags to read out correctly. 
 * since I'm using a grid based approach
 * it should be relatively easy to convert xy coords into grid coords 
 * and have a look up table for tags vs coords
 * or some nested logic
 */
typedef byte(*SCREEN_TAG_FUNC)(void);
const SCREEN_TAG_FUNC screen_tag_funcs[NO_OF_SCREENS] = {screen_basic_tags, NULL, NULL, NULL, screen_config_tags};

byte fetch_tag()
{
  // fist off, check if the touch is in the screen selector area 
  if (GD.inputs.xytouch.x < GRID_SX(SCREEN_BUTTON_XN))
  {
    // find the index by dividing the y position by the vertical grid size
    // and adding to the base screen tag 
    return TAG_SCREEN_1 + (GD.inputs.xytouch.y / GRID_SY(NO_OF_SCREENS));
  }
  else
  {
    // let screens figure things out for them selves
    return screen_tag_funcs[current_screen]();
    
  }
}

#endif



/* opens comms to display chip and calls the current screen's draw function */

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
      /* common screen components */
      draw_screen_background();
      draw_screen_selector();
      /* draw the current screen */
      draw_screen_funcs[current_screen]();

      #ifdef DEBUG_TOUCH_INPUT
      if(GD.inputs.touching)
      {
        GD.TagMask(0);
        GD.ColorRGB(C_MARKER);
        GD.Begin(POINTS);
        GD.PointSize(512);
        GD.Vertex2ii(GD.inputs.xytouch.x,GD.inputs.xytouch.y,0,0);

        GD.TagMask(1);
      }
      #endif
      
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


void read_touch()
{
  byte touch_event = TOUCH_NONE;
  
  #ifdef DEBUG_TOUCH_TIME
  unsigned int timestamp_us = micros();
  #endif


  // copy old tag
  byte touch_tag_old = GD.inputs.tag;

  //fetch coords
#if defined(DEBUG_TOUCH_INPUT) || defined(TAG_BYPASS)
  
  
  
  long int val = GD.rd32(REG_TOUCH_SCREEN_XY);
  xy *coord = (xy*) &val;
  GD.inputs.xytouch.x = coord->y;
  GD.inputs.xytouch.y = coord->x - 0x400;
  
#else
//  xy touch_coord;
  GD.inputs.xytouch.x = GD.rd16(REG_TOUCH_SCREEN_XY);
#endif

  //check if a touch is registered
  if(GD.inputs.xytouch.x != 0x8000)
  {
    GD.inputs.touching = true;

    #ifdef TAG_BYPASS
    //use our own function for fetching the tag
    GD.inputs.tag = fetch_tag();
    #else
    // get the tag, and optionally get the tracker tag and value
    GD.inputs.tag = GD.rd(REG_TAG);
    #endif
    
#ifdef TRACKERS_ENABLED
#endif
  }
  else
  {
    // clear the track tag and touch
    GD.inputs.touching = false;
    GD.inputs.tag = TAG_NONE;
    
#ifdef TRACKERS_ENABLED
#endif    
  }
  
  
  //check for change in tag
  if (touch_tag_old != GD.inputs.tag)
  {
    flags_status.redraw_pending = true;
    
    //detirmine the event type - for simplicities' sake, we have only on and off events, maybe cancel also
    if(touch_tag_old == 0)  {touch_event = TOUCH_ON; touch_tag_old = GD.inputs.tag;}
    else if(GD.inputs.tag == 0) {touch_event = TOUCH_OFF;}

    switch(touch_tag_old)
    {
      case TAG_SCREEN_1:
        if(touch_event == TOUCH_OFF)  current_screen = SCREEN_1;
        break;
//      case TAG_SCREEN_2:
//        if(touch_event == TOUCH_OFF)  current_screen = SCREEN_2;
//        break;
//      case TAG_SCREEN_3:
//        if(touch_event == TOUCH_OFF)  current_screen = SCREEN_3;
//        break;
//      case TAG_SCREEN_4:
//        if(touch_event == TOUCH_OFF)  current_screen = SCREEN_4;
//        break;
      case TAG_SCREEN_5:
        if(touch_event == TOUCH_OFF)  current_screen = SCREEN_5;
        break;
      case TAG_LOG_TOGGLE:
        if(touch_event == TOUCH_OFF)
        {
          //if not previously logging and an sd card is available, generate the filename for logging
          flags_status.logging_active = ~flags_status.logging_active;
          if(flags_status.sdcard_available && flags_config.do_sdcard_write && flags_status.logging_active) generate_file_name();
        }
        break;
//      case TAG_CAL_SV0:
//        //calibrate_servo(0);
//        break;
//      case TAG_CAL_SV1:
//        //calibrate_servo(1);
//        break;
//      case TAG_CAL_SV2:
//        //calibrate_servo(2);
//        break;
      case TAG_ENGINE_START:
        break;
      case TAG_ENGINE_STOP:
        break;
      default:
//        flags_status.redraw_pending = true;
        break;
    }
  }

  #ifdef DEBUG_TOUCH_TIME
  timestamp_us = micros() - timestamp_us;
  Serial.print(F("t_touch us: "));
  Serial.println(timestamp_us);
  #endif

  #ifdef DEBUG_TOUCH_INPUT
  Serial.print(F("Tag: "));
  Serial.print(GD.inputs.tag);
  Serial.print(F("; XY: "));
  Serial.print(GD.inputs.xytouch.x);
  MAKE_STRING(S_COMMA);
  Serial.print(S_COMMA_str);
  Serial.print(GD.inputs.xytouch.y);
  Serial.println();
  if(GD.inputs.touching) flags_status.redraw_pending = true;
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
  GD.Vertex2ii(px+BORDER,py+BORDER,0,0);
  GD.Vertex2ii(sx-BORDER,sy-BORDER,0,0);
}

/* screen selection */

/* datetime */
void draw_datetime(int pos_x, int pos_y, unsigned int optx)
{
  unsigned int opts = OPT_CENTERY | optx;
  
  GD.ColorA(A_BKG_WINDOW);
  GD.ColorRGB(C_BKG_NORMAL);
  draw_box(pos_x, pos_y, GRID_SX(4), GRID_SY(4), BOX_WIDTH, opts);
  
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
  draw_box(pos_x, pos_y, GRID_SX(4), GRID_SY(4), BOX_WIDTH, opts);
  
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
  draw_box(pos_x, pos_y, GRID_SX(4), GRID_SY(4), BOX_WIDTH, opts);

  
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

byte is_touching_inside(int x, int y, byte sx, byte sy)
{
  return (GD.inputs.xytouch.x > x && GD.inputs.xytouch.x < (x+sx) && GD.inputs.xytouch.y > y && GD.inputs.xytouch.y < (y+sy));
}

void draw_log_toggle_button(int x, int y, byte sx, byte sy)
{
  GD.Tag(TAG_LOG_TOGGLE);

  int opt = (GD.inputs.tag == TAG_LOG_TOGGLE) * OPT_FLAT;

  if(flags_status.logging_active)
  {
    GD.cmd_fgcolor(C_BTN_LOGGING);
  }
  else
  {
    GD.cmd_fgcolor(C_BUTTON_FG);
  }
  MAKE_STRING(S_REC);
  GD.cmd_button(x+BORDER,y+BORDER,sx-(BORDER<<1),sy-(BORDER<<1), 26, opt, S_REC_str);

  x+=sx>>1;
  y+=sy>>1;
  sy>>=2;
  
  char string[14] = {0};
  if(flags_config.do_serial_write)
  {
    byte i = strlen_P(S_SERIAL);
    GET_STRING(S_SERIAL);
    string[i++]=' ';
    if(flags_config.do_serial_write_hex)
    {
      READ_STRING(S_0X,&string[i]);
    }
    else
    {
      READ_STRING(S_TXT,&string[i]);
    }
    
    GD.cmd_text(x, y-sy,20,OPT_CENTER,string);
  }

  if(flags_config.do_sdcard_write)
  {
    if(!flags_status.sdcard_available)
    {
      GET_STRING(S_NO_SD_CARD);
      GD.cmd_text(x, y+sy,20,OPT_CENTER,string);
    }
    else if(flags_status.file_openable)
    {
      GD.cmd_text(x, y+sy,20,OPT_CENTER,output_filename);
    }
    else
    {
      GET_STRING(S_NO_FILE_OPEN);
      GD.cmd_text(x, y+sy,20,OPT_CENTER,string);
    }
  }
  GD.Tag(TAG_INVALID);
  
}
void draw_screen_selector()
{
  int colour_bg = C_BKG_NORMAL; 
  byte gx = GRID_XL(0,SCREEN_BUTTON_XN);

  char label[10];
  int opt;
  GD.cmd_fgcolor(C_BUTTON_FG);
  for(char n = 0; n<NO_OF_SCREENS; n++)
  {
    GD.Tag(TAG_SCREEN_1 + n);                                                                         //set the touch tag
    opt = ( (current_screen == (SCREEN_1 + n)) || (GD.inputs.tag == (TAG_SCREEN_1 + n)) ) * OPT_FLAT;     //draw flat if currently selected, or button is touched
    READ_STRING_FROM(screen_labels, n, label);
    GD.cmd_button(GRID_XL(0,SCREEN_BUTTON_XN)+BORDER, GRID_YT(n,NO_OF_SCREENS)+BORDER, GRID_SX(SCREEN_BUTTON_XN)-(BORDER<<1), GRID_SY(NO_OF_SCREENS)-(BORDER<<1), SCREEN_BUTTON_FONT, opt, label);
  }
  GD.Tag(TAG_INVALID);
}

void draw_screen_background()
{
    /* data record to read */
  DATA_RECORD *data_record = &Data_Record[1-Data_Record_write_idx];

  int colour_bg = C_BKG_NORMAL;
  /* simple background colour setting by rpm range
     this should be changed to read from system status flags */
  if (data_record->RPM_avg == 0)
  {    colour_bg = C_BKG_STOPPED;  }
  else if (data_record->RPM_avg < 1300 || data_record->RPM_avg > 3800)
  {    colour_bg = C_BKG_WARNING;  }
  else
  {    colour_bg = C_BKG_RUNNING;  }

  GD.ClearColorRGB(colour_bg);
  GD.Clear();
}
