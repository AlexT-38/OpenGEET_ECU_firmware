/*
 * torque cal readout screen
 */
#ifdef XN
#undef XN
#endif
#define XN 4
#ifdef YN
#undef YN
#endif
#define YN 4


void screen_draw_torque()
{
  byte gx, gy;

  /* data record to read */
  DATA_RECORD *data_record = &LAST_RECORD;

  //clear tag
  GD.Tag(TAG_INVALID);
  
  // Right hand column
  gx = XN-1;
  gy = YN-1;
  draw_datetime(GRID_XR(gx,XN), GRID_YC(gy++,YN), OPT_RIGHTX );

  // Mid right column
  gy = 0;
  gx--;
  draw_readout_int(GRID_XR(gx,XN), GRID_YC(gy++,YN), DRO_OPT, torque_cal.counts_zero, S_TQ_ZR_LSB);
  draw_readout_int(GRID_XR(gx,XN), GRID_YC(gy++,YN), DRO_OPT, torque_cal.counts_max, S_TQ_MX_LSB);
  draw_readout_int(GRID_XR(gx,XN), GRID_YC(gy++,YN), DRO_OPT, torque_cal.counts_min, S_TQ_MN_LSB);
  draw_readout_int(GRID_XR(gx,XN), GRID_YC(gy++,YN), DRO_OPT, Data_Averages.TRQ, S_TORQUE_MNM);
  
  
  //mid left column
  gy = 0;
  gx--;
  MAKE_STRING(S_TQ_SET_ZR);
  draw_button(GRID_XL(XN-3,XN), GRID_YT(0,YN), GRID_SX(XN),GRID_SY(YN), TAG_CAL_TORQUE_ZERO, S_TQ_SET_ZR_str);
  MAKE_STRING(S_TQ_SET_MX);
  draw_button(GRID_XL(XN-3,XN), GRID_YT(1,YN), GRID_SX(XN),GRID_SY(YN), TAG_CAL_TORQUE_MAX, S_TQ_SET_MX_str);
  MAKE_STRING(S_TQ_SET_MN);
  draw_button(GRID_XL(XN-3,XN), GRID_YT(2,YN), GRID_SX(XN),GRID_SY(YN), TAG_CAL_TORQUE_MIN, S_TQ_SET_MN_str);

  #ifdef DEBUG_TORQUE_CAL
  print_torque_cal(Serial);
  #endif
  

}
