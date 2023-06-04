//#define VERBOSE

/* ADC handling.
 *  After the first sample, it takes 13 Tclk(adc) ticks to complete a conversion
 *  at 16Mhz with 128 prescaler, Tclk(adc) = 104 us     measured: 108us
 *  to sample all sixteen channels takes 1664 us        measured: 1892 (analogRead) 1936 (this)
 *  to sample all sixteen channels twice takes 3328 us  measured: 3692 (analogRead) 3888 (this)
 */


#define ADC_CHANNELS                  16//(ADC_CHN_TMP_END+1)        // number of ADC inputs to scan
//#define ADC_MAP                   // enable remapping of inputs - only required if using a sheild that blocks access to some ADC pins, or not using an R3 board with dedicated SDA/SCL pins
#define ADC_MULTISAMPLE           //enable sampling the ADC multiple times and taking the average. 
                                  //this feature needs some thought, as it may be better to sample over time using a timer to trigger a sampling run
                                  //or it may be better to retain the extra bits from oversampling instead of rounding back down to 10bit
                                  //or it could just be a complete waste of time.
//#define ADC_USE_ISR               //use the ADC ISR to handle the sample run. given that the time required to sample all 16 channels twice is 1/3rd of the available processing time
                                  //and there is only 1x 10Hz external ADC (torque sensor)





#ifdef ADC_MULTISAMPLE 
#define ADC_EXTRA_BITS  1
#define ADC_SAMPLES _BV(ADC_EXTRA_BITS)
byte ADC_sample;
volatile int ADC_each_result[ADC_SAMPLES][ADC_CHANNELS];
#endif

#ifdef ADC_MAP
static byte ADC_channel_map[ADC_CHANNELS] = {1,2,0};
#define ADC_SELECT  ADC_channel_map[ADC_channel]
#else
#define ADC_SELECT  ADC_channel
#endif


volatile int ADC_results[ADC_CHANNELS];
volatile byte ADC_channel;
volatile bool ADC_complete;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(1000000);
  Serial.println("ADC test");

  int t0 = micros();
  #ifdef ADC_MULTISAMPLE
  memset(ADC_results,0,sizeof(ADC_results));
  for(byte smp = 0; smp < ADC_SAMPLES; smp++)
  #endif
  {
    for(int n = 0; n<ADC_CHANNELS; n++)
    {
      ADC_results[n] += analogRead(A0+n);
    }
  }
    
  t0=micros()-t0;
  Serial.println(t0);
  

  
  configureADC();
}

void loop() {
  // put your main code here, to run repeatedly:
  static unsigned int pattern = 0;
  
  PORTF = pattern;
  PORTK = pattern>>8;
  
  process_analog_inputs();
  
  Serial.print("Pattern: b"); Serial.println(pattern,BIN);
  //delay(3000);
  while(Serial.available()==0);
  while(Serial.available()) Serial.read();
  
  //select each channel in turn
  if(pattern == 0) { pattern=1; }
  else { pattern <<= 1; }
}



void configureADC()
{
  //set the reference
  ADMUX = _BV(REFS0);
  //disable the digital inputs
  //DIDR0 = 0xFF;
  //DIDR1 = 0xFF;
  //enable pullups for testing purposes only
  PORTF=0;//xFF; 
  PORTK=0;//xFF;
  // or set to output and cycle through
  DDRF=0xFF;
  DDRK=0xFF;

  //enable the ADC and interrupt flag, prescaler to slowest rate
  #ifdef ADC_USE_ISR
  ADCSRA = _BV(ADEN) | _BV(ADIE) | 0x7;
  #else
  ADCSRA = _BV(ADEN) | 0x7;
  #endif
  
  ADC_reset();
  
  #ifdef VERBOSE
  Serial.println("ADC configured");
  #endif
}

void ADC_reset()
{
  #ifdef ADC_MULTISAMPLE
  ADC_sample = 0;
  #endif
  
  //Set the channel select registers
  ADC_channel = 0;
  ADC_set_channel();

  
  //clear the results table
  memset(ADC_results,0,sizeof(ADC_results));

  //clear the completion flag
  ADC_complete = false;
}
void startADC()
{
  #ifdef VERBOSE
  Serial.println("ADC start");
  #endif
  
  ADC_reset();

  //start the first conversion
  ADCSRA |= _BV(ADSC);

  #ifdef VERBOSE
  Serial.println("ADC started");
  #endif
}

void ADC_set_channel()
{
  #ifdef VERBOSE
  Serial.print("ADC set channel: ");
  Serial.print(ADC_channel);
  Serial.print("; sample: ");
  Serial.println(ADC_sample);
  #endif
  
  //ADC is in two banks of 8, ADMUX set the bank's channel and voltage reference
  ADMUX = (ADC_SELECT & 0x7) | _BV(REFS0);
  //select the bank
  ADCSRB = ((ADC_SELECT & 0x8)!=0) * _BV(MUX5); //shift bit 3 into bit 5

  #ifdef VERBOSE
  Serial.println("ADC channel set");
  #endif
}

