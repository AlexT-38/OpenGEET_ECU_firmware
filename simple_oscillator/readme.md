# GEET Dyno Rectifier Tester Firmware

## What is this?

This is firmware for an Arduino Nano based test circuit for testing and trimming a single phase of the Dyno Rectifier Module.

### Features

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

### Command Summary

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

## How to use it

To prepare a synchronous rectifier phase board, there are currently two basic tests:

1. Comparator Resistor Network Balancing
2. Sweep Test / Functional Test

### Comparator Resistor Network Balancing

Resistor networks are balanced by measuring the voltage thresholds at which the comparators switch.
The low side FET  only switches when the phase voltage is zero, which simplifies testing and balancing.
The high side FET switches when phase voltage is equal to bus voltage, which complicates testing and balancing, as both thresholds will change depending on voltage.
For the high side, thresholds must be measured at both 0V and close to the maximum operating voltage, and the adjustment factors are calculated simultaneously.
Presently only the switch-off threshold is adjusted, as this is more critical than the switch-on threshold which merely needs to be less than the forward voltage drop across the FET's body diode.

The tester provides a voltage difference to the comparators and checks for a change in comparator output.
The DAC must be calibrated before use. Calibration is only valid for a single invert/negate configuration, though only one config should be needed.


#### DAC Calibration

1. On either J9 or J10 (LO and HI test connectors) connect pins 7 and 10
2. Connect a good DMM set to measure Vdc to J9/10 pin 7 and ground (J8-8, J9/10-3/8)
3. Enter command `K1`
4. Enter values read from DMM when requested

#### DAC Verification

1. Use the same setup as for calibration
2. Clear the terminal and enter command `K3`, copy the results to a spreadsheet for analysis
3. Clear and enter command `K4`, copy the results to the spreadsheet.
4. Enter command 'K2' to toggle the range and repeat steps (2) and (3)
5. Check the data in the spreadsheet (details?)
6. If the calibration is correct, enter command `W1` to write the calibration data to EEPROM

#### COMP Test

This procedure needs clarification.

1. The FET driver chip must be shut down, or removed and the FET gates tied to their respective sources.
2. Connect the Unit Under Test to the LO test connector (J9) with the test loom.
    - Croc clip connects to V_BUS, connecting it to GND on the Tester.
    - Crimp connects to V_PHASE, connecting it to DAC/V_PHASE on the Tester.
    - High side FET will see -DAC V, Low side FET sees +DAC V.
3. Connect the tester to a power supply with +12V and -20V outputs, and then connect USB to the tester.
4. Ensure DAC range is set to HIGH. (TODO)Use command `A` to get the selected range and enter command `K2` to toggle.
5. Enter command `K3`, check that switch-on occurs at around 450mV. Ignore switch-off.
6. Enter command `K4`, record the threshold voltages.
7. Set DAC range to LO by entering command `K2`.
8. Enter command `K3`. Record thresholds. Switch-offs should occur around 0v, switch-ons should not occur.
9. Enter `K2` twice. It is nescessary to switch from low to high DAC range and back again to trip the low side comparator into the on state.
10. Enter command `K4`. Record thresholds.
11. Disconnect the tester from USB and turn off the PSU.
12. Move the test loom from J9 (LO) to J10 (HI).
13. Turn on the PSU and reconnect USB. If necessary, reconnect the terminal/serial monitor to the USB com port.
14. Connect a good DMM to V0 and VBUS on the UUT, set to measure Vdc.
15. Use `K2` to ensure that the high DAC range is selected.
16. Enter `D65535` to ensure that DAC output is below the switch-on threshold for the high side comparator.
17. Enter `K4` to run the sweep up test. Record thresholds for channel 2 only. Ignore channel 0 and 1.
18. Enter `K2` to switch to the low DAC range.
19. Enter `K3` to sweep up. Record thresholds for channel 2 only. This should be the switch-off voltage.

