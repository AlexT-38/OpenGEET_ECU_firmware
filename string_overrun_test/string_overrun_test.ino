void setup() {
  // put your setup code here, to run once:
 // set data logger SD card CS high
  pinMode(10, OUTPUT);
  digitalWrite(10, HIGH);
  // set Gameduino CS pins high
  pinMode(9, OUTPUT);
  digitalWrite(9, HIGH);
  pinMode(8, OUTPUT);
  digitalWrite(8, HIGH);
  // set egt sensor CS high
  pinMode(7, OUTPUT);
  digitalWrite(7, HIGH);

  
  Serial.begin(1000000);
  Serial.println(F("ADC test Mk2 (issue #10)"));



  String string;
  Serial.print("string size b/f reserve : ");
  Serial.println(sizeof(string.c_str()));
  //consume most of the available memory
  string.reserve(7000);

  Serial.print("string size aft reserve : ");
  Serial.println(sizeof(string.c_str()));

  //fill the string with data, printing the address and string length as we go
  for(int n = 0; n < 8200; n++)
  {
    Serial.print("string address: ");
    Serial.println((unsigned int)&string,HEX);
    Serial.print("string length:  ");
    Serial.println(string.length());
    byte count = 255;
    while(count--){ string+='.';}
    Serial.println();
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  
}
