# -*- coding: utf-8 -*-
"""
Created on Wed Jun 14 16:24:04 2023

OpenGEET ECU Log File Visiualiser
Parses version 3 log files.

@author: AlexT-38
"""


"""
1) load log file
2) parse data
3) use matplotlib to plot the data
"""


import numpy as np
import matplotlib as mpl
import matplotlib.pyplot as plt
import os

#default plot res
mpl.rcParams['figure.figsize'] = [8.0, 6.0]
mpl.rcParams['figure.dpi'] = 300

#log file path
log_file_folder = "../../controller logs/"
#log_file_name = "23061202.TXT"
log_file_name = "23061203.TXT"
log_file_path = os.path.join(log_file_folder, log_file_name)


record_version = None
date = None
time = None

RECORD_INTERVAL = 0.5
EGT_FRACTION = 0.25
RPM_OVERTIME_ADJUST = 0.5

#average values for each record
USR_avg = []#[np.array([])]
MAP_avg = []#[np.array([])]
TMP_avg = []#[np.array([])]
EGT_avg = []#[np.array([])]
SRV_avg = []#[np.array([])]
RPM_avg = np.array([])
TRQ_avg = np.array([])
RPM_avg = np.array([])
POW_avg = np.array([])
#number of rpm counter ticks each record
RPM_no = np.array([]) 
#rpm calculated from the above tick count
RPM_calc = np.array([])

#time stamps for each record
AVG_t = np.array([])

#rpm interval times
RPM_intervals_ms = np.array([])
#rpm intervals integrated from start of record
RPM_tick_times_ms = np.array([])
#rpm calculated from the above tick intervals
RPM_ticks = np.array([])

#analog samples
USR = []#[np.array([])]
MAP = []#[np.array([])]
TMP = []#[np.array([])]
#load cell samples
TRQ = np.array([])
#timestamps for analog samples
ANA_t = np.array([])

#egt samples
EGT = []#[np.array([])]
#timestamps for egt samples
#as each egt is a seperate chip that may fail independantly, each channel needs its own timestamps
EGT_t = []#[np.array([])]

#servo/pid samples - not apparently implemented
SRV = []#[np.array([])]
#timestamps for servo samples
SRV_t = np.array([])


read_avg = True
sample_count = 0
index = 0
no_of_ana_samples = 0
no_of_egt_samples = 0
no_of_srv_samples = 0
add_ana_timestamps = True
add_egt_timestamps = True
add_srv_timestamps = True

verbose = False


line_no = 0
record_no = 0