We are aiming to have a switch-on voltage below the saturation voltage, say around 450 mV.
And a switch-off voltage as close as possible to 0 mV while still being the same polarity as the switch-on voltage.

##### Results 

Results from the K3/4 commands have the following format:

|DAC|DAC|ADC|ADC|ADC Tol|Cl|Ch|Ch-|
|-|-|-|-|-|-|-|-|
|LSB|mV|LSB|mV|mV|Bool|Bool|Bool|

ADC Tolerance is constant for each range setting. Included in the results for convenience.
Cl and Ch are the comparator states for low and high side fets respectively, when testing using the LO test port.
Ch- is the comparator state for the high side fet when testing using the HI test port.

Note that the polarity of Ch- is inverted, so 1 means off and 0 means on.

The test concludes with a summary of the threshold voltages:

```
Comp Thresholds:
0, <DAC mV>, <ADC mV>
1, <DAC mV>, <ADC mV>
2, <DAC mV>, <ADC mV>
```
0 is the low side, 1 is the high side. For a sweep down test, `K3`.
Only comparators that switched state are listed.

##### Example

Starting a sweep down test:

```
>K3

K: TEST_CALIBRATE
Starting DAC verify
DAC LSB, DAC mV, ADC LSB, ACD mV, ADC Tol mV, Cl, Ch, Ch-
16, 758.96, 2000, 592.64, 6.14, 0, 1, 1
32, 758.59, 2000, 592.64, 6.14, 0, 1, 1
48, 758.22, 2001, 592.55, 6.14, 0, 1, 1

...

```

Test reaches the saturation point of the FET body diode:

```
...

7200, 592.43, 2006, 592.07, 6.14, 0, 1, 1
7216, 592.06, 2007, 591.97, 6.14, 0, 1, 1
7232, 591.69, 2006, 592.07, 6.14, 0, 1, 1
7248, 591.32, 2004, 592.26, 6.14, 0, 1, 1
7264, 590.95, 2005, 592.17, 6.14, 0, 1, 1
7280, 590.58, 2005, 592.17, 6.14, 0, 1, 1
7296, 590.21, 2003, 592.36, 6.14, 0, 1, 1
7312, 589.84, 2006, 592.07, 6.14, 0, 1, 1
7328, 589.47, 2004, 592.26, 6.14, 0, 1, 1
7344, 589.10, 2004, 592.26, 6.14, 0, 1, 1
7360, 588.72, 2008, 591.88, 6.14, 0, 1, 1
7376, 588.35, 2013, 591.40, 6.14, 0, 1, 1
7392, 587.98, 2017, 591.02, 6.14, 0, 1, 1
7408, 587.61, 2020, 590.73, 6.14, 0, 1, 1
7424, 587.24, 2025, 590.25, 6.14, 0, 1, 1
7440, 586.87, 2029, 589.86, 6.14, 0, 1, 1
7456, 586.50, 2036, 589.19, 6.14, 0, 1, 1
7472, 586.13, 2040, 588.81, 6.14, 0, 1, 1
7488, 585.76, 2043, 588.52, 6.14, 0, 1, 1
7504, 585.39, 2045, 588.33, 6.14, 0, 1, 1
7520, 585.02, 2048, 588.04, 6.14, 0, 1, 1
7536, 584.64, 2049, 587.95, 6.14, 0, 1, 1

...
```

Test reaches high side switch-off voltage:

```
...

32704, 1.24, 8121, 5.84, 6.14, 0, 1, 1
32720, 0.87, 8124, 5.55, 6.14, 0, 1, 1
32736, 0.50, 8128, 5.17, 6.14, 0, 1, 1
32752, 0.13, 8133, 4.69, 6.14, 0, 1, 1
32768, -0.24, 8137, 4.31, 6.14, 0, 0, 1
32784, -0.61, 8141, 3.92, 6.14, 0, 0, 1
32800, -0.98, 8145, 3.54, 6.14, 0, 0, 1
32816, -1.35, 8149, 3.16, 6.14, 0, 0, 1

...
```

