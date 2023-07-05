
#define FS(s) ((__FlashStringHelper *)(s))  //for functions that accept it, use in place of F() for globally defined PSTRings
const char S_DATE[] PROGMEM = __DATE__;
const char S_NL[] PROGMEM = "\n";

void setup() {
  // put your setup code here, to run once:
  String string;
  char tmp[0];
  memset(tmp, 1,0);
  char foop[5];
  //foop[4] = 0;
  
  //string += String(FS(S_DATE)); //2130 f
  //string += (FS(S_DATE)); //2014 f
  //string += (10); //2136 f
  //string += String(10); 2236 f
  //string += '\n'; //1820 f, 21 r
  //string += "\n"; //1806 f, 21 r
  //string += F("\n"); //1822 f , 21 r
  //string += FS(S_NL); //1822 f, 21 r

  //string += F("\n"); string += F("\n"); string += F("\n"); string += F("\n");
  //1932 , 21

  
  Serial.begin(1000000);
  //Serial.println(string);
  Serial.println(foop);
}

void loop() {
  // put your main code here, to run repeatedly:

}
