/*
 * This is a test bed for the mixed rate ADC process
 */


/*          event         duration  
 *                      typ.      max.    log sdcard    log serial                                                      min gap to next, max if different
 *      F - finalise record                                             000                                     500     40    80ms till next spi use
 *      A - analog                                                      070     170     270     370     470     570     10
 *      
 *  2.5 ms per char
 *      010     030     050     070     090     110     130     150     170     190     210     230     250     270     290     310     330     350     370     390     410     430     450     470     490
 *  000     020     040     060     080     100     120     140     160     180     200     220     240     260     280     300     320     340     360     380     400     420     440     460     480     500
 *  F                           A                                       A                                       A                                       A                                       A       
 *  * 1 2 3 4 5 6 7 8 9 0 1 2 3 * 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 * 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 * 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 * 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 * 1 2 3 4 5 6
 *      s           p               t   p       e           p               t   p   r               p               t   p   d               p       e       t   p   f               p               t   p        
 *      
 *  F   s           p           A   t   p       e           p           A   t   p   r               p           A   t   p   d               p       e   A   t   p   f               p           A   t   p   F     
 *  
 *  Note that the fast adc samples overlap the record update.
 *  It might be more convenient to write directly to the record, rather than have an intermediate buffer
 *  for averages, have an accumulator that is read cleared by which ever process needs the average (ie, PID or Record Update)
 *  the slow adc process would only need to reset the timer and disable the interrupt
 *  there may be up to 10 slow channels, each taking 104us, so a total of 1-2ms to process all of them before another round of fast samples is needed
 *  
 *  NB: Timer0 is used for millis() and micros(), not Timer1 as I had previously thought.
 */

#define ADC_NO_SLOW sizeof(channels_slow)
#define ADC_NO_FAST sizeof(channels_fast)

#define CURRENT_RECORD    Data_Records[Data_Record_write_idx]
#define LAST_RECORD       Data_Records[1-Data_Record_write_idx]
#define SWAP_RECORDS()    Data_Record_write_idx = 1-Data_Record_write_idx


#define RECORD_UPDATE_INTERVAL_ms       500
#define ADC_UPDATE_INTERVAL_ms          100
#define ADC_SAMPLE_INTERVAL_ms          5
#define NO_OF_ADC_SLOW_PER_RECORD       (RECORD_UPDATE_INTERVAL_ms/ADC_UPDATE_INTERVAL_ms)
#define NO_OF_ADC_FAST_PER_SLOW         (ADC_UPDATE_INTERVAL_ms/ADC_SAMPLE_INTERVAL_ms)
#define NO_OF_ADC_FAST_PER_RECORD       (RECORD_UPDATE_INTERVAL_ms/ADC_SAMPLE_INTERVAL_ms)

#define RECORD_UPDATE_START_ms        (UPDATE_INTERVAL_ms + 0)      //setting update loops to start slightly offset so they aren't tying to execute at the same time
#define ANALOG_UPDATE_START_ms        70


#define CURRENT_RECORD_IDX            (Data_Record_write_idx)
#define LAST_RECORD_IDX               (1-Data_Record_write_idx)
#define SWAP_RECORDS()                Data_Record_write_idx = LAST_RECORD_IDX
#define CURRENT_RECORD                Data_Records[CURRENT_RECORD_IDX]
#define LAST_RECORD                   Data_Records[LAST_RECORD_IDX]

//config for timer 0 to trigger adc
#define ADC_TMR_PRESCALE              (3<<CS10)       // div by 64, clk 250kHz

#define ADC_TMR_STOP()                TCCR1B &= ~(_BV(CS10)|_BV(CS11)|_BV(CS12))
#define ADC_TMR_START()               TCCR1B |= ADC_TMR_PRESCALE
#define ADC_TMR_CLR()                 TCNT1 = 0

#define ADC_TMR_CFG()                 TCCR1A = 0; TCCR1B = _BV(WGM12);    //CTC, TOP = OCR1A
#define ADC_TMR_TOP                   (1250-1)
#define ADC_TMR_TRIG                  1
#define ADC_TMR_SET()                 OCR1A = ADC_TMR_TOP; OCR1B = ADC_TMR_TRIG;

#define ADC_SET_TRIG_ADC()            ADCSRB &= ~0x7   //free running
#define ADC_SET_TRIG_TMR()            ADCSRB |= 0x5   //trig on OC1B.

#define ADC_TMR_CLRF()                TIFR1 |= _BV(OCF1B)

