/* generate a filename from the rtc date/time
 *  if more than 255 files are required in a day, we'll
 *  have to make use of sub folders (or increase the base of NN to 36)
 *  eg /001/YY_MM_DD.txt
 *  or /YYYMMDD/LOG_0001.txt
 *  meanwhile, we'll just overwrite older files
 */
 #define NO_IDX 0xff

/* the records to write to */
DATA_RECORD Data_Records[2];
/* the index of the currently written record */
byte Data_Record_write_idx = 0;

/* averages for all data over the update period */
DATA_AVERAGES Data_Averages;

/* list of numbers of parameter types in data record */
DATA_CONFIG Data_Config = {DATA_RECORD_VERSION, NO_OF_USER_INPUTS, NO_OF_MAP_SENSORS, NO_OF_TMP_SENSORS, NO_OF_EGT_SENSORS, NO_OF_SERVOS};


static DATA_STORAGE data_store;

void generate_file_name()
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
      Serial.print(F("File exists: "));
      Serial.println(output_filename);
    }
  }
  //report the selected filename
  Serial.print(FS(S_OUTPUT_FILE_NAME_C));
  Serial.println(output_filename);
  
  //check that the file can be opened
  File dataFile = SD.open(output_filename, FILE_WRITE);
  flags_status.file_openable = true;
  if(dataFile)
  {
    //print the header
    dataFile.print(FS(S_RECORD_VER_C));     dataFile.println(DATA_RECORD_VERSION);
    stream_print_date_time(t_now, dataFile);
    dataFile.println();
    dataFile.close();
    
    Serial.println(F("Opened OK"));
  }
  else
  {
    //open failed on first attempt, do not attempt to save to this file
    Serial.println(F("Open FAILED"));
    flags_status.file_openable = false;
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

void stream_print_int_array(Stream *file, int *array_data, unsigned int array_size, const char *title_pgm, const byte idx)
{
  if(idx < 0xff) file->print(idx);
  file->print(FS(title_pgm));
  for(int idx = 0; idx < array_size; idx++)
  {
    file->print(array_data[idx]);
    file->print(FS(S_COMMA));
  }
  file->println();
}
void stream_print_byte_array(Stream *file, byte *array_data, unsigned int array_size, const char *title_pgm, const byte idx)
{
  if(idx < 0xff) file->print(idx);
  file->print(FS(title_pgm));
  for(int idx = 0; idx < array_size; idx++)
  {
    file->print(array_data[idx]);
    file->print(FS(S_COMMA));
  }
  file->println();
}

/* reset the back record after use */
void reset_record()
{
  LAST_RECORD = (DATA_RECORD){0};
  LAST_RECORD.timestamp = millis();
}


/* swap records and calculate averages */
void finalise_record()
{
  /* switch the records */
  Data_Record_write_idx = 1-Data_Record_write_idx; 
  
  /* data record to read */
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
  //keep elapsed time in a sensible range to avoid overflows
  //rpm_clip_time();
  // get the average rpm for this record 
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

  //calculate shaft power

  //this calculation is wrong
  //the correct formula is Power (kW) = Torque (N.m) x Speed (RPM) / 9.5488
  
  //power in (W?) product of torque(Nm) and rpm(rad/s); rad/s = 0.10471975511965977461542144610932 per/rpm, Nm = 0.001 mNm
  //rpm will have range ~150-450rad/s, torque in range +/- ~10Nm, result would be +/- ~4500W
  //native range 4500(13bit) and 20000(15bit)
  //native power units range +/- ~45,000,000 (27bits)
  //convert to W (0.1W) multiply by 0.000105, range +/- ~4,712 (13bit)
  //multiply by 224,884 (18bit) div by 2^31, intermediate is 45bit
  //prescale by 8bit, power range now 19bit, intermediate is 37bit
  //reduce multiplier by 5 bit, 7072 (13bit) div by 2^(26-8=18), intermediate is 32bit, result is 14bit
  
  //so max signed value is 45,000,000, >>8 =175,781, *7072 =1,243,123,232, >>18 = 4742W
  //probably best to split the difference, 19/13 to 16/16
  //prescale by 11bit, multiplier reduced by 2bit to 56,221, div by 2^(26-11=15) --wrong, out by 3 bits

  #define POW_CALC_PRESCALE 10
  #define POW_CALC_POSTSCALE 15
  //this define doesn't seem to evaluate at compile time
  //#define POW_CALC_CONSTANT (long)(1.0471975511965977461542144610932e-4 * (float)_BV(POW_CALC_PRESCALE + POW_CALC_POSTSCALE))
  #define POW_CALC_CONSTANT 3514
  
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
}


/* writes the current data record to serial port and SD card as required */
static byte write_data_step = 0;

bool write_data_record_to_stream(DATA_RECORD *data_record, Stream &dst, byte write_data_step)
{
  bool keep_going = true;
  switch(write_data_step)
  {
    case 0: //ser: 712/1128
        {
          dst.println(FS(S_RECORD_MARKER));
          dst.print(FS(S_TIMESTAMP_C));     dst.println(data_record->timestamp);
        }
        break;
        
    case 1: //ser: 
        for(byte idx = 0; idx < Data_Config.USR_no; idx++)
        {
          dst.print(idx); dst.print(FS(S_USR_C));     dst.println(Data_Averages.USR[idx]);
        }
        for(byte idx = 0; idx < Data_Config.MAP_no; idx++)
        {
          dst.print(idx); dst.print(FS(S_MAP_C));     dst.println(Data_Averages.MAP[idx]);
        }
        for(byte idx = 0; idx < Data_Config.TMP_no; idx++)
        {
          dst.print(idx); dst.print(FS(S_TMP_C));     dst.println(Data_Averages.TMP[idx]);
        }
        dst.print(FS(S_TRQ_C));     dst.println(Data_Averages.TRQ);
        dst.print(FS(S_ANA_SAMPLES_C)); dst.println(data_record->ANA_no_of_samples);
        break;
        
    case 2: //ser: 
        for(byte idx = 0; idx < Data_Config.USR_no; idx++)
        {
          stream_print_int_array(&dst, data_record->USR[idx], data_record->ANA_no_of_samples, S_USR_C, idx);
        }
        break;
        
    case 3: //ser: 
        for(byte idx = 0; idx < Data_Config.MAP_no; idx++)
        {
          stream_print_int_array(&dst, data_record->MAP[idx], data_record->ANA_no_of_samples, S_MAP_C, idx);
        }
        break;
        
    case 4: //ser: 
        for(byte idx = 0; idx < Data_Config.TMP_no; idx++)
        {
          stream_print_int_array(&dst, data_record->TMP[idx], data_record->ANA_no_of_samples, S_TMP_C, idx);
        }
        break;
        
    case 5: //ser: 
        stream_print_int_array(&dst, data_record->TRQ, data_record->ANA_no_of_samples, S_TRQ_C, NO_IDX);

        break;
    case 6: //ser: 
        for(byte idx = 0; idx < Data_Config.EGT_no; idx++)
        {
          dst.print(idx); dst.print(FS(S_EGT_AVG_C));       dst.println(Data_Averages.EGT[idx]);
        }
        break;
        dst.print(FS(S_EGT_SAMPLES_C)); dst.println(data_record->EGT_no_of_samples);
    case 7: //ser: 
        for(byte idx = 0; idx < Data_Config.EGT_no; idx++)
        {
          stream_print_int_array(&dst, data_record->EGT[idx], data_record->EGT_no_of_samples, S_EGT_C, idx);
        }
        break;
    case 8: //ser: 588/1008
      {
        dst.print(FS(S_RPM_AVG));          dst.println(Data_Averages.RPM);
        dst.print(FS(S_RPM_NO_OF_TICKS));  dst.println(data_record->RPM_no_of_ticks);
        dst.print(FS(S_RPM_TICK_OFFSET));  dst.println(data_record->RPM_tick_offset_ms);
        break;
      }
    case 9: //ser: 540/964
      {
        stream_print_int_array(&dst, data_record->RPM_tick_times_ms, data_record->RPM_no_of_ticks, S_RPM_TICK_TIMES, NO_IDX);
        dst.print(FS(S_POW_C));          dst.println(Data_Averages.POW);
        dst.println(FS(S_RECORD_MARKER));
      }
    default:
      keep_going = false;
      break;
      
  }
  
  return keep_going;
}
bool write_serial_data_record()
{
  /* send the data to the serial port */
  if(flags_status.logging_active && flags_config.do_serial_write)
  {
    #ifdef DEBUG_SERIAL_TIME    
    unsigned int timestamp_us = micros();
    Serial.print(F("ser_stp: ")); Serial.println(write_data_step);
    #endif

    /* data record to read */
    DATA_RECORD *data_record = (DATA_RECORD *)data_store.data;//&LAST_RECORD;//
  
    if(flags_config.do_serial_write_hex)
    {
      write_data_step = 0;
      Serial.write((byte*)data_store.bytes_stored, sizeof(data_store.bytes_stored));    //write the number of bytes sent
      Serial.write(data_store.hash);                                              //write the hash value
      Serial.write(data_store.data, sizeof(data_store.bytes_stored));            //write the data
    }
    else
    {
      if (!write_data_record_to_stream(data_record, Serial, write_data_step++))
      {
        write_data_step = 0;
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

char output_filename[13] = ""; //8+3 format

/* writes a record to an sd card file 
 * returns true when complete
 * if it fails to write it will skip this record, but try again next time
 * we might be able to optimise here by checking for elapsed time,
 * and if time has exceeded some threshold, the file will close
 */
bool write_sdcard_data_record()
{
  File log_data_file;
  
  /* send the data to the SD card, if available */
  if(flags_status.logging_active && flags_config.do_sdcard_write && flags_status.sdcard_available && flags_status.file_openable)
  {
    #ifdef DEBUG_SDCARD_TIME    
    unsigned int timestamp_us = micros();
    #endif

    /* data record to read */
    DATA_RECORD *data_record = (DATA_RECORD *)data_store.data;

    
    /* replace all these serial calls with SD card file calls */
    if(flags_config.do_sdcard_write_hex)
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
      else
      {
//        flags_status.file_openable = false;
        Serial.print(F("failed to open: "));
        Serial.println(output_filename);
      }
    }
    else
    {
      write_data_step = 0;
      if (!log_data_file) 
      {
        log_data_file = SD.open(output_filename, O_WRITE | O_APPEND);
      }
      if (!log_data_file)
      { 
        Serial.print(F("failed to open: "));
        Serial.println(output_filename);
      }
      else
      {
        while (write_data_record_to_stream(data_record, log_data_file, write_data_step++));
        write_data_step = 0;
        log_data_file.close();
      }
    }

    #ifdef DEBUG_SDCARD_TIME
    timestamp_us = micros() - timestamp_us;
    Serial.print(F("t_sdc us: "));
    Serial.println(timestamp_us);
    #endif
  }
  else if(log_data_file)
  {
    // close the file if its open, but we're not logging
    log_data_file.close();
  }

  return write_data_step == 0;
}

void stop_logging()
{
  
}

void start_logging()
{
  
}
