#ifndef __ADC_H__
#define __ADC_H__

#define ADC_UPDATE_INTERVAL_ms          ANALOG_SAMPLE_INTERVAL_ms
#define ADC_SAMPLE_INTERVAL_us          5000                            //4800->1199(105) 4950->1236(105) 4990->1246(100) 4968->1241(105)
#define ADC_SAMPLE_INTERVAL_ms          (ADC_SAMPLE_INTERVAL_us/1000) //5

#define NO_OF_ADC_SLOW_PER_RECORD       (UPDATE_INTERVAL_ms/ADC_UPDATE_INTERVAL_ms)
#define NO_OF_ADC_FAST_PER_SLOW         (ADC_UPDATE_INTERVAL_ms/ADC_SAMPLE_INTERVAL_ms)
#define NO_OF_ADC_FAST_PER_RECORD       (UPDATE_INTERVAL_ms/ADC_SAMPLE_INTERVAL_ms)

#define ADC_INT_DIS()                 ADCSRA &= ~(_BV(ADIE) | _BV(ADIF))
#define ADC_INT_EN()                  ADCSRA &= ~_BV(ADIF); ADCSRA |= _BV(ADIE)

// MAP CALABIRATION
// values specified as Long type to ensure correct evaluation

//each sensor has 2 calibration points
//these points are used to extrapolate MIN and MAX pressure values using a macro below
//this simplifies mapping ADC values to pressure values
// MIN and MAX values are selected at compile time for each MAP channel and stored in ADC_map_cal
// though this could be made configurable, or editable eeprom config values

//CALIBRATION POINTS

// 1BAR MAPS
// calibration values for Lemark LMS184 1 bar MAP sensor 
// (this is from the correct datasheet for the 1BAR MAP I have in stock, so I can use this to ckeck the calibration of generic MAP sensors)
// (strange that this one measures vacuum, but the others measure abs pressure... )
#define SENSOR_MAP_CAL_1BAR_LMS184_HIGH_LSb         39L    //analog sample value for the high cal point
#define SENSOR_MAP_CAL_1BAR_LMS184_LOW_LSb         969L    //analog sample value for the low cal point

#define SENSOR_MAP_CAL_1BAR_LMS184_HIGH_mbar      1050L    //absolute pressure at the high cal point
#define SENSOR_MAP_CAL_1BAR_LMS184_LOW_mbar        200L    //absolute pressure at the low cal point

//calibration values GM 1-bar MAP PN# 16137039 according to http://www.robietherobot.com/storm/mapsensor.htm chart 3
#define SENSOR_MAP_CAL_1BAR_GM3_HIGH_LSb   1023L    //analog sample value for the high cal point
#define SENSOR_MAP_CAL_1BAR_GM3_LOW_LSb      51L    //analog sample value for the low cal point

#define SENSOR_MAP_CAL_1BAR_GM3_HIGH_mbar  1047L    //absolute pressure at the high cal point
#define SENSOR_MAP_CAL_1BAR_GM3_LOW_mbar    150L    //absolute pressure at the low cal point


// 2BAR MAPS
//calibration values for RaceGrade brand M16-9886.2
#define SENSOR_MAP_CAL_2BAR_RG_HIGH_LSb     982L    //analog sample value for the high cal point
#define SENSOR_MAP_CAL_2BAR_RG_LOW_LSb       61L    //analog sample value for the low cal point

#define SENSOR_MAP_CAL_2BAR_RG_HIGH_mbar   2000L    //absolute pressure at the high cal point
#define SENSOR_MAP_CAL_2BAR_RG_LOW_mbar     200L    //absolute pressure at the low cal point

//calibration values for Raceworks brand MAP-003
#define SENSOR_MAP_CAL_2BAR_RW_HIGH_LSb    1003L    //analog sample value for the high cal point
#define SENSOR_MAP_CAL_2BAR_RW_LOW_LSb       61L    //analog sample value for the low cal point

