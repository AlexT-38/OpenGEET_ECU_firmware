/* scan for serial commands,
 */


/* list of commands */
//config commands should only work when stopped or disabled


/* table of letters representing commands, starting with A */
const COMMAND commands_by_letter[] = {C_PRINT_STATE, C_HELP, C_SET_PRESCALE, C_HELP,           //A,B,C,D
                                      C_LOAD_EEP, C_FORCE_PWM, C_HELP, C_HELP,               //E,F,G,H
                                      C_SET_PWM_INVERT, C_HELP, C_HELP, C_SET_PWM_LIMIT,  //I,J,K,L
                                      C_HELP, C_HELP, C_OSCILLATE, C_SET_PWM,            //M,N,O,P
                                      C_HELP, C_SET_RATE, C_STOP_ALL, C_USE_LUT,        //Q,R,S,T
                                      C_HELP, C_SET_PWM_RAMP, C_SAVE_EEP,                   //U,V,W
                                      C_RESET_CONFIG, C_HELP, C_PRINT_CONFIG };   //X,Y,Z
                          
const char command_letters[] = {'S','R','P','F','C','I','V','O','T','L','W','E','X','Z','A','H'}; //this ought to be an indexed intialiser



/* check for serial commands and produce periodic report */
void Process_Commands()
{
  //Serial.println(F("process commands"));
  if (Serial.available() > 0)
  {
    recieve_command();
  }
}



