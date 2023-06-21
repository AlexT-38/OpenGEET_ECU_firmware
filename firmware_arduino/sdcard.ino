char output_filename[13] = ""; //8+3 format
File dataFile;



void dateTime(uint16_t* date, uint16_t* time) {
 DateTime t_now = DS1307_now();
 // return date using FAT_DATE macro to format fields
 *date = FAT_DATE(t_now.year, t_now.month, t_now.day);

 // return time using FAT_TIME macro to format fields
 *time = FAT_TIME(t_now.hour, t_now.minute, t_now.second);
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