Test reaches low side switch-on voltage:

```
...

50960, -423.77, 12547, -419.26, 6.15, 0, 0, 1
50976, -424.14, 12553, -419.84, 6.15, 0, 0, 1
50992, -424.52, 12555, -420.03, 6.15, 0, 0, 1
51008, -424.89, 12559, -420.42, 6.15, 0, 0, 1
51024, -425.26, 12562, -420.70, 6.15, 1, 0, 1
51040, -425.63, 12568, -421.28, 6.15, 1, 0, 1
51056, -426.01, 12571, -421.57, 6.15, 1, 0, 1
51072, -426.38, 12575, -421.95, 6.15, 1, 0, 1

...
```

Test reaches low-side saturation voltage:

```
...

8720, -604.53, 14393, -596.61, 6.15, 1, 0, 1
58736, -604.90, 14399, -597.19, 6.15, 1, 0, 1
58752, -605.27, 14400, -597.28, 6.15, 1, 0, 1
58768, -605.65, 14403, -597.57, 6.15, 1, 0, 1
58784, -606.02, 14407, -597.96, 6.15, 1, 0, 1
58800, -606.39, 14412, -598.44, 6.15, 1, 0, 1
58816, -606.77, 14415, -598.73, 6.15, 1, 0, 1
58832, -607.14, 14416, -598.82, 6.15, 1, 0, 1
58848, -607.51, 14420, -599.21, 6.15, 1, 0, 1
58864, -607.88, 14426, -599.78, 6.15, 1, 0, 1
58880, -608.26, 14430, -600.17, 6.15, 1, 0, 1
58896, -608.63, 14430, -600.17, 6.15, 1, 0, 1
58912, -609.00, 14432, -600.36, 6.15, 1, 0, 1
58928, -609.37, 14433, -600.46, 6.15, 1, 0, 1
58944, -609.75, 14433, -600.46, 6.15, 1, 0, 1
58960, -610.12, 14433, -600.46, 6.15, 1, 0, 1
58976, -610.49, 14433, -600.46, 6.15, 1, 0, 1
58992, -610.87, 14433, -600.46, 6.15, 1, 0, 1
59008, -611.24, 14433, -600.46, 6.15, 1, 0, 1

...
```

Test completes:

```
...
65456, -761.44, 14433, -600.46, 6.15, 1, 0, 1
65472, -761.81, 14432, -600.36, 6.15, 1, 0, 1
65488, -762.18, 14433, -600.46, 6.15, 1, 0, 1
65504, -762.55, 14433, -600.46, 6.15, 1, 0, 1

Comp Thresholds:
0, -425.26, -420.70
1, -0.24, 4.31
------------------------
Elapsed test time (ms): 59348
------------------------
```

Now we run the test the other way.

```
>K4

K: TEST_CALIBRATE
Starting DAC verify
DAC LSB, DAC mV, ADC LSB, ACD mV, ADC Tol mV, Cl, Ch, Ch-
65504, -762.55, 14432, -600.36, 6.15, 1, 0, 1
65488, -762.18, 14432, -600.36, 6.15, 1, 0, 1
65472, -761.81, 14432, -600.36, 6.15, 1, 0, 1

...

64, 757.85, 2000, 592.64, 6.14, 0, 1, 1
48, 758.22, 2000, 592.64, 6.14, 0, 1, 1
32, 758.59, 2000, 592.64, 6.14, 0, 1, 1
16, 758.96, 2000, 592.64, 6.14, 0, 1, 1

Comp Thresholds:
0, 8.66, 3.16
1, 441.48, 436.67
------------------------
Elapsed test time (ms): 59349
------------------------

```

Now we switch the DAC to the low range and repeat.

