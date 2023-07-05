
void write_serial_data_record()
{
  #ifdef DEBUG_SERIAL_TIME    
  unsigned int timestamp_us = micros();
  #endif
    
  /* send the data to the serial port */
  if(flags_status.logging_state && flags_config.do_serial_write)
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
      // buffer will be cleared in the next sdcard update call
    }
  }

  #ifdef DEBUG_SERIAL_TIME
  timestamp_us = micros() - timestamp_us;
  Serial.print(F("t_ser us: "));    Serial.println(timestamp_us);
  #endif
    
  return;
}