#define SENSOR_MAP_CAL_2BAR_RG_HIGH_mbar   2000L    //absolute pressure at the high cal point
#define SENSOR_MAP_CAL_2BAR_RG_LOW_mbar     200L    //absolute pressure at the low cal point

//calibration values GM 2-bar MAP PN# 16040609 according to http://www.robietherobot.com/storm/mapsensor.htm chart 1
#define SENSOR_MAP_CAL_2BAR_GM1_HIGH_LSb   1003L    //analog sample value for the high cal point
#define SENSOR_MAP_CAL_2BAR_GM1_LOW_LSb      72L    //analog sample value for the low cal point

#define SENSOR_MAP_CAL_2BAR_GM1_HIGH_mbar  1979L    //absolute pressure at the high cal point
#define SENSOR_MAP_CAL_2BAR_GM1_LOW_mbar    334L    //absolute pressure at the low cal point

//calibration values GM 2-bar MAP PN# 16040609 according to http://www.robietherobot.com/storm/mapsensor.htm chart 3
#define SENSOR_MAP_CAL_2BAR_GM3_HIGH_LSb   1023L    //analog sample value for the high cal point
#define SENSOR_MAP_CAL_2BAR_GM3_LOW_LSb      51L    //analog sample value for the low cal point

#define SENSOR_MAP_CAL_2BAR_GM3_HIGH_mbar  2080L    //absolute pressure at the high cal point
#define SENSOR_MAP_CAL_2BAR_GM3_LOW_mbar    180L    //absolute pressure at the low cal point


// 3BAR MAPS
//calibration values GM 3-bar MAP PN# 12223861 according to http://www.robietherobot.com/storm/mapsensor.htm chart 3
#define SENSOR_MAP_CAL_3BAR_GM3_HIGH_LSb    972L    //analog sample value for the high cal point
#define SENSOR_MAP_CAL_3BAR_GM3_LOW_LSb      51L    //analog sample value for the low cal point

#define SENSOR_MAP_CAL_3BAR_GM3_HIGH_mbar  3000L    //absolute pressure at the high cal point
#define SENSOR_MAP_CAL_3BAR_GM3_LOW_mbar    170L    //absolute pressure at the low cal point




// MAP Calibration MIN/MAX values

// ADC min/max
#define SENSOR_MAP_CAL_MAX_LSb    1024L    //adc value for maximum possible pressure value
#define SENSOR_MAP_CAL_MIN_LSb       0L    //adc value for minimum possible pressure value

/* while using these points to map the analog input is technically correct
 *  we can get better performance if the input range is a power of two,
 *  (which it is, being 10bit adc)
 *  so we can rescale HIGH_mbar and LOW_mbar to fit the range
 */
// hopefully this will be evaluated at compile time! //yeah, this seems to be as fast as casting and writing an interger literal, and fractionally slower than a non cast literal
#define SENSOR_CAL_LIMIT_out(in_limit, in_low, in_high, out_low, out_high)   ( ( ((in_limit-in_low) * (out_high-out_low)) / (in_high-in_low)  ) + out_low + 0.5)
//lim=maxlsb=0, in=lim-inlo=-969, outrng=outhi-outlo=850, numr=in*outrng=-823650, inrng=inhi-inlo=-930, outfrac=numr/inrng=885, out=outfrac+outlo=1085
//lim=minlsb=1024, in=lim-inlo=55, outrng=outhi-outlo=850, numr=in*outrng=46750, inrng=inhi-inlo=-930, outfrac=numr/inrng=-50, out=outfrac+outlo=150

