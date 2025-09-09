## GEET Dyno Rectifier Tester Firmware

### What is this?

This is firmware for an Arduino Nano based test circuit for testing and trimming a single phase of the Dyno Rectifier Module.

#### Features

- Control via serial interface, simple single letter command codes.
- Software generated LFO, square wave up to 25Hz.
- PWM output with optional min, max and ramp rates.
- PWM range can include zero (using PWM_INVERT).
- PWM specified by 8bit value, regardless of resolution.
- LFO modulation of PWM.
- Configurable lookup table for boost conversion.
- Configuration stored in EEPROM.
- Override command to instantly set PWM and disable modulation
- Calibration of comparator resistor networks:
    - Buffered DAC to bias the FETs
    - Amp to match DAC range to ADC
    - Selectable gain, approx +/-800mV or +/-80mV
    - Comparator monitor inputs
- Measurement of Input and Output voltages
    - Various gain ranges for both input and output

#### Command Summary

| Code | Name | Parameters | Description |
| ---- | ---- | ---------- | ----------- |
| S | STOP_ALL | N/A | Sets PWM to 0, disables the LFO output and pwm oscillator |
| R | SET_RATE | LFO rate | Sets LFO rate, or toggles LFO when no parameter is supplied |
| P | SET_PWM | PWM ratio | Sets PWM value, restores value after FORCE_PWM if no parameter given |
| F | FORCE_PWM | PWM ratio | Instantly overrides PWM value, ignoring limits |
| G | FORCE_PWM_W | PWM ratio | 16bit version of FORCE_PWM |
| C | SET_PRESCALE | Prescale | Sets clock prescale for PWM timer |
| L | SET_PWM_LIMIT | Min, Max PWM | Sets the minimum and maximum PWM value limits |
| I | SET_PWM_INVERT | Invert flags | bit0: invert input (255-pwm), bit1: invert output (hi/lo) |
| V | SET_PWM_RAMP | Ramp Rate | Sets the rate at which PWM will change on SET_PWM |
| B | SET_PWM_BITS | Bits | Set the number of bits to use for PWM |
| O | OSCILLATE | N/A | Toggles LFO control of PWM, from MIN to MAX at RAMP rate |
| T | USE_LUT | N/A | Toggles use of voltage boost look-up-table |
| W | SAVE_EEP | N/A | Writes current configuration to EEPROM |
| E | LOAD_EEP | N/A | Reads configuration from EEPROM |
| X | RESET_CONFIG | N/A | Resets configuration to default |
| Z | PRINT_CONFIG | N/A | Prints configuration to serial |
| A | PRINT_STATE | N/A | Prints current state to serial |
| Q | RUN_TEST | N/A | Sweeps through a range of pwm values and frequencies, measures voltages and reports |
| K | CALIBRATE_TEST | N/A | Tests the electrical set up for the test |
| M | MEASURE_TEST | N/A | Takes a single measurement with the current configuration and reports |
| D | SET_DAC | ? | Sets the PWM using the DAC configuration |
| H | HELP | N/A | Prints available commands |
| ? | IS_QUERY | N/A | After a command, reports the current values instead of setting it |


### How it works

#### Configuration

##### Config

The following configurable parameters are stored in EEPROM:

| Parameter | Size | Description |
| --------- | ---- | ----------- |
| pwm_prescale | 8 |TCCR1B 0:3 |
| pwm | 8 | programmed target value |
| pwm_osc | 8 | alternate pwm value for oscilation |
| pwm_min | 8 | minimum setable pwm value, values below clamp to zero if 'negate' set |
| pwm_max | 8 | maximum setable pwm value, values above clamp to 255 if 'negate' clear |
| pwm_ramp | 16 | number of PWM cycles between inc/dec of the current pwm value |
| pwm_bits | 8 | number of bits to use for pwm (defines TOP value) |
| pwm_invert | 1 | inverts the polarity of the output pin |
| pwm_negate | 1 | negates the pwm value (255-pwm) prior to writing the reg, allows 0 output |
| oscillate | 1 | LFO sets pwm, switching between .pwm and .pwm_osc every .interval (ms?) |
| enable | 1 | enables LFO state output (on some pin? for what pupose?) |
| lut | 1 | enables the look-up-table when setting pwm, applied prior to negation |
| interval | 32 | timer interval between LFO state changes (ms?) |

##### Calibration

Additionally, 3 calibration points for the DAC and ADC are stored for each of the voltage ranges.

| DAC LSB | ADC LSB | DMM mV |

##### State

The following parameters control pwm, but are not stored in EEPROM:

| Parameter | Description |
| --------- | ----------- |
| osc_time_stamp | Timestamp for next LFO event |
| osc_state | Current LFO state |
| ramp_counter | Counts up to config.pwm_ramp, triggers pwm inc/dec event when reached |
| ramp_value | Current pwm value during ramping |
| ramp_target | Target pwm value during ramping |
| forced | TRUE when force_pwm(_w) called |
| forced_value | Value set by force_pwm(_w), used for glitchless resumption of ramping |

The following parameters control tests, and are not stored in EEPROM:

| Parameter | Description |
| --------- | ----------- |
| test_type | Specifies the test type being performed |
| test_stage | Specifies which stage of testing is next |
| TEST_RAMP_TIME_ms | Specifies... ramping used during test? |
| test_debug | Enables debug info about the testing process |
| test_output_gain_idx | The selected output gain |
| test_input_gain_idx | The selected input gain |
| test_output_overload | Indicates that the output voltage exceeds maximum gain range |
| test_input_overload | Indicates that the input voltage exceeds maximum gain range |

#### Principal PWM functions

| function name | parameters | behaviour description |
| ------------- | ---------- | --------------------- |
| set_pwm() | (byte) pwm | writes pwm to config.pwm, calls set_target(pwm) if not oscilating or osc state is 0 |
| force_pwm() | (byte) pwm | disables oscillation, sets the forced flag and value, and calls write_pwm(pwm) |
| force_pwm_w() | (uint) pwm | as force_pwm(), but accepts a 16 bit value |
| update_pwm() | (byte) pwm | applies the look-up-table if enabled, applies pwm_min/max, calls write_pwm() with the modified value |
| write_pwm() | (uint) pwm | applies pwm bit width, performs negation, writes to timer OC register |
| set_target() | (byte) pwm | sets ramp_target. if forced, sets ramp_value to nearest value to forced_value |

##### PWM frequency table

| Bits \ Prescale | 1 | 2 | 3 | 4 | 5 |
| Clock | 16MHz | 2MHz | 250kHz | 62500Hz | 15625Hz |
| 6 | 250000 | 62500 | 3906.3 | 976.6 | 244.1 |
| 7 | 125000 | 31250 | 1953.1 | 488.3 | 122.0 |
| 8 | 62500 | 7812.5 | 976.6 | 244.1 | 61.04 |
| 9 | 31250 | 3906.3 | 488.3 | 122.1 | 30.52 |
| 10 | 15625 | 1953.1 | 244.1 | 61.0 | 15.26 |
| 11 | 7812.5 | 976.6 | 122.1 | 30.5 | 7.63 |
| 12 | 3906.3 | 488.3 | 61.0 | 15.3 | 3.81 |
| 13 | 1953.1 | 244.1 | 30.5 | 7.6 | 1.91 |
| 14 | 976.7 | 122.1 | 15.3 | 3.8 | 0.95 |

#### Command details

##### STOP_ALL

- Takes no parameters.
- Disables LFO output.
- Disables LFO control of PWM.
- Sets PWM immediately to 0, calling update_pwm().

##### SET_RATE

- If a query (?) returns the LFO time interval.
- Accepts a value in milliseconds. If above INTERVAL_MIN (20ms), sets interval to this value.
- Otherwise toggles LFO output on D13.

##### SET_PWM

- Accepts values from 0 to 255 and calls set_pwm(value).
- If no value provided, calls set_target(config.pwm).

##### FORCE_PWM(_W)

- Accepts values from 0 to 255 (0 to 65535) and calls force_pwm(_w)(value).

##### SET_PRESCALE

- Accepts values from 1 to 5, representing 1, 8, 64, 256 and 1024 clock dividers.
- See PWM frequency table (above).

##### SET_PWM_LIMIT

- Accepts values from -254 to 0 to set config.pwm_min (discarding negative).
- Accepts values from 1 to 255 to set config.pwm_max.

##### SET_PWM_INVERT

- Accepts two boolean values for Negate and Invert respectively.
- Zero is FALSE, non zero is TRUE.
- Negate makes a pwm value of 0 fully ON, and 255 OFF but for one bit.
- Invert makes ON output LOW and OFF output HIGH.
- Setting both Negate and Invert has no effect on polarity, but allows PWM to be reduced to ZERO. This is typically required for regen braking, where fully OFF is required, but fully ON is to be avoided.

##### SET_PWM_RAMP

- Accepts values from 0 to 65535 milliseconds.
- TIMER2_OVF ISR handles ramping pwm values, and has a resolution of 32.5ms.
- Interval value is converted to ISR counts by dividing by 33, and stored in config.pwm_ramp.
- Each ISR, ramp counter is incremented and once config.pwm_ramp has been reached, pwm is inc/decremented.
- Ramp counter is then reset, and ramping continuous until ramp_target is reached.
- A ramp time of 0 disables ramping.

##### SET_PWM_BITS

- Accepts values from 6 to 14. Clamps values to this range. 
- Sets the PWM timer TOP value to 2^(value)-1.
- Updates all registers and parameters accordingly, using ISRs where needed.

##### OSCILLATE

- Accepts values of 0 to 255. Sets config.pwm_osc to this value and enables oscillation.
- If no valid value provided, toggles oscillation.
- When oscillation is enabled, the LFO will alternately set the pwm ramp target to config.pwm and config.pwm_osc.
- Oscillation is disabled when a force_pwm command is given.

##### USE_LUT

- Toggles the config.lut flag.
- With config.lut set, pwm values will be selected from the look-up-table.
- Currently there is only one look-up-table, which converts boost amount in tenths to pwm, and is 8-bit.
- eg. setting PWM to 50 with LUT enabled, will provide approximately 6x boost (1 + 5.0).
- There is no accounting for load or inefficiency, so actual boost values may be lower.