```
>K2
K: TEST_CALIBRATE
>K3
K: TEST_CALIBRATE
Starting DAC verify
DAC LSB, DAC mV, ADC LSB, ACD mV, ADC Tol mV, Cl, Ch, Ch-
16, 80.33, 1344, 75.12, 0.70, 0, 1, 1
32, 80.29, 1344, 75.12, 0.70, 0, 1, 1
48, 80.25, 1344, 75.12, 0.70, 0, 1, 1
64, 80.21, 1344, 75.12, 0.70, 0, 1, 1

...

65456, -79.40, 15456, -79.87, 0.70, 0, 0, 1
65472, -79.44, 15456, -79.87, 0.70, 0, 0, 1
65488, -79.47, 15456, -79.87, 0.70, 0, 0, 1
65504, -79.51, 15458, -79.90, 0.70, 0, 0, 1

Comp Thresholds:
1, 5.60, 5.44
------------------------
Elapsed test time (ms): 92465
------------------------
```

If we run back up with `K4`, we won't get a result, because the low side voltage hasn't reached the switch on voltage yet.
We can fix this by toggling back to the high range and back.

```
>K2
K: TEST_CALIBRATE
>K2
K: TEST_CALIBRATE
>K4
K: TEST_CALIBRATE
Starting DAC verify
DAC LSB, DAC mV, ADC LSB, ACD mV, ADC Tol mV, Cl, Ch, Ch-
65504, -79.51, 15480, -80.14, 0.70, 1, 0, 1
65488, -79.47, 15473, -80.06, 0.70, 1, 0, 1
65472, -79.44, 15472, -80.05, 0.70, 1, 0, 1

...

64, 80.21, 1336, 75.21, 0.70, 0, 0, 1
48, 80.25, 1334, 75.23, 0.70, 0, 0, 1
32, 80.29, 1334, 75.23, 0.70, 0, 0, 1
16, 80.33, 1336, 75.21, 0.70, 0, 0, 1

Comp Thresholds:
0, 4.47, 4.14
------------------------
Elapsed test time (ms): 91024
------------------------
```

Finally, we have both switch-on and switch-off voltages for high and low sides in the LO test mode.

This is sufficient to trim the low side resistors, but for the high side we now need to set up for HI test mode.

```
>K2
K: TEST_CALIBRATE
>K4
K: TEST_CALIBRATE
Starting DAC verify
DAC LSB, DAC mV, ADC LSB, ACD mV, ADC Tol mV, Cl, Ch, Ch-
65504, -762.55, 16163, -766.66, 6.15, 1, 1, 1
65488, -762.18, 16162, -766.56, 6.15, 1, 1, 1
65472, -761.81, 16161, -766.47, 6.15, 1, 1, 1

...

Comp Thresholds:
0, -743.92, -750.90
2, 444.82, 439.45
------------------------
Elapsed test time (ms): 66113
------------------------

>K2
K: TEST_CALIBRATE
>K4
K: TEST_CALIBRATE
Starting DAC verify
DAC LSB, DAC mV, ADC LSB, ACD mV, ADC Tol mV, Cl, Ch, Ch-
16, 80.33, 1351, 75.04, 0.70, 1, 1, 0
32, 80.29, 1356, 74.99, 0.70, 0, 1, 0


65456, -79.40, 15488, -80.23, 0.70, 0, 1, 1
65472, -79.44, 15492, -80.27, 0.70, 0, 1, 1
65488, -79.47, 15501, -80.37, 0.70, 0, 1, 1
65504, -79.51, 15504, -80.40, 0.70, 0, 1, 1

Comp Thresholds:
0, 80.29, 75.00
2, 1.78, 1.31
------------------------
Elapsed test time (ms): 97629
------------------------
```

We should now have a table of results that we can enter into the calculator spreadsheet.
We'll use the DAC mV values, unless the difference between DAC and ADC is greater than ADC Tol.
If the discrepency is very large, there was probably a problem with the test, and should be investigated.
Otherwise, use the ADC value, or the average of the two, at your discretion.

|Parameter|Value|
|-|-|
|LS Vth-on|-425.26|
|LS Vth-off|4.47|
|HS Vth-on|441.48|
|HS Vth-off|5.60|
|HI Vth-on|444.82|
|HI Vth-off|1.78|
|HI Bias| 12.21|

