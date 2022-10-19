

SV_CAL servo_cal[NO_OF_SERVOS];

void initialise_servos()
{
  const byte servo_pins[] = {PIN_SERVO_0,PIN_SERVO_1,PIN_SERVO_2};
  for(byte n=0; n<NO_OF_SERVOS; n++) 
  {
    servo_cal[n] = (SV_CAL){SERVO_MIN,SERVO_MAX};
    servo[n].attach(servo_pins[n]);
  }
  
}

void reset_servos()
{
  for(byte n=0;n<NO_OF_SERVOS;n++)
  {
    set_servo_position(n,0);
  }
}



/* set the position of servo n as a ratio of min to max on a 10bit scale */
void set_servo_position(byte n, unsigned int ratio)
{
    //map input to output range - MAP servo takes 72us, amap takes 76 (due to rounding) barely any difference so use amap
    unsigned int servo_pos_us = amap(ratio, servo_cal[n].lower, servo_cal[n].upper);
    //write output
    servo[n].writeMicroseconds(servo_pos_us);
    #ifdef DEBUG_SERVO
    Serial.print(F("sv["));Serial.write('0'+n);Serial.print(F("]: ")); 
    Serial.print(ratio);
    Serial.print(F(" (")); 
    Serial.print(servo_cal[n].lower);
    Serial.print(F("-")); 
    Serial.print(servo_cal[servo].upper);
    Serial.print(F(") -> ")); 
    Serial.print(servo_pos_us);
    Serial.println();
    #endif
}
