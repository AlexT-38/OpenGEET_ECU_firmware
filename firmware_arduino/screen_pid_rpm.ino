/*
 * single channel pid monitor
 */
//#define DEBUG_PID_SLIDER_SET
//#define DEBUG_PID_SHOW_PARAMS
 
#ifdef XN
#undef XN
#endif
#define XN 4
#ifdef YN
#undef YN
#endif
#define YN 4

#define PID_SLIDER_Y0           0
#define PID_SLIDER_X0           GRID_XL(1,6)
#define PID_SLIDER_SX           (GRID_SX(9))
#define PID_SLIDER_LABEL_SY     GRID_SY(6)
#define PID_SLIDER_RANGE_SY     (GRID_SY(6)*4)
#define PID_SLIDER_SY           (PID_SLIDER_RANGE_SY + PID_SLIDER_LABEL_SY)

#define LOG_SLIDER_PX_RANGE     174



void pid_set_parameter(int *param)
{
//  int out_max = pid_convert_k_to_px(PID_FP_MAX);
//  int in_y = get_slider_value_vert(PID_SLIDER_LABEL_SY, PID_SLIDER_SY , out_max, 0);
  // get number of pixels up to range max from base of slider
  int in_y = get_slider_value_vert_px(PID_SLIDER_LABEL_SY, LOG_SLIDER_PX_RANGE);
  #ifdef DEBUG_PID_SLIDER_SET
  Serial.print(F("pid slider px: "));
  Serial.println(in_y);
  #endif
  *param = pid_convert_px_to_k(in_y);
}


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
  #ifdef  DEBUG_PID_FEEDBACK
  if(sys_mode!=MODE_DIRECT)
  {
    draw_readout_int(GRID_XR(gx,XN), GRID_YC(gy++,YN), OPT_RIGHTX | OPT_CENTERY, MS_TO_RPM(DEBUG_PID_FEEDBACK_VALUE), S_RPM);
  }
  else
  #endif
  {
    draw_readout_int(GRID_XR(gx,XN), GRID_YC(gy++,YN), OPT_RIGHTX | OPT_CENTERY, data_record->RPM_avg, S_RPM);
  }
  
  
  draw_readout_int(GRID_XR(gx,XN), GRID_YC(gy++,YN), OPT_RIGHTX | OPT_CENTERY, data_record->A0_avg, S_MAP_MBAR);
  draw_readout_fixed(GRID_XR(gx,XN), GRID_YC(gy++,YN), OPT_RIGHTX | OPT_CENTERY, data_record->EGT_avg, 2,0, S_EGT1_DEGC, false);
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


  char string[] = {'P',0};

  //byte pos_x = PID_SLIDER_X0;

  int slider_max = LOG_SLIDER_PX_RANGE;//pid_convert_k_to_px(PID_FP_MAX);
  int slider_val = slider_max - pid_convert_k_to_px(RPM_control.k.p);
  draw_slider_vert(PID_SLIDER_X0,                   PID_SLIDER_Y0, PID_SLIDER_SX, PID_SLIDER_SY, PID_SLIDER_LABEL_SY, string, TAG_CAL_PID_RPM_P, slider_val, slider_max);
  #ifdef DEBUG_PID_SHOW_PARAMS
  //                 px,                            py,      align opts,          value,        frac_bits,  extra_sf, label_pgm, small
  draw_readout_fixed(PID_SLIDER_X0+PID_SLIDER_SX, PID_SLIDER_SY-8, OPT_RIGHTX, RPM_control.k.p, PID_FP_FRAC_BITS, 0, NULL, true );
  draw_readout_fixed(PID_SLIDER_X0+PID_SLIDER_SX, PID_SLIDER_SY+8, OPT_RIGHTX, RPM_control.p, PID_FP_FRAC_BITS, 0, NULL, true );
  #else
  draw_readout_fixed(PID_SLIDER_X0+PID_SLIDER_SX, PID_SLIDER_SY, OPT_RIGHTX, RPM_control.k.p, PID_FP_FRAC_BITS, 0, NULL, true );
  #endif
  
  string[0] = 'I';
  slider_val = slider_max - pid_convert_k_to_px(RPM_control.k.i);
  draw_slider_vert(PID_SLIDER_X0+PID_SLIDER_SX,     PID_SLIDER_Y0, PID_SLIDER_SX, PID_SLIDER_SY, PID_SLIDER_LABEL_SY, string, TAG_CAL_PID_RPM_I, slider_val, slider_max);
  #ifdef DEBUG_PID_SHOW_PARAMS
  draw_readout_fixed(PID_SLIDER_X0+(PID_SLIDER_SX*2),   PID_SLIDER_SY-8, OPT_RIGHTX, RPM_control.k.i, PID_FP_FRAC_BITS, 0, NULL, true );
  draw_readout_fixed(PID_SLIDER_X0+(PID_SLIDER_SX*2),   PID_SLIDER_SY+8, OPT_RIGHTX, RPM_control.i, PID_FP_FRAC_BITS, 0, NULL, true );
  #else
  draw_readout_fixed(PID_SLIDER_X0+(PID_SLIDER_SX*2),   PID_SLIDER_SY, OPT_RIGHTX, RPM_control.k.i, PID_FP_FRAC_BITS, 0, NULL, true );
  #endif
  
  string[0] = 'D';
  slider_val = slider_max - pid_convert_k_to_px(RPM_control.k.d);
  draw_slider_vert(PID_SLIDER_X0+(PID_SLIDER_SX*2),  PID_SLIDER_Y0, PID_SLIDER_SX, PID_SLIDER_SY, PID_SLIDER_LABEL_SY, string, TAG_CAL_PID_RPM_D, slider_val, slider_max);
  #ifdef DEBUG_PID_SHOW_PARAMS
  draw_readout_fixed(PID_SLIDER_X0+(PID_SLIDER_SX*3),PID_SLIDER_SY-8, OPT_RIGHTX, RPM_control.k.d, PID_FP_FRAC_BITS, 0, NULL, true );
  draw_readout_fixed(PID_SLIDER_X0+(PID_SLIDER_SX*3),PID_SLIDER_SY+8, OPT_RIGHTX, RPM_control.d, PID_FP_FRAC_BITS, 0, NULL, true );
  #else
  draw_readout_fixed(PID_SLIDER_X0+(PID_SLIDER_SX*3),PID_SLIDER_SY, OPT_RIGHTX, RPM_control.k.d, PID_FP_FRAC_BITS, 0, NULL, true );
  #endif
  
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
    if(is_touching_inside(PID_SLIDER_X0, PID_SLIDER_Y0, PID_SLIDER_SX*3, PID_SLIDER_SY))
    {
      if(GD.inputs.xytouch.x > (PID_SLIDER_X0+(PID_SLIDER_SX*2)))
      {
        tag = TAG_CAL_PID_RPM_D;
      }
      else if(GD.inputs.xytouch.x > (PID_SLIDER_X0+PID_SLIDER_SX))
      {
        tag = TAG_CAL_PID_RPM_I;
      }
      else
      {
        tag = TAG_CAL_PID_RPM_P;
      }
    }
  }
  
  return tag;
}
#endif