Here we can see that the low side switch-off is not the same polarity as switch-on.


### Sweep Test / Functional Test



## How it works

### Configuration

#### Config

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

#### Calibration

Additionally, 3 calibration points for the DAC and ADC are stored for each of the voltage ranges.
|||
|-|-|
| DAC LSB | ADC LSB | DMM mV |

#### State

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

### Principal PWM functions

| function name | parameters | behaviour description |
| ------------- | ---------- | --------------------- |
| set_pwm() | (byte) pwm | writes pwm to config.pwm, calls set_target(pwm) if not oscilating or osc state is 0 |
| force_pwm() | (byte) pwm | disables oscillation, sets the forced flag and value, and calls write_pwm(pwm) |
| force_pwm_w() | (uint) pwm | as force_pwm(), but accepts a 16 bit value |
| update_pwm() | (byte) pwm | applies the look-up-table if enabled, applies pwm_min/max, calls write_pwm() with the modified value |
| write_pwm() | (uint) pwm | applies pwm bit width, performs negation, writes to timer OC register |
| set_target() | (byte) pwm | sets ramp_target. if forced, sets ramp_value to nearest value to forced_value |

#### PWM frequency table

| Bits \ Prescale | 1 | 2 | 3 | 4 | 5 |
|-|-|-|-|-|-|
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

### Command details

#### STOP_ALL

- Takes no parameters.
- Disables LFO output.
- Disables LFO control of PWM.
- Sets PWM immediately to 0, calling update_pwm().

#### SET_RATE

- If a query (?) returns the LFO time interval.
- Accepts a value in milliseconds. If above INTERVAL_MIN (20ms), sets interval to this value.
- Otherwise toggles LFO output on D13.

#### SET_PWM

- Accepts values from 0 to 255 and calls set_pwm(value).
- If no value provided, calls set_target(config.pwm).

#### FORCE_PWM(_W)

- Accepts values from 0 to 255 (0 to 65535) and calls force_pwm(_w)(value).

#### SET_PRESCALE

- Accepts values from 1 to 5, representing 1, 8, 64, 256 and 1024 clock dividers.
- See PWM frequency table (above).

#### SET_PWM_LIMIT

- Accepts values from -254 to 0 to set config.pwm_min (discarding negative).
- Accepts values from 1 to 255 to set config.pwm_max.

#### SET_PWM_INVERT

- Accepts two boolean values for Negate and Invert respectively.
- Zero is FALSE, non zero is TRUE.
- Negate makes a pwm value of 0 fully ON, and 255 OFF but for one bit.
- Invert makes ON output LOW and OFF output HIGH.
- Setting both Negate and Invert has no effect on polarity, but allows PWM to be reduced to ZERO. This is typically required for regen braking, where fully OFF is required, but fully ON is to be avoided.

#### SET_PWM_RAMP

- Accepts values from 0 to 65535 milliseconds.
- TIMER2_OVF ISR handles ramping pwm values, and has a resolution of 32.5ms.
- Interval value is converted to ISR counts by dividing by 33, and stored in config.pwm_ramp.
- Each ISR, ramp counter is incremented and once config.pwm_ramp has been reached, pwm is inc/decremented.
- Ramp counter is then reset, and ramping continuous until ramp_target is reached.
- A ramp time of 0 disables ramping.

#### SET_PWM_BITS

- Accepts values from 6 to 14. Clamps values to this range. 
- Sets the PWM timer TOP value to 2^(value)-1.
- Updates all registers and parameters accordingly, using ISRs where needed.

#### OSCILLATE

- Accepts values of 0 to 255. Sets config.pwm_osc to this value and enables oscillation.
- If no valid value provided, toggles oscillation.
- When oscillation is enabled, the LFO will alternately set the pwm ramp target to config.pwm and config.pwm_osc.
- Oscillation is disabled when a force_pwm command is given.

