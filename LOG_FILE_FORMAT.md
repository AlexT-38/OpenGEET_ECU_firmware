Log file format description.

Log file format version: 4

Base format: JSON

HEADER
Describes the configuration of the test hardware.
Fields:
	version : Log file format version number (int, 4)
	firmware : Firmware name and version number (str)
	rate_ms : Interval between records in milliseconds (int)
	date : Date at which recording began (YYYY/MM/DD)
	time : Time at which recording began (HH:MM:SS 24hr)
	sensors : Array of sensor descriptors
	servos : Servo configuration
	pid : PID controller configuration
	
	SENSOR Descriptor:
		name : Name of sensor (str, 3 chars)
		num : Number of sensors of this type (optional, int)
		rate : Sample rate for this sensor (optional - if not provided, assumed 1 per record, int)
		rate_units : Sample rate units, typically milliseconds (ms), but could also be per rotation (pr) etc. (optional - must be provided if rate is specified, str)
		units : Units measured, eg m.Nm (str)
		min : Minimum sensor value (optional)
		max : Maximum sensor value (optional)
		scale : Power of 2 scale factor
	
	SERVOS Configuration :
		num : Number of servos (int)
		rate : Servo position update rate (int)
		rate_units : Servo update rate units, typically milliseconds (str) (Not sure why this would be anything other than ms)
		units : Servo position units, typically LSB (str)
		min : Minimum position value
		max : Maximum position value
		names : Array of servo channel names, eg "throttle" (str) (length must match 'num')
	
	PID Configuration :
		rate : PID update rate (int)
		rate_units : PID update rate units, typically milliseconds (str) (Not sure why this would be anything other than ms)
		k_frac : Number of fractional bits used for coefficients (int)
		configs : Array of configurations for each PID controller:
			name : Name of PID controller (str)
			src : Name of sensor input (str)
			idx : Index of sensor where there are multiple sensors with the same name (idx)
			dst : Name of servo output (str)
			kp : Proportional coefficient (fixed point int)
			ki : Integral coefficient (fixed point int)
			kd : Derivitive coefficient (fixed point int)

RECORDS
Records data at fixed intervals.
Array of RECORD :
	timestamp : Timestamp of record in milliseconds (int)
	<xxx> (name of sensor) : 2D Array or sensor index/sensor values for sensor <xxx>, not including sensors without a 'rate' property
	<xxx>_t0 (name of sensor) : Time offset of first sample for sensor with non fixed update rates, typically 'spd' which records tacho pulse intervals (int)
	avg: Dictionary of average values for each sensor (including those without a 'rate'), servo and the coefficients of each PID 
	pid: Array of PID parameter reports:
		trg : Target value
		act : Input value
		err : Difference between trg and act
		out : Output value (sum of p i and d)
		p : proportional component (kp * err)
		i : integral component (need to check if this is pre or post scaled)
		d : derivative component (kd * (err-err_old)?)