with open(log_file_path) as file:
    print("reading file:", log_file_path)
    
    for line in file:
        line = line.strip()
        
        print(f"{line_no}, {record_no}: {line}")
        line_no = line_no+1
        
        #blank lines
        if len(line) == 0:
            continue
        
        #record marker
        if line == "----------":
            if read_avg == True:
                #end of record, do per record processing
                #add the record timestamp
                if len(AVG_t)>0:
                    timestamp = AVG_t[-1] + RECORD_INTERVAL
                    AVG_t = np.append(AVG_t, timestamp)
                    record_no = record_no+1
                else:
                    AVG_t = np.array([0])
                
            else: 
                read_avg = True
            continue
        
        #split line int 'PAR:VAL'
        line_split = line.split(": ")
        
        #an unsplit string is probably the start date and time
        if len(line_split) < 2:
            #probably the opening date stamp            
            line_split = line.split(',')
            if len(line_split) == 2:
                if date is None and len(line_split[0])>0:
                    date = line_split[0]
                    print("File start date: ", date)
                if time is None and len(line_split[1])>0:
                    time = line_split[1]
                    print("File start time: ", time)
            continue
            
        if len(line_split) == 2:
            #grab the parameter name
            param = line_split[0]
            #grab the value string
            value = line_split[1]
            
            
            #log file version
            if record_version is None and param == "Record Ver.":
                record_version = int(value)
                print("Record Version:", record_version)
                continue
            
            
            #anything else at this point is a param:value pair
            
            #convert the values to arrays of floats
            values = np.fromstring(value.strip(), dtype=float, sep=',').astype(np.float32)
            
            #timestamp (present, but not currently used)
            if param == "Timestamp":
                #print(line)
                #AVG_t = np.append(AVG_t, values)
                continue
            
            #parse number of analog samples per record
            if param == "ANA no.":
                no_of_ana_samples = int(values[0])
                #we are now parsing samples, not averages
                read_avg = False #this is dumb, but there are errors in the record format, and this is how I'm getting around it.
                #generate timestamps for the samples
                values = np.linspace(AVG_t[-1],AVG_t[-1]+RECORD_INTERVAL, no_of_ana_samples,endpoint=False)
                ANA_t = np.append(ANA_t, values)
                if verbose: print("ana samp times:", ANA_t[-no_of_ana_samples:])
                continue

            #parse user inputs
            if param.endswith("USR"):
                #user parameter no.
                index = int(param[0])
              
                if read_avg:
                    #expand the number of channels if index exceed current max
                    if index >= len(USR_avg):
                        USR_avg.append(np.array([]))
                    #append to averages
                    USR_avg[index] = np.append(USR_avg[index], values)
                    if verbose: print(f"USR avg {index}:", USR_avg[index][-1])
                else:
                    #expand the number of channels if index exceed current max
                    if index >= len(USR):
                        USR.append(np.array([]))
                    #append to samples
                    USR[index] = np.append(USR[index], values)
                    if verbose: print(f"USR {index}:", USR[index][-no_of_ana_samples:])
                continue
                        
            #parse MAP sensors
            if param.endswith("MAP"):
                #map sensor no.
                index = int(param[0])
               
                if read_avg:
                    #expand the number of channels if index exceed current max
                    if index >= len(MAP_avg):
                        MAP_avg.append(np.array([]))
                    #append to averages
                    MAP_avg[index] = np.append(MAP_avg[index], values)
                    if verbose: print(f"MAP avg {index}:", MAP_avg[index][-1])
                else:
                    #expand the number of channels if index exceed current max
                    if index >= len(MAP):
                        MAP.append(np.array([]))
                    #append to samples
                    MAP[index] = np.append(MAP[index], values)
                    if verbose: print(f"MAP {index}:", MAP[index][-no_of_ana_samples:])
                continue
            
            #parse TMP sensors
            if param.endswith("TMP"):
                #thermistor no.
                index = int(param[0])
                
                
               
                if read_avg:
                    #expand the number of channels if index exceed current max
                    if index >= len(TMP_avg):
                        TMP_avg.append(np.array([]))
                    #append to averages
                    TMP_avg[index] = np.append(TMP_avg[index], values)
                    if verbose: print(f"TMP avg {index}:", TMP_avg[index][-1])
                else:
                    #expand the number of channels if index exceed current max
                    if index >= len(TMP):
                        TMP.append(np.array([]))
                    #append to samples
                    TMP[index] = np.append(TMP[index], values)
                    if verbose: print(f"TMP {index}:", TMP[index][-no_of_ana_samples:])
                continue
            
            #parse TRQ sensors
            if param == "TRQ":
                #there is only one torque sensor
                #presently it is tied in with analog samples
                if read_avg:
                    #append to averages
                    TRQ_avg = np.append(TRQ_avg, values)
                    if verbose: print("TRQ avg:", TRQ_avg[-1])
                else:
                    #append to samples
                    TRQ = np.append(TRQ, values)
                    if verbose: print("TRQ:", TRQ[-no_of_ana_samples:])
                continue
            
            #parse EGT sensors
            if param.endswith("EGT"):
                #egt no.
                index = int(param[0])
                
                                #expand the number of channels if index exceed current max
                if index >= len(EGT):
                    EGT.append(np.array([]))
                    EGT_t.append(np.array([]))
                    
                #get the correct scale
                values = values * EGT_FRACTION
               
                #append to samples
                EGT[index] = np.append(EGT[index], values)
                
                if verbose: print(f"EGT {index}:", EGT[-no_of_egt_samples:])
                
                #generate time stamps for egt readings
                no_of_egt_samples = len(values)
                if no_of_egt_samples > 0:
                    EGT_t[index] = np.append(EGT_t[index], np.linspace(AVG_t[-1],AVG_t[-1]+RECORD_INTERVAL, no_of_egt_samples, endpoint=False))
                    if verbose: print(f"EGT_t {index}:", EGT_t[-no_of_egt_samples:])
                continue
            
            if param.endswith("EGT avg"):
                #egt no.
                index = int(param[0])
                
                
                #expand the number of channels if index exceed current max
                if index >= len(EGT_avg):
                    EGT_avg.append(np.array([]))
                    
                #get the correct scale
                values = values * EGT_FRACTION
                
                #append to averages
                EGT_avg[index] = np.append(EGT_avg[index], values)
                
                if verbose: print(f"EGT avg {index}:", EGT_avg[-1])
                continue
            
            
            #rpm counter pre calculated average
            if param == "RPM avg":
                RPM_avg = np.append(RPM_avg, values)
                if verbose: print("RPM avg:", RPM_avg[-1])
                continue
            #rpm counter tick count
            if param == "RPM no.":
                RPM_no = np.append(RPM_no, values)
                if verbose: print("RPM no.:", RPM_no[-1])
                continue
            #rpm counter tick intervals
            if param == "RPM times (ms)":
                if len(values) == 0:
                    continue
                RPM_intervals_ms = np.append(RPM_intervals_ms, values)
                if verbose: print("RPM intervals (ms):", RPM_intervals_ms[-len(values):])
                #convert to seconds
                values = values
                #cumalative sum to get the timestamps relative to the last tick
                values = np.cumsum(values)
                #add the previous tick time, if there was one
                if len(RPM_tick_times_ms) > 0:
                    values = values + RPM_tick_times_ms[-1]
                #check that the last timestamps is not after the end of the record
                """
                overtime = values[-1] - (AVG_t[-1] + RECORD_INTERVAL)
                if verbose: print("RPM count overtime: ", overtime)
                if overtime < 0:
                    #this may occur due to rounding errors
                    
                    # move this record's rpm tick times back by a fraction of the overtime
                    overtime = overtime * RPM_OVERTIME_ADJUST
                    values = values + overtime
                """
                #append the ticks times
                RPM_tick_times_ms = np.append(RPM_tick_times_ms, values)
                if verbose: print(f"RPM tick times (s):", RPM_tick_times_ms[-len(values):])
                continue
            
            #calculated average power
            if param == "POW":
                POW_avg = np.append(POW_avg, values)
                if verbose: print(f"POW:", POW_avg[-1])
                continue
    #end of file
    print("Done")

