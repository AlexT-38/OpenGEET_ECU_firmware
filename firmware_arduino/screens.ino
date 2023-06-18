/* screen resolution is WXVGA, 480,272 
 *   so we can use Vertex2II exclusively 
 */
 
//#define DEBUG_SCREENS_DRAW_SLIDER_HORZ
//#define DEBUG_SLIDER_VALUE_HORZ
#define SLIDER_HEIGHT 8

//#define DEBUG_SCREENS_DRAW_SLIDER_VERT
//#define DEBUG_SLIDER_VALUE_VERT
#define SLIDER_WIDTH 8

typedef void(*SCREEN_DRAW_FUNC)(void);


/* pointer to the func to use to draw the screen */
const SCREEN_DRAW_FUNC draw_screen_funcs[NO_OF_SCREENS] = {screen_draw_basic, screen_draw_pid_rpm, NULL, screen_draw_torque, screen_draw_config};
const char * const screen_labels[NO_OF_SCREENS] PROGMEM = {S_BASIC, S_PID_RPM, S_NONE, S_TORQUE, S_CONFIG};


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
const SCREEN_TAG_FUNC screen_tag_funcs[NO_OF_SCREENS] = {screen_basic_tags, screen_pid_rpm_tags, NULL, NULL, screen_config_tags};

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

