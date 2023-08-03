
 #define NO_IDX 0xff
 #define RECORD_RECORD_SIZE 1500
 #define RECORD_BUFFER_SIZE (2*RECORD_RECORD_SIZE)

/* the records to write to */
DATA_RECORD Data_Records[2];
/* the index of the currently written record */
byte Data_Record_write_idx = 0;

/* averages for all data over the update period */
DATA_AVERAGES Data_Averages;

/* list of numbers of parameter types in data record */
//this might work better as a bit mask for the channels that are enabled
DATA_CONFIG Data_Config = {DATA_RECORD_VERSION, NO_OF_USER_INPUTS, NO_OF_MAP_SENSORS, NO_OF_TMP_SENSORS, NO_OF_EGT_SENSORS, NO_OF_SERVOS, NO_OF_PIDS};


static DATA_STORAGE data_store;


String dataBuffer;
unsigned int recordStart; //length of dataBuffer at time of adding record to buffer;


unsigned int lastRecordSize = 0;
void configure_records()
{
  dataBuffer.reserve(RECORD_BUFFER_SIZE);
}


/* UI calls to start or stop logging
 *  defers actual state changes till log_state_update()
 */
void toggle_logging()
{
  flags_status.logging_requested ^= 1;
  
  #ifdef DEBUG_LOG_STATE
  if (flags_status.logging_requested)
    Serial.println(F("log start requested"));
  else
    Serial.println(F("log stop requested"));
  #endif
}

/* called every record update, after procesing data, but before stringifying
 * handles the process of changing logging state, logging state should only change here
 * depending on request state, current log state, log output config, datafile access, databuffer length
 */
void log_state_update()
{
  switch (flags_status.logging_state)
  {
    case LOG_STOPPED:
      if(flags_status.logging_requested) 
      {
        flags_status.logging_state = LOG_STARTING;
        #ifdef DEBUG_LOG_STATE
        Serial.println(F("log starting"));
        #endif
      }
      break;
    case LOG_STARTED:
      if(!flags_status.logging_requested) 
      {
        flags_status.logging_state = LOG_STOPPING;
        #ifdef DEBUG_LOG_STATE
        Serial.println(F("log stopping"));
        #endif
      }
      break;
    case LOG_STARTING:
      
      if(flags_config.do_sdcard_write && dataFile && flags_status.file_openable)
      { //move to the started state if logging to sd card and the file is open and openable
        flags_status.logging_state = LOG_STARTED;
        #ifdef DEBUG_LOG_STATE
        Serial.println(F("log started (sd)"));
        #endif
      }
      else if(flags_config.do_serial_write)
      { //otherwise, move to the started state if serial logging is enabled
        flags_status.logging_state = LOG_STARTED;
        #ifdef DEBUG_LOG_STATE
        Serial.println(F("log started (com)"));
        #endif
      }
      else
      { //otherwise stop logging
        flags_status.logging_state = LOG_STOPPED;
        #ifdef DEBUG_LOG_STATE
        Serial.println(F("log start failed"));
        #endif
      }
      break;

    case LOG_STOPPING:
      if(!dataBuffer.length())
      {
        flags_status.logging_state = LOG_STOPPED;
        #ifdef DEBUG_LOG_STATE
        Serial.println(F("log stopped"));
        #endif
      }
      break;
  }
  
}



