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
import json


    



class log_file:
        
    def __init__(self, path):
        with open(path) as file:
            print("reading file:", path)
            self.path = path.lower()
            if self.path.endswith(".txt"):
                self.parse_v3(file)
            elif self.path.endswith(".json") or self.path.endswith(".jsn"):
                self.parse_v4(file)
                
    verbose = False
    echo = True
    detailed_rpm = False


    record_version = None
    date = None
    time = None
    
    RECORD_INTERVAL = 0.5
    EGT_FRACTION = 0.25
    
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

#timestamps from each record
    TIME_t = np.array([])

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

    PID = [{}]
    PID_t = [np.array([])]

    PID_k = [{}]


    def parse_v4(self, file):
        print("parse_v4")
        print("file len:", file.readable())
        log = json.load(file)

        header = log['header']
        records = log['records']
        self.record_version = header['version']
        if self.record_version != 4:
            raise ValueError("wrong record version")
        self.firmware = header['firmware']
        self.date = header['date']
        self.time = header['time']
        
        self.RECORD_INTERVAL=header['rate_ms']/1000
        
        t0 = 0
        timestamp = 0
        pid_records = 0
        pid_samples = 0
        for r in records:
            ###print()
            time_s = r['timestamp']/1000
            #record timestamp
            if len(self.AVG_t)>0:
                timestamp = time_s - t0
                ###print("t:",timestamp)
                self.AVG_t = np.append(self.AVG_t, timestamp)
            else:
                #first timestamp, set as t0
                t0 = time_s
                ###print("t0:",t0)
                self.AVG_t = np.array([0])
            
            #user inputs
            length = len(r['usr'])
            avail = len(self.USR)
            if length > avail:
                ###print("len usr:", length)
                ###print("len USR:", avail)
                diff = length - avail
                self.USR.extend([np.array([])]*diff)
            for n in range(length):
                self.USR[n] = np.append(self.USR[n], np.array(r['usr'][n]))
                
            #analog sample times
            ###print("len usr0:", len(r['usr'][0]))
            values = np.linspace(self.AVG_t[-1],self.AVG_t[-1]+self.RECORD_INTERVAL, len(r['usr'][0]),endpoint=False)
            self.ANA_t = np.append(self.ANA_t, values)    
            
            #read pids
            try:
                length = len(r['pid'])
                avail = len(self.PID)
                #ensure the lists have enough elements
                if length > avail:
                    ###print("len pid:", length)
                    ###print("len PID:", avail)
                    diff = length - avail
                    self.PID.extend([{}]*diff)
                    self.PID_k.extend([{}]*diff)
                    self.PID_t.extend([np.array([])]*diff)
                #make sure that there were any elements
                if len(self.PID):
                    pid_records = pid_records+1
                    samples_max = 0
                    #loop through each logged pid
                    for n in range(avail):
                        samples = 0
                        #check if first record for pid
                        if len(self.PID[n]) == 0:
                            #fetch all available parameters from the first pid object
                            self.PID[n] = r['pid'][n]
                            #convert lists to ndarray
                            for key in self.PID[n]:
                                samples = len(r['pid'][n][key])
                                samples_max = max(samples_max, samples)
                                self.PID[n][key] = np.array(self.PID[n][key]);
                        else:
                            #go through each available field and append the new data
                            for key in r['pid'][n]:
                                samples = len(r['pid'][n][key])
                                samples_max = max(samples_max, samples)
                                self.PID[n][key] = np.append(self.PID[n][key], r['pid'][n][key])
                        
                        #now create timestamps for this record and PID
                        values = np.linspace(self.AVG_t[-1],self.AVG_t[-1]+self.RECORD_INTERVAL, samples,endpoint=False)
                        print("times:",len(values))
                        self.PID_t[n] = np.append(self.PID_t[n], values)  
                        
                        #now fetch the coefs (todo)
                        #for key in r['avg']['pid'][n]:
                        #    self.PID_k[n][key] = np.append(self.PID_t[n][key], r['']) 
                    pid_samples += samples_max
                    #loop through each logged pid
                #make sure there are elements
                print("pid records:", pid_records)        
                print("pid samples:", pid_samples)
            except KeyError as e:
                ###print("no key", e)
                pass
             
            
    
    
    
    def parse_v3(self, file):
        print("parse_v3")
        print("file len:", file.readable())
        
        read_avg = True
        index = 0
        no_of_ana_samples = 0
        no_of_egt_samples = 0
        no_of_srv_samples = 0
        
        line_no = 0
        record_no = 0
        
        
        tick_offset = 0
        first_tick = True
    
        
        for line in file:
            line = line.strip()
            
            if self.echo : print(f"{line_no}, {record_no}: {line}")
            line_no = line_no+1
            
            #blank lines
            if len(line) == 0:
                continue
            
            #record marker
            if line == "----------":
                if read_avg == True:
                    #start of record
                    #add the inferred record timestamp
                    if len(self.AVG_t)>0:
                        timestamp = self.AVG_t[-1] + self.RECORD_INTERVAL
                        self.AVG_t = np.append(self.AVG_t, timestamp)
                        record_no = record_no+1
                    else:
                        self.AVG_t = np.array([0])
                    #reset the tick offset
                    tick_offset = -1
                    
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
                    if self.date is None and len(line_split[0])>0:
                        self.date = line_split[0]
                        print("File start date: ", self.date)
                    if self.time is None and len(line_split[1])>0:
                        self.time = line_split[1]
                        print("File start time: ", self.time)
                continue
                
            if len(line_split) == 2:
                #grab the parameter name
                param = line_split[0]
                #grab the value string
                value = line_split[1]
                
                
                #log file version
                if self.record_version is None and param == "Record Ver.":
                    self.record_version = int(value)
                    print("Record Version:", self.record_version)
                    continue
                
                
                #anything else at this point is a param:value pair
                
                #convert the values to arrays of floats
                values = np.fromstring(value.strip(), dtype=float, sep=',').astype(np.float32)
                
                #timestamp (present, but not currently used)
                if param == "Timestamp":
                    self.TIME_t = np.append(self.TIME_t, values)
                    continue
                
                #parse number of analog samples per record
                if param == "ANA no.":
                    no_of_ana_samples = int(values[0])
                    #we are now parsing samples, not averages
                    read_avg = False #this is dumb, but there are errors in the record format, and this is how I'm getting around it.
                    #generate timestamps for the samples
                    values = np.linspace(self.AVG_t[-1],self.AVG_t[-1]+self.RECORD_INTERVAL, no_of_ana_samples,endpoint=False)
                    self.ANA_t = np.append(self.ANA_t, values)
                    if self.verbose: print("ana samp times:", self.ANA_t[-no_of_ana_samples:])
                    continue
    
                #parse user inputs
                if param.endswith("USR"):
                    #user parameter no.
                    index = int(param[0])
                  
                    if read_avg:
                        #expand the number of channels if index exceed current max
                        if index >= len(self.USR_avg):
                            self.USR_avg.append(np.array([]))
                        #append to averages
                        self.USR_avg[index] = np.append(self.USR_avg[index], values)
                        if self.verbose: print(f"USR avg {index}:", self.USR_avg[index][-1])
                    else:
                        #expand the number of channels if index exceed current max
                        if index >= len(self.USR):
                            self.USR.append(np.array([]))
                        #append to samples
                        self.USR[index] = np.append(self.USR[index], values)
                        if self.verbose: print(f"USR {index}:", self.USR[index][-no_of_ana_samples:])
                    continue
                            
                #parse MAP sensors
                if param.endswith("MAP"):
                    #map sensor no.
                    index = int(param[0])
                   
                    if read_avg:
                        #expand the number of channels if index exceed current max
                        if index >= len(self.MAP_avg):
                            self.MAP_avg.append(np.array([]))
                        #append to averages
                        self.MAP_avg[index] = np.append(self.MAP_avg[index], values)
                        if self.verbose: print(f"MAP avg {index}:", self.MAP_avg[index][-1])
                    else:
                        #expand the number of channels if index exceed current max
                        if index >= len(self.MAP):
                            self.MAP.append(np.array([]))
                        #append to samples
                        self.MAP[index] = np.append(self.MAP[index], values)
                        if self.verbose: print(f"MAP {index}:", self.MAP[index][-no_of_ana_samples:])
                    continue
                
                #parse TMP sensors
                if param.endswith("TMP"):
                    #thermistor no.
                    index = int(param[0])
                    
                    
                   
                    if read_avg:
                        #expand the number of channels if index exceed current max
                        if index >= len(self.TMP_avg):
                            self.TMP_avg.append(np.array([]))
                        #append to averages
                        self.TMP_avg[index] = np.append(self.TMP_avg[index], values)
                        if self.verbose: print(f"TMP avg {index}:", self.TMP_avg[index][-1])
                    else:
                        #expand the number of channels if index exceed current max
                        if index >= len(self.TMP):
                            self.TMP.append(np.array([]))
                        #append to samples
                        self.TMP[index] = np.append(self.TMP[index], values)
                        if self.verbose: print(f"TMP {index}:", self.TMP[index][-no_of_ana_samples:])
                    continue
                
                #parse TRQ sensors
                if param == "TRQ":
                    #there is only one torque sensor
                    #presently it is tied in with analog samples
                    if read_avg:
                        #append to averages
                        self.TRQ_avg = np.append(self.TRQ_avg, values)
                        if self.verbose: print("TRQ avg:", self.TRQ_avg[-1])
                    else:
                        #append to samples
                        self.TRQ = np.append(self.TRQ, values)
                        if self.verbose: print("TRQ:", self.TRQ[-no_of_ana_samples:])
                    continue
                
                #parse EGT sensors
                if param.endswith("EGT"):
                    #egt no.
                    index = int(param[0])
                    
                                    #expand the number of channels if index exceed current max
                    if index >= len(self.EGT):
                        self.EGT.append(np.array([]))
                        self.EGT_t.append(np.array([]))
                        
                    #get the correct scale
                    values = values * self.EGT_FRACTION
                   
                    #append to samples
                    self.EGT[index] = np.append(self.EGT[index], values)
                    
                    if self.verbose: print(f"EGT {index}:", self.EGT[-no_of_egt_samples:])
                    
                    #generate time stamps for egt readings
                    no_of_egt_samples = len(values)
                    if no_of_egt_samples > 0:
                        self.EGT_t[index] = np.append(self.EGT_t[index], np.linspace(self.AVG_t[-1],self.AVG_t[-1]+self.RECORD_INTERVAL, no_of_egt_samples, endpoint=False))
                        if self.verbose: print(f"EGT_t {index}:", self.EGT_t[-no_of_egt_samples:])
                    continue
                
                if param.endswith("EGT avg"):
                    #egt no.
                    index = int(param[0])
                    
                    
                    #expand the number of channels if index exceed current max
                    if index >= len(self.EGT_avg):
                        self.EGT_avg.append(np.array([]))
                        
                    #get the correct scale
                    values = values * self.EGT_FRACTION
                    
                    #append to averages
                    self.EGT_avg[index] = np.append(self.EGT_avg[index], values)
                    
                    if self.verbose: print(f"EGT avg {index}:", self.EGT_avg[-1])
                    continue
                
                
                #rpm counter pre calculated average
                if param == "RPM avg":
                    self.RPM_avg = np.append(self.RPM_avg, values)
                    if self.verbose: print("RPM avg:", self.RPM_avg[-1])
                    continue
                #rpm counter tick count
                if param == "RPM no.":
                    self.RPM_no = np.append(self.RPM_no, values)
                    if self.verbose: print("RPM no.:", self.RPM_no[-1])
                    continue
                #rpm counter tick offset
                if param == "RPM tick offset (ms)":
                    tick_offset = values[0]
                    if self.verbose: print("RPM tick offset (ms):", self.RPM_no[-1])
                    continue
                #rpm counter tick intervals
                if param == "RPM times (ms)":
                    if len(values) == 0:
                        continue
                    
                    #check if this is the first recorded tick
                    if len(self.RPM_intervals_ms) == 0:
                        #in which case, ignore this tick
                        values= values [1:]
                        first_tick = True
                        print("first tick")
                    #also ignore any values less than 1
                    if len(values[values<=0]) > 0:
                        print("some ticks ignored:", np.argwhere(values<=0), ":", values[values<=0])
                        values = values[values>0]
                        
                    self.RPM_intervals_ms = np.append(self.RPM_intervals_ms, values)
                    if self.verbose: print("RPM intervals (ms):", self.RPM_intervals_ms[-len(values):])
                    
                    #this mostly works...
                        
                    #instead of accumilating times from the start of logging,
                    #we will use the 1st tick offset from the current record's timestamp
                    # using AVG_t starting at 0ms, rather than TIME_t, which does not start at 0
                    # if a tick offset hasnt been read out, default to the old method
                    if tick_offset >=0:
                        #if this is the first tick in the log file,the first ick is ignored
                        #so we must add the tick offset to the next tick (which is the time since the first uncounted tick)
                        if first_tick:
                            first_tick =  False;
                            values[0] = values[0]+(self.AVG_t[-1]*1000) + tick_offset
                        else:
                            values[0] = (self.AVG_t[-1]*1000) + tick_offset
                        #cumalative sum to get the timestamps relative to the last tick
                        
                    else:
                        
                        print("tick times, but no tick offset")
                        #this is an error, but we can recover
                        
                        #add the previous tick time, if there was one
                        if len(self.RPM_tick_times_ms) > 0:
                            values[0] = values[0] + self.RPM_tick_times_ms[-1]
                        else:
                            print("also no previous times recorded")
                            #just use the record timestamp
                            values[0] = (self.AVG_t[-1]*1000)
                            
                    #add up all the intervals to get the timestamps for each tick
                    values = np.cumsum(values)
                        
                    
    
                    #append the ticks times
                    self.RPM_tick_times_ms = np.append(self.RPM_tick_times_ms, values)
                    if self.verbose: print("RPM tick times (s):", self.RPM_tick_times_ms[-len(values):])
                    continue
                
                #calculated average power
                if param == "POW":
                    self.POW_avg = np.append(self.POW_avg, values)
                    if self.verbose: print("POW:", self.POW_avg[-1])
                    continue
        #end of file
        print("Done")
        
        
    
    def plot_data(self):
        #global labels
        #global RPM_calc, RPM_ticks
        
        labels = []
        
        def show_plot(title=None, labels=None):
            plt.title(title)
            plt.legend(labels)
            plt.show()
            labels.clear()
        
        
            
        
        for (m,n) in enumerate(self.USR):
            plt.plot(self.ANA_t, n)
            labels.append(f'USR {m}')
        for (m,n) in enumerate(self.USR_avg):
            ln = min(len(self.AVG_t),len(n))
            plt.plot(self.AVG_t[:ln], n[:ln])
            labels.append(f'USR avg. {m}')
            
        show_plot('User Input (LSB, max 1023)',labels)
            
        for (m,n) in enumerate(self.MAP):
            plt.plot(self.ANA_t, n)
            labels.append(f'MAP {m}')
        for (m,n) in enumerate(self.MAP_avg):
            ln = min(len(self.AVG_t),len(n))
            plt.plot(self.AVG_t[:ln], n)[:ln]
            labels.append(f'MAP avg. {m}')
            
        show_plot('MAP (mbar)', labels)
        
            
        for (m,n) in enumerate(self.TMP):
            plt.plot(self.ANA_t, n)
            labels.append(f'TMP {m}')
        for (m,n) in enumerate(self.TMP_avg):
            ln = min(len(self.AVG_t),len(n))
            plt.plot(self.AVG_t[:ln], n[:ln])
            labels.append(f'TMP avg. {m}')
        
        show_plot('Thermistor (°C)', labels)
        
        
        for (m,n) in enumerate(self.EGT):
            plt.plot(self.EGT_t[m], self.EGT[m])
            labels.append(f'EGT {m}')
        for (m,n) in enumerate(self.EGT_avg):
            ln = min(len(self.AVG_t),len(n))
            plt.plot(self.AVG_t[:ln], n[:ln])
            labels.append(f'EGT avg. {m}')  
        
        show_plot('EGT (°C)', labels)
        
        ln = min(len(self.AVG_t),len(self.TRQ))
        plt.plot(self.ANA_t[:ln], self.TRQ[:ln])
        labels.append('TRQ')
        
        ln = min(len(self.AVG_t),len(self.TRQ_avg))
        plt.plot(self.AVG_t[:ln], self.TRQ_avg[:ln]/1000)
        labels.append('TRQ avg.')
    
        show_plot('Torque (N.m)', labels)
        
        
        
        RPM_calc = 60*self.RPM_no/self.RECORD_INTERVAL
        RPM_ticks = 60000/self.RPM_intervals_ms
        
        ln = min(len(self.AVG_t),len(RPM_calc))
        plt.plot(self.AVG_t[:ln], RPM_calc[:ln])
        labels.append('avg. from count')
        
        plt.plot(self.RPM_tick_times_ms/1000, RPM_ticks)
        labels.append('avg. from tick times')
        
        ln = min(len(self.AVG_t),len(self.RPM_avg))
        plt.plot(self.AVG_t[:ln], self.RPM_avg[:ln])
        labels.append('reported avg.')
        
        show_plot('Engine Speed (RPM)', labels)
        
        
        
        ln = min(len(self.AVG_t),len(self.POW_avg))
        plt.plot(self.AVG_t[:ln], self.POW_avg[:ln])
        labels.append('brake power avg.')
        
        show_plot('Power (W)', labels)
            
        
        for n,pid in enumerate(self.PID):
            #print("plotting pid,",pid)
            for key in pid:
                ln = min(len(self.PID_t[n]),len(pid[key]))
                plt.plot(self.PID_t[n][:ln], pid[key][:ln])
                labels.append(key)
        
            show_plot('PID', labels)
        
        
        if self.detailed_rpm:
        
            try:
                duration_ms = np.ceil(self.RPM_tick_times_ms[-1]).astype(np.int32) + 1
                ticks = np.zeros(duration_ms)
                ticks_t = np.arange(duration_ms)/1000
                
                np.put(ticks, np.round(self.RPM_tick_times_ms).astype(np.int32), RPM_ticks)
                
                
                
                
                
                
                step = 2
                for start in range ( np.ceil(duration_ms/1000).astype(np.int32) - step):
                    stop = start+step
                    
                    if start is not None: start_rpm = int(start/self.RECORD_INTERVAL)
                    else: start_rpm = None
                    if stop is not None: stop_rpm = int(stop/self.RECORD_INTERVAL)+1
                    else: stop_rpm = None
                    
                    plt.plot(self.AVG_t[start_rpm:stop_rpm], RPM_calc[start_rpm:stop_rpm])
                    labels.append('avg. from count')
                    
                    
                
                    
                    if start is not None: start_tick = int(start*1000)
                    else: start_tick = None
                    if stop is not None: stop_tick = int(stop*1000)
                    else: stop_tick = None
                    
                    plt.plot(ticks_t[start_tick:stop_tick], ticks[start_tick:stop_tick])
                    labels.append('avg. from ticks')
                    
                    plt.ylim(0,5000)
                    
                    show_plot('Engine Speed (RPM)')
            except IndexError as e:
                print (e)


if __name__ == "__main__":
    #default plot res
    mpl.rcParams['figure.figsize'] = [8.0, 6.0]
    mpl.rcParams['figure.dpi'] = 300
    mpl.rcParams['lines.linewidth'] = 0.25
    
    #log file path
    log_file_folder = "../../controller logs/"#
    #log_file_name = "23061202.TXT"
    #log_file_name = "20-06-2023/23062004.TXT"
    #log_file_name = "26-06-2023/23062700.JSN"
    #log_file_name = "27-06-2023/23062702.JSN"
    log_file_name = "27-06-2023/23062700.JSN"
    
    log_file_path = log_file_folder + log_file_name
    
    log = log_file(log_file_path)

    log.plot_data()
    
    