void draw_screen()
{
  //flags_status.redraw_pending = true;
  
  // check if enough time has passed since the last redraw
  //int time_now = millis();
  //if ((time_now - display_timestamp) > 0)
  {
    //display_timestamp = time_now + SCREEN_REDRAW_INTERVAL_MIN_ms;
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
  return;// flags_status.redraw_pending;
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
  GD.resume();
  long int val = GD.rd32(REG_TOUCH_SCREEN_XY);
  xy *coord = (xy*) &val;
  GD.inputs.xytouch.x = coord->y;
  GD.inputs.xytouch.y = coord->x;


  //check if a touch is registered
  if(GD.inputs.xytouch.x != 0x8000)
  {
    GD.inputs.touching = true;

    #ifdef TAG_BYPASS
    //use our own function for fetching the tag
    GD.inputs.tag = fetch_tag();
    #else
    // get the tag, and optionally get the tracker tag and value
    GD.inputs.tag = GD.rd(REG_TOUCH_TAG);
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

  //finish with the SPI bus
  GD.__end();
  
  //check for event (change in tag)
  if (touch_tag_old != GD.inputs.tag)
  {
    //redraw on event
    flags_status.redraw_pending = true;
    
    //detirmine the event type - for simplicities' sake, we have only on and off events,
    if(touch_tag_old == TAG_NONE)  {touch_event = TOUCH_ON; touch_tag_old = GD.inputs.tag;}
    else if(GD.inputs.tag == TAG_NONE) {touch_event = TOUCH_OFF;}
    else {touch_event = TOUCH_CANCEL; GD.inputs.tag = TAG_INVALID;} //if this is a cancel, set the tag to invalid
  }

  //process tags when touching or event
  if(GD.inputs.touching || touch_event != TOUCH_NONE)
  {
    switch(touch_tag_old)
    {
      case TAG_SCREEN_1:
        if(touch_event == TOUCH_OFF && draw_screen_funcs[SCREEN_1] != NULL)  current_screen = SCREEN_1;
        break;
      case TAG_SCREEN_2:
        if(touch_event == TOUCH_OFF && draw_screen_funcs[SCREEN_2] != NULL)  current_screen = SCREEN_2;
        break;
      case TAG_SCREEN_3:
        if(touch_event == TOUCH_OFF && draw_screen_funcs[SCREEN_3] != NULL)  current_screen = SCREEN_3;
        break;
      case TAG_SCREEN_4:
        if(touch_event == TOUCH_OFF && draw_screen_funcs[SCREEN_4] != NULL)  current_screen = SCREEN_4;
        break;
      case TAG_SCREEN_5:
        if(touch_event == TOUCH_OFF && draw_screen_funcs[SCREEN_5] != NULL)  current_screen = SCREEN_5;
        break;
      case TAG_LOG_TOGGLE:
        if(touch_event == TOUCH_OFF)
        {
          //if not previously logging and an sd card is available, generate the filename for logging
          flags_status.logging_active = ~flags_status.logging_active;
          if(flags_status.sdcard_available && flags_config.do_sdcard_write && flags_status.logging_active) generate_file_name();
        }
        break;
      case TAG_CAL_SV0_MIN:
        if(touch_event == TOUCH_ON || touch_event == TOUCH_NONE)
        {
          set_servo_min(0);
          flags_status.redraw_pending = true;
        }
        break;
      case TAG_CAL_SV1_MIN:
        if(touch_event == TOUCH_ON || touch_event == TOUCH_NONE)
        {
          set_servo_min(1);
          flags_status.redraw_pending = true;
        }
        break;
      case TAG_CAL_SV2_MIN:
        if(touch_event != TOUCH_OFF)
        {
          set_servo_min(2);
          flags_status.redraw_pending = true;
        }
        break;
      case TAG_CAL_SV0_MAX:
        if(touch_event != TOUCH_OFF)
        {
          set_servo_max(0);
          flags_status.redraw_pending = true;
        }
        break;
      case TAG_CAL_SV1_MAX:
        if(touch_event != TOUCH_OFF)
        {
          set_servo_max(1);
          flags_status.redraw_pending = true;
        }
        break;
      case TAG_CAL_SV2_MAX:
        if(touch_event != TOUCH_OFF)
        {
          set_servo_max(2);
          flags_status.redraw_pending = true;
        }
        break;
      case TAG_EEPROM_SAVE:
        if(touch_event == TOUCH_OFF)
        {
          save_eeprom();
        }
        break;
      case TAG_EEPROM_LOAD:
        if(touch_event == TOUCH_OFF)
        {
          load_eeprom();
        }
        break;
      case TAG_EEPROM_EXPORT:
        if(touch_event == TOUCH_OFF)
        {
          export_eeprom(&Serial);
        }
        break;
//      case TAG_EEPROM_IMPORT:
//        if(touch_event == TOUCH_OFF)
//        {
//          import_eeprom(&Serial);
//        }
//        break;
      case TAG_LOG_TOGGLE_SDCARD:
        if(touch_event == TOUCH_ON && !flags_status.logging_active) { flags_config.do_sdcard_write ^=1; }
        break;
      case TAG_LOG_TOGGLE_SDCARD_HEX:
        if(touch_event == TOUCH_ON && !(flags_status.logging_active&&flags_config.do_sdcard_write)) { flags_config.do_sdcard_write_hex ^=1; }
        break;
      case TAG_LOG_TOGGLE_SERIAL:
        if(touch_event == TOUCH_ON && !flags_status.logging_active) { flags_config.do_serial_write ^=1; }
        break;
      case TAG_LOG_TOGGLE_SERIAL_HEX:
        if(touch_event == TOUCH_ON && !(flags_status.logging_active&&flags_config.do_serial_write)) { flags_config.do_serial_write_hex ^=1; }
        break;
      case TAG_MODE_SET_PID_RPM:
        if(touch_event == TOUCH_ON) {
          if (sys_mode == MODE_PID_RPM_CARB)
          {
            sys_mode = MODE_DIRECT;
          }
          else
          {
            if(sys_mode == MODE_DIRECT)
            {
              configure_PID();
            }
            sys_mode = MODE_PID_RPM_CARB; //we need some sort of 'change mode' function to take care of intialisation
          }
        }
        break;
      case TAG_CAL_PID_RPM_P:
        if(touch_event != TOUCH_OFF)
        {
          pid_set_parameter(&RPM_control.k.p);
          flags_status.redraw_pending = true;
        }
        break;
      case TAG_CAL_PID_RPM_I:
        if(touch_event != TOUCH_OFF)
        {
          pid_set_parameter(&RPM_control.k.i);
          flags_status.redraw_pending = true;
        }
        break;
      case TAG_CAL_PID_RPM_D:
        if(touch_event != TOUCH_OFF)
        {
          pid_set_parameter(&RPM_control.k.d);
          flags_status.redraw_pending = true;
        }
        break;
      case TAG_ENGINE_START:
        break;
      case TAG_ENGINE_STOP:
        break;
      case TAG_CAL_TORQUE_ZERO:
        if(touch_event == TOUCH_OFF)
        {
          tq_set_zero();
          flags_status.redraw_pending = true;
        }
        break;
      case TAG_CAL_TORQUE_MAX:
        if(touch_event == TOUCH_OFF)
        {
          tq_set_high();
          flags_status.redraw_pending = true;
        }
        break;
      case TAG_CAL_TORQUE_MIN:
        if(touch_event == TOUCH_OFF)
        {
          tq_set_low();
          flags_status.redraw_pending = true;
        }
        break;
      case TAG_HOLD_INPUT:
        if(touch_event == TOUCH_OFF)
        {
          flags_status.hold_direct_input ^= 1;
          flags_status.redraw_pending = true;
        }
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
  Serial.print(FS(S_COMMA));
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
  GD.Vertex2ii(px+CELL_BORDER,py+CELL_BORDER,0,0);
  GD.Vertex2ii(sx-CELL_BORDER,sy-CELL_BORDER,0,0);
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
/* draw an integer value and a label from progmem, using a 4x4 grid size*/
void draw_readout_int(const int pos_x, const int pos_y, int opts, const int value, const char *label_pgm)
{
  byte font = 31;
  char dx = CELL_BORDER, dy =CELL_BORDER;
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
  if(labs(value)>9999) { font-=3; }
  //else if(labs(value)>99999) { font-=2; }
  //else if(labs(value)>9999) { font-=1; }
  
  GD.cmd_number(pos_x+dx, pos_y-8, font, opts, value);
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
void draw_readout_fixed(const int pos_x, const int pos_y, int opts, const int value, const byte frac_bits, byte extra_sf, const char *label_pgm, bool small)
{
  int  dx = CELL_BORDER*2, dy = -8;
  if(opts&OPT_RIGHTX)
  {    dx = -dx;  }
  else if(opts&OPT_CENTERX)
  {    dx = 0;   }
  
  int grid_sx = GRID_SX(4);   //default size is 1/4 x 1/4 screen
  int grid_sy = GRID_SY(4);
  if(small)
  {
    grid_sx = GRID_SX(9);      //small size is width of vertical sliders
    grid_sy = GRID_SY(6);      //small size is width of vertical sliders
    dy = dy + (grid_sy>>1);
  }
  
//  if(opts&OPT_CENTERY)
//  {    dy = 0;  }
  
  
  GD.ColorA(A_BKG_WINDOW);
  GD.ColorRGB(C_BKG_NORMAL);
  draw_box(pos_x, pos_y, grid_sx, grid_sy, BOX_WIDTH, opts);

  GD.ColorA(A_OPAQUE);
  if(label_pgm != NULL && !small)
  {
    MAKE_STRING(label_pgm);
  
    
    GD.ColorRGB(C_LABEL);
    GD.cmd_text(pos_x+dx, pos_y+20, 28, opts, label_pgm_str);
  }

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
    Serial.println(F("FPERR: dec.")); //has no impact on flash usage? is it being optimised out?
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
    Serial.println(F("FPERR: int."));
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
  byte str_len = strlen(str_ptr);
  if(small)
  {
    font_size = 29;
    if(str_len > 6)      {font_size = 20;}
    else if(str_len > 3) {font_size -= (str_len -3);}
  }
  else
  {
    str_len >>=1;
    if(str_len > 9) {font_size = 26;}
    else if(str_len > 4) {font_size -= (str_len - 4);}
  }

  GD.ColorRGB(C_VALUE);
  GD.cmd_text(pos_x+dx, pos_y+dy, font_size, opts, str_ptr);
}


void draw_slider_horz(int x, int y, int sx, int sy, int label_size, const char *label_str, byte tag, int value, int val_max)
{
  int x2 = x + (CELL_BORDER<<1);
  int sx2 = sx - (label_size + (CELL_BORDER<<1) + SLIDER_HEIGHT);
  int y2 = y + (sy>>1);
  int opt = (tag==GD.inputs.tag)? OPT_FLAT:0;
  
  GD.Tag(tag);
  GD.ColorA(A_BKG_WINDOW);
  GD.ColorRGB(C_BKG_NORMAL);
  draw_box(x,y,sx,sy,BOX_WIDTH, 0);
#ifdef DEBUG_SCREENS_DRAW_SLIDER_VERT
  Serial.print(F("draw slider box: "));
//  Serial.print(x);
//  Serial.print(FS(S_COMMA));
  Serial.print(y);
  Serial.print(FS(S_COMMA));
  Serial.print(sx);
  Serial.print(FS(S_COMMA));
  Serial.print(sy);
  Serial.println();
#endif
  GD.ColorA(A_OPAQUE);
  GD.ColorRGB(C_LABEL);
  GD.cmd_text(x2, y2, 26, OPT_CENTERY, label_str);
#ifdef DEBUG_SCREENS_DRAW_SLIDER_VERT
  Serial.print(F("draw slider text: "));
//  Serial.print(x2);
//  Serial.print(FS(S_COMMA));
  Serial.print(y2);
  Serial.print(FS(S_COMMA));
  Serial.println();
#endif
  x2 = x + label_size;
  y2 -= SLIDER_HEIGHT>>1;
#ifdef DEBUG_SCREENS_DRAW_SLIDER_VERT
  Serial.print(F("draw slider widget: "));
  Serial.print(x2);
  Serial.print(FS(S_COMMA));
  Serial.print(y2);
  Serial.print(FS(S_COMMA));
  Serial.print(sx2);
  Serial.print(FS(S_COMMA));
  Serial.println(label_size);
  Serial.println();
#endif
  
  GD.cmd_slider(x2,y2,sx2,SLIDER_HEIGHT,opt,value,val_max);
  GD.Tag(TAG_INVALID);
}



/* map touch coord to value over a given range */
int get_slider_value_horz(int px_min, int px_max, int param_min, int param_max)
{

  px_min += SLIDER_HEIGHT+CELL_BORDER;
  px_max -= SLIDER_HEIGHT+CELL_BORDER;
  //clamp the x coordinate to the active range
  int x_coord = constrain(GD.inputs.xytouch.x,px_min,px_max);
  //map the active range to servo range
  int x_val = map(x_coord, px_min, px_max, param_min, param_max);
  #ifdef DEBUG_SLIDER_VALUE_HORZ
  Serial.print(F("slider horz: "));
  Serial.print(x_coord);
  Serial.print(F(" ("));
  Serial.print(px_min);
  Serial.print(F(", "));
  Serial.print(px_max);
  Serial.print(F(") -> "));
  Serial.println(x_val);
  #endif
  return x_val;
}

void draw_slider_vert(int x, int y, int sx, int sy, int label_size, const char *label_str, byte tag, int value, int val_max)
{
  int y2 = y + label_size/2;
  int sy2 = sy - (label_size + (CELL_BORDER<<1) + SLIDER_WIDTH);
  int x2 = x + (sx>>1);
  int opt = (tag==GD.inputs.tag)? OPT_FLAT:0;
  
  GD.Tag(tag);
  GD.ColorA(A_BKG_WINDOW);
  GD.ColorRGB(C_BKG_NORMAL);
  draw_box(x,y,sx,sy,BOX_WIDTH, 0);
#ifdef DEBUG_SCREENS_DRAW_SLIDER_VERT
  Serial.print(F("draw slider box: "));
  Serial.print(x);
  Serial.print(FS(S_COMMA));
  Serial.print(y);
  Serial.print(FS(S_COMMA));
  Serial.print(sx);
  Serial.print(FS(S_COMMA));
  Serial.print(sy);
  Serial.println();
#endif
  GD.ColorA(A_OPAQUE);
  GD.ColorRGB(C_LABEL);
  GD.cmd_text(x2, y2, 26, OPT_CENTER, label_str);
#ifdef DEBUG_SCREENS_DRAW_SLIDER_VERT
  Serial.print(F("draw slider text: "));
  Serial.print(x2);
  Serial.print(FS(S_COMMA));
  Serial.print(y2);
  Serial.print(FS(S_COMMA));
  Serial.println();
#endif
  y2 = y + label_size;
  x2 -= SLIDER_WIDTH>>1;
#ifdef DEBUG_SCREENS_DRAW_SLIDER_VERT
  Serial.print(F("draw slider widget: "));
  Serial.print(x2);
  Serial.print(FS(S_COMMA));
  Serial.print(y2);
  Serial.print(FS(S_COMMA));
  Serial.print(sy2);
  Serial.print(FS(S_COMMA));
  Serial.println(label_size);
  Serial.println();
#endif
  GD.ColorRGB(C_SLIDER_VERT);
  GD.cmd_slider(x2,y2,SLIDER_WIDTH,sy2,opt,value,val_max);
  GD.Tag(TAG_INVALID);
}



int get_slider_value_vert(int py_min, int py_max, int param_min, int param_max)
{

  py_min += SLIDER_WIDTH+CELL_BORDER;
  py_max -= SLIDER_WIDTH+CELL_BORDER;
  //clamp the x coordinate to the active range
  int y_coord = constrain(GD.inputs.xytouch.y, py_min, py_max);
  //map the active range to servo range
  int y_val = map(y_coord, py_min, py_max, param_min, param_max);
  #ifdef DEBUG_SLIDER_VALUE_VERT
  Serial.print(F("slider: "));
  Serial.print(GD.inputs.xytouch.y);
  Serial.print(FS(S_COMMA));
  Serial.print(y_coord);
  Serial.print(F(" ("));
  Serial.print(py_min);
  Serial.print(FS(S_COMMA));
  Serial.print(py_max);
  Serial.print(F(") -> ("));
  Serial.print(param_min);
  Serial.print(FS(S_COMMA));
  Serial.print(param_max);
  Serial.print(F(") "));
  Serial.println(y_val);
  #endif
  return y_val;
}

/* fetch the slider pixel position
 *  int 30356
 *  byte 30356
 */
int get_slider_value_vert_px(int py_min, int py_range)
{
//  py_min += SLIDER_WIDTH+CELL_BORDER;
//  py_range -= SLIDER_WIDTH+CELL_BORDER;
  //clamp the x coordinate to the active range
  int y_coord = constrain(GD.inputs.xytouch.y - py_min, 0, py_range);
  y_coord = py_range - y_coord;
  #ifdef DEBUG_SLIDER_VALUE_VERT
  Serial.print(F("slider: "));
  Serial.print(GD.inputs.xytouch.y);
  Serial.print(FS(S_COMMA));
  Serial.print(y_coord);
  
  #endif
  return y_coord;
}

void draw_button(int x, int y, byte sx, byte sy, byte tag, const char * string)
{
  GD.Tag(tag);
  int opt = (GD.inputs.tag == tag)? OPT_FLAT : 0;
  GD.cmd_button(x+CELL_BORDER,y+CELL_BORDER,sx-(CELL_BORDER<<1),sy-(CELL_BORDER<<1), 26, opt, string);
}

void draw_toggle_button(int x, int y, byte sx, byte sy, byte tag, byte state, const char * string)
{
  GD.Tag(tag);
  int opt = state ? OPT_FLAT : 0;
  GD.cmd_button(x+CELL_BORDER,y+CELL_BORDER,sx-(CELL_BORDER<<1),sy-(CELL_BORDER<<1), 26, opt, string);
}

void draw_log_toggle_button(int x, int y, byte sx, byte sy)
{
  GD.Tag(TAG_LOG_TOGGLE);

  int opt = (GD.inputs.tag == TAG_LOG_TOGGLE) ? OPT_FLAT : 0;

  if(flags_status.logging_active)
  {
    GD.cmd_fgcolor(C_BTN_LOGGING);
  }
  else
  {
    GD.cmd_fgcolor(C_BUTTON_FG);
  }
  MAKE_STRING(S_REC);
  GD.cmd_button(x+CELL_BORDER,y+CELL_BORDER,sx-(CELL_BORDER<<1),sy-(CELL_BORDER<<1), 26, opt, S_REC_str);

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
      READ_STRING(S_HEX,&string[i]);
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
    GD.cmd_button(GRID_XL(0,SCREEN_BUTTON_XN)+CELL_BORDER, GRID_YT(n,NO_OF_SCREENS)+CELL_BORDER, GRID_SX(SCREEN_BUTTON_XN)-(CELL_BORDER<<1), GRID_SY(NO_OF_SCREENS)-(CELL_BORDER<<1), SCREEN_BUTTON_FONT, opt, label);
  }
}

void draw_screen_background()
{
    /* data record to read */
  DATA_RECORD *data_record = &LAST_RECORD;

  long colour_bg = C_BKG_NORMAL;
  /* simple background colour setting by rpm range
     this should be changed to read from system status flags */
  if (Data_Averages.RPM == 0)
  {    colour_bg = C_BKG_STOPPED;  }
  else if (Data_Averages.RPM < 1300 || Data_Averages.RPM > 3800)
  {    colour_bg = C_BKG_WARNING;  }
  else
  {    colour_bg = C_BKG_RUNNING;  }

  GD.ClearColorRGB(colour_bg);
  GD.Clear();
}



byte is_touching_inside(int x, int y, byte sx, byte sy)
{
  return (GD.inputs.xytouch.x > x && GD.inputs.xytouch.x < (x+sx) && GD.inputs.xytouch.y > y && GD.inputs.xytouch.y < (y+sy));
}