#### USE_LUT

- Toggles the config.lut flag.
- With config.lut set, pwm values will be selected from the look-up-table.
- Currently there is only one look-up-table, which converts boost amount in tenths to pwm, and is 8-bit.
- eg. setting PWM to 50 with LUT enabled, will provide approximately 6x boost (1 + 5.0).
- There is no accounting for load or inefficiency, so actual boost values may be lower.

#### SAVE_EEP

- Accepts a single boolean value.
- If TRUE, writes the calibration to EEPROM.
- If FALSE, or omitted, writes the config to EEPROM.

#### LOAD_EEP

- Reloads the configuration from eeprom (including calibration).
- Prints loaded config and cal to serial.
- Has no effect on state, but does call update_pwm, which may interfere with ramping (possible bug).
- Reports failure if CRC does not match (CRC not currently implemented).

#### RESET_CONFIG

- Resets config to default values
- As above, has no effect on state, but may interfere with ramping.

|Config parameter | prescale_n | pwm | pwm_osc | pwm_min | pwm_max | pwm_ramp | pwm_bits | pwm_invert | pwm_negate | oscillate | enable | lut | interval |
|-|-|-|-|-|-|-|-|-|-|-|-|-|-|
|Default value    | 1 | 0 | PWM_MAX | PWM_MIN | PWM_MAX | 1 | 8 | false | true | false | false | true | 500 |

|||
|-|-|
| PWM_MIN | 10 |
| PWM_MAX | 245 |

#### PRINT_CONFIG

- Prints configuration to Serial.
- If a query (?), prints calibration instead.

#### CALIBRATE_TEST

- If a query (?), prints available calibration modes.
- Currently accepts the following values to start the given tests.

0. i/o voltage read test (CALIBRATE)
1. calibrate dac/adc (CAL_DAC)
2. toggle dac/adc range
3. dac sweep up LSB (VERIFY_CAL_UP)
4. dac sweep down LSB (VERIFY_CAL_DN)

#### MEASURE_TEST

- Begins the MEASURE test.

#### SET_DAC

- Accepts a value from 0 to 65535.
- Sets PWM bits to 12 and forces pwm to the supplied value.
- Waits for DAC value to stabilise.


### Test Types

The following test types are defined:

| Test type | Description |
|-|-|
| NONE | No test. Outputs are as commanded |
| CALIBRATE | Take a single measurement. Provide a custom voltage and check that the output value is correct. Enables debug messages |
| FULL_SWEEP | Perform a full sweep through all bit ranges, from 0 to max_pwm with decremental step ups, starting at 21 |
| MEASURE | Performs a single measurement |
| CAL_DAC | Calibrates the DAC and its monitor ADC channel |
| VERIFY_DAC_CAL_UP | Sweep through some DAC values and read the ADC, print the results including sensor states |
| VERIFY_DAC_CAL_DN | As above, but sweeping down |
| COMPARATOR_LO | Tests the UUT's upper and lower comparator thresholds at 0v using J9 Test Connector<br>Not implemented yet.|
| COMPARATOR_HI | Tests the UUT's comparator thresholds at -20V (using which connector?)<br>Not implemented yet.|

Tests are broken down into the following steps:
|||
|-|-|
| IDLE | Test is not running or has completed |
| SETUP | Configures the outputs as required for the test |
| SAMPLE | Take ADC readings of input and output until they are stable |
| REPORT | Prints the collected data to serial |
| NEXT | Selects the next test configuration |

#### Principal Test Functions:

##### test_print_ratios()

Prints input and output voltage measurment gains based on hard coded resistor values.
Resistor values must be accurately measured and entered into the code prior to compiling and uploading.

- print input gain resistors
- print gains and gain steps for these resistors
- print output gain resistors
- print gains and gain steps for these resistors

##### test_stop()

Resets test state.

- sets ouptut and input gains to 0
- sets test type to NONE
- sets test stage to IDLE
- disables test_debug

##### test_reset()

