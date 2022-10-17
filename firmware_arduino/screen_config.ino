/*
 * Config Screen
 * Allows configuration of eeprom parameters: servo calibration, logging format
 */

 void screen_draw_config()
{
  
  byte gx, gy;

  /* data record to read */
  DATA_RECORD *data_record = &Data_Record[1-Data_Record_write_idx];




  
}

/* work around for tag always zero issue*/
#ifdef TAG_BYPASS
byte screen_config_tags()
{
  return TAG_INVALID;
}
#endif
