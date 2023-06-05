#ifndef __RECORDS_H__
#define __RECORDS_H__

/* averages will now be stored seperately and not logged (or logged instead of all samples, depending on setting)
 *  
 */

#define DATA_RECORD_VERSION     3
typedef struct data_config
{
  unsigned int data_version;
  byte USR_no, MAP_no, TMP_no, EGT_no, SRV_no;
  
}DATA_CONFIG;

typedef struct data_averages
{
  int USR[NO_OF_USER_INPUTS];
  int MAP[NO_OF_MAP_SENSORS];
  int TMP[NO_OF_TMP_SENSORS];
  int EGT[NO_OF_EGT_SENSORS];
  int TRQ;
  unsigned int RPM;                          //average rpm over this time period, can be caluclated from number of ticks and update rate
  int SRV[NO_OF_SERVOS];
  int POW;                                  //power in (W?) product of torque(Nm) and rpm(rad/s); rad/s = 0.10471975511965977461542144610932 per/rpm, Nm = 0.001 mNm
                                            //rpm will have range ~150-450rad/s, torque in range +/- ~10Nm, result would be +/- ~4500W
                                            //native units are RPM (x0.105), mNm (x0.001)
                                            //native power units range +/- ~45,000,000 (27bits)
                                            //to convert to mW, multiply by 0.105, range +/- ~4,712,389 (24bits)
                                            //convert to dW (0.1W) multiply by 0.00105, range +/- ~47,123 (17bit)
                                            //convert to W (0.1W) multiply by 0.000105, range +/- ~4,712 (13bit)
}DATA_AVERAGES;

  /* struct for storing sensor samples over the update period */
typedef struct data_record
{
  long int timestamp;

//sensors using the internal ADC, or HX771 at 10Hz
  byte ANA_no_of_samples;
  int USR[NO_OF_USER_INPUTS][ANALOG_SAMPLES_PER_UPDATE];
  int MAP[NO_OF_MAP_SENSORS][ANALOG_SAMPLES_PER_UPDATE];
  int TMP[NO_OF_TMP_SENSORS][ANALOG_SAMPLES_PER_UPDATE];
  int TRQ[ANALOG_SAMPLES_PER_UPDATE];

//sensors using the MAX6675 at 4Hz
  byte EGT_no_of_samples;
  int EGT[NO_OF_EGT_SENSORS][EGT_SAMPLES_PER_UPDATE];
  
//rpm from flywheel magnet pickup  
  byte RPM_no_of_ticks;
  unsigned int RPM_tick_times_ms[RPM_MAX_TICKS_PER_UPDATE];           //rpm is between 1500 and 4500, giving tick times in ms of 40 and 13.3, 

//servo outputs
  byte SRV_no_of_samples;
  int SRV[NO_OF_SERVOS][PID_LOOPS_PER_UPDATE];
  
} DATA_RECORD; //... bytes per record with current settings

/* struct for wrapping around any data */ //this is unneccesarily general
typedef struct data_storage
{
  unsigned int bytes_stored;
  byte hash;
  byte *data;
}DATA_STORAGE;




/* the records to write to */
extern DATA_RECORD Data_Records[2];
/* the index of the currently written record */
extern byte Data_Record_write_idx;

#define CURRENT_RECORD    Data_Records[Data_Record_write_idx]
#define LAST_RECORD       Data_Records[1-Data_Record_write_idx]

extern DATA_AVERAGES Data_Averages;
extern DATA_CONFIG Data_Config;


extern char output_filename[13]; //8+3 format

extern File log_data_file; 


#endif //__RECORDS_H__
