/* generate a filename from the rtc date/time
 *  if more than 255 files are required in a day, we'll
 *  have to make use of sub folders (or increase the base of NN to 36)
 *  eg /001/YY_MM_DD.txt
 *  or /YYYMMDD/LOG_0001.txt
 *  meanwhile, we'll just overwrite older files
 */
 #define NO_IDX 0xff
 #define RECORD_BUFFER_SIZE 2048

/* the records to write to */
DATA_RECORD Data_Records[2];
/* the index of the currently written record */
byte Data_Record_write_idx = 0;

/* averages for all data over the update period */
DATA_AVERAGES Data_Averages;

/* list of numbers of parameter types in data record */
DATA_CONFIG Data_Config = {DATA_RECORD_VERSION, NO_OF_USER_INPUTS, NO_OF_MAP_SENSORS, NO_OF_TMP_SENSORS, NO_OF_EGT_SENSORS, NO_OF_SERVOS};


static DATA_STORAGE data_store;

char output_filename[13] = ""; //8+3 format

String dataBuffer;
unsigned int recordStart; //length of dataBuffer at time of adding record to buffer;

File dataFile;

unsigned int lastRecordSize = 0;
void configure_records()
{
  dataBuffer.reserve(RECORD_BUFFER_SIZE*2);
}

void create_file()
{
  static byte file_index = 0;
  char *str_ptr;

  int value;    //date value to be converted to string
  int count = 0;//count of tries to find non existant filename

  // set the file extension
  str_ptr = output_filename + 8;
  if(flags_config.do_sdcard_write_hex) {strcpy_P(str_ptr,S_DOT_RAW);}
  else                          {strcpy_P(str_ptr,S_DOT_TXT);}

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
    value = file_index++;  *str_ptr-- = HEX_CHAR(value);  value >>=4;  *str_ptr-- = HEX_CHAR(value);

    count++; // count how many indices we've tried
    //break the loop if the target file doenst exist, or we have run out of indices
    if (!SD.exists(output_filename) || count >= 256 )
    {
      do_loop = false;
    }
    
    else
    {
      #ifdef DEBUG_SDCARD
      Serial.print(F("SD File exists: "));
      Serial.println(output_filename);
      #endif
    }
    
  }

  //report the selected filename
  #ifdef DEBUG_SDCARD
  Serial.print(FS(S_OUTPUT_FILE_NAME_C));
  Serial.println(output_filename);
  #endif
  
  if(dataFile)
  {
    //the file shouldn't be open
    dataFile.close();

    #ifdef DEBUG_SDCARD
    Serial.println(F("SD Already open?"));
    #endif
  }
  
  //check that the file can be opened
  dataFile = SD.open(output_filename, O_WRITE | O_CREAT);
  
  if(dataFile)
  {
    
    flags_status.file_openable = true;
    #ifdef DEBUG_SDCARD
    Serial.println(F("SD Opened"));
    #endif
  }
  else
  {
    //open failed on first attempt, do not attempt to save to this file
    flags_status.file_openable = false;
    #ifdef DEBUG_SDCARD
    Serial.println(F("SD Open FAILED"));
    #endif
  }

}