#define ADC_TRIG_DIS()                ADCSRA &= ~_BV(ADATE)
#define ADC_TRIG_EN()                 ADCSRA |= _BV(ADATE)

#define ADC_INT_DIS()                 ADCSRA &= ~(_BV(ADIE) | _BV(ADIF))
#define ADC_INT_EN()                  ADCSRA &= ~_BV(ADIF); ADCSRA |= _BV(ADIE)

#define ADC_START()                   ADCSRA |= _BV(ADSC)

#define ADC_COMPLETE()                (ADCSRA & _BV(ADIF))
#define ADC_CLR_FLAG()                ADCSRA |= _BV(ADIF)

byte channels_slow[] = {0,3,4}; //channels to sample at low rate
byte channels_fast[] = {1,2};       //channels to sample at high rate
byte adc_chn;                         //channel index, increment each adc interrupt
byte adc_smp;                         //sample index, increment each adc interrupt if chn overflows

int adc_readings_slow[ADC_NO_SLOW];

//accumulator for pid, each pid has it's own
//this needs some thought if more than 1 pid needs to do this
int pid_total;
byte pid_count;
char pid_chn = -1;  //fast channel to read, -1 for none

typedef struct data_averages
{
  int USR[ADC_NO_SLOW];
  int MAP[ADC_NO_FAST];
}DATA_AVG;
typedef struct data_record
{
  unsigned long timestamp;

//sensors using the internal ADC, or HX771 at 10Hz
  byte ANA_no_of_slow_samples;
  byte ANA_no_of_fast_samples;
  int USR[ADC_NO_SLOW][NO_OF_ADC_SLOW_PER_RECORD];
  int MAP[ADC_NO_FAST][NO_OF_ADC_FAST_PER_RECORD];
}DATA_RECORD;

char (*__kaboom)[sizeof( DATA_RECORD )] = 1;

/* the records to write to */
DATA_RECORD Data_Records[2] = {0};
DATA_AVG Data_Averages;
/* the index of the currently written record */
byte Data_Record_write_idx;


//this should help with accessing specific sensor types within the record structure
const unsigned int record_offsets[] = {(unsigned int)((byte*)&Data_Records[0].USR - (byte*)&Data_Records[0]), (unsigned int)((byte*)&Data_Records[0].MAP - (byte*)&Data_Records[0])};


static int timestamp_adc_ms;
static int timestamp_record_ms;

volatile static int isr_duration_tk;

bool isr_print = false;

/* select the adc channel to sample next time */
void ADC_set_channel(byte chn)
{
  //ADC is in two banks of 8, ADMUX set the bank's channel and voltage reference (set to ext AREF)
  ADMUX = (chn & 0x7);
  //select the bank
  if((chn & 0x8)==0)
    ADCSRB &= ~_BV(MUX5);
  else
    ADCSRB |= _BV(MUX5);
}


void configure_ADC()
{
  //disable the digital inputs and outputs
  DIDR0 = 0xFF;
  DIDR1 = 0xFF;
  PORTF=0; 
  PORTK=0;
  DDRF=0;
  DDRK=0;

  //enable the ADC and set prescaler to slowest rate
  ADCSRA = _BV(ADEN) | 0x7;

  /* only timers 0 and 1 can trigger ADC directly without needing an ISR
   *  timer0 is used for millis() and micros()
   *  timer1 is 16bit
   *  
   *  CTC mode for T1,3,4,5 is 0x4 (WGMn), WGM12 is bit 3 of TCCRB
   *  timer1 in CTC mode does not have an overflow interrupt
   *  ADC must be driven from OC1B, TOP is set by OC1A
   *
   * according to the datasheet, in CTC mode, frequency is:
   * f(OC1A) = f(clk_IO) / (2*N*(1+OCR1A))
   * T(OC1A) = (2*N*(1+OCR1A)) / f(clk_IO)
   * OCR1A = (T(OC1A) * f(clk_IO) / (2*N)) -1
   * 
   * N = prescale factor; f_clk = 16MHz, F_CPU
   * so 5ms, ps/64 would be
   * OCR1A = (5m * 16M / 128) - 1
   *       = 624 wrong...
   * prescaler        clk    time per tick   ticks per 5ms
   * 1  /1    >>0     16M   62.5n            80000     *this is the only option that will not work
   * 2  /8    >>3     2.0M  500n             10000
   * 3  /64   >>6     250k  4us               1250
   * 4  /256  >>8     62.5k 16us               312.5
   * 5  /1024 >>10    15.6k 64us                78.125     
   * 
   * actual rate @ 15kHz, OCR1A=78: 4.992ms (1 sample)
   *                                99.84ms (20 samples)   
   * this would give us an extra 160us between the last sample and the slow adc process, 
   * it also would mean that the ISR will have to stop itself after 20 readings
   * and allow the slow ADC process to restart it
   * should proably do this anyway
   */

  // set timer 0 to CTC (mode 2, no output compare) (also clears default arduino config for TMR1)
  ADC_TMR_CFG();
  // clear timer 0
  ADC_TMR_CLR();
  // setup timer 0 to reset every 5ms
  ADC_TMR_SET();

}

