/*
 * basic readout screen
 */
#ifdef XN
#undef XN
#endif
#define XN 5
#ifdef YN
#undef YN
#endif
#define YN 4
void screen_draw_basic()
{
  byte gx, gy=0;

  /* data record to read */
  DATA_RECORD *data_record = &Data_Record[1-Data_Record_write_idx];
  
  // Right hand column
  gx = XN-1;
  draw_readout_int(GRID_XR(gx,XN), GRID_YC(gy++,YN), OPT_RIGHTX | OPT_CENTERY, data_record->RPM_avg, S_RPM);
  draw_readout_int(GRID_XR(gx,XN), GRID_YC(gy++,YN), OPT_RIGHTX | OPT_CENTERY, data_record->A0_avg, S_MAP_MBAR);
  draw_readout_fixed(GRID_XR(gx,XN), GRID_YC(gy++,YN), OPT_RIGHTX | OPT_CENTERY, data_record->EGT_avg, 2,1, S_EGT1_DEGC);
  draw_datetime(GRID_XR(gx,XN), GRID_YC(gy++,YN), OPT_RIGHTX );

  // Mid right column
  gy = 0;
  gx--;
  draw_readout_int(GRID_XR(gx,XN), GRID_YC(gy++,YN), OPT_RIGHTX | OPT_CENTERY, data_record->A1_avg, S_INPUT_1);
  draw_readout_int(GRID_XR(gx,XN), GRID_YC(gy++,YN), OPT_RIGHTX | OPT_CENTERY, data_record->A2_avg, S_INPUT_2);
  draw_readout_int(GRID_XR(gx,XN), GRID_YC(gy++,YN), OPT_RIGHTX | OPT_CENTERY, data_record->A3_avg, S_INPUT_3);
  
}