void dateTime(uint16_t* date, uint16_t* time) {
 DateTime t_now = DS1307_now();
 // return date using FAT_DATE macro to format fields
 *date = FAT_DATE(t_now.year, t_now.month, t_now.day);

 // return time using FAT_TIME macro to format fields
 *time = FAT_TIME(t_now.hour, t_now.minute, t_now.second);
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

void string_print_int_array(String &dst, int *array_data, unsigned int array_size, const char *title_pgm, const byte idx)
{
  if(idx < 0xff) { dst += (idx);}
  dst += (FS(title_pgm));
  
  for(int idx = 0; idx < array_size; idx++)
  {
    dst += (array_data[idx]);
    dst += (FS(S_COMMA));
  }
  dst += "\n";
}
void string_print_byte_array(String &dst, byte *array_data, unsigned int array_size, const char *title_pgm, const byte idx)
{
  dst += (FS(title_pgm));
  if(idx < 0xff) { dst += " "; dst += (idx);}
  for(int idx = 0; idx < array_size; idx++)
  {
    dst += (array_data[idx]);
    dst += (FS(S_COMMA));
  }
  dst += "\n";
}



/* swap records, calculate averages, generate text report */
void update_record()
{
  #ifdef DEBUG_RECORD_TIME
  unsigned int timestamp_us = micros();
  #endif //DEBUG_RECORD_TIME
  
    
  /* prepare the new record */
  SWAP_RECORDS();
  CURRENT_RECORD = (DATA_RECORD){0};
  CURRENT_RECORD.timestamp = millis();
  
  /* get the previous record for processing */
  DATA_RECORD *data_record = &LAST_RECORD;

  /* calculate averages */

  /* analog samples */
  if(data_record->ANA_no_of_samples > 0)
  {
    int rounding = data_record->ANA_no_of_samples>>1;
    /* User input */
    for(byte idx = 0; idx<Data_Config.USR_no; idx++)
    {
      Data_Averages.USR[idx] = rounding;
      for(byte sample = 0; sample < data_record->ANA_no_of_samples; sample++) {Data_Averages.USR[idx] += data_record->USR[idx][sample];}
      
      Data_Averages.USR[idx] /= data_record->ANA_no_of_samples;

    }
    /* MAP sensors */
    for(byte idx = 0; idx<Data_Config.MAP_no; idx++)
    {
      Data_Averages.MAP[idx] = rounding;
      for(byte sample = 0; sample < data_record->ANA_no_of_samples; sample++) {Data_Averages.MAP[idx] += data_record->MAP[idx][sample];}
      Data_Averages.MAP[idx] /= data_record->ANA_no_of_samples;
    }
    /* Thermistors */
    for(byte idx = 0; idx<Data_Config.TMP_no; idx++)
    {
      Data_Averages.TMP[idx] = rounding;
      for(byte sample = 0; sample < data_record->ANA_no_of_samples; sample++) {Data_Averages.TMP[idx] += data_record->TMP[idx][sample];}
      Data_Averages.TMP[idx] /= data_record->ANA_no_of_samples;
    }
    /* Torque loadcell */
    Data_Averages.TRQ = rounding;
    for(byte sample = 0; sample < data_record->ANA_no_of_samples; sample++) {Data_Averages.TRQ += data_record->TRQ[sample];}
    Data_Averages.TRQ /= data_record->ANA_no_of_samples;
  
  }
  /* Thermocouples */
  if(data_record->EGT_no_of_samples > 0)
  {
    unsigned int rounding = data_record->EGT_no_of_samples>>1;
    for(byte idx = 0; idx<Data_Config.EGT_no; idx++)
    {
      Data_Averages.EGT[idx] = rounding;
      for(byte sample = 0; sample < data_record->EGT_no_of_samples; sample++) {Data_Averages.EGT[idx] += data_record->EGT[idx][sample];}
      Data_Averages.EGT[idx] /= data_record->EGT_no_of_samples;
    }
  }
  /* RPM counter */
  Data_Averages.RPM = get_rpm(data_record);

  /* Servos */
  // calculate the average servo demand from the pid
  if(data_record->SRV_no_of_samples > 0)
  {
    int rounding = data_record->SRV_no_of_samples>>1;
    for(byte idx = 0; idx<Data_Config.SRV_no; idx++)
    {
      Data_Averages.SRV[idx] = rounding;
      for(byte sample = 0; sample < data_record->SRV_no_of_samples; sample++) {Data_Averages.SRV[idx] += data_record->SRV[idx][sample];}
      Data_Averages.SRV[idx] /= data_record->SRV_no_of_samples;
    }
  }

  /* Shaft Power */

  //power in (W?) product of torque(Nm) and speed(rad/s); rad/s = 0.10471975511965977461542144610932 per/rpm, Nm = 0.001 mNm
  //power (W) = trq(mNm) * speed(rpm) * 0.000105

  // convert the float constant to an integer shift-multiply-shift
  #define POW_CALC_PRESCALE 10
  #define POW_CALC_POSTSCALE 15
  #define POW_CALC_CONSTANT 3514 // (int)(1.0471975511965977461542144610932e-4 * _BV(POW_CALC_PRESCALE + POW_CALC_POSTSCALE))
  
  long native_power = (long)Data_Averages.RPM * Data_Averages.TRQ;

  #ifndef DEBUG_POWER_CALC
  native_power += _BV(POW_CALC_PRESCALE-1);
  native_power >>= POW_CALC_PRESCALE;
  native_power *= POW_CALC_CONSTANT;
  native_power += _BV(POW_CALC_POSTSCALE-1);
  native_power >>= POW_CALC_POSTSCALE;

  #else //DEBUG_POWER_CALC
  Serial.println("power calc (no rounding)");
  Serial.print(F("RPM:  ")); Serial.println(Data_Averages.RPM);
  Serial.print(F("TRQ:  ")); Serial.println(Data_Averages.TRQ);
  Serial.print(F("RxT:  ")); Serial.println(native_power);
  native_power >>= POW_CALC_PRESCALE;
  Serial.print(F(">>10: ")); Serial.println(native_power);
  native_power *= POW_CALC_CONSTANT;
  Serial.print(F("*CON: ")); Serial.println(native_power);
  native_power >>= POW_CALC_POSTSCALE;
  Serial.print(F(">>15: ")); Serial.println(native_power);
  #endif //DEBUG_POWER_CALC

  Data_Averages.POW = (int)native_power;

  /* hash the data for hex storage*/
  
  data_store.data = (byte*)data_record;
  data_store.bytes_stored = sizeof(DATA_STORAGE);
  hash_data(&data_store);

  #ifdef DEBUG_BUFFER_TIME
  unsigned int timestamp_buffer_us = micros();
  #endif

  if(flags_status.logging_active)  recordStart = write_data_record_to_buffer(data_record, dataBuffer, recordStart);

  #ifdef DEBUG_BUFFER_TIME
  timestamp_buffer_us = micros() - timestamp_buffer_us;
  Serial.print(F("t_rec us: "));
  Serial.println(timestamp_buffer_us);
  #endif


  #ifdef DEBUG_RECORD_TIME
  timestamp_us = micros() - timestamp_us;
  Serial.print(F("t_rec us: "));
  Serial.println(timestamp_us);
  #endif //DEBUG_RECORD_TIME
}

unsigned int write_data_record_to_buffer(DATA_RECORD *data_record, String &dst, int prev_record_idx)
{
    //write the record to a temporary buffer, so that we can handle dst buffer overflows
    String buf;
    buf.reserve(RECORD_BUFFER_SIZE);

    buf += FS(S_RECORD_MARKER); buf += "\n";
    buf += FS(S_TIMESTAMP_C); buf += data_record->timestamp; buf += "\n";

    for(byte idx = 0; idx < Data_Config.USR_no; idx++)
    {
      buf += (idx); buf += (FS(S_USR_C));     buf += (Data_Averages.USR[idx]); buf += "\n";
    }
    for(byte idx = 0; idx < Data_Config.MAP_no; idx++)
    {
      buf += (idx); buf += (FS(S_MAP_C));     buf += (Data_Averages.MAP[idx]); buf += "\n";
    }
    for(byte idx = 0; idx < Data_Config.TMP_no; idx++)
    {
      buf += (idx); buf += (FS(S_TMP_C));     buf += (Data_Averages.TMP[idx]); buf += "\n";
    }
    buf += (FS(S_TRQ_C));     buf += (Data_Averages.TRQ); buf += "\n";
    buf += (FS(S_ANA_SAMPLES_C)); buf += (data_record->ANA_no_of_samples); buf += "\n";

    for(byte idx = 0; idx < Data_Config.USR_no; idx++)
    {
      string_print_int_array(buf, data_record->USR[idx], data_record->ANA_no_of_samples, S_USR_C, idx);
    }

    for(byte idx = 0; idx < Data_Config.MAP_no; idx++)
    {
      string_print_int_array(buf, data_record->MAP[idx], data_record->ANA_no_of_samples, S_MAP_C, idx);
    }

    for(byte idx = 0; idx < Data_Config.TMP_no; idx++)
    {
      string_print_int_array(buf, data_record->TMP[idx], data_record->ANA_no_of_samples, S_TMP_C, idx);
    }

    string_print_int_array(buf, data_record->TRQ, data_record->ANA_no_of_samples, S_TRQ_C, NO_IDX);


    for(byte idx = 0; idx < Data_Config.EGT_no; idx++)
    {
      buf += (idx); buf += (FS(S_EGT_AVG_C));       buf += (Data_Averages.EGT[idx]); buf += "\n";
    }

    buf += (FS(S_EGT_SAMPLES_C)); buf += (data_record->EGT_no_of_samples); buf += "\n";

    for(byte idx = 0; idx < Data_Config.EGT_no; idx++)
    {
      string_print_int_array(buf, data_record->EGT[idx], data_record->EGT_no_of_samples, S_EGT_C, idx);
    }

    buf += (FS(S_RPM_AVG));          buf += (Data_Averages.RPM); buf += "\n";
    buf += (FS(S_RPM_NO_OF_TICKS));  buf += (data_record->RPM_no_of_ticks); buf += "\n";
    buf += (FS(S_RPM_TICK_OFFSET));  buf += (data_record->RPM_tick_offset_ms); buf += "\n";

    string_print_int_array(buf, data_record->RPM_tick_times_ms, data_record->RPM_no_of_ticks, S_RPM_TICK_TIMES, NO_IDX);
    buf += (FS(S_POW_C));          buf += (Data_Averages.POW); buf += "\n";
    buf += (FS(S_RECORD_MARKER)); buf += "\n";

    int this_record_idx = dst.length();
    
    if( (this_record_idx + buf.length()) < RECORD_BUFFER_SIZE )
    {
      // add the new record to the output buffer
      dst += buf;
      
      #ifdef DEBUG_BUFFER
      Serial.println(F("BUF: Record appended"));
      #endif
    }
    else if( (prev_record_idx + buf.length()) < RECORD_BUFFER_SIZE )
    {
      //over write the previous record, if not enough space
      dst.remove(prev_record_idx, this_record_idx - prev_record_idx);
      dst += buf;
      this_record_idx = prev_record_idx;
      
      #ifdef DEBUG_BUFFER
      Serial.println(F("BUF: Record overwrite"));
      #endif
    }
    else 
    {
      #ifdef DEBUG_BUFFER
      Serial.println(F("BUF: Record dropped"));
      #endif
    } //otherwise drop this record
  #ifdef DEBUG_BUFFER
  Serial.print(F("BUF (this idx, this size, buf size): "));
  Serial.print(this_record_idx); Serial.print(", "); Serial.print(buf.length()); Serial.print(", "); Serial.println(dst.length());
  #endif
  return this_record_idx;
}



void write_serial_data_record()
{
  #ifdef DEBUG_SERIAL_TIME    
  unsigned int timestamp_us = micros();
  #endif
    
  /* send the data to the serial port */
  if(flags_status.logging_active && flags_config.do_serial_write)
  {
    /* data record to read */
    DATA_RECORD *data_record = (DATA_RECORD *)data_store.data;//&LAST_RECORD;//
  
    if(flags_config.do_serial_write_hex)
    {
      Serial.write((byte*)data_store.bytes_stored, sizeof(data_store.bytes_stored));    //write the number of bytes sent
      Serial.write(data_store.hash);                                              //write the hash value
      Serial.write(data_store.data, sizeof(data_store.bytes_stored));            //write the data
    }
    else
    {
      //write_data_record_to_stream(data_record, Serial);
      //check there's new data in the buffer
      if(dataBuffer.length() > recordStart)
      {
        Serial.write(&dataBuffer.c_str()[recordStart]);
      }
    }
  }

  #ifdef DEBUG_SERIAL_TIME
  timestamp_us = micros() - timestamp_us;
  Serial.print(F("t_ser us: "));    Serial.println(timestamp_us);
  #endif
    
  return;
}



void sync_sdcard_data_record()
{
  #ifdef DEBUG_SDCARD_TIME    
  unsigned int timestamp_us = micros();
  #endif
   
  if(flags_status.logging_active)
  {
    
    if(dataFile) 
    {
      dataFile.flush(); //sync the file - hopefully this will perform files system updates without blocking
      #ifdef DEBUG_SDCARD
      Serial.println(F("SD Flush"));
      #endif
    }
    else
    {
      create_file(); //creating a file could involve long writes, so this task is scheduled here instead of during read_touch()
    }
  }
  else if(dataFile)
  {
     // close the file if its open when not logging
    dataFile.close(); 
    #ifdef DEBUG_SDCARD
    Serial.println(F("SD Close"));
    #endif
  }

  #ifdef DEBUG_SDCARD_TIME
  timestamp_us = micros() - timestamp_us;
  Serial.print(F("t_sdf us: "));
  Serial.println(timestamp_us);
  #endif
}
/* writes a record to an sd card file 
 * returns true when complete
 * if it fails to write it will skip this record, but try again next time
 * we might be able to optimise here by checking for elapsed time,
 * and if time has exceeded some threshold, the file will close
 */
void write_sdcard_data_record()
{
  #ifdef DEBUG_SDCARD_TIME    
  unsigned int timestamp_us = micros();
  #endif

  //if the data file is open, we write to it
  if(dataFile)
  {
    /* data record to read */
    DATA_RECORD *data_record = (DATA_RECORD *)data_store.data;

    /* write in hex format */
    if(flags_config.do_sdcard_write_hex)
    {
      //TODO: this also need to be buffered - however, we might not have enough ram for two different buffers
      dataFile.write((byte*)data_store.bytes_stored, sizeof(data_store.bytes_stored));    //write the number of bytes sent
      dataFile.write(data_store.hash);                                              //write the hash value
      dataFile.write(data_store.data, sizeof(data_store.bytes_stored));            //write the data
    }
    /* write plain text format */
    else
    {
      #ifdef DEBUG_SDCARD
      unsigned int blocks_written = 0;
      static unsigned long total_blocks_written = 0;
      #endif

      #define BLOCK_SIZE 512
      while (dataBuffer.length() >= BLOCK_SIZE)
      {
        // write a single block of data
        if(dataFile.write(dataBuffer.c_str(), BLOCK_SIZE))
        {
          // remove written data from dataBuffer if write was sucessfull
          dataBuffer.remove(0, BLOCK_SIZE);
        }
        else
        {
          Serial.println(F("File write failed"));
          //if not logging to serial, stop logging
          if(!flags_config.do_serial_write)
          {
            flags_status.logging_active = false;
          }
        }
        
        #ifdef DEBUG_SDCARD
        blocks_written++;
        #endif
      }

      #ifdef DEBUG_SDCARD
      total_blocks_written += blocks_written;
      Serial.print(F("SD blocks written: "));        Serial.println(blocks_written);
      Serial.print(F("SD total blocks: "));        Serial.println(total_blocks_written);
      #endif
    } // format selection
  }

  #ifdef DEBUG_SDCARD_TIME
  timestamp_us = micros() - timestamp_us;
  Serial.print(F("t_sdc us: "));
  Serial.println(timestamp_us);
  #endif

  return;
}
