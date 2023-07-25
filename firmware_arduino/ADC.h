#ifndef __ADC_H__
#define __ADC_H__

#define ADC_UPDATE_INTERVAL_ms          ANALOG_SAMPLE_INTERVAL_ms
#define ADC_SAMPLE_INTERVAL_ms          5
#define NO_OF_ADC_SLOW_PER_RECORD       (UPDATE_INTERVAL_ms/ADC_UPDATE_INTERVAL_ms)
#define NO_OF_ADC_FAST_PER_SLOW         (ADC_UPDATE_INTERVAL_ms/ADC_SAMPLE_INTERVAL_ms)
#define NO_OF_ADC_FAST_PER_RECORD       (UPDATE_INTERVAL_ms/ADC_SAMPLE_INTERVAL_ms)

#define ADC_INT_DIS()                 ADCSRA &= ~(_BV(ADIE) | _BV(ADIF))
#define ADC_INT_EN()                  ADCSRA &= ~_BV(ADIF); ADCSRA |= _BV(ADIE)


//calibration values for Lemark LMS184 1 bar MAP sensor
//specifying them as Long type to ensure correct evaluation
#define SENSOR_MAP_CAL_1BAR_HIGH_LSb     39L    //analog sample value for the high cal point
#define SENSOR_MAP_CAL_1BAR_LOW_LSb     969L    //analog sample value for the low cal point

#define SENSOR_MAP_CAL_1BAR_HIGH_mbar  1050L    //absolute pressure at the high cal point
#define SENSOR_MAP_CAL_1BAR_LOW_mbar    200L    //absolute pressure at the low cal point

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
#define SENSOR_MAP_CAL_1BAR_MAX_mbar   (int)SENSOR_CAL_LIMIT_out( (float)SENSOR_MAP_CAL_MAX_LSb, SENSOR_MAP_CAL_1BAR_LOW_LSb, SENSOR_MAP_CAL_1BAR_HIGH_LSb, SENSOR_MAP_CAL_1BAR_LOW_mbar ,SENSOR_MAP_CAL_1BAR_HIGH_mbar)
#define SENSOR_MAP_CAL_1BAR_MIN_mbar   (int)SENSOR_CAL_LIMIT_out( (float)SENSOR_MAP_CAL_MIN_LSb, SENSOR_MAP_CAL_1BAR_LOW_LSb, SENSOR_MAP_CAL_1BAR_HIGH_LSb, SENSOR_MAP_CAL_1BAR_LOW_mbar ,SENSOR_MAP_CAL_1BAR_HIGH_mbar)

typedef struct map_cal
{
  int min_mbar, max_mbar;
}MAP_CAL;

#endif // __ADC_H__