/* swap records, calculate averages, generate text report */
void update_record()
{
  #ifdef DEBUG_RECORD_TIME
  unsigned int timestamp_us = micros();
  #endif //DEBUG_RECORD_TIME
  
    
  
  //disable RPM and ADC interrupts, ADC first because it will wait for any ongoing conversions to finish
  ADC_stop_fast();
  RPM_INT_DIS();

  /* prepare the new record */
  SWAP_RECORDS();
  CURRENT_RECORD = (DATA_RECORD){0};
  CURRENT_RECORD.timestamp = millis();
  CURRENT_RECORD.RPM_tick_offset_tk = RPM_counter;

  // reenable the RPM and ADC interrupts
  RPM_INT_EN();
  ADC_start_fast();
  
  /* get the previous record for processing */
  DATA_RECORD *data_record = &LAST_RECORD;

  /* calculate averages */

  /* analog samples */
  if(data_record->ANA_no_of_slow_samples > 0)
  {
    int rounding = data_record->ANA_no_of_slow_samples>>1;
    /* User input */
    for(byte idx = 0; idx<Data_Config.USR_no; idx++)
    {
      Data_Averages.USR[idx] = rounding;
      for(byte sample = 0; sample < data_record->ANA_no_of_slow_samples; sample++) {Data_Averages.USR[idx] += data_record->USR[idx][sample];}
      
      Data_Averages.USR[idx] /= data_record->ANA_no_of_slow_samples;

    }
    /* Thermistors */
    for(byte idx = 0; idx<Data_Config.TMP_no; idx++)
    {
      Data_Averages.TMP[idx] = rounding;
      for(byte sample = 0; sample < data_record->ANA_no_of_slow_samples; sample++) {Data_Averages.TMP[idx] += data_record->TMP[idx][sample];}
      Data_Averages.TMP[idx] /= data_record->ANA_no_of_slow_samples;
    }
    /* Torque loadcell */
    Data_Averages.TRQ = rounding;
    for(byte sample = 0; sample < data_record->ANA_no_of_slow_samples; sample++) {Data_Averages.TRQ += data_record->TRQ[sample];}
    Data_Averages.TRQ /= data_record->ANA_no_of_slow_samples;
  
  }
  
  /* MAP sensors, also perform calibration here */
  if(data_record->ANA_no_of_fast_samples > 0)
  {
    #ifdef DEBUG_ADC_FAST
    Serial.print("\nfast samples: "); Serial.println(data_record->ANA_no_of_fast_samples);
    #endif
    int rounding = data_record->ANA_no_of_fast_samples>>1;
    
    for(byte idx = 0; idx<Data_Config.MAP_no; idx++)
    {
      //maximum sum is greater than INT16_MAX, so we must sum to a long int first.
      long map_average = rounding;
      for(byte sample = 0; sample < data_record->ANA_no_of_fast_samples; sample++) 
      {
        
        data_record->MAP[idx][sample] = ADC_apply_map_calibration(idx, data_record->MAP[idx][sample]);
        //sum
        map_average += data_record->MAP[idx][sample];
      }
      //find average and store
      Data_Averages.MAP[idx] = (int)(map_average / data_record->ANA_no_of_fast_samples);
    }
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

  log_state_update();

  // convert data to string report if logging and either sdcard or serial is configured for output and in text format
  if(flags_status.logging_state && ( (!flags_config.do_serial_write_hex && flags_config.do_serial_write) || (!flags_config.do_sdcard_write_hex && flags_config.do_sdcard_write) ))
  {
    recordStart = write_data_record_to_buffer(data_record, dataBuffer, recordStart);
  }

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
    buf.reserve(RECORD_RECORD_SIZE);

    switch (flags_status.logging_state)
    {
        case LOG_STARTING:
          start_log(buf);
          break;
        case LOG_STARTED:
          write_record(buf, data_record);
          break;
        case LOG_STOPPING:
          finish_log(buf);
          break;
    }
    #ifdef DEBUG_RECORD
    Serial.print(F("buf.len: ")); Serial.println(buf.length());
    #endif
      

    ///////////////////////////////////////////////////////////////////////////////////

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


void log_io_info( String &dst, const __FlashStringHelper * name_pgm, byte number, int rate, const __FlashStringHelper * units, const __FlashStringHelper * rate_units)
{
    if(name_pgm)             json_append(dst, F("name"), name_pgm);
    if(number > 1)           json_append(dst, F("num"), number);
    if(rate)                 json_append(dst, F("rate"), rate);
    if(rate_units)           json_append(dst, F("rate_units"), rate_units);
    if(units)                json_append(dst, F("units"), units);

}
void log_io_info( String &dst, const __FlashStringHelper * name_pgm, byte number, int rate, const __FlashStringHelper * units, const __FlashStringHelper * rate_units,
                  int low, int high, char scale)
{
  log_io_info(dst, name_pgm, number, rate, units, rate_units);
  json_append(dst, F("min"), low);
  json_append(dst, F("max"), high);
  if(scale)
  {
    json_append(dst, F("scale"), scale); //scale is power of 2 multiplier, ie negative for fractional values, eg -2 gives steps of 0.25 units
  }
}
/* write the log header */
void start_log(String &dst)
{
  #ifdef DEBUG_RECORD
  Serial.println(F("log wr head"));
  #endif
  
  json_wr_start(dst);
  
  json_append_obj(dst, F("header"));
  
    //basic record info
    json_append(dst, F("version"), JSON_RECORD_VERSION);
    json_append(dst, F("firmware"), FS(S_FIRMWARE_NAME));
    json_append(dst, F("rate_ms"), UPDATE_INTERVAL_ms);
  
    DateTime t_now = DS1307_now();
    char str[11];
    date_to_string(t_now, str);
    json_append(dst, F("date"), str);
    time_to_string(t_now, str);
    json_append(dst, F("time"), str);

    //info about the sensors being recorded
    
    json_append_arr(dst, F("sensors"));
    
      // user inputs
      if(Data_Config.USR_no > 0)
      {
        json_append_obj(dst);
          log_io_info(dst, F("usr"), Data_Config.USR_no, ANALOG_SAMPLE_INTERVAL_ms, F("LSB"), F("ms"),   0, 1023, 0);
        json_append_close_object(dst);
      }
      
      // pressure sensors
      if(Data_Config.MAP_no > 0)
      {
        json_append_obj(dst);
          log_io_info(dst, F("map"), Data_Config.MAP_no, ADC_SAMPLE_INTERVAL_ms, F("mbar"), F("ms"));
        json_append_close_object(dst); //map
      }

      // thermocouple sensors
      if(Data_Config.EGT_no > 0)
      {
        json_append_obj(dst);
          log_io_info(dst, F("egt"), Data_Config.EGT_no, EGT_SAMPLE_INTERVAL_ms, F("°C"), F("ms"), 0, 1024, -2);
        json_append_close_object(dst); //egt
      }

      // thermistor sensors
      if(Data_Config.TMP_no > 0)
      {
        json_append_obj(dst);
          log_io_info(dst, F("tmp"), Data_Config.TMP_no, ANALOG_SAMPLE_INTERVAL_ms, F("°C"), F("ms"), 0, 256, -2);
        json_append_close_object(dst); //tmp
      }

      //torque sensor
      json_append_obj(dst);
        log_io_info(dst, F("trq"), 1, ANALOG_SAMPLE_INTERVAL_ms, F("mN.m"), F("ms"), -32000, 32000, 0);
      json_append_close_object(dst); //trq
      
      //rotation time sensor
      json_append_obj(dst);
        log_io_info(dst, F("spd"), 1, 1, F("us"), F("pr"), RPM_MAX_tk, RPM_MIN_tk, 4);
      json_append_close_object(dst); //spd

      //derived rpm
      json_append_obj(dst);
        log_io_info(dst, F("rpm"), 1, 0, F("rpm"), NULL); //rate of zero (or 1?) and no rate units -> avg per record
      json_append_close_object(dst); //rpm

      //derived power
      json_append_obj(dst);
        log_io_info(dst, F("pow"), 1, 0, F("W"), NULL);
      json_append_close_object(dst); //pow

    json_append_close_array(dst); ////////////////////////////////////////////////////// end of sensors
    
    json_append_obj(dst, F("servos"));
      //common parameters
      log_io_info(dst, NULL, Data_Config.SRV_no, PID_UPDATE_INTERVAL_ms, F("LSB"), F("ms"), 0, 1023, 0);

      json_append_arr(dst, F("names"));
        json_add(dst, F("throttle"));
        json_wr_list(dst);
        json_add(dst, F("mixture"));
        json_wr_list(dst);
        json_add(dst, F("brake"));
      json_close_array(dst);
    json_append_close_object(dst);

    json_append_obj(dst, F("pid"));
      log_io_info(dst, NULL, Data_Config.PID_no, PID_UPDATE_INTERVAL_ms, NULL, F("ms"));
      json_append(dst, F("k_frac"), PID_FP_FRAC_BITS);
      
      json_append_arr(dst, F("configs"));
        json_append_obj(dst);
          json_append(dst, F("name"), F("rpm"));
          json_append(dst, F("src"), F("spd"));
          json_append(dst, F("dst"), F("throttle"));
          json_append(dst, F("kp"), PIDs[PID_RPM].k.p);
          json_append(dst, F("ki"), PIDs[PID_RPM].k.i);
          json_append(dst, F("kd"), PIDs[PID_RPM].k.d);
        json_append_close_object(dst); //pid_rpm
        json_append_obj(dst);
          json_append(dst, F("name"), F("vac"));
          json_append(dst, F("src"), F("map"));
          json_append(dst, F("idx"), 0);
          json_append(dst, F("dst"), F("mixture"));
          json_append(dst, F("kp"), PIDs[PID_VAC].k.p);
          json_append(dst, F("ki"), PIDs[PID_VAC].k.i);
          json_append(dst, F("kd"), PIDs[PID_VAC].k.d);
        json_append_close_object(dst); //pid_rpm
      json_append_close_array(dst); //configs
      
    json_append_close_object(dst); //pid
    
  json_append_close_object(dst); //header
  
  json_append_arr(dst, F("records"));
}
/* write the log footer */
void finish_log(String &dst)
{
  #ifdef DEBUG_RECORD
  Serial.println(F("log wr tail"));
  #endif
  
  json_append_close_array(dst);
  json_append_close_object(dst);
}

/* write a record into the "records" array */
void write_record(String &dst, DATA_RECORD *data_record)
{
  #ifdef DEBUG_RECORD
  Serial.println(F("log wr rec"));
  #endif

  byte depth = json_depth;
  
  json_append_obj(dst);
  //add this records members here
  
    json_append(dst, F("timestamp"), data_record->timestamp);

////////////////////////////////////////////////////////////////////////// report user inputs
    if(Data_Config.USR_no > 0)
    {
      json_append_arr(dst, F("usr"));
      for(byte idx = 0; idx < Data_Config.USR_no; idx++ )
      {
        json_append_arr(dst, data_record->USR[idx], data_record->ANA_no_of_slow_samples);
      }
      json_append_close_array(dst);  
    }
////////////////////////////////////////////////////////////////////////// report MAP sensors    
    if(Data_Config.MAP_no > 0)
    {
      json_append_arr(dst, F("map"));
      for(byte idx = 0; idx < Data_Config.MAP_no; idx++ )
      {
        json_append_arr(dst, data_record->MAP[idx], data_record->ANA_no_of_fast_samples);
      }
      json_append_close_array(dst);
    }
////////////////////////////////////////////////////////////////////////// report thermistors
    if(Data_Config.TMP_no > 0)
    {
      json_append_arr(dst, F("tmp"));
      for(byte idx = 0; idx < Data_Config.TMP_no; idx++ )
      {
        json_append_arr(dst, data_record->TMP[idx], data_record->ANA_no_of_slow_samples);
      }
      json_append_close_array(dst);
    }
////////////////////////////////////////////////////////////////////////// report thermocouples
    if(Data_Config.EGT_no > 0)
    {
      json_append_arr(dst, F("egt"));
      for(byte idx = 0; idx < Data_Config.EGT_no; idx++ )
      {
        json_append_arr(dst, data_record->EGT[idx], data_record->EGT_no_of_samples);
      }
      json_append_close_array(dst);
    }
////////////////////////////////////////////////////////////////////////// report toque and speed
    json_append_arr(dst, F("trq"), data_record->TRQ, data_record->ANA_no_of_slow_samples);
    
    json_append(dst, F("spd_t0"), data_record->RPM_tick_offset_tk);
    json_append_arr(dst, F("spd"), data_record->RPM_tick_times_tk, data_record->RPM_no_of_ticks);

////////////////////////////////////////////////////////////////////////// report servo outputs
    if(Data_Config.SRV_no > 0)
    {
      json_append_arr(dst, F("srv"));
      for(byte idx = 0; idx < Data_Config.SRV_no; idx++ )
      {
        json_append_arr(dst, data_record->SRV[idx], data_record->SRV_no_of_samples);
      }
      json_append_close_array(dst);
    }
////////////////////////////////////////////////////////////////////////// report pid states
    if(Data_Config.PID_no > 0)
    {
      json_append_arr(dst, F("pid"));
      for(byte idx = 0; idx < Data_Config.PID_no; idx++ )
      {
        json_append_obj(dst);
          //could do with a mask to set which fields are reported
          json_append_arr(dst, F("trg"), data_record->PIDs[idx].target, data_record->SRV_no_of_samples);
          json_append_arr(dst, F("act"), data_record->PIDs[idx].actual, data_record->SRV_no_of_samples);
          json_append_arr(dst, F("err"), data_record->PIDs[idx].err, data_record->SRV_no_of_samples);
          json_append_arr(dst, F("out"), data_record->PIDs[idx].output, data_record->SRV_no_of_samples);
          json_append_arr(dst, F("p"), data_record->PIDs[idx].p, data_record->SRV_no_of_samples);
          json_append_arr(dst, F("i"), data_record->PIDs[idx].i, data_record->SRV_no_of_samples);
          json_append_arr(dst, F("d"), data_record->PIDs[idx].d, data_record->SRV_no_of_samples);
        json_append_close_object(dst);
      }
      json_append_close_array(dst);
    }

////////////////////////////////////////////////////////////////////////// report averages and other 1ce per record values
    json_append_obj(dst, F("avg"));

      if(Data_Config.USR_no > 0)
      {
          json_append_arr(dst, F("usr"), Data_Averages.USR, Data_Config.USR_no);
      }
      if(Data_Config.MAP_no > 0)
      {
          json_append_arr(dst, F("map"), Data_Averages.MAP, Data_Config.MAP_no);
      }
      if(Data_Config.TMP_no > 0)
      {
          json_append_arr(dst, F("tmp"), Data_Averages.TMP, Data_Config.TMP_no);
      }
      if(Data_Config.EGT_no > 0)
      {
          json_append_arr(dst, F("egt"), Data_Averages.EGT, Data_Config.EGT_no);
      }
      json_append(dst, F("trq"), Data_Averages.TRQ);
      json_append(dst, F("rpm"), Data_Averages.RPM);
      json_append(dst, F("pow"), Data_Averages.POW);
      if(Data_Config.SRV_no > 0)
      {
          json_append_arr(dst, F("srv"), Data_Averages.SRV, Data_Config.SRV_no);
      }
      //report latest PID coeffs
      if(Data_Config.PID_no > 0)
      {
          json_append_arr(dst, F("pid"));
          for(byte idx = 0; idx < Data_Config.PID_no; idx++)
          {
            json_append_obj(dst);
              json_add(dst, F("kp"),PIDs[idx].k.p);
              json_add(dst, F("ki"),PIDs[idx].k.i);
              json_add(dst, F("kd"),PIDs[idx].k.d);
            json_close_object(dst);
          }
          json_close_array(dst);
            
      }


    json_append_close_object(dst);
    
  

  //complete the record
  json_append_close_object(dst); 

  if(json_depth != depth)
  {
    Serial.println(F("Record Error, JSON depth wrong"));
    flags_status.logging_requested = false;
  }
}
