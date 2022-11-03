/*
 * looking at the efficiency of manually pulling strings from PROGMEM into ram before printing, 
 * vs using the F() macro for each instnace
 */

// strlen_P reduces to constant if string size known at compile time, hence replacing with arbitrary size does nothing
#define MAKE_STRING(STRING_NAME)  char STRING_NAME ## _str[strlen_P(STRING_NAME)+1]; strcpy_P(STRING_NAME ## _str,STRING_NAME)

//like F but with a global progmem pointer, instead of one created inline from a string literal and PSTR()
#define FS(s) ((__FlashStringHelper *)(s))

#define SL_123 "123"
#define SL_123x10 "1234567890"
#define SL_123x30 "123456789012345678901234567890"
#define SL_123x100 "1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890"
void setup() {
  // put your setup code here, to run once:

  Serial.begin(1000000);

  /*                        //1538 bytes used
  Serial.println(F(SL_123));
  Serial.println(F(SL_123));
//   */
  /*                           //1560 bytes used - 12 bytes more for a 4 byte string
  static const char S_123[] PROGMEM = SL_123;
  MAKE_STRING(S_123);
  Serial.println(S_123_str);
  Serial.println(S_123_str);
//  */
  /*                        //1732 bytes used - 194 bytes more for 2x 101 byte strings vs 2x 4 bytes
  Serial.println(F(SL_123x100));
  Serial.println(F(SL_123x100));
//  */
  /*                         //1688 bytes used - 44 bytes less than 2x F(), 128 bytes more usage for 96 extra chars
  static const char S_123[] PROGMEM = SL_123x100;
  MAKE_STRING(S_123);
  Serial.println(S_123_str);
  Serial.println(S_123_str);
//  */

  /*                        //2604 bytes used - 872 bytes for 8x more 101 byte strings
  Serial.println(F(SL_123x100));
  Serial.println(F(SL_123x100));
  Serial.println(F(SL_123x100));
  Serial.println(F(SL_123x100));
  Serial.println(F(SL_123x100));
  Serial.println(F(SL_123x100));
  Serial.println(F(SL_123x100));
  Serial.println(F(SL_123x100));
  Serial.println(F(SL_123x100));
  Serial.println(F(SL_123x100));
//  */
  
  /*                         //1736 bytes used - 868 bytes less than 10x F(), 48 bytes more than 2x prints (6 bytes per print())
  static const char S_123[] PROGMEM = SL_123x100;
  MAKE_STRING(S_123);
  Serial.println(S_123_str);
  Serial.println(S_123_str);
  Serial.println(S_123_str);
  Serial.println(S_123_str);
  Serial.println(S_123_str);
  Serial.println(S_123_str);
  Serial.println(S_123_str);
  Serial.println(S_123_str);
  Serial.println(S_123_str);
  Serial.println(S_123_str);
//  */

  /*                        //1634  bytes used - 96 more for 10x than 2x with 4 byte string
  Serial.println(F(SL_123));
  Serial.println(F(SL_123));
  Serial.println(F(SL_123));
  Serial.println(F(SL_123));
  Serial.println(F(SL_123));
  Serial.println(F(SL_123));
  Serial.println(F(SL_123));
  Serial.println(F(SL_123));
  Serial.println(F(SL_123));
  Serial.println(F(SL_123));
//   */
  /*                           //1624 bytes used - 10 bytes less than 10x F() with 4 byte string
  static const char S_123[] PROGMEM = SL_123;
  MAKE_STRING(S_123);
  Serial.println(S_123_str);
  Serial.println(S_123_str);
  Serial.println(S_123_str);
  Serial.println(S_123_str);
  Serial.println(S_123_str);
  Serial.println(S_123_str);
  Serial.println(S_123_str);
  Serial.println(S_123_str);
  Serial.println(S_123_str);
  Serial.println(S_123_str);
//  */

  /*                        //1704  bytes used, 70 more 10x4byte string (matches increase in string length)
  Serial.println(F(SL_123x10));
  Serial.println(F(SL_123x10));
  Serial.println(F(SL_123x10));
  Serial.println(F(SL_123x10));
  Serial.println(F(SL_123x10));
  Serial.println(F(SL_123x10));
  Serial.println(F(SL_123x10));
  Serial.println(F(SL_123x10));
  Serial.println(F(SL_123x10));
  Serial.println(F(SL_123x10));
//   */
  /*                           //1640 bytes used - 64 bytes less than 10x F() with 11 byte string
  static const char S_123[] PROGMEM = SL_123x10;
  MAKE_STRING(S_123);
  Serial.println(S_123_str);
  Serial.println(S_123_str);
  Serial.println(S_123_str);
  Serial.println(S_123_str);
  Serial.println(S_123_str);
  Serial.println(S_123_str);
  Serial.println(S_123_str);
  Serial.println(S_123_str);
  Serial.println(S_123_str);
  Serial.println(S_123_str);
//  */
  /*                           //1748 bytes used - 44 bytes more than 10x F() with 11 byte string
  static const char S_123[] PROGMEM = SL_123x10;
  {MAKE_STRING(S_123);  Serial.println(S_123_str);}
  {MAKE_STRING(S_123);  Serial.println(S_123_str);}
  {MAKE_STRING(S_123);  Serial.println(S_123_str);}
  {MAKE_STRING(S_123);  Serial.println(S_123_str);}
  {MAKE_STRING(S_123);  Serial.println(S_123_str);}
  {MAKE_STRING(S_123);  Serial.println(S_123_str);}
  {MAKE_STRING(S_123);  Serial.println(S_123_str);}
  {MAKE_STRING(S_123);  Serial.println(S_123_str);}
  {MAKE_STRING(S_123);  Serial.println(S_123_str);}
  {MAKE_STRING(S_123);  Serial.println(S_123_str);}
//  */
//  /*                           //1564 bytes used - 140 bytes less than 10x F() with 11 byte string
                // we have a winner, at least for direct printing of recurrent strings
  static const char S_123[] PROGMEM = SL_123x10;
  Serial.println(FS(S_123));
  Serial.println(FS(S_123));
  Serial.println(FS(S_123));
  Serial.println(FS(S_123));
  Serial.println(FS(S_123));
  Serial.println(FS(S_123));
  Serial.println(FS(S_123));
  Serial.println(FS(S_123));
  Serial.println(FS(S_123));
  Serial.println(FS(S_123));
//  */
}