void setup() {
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

  configure_ADC();
  ADC_start_fast();

  int timenow_ms = millis();
  timestamp_adc_ms = timenow_ms+ADC_UPDATE_INTERVAL_ms;
  timestamp_record_ms = timenow_ms+RECORD_UPDATE_INTERVAL_ms;

  
}

void loop() {
  int timenow_ms = millis();
  
  int elapsed_ms = timestamp_adc_ms - timenow_ms;
  if(elapsed_ms <= 0)
  {
    timestamp_adc_ms = timenow_ms+ADC_UPDATE_INTERVAL_ms;
    process_ADC();
    timenow_ms = millis();
  }
  
  elapsed_ms = timestamp_record_ms - timenow_ms;
  if(elapsed_ms <= 0)
  {
    timestamp_record_ms = timenow_ms+RECORD_UPDATE_INTERVAL_ms;
    process_Record();
    timenow_ms = millis();
    
  }
}
/* call to stop the interrupt driven ADC process */
void ADC_stop_fast()
{
  ADC_TRIG_DIS();
  ADC_INT_DIS();
  ADC_TMR_STOP();
}

/* call to start the interrupt driven ADC process */
void ADC_start_fast()
{
  adc_chn = 0;
  //select the first fast channel
  ADC_set_channel(channels_fast[adc_chn]);
  //enable the ADC interrupt
  ADC_INT_EN();
  //set the timer as the trigger source
  ADC_SET_TRIG_TMR();
  //clear the timer
  ADC_TMR_CLR();
  //enable the trigger
  ADC_TRIG_EN();
  //start the timer
  ADC_TMR_START();
  
}

/* call to process slow adc samples */
void process_ADC()
{
  
  //make sure the fast process isn't running
  ADC_stop_fast();
  
  
  //slow channels
  for(byte n = 0; n < ADC_NO_SLOW; n++)
  {
    //set channel
    ADC_set_channel(channels_slow[n]);
    //start ADC
    ADC_START();
    //wait for conversion
    while(! ADC_COMPLETE() )
    {
      //Serial.println(ADCSRA,HEX);
    }
    //read value
    adc_readings_slow[n] = ADC;
    //Serial.println(ADC);
    //clear the interrupt flag
    ADC_CLR_FLAG();
  }
  //fast channels
  adc_smp = 0;          //reset the fast sample counter
  ADC_start_fast(); //start the fast adc process

  //check for space in the record
  if(CURRENT_RECORD.ANA_no_of_slow_samples < NO_OF_ADC_SLOW_PER_RECORD)
  {
    //process each type of input
    #define ADC_USR_START 0
    #define ADC_USR_END ADC_NO_SLOW
    #define ADC_TMP_START ADC_NO_SLOW
    #define ADC_TMP_END ADC_NO_SLOW
    for(byte n = ADC_USR_START; n < ADC_USR_END; n++)
    {
      CURRENT_RECORD.USR[n][CURRENT_RECORD.ANA_no_of_slow_samples] = adc_readings_slow[n];
    }
    for(byte n = ADC_TMP_START; n < ADC_TMP_END; n++)
    {
      //calibrate value
      adc_readings_slow[n] = adc_readings_slow[n];
      CURRENT_RECORD.USR[n][CURRENT_RECORD.ANA_no_of_slow_samples] = adc_readings_slow[n];
    }
    CURRENT_RECORD.ANA_no_of_slow_samples++;
  }
}

