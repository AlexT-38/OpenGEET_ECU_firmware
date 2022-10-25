#ifndef __RECORDS_H__
#define __RECORDS_H__


/* struct for storing sensor samples over the update period */
#define DATA_RECORD_VERSION     2

typedef struct data_record
{
  long int timestamp;                   // TODO: use the RTC for this
  unsigned int A0[ANALOG_SAMPLES_PER_UPDATE];    // TODO: replace these with meaningful values, consider seperate update rates, lower resolution
  unsigned int A1[ANALOG_SAMPLES_PER_UPDATE];
  unsigned int A2[ANALOG_SAMPLES_PER_UPDATE];
  unsigned int A3[ANALOG_SAMPLES_PER_UPDATE];
  unsigned int A0_avg;
  unsigned int A1_avg;
  unsigned int A2_avg;
  unsigned int A3_avg;
  byte ANA_no_of_samples;
  int EGT[EGT_SAMPLES_PER_UPDATE];
  int EGT_avg;
  byte EGT_no_of_samples;
  unsigned int RPM_avg;                          //average rpm over this time period, can be caluclated from number of ticks and update rate
  byte RPM_no_of_ticks;                 //so we can use bytes for storage here and have 235RPM as the slowest measurable rpm by tick time.
  byte RPM_tick_times_ms[RPM_MAX_TICKS_PER_UPDATE];           //rpm is between 1500 and 4500, giving tick times in ms of 40 and 13.3, 
  unsigned int PID_SV0[PID_LOOPS_PER_UPDATE];
  unsigned int PID_SV0_avg;
  byte PID_no_of_samples;
} DATA_RECORD; //... bytes per record with current settings

/* struct for wrapping around any data */ //this is unneccesarily general
typedef struct data_storage
{
  unsigned int bytes_stored;
  byte hash;
  byte *data;
}DATA_STORAGE;




/* the records to write to */
DATA_RECORD Data_Record[2];
/* the index of the currently written record */
byte Data_Record_write_idx = 0;

#define CURRENT_RECORD    Data_Record[Data_Record_write_idx]
char output_filename[13] = ""; //8+3 format


#endif //__RECORDS_H__
