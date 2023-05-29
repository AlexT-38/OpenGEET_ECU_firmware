/*
 * Config Screen
 * Allows configuration of eeprom parameters: servo calibration, logging format
 */

#ifdef XN
#undef XN
#endif
#define XN 5
#ifdef YN
#undef YN
#endif
#define YN 10
#define YNB (YN>>1)

//#define DEBUG_SCREEN_CONFIG_DRAW_SLIDERS
//#define DEBUG_SCREEN_CONFIG_TOUCH_SLIDERS
//#define DEBUG_EEP_UI

void screen_draw_config()
{
  MAKE_STRING(S_SAVE);
  draw_button(GRID_XL(4,XN),GRID_YT(3,YNB),GRID_SX(XN),GRID_SY(YNB),TAG_EEPROM_SAVE,S_SAVE_str);
  MAKE_STRING(S_LOAD);
  draw_button(GRID_XL(4,XN),GRID_YT(4,YNB),GRID_SX(XN),GRID_SY(YNB),TAG_EEPROM_LOAD,S_LOAD_str);

  MAKE_STRING(S_EXPORT);
  draw_button(GRID_XL(5,XN),GRID_YT(3,YNB),GRID_SX(XN),GRID_SY(YNB),TAG_EEPROM_EXPORT,S_EXPORT_str);
//  MAKE_STRING(S_IMPORT);
//  draw_button(GRID_XL(5,XN),GRID_YT(4,YNB),GRID_SX(XN),GRID_SY(YNB),TAG_EEPROM_IMPORT,S_IMPORT_str);

  MAKE_STRING(S_SERIAL);
  draw_toggle_button(GRID_XL(1,XN),GRID_YT(3,YNB),GRID_SX(XN),GRID_SY(YNB),TAG_LOG_TOGGLE_SERIAL,flags_config.do_serial_write,S_SERIAL_str);
  MAKE_STRING(S_HEX);
  draw_toggle_button(GRID_XL(1,XN),GRID_YT(4,YNB),GRID_SX(XN),GRID_SY(YNB),TAG_LOG_TOGGLE_SERIAL,flags_config.do_serial_write_hex,S_HEX_str);
  MAKE_STRING(S_SDCARD);
  draw_toggle_button(GRID_XL(2,XN),GRID_YT(3,YNB),GRID_SX(XN),GRID_SY(YNB),TAG_LOG_TOGGLE_SERIAL,flags_config.do_sdcard_write,S_SDCARD_str);
  MAKE_STRING(S_DOT_RAW);
  draw_toggle_button(GRID_XL(2,XN),GRID_YT(4,YNB),GRID_SX(XN),GRID_SY(YNB),TAG_LOG_TOGGLE_SERIAL,flags_config.do_sdcard_write_hex,S_DOT_RAW_str);
  
  char strings[6][10];
  READ_STRING(S_SV0_MIN, strings[0]);
  READ_STRING(S_SV0_MAX, strings[1]);
  READ_STRING(S_SV1_MIN, strings[2]);
  READ_STRING(S_SV1_MAX, strings[3]);
  READ_STRING(S_SV2_MIN, strings[4]);
  READ_STRING(S_SV2_MAX, strings[5]);
  int val;

  
  for (byte n = 0; n< 6; n++)
  {
    byte sv = n>>1;
    val = ((n&1)?(servo_cal[sv].lower):(servo_cal[sv].upper)) - SERVO_MIN;
#ifdef DEBUG_SCREEN_CONFIG_DRAW_SLIDERS
    Serial.print(strings[n]);
    Serial.print(GRID_XL(1,XN));
    Serial.print(FS(S_COMMA));
    Serial.print(GRID_YT(n,YN));
    Serial.print(FS(S_COMMA));
    Serial.print(GRID_SX(XN)<<2);
    Serial.print(FS(S_COMMA));
    Serial.print(GRID_SY(YN));
    Serial.print(FS(S_COMMA));
    Serial.print(GRID_SX(XN));
    Serial.println();
#endif
    draw_slider_horz(GRID_XL(1,XN), GRID_YT(n,YN), GRID_SX(XN)<<2, GRID_SY(YN), GRID_SX(XN), strings[n], TAG_CAL_SV0_MIN+n,val, SERVO_RANGE);

  }
#ifdef DEBUG_SCREEN_CONFIG_DRAW_SLIDERS
  Serial.println();
#endif

}

/* work around for tag always zero issue*/
#ifdef TAG_BYPASS
byte screen_config_tags()
{
  byte tag = TAG_INVALID;

  //check sliders
  if(GD.inputs.xytouch.x > GRID_XL(2,XN) && GD.inputs.xytouch.x < SCREEN_W && GD.inputs.xytouch.y > 0 && GD.inputs.xytouch.y < GRID_YB(5,YN))
  {
    tag = TAG_CAL_SV0_MIN + ((GD.inputs.xytouch.y * YN) / SCREEN_H); //checking against each y pos may be faster
#ifdef DEBUG_SCREEN_CONFIG_TOUCH_SLIDERS
    Serial.print(F("slider y, idx: "));
    Serial.print(GD.inputs.xytouch.y);
    Serial.print(FS(S_COMMA));
    Serial.print(GRID_YB(5,YN));
    Serial.print(FS(S_COMMA));
    Serial.print((GD.inputs.xytouch.y * YN) / SCREEN_H);
    Serial.println();
#endif
  }
  else
  {
    
    byte row = GD.inputs.xytouch.y > GRID_YT(4,YNB);
    /* check eep store buttons */
    if(is_touching_inside(GRID_XL(4,XN),GRID_YT(3,YNB),GRID_SX(XN),GRID_SY(YNB)*2))
    {
      if(row)
      {
        tag = TAG_EEPROM_LOAD;
        #ifdef DEBUG_EEP_UI
        Serial.println(FS(S_LOAD));
        #endif
      }
      else
      {
        tag = TAG_EEPROM_SAVE;
        #ifdef DEBUG_EEP_UI
        Serial.println(FS(S_SAVE));
        #endif
      }
    }
    /* check serial options */
    else if(is_touching_inside(GRID_XL(1,XN),GRID_YT(3,YNB),GRID_SX(XN),GRID_SY(YNB)*2))
    {
      tag = TAG_LOG_TOGGLE_SERIAL + row;
    }
    /* check sd card options */
    else if(is_touching_inside(GRID_XL(2,XN),GRID_YT(3,YNB),GRID_SX(XN),GRID_SY(YNB)*2))
    {
      tag = TAG_LOG_TOGGLE_SDCARD + row;
    }
  }
  return tag;
}
#endif
