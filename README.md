# OpenGEET_ECU_firmware
Open GEET Engine Control Unit firmware

Reads sensor values (rpm, egt, map, torque) and user inputs, control valve positions.
Optionally drives fuel injection and ignition, reads magnetic fields.
Logs all values to sd card.

Initially, control inputs will be mapped directly to valve positions.
Once the behaviour of the reactor and engine are better understood, control input can be reduced to rpm control via PID loops.
A 3-phase BLDC motor can then be added to provide engine start and power generation using an off the shelf motor controller.
Control input can then be tied to output power draw.
To keep costs down, a prony brake will be used to load the engine as an intermediate step before fitting a motor/generator.

Hardware is currently an Arduino MEGA with a Deek-Robot datalogger and Gameduino III. Arduino UNO had insufficient SRAM and Flash.
Upgrade to an STM10x or 40x device can be made later if required.

#### Development roadmap:

### 0.8+

**Objectives:**
- Starter/Generator BLDC integration
- PID control of reactor vacuum
- Reactor state monitoring (starting, in spec, out of spec)
- Updated UI: configurable screen, multiple settings pages
- Updated UI: dynamic labels for physical input
- Prony brake servo
- Time/date config (trim not available)
- Calibration for thermistor inputs (for air inlet, reactor output, oil temperatures etc).
- Full EEPROM export/import
- Engine stop relay output
- Physical push buttons for start/stop engine and log

### v0.7

**Objectives:**
- High sample rate ADC channels
- RAM use optimisation
- Improved Servo calibration screen

### v0.6

**Features:**

- Arduino MEGA update
- Data records store calibrated data by sensor type, including thermistors (raw values)
- Configurable maximum no of sensors by type
- Replace 3rd party thermocouple library
- Shaft power readout
- Updated UI: better basic screen, torque cal page
- Separate record/screen/sdcard log/serial log processes

### v0.5

**Features:**

- Control mode: PID control of RPM via throttle servo
- RPM PID parameter control via ui
- View RPM PID state
- Torque Sensor
- EEPROM dump (partial)

### v0.3

**Features:**

- Screen selection via buttons on left side
- Control mode: direct servo mapping
- Logging start/stop, hex/text and serial/SDcard UI control
- Servo calibration

### v0.2

**Features:**

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