/* call to update record */
void process_Record()
{
  //usual swap records with interrupts suspended
  //disable ICP and ADC interrupts
  //swap records
  SWAP_RECORDS();
  CURRENT_RECORD = (DATA_RECORD){0};
  CURRENT_RECORD.timestamp = millis();
  
  //reenable ICP and ADC interrupts

  //calibrate and find averages for fast channels
  if(LAST_RECORD.ANA_no_of_fast_samples)
  {
    //Serial.println(LAST_RECORD.ANA_no_of_fast_samples);
    for(byte c = 0; c < ADC_NO_FAST; c++)
    {
      unsigned long total = 0;
      for(byte n = 0; n < LAST_RECORD.ANA_no_of_fast_samples; n++)
      {
        //calibrate value
        LAST_RECORD.MAP[c][n] = LAST_RECORD.MAP[c][n];
        //accumulate value for average
        total += LAST_RECORD.MAP[c][n];
      }
      //get average (and store)
      total /= LAST_RECORD.ANA_no_of_fast_samples;
      Data_Averages.MAP[c] = total;
    }
  }

  //find averages of slow channels (already calibrated)
  int no_samps = LAST_RECORD.ANA_no_of_slow_samples;
  if(no_samps)
  {
    //Serial.println(LAST_RECORD.ANA_no_of_slow_samples);
    for(byte c = 0; c < ADC_NO_SLOW; c++)
    {
      unsigned int total = 0;
      
      for(byte n = 0; n < no_samps; n++)
      {
        //calibrate value
        LAST_RECORD.USR[c][n] = LAST_RECORD.USR[c][n];
        //accumulate value for average
        total += LAST_RECORD.USR[c][n];
      }
      //get average (and store)
      total /= no_samps;
      Data_Averages.USR[c] = total;
    }
  }
  
  //fetch isr duration (not including header/footer)
  int isr_time = isr_duration_tk;
  isr_time<<=2; //convert to us
  Serial.print(isr_time); Serial.println(" us");
  
  /// now print out some data
  for(byte c = 0; c < ADC_NO_SLOW; c++)
  {
    Serial.print(Data_Averages.USR[c]); Serial.print(";  ");
    for(byte n = 0; n < LAST_RECORD.ANA_no_of_slow_samples; n++)
    {
      Serial.print(LAST_RECORD.USR[c][n]); Serial.print(", ");
    }
    Serial.println();
  }
  
  for(byte c = 0; c < ADC_NO_FAST; c++)
  {
    Serial.print(c); Serial.print(": ");
    Serial.print(Data_Averages.MAP[c]); Serial.print(";  ");
    for(byte n = 0; n < LAST_RECORD.ANA_no_of_fast_samples; n++)
    {
      Serial.print(LAST_RECORD.MAP[c][n]); Serial.print(", ");
    }
    Serial.println();
  }
  Serial.println();
}


ISR(ADC_vect)
{
  //clear the timer interrupt flag
  ADC_TMR_CLRF();
  
  isr_duration_tk = TCNT1;
  static long tl = micros();
  long t_now = micros();
  long dt = t_now - tl;
  tl = t_now;
  if(isr_print){Serial.print("ISR: ");Serial.print(dt);Serial.print("; ");}

  //accumulate for pid
  if (pid_chn == adc_chn && pid_count < 255)
  {
    pid_total += ADC;
    pid_count++;
  }
  //store value
  if(CURRENT_RECORD.ANA_no_of_fast_samples < NO_OF_ADC_FAST_PER_RECORD)
  {
    CURRENT_RECORD.MAP[adc_chn][CURRENT_RECORD.ANA_no_of_fast_samples] = ADC;
  }
  //increment channel         (alt do nothing)
  adc_chn++;
  //if next channel is valid, (alt if not valid)
  if(adc_chn < ADC_NO_FAST)
  {
    if(isr_print){Serial.print("chn: "); Serial.println(adc_chn);}
    //start adc               (alt disable autotirgger)
    ADC_set_channel(channels_fast[adc_chn]);
    ADC_START();
  }
  else
  {
    //increment ADC process count
    adc_smp++;
    if(isr_print){Serial.print("smp: "); Serial.println(adc_smp);}
    //increment record sample count
    if(CURRENT_RECORD.ANA_no_of_fast_samples < NO_OF_ADC_FAST_PER_RECORD);
    {
      CURRENT_RECORD.ANA_no_of_fast_samples++;
    }
    
    //check if this is the last sample in this ADC process cycle
    if(adc_smp < NO_OF_ADC_FAST_PER_SLOW)
    {
      //back to first channel
      adc_chn = 0;
      ADC_set_channel(channels_fast[adc_chn]);
    }
    else
    {
      if(isr_print){Serial.println("stop");}
      //stop ADC fast handling
      ADC_stop_fast();
    }
  }
  isr_duration_tk = TCNT1 - isr_duration_tk;
  //Serial.println();
}

// end of file
