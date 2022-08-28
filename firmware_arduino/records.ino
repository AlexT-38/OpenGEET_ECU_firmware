/* generate a filename from the rtc date/time
 *  if more than 255 files are required in a day, we'll
 *  have to make use of sub folders (or increase the base of NN to 36)
 *  eg /001/YY_MM_DD.txt
 *  or /YYYMMDD/LOG_0001.txt
 *  meanwhile, we'll just overwrite older files
 */

static DATA_STORAGE data_store;

void generate_file_name()
{
  char *str_ptr;
  
  int value;
  byte nn = 0;

  // set the file extension
  str_ptr = output_filename + 8;
  if(flags.do_sdcard_write_hex) {strcpy_P(str_ptr,FILE_EXT_RAW);}
  else                          {strcpy_P(str_ptr,FILE_EXT_TXT);}

  //set the base name from the year month and day
  DateTime t_now = DS1307_now();
  //fill in the digits backwards
  str_ptr = output_filename + 5;
  
  value = t_now.day;    *str_ptr-- = DEC_CHAR(value); value /=10; *str_ptr-- = DEC_CHAR(value);
  value = t_now.month;  *str_ptr-- = DEC_CHAR(value); value /=10; *str_ptr-- = DEC_CHAR(value);
  value = t_now.year;   *str_ptr-- = DEC_CHAR(value); value /=10; *str_ptr-- = DEC_CHAR(value);

  bool do_loop = true;
  while (do_loop)
  {
    //set the iterator number
    str_ptr = output_filename + 7;
    value = nn++;  *str_ptr-- = HEX_CHAR(value);  value >>=4;  *str_ptr-- = HEX_CHAR(value);

    //break the loop if the target file doenst exist, or we have run out of indices
    if (!SD.exists(output_filename) || nn==0 )
    {
      do_loop = false;
    }
  }
  //report the selected filename
  MAKE_STRING(OUTPUT_FILE_NAME_C);
  Serial.print(OUTPUT_FILE_NAME_C_str);
  Serial.println(output_filename);
  
  //check that the file can be opened
  File dataFile = SD.open(output_filename, FILE_WRITE);
  if(dataFile)
  {
    MAKE_STRING(RECORD_VER_C);    dataFile.print(RECORD_VER_C_str);    dataFile.println(DATA_RECORD_VERSION);
    file_print_date_time(t_now, dataFile);
    dataFile.println();
    dataFile.close();
    
    Serial.println(F("Opened OK"));
    flags.do_sdcard_write = true;

    if(nn == 0)
    {
      Serial.println(F("Overwriting older file"));
    }
  }
  else
  {
    Serial.println(F("Open FAILED"));
    flags.do_sdcard_write = false;
  }

}

/* generate a hash for the given data storage struct */
void hash_data(DATA_STORAGE *data)
{
  byte hash = 0;
  for (unsigned int next_byte = 0; next_byte < data->bytes_stored; next_byte++)
  {
    byte index = next_byte;
    index += data->data[next_byte];
    hash = hash ^ index;
  }
  data->hash = hash;
}

void serial_print_int_array(int *array_data, unsigned int array_size, const char *title)
{
  MAKE_STRING(title);
  Serial.print(title_str);
  MAKE_STRING(LIST_SEPARATOR);
  for(int idx = 0; idx < array_size; idx++)
  {
    Serial.print(array_data[idx]);
    Serial.print(LIST_SEPARATOR_str);
  }
  Serial.println();
}
void serial_print_byte_array(byte *array_data, unsigned int array_size, const char *title)
{
  MAKE_STRING(title);
  Serial.print(title_str);
  MAKE_STRING(LIST_SEPARATOR);
  for(int idx = 0; idx < array_size; idx++)
  {
    Serial.print(array_data[idx]);
    Serial.print(LIST_SEPARATOR_str);
  }
  Serial.println();
}
void file_print_int_array(File *file, int *array_data, unsigned int array_size, const char *title)
{
  MAKE_STRING(title);
  file->print(title_str);
  MAKE_STRING(LIST_SEPARATOR);
  for(int idx = 0; idx < array_size; idx++)
  {
    file->print(array_data[idx]);
    file->print(LIST_SEPARATOR_str);
  }
  file->println();
}
void file_print_byte_array(File *file, byte *array_data, unsigned int array_size, const char *title)
{
  MAKE_STRING(title);
  file->print(title_str);
  MAKE_STRING(LIST_SEPARATOR);
  for(int idx = 0; idx < array_size; idx++)
  {
    file->print(array_data[idx]);
    file->print(LIST_SEPARATOR_str);
  }
  file->println();
}

/* reset the back record after use */
void reset_record()
{
  Data_Record[1-Data_Record_write_idx] = (DATA_RECORD){0};
  Data_Record[1-Data_Record_write_idx].timestamp = 0;//millis();
}