void process_analog_inputs()
{
  Serial.println("\n\n\n\n\n\n\n\n\n\n");
  unsigned int timestamp_us = micros();
  
  startADC();

  #ifndef ADC_MULTISAMPLE 
  byte ADMUX_dump[16];
  byte ADCSRB_dump[16];
  #else
  byte ADMUX_dump[ADC_SAMPLES][16];
  byte ADCSRB_dump[ADC_SAMPLES][16];
  #endif
  long int time_dump[16];

  memset(time_dump,0,sizeof(time_dump));
  
  while(!ADC_complete)
  {
    #ifndef ADC_MULTISAMPLE 
    ADMUX_dump[ADC_channel] = ADMUX;
    ADCSRB_dump[ADC_channel] = ADCSRB;
    #else
    ADMUX_dump[ADC_sample][ADC_channel] = ADMUX;
    ADCSRB_dump[ADC_sample][ADC_channel] = ADCSRB;
    #endif
    
#ifndef ADC_USE_ISR
    //wait for interrupt flag
    long int time_us = micros();
    while((ADCSRA&_BV(ADIF))==0)
    {
      #ifdef VERBOSE
      //Serial.println(ADCSRA,HEX);
      //delayMicroseconds(100);
      
      #endif
    }
    time_dump[ADC_channel] += micros() - time_us;
    
    //clear the flag by writing 1 to it
    ADCSRA |= _BV(ADIF);
    
    //run the sample process
    ADC_process();
#endif
  }
  #ifdef VERBOSE
  Serial.println("ADC average multi samples");
  #endif

  // average extra samples
#ifdef ADC_MULTISAMPLE
  for (int chn = 0; chn < ADC_CHANNELS; chn++)
  {
    ADC_results[chn] = (ADC_results[chn]+_BV(ADC_EXTRA_BITS-1)) >> ADC_EXTRA_BITS;
  }
#endif


  timestamp_us = micros() - timestamp_us;
  Serial.print(F("t_ana us: "));
  Serial.println(timestamp_us);



  Serial.println(F("ADC results:"));
  for (int chn = 0; chn < ADC_CHANNELS; chn++)
  {
    Serial.print(chn);
    Serial.print(": ");
    Serial.print(ADC_results[chn]);
    #ifdef ADC_MULTISAMPLE 
    Serial.print("; ");
    for (byte n = 0; n < ADC_SAMPLES; n++)
    {
      
      Serial.print(ADC_each_result[n][chn]);
      Serial.print(", ");
    }
    Serial.print("; ADMUX/ADCSRB: ");
    for (byte n = 0; n < ADC_SAMPLES; n++)
    {
      Serial.print("(");
      Serial.print(ADMUX_dump[n][chn],HEX);
      Serial.print("; ");
      Serial.print(ADCSRB_dump[n][chn],HEX);
      Serial.print("), ");
    }
    
    #else
    Serial.print("; ADMUX: 0x");
    Serial.print(ADMUX_dump[chn],HEX);
    Serial.print("; ADCSRB: 0x");
    Serial.print(ADCSRB_dump[chn],HEX);
    #endif

    Serial.print("; time(us): ");
    Serial.print(time_dump[chn]);
    Serial.println();
  }
  Serial.println();

}

#ifdef ADC_USE_ISR
ISR(ADC_vect)
#else
void ADC_process()
#endif
{
  #ifdef VERBOSE
  Serial.println();
  Serial.print("ADC process start, chn/smp: ");
  Serial.print(ADC_channel);
  Serial.print(", ");
  Serial.print(ADC_sample);
  Serial.print(" = ");
  Serial.print(ADC);
  Serial.println();
  #endif

  #ifdef ADC_MULTISAMPLE 
  ADC_each_result[ADC_sample][ADC_channel] = ADC;
  #endif
  //grab the result and increment the channel
  ADC_results[ADC_channel++] += ADC;

  
  
  if(ADC_channel < ADC_CHANNELS)
  {
    ADC_set_channel();
    
    //start the next conversion
    #ifdef VERBOSE
    Serial.print("ADC process, next chn: ");
    Serial.print(ADC_channel);
    Serial.println();
    #endif
    ADCSRA |= _BV(ADSC);

    //Serial.print(F("ADMUX/ADSCRB (hex): "));     Serial.print(ADMUX /*& 0x1F*/,HEX); Serial.print(", "); Serial.println(ADCSRB /*& _BV(MUX5)*/,HEX);
  }
  else //run through channels done
  {
    ADC_channel = 0;
    ADC_set_channel();
    #ifndef ADC_MULTISAMPLE 
    ADC_complete = true;
    #else

    //next sample
    ADC_sample++;

    
    
    //run multiple samples
    if(ADC_sample < ADC_SAMPLES)
    {
      #ifdef VERBOSE
      Serial.print("ADC process, next smp/chn: ");
      Serial.print(ADC_sample);
      Serial.print(", ");
      Serial.print(ADC_channel);
      Serial.println();
      #endif
      //start the next conversion
      ADCSRA |= _BV(ADSC);
    }
    else
    {
      ADC_complete = true;
      #ifdef VERBOSE
      Serial.print("ADC process complete");
      Serial.println();
      #endif
    }
    #endif
  }

  #ifdef VERBOSE
  Serial.println("ADC process end");
//  Serial.print("ADC (complete, running): "); Serial.print(ADC_complete); 
//  Serial.print(", "); Serial.print((ADCSRA & _BV(ADSC))!=0);
//  Serial.println();
  #endif
}
