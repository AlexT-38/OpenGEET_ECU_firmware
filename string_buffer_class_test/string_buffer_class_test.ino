#include "src/StringBuffer/StringBuffer.h"


#define BUFFER_SIZE (_BV(10)+1)
static char buf_c[BUFFER_SIZE];

StringBuffer string_buffer(BUFFER_SIZE, buf_c);
//StringBuffer string_buffer(BUFFER_SIZE);

void setup() {
  
  Serial.begin(1000000);
  Serial.println("StringBuffer Test");

  Serial.println();

  //Serial.println(string_buffer.length());

  for (int m = 0; m < 200; m++)
  {
    string_buffer += "_1234567890_";
    string_buffer += m;

    Serial.print("ovr: "); Serial.println(string_buffer.get_overrun());
    Serial.print("len: "); Serial.println(string_buffer.length());
    Serial.print("rd : "); Serial.println(string_buffer.get_rd());
    Serial.print("wr : "); Serial.println(string_buffer.get_wr());
    Serial.print("cmt: "); Serial.println(string_buffer.get_committed());
    Serial.print("old: "); Serial.println(string_buffer.get_old());
    Serial.print("str: "); Serial.println(string_buffer.str());

    if(m%10 == 9)
    {
      unsigned int pop_length = string_buffer.length()>>1;
      
      Serial.print("pop (");Serial.print(pop_length);Serial.print(" ): "); unsigned int pop_count = string_buffer.pop(Serial,string_buffer.length()>>1); Serial.println();
      Serial.print("pop count: "); Serial.println(pop_count);
    }
    if(m%20==18)
    {
      string_buffer.commit();
      
      Serial.println("\ncommit...");
      Serial.print("cmt: "); Serial.println(string_buffer.get_committed());
      Serial.print("wr : "); Serial.println(string_buffer.get_wr());
      Serial.print("old: "); Serial.println(string_buffer.get_old());
    }
    if(m%50==47)
    {
      unsigned long time_us = micros();
      unsigned int rewind = string_buffer.rewind(512);
      time_us = micros() - time_us;
      Serial.println("\nrewind...");
      Serial.print("cnt: "); Serial.println(rewind);
      Serial.print("time: "); Serial.println(time_us);
    }
    Serial.println("\n//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////\n");
  }
  
}


void loop() {
  // put your main code here, to run repeatedly:

}