As above, but also calls set_pwm(0).

##### test_setup()

Sets the configuration according to the test parameters.

- calls set_pwm(test_pwm)
- calls test_ramp_delay()
- calls set_pwm_bits(test_bits)
- increments test_stage and test_setup_idx

##### test_sample()

Takes ADC measurments of Input and Output voltages, iteratively adjusts gain range until both are in range, or overflowed.
Prints values each iteration if test_debug enabled.

- Sets gains to 0 (lowest gain)
- calls test_sample_read() to perform ADC reads
- checks the spread of samples
- repeats reads until spread is within tolerance, uses average of repeats if persistantly out of tolerance
- adjusts gains to maximise resolution, repeats measurements
- resets output gain to 0 (why not input?)

##### test_sample_read()

Params:
- sample: a pointer to the results sample struct to write to.

Handles multisampling of ADC channels for Input and Output.

##### test_sample_adjust_gain()

Params:
- sample: last result from test_sample_read
- is_output: flag to indicate if this was an input or output sample

Handles automatic gain adjustment.


### Test Details

#### CALIBRATE

- Take a single measurement.
- Provide a custom voltage and check that the output value is correct. (which means what?)
- Enables debug messages (allegedly, presumably this means enabling test_debug, which prints selected gain ranges)

This seems to be for testing the input/ouput voltage measurements, validating the resistor divider values.

|Stage|Action|
|-|-|
|Start| disables LUT<br>test ramp time from pwm ramp time<br>start pwm = 0<br>start pwm bits = 8<br>pwm inc = 0|
|SETUP| call test_print_ratios()<br> call test_setup()|
|SAMPLE|call test_sample()|
|REPORT|call test_report()|
|NEXT|call test_reset()|
|End|call print_test_time()|


#### CAL_DAC

- Take a set of `NO_OF_CALIBRATION_POINTS` measurements from `DAC_CAL_START_OFFSET` to `DAC_CAL_END_OFFSET` for both high and low ranges.
- Requests voltage measurements from a DMM for each measurement.
- Stores the calibration data in RAM.

|Stage|Action|
|-|-|
|Start| call start_dac_cal():<br>set_pwm_bits(DAC_BITS)<br>set_dac(0)<br>set_dac_range(HIGH)|
|SETUP| call cal_dac_setup()|
|SAMPLE|call cal_dac_read_sample()|
|REPORT|call cal_dac_report()<br>returns to SETUP if not complete|
|NEXT|call cal_dac_complete()|
|End| |


#### MEASURE

-Takes a single measurment

|Stage|Action|
|-|-|
|Start| |
|SETUP| |
|SAMPLE|call test_sample()<br>call dac_read_sample()|
|REPORT|call test_report()<br>call dac_Calculate_voltages()<br>call dac_report()|
|NEXT|call test_stop()|
|End| |


#### FULL_SWEEP

- Sweeps though all pwm-bitage and pwm-value settings
- Reads and reports input and output voltages for each setup

|Stage|Action|
|-|-|
|Start| disables LUT<br>test ramp time from pwm ramp time<br>start pwm = 0<br>start pwm bits = 6 (PWM_BITS_MIN)<br>pwm inc = 21 (TEST_PWM_INC_INIT)|
|SETUP| call test_setup()|
|SAMPLE|call test_sample()|
|REPORT|call test_report()|
|NEXT|call test_sweep_next()|
|End|call print_test_time()|


#### VERIFY_DAC_CAL_UP/DN

- Sweeps DAC output up or down in `DAC_STEP` steps.
- While this is called 'verify' it seems, it's currently the only DAC sweep test, and should probably be used for testing comparator thresholds.

|Stage|Action|
|-|-|
|Start| call start_dac_verify()<br>sets dac to min or max as per direction|
|SETUP| call dac_verify_setup()|
|SAMPLE|call dac_verify_sample()|
|REPORT|call dac_verify_report()|
|NEXT|call dac_verify_next()|
|End||