void recieve_command()
{
  //Serial.println(F("rcv"));
  #define CMD_BUF_SIZE 255
  char buf[CMD_BUF_SIZE+1];
  byte bpos = 0; //buffer position
  char * bpos_ptr = &buf[0];

  //read a line from the buffer
  byte bend = Serial.readBytesUntil('/n', buf, CMD_BUF_SIZE);

  //return if no bytes read
  if (!bend) return;
  //ensure buffer is null terminated
  buf[bend]=0;
  //otherise look for the first capital alpha
  while(!(buf[bpos] >= 'A' && buf[bpos] <= 'Z') && !(buf[bpos] >= 'a' && buf[bpos] <= 'z') && bpos < 255) bpos++;
  //return if command not found
  if (bpos == 255) return;
  
  //get the command from the commands table
  COMMAND cmd = print_command(buf[bpos]);
  
  bpos++;
  switch (cmd)
  {
       //basic commands
  case C_STOP_ALL:       //S stop output
    Serial.println(FS(S_STOP_ALL));
    config.enable = false;
    config.oscillate = false;
    write_pwm(0);
    break;
  case C_SET_RATE:   //R set relay rate
    {
      long int new_interval = atol(&buf[bpos]);
      
      if (new_interval > INTERVAL_MIN)
      {
        set_time_interval(new_interval);
        
        Serial.print(FS(S_SET_INTERVAL));
        Serial.print(FS(S_COLON));
        Serial.println(new_interval<<1);
      }
      else
      {
        if (!config.enable)
        {
          Serial.println(FS(S_LFO_OUT_ENABLE));
          config.enable = true;
        }
        else
        {
          Serial.println(FS(S_LFO_OUT_DISABLE));
          config.enable = false;
        }
        
      }
    }
    break;
  case C_SET_PWM:    //P set pwm ratio
    {
      //fetch a parameter
      int new_pwm = strtol(&buf[bpos], &bpos_ptr, 10);
      //if no parameter provided and not oscillating, reapply 
      //the configured pwm to the ramp target
      if (bpos_ptr == &buf[bpos] && !config.oscillate)
      {
        set_target(config.pwm);
      }
      //discard out of range values and set the new pwm value
      else if (new_pwm <= 255 && new_pwm >=0)
      {
        set_pwm(new_pwm);
        Serial.print(F("pwm: "));
        Serial.println(new_pwm);
      }
    }
    break;
  case C_FORCE_PWM:
    {
      int new_pwm = atoi(&buf[bpos]);
      
      if (new_pwm <= 255 && new_pwm >=0)
      {
        force_pwm(new_pwm);
        Serial.print(FS(S_FORCE_PWM));
        Serial.print(FS(S_COLON));
        Serial.println(new_pwm);
      }
    }
    break;
  case C_SET_PRESCALE:
    {
      byte data = (long)atoi(&buf[bpos]);
      set_pwm_prescale(data);
    }
    break;
  case C_SET_PWM_LIMIT:
    {
      int data = (long)atoi(&buf[bpos]);
      if (data <= 0 && data > -255)
          {
            config.pwm_min = -data;
            Serial.print(F("PWM min: "));
            Serial.println(config.pwm_min);
          }
      else if (data > 0 && data <= 255)
      {
        config.pwm_max = data;
        Serial.print(F("PWM max: "));
        Serial.println(config.pwm_max);
      }
    }
    break;
    case C_SET_PWM_INVERT:
    {
      byte data = strtol(&buf[bpos],&bpos_ptr,10);
      bpos = bpos_ptr - &buf[0];
      byte data2 = strtol(bpos_ptr,&bpos_ptr,10);
      set_pwm_invert(data,data2);
    }  
    break;
    case C_SET_PWM_RAMP:
    {
      config.pwm_ramp = atoi(&buf[bpos]);
      Serial.print(F("ramp: "));
      Serial.println(config.pwm_ramp);
    }
    break;
    case C_OSCILLATE:
    {
      Serial.print(F("osc: "));
      int val = strtol(&buf[bpos], &bpos_ptr, 10);
      if (bpos_ptr == &buf[bpos] || val < 0 || val > 255)
      {
        config.oscillate = !config.oscillate;
        Serial.println(config.oscillate);
      }
      else
      {
        config.oscillate = true;
        config.pwm_osc = val;
      }
      if(config.oscillate)
      {
        Serial.print(config.pwm);
        Serial.print(FS(S_ARROW));
        Serial.println(config.pwm_osc);
      }
    }
    break;
  case C_USE_LUT:
    config.lut = !config.lut;
    Serial.print(F("lut: "));
    Serial.println(config.lut);
    break;
  case C_SAVE_EEP:
    update_eeprom();
    break;
  case C_LOAD_EEP:
    load_eeprom();
    break;
  case C_RESET_CONFIG:
    reset_config();
    break;
  case C_PRINT_CONFIG:
    export_config(&Serial, &config);
    break;
  case C_PRINT_STATE:
    print_state(&Serial);
    break;
  case NO_OF_COMMANDS:
    break;
  default:
    Serial.println(F("Unkown command"));
  case C_HELP:                   //H prints available commands
    print_help();
    break;
  }
  
}



COMMAND print_command(char letter)
{
  //Serial.print(F("command: "));
  //Serial.println(letter);
  COMMAND cmd;
  if(letter >= 'a' && letter <= 'z')
  {
    letter += 'A'-'a';
  }
  if(letter >= 'A' && letter <= 'Z')
  {
    cmd = commands_by_letter[letter - 'A'];
    char buf[50];
    READ_STRING_FROM(command_str,cmd,buf);
    Serial.print(letter);
    Serial.print(FS(S_COLON));
    Serial.println(buf);
  }
  else
  {
    cmd = NO_OF_COMMANDS;
  }
  return cmd;
}
void print_help()
{

  /*This takes too long to print.
    which means that at least this function should have a handler in the main loop
    and probably ought to move command handling out of motor update, 
    and ensure that no handler takes longer than time left till next motion process update
  */
  
  int elapsed_time = millis();
  Serial.println();
  for(byte n = 0; n<(sizeof(commands_by_letter)/sizeof(COMMAND)); n++)
  {
    print_command('A'+n);
  }
  Serial.println();
  for(byte n = 0; n < NO_OF_COMMANDS; n++)
  {
    print_command(command_letters[n]);
  }
  Serial.println();


  
  Serial.print(F("time: "));

  elapsed_time = millis() - elapsed_time;

  Serial.println(elapsed_time);
  Serial.println();
  
}