/* swap records and calculate averages */
void finalise_record()
{
  /* switch the records */
  Data_Record_write_idx = 1-Data_Record_write_idx; 
  
  /* data record to read */
  DATA_RECORD *data_record = &Data_Record[1-Data_Record_write_idx];
  
  
  /* calculate averages */
  if(data_record->ANA_no_of_samples > 0)
  {
    int rounding = data_record->ANA_no_of_samples/2;
    data_record->A0_avg += rounding;
    data_record->A1_avg += rounding;
    data_record->A2_avg += rounding;
    data_record->A3_avg += rounding;
    
    data_record->A0_avg /= data_record->ANA_no_of_samples;
    data_record->A1_avg /= data_record->ANA_no_of_samples;
    data_record->A2_avg /= data_record->ANA_no_of_samples;
    data_record->A3_avg /= data_record->ANA_no_of_samples;
  }
  if(data_record->EGT_no_of_samples > 0)
  {
    unsigned int rounding = data_record->EGT_no_of_samples/2;
    data_record->EGT_avg += rounding;
    data_record->EGT_avg /= data_record->EGT_no_of_samples;
  }
//  if(data_record->RPM_no_of_samples > 0)
//  {
    // for rpm, we can approximate by the number of ticks
    data_record->RPM_avg = data_record->RPM_no_of_ticks * 60000/UPDATE_INTERVAL_ms;
    // for a more acurate reading, we must sum all tick intervals
    // and divide by number of ticks
    // but this should be sufficient
    
    //unsigned int rounding = data_record->RPM_no_of_samples/2;
    //data_record->RPM_avg += rounding;
    //data_record->RPM_avg = (unsigned int)(60000*(unsigned long int)data_record->RPM_no_of_samples/data_record->RPM_avg);  
//  }

  /* hash the data for hex storage*/
  
  data_store.data = (byte*)data_record;
  data_store.bytes_stored = sizeof(DATA_STORAGE);
  hash_data(&data_store);
}


/* writes the current data record to serial port and SD card as required */
static byte write_data_step = 0;


bool write_serial_data_record()
{
  /* send the data to the serial port */
  if(flags.do_serial_write)
  {
    #ifdef DEBUG_SERIAL_TIME    
    unsigned int timestamp_us = micros();
    Serial.print(F("ser_stp: ")); Serial.println(write_data_step);
    #endif

    /* data record to read */
    DATA_RECORD *data_record = (DATA_RECORD *)data_store.data;//&Data_Record[1-Data_Record_write_idx];//
  
    if(flags.do_serial_write_hex)
    {
      write_data_step = 0;
      Serial.write((byte*)data_store.bytes_stored, sizeof(data_store.bytes_stored));    //write the number of bytes sent
      Serial.write(data_store.hash);                                              //write the hash value
      Serial.write(data_store.data, sizeof(data_store.bytes_stored));            //write the data
    }
    else
    {
      switch(write_data_step++)
      {
        case 0: 
          {
            Serial.println(F("----------"));
            MAKE_STRING(RECORD_VER_C);        Serial.print(RECORD_VER_C_str);     Serial.println(DATA_RECORD_VERSION);
            MAKE_STRING(TIMESTAMP_C);         Serial.print(TIMESTAMP_C_str);      Serial.println(data_record->timestamp);
          }
          break;
        case 1: 
          {
            Serial.print(F("A0_avg: "));      Serial.println(data_record->A0_avg);
            Serial.print(F("A1_avg: "));      Serial.println(data_record->A1_avg);
            Serial.print(F("A2_avg: "));      Serial.println(data_record->A2_avg);
            Serial.print(F("A3_avg: "));      Serial.println(data_record->A3_avg);
            Serial.print(F("ANA samples: ")); Serial.println(data_record->ANA_no_of_samples);
          }
          break;
        case 2:
            serial_print_int_array(data_record->A0, data_record->ANA_no_of_samples, A0_C);
            break;
        case 3:
            serial_print_int_array(data_record->A1, data_record->ANA_no_of_samples, A1_C);
            break;
        case 4:
            serial_print_int_array(data_record->A2, data_record->ANA_no_of_samples, A2_C);
            break;
        case 5:
            serial_print_int_array(data_record->A3, data_record->ANA_no_of_samples, A3_C);
            break;
        case 6:
          {
            Serial.print(F("EGT_avg:"));       Serial.println(data_record->EGT_avg);
            Serial.print(F("EGT samples: ")); Serial.println(data_record->EGT_no_of_samples);
            break;
          }
        case 7:
            serial_print_int_array(data_record->EGT, data_record->EGT_no_of_samples, EGT1_C);
            break;
        case 8:
          {
            MAKE_STRING(RPM_AVG);             Serial.print(RPM_AVG_str);          Serial.println(data_record->RPM_avg);
            MAKE_STRING(RPM_NO_OF_TICKS);     Serial.print(RPM_NO_OF_TICKS_str);  Serial.println(data_record->RPM_no_of_ticks);
            break;
          }
        case 9:
          {
            serial_print_byte_array(data_record->RPM_tick_times_ms, data_record->RPM_no_of_ticks, RPM_TICK_TIMES);
            Serial.println(F("----------"));
          }
        default:
          write_data_step = 0;
          break;
      }
    }
    #ifdef DEBUG_SERIAL_TIME
    timestamp_us = micros() - timestamp_us;
    Serial.print(F("t_ser us: "));
    Serial.println(timestamp_us);
    #endif
  }
  return write_data_step == 0;
}