#define SENSOR_CAL_LIMIT_MAX( NAME )          (int)SENSOR_CAL_LIMIT_out( (float)SENSOR_MAP_CAL_MAX_LSb, SENSOR_MAP_CAL_##NAME##_LOW_LSb, SENSOR_MAP_CAL_##NAME##_HIGH_LSb, SENSOR_MAP_CAL_##NAME##_LOW_mbar ,SENSOR_MAP_CAL_##NAME##_HIGH_mbar)
#define SENSOR_CAL_LIMIT_MIN( NAME )          (int)SENSOR_CAL_LIMIT_out( (float)SENSOR_MAP_CAL_MIN_LSb, SENSOR_MAP_CAL_##NAME##_LOW_LSb, SENSOR_MAP_CAL_##NAME##_HIGH_LSb, SENSOR_MAP_CAL_##NAME##_LOW_mbar ,SENSOR_MAP_CAL_##NAME##_HIGH_mbar)

//question now is can we pass part of the name in and derive the rest?
//yes, but it's easier if the unique parts are all together in one place in the name

// fill in the blanks...
// #define SENSOR_MAP_CAL__MAX_mbar     SENSOR_CAL_LIMIT_MAX(  )
// #define SENSOR_MAP_CAL__MIN_mbar     SENSOR_CAL_LIMIT_MIN(  )

#define SENSOR_MAP_CAL_1BAR_LMS184_MAX_mbar     SENSOR_CAL_LIMIT_MAX( 1BAR_LMS184 )
#define SENSOR_MAP_CAL_1BAR_LMS184_MIN_mbar     SENSOR_CAL_LIMIT_MIN( 1BAR_LMS184 )

#define SENSOR_MAP_CAL_1BAR_GM3_MAX_mbar     SENSOR_CAL_LIMIT_MAX( 1BAR_GM3 )
#define SENSOR_MAP_CAL_1BAR_GM3_MIN_mbar     SENSOR_CAL_LIMIT_MIN( 1BAR_GM3 )

#define SENSOR_MAP_CAL_2BAR_RG_MAX_mbar     SENSOR_CAL_LIMIT_MAX( 2BAR_RG )
#define SENSOR_MAP_CAL_2BAR_RG_MIN_mbar     SENSOR_CAL_LIMIT_MIN( 2BAR_RG )

#define SENSOR_MAP_CAL_2BAR_RW_MAX_mbar     SENSOR_CAL_LIMIT_MAX( 2BAR_RW )
#define SENSOR_MAP_CAL_2BAR_RW_MIN_mbar     SENSOR_CAL_LIMIT_MIN( 2BAR_RW )

#define SENSOR_MAP_CAL_2BAR_GM1_MAX_mbar     SENSOR_CAL_LIMIT_MAX( 2BAR_GM1 )
#define SENSOR_MAP_CAL_2BAR_GM1_MIN_mbar     SENSOR_CAL_LIMIT_MIN( 2BAR_GM1 )

#define SENSOR_MAP_CAL_2BAR_GM3_MAX_mbar     SENSOR_CAL_LIMIT_MAX( 2BAR_GM3 )
#define SENSOR_MAP_CAL_2BAR_GM3_MIN_mbar     SENSOR_CAL_LIMIT_MIN( 2BAR_GM3 )

#define SENSOR_MAP_CAL_3BAR_GM3_MAX_mbar     SENSOR_CAL_LIMIT_MAX( 3BAR_GM3 )
#define SENSOR_MAP_CAL_3BAR_GM3_MIN_mbar     SENSOR_CAL_LIMIT_MIN( 3BAR_GM3 )


// MAP CALIBRATION SELECTION PER INPUT
#define SENSOR_MAP_CAL_MAX_mbar_0     SENSOR_MAP_CAL_1BAR_LMS184_MAX_mbar
#define SENSOR_MAP_CAL_MIN_mbar_0     SENSOR_MAP_CAL_1BAR_LMS184_MIN_mbar

#define SENSOR_MAP_CAL_MAX_mbar_1     SENSOR_MAP_CAL_2BAR_GM3_MAX_mbar
#define SENSOR_MAP_CAL_MIN_mbar_1     SENSOR_MAP_CAL_2BAR_GM3_MIN_mbar

typedef struct map_cal
{
  int min_mbar, max_mbar;
}MAP_CAL;

#endif // __ADC_H__
