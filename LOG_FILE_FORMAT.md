## Log file format description.

Log file format version: 5

Base format: JSON

#### HEADER
Describes the configuration of the test hardware.
Fields:
	version : Log file format version number (int, 4)
	firmware : Firmware name and version number (str)
	date : Date at which recording began (YYYY/MM/DD)
	time : Time at which recording began (HH:MM:SS 24hr)
	record_rate_ms : Interval between records in milliseconds (int)
	channel_groups: Dictionary of Dictionary Channel Descriptors
	
	Channel Descriptor:
		type: channel sampling type, 'sample', 'interval', 'timestamp' etc. default is 'sample'
		description: optional description
		num : Number of sub channels. (optional, default 1)
		names : Names of sub channels (optional)
		sample_rate_ms : Sample rate for this sensor for 'sample' type channels.
		units : Units measured, eg m.Nm (str), defaults to 'LSB' for samples, 'ms' for intervals etc
		min : Minimum sensor value (optional, before applying scale)
		max : Maximum sensor value (optional, before applying scale)
		scale : Power of 2 scale factor


#### RECORDS
Records data at fixed intervals.
Array of RECORD :
	timestamp : Timestamp of record in milliseconds (int)
	"channel_group_name": List of channels, lists of sub channels, lists of data points
	
#### Example
```
{
	"header":
	{
		"version" : 5,
		"firmware" : "OpenGEET ECU firmware v0.7.1",
		"date" : "1/1/2025",
		"time" : "12:00",
		"record_rate_ms" : 500
		"channel_groups" :
		{
			"chn_grp_0":
			{
				"avg" :
				{
					"type" : "sample",
					"description" : "example channel value average",
					"num" : 2
					"names" : ["left", "right"]
					"units" : "LSB",
					"scale" : -2,
					"min" : 0,
					"max" : 1024
				},
				"data" :
				{
					"type" : "sample",
					"description" : "example channel value",
					"num" : 2
					"names" : ["left", "right"]
					"sample_rate_ms" : 50,
					"units" : "LSB",
					"scale" : -2,
					"min" : 0,
					"max" : 1024
				}
				"config" : 
				{
					"type" : "sample",
					"description" : "example channel config"
					"units" : "LSB",
					"scale" : -12,
					"min" : -32768,
					"max" : 32767,
				}
			}
		}
	},
	"records" :
	[
		{
			"timestamp" : 123456,
			"chn_grp_0" :
			[
				[[5],[6]],
				[[1,2,3,4,5,6,7,8,9,10], [2,3,4,5,6,7,8,9,10,11]],
				[[-10000]]
			]
		}
	]
}
```