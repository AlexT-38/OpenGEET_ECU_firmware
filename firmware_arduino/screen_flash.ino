/* flash screen shown at startup only */

void screen_draw_flash(const char * firwmare_string, const char * card_status_string)
{
    
  //initialise the display
  GD.begin(~GD_STORAGE);

  // draw splash screen...
  GD.Clear();

  //firmware name and version
  GD.cmd_text(GD.w/2, GD.h/2-32, 29, OPT_CENTER, firwmare_string);

  //date and time
  DateTime t_compile = CompileDateTime();
  char datetime_string[11];
  date_to_string(t_compile, datetime_string);
  GD.cmd_text(GD.w/2-48,  GD.h/2, 26, OPT_CENTER, datetime_string);
  time_to_string(t_compile, datetime_string);
  GD.cmd_text(GD.w/2+48,  GD.h/2, 26, OPT_CENTER, datetime_string);

  //sd card status
  GD.cmd_text(GD.w/2, GD.h-32, 27, OPT_CENTER, card_status_string);
  
  //output file name, if valid           
//  if (flags_status.sdcard_available && flags_config.do_sdcard_write)
//  {
//    MAKE_STRING(S_OUTPUT_FILE_NAME_C);
//    GD.cmd_text(GD.w/2, GD.h/2+64, 26, OPT_CENTERY | OPT_RIGHTX, S_OUTPUT_FILE_NAME_C_str);
//    GD.cmd_text(GD.w/2, GD.h/2+64, 26, OPT_CENTERY, output_filename);
//  }
  
  // show the screen  
  GD.swap();
  // free the SPI port
  GD.__end();

  #ifdef DEBUG_SCREEN_RES
  Serial.print(F("Screen res w,h: "));
  Serial.print(GD.w);
  Serial.print(F(", "));
  Serial.print(GD.h);
  
  #endif
}
