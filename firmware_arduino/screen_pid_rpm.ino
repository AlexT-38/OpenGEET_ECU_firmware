/*
 * single channel pid monitor
 */
#ifdef XN
#undef XN
#endif
#define XN 4
#ifdef YN
#undef YN
#endif
#define YN 4


void screen_draw_pid_rpm()
{
  byte gx, gy;

  /* data record to read */
  DATA_RECORD *data_record = &Data_Record[1-Data_Record_write_idx];

  //clear tag
  GD.Tag(TAG_INVALID);
  
  // Right hand column
  gx = XN-1;
  gy = 0;
  draw_readout_int(GRID_XR(gx,XN), GRID_YC(gy++,YN), OPT_RIGHTX | OPT_CENTERY, data_record->RPM_avg, S_RPM);
  draw_readout_int(GRID_XR(gx,XN), GRID_YC(gy++,YN), OPT_RIGHTX | OPT_CENTERY, data_record->A0_avg, S_MAP_MBAR);
  draw_readout_fixed(GRID_XR(gx,XN), GRID_YC(gy++,YN), OPT_RIGHTX | OPT_CENTERY, data_record->EGT_avg, 2,0, S_EGT1_DEGC);
  draw_datetime(GRID_XR(gx,XN), GRID_YC(gy++,YN), OPT_RIGHTX );

  // Mid right column
  gy = 0;
  gx--;

  if(sys_mode!=MODE_DIRECT)
  {
    draw_readout_int(GRID_XR(gx,XN), GRID_YC(gy++,YN), OPT_RIGHTX | OPT_CENTERY, MS_TO_RPM(data_record->A1_avg), S_TARGET);
  }
  else
  {
    draw_readout_int(GRID_XR(gx,XN), GRID_YC(gy++,YN), OPT_RIGHTX | OPT_CENTERY, data_record->A1_avg, S_INPUT_1);
  }  
  draw_readout_int(GRID_XR(gx,XN), GRID_YC(gy++,YN), OPT_RIGHTX | OPT_CENTERY, data_record->PID_SV0_avg, S_OUTPUT);
  
  MAKE_STRING(S_ACTIVE);
  draw_toggle_button(GRID_XL(XN-2,XN),GRID_YT(YN-2,YN),GRID_SX(XN),GRID_SY(YN),TAG_MODE_SET_PID_RPM,(sys_mode == MODE_PID_RPM_CARB),S_ACTIVE_str);
  
  draw_log_toggle_button(GRID_XL(XN-2,XN),GRID_YT(YN-1,YN),GRID_SX(XN),GRID_SY(YN));

}

/* work around for tag always zero issue*/
#ifdef TAG_BYPASS

byte screen_pid_rpm_tags()
{
  byte tag = TAG_INVALID;
  // log and active buttons
  if(is_touching_inside(GRID_XL(XN-2,XN),GRID_YT(YN-2,YN),GRID_SX(XN),GRID_SY(YN)*2)) 
  { 
    tag = GD.inputs.xytouch.y > GRID_YT(YN-1,YN) ? TAG_LOG_TOGGLE : TAG_MODE_SET_PID_RPM;
  }
  else
  {
    //pid tuning controls
  }
  
  return tag;
}
#endif
