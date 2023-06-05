/*
 * basic readout screen
 */
#ifdef XN
#undef XN
#endif
#define XN 4
#ifdef YN
#undef YN
#endif
#define YN 4

/*
 *  IN1 RPM MAP
 *  IN2 TRQ TMP
 *  IN3 POW EGT
 */

void screen_draw_basic()
{
  byte gx, gy;

  /* data record to read */
  DATA_RECORD *data_record = &LAST_RECORD;

  //clear tag
  GD.Tag(TAG_INVALID);
  
  // Right hand column
  gx = XN-1;
  gy = 0;
  if(Data_Config.MAP_no > 0)  {draw_readout_int(GRID_XR(gx,XN), GRID_YC(gy++,YN), DRO_OPT, Data_Averages.MAP[0], S_MAP_MBAR);} else {gy++;}
  if(Data_Config.TMP_no > 0)  {draw_readout_int(GRID_XR(gx,XN), GRID_YC(gy++,YN), DRO_OPT, Data_Averages.TMP[0], S_TEMP_DEGC);} else {gy++;}
  if(Data_Config.EGT_no > 0)  {draw_readout_fixed(GRID_XR(gx,XN), GRID_YC(gy++,YN), DRO_OPT, Data_Averages.EGT[0], 2,0, S_EGT_DEGC, false);} else {gy++;}
  
  draw_datetime(GRID_XR(gx,XN), GRID_YC(gy++,YN), OPT_RIGHTX );

  // Mid right column
  gy = 0;
  gx--;
  draw_readout_int(GRID_XR(gx,XN), GRID_YC(gy++,YN), DRO_OPT, Data_Averages.RPM, S_RPM);
  draw_readout_int(GRID_XR(gx,XN), GRID_YC(gy++,YN), DRO_OPT, Data_Averages.TRQ, S_TORQUE_MNM);
  draw_readout_int(GRID_XR(gx,XN), GRID_YC(gy++,YN), DRO_OPT, Data_Averages.POW, S_POWER_W);
  
  
  
  draw_log_toggle_button(GRID_XL(XN-2,XN),GRID_YT(YN-1,YN),GRID_SX(XN),GRID_SY(YN));

  //mid left column
  gy = 0;
  gx--;
  draw_readout_int(GRID_XR(gx,XN), GRID_YC(gy++,YN), DRO_OPT, Data_Averages.USR[0], S_INPUT_1);
  draw_readout_int(GRID_XR(gx,XN), GRID_YC(gy++,YN), DRO_OPT, Data_Averages.USR[1], S_INPUT_2);
  draw_readout_int(GRID_XR(gx,XN), GRID_YC(gy++,YN), DRO_OPT, Data_Averages.USR[2], S_INPUT_3);

  MAKE_STRING(S_HOLD);
  draw_toggle_button(GRID_XL(XN-3,XN), GRID_YT(3,YN), GRID_SX(XN),GRID_SY(YN), TAG_HOLD_INPUT, flags_status.hold_direct_input, S_HOLD_str);

}

/* work around for tag always zero issue*/
#ifdef TAG_BYPASS

byte screen_basic_tags()
{
  byte tag = TAG_INVALID;
  if(is_touching_inside(GRID_XL(XN-2,XN),GRID_YT(YN-1,YN),GRID_SX(XN),GRID_SY(YN))) { tag = TAG_LOG_TOGGLE;}
  else if(is_touching_inside(GRID_XL(XN-3,XN),GRID_YT(0,YN),GRID_SX(XN),GRID_SY(YN))) { tag = TAG_CAL_TORQUE_ZERO;}
  else if(is_touching_inside(GRID_XL(XN-3,XN),GRID_YT(1,YN),GRID_SX(XN),GRID_SY(YN))) { tag = TAG_CAL_TORQUE_MAX;}
  else if(is_touching_inside(GRID_XL(XN-3,XN),GRID_YT(2,YN),GRID_SX(XN),GRID_SY(YN))) { tag = TAG_CAL_TORQUE_MIN;}
  else if(is_touching_inside(GRID_XL(XN-3,XN),GRID_YT(3,YN),GRID_SX(XN),GRID_SY(YN))) { tag = TAG_HOLD_INPUT;}
  return tag;
}
#endif
