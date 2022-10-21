# OpenGEET_ECU_firmware
Open GEET Engine Control Unit firmware

Reads sensor values (rpm, egt, map) and user inputs, control valve positions.
Optionally drives fuel injection and ignition, reads magnetic fields.
Logs all values to sd card.

Initially, control inputs will be mapped directly to valve positions.
Once the behaviour of the reactor and engine are better understood, control input can be reduced to rpm control via PID loops.
A 3-phase BLDC motor can then be added to provide engine start and power generation using an off the shelf motor controller.
Control input can then be tied to output power draw.

Hardware will initially be an Arduino Uno with a datalogger sheild. Upgrade to an STM10x or 40x device can be made later if required.


### v0.3

**Objectives:**

- Screen selection via buttons on left side
- Control modes: direct servo mapping, target rpm -> throttle pid
- Logging start/stop, hex/text and serial/SDcard UI control
- Servo calibration

### v0.2

**New Features:**

- RPM calculated from sum of tick intervals
- MAP calibration hard coded from factory specs
- Logging now uses calibrated MAP value
- Mapping optimised for pow2 input ranges, and specifically 10bit analog values
- User interface improved. Grid based approach
- Basic touch screen input and multiscreen support, currently unused
- Rudimentary PID, currently unused

### v0.1

**Features:**

- Monitors analog ports A0-A3
- Reads temperature from a MAX6675 via SPI
- Controls up to 3 servos from A1-A3
- Measures rpm by counting ticks on INT0 over 1 second
- Logs measured values over 1 second and their averages over that period
- Log outputs to serial and/or SD card, in plain text or binary format
- Scaling values for analog input and servo output, and log option flags stored in EEPROM
- Gameduino2 WXVGA display shows average values and time/date
- Data Logger sheild for RTC and SD Card
- Bare bones date-time library for DS1307
