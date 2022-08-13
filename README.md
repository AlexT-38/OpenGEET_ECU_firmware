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
