#define BLOCK_SIZE 512


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

  
  if(flags_status.logging_state && flags_config.do_sdcard_write )
  {
    if(dataFile && flags_status.file_openable) 
    {
      dataFile.flush(); //sync the file - hopefully this will perform files system updates without blocking
      #ifdef DEBUG_SDCARD
      Serial.println(F("SD Flush"));
      #endif
    }
    else if (flags_status.logging_state == LOG_STARTING)
    {
      create_file(); //creating a file could involve long writes, so this task is scheduled here instead of during read_touch()
    }
    else
    {
      flags_status.file_openable = false;
      Serial.println(F("no file open while logging"));
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
/* writes data to an sd card file 
 * if it fails to write it close the file, and if not logging to serial, stop logging.
 * if writing is inactive or fails, the buffer is cleared
 */
void write_sdcard_data_record()
{
  #ifdef DEBUG_SDCARD_TIME    
  unsigned int timestamp_us = micros();
  #endif

  bool clear_buffer = true;
  
  //first check if we're logging to sd card
  if(flags_config.do_sdcard_write)
  {
    //write only if logging active and dataFile ready for writes
    if(flags_status.logging_state && dataFile )
    {
      // write blocks of data
      /* write in hex format */
      if(flags_config.do_sdcard_write_hex)
      {
        /* data record to read */
        DATA_RECORD *data_record = (DATA_RECORD *)data_store.data;
  
        dataFile.write((byte*)data_store.bytes_stored, sizeof(data_store.bytes_stored));    //write the number of bytes sent
        dataFile.write(data_store.hash);                                              //write the hash value
        dataFile.write(data_store.data, sizeof(data_store.bytes_stored));            //write the data
        
        //for now, we can just pad each write to make up the block size
        int bytes_left = (sizeof(data_store.bytes_stored) + sizeof(data_store.hash) + data_store.bytes_stored);
        bytes_left %= BLOCK_SIZE;
        bytes_left = BLOCK_SIZE - bytes_left;
        byte pad[bytes_left] = {0};
        dataFile.write(pad, bytes_left);
      }
      /* write plain text format */
      else
      {
        #ifdef DEBUG_SDCARD
        unsigned int blocks_written = 0;
        static unsigned long total_blocks_written = 0;
        #endif
  
        
        while (dataBuffer.length() >= BLOCK_SIZE)
        {
          // write a single block of data
          if(dataFile.write(dataBuffer.c_str(), BLOCK_SIZE))
          {
            // remove written data from dataBuffer if write was sucessfull
            dataBuffer.remove(0, BLOCK_SIZE);
            clear_buffer = false;

            #ifdef DEBUG_SDCARD
            blocks_written++;
            #endif
          }
          else
          {
            Serial.println(F("File write failed"));
            flags_status.file_openable = false;
            clear_buffer = true;
            break; //exit the while loop
          }
        }

        if(flags_status.logging_state == LOG_STOPPING)
        {
          // write the last of the data, even if it's less than one block
          // if this results in long waits, pad the buffer with zeros before writing.
          // non buffer sized writes typically are a few ms longer, 
          // and only seem to cause big delays after many non buffer sized writes
          dataFile.write(dataBuffer.c_str(), dataBuffer.length());
          clear_buffer = true;
        }

        
        #ifdef DEBUG_SDCARD
        total_blocks_written += blocks_written;
        Serial.print(F("SD blocks written: "));        Serial.println(blocks_written);
        Serial.print(F("SD total blocks: "));        Serial.println(total_blocks_written);
        #endif
      } // format selection
    } //is logging and file is open
    
  }
  
  if(clear_buffer)
  {
    //clear the buffer when not logging to sdcard
    dataBuffer = "";
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