def plot_data():
    global labels
    global RPM_calc, RPM_ticks
    
    labels = []
    
    def show_plot(title=None):
        global labels
        plt.title(title)
        plt.legend(labels)
        plt.show()
        labels = []
    
    
        
    
    for (m,n) in enumerate(USR):
        plt.plot(ANA_t, n)
        labels.append(f'USR {m}')
    for (m,n) in enumerate(USR_avg):
        plt.plot(AVG_t, n)
        labels.append(f'USR avg. {m}')
        
    show_plot('User Input (LSB, max 1023)')
        
    for (m,n) in enumerate(MAP):
        plt.plot(ANA_t, n)
        labels.append(f'MAP {m}')
    for (m,n) in enumerate(MAP_avg):
        plt.plot(AVG_t, n)
        labels.append(f'MAP avg. {m}')
        
    show_plot('MAP (mbar)')
    
        
    for (m,n) in enumerate(TMP):
        plt.plot(ANA_t, n)
        labels.append(f'TMP {m}')
    for (m,n) in enumerate(TMP_avg):
        plt.plot(AVG_t, n)
        labels.append(f'TMP avg. {m}')
    
    show_plot('Thermistor (°C)')
    
    
    for (m,n) in enumerate(EGT):
        plt.plot(EGT_t[m], EGT[m])
        labels.append(f'EGT {m}')
    for (m,n) in enumerate(EGT_avg):
        plt.plot(AVG_t, n)
        labels.append(f'EGT avg. {m}')    
    
    show_plot('EGT (°C)')
    
    
    
    plt.plot(ANA_t, TRQ)
    labels.append(f'TRQ')
    
    plt.plot(AVG_t, TRQ_avg/1000)
    labels.append(f'TRQ avg.')
    
    show_plot('Torque (N.m)')

    RPM_calc = 60*RPM_no/RECORD_INTERVAL
    RPM_ticks = 60000/RPM_intervals_ms
    
    
    plt.plot(AVG_t, RPM_calc)
    labels.append(f'avg. from count')
    
    plt.plot(AVG_t, RPM_avg)
    labels.append(f'reported avg.')
    
    plt.plot(RPM_tick_times_ms/1000, RPM_ticks)
    labels.append(f'avg. from tick times')
    
    show_plot('Engine Speed (RPM)')
    
    duration_ms = np.ceil(RPM_tick_times_ms[-1]).astype(np.int32) + 1
    ticks = np.zeros(duration_ms)
    ticks_t = np.arange(duration_ms)/1000
    
    np.put(ticks, np.round(RPM_tick_times_ms).astype(np.int32), RPM_ticks)
    
    
    
    
    
    
    step = 2
    for start in range ( np.ceil(duration_ms/1000).astype(np.int32) - step):
        stop = start+step
        
        if start is not None: start_rpm = int(start/RECORD_INTERVAL)
        else: start_rpm = None
        if stop is not None: stop_rpm = int(stop/RECORD_INTERVAL)+1
        else: stop_rpm = None
        
        plt.plot(AVG_t[start_rpm:stop_rpm], RPM_calc[start_rpm:stop_rpm])
        labels.append(f'avg. from count')
        
        
    
        
        if start is not None: start_tick = int(start*1000)
        else: start_tick = None
        if stop is not None: stop_tick = int(stop*1000)
        else: stop_tick = None
        
        plt.plot(ticks_t[start_tick:stop_tick], ticks[start_tick:stop_tick])
        labels.append(f'avg. from ticks')
        
        plt.ylim(0,4500)
        
        show_plot('Engine Speed (RPM)')

plot_data()
    
    