##### SAVE_EEP

- Accepts a single boolean value.
- If TRUE, writes the calibration to EEPROM.
- If FALSE, or omitted, writes the config to EEPROM.

##### LOAD_EEP

- Reloads the configuration from eeprom (including calibration).
- Prints loaded config and cal to serial.
- Has no effect on state, but does call update_pwm, which may interfere with ramping (possible bug).
- Reports failure if CRC does not match (CRC not currently implemented).

##### RESET_CONFIG

- Resets config to default values
- As above, has no effect on state, but may interfere with ramping.

|Config parameter | prescale_n | pwm | pwm_osc | pwm_min | pwm_max | pwm_ramp | pwm_bits | pwm_invert | pwm_negate | oscillate | enable | lut | interval |
|Default value    | 1 | 0 | PWM_MAX | PWM_MIN | PWM_MAX | 1 | 8 | false | true | false | false | true | 500 |

| PWM_MIN | 10 |
| PWM_MAX | 245 |

##### PRINT_CONFIG

- Prints configuration to Serial.
- If a query (?), prints calibration instead.

##### CALIBRATE_TEST

- If a query (?), prints available calibration modes.
- Currently accepts the following values to start the given tests.

0. i/o voltage read test (CALIBRATE)
1. calibrate dac/adc (CAL_DAC)
2. toggle dac/adc range
3. dac sweep up LSB (VERIFY_CAL_UP)
4. dac sweep down LSB (VERIFY_CAL_DN)

##### MEASURE_TEST

- Begins the MEASURE test.

##### SET_DAC

- Accepts a value from 0 to 65535.
- Sets PWM bits to 12 and forces pwm to the supplied value.
- Waits for DAC value to stabilise.


#### Test Types

The following test types are defined:

| Test type | Description |
| NONE | No test. Outputs are as commanded |
| CALIBRATE | Take a single measurement. Provide a custom voltage and check that the output value is correct. Enables debug messages |
| FULL_SWEEP | Perform a full sweep through all bit ranges, from 0 to max_pwm with decremental step ups, starting at 21 |
| MEASURE | Performs a single measurement |
| CAL_DAC | Calibrates the DAC and its monitor ADC channel |
| VERIFY_DAC_CAL_UP | Sweep through some DAC values and read the ADC, print the results including sensor states |
| VERIFY_DAC_CAL_DN | As above, but sweeping down |
| COMPARATOR_LO | Tests the UUT's upper and lower comparator thresholds at 0v using J9 Test Connector |
| COMPARATOR_HI | Tests the UUT's comparator thresholds at -20V (using which connector?) |

Tests are broken down into the following steps:

| IDLE | Test is not running or has completed |
| SETUP | Configures the outputs as required for the test |
| SAMPLE | Take ADC readings of input and output until they are stable |
| REPORT | Prints the collected data to serial |
| NEXT | Selects the next test configuration |

##### Principal Test Functions:

###### test_print_ratios()

Prints input and output voltage measurment gains based on hard coded resistor values.
Resistor values must be accurately measured and entered into the code prior to compiling and uploading.

- print input gain resistors
- print gains and gain steps for these resistors
- print output gain resistors
- print gains and gain steps for these resistors

###### test_stop()

Resets test state.

- sets ouptut and input gains to 0
- sets test type to NONE
- sets test stage to IDLE
- disables test_debug

###### test_reset()

As above, but also calls set_pwm(0).

###### test_setup()

Sets the configuration according to the test parameters.

- calls set_pwm(test_pwm)
- calls test_ramp_delay()
- calls set_pwm_bits(test_bits)
- increments test_stage and test_setup_idx

###### test_sample()

Takes ADC measurments of Input and Output voltages, iteratively adjusts gain range until both are in range, or overflowed.
Prints values each iteration if test_debug enabled.

- Sets gains to 0 (lowest gain)
- calls test_sample_read() to perform ADC reads
- checks the spread of samples
- repeats reads until spread is within tolerance, uses average of repeats if persistantly out of tolerance
- adjusts gains to maximise resolution, repeats measurements
- resets output gain to 0 (why not input?)

###### test_sample_read()

Params:
- sample: a pointer to the results sample struct to write to.

Handles multisampling of ADC channels for Input and Output.

###### test_sample_adjust_gain()

Params:
- sample: last result from test_sample_read
- is_output: flag to indicate if this was an input or output sample

Handles automatic gain adjustment.


#### Test Details

##### CALIBRATE

- Take a single measurement.
- Provide a custom voltage and check that the output value is correct. (which means what?)
- Enables debug messages (allegedly, presumably this means enabling test_debug, which prints selected gain ranges)

I'm still not sure what exactly this does.

###### Start Test:

- disables LUT
- test ramp time from pwm ramp time
- start pwm = 0
- start pwm bits = 8
- pwm inc = 0

###### SETUP:

- call test_print_ratios()
- call test_setup()

###### SAMPLE:

- call test_sample()

###### REPORT:

- call test_report()

###### NEXT:

- call test_reset()


TODO: finish summarising the various tests.
