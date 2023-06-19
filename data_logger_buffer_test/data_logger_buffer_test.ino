#include <SD.h>         //sd card library
//#include <SPI.h>        //needed for gameduino

#define SD_CARD_CS_PIN 10

void setup() {
  // set data logger SD card CS high
  pinMode(SD_CARD_CS_PIN, OUTPUT);
  digitalWrite(SD_CARD_CS_PIN, HIGH);
  // set Gameduino CS pins high
  pinMode(9, OUTPUT);
  digitalWrite(9, HIGH);
  pinMode(8, OUTPUT);
  digitalWrite(8, HIGH);
  // set egt sensor CS high
  pinMode(7, OUTPUT);
  digitalWrite(7, HIGH);
  Serial.begin(1000000);

  Serial.println(F("data_logger_begin_test"));
  if(SD.begin(10))
  {
    Serial.println("ok");
  }
  else
  {
    Serial.println("fail");
  }
  delay(5000);
  generate_file_name();
}

char output_filename[13] = "test0009.txt"; //8+3 format


#define BUF_LEN sizeof(str_buffer)
const char str_buffer[] = "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";
int idx = 0;
int len;
int cycles = 0;

#define CYCLES 10
long t_tot = 0;
long b_tot = 0;

File dataFile;

void loop() {  // put your main code here, to run repeatedly:

  delay(100);

  len=random(1500,2000);
  b_tot += len+1;
  
  //wrap the start around the buffer
  if(idx >= BUF_LEN) idx=0;
  //get the last character
  int last = idx + len;
  if(last >= BUF_LEN) last = BUF_LEN-1;
  //get the total number of bytes being written
  int wr_size = 1 + last - idx;

  //copy a section of the string
  char wr_buffer[wr_size+1];
  memcpy(wr_buffer,&str_buffer[idx],wr_size);
  wr_buffer[wr_size]=0;

  long timestamp = micros();
  //open, write and close
  dataFile = SD.open(output_filename, FILE_WRITE);
  dataFile.print(wr_buffer);
  dataFile.close();
  //find the time taken
  timestamp = micros() - timestamp;

  Serial.print("op/wr/cl bytes: "); Serial.print(wr_size); Serial.print("; bytes(tot): "); Serial.print(b_tot); Serial.print("; time (us): "); Serial.print(timestamp); Serial.println();

  t_tot += timestamp;
  cycles++;

  if (cycles >= CYCLES)
  {
    Serial.print("t avg (us): "); Serial.println(t_tot/CYCLES); Serial.println();
    t_tot = 0;
    cycles = 0;
    len+=0;
    if(len >= (BUF_LEN-1)) while(1); //stop 
  }
}



void generate_file_name()
{
  static byte file_index = 0;
  char *str_ptr;
  
  if (!SD.exists(output_filename))
  {
    //report the selected filename
    Serial.print(F("Output file name:"));
    Serial.println(output_filename);
  }
  else
  {
    Serial.print(F("File exists: "));
    Serial.println(output_filename);
  }
  
  
  //check that the file can be opened
  dataFile = SD.open(output_filename, FILE_WRITE);
  if(dataFile)
  {
    //print the header
    //dataFile.println(F("Test data"));
    //dataFile.close();

    
    
    Serial.println(F("Opened OK"));
  }
  else
  {
    //open failed on first attempt, do not attempt to save to this file
    Serial.println(F("Open FAILED"));
  }

}
