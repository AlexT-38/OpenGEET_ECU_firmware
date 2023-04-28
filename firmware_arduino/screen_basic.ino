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
  draw_readout_int(GRID_XR(gx,XN), GRID_YC(gy++,YN), OPT_RIGHTX | OPT_CENTERY, data_record->RPM_avg, S_RPM);
  draw_readout_int(GRID_XR(gx,XN), GRID_YC(gy++,YN), OPT_RIGHTX | OPT_CENTERY, data_record->A0_avg, S_MAP_MBAR);
  draw_readout_fixed(GRID_XR(gx,XN), GRID_YC(gy++,YN), OPT_RIGHTX | OPT_CENTERY, data_record->EGT_avg, 2,0, S_EGT1_DEGC, false);
  draw_datetime(GRID_XR(gx,XN), GRID_YC(gy++,YN), OPT_RIGHTX );

  // Mid right column
  gy = 0;
  gx--;
  draw_readout_int(GRID_XR(gx,XN), GRID_YC(gy++,YN), OPT_RIGHTX | OPT_CENTERY, data_record->A1_avg, S_INPUT_1);
  draw_readout_int(GRID_XR(gx,XN), GRID_YC(gy++,YN), OPT_RIGHTX | OPT_CENTERY, data_record->A2_avg, S_INPUT_2);
  #ifndef ADC_TORQUE_OVR_CHN3
  #define A3_STRING S_INPUT_3
  #else
  #define A3_STRING S_TORQUE_MNM
  #endif
  draw_readout_int(GRID_XR(gx,XN), GRID_YC(gy++,YN), OPT_RIGHTX | OPT_CENTERY, data_record->A3_avg, A3_STRING);
  
  draw_log_toggle_button(GRID_XL(XN-2,XN),GRID_YT(YN-1,YN),GRID_SX(XN),GRID_SY(YN));

  //mid left column
  gy = 0;
  gx--;
  MAKE_STRING(S_TQ_SET_ZR);
  draw_toggle_button(GRID_XL(XN-3,XN), GRID_YT(0,YN), GRID_SX(XN),GRID_SY(YN), TAG_CAL_TORQUE_ZERO, flags_status.hold_direct_input, S_TQ_SET_ZR_str);
  MAKE_STRING(S_TQ_SET_MX);
  draw_toggle_button(GRID_XL(XN-3,XN), GRID_YT(1,YN), GRID_SX(XN),GRID_SY(YN), TAG_CAL_TORQUE_MAX, flags_status.hold_direct_input, S_TQ_SET_MX_str);
  MAKE_STRING(S_TQ_SET_MN);
  draw_toggle_button(GRID_XL(XN-3,XN), GRID_YT(2,YN), GRID_SX(XN),GRID_SY(YN), TAG_CAL_TORQUE_MIN, flags_status.hold_direct_input, S_TQ_SET_MN_str);
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
