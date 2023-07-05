/* some alternate, possibly faster mapping functions 
 *  map : 39673   - built in function
 *  imap :14914   - copy of map, using 16bit integers only. bit range of input and output limited to 16 total
 *  lmap : 38538 - copy of map, inputs and output are 16bit, cast to long where needed
 *  ib2map: 643   - copy of map, but input range is a power of 2. eliminates division, thus much faster
 *  ob2map: 39675 - copy of ib2map, but output range is power of 2. no benefit
 */
  
/* map(), but with all inputs and outputs as 16bit integers: (out_range * in_range) must be less than 32k */
int imap(int x, int in_min, int in_max, int out_min, int out_max)
{
  return ((x - in_min) * (out_max - out_min) / (in_max - in_min)) + out_min;
}
/* map(), but with all inputs and outputs as 16bit integers: (out_range * in_range) can exceed 32k */
int lmap( int x,  int in_min,  int in_max,  int out_min,  int out_max)
{
  return (int)((( long)(x - in_min) * (out_max - out_min)) / (in_max - in_min)) + out_min;
}
/* map(), but with long inputs SLOW, but needed for 24bit sensors */
long longmap(long x, long in_min, long in_max, long out_min, long out_max) 
{
  return (long)((long long)(x - in_min) * (out_max - out_min) / (in_max - in_min)) + out_min;
}


/* if input range is a power of two, this function is much faster */
int ib2map(int x, int in_min, byte in_range_log2, int out_min, int out_max)
{
  return ((int)(((long)(x - in_min) * (long)(out_max - out_min)) >> in_range_log2 )) + out_min;
}

/* for mapping ADC values, we can remove some extraneous parameters and add rounding */
int amap(int x, int out_min, int out_max)
{
  long result = ((long)x * (out_max - out_min)) + (1 << 9);
  return (int)(result >> 10) + out_min;
}

void test_map_implementations()
{
  Serial.println(F("Testing mmap:"));

  unsigned long tt = 0;
  unsigned int total = 0;
  for (byte in_max =4; in_max < 15; in_max += 2)
  {
    for (byte out_max =4; out_max < 15; out_max += 2)
    {
      for (byte in_val =0; in_val < in_max; in_val++)
      {
        unsigned int iv = (1<<in_val);//in_val;//
        unsigned int im = (1<<in_max);//in_max;//
        unsigned int om = (1<<out_max);//out_max;//
        unsigned int t;
        
        volatile unsigned int out;
        int n = 1000;

        t = micros();
        while( n--)  
        //{ out = ib2map(iv, 0, im, 0, om );}
        { out = lmap(iv, 0, im, 0, om );}
        //{out = SENSOR_MAP_CAL_MAX_mbar;} //760 val = 170 - both these values are wrong
        //{out = SENSOR_MAP_CAL_MIN_mbar;} //760   val = 220
        //{out = (const int)(1.0);}  //760
        //{out = (const int)(1);} //760
         //{out = 0;} //628
        t = micros()-t;
        
        tt += t;
        total++;

        iv = (1<<in_val);
        im = (1<<in_max);
        om = (1<<out_max);        
        Serial.print(F("dt, o, om, im, i: "));
        Serial.print(t);
        Serial.print(FS(S_COMMA));
        Serial.print(out);
        Serial.print(FS(S_COMMA));
        Serial.print(om);
        Serial.print(FS(S_COMMA));
        Serial.print(im);
        Serial.print(FS(S_COMMA));
        Serial.print(iv);
        Serial.println();
      }
    }
  }
  Serial.print(F("mean time: "));
  Serial.println(tt/total);

  // mmap :14914
  // map : 39673
  // ulmap : 38538
  // ib2map: 643
  // ob2map: 39675
}