void loop() {}
//  // put your main code here, to run repeatedly:
//  unsigned long t= micros();
//
//// sizes were with this code in setup() loop/setup
//
//  /*                        //1896/1538 bytes used
//  Serial.println(F(SL_123));
//  Serial.println(F(SL_123));
////   */
//  /*                           //1884/1648 bytes used - 110 bytes more for a 4 byte string (was 88 in ecu code)
//  static const char S_123[] PROGMEM = SL_123;
//  MAKE_STRING(S_123);
//  Serial.println(S_123_str);
//  Serial.println(S_123_str);
////  */
//  /*                        //2090/1732 bytes used - 194 bytes more for 2x 101 byte strings vs 2x 4 bytes
//  Serial.println(F(SL_123x100));
//  Serial.println(F(SL_123x100));
////  */
////  /*                         //2018/1748 bytes used - 16 bytes more for a 101 byte string (was 492 in ecu code)
//  static const char S_123[] PROGMEM = SL_123x100;
//  MAKE_STRING(S_123);
//  Serial.println(S_123_str);
//  Serial.println(S_123_str);
////  */
//
//  /*                        //2604 bytes used - 856 bytes for 8x more 101 byte strings (6 bytes per serial.println() call)
//  Serial.println(F(SL_123x100));
//  Serial.println(F(SL_123x100));
//  Serial.println(F(SL_123x100));
//  Serial.println(F(SL_123x100));
//  Serial.println(F(SL_123x100));
//  Serial.println(F(SL_123x100));
//  Serial.println(F(SL_123x100));
//  Serial.println(F(SL_123x100));
//  Serial.println(F(SL_123x100));
//  Serial.println(F(SL_123x100));
////  */
//  
//  /*                         //1796 bytes used - 808 bytes less than 10x F() for a 101 byte string
//  static const char S_123[] PROGMEM = SL_123x100;
//  MAKE_STRING(S_123);
//  Serial.println(S_123_str);
//  Serial.println(S_123_str);
//  Serial.println(S_123_str);
//  Serial.println(S_123_str);
//  Serial.println(S_123_str);
//  Serial.println(S_123_str);
//  Serial.println(S_123_str);
//  Serial.println(S_123_str);
//  Serial.println(S_123_str);
//  Serial.println(S_123_str);
////  */
//
//  /*                        //1634  bytes used - 96 more for 10x than 2x with 4 byte string
//  Serial.println(F(SL_123));
//  Serial.println(F(SL_123));
//  Serial.println(F(SL_123));
//  Serial.println(F(SL_123));
//  Serial.println(F(SL_123));
//  Serial.println(F(SL_123));
//  Serial.println(F(SL_123));
//  Serial.println(F(SL_123));
//  Serial.println(F(SL_123));
//  Serial.println(F(SL_123));
////   */
//  /*                           //1696 bytes used - 62 bytes more than 10x F() with 4 byte string
//  static const char S_123[] PROGMEM = SL_123;
//  MAKE_STRING(S_123);
//  Serial.println(S_123_str);
//  Serial.println(S_123_str);
//  Serial.println(S_123_str);
//  Serial.println(S_123_str);
//  Serial.println(S_123_str);
//  Serial.println(S_123_str);
//  Serial.println(S_123_str);
//  Serial.println(S_123_str);
//  Serial.println(S_123_str);
//  Serial.println(S_123_str);
////  */
//
//  /*                        //2060  bytes used 
//  Serial.println(F(SL_123x10));
//  Serial.println(F(SL_123x10));
//  Serial.println(F(SL_123x10));
//  Serial.println(F(SL_123x10));
//  Serial.println(F(SL_123x10));
//  Serial.println(F(SL_123x10));
//  Serial.println(F(SL_123x10));
//  Serial.println(F(SL_123x10));
//  Serial.println(F(SL_123x10));
//  Serial.println(F(SL_123x10));
////   */
//  /*                           //1968 bytes used - 92 bytes less than 10x F() with 11 byte string
//  static const char S_123[] PROGMEM = SL_123x10;
//  MAKE_STRING(S_123);
//  Serial.println(S_123_str);
//  Serial.println(S_123_str);
//  Serial.println(S_123_str);
//  Serial.println(S_123_str);
//  Serial.println(S_123_str);
//  Serial.println(S_123_str);
//  Serial.println(S_123_str);
//  Serial.println(S_123_str);
//  Serial.println(S_123_str);
//  Serial.println(S_123_str);
////  */
//  t=micros()-t;
//  Serial.println(t);
//  Serial.println();
//  delay(100);
//}
