/*
 * basic readout screen
 */

void draw_basic()
{
  int colour_bg = RGB(0x00,0x20,0x40);

  int pos_y;
  int pos_x;

  
  GD.ClearColorRGB(colour_bg);
  GD.Clear();

  // fetch the average temperature 
  int temperature_degC = CURRENT_RECORD.EGT_avg;
  byte temperature_degC_frac = (25*(temperature_degC&3));
  temperature_degC >>=2;
  
  pos_x = GD.w - 60;
  pos_y = 32;
  GD.cmd_number(pos_x-4 , pos_y, 31, OPT_RIGHTX | OPT_CENTERY, temperature_degC);
  GD.cmd_text(pos_x, pos_y, 31, OPT_CENTER, ".");
  GD.cmd_number(pos_x+4 , pos_y, 31, OPT_CENTERY, temperature_degC_frac);
  pos_x = GD.w - 16;
  MAKE_STRING(EGT1_DEGC);
  GD.cmd_text(pos_x, pos_y+28, 28, OPT_RIGHTX | OPT_CENTERY, EGT1_DEGC_str);

  pos_y += 64;
  MAKE_STRING(RPM);
  GD.cmd_text(pos_x, pos_y+28, 28, OPT_RIGHTX | OPT_CENTERY, RPM_str);
  GD.cmd_number(pos_x, pos_y, 31, OPT_RIGHTX | OPT_CENTERY, CURRENT_RECORD.RPM_avg);


  pos_x = 16;
  pos_y = 32;
  STRING_BUFFER(A0_VALUE);
  GET_STRING(A0_VALUE);
  GD.cmd_text(pos_x, pos_y+28, 28, OPT_CENTERY, string);
  GD.cmd_number(pos_x, pos_y, 31, OPT_CENTERY, CURRENT_RECORD.A0_avg);

  pos_y += 64;
  GET_STRING(A1_VALUE);
  GD.cmd_text(pos_x, pos_y+28, 28, OPT_CENTERY, string);
  GD.cmd_number(pos_x, pos_y, 31, OPT_CENTERY, CURRENT_RECORD.A1_avg);

  pos_y += 64;
  GET_STRING(A2_VALUE);
  GD.cmd_text(pos_x, pos_y+28, 28, OPT_CENTERY, string);
  GD.cmd_number(pos_x, pos_y, 31, OPT_CENTERY, CURRENT_RECORD.A2_avg);

  pos_y += 64;
  GET_STRING(A3_VALUE);
  GD.cmd_text(pos_x, pos_y+28, 28, OPT_CENTERY, string);
  GD.cmd_number(pos_x, pos_y, 31, OPT_CENTERY, CURRENT_RECORD.A3_avg);
  

}