bool write_sdcard_data_record()
{
  
  /* send the data to the SD card, if available */
  if(flags.do_sdcard_write && flags.sd_card_available)
  {
    #ifdef DEBUG_SDCARD_TIME    
    unsigned int timestamp_us = micros();
    Serial.print(F("sdc_stp: ")); Serial.println(write_data_step);
    #endif

    /* data record to read */
    DATA_RECORD *data_record = (DATA_RECORD *)data_store.data;//&Data_Record[1-Data_Record_write_idx];//

//    Serial.print(F("wds_addr"));
//    Serial.println((unsigned int)&data_store);
//    Serial.print(F("wdr_addr"));
//    Serial.println((unsigned int)&Data_Record[1-Data_Record_write_idx]);
//    Serial.print(F("wdr_saddr"));
//    Serial.println((unsigned int)data_store.data);
    
    static File log_data_file; //costs 35 bytes
    
    /* replace all these serial calls with SD card file calls */
    if(flags.do_sdcard_write_hex)
    {
      write_data_step = 0;
      log_data_file = SD.open(output_filename, O_WRITE | O_APPEND);
      if (log_data_file)
      {    
       
        log_data_file.write((byte*)data_store.bytes_stored, sizeof(data_store.bytes_stored));    //write the number of bytes sent
        log_data_file.write(data_store.hash);                                              //write the hash value
        log_data_file.write(data_store.data, sizeof(data_store.bytes_stored));            //write the data
        log_data_file.close();
      }
      
    }
    else
    {
      switch(write_data_step++)
      {
        case 0:
          {
            if (!log_data_file) 
            {
              log_data_file = SD.open(output_filename, O_WRITE | O_APPEND);
              if (!log_data_file)
              { 
                write_data_step = 0; //reset to 0 if unable to open
              }
            }
          } break;
        case 1:
          {
            log_data_file.println(F("----------"));
            MAKE_STRING(TIMESTAMP_C);                 log_data_file.print(TIMESTAMP_C_str);      log_data_file.println(data_record->timestamp);
          } break;
        case 2:
          {
            log_data_file.print(F("A0_avg: "));       log_data_file.println(data_record->A0_avg);
            log_data_file.print(F("A1_avg: "));       log_data_file.println(data_record->A1_avg);
          } break;
        case 3:
          {
            log_data_file.print(F("A2_avg: "));       log_data_file.println(data_record->A2_avg);
            log_data_file.print(F("A3_avg: "));       log_data_file.println(data_record->A3_avg);
          } break;
        case 4:
          {
            log_data_file.print(F("ANA samples: "));  log_data_file.println(data_record->ANA_no_of_samples);
          } break;
        case 5:
          {
            file_print_int_array(&log_data_file, data_record->A0, data_record->ANA_no_of_samples, A0_C);
          } break;
        case 6:
          {
            file_print_int_array(&log_data_file, data_record->A1, data_record->ANA_no_of_samples, A1_C);
          } break;
        case 7:
          {
            file_print_int_array(&log_data_file, data_record->A2, data_record->ANA_no_of_samples, A2_C);
          } break;
        case 8:
          {
            file_print_int_array(&log_data_file, data_record->A3, data_record->ANA_no_of_samples, A3_C);
          } break;
        case 9:
          {
            log_data_file.print(F("EGT_avg: "));        log_data_file.println(data_record->EGT_avg);
            log_data_file.print(F("EGT samples: "));  log_data_file.println(data_record->EGT_no_of_samples);
          } break;
        case 10:
          {
            file_print_int_array(&log_data_file, data_record->EGT, data_record->EGT_no_of_samples, EGT1_C);
          } break;
        case 11:
          {
            MAKE_STRING(RPM_AVG);                     log_data_file.print(RPM_AVG_str);          log_data_file.println(data_record->RPM_avg);
            MAKE_STRING(RPM_NO_OF_TICKS);             log_data_file.print(RPM_NO_OF_TICKS_str);  log_data_file.println(data_record->RPM_no_of_ticks);
          } break;
        case 12:
          {
            file_print_byte_array(&log_data_file, data_record->RPM_tick_times_ms, data_record->RPM_no_of_ticks, RPM_TICK_TIMES);
          } break;
        case 13:
          {
            log_data_file.close();
          }
        default:
          write_data_step = 0;
          break;
      }
      
    }

    #ifdef DEBUG_SDCARD_TIME
    timestamp_us = micros() - timestamp_us;
    Serial.print(F("t_sdc us: "));
    Serial.println(timestamp_us);
    #endif
  }
  return write_data_step == 0;
}
