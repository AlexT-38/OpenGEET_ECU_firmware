/*
 * Comparator Resistor Network Balancing
 * 
 * Resistor networks are balance by measuring the threshold
 * voltages and entering the results into a spreadsheet calculator.
 * 
 * Presently only the switch-off threshold voltage has a calculator.
 * Both on and off thresholds will be measured.
 * 
 * The FET driver should be replaced with two links to hold the FETs off.
 * Only the comparators need to be connected.
 * The test must be run twice, once with Vbus and V0 shorted to GND,
 * and the second time with V0 connected to -20V (or more)
 * The nescessary connection are provided on two headers.
 * Once the first test is complete, power down and connect to the 
 * second header.
 * 
 * The procedure is essentially a binary search.
 * The pwm output drives a simple DAC to bias the comparator inputs
 * Once a threshold has been passed, the output must be set past the
 * other threshold to reset.
 * 
 * ADC feedback from the the DAC allows the DAC to be calibrated and 
 * results verified.
 * The DAC can be higher resolution than the ADC, so the DAC value should 
 * be taken as authorative.
 * 
 * Calibration requires only that the DAC output and ADC input be 
 * shorted together, then run the calibration process. The DAC will be set
 * to +,0,- FSD (and maybe 50% points also) and the ADC read. 
 * The DAC should be monitored with a DMM, and the resulting values 
 * should be entered via serial comms. 
 * The DAC and ADC are calibrated independantly.
 */


 
