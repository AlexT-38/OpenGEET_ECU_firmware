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

data is now supplied as json and imported as dict/list structures
parsing should work by collecting all arrays of values in each record into a single record with the same overall structure
and adding timestamps for each, preferably reading the sample interval from the header using the same key,
so that groups of parameters that always have the same sample times only need the one timestamp channel
"""

"""
when plotting "09-08-2023/23080401.JSN"
'avg from count' suffers timestamp offset of approx 12 seconds
from timestamp 137/137.5 or thereabouts
when plotting in detail mode.
need to look into that.
"""


import numpy as np
import matplotlib as mpl
import matplotlib.pyplot as plt
import os
import json


    

# LogFile encapsulates all logged data and parses log files
# takes each record and collates the data for each parameter
class LogFile:
        
    def __init__(self, path, old_method=False):
        self.old_method = old_method
        # open the specified file and attempt to parse data
        with open(path) as file:
            print("reading file:", path)
            self.path = path.lower()
            if self.path.endswith(".json") or self.path.endswith(".jsn"):
                self.parse_json(file)
            else: raise Exception("File format not supported.")
                
    verbose = False         #print extra info about parse process
    
    
    record_version = None   #record version as per record
    date = None             #record start date as per record
    time = None             #record start time as per record
    
    RECORD_INTERVAL = 0.5   #record interval as per record (default vaule set here)
    RECORD_INTERVAL_ms = 500
    
    
    
    class DataChannel:
        def from_sensor(sensor):
            try:                num = sensor['num']
            except KeyError:  num = 1
            try:                names = sensor['names']
            except KeyError:  names = []
            try:                scale = pow(2,sensor['scale'])
            except KeyError:  scale = 1.0
            try:                units = sensor['units']
            except KeyError:  units = 'LSB'
            
            channel = LogFile.DataChannel(sensor['name'],num,names,scale,units) 
            channel_avg = LogFile.DataChannel(sensor['name']+'_avg',num,names,scale,units) 
            
            try: 
                channel.min = sensor['min']
                channel_avg.min = sensor['min']
            except KeyError: pass
            try: 
                channel.max = sensor['max']
                channel_avg.max = sensor['max']
            except KeyError: pass
        
            channels = [channel,channel_avg]
            
            try: channel.rate = sensor['rate']
            except KeyError: 
                #if there's no rate, this is a once per record channel
                #and its values are reported under 'avg'
                channels = [channel_avg]
            try: channel.rate_units = sensor['rate_units']
            except KeyError: pass
        
            return channels
        
        def from_servos(servos):
            channel = LogFile.DataChannel('srv',servos['num'],servos['names'])
            channel_avg = LogFile.DataChannel('srv_avg',servos['num'],servos['names'])
            
            channel.rate = servos['rate']
            channel.rate_units = servos['rate_units']
            channel.min = servos['min']
            channel_avg.min = servos['min']
            channel.max = servos['max']
            channel_avg.max = servos['max']
            
            return channel, channel_avg
        
        def from_pids(pids):
            scale = pow(2,-pids['k_frac'])
            #this is a little more complex, as we want different scale factors to apply to (k)p/i/d vs in/trg/err/out
            #for now, we'll apply the scale factor only to kp/ki/kd
            #its also more complicated because there are multiple pids, each with multiple subchannels
            #i don't see how to make this work smoothly when reading out data (Work, yes. smooth, no)
            channels = []
            for n,pid in enumerate(pids['configs']):
                channel = LogFile.DataChannel('pid_'+str(n),7,['trg','act','err','out','p','i','d'])
                channel.pid = {'name':pid['name'], 'src':pid['src'], 'dst':pid['dst'],}
                
                channel_avg = LogFile.DataChannel('pid_'+str(n)+'_avg',3,['kp','ki','kd'],scale)
                channel_avg.pid = channel.pid
                channels += [channel,channel_avg]
            return channels
                
            
            
            
            #TODO: if we allow scale to be an array, we can apply different scales to each channel.
        def __init__(self, name, sub_channels=1, sub_channel_names=[],scale=1,units='LSB'):
            if sub_channels < 1: raise ValueError("sub_channels cannot be nless than 1; {sub_channels}")
            self.data = np.ndarray([sub_channels+1,0])
            self.name = name
            self.channel_names = [str(scn) for scn in sub_channel_names] + [str(n) for n in range(len(sub_channel_names),sub_channels)]
            self.scale = scale
            self.units = units
            if units in ['s','ms','us','ns']: #ideally in future versions, 'type' will be declared in the header
                if sub_channels > 1:
                    raise ValueError("interval data can only have one sub channel, not {sub_channels}")
                self.type = 'interval'
                self.t0 = -1    #it'd be better not to assume 'xxx_t0' follows or preceeds 'xxx'. set this to -1 after appending data
                self.idx_start_last = 0
            else:
                self.type = 'sample'
            self.timestart_last = 0
            self.timestop_last = 0
            
            
        def __len__(self) -> int:
            try :return self.data.shape[1]
            except IndexError: return 0
            
        
        def get_time_last(self):
            return self.data[0,-1]
        def get_time_start(self):
            return self.data[0,0]
        def get_time_span(self):
            return self.data[0,-1]-self.data[0,0]
        def get_subchannels(self):
            return self.data.shape[0]-1
        
        def set_t0(self, t0, n=0):
            #we need to know the timestamp of the last data set
            if self.type == 'interval':
                t0 = self.apply_tscale(t0)
                if self.t0 == -1:
                    #apply t0 to last data if there was any
                    if len(self) > self.idx_start_last:
                        self.t0 = t0
                        time = self.apply_t0(self.data[0,self.idx_start_last:])
                        self.data[0,self.idx_start_last:] = time
                else: #otherwise stash for next data
                    self.t0 = t0
        
        #converts intervals to timestamps using timestart_last and t0
        #we'll keep this simple till we test
        
        def apply_t0(self, time):
            if self.t0 != -1:
                #apply t0 to this data
                timestart = self.timestart_last + self.t0
                if timestart > self.timestop_last:
                    timespan = self.timestop_last - self.timestart_last
                    print(f't0 is greater than timespan: {self.t0} vs {timespan}') #raise ValueError
                    time[0] = self.timestart_last
                else: 
                    time[0] = timestart
                time = time.cumsum()
                self.t0 = -1
            return time
                
        #convert time usints to seconds
        def apply_tscale(self, time):
            if self.units == 'ms':
                time = time/1e3
            elif self.units == 'us':
                time = time/1e6
            elif self.units == 'ns':
                time = time/1e9
            elif self.units == 'min':
                time = time*60
            return time
        
        #append the given data at the specified start time and timespan
        def append(self, new_data, timestart, timespan):
            if len(self)>1 and timestart < self.get_time_last():
                print(f"{self.name}.append: input timestamp is before last recorded timestamp, {timestart} < {self.get_time_last()}")#raise ValueError
            #make sure the input is a numpy array
            #TODO: ideally chaeck if the data is a dictionary, then match keys to sub channel names
            data = np.array(new_data)
            
            #check the shape of the data
            if data.ndim > 2: raise ValueError(f"{self.name}.append: input data has more than two dimensions: {data.ndim}")
            if data.ndim < 2:
                if hasattr(self,'rate'): data = data.reshape([1,-1]) #if rate is specified, 1d data is a time series
                else:                    data = data.reshape([-1,1]) #otherwise, 1d data is sub channels
            if data.shape[0]+1 != self.data.shape[0]:
                raise ValueError(f"{self.name}.append: input data has {data.shape[0]} sub channels but DataChannel has {self.data.shape[0]-1}")
            #skip empty data
            if data.shape[1] == 0:
                print(f"{self.name}.append: no data to add at time {timestart}")
                if self.type == 'interval':
                    self.idx_start_last = len(self) #makesure this is updated even if no data, otherwise set_t0 will be applied to old data
                return
            #apply scale factor
            data = data * self.scale
            #create time axis data
            #record the timestamps
            timestop = timestart + timespan
            self.timestart_last = timestart
            self.timestop_last = timestop
            #create linear time axis for sampled data
            if self.type == 'sample':
                time = np.linspace(timestart, timestop, data.shape[1],endpoint=False).reshape([1,-1])
            #create cumalative sum for time interval data
            elif self.type == 'interval':
                self.idx_start_last = len(self)
                #generate timestamps
                time = self.apply_tscale(data.copy())
                time = self.apply_t0(time)
                
            #append the data to the time axis
            try:
                data = np.append(time, data,axis=0)
            except ValueError:
                raise
            #append the completed input data to the internal data
            self.data = np.append(self.data, data, axis=1)

            
        # returns a time slice of data over the specified timespan
        # channel mask selects which sub channels to fetch. can be indices or strings
        def get_slice(self, timestart, timestop, channel_mask=[]):
            timespan = timestop - timestart
            #TODO fixme argmin fails if data is ampty. Also, will return values outside of the specified range
            idx_start = np.argmin(np.abs(self.data[0] - timestart))
            idx_stop = np.argmin(np.abs(self.data[0] - timestop))
            if idx_stop == idx_start and idx_stop < len(self): idx_stop += 1
            sliced = self.data[:,idx_start:idx_stop]
            #apply mask
            if len(channel_mask)>0:
                #check if mask specified by names
                if type(channel_mask[0]) == str:
                    mask = []   #convert names to indices
                    for n, string in enumerate(channel_mask):
                        try: mask.append(self.channel_names.index(string))
                        except ValueError: pass #name is not in list
                    channel_mask = mask
                if len(channel_mask)>0: #incrment the indices to account for and include time axis
                    sliced = sliced.take(np.array([-1]+channel_mask)+1, axis=0)
            return sliced
        
        def get_slices(self, timestart, timestop, channel_mask=[]):
            slices = self.get_slice(timestart, timestop, channel_mask)
            return np.split(slices, slices.shape[0])
    #end of DataChannel declaration
    
#collated data fields....
    channels = {} #add channel objects here
    def add_channel(self, channel):
        if type(channel) != self.DataChannel:
            raise TypeError("LogFile.add_channel can only add LogFile.DataChannel type, not '{type(channel)}'")
        if channel.name in self.channels.keys():
            raise IndexError("a channel called '{channel.name}' has already been added")
        self.channels[channel.name] = channel
    
    def add_channels(self, channels):
        for channel in channels: self.add_channel(channel)
    
    def add_old_attributes(self):
        self.detailed_rpm = True     #plot detailed rpm data... plot tacho ticks?
        self.detailed_pid = True     #plot detailed pid data...?
        self.rpm_pid_no_ms = True    #change the way pid is plotted?
        self.rpm_pid_native = True   #change the way pid is plotted?

        self.EGT_FRACTION = 0.25     #egt value scale - why isn't this taken from sensor config?

    #average values for each record
        self.USR_avg = []#[np.array([])]
        self.MAP_avg = []#[np.array([])]
        self.TMP_avg = []#[np.array([])]
        self.EGT_avg = []#[np.array([])]
        self.SRV_avg = []#[np.array([])]
        self.RPM_avg = np.array([])
        self.TRQ_avg = np.array([])
        self.POW_avg = np.array([])
    #number of rpm counter ticks each record
        self.RPM_no = np.array([]) 
    #rpm calculated from the above tick count
        self.RPM_calc = np.array([])
    
    #time stamps for each record
        self.AVG_t = np.array([])
    
    #timestamps from each record
        self.TIME_t = np.array([])
    
    #rpm interval times
        self.RPM_intervals_ms = np.array([])
    #rpm intervals integrated from start of record
        self.RPM_tick_times_ms = np.array([])
        self.RPM_tick_time_record_s = np.array([]) #timestamps of the record associated with each tick
        self.RPM_tick_time_record = np.array([]) #record index associated with each tick
        self.RPM_tick_time_record_offset = np.array([]) #tick offset of the record associated with each tick
    #rpm calculated from the above tick intervals
        self.RPM_ticks = np.array([])
        
        self.RPM_offset = np.array([])
        
        self.RPM_tick_scale = 0
    
    #analog samples
        self.USR = []#[np.array([])]
        self.MAP = []#[np.array([])]
        self.TMP = []#[np.array([])]
    #load cell samples
        self.TRQ = np.array([])
    #timestamps for analog samples
        self.ANA_t = np.array([])
        self.MAP_t = np.array([])
    
    #egt samples
        self.EGT = []#[np.array([])]
    #timestamps for egt samples
    #as each egt is a seperate chip that may fail independantly, each channel needs its own timestamps
        self.EGT_t = []#[np.array([])]
    
    #servo/pid samples - not apparently implemented
        self.SRV = []#[np.array([])]
    #timestamps for servo samples
        self.SRV_t = np.array([])
    
        self.PID = [{}]
        self.PID_t = [np.array([])]
    
        self.PID_k = [{}]

    def parse_json(self, file):
        print("parsing json file")
        print("file size:", os.fstat(file.fileno()).st_size)
        log = json.load(file)
        try:
            header = log['header']
        except KeyError:
            raise Exception("Cannot parse file: No header found.")
        self.record_version = header['version']
        if self.record_version == 4:
            if self.old_method:
                self.parse_v4_old(log)
            else:
                self.parse_v4(log)
        else:
            raise Exception(f"Cannot parse file: Version not supported, {self.record_version}")
        
    
    def parse_v4(self, log):
        import copy
        
        header = log['header']
        records = log['records']

        self.firmware = header['firmware']
        self.date = header['date']
        self.time = header['time']
        
        self.RECORD_INTERVAL_ms = header['rate_ms']         #it would probably be sensible to restrict all time units to s, or ms
        self.RECORD_INTERVAL    = header['rate_ms']/1000
        
        for sensor in header['sensors']:
            self.add_channels( self.DataChannel.from_sensor(sensor) )
            
            #old stuff
            if sensor['name'] == 'spd':
                #get the scale for rpm counter ticks
                self.RPM_tick_scale = pow(2,sensor['scale'])
                #convert us to ms
                if sensor['units'] == 'us':
                    self.RPM_tick_scale = self.RPM_tick_scale/1000
                    
        servos = header['servos']
        self.add_channels(self.DataChannel.from_servos(servos))
        
        pids = header['pid']
        self.add_channels(self.DataChannel.from_pids(pids))
        
        t0 = 0
        timestamp = 0
        no_of_records = 0
        timestamp_last = 0
        
        for r in records:                   # LOOP THROUGH EVERY RECORD
            if self.verbose: print()
            time_s = r['timestamp']/1000
            #record timestamp               # TIMESTAMP
            if no_of_records:
                timestamp_last = timestamp
                timestamp = time_s - t0
                if self.verbose: print("t:",timestamp)
            else:
                #first timestamp, set as t0
                t0 = time_s
                if self.verbose: print("t0:",t0)
            no_of_records += 1    
            
            rr=copy.deepcopy(r)
            
            for key in rr.keys():
                #handle record timestamps
                if key == 'timestamp':
                    pass
                #handle average values (single value per record)
                elif key == 'avg':
                    avg = rr[key]
                    for key in avg.keys():
                        if key == 'pid': #pids need extra handling
                            for n,pid in enumerate (avg[key]):
                                chn = key+'_'+str(n)+'_avg'
                                self.channels[chn].append(list(pid.values()), timestamp, self.RECORD_INTERVAL)
                        else:
                            chn = key+'_avg'
                            self.channels[chn].append(avg[key], timestamp, self.RECORD_INTERVAL)
                #handle pid values (array of dicts)
                elif key == 'pid':
                    pids = rr[key]
                    for n,pid in enumerate(pids):
                        chn= 'pid_'+str(n)
                        self.channels[chn].append(list(pid.values()), timestamp, self.RECORD_INTERVAL)
                #handle time offsets for interval data
                elif key.endswith('_t0'):
                    chn_t0 = rr[key]
                    key = key[:-3]
                    self.channels[key].set_t0(chn_t0)
                #handle regular sampled data (multiple values and sub channels per record)
                else:
                    self.channels[key].append(rr[key], timestamp, self.RECORD_INTERVAL)
        

        
    def parse_v4_old(self, log):
        print("parse_v4 old")
        
        self.add_old_attributes()
        

        header = log['header']
        records = log['records']

        self.firmware = header['firmware']
        self.date = header['date']
        self.time = header['time']
        
        self.RECORD_INTERVAL_ms = header['rate_ms']         #it would probably be sensible to restrict all time units to s, or ms
        self.RECORD_INTERVAL    = header['rate_ms']/1000
        
        for sensor in header['sensors']:
            if sensor['name'] == 'spd':
                #get the scale for rpm counter ticks
                self.RPM_tick_scale = pow(2,sensor['scale'])
                #convert us to ms
                if sensor['units'] == 'us':
                    self.RPM_tick_scale = self.RPM_tick_scale/1000

        #extend the length of an array to accept new data
        def update_length(src, trg):
            length = len(src)
            avail = len(trg)
            if length > avail:
                if self.verbose:
                    print("len usr:", length)
                    print("len USR:", avail)
                diff = length - avail
                trg.extend([np.array([])]*diff)
            return length
                
        #append indexed arrays (record, source field in record, target field)
        def append_multi(rec, src, trg):
            try:
                src = rec[src]
                for n in range(update_length(src,trg)):
                    trg[n] = np.append(trg[n], np.array(src[n]))
            except KeyError as e:
                if self.verbose : print(e)
                pass
        #append single arrays (record, source field in record, target field)
        def append_single(rec, src, trg):
            try:
                src = rec[src]
                setattr( self, trg, np.append(getattr(self,trg), np.array(src)) )
            except AttributeError as e:
                if self.verbose : print(e)
                pass
        
        def append_dict(): #todo - I guess this is for PID records
            pass
        
        
        t0 = 0
        timestamp = 0
        pid_records = 0
        pid_samples = 0
        for r in records:                   # LOOP THROUGH EVERY RECORD
            if self.verbose: print()
            time_s = r['timestamp']/1000
            #record timestamp               # TIMESTAMP
            if len(self.AVG_t)>0:
                timestamp = time_s - t0
                if self.verbose: print("t:",timestamp)
                self.AVG_t = np.append(self.AVG_t, timestamp)
            else:
                #first timestamp, set as t0
                t0 = time_s
                if self.verbose: print("t0:",t0)
                self.AVG_t = np.array([0])
                
            if len(self.AVG_t) > 2 and self.AVG_t[-2]+(self.RECORD_INTERVAL*1.01) < self.AVG_t[-1]:
                print("record timstamp incremented by more than record interval", self.AVG_t[-2], self.AVG_t[-1])
                
            
            #get user inputs                    # GET USER INPUTS
            append_multi(r,'usr',self.USR)
            #analog sample times

            #generate timestamps for user inputs
            values = np.linspace(self.AVG_t[-1],self.AVG_t[-1]+self.RECORD_INTERVAL, len(r['usr'][0]),endpoint=False)
            self.ANA_t = np.append(self.ANA_t, values)   #this step should probably be a function add_timestamps(src, rec, trg) : values = np.linspace(self.AVG_t[-1],self.AVG_t[-1]+self.RECORD_INTERVAL, len(rec[src][0]),endpoint=False) \n trg = np.append(trg, values)
            
            #get Manifold Air Presure sensors   # GET MAPS
            append_multi(r,'map',self.MAP)
            
            #map sensor time axis, if different - why if different?
            if len(r['usr'][0]) != len(r['map'][0]):
                values = np.linspace(self.AVG_t[-1],self.AVG_t[-1]+self.RECORD_INTERVAL, len(r['map'][0]),endpoint=False)
                self.MAP_t = np.append(self.MAP_t, values) #we still need to do this if same though, no?   just generate the time stamps - checking for them being the same has not benefit

            #thermistors - not implemented yet  # GET THERMISTORS
            append_multi(r,'tmp',self.TMP)
            
            #thermocouples                      # GET THERMOCOUPLES
            append_multi(r,'egt', self.EGT)
            
            #thermocouple time axis per sensor      -again, make this a funciton, consider creating a class of {[DATA],[TIMESTAMP]} or np.array([N, V]) with N=0 being timestamps, put all these adding functions in there too.
            for n in range(update_length(r['egt'], self.EGT_t)):
                values = np.linspace(self.AVG_t[-1],self.AVG_t[-1]+self.RECORD_INTERVAL, len(r['egt'][n]),endpoint=False)
                self.EGT_t[n] = np.append(self.EGT_t[n], values)
            
            
            #torque
            append_single(r,'trq', 'TRQ')
            
            #servos
            append_multi(r,'srv', self.SRV)
            
            #servo time axis
            values = np.linspace(self.AVG_t[-1],self.AVG_t[-1]+self.RECORD_INTERVAL, len(r['srv'][0]),endpoint=False)
            self.SRV_t = np.append(self.SRV_t, values) 
            
            #record averages
            avg = r['avg']
            
            #each category we are to collect - this is constant wrt LOOP, why is it here?
            avgs_src = ['usr','map' ,'tmp' ,'egt' ,'srv','rpm','trq','pow']
            avgs_dst = [x.upper() + '_avg' for x in avgs_src]
            
            
            #go through each name in the list and append the values within
            for n in range(len(avgs_src)):
                try:
                    #get the source and destination
                    src = avg[avgs_src[n]]
                    dst = getattr(self, avgs_dst[n])
                    #add to lists
                    if type(src) == list:
                        #loop through entry indices
                        for m in range(update_length(src, dst)):
                            dst[m] = np.append(dst[m], src[m])
                    #add to dicts
                    elif type(src) == dict:
                        pass #todo
                    #add single values
                    else:
                        setattr(self, avgs_dst[n], np.append(dst, src))
                except KeyError as e:
                    if self.verbose: print(e) #when an element is not in the record
                except AttributeError as e:
                    print ('Error!',e)
            
                
            
            
            #read pids
            try:
                length = len(r['pid'])
                avail = len(self.PID)
                #ensure the lists have enough elements
                if length > avail:
                    if self.verbose: print("len pid:", length)
                    if self.verbose: print("len PID:", avail)
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
                        if self.verbose: print("times:",len(values))
                        self.PID_t[n] = np.append(self.PID_t[n], values)  
                        
                        #now fetch the coefs (todo)
                        #for key in avg['pid'][n]:
                        #    self.PID_k[n][key] = np.append(self.PID_t[n][key], avg['pid'][n]) 
                    pid_samples += samples_max
                    #loop through each logged pid
                #make sure there are elements
                if self.verbose: print("pid records:", pid_records)        
                if self.verbose: print("pid samples:", pid_samples)
            except KeyError as e: #no PID date this sample
                ###print("no key", e)
                pass
            
            
            ###process rpm (copied from v3)
            if len(self.RPM_intervals_ms) > 0:
              pass
            #what units are these in? ms or s? tick_scale is 0.016, so should be ms?
            values = np.array(r['spd']) * self.RPM_tick_scale #these are the interval values for each tacho tick
            tick_offset = r['spd_t0'] * self.RPM_tick_scale #this is the offset of the first tick from the record start, if a tick is recorded
            #tick offset appears to be the same each time before rotation detected.
            self.RPM_offset = np.append(self.RPM_offset, tick_offset)
            
            self.RPM_no = np.append(self.RPM_no, len(values))
            if len(values > 0):
              #check if this is the first recorded tick
              if len(self.RPM_intervals_ms) == 0: # !? this will run even if no ticks recorded
                  #in which case, ignore this tick #   but why? first tick interval is valid, or else it doesnt get recorded
                  #values= values [1:]            # also, these arent ticks, these are intervals.
                  first_tick = True
                  if self.verbose: print("first tick") 
              #also ignore any values less than 1 #not sure why there would be, as the src is uint, but good to catch data errors
              if len(values[values<=0]) > 0:
                  print("some ticks ignored:", np.argwhere(values<=0), ":", values[values<=0])
                  values = values[values>0]
              
              self.RPM_intervals_ms = np.append(self.RPM_intervals_ms, values)
              if self.verbose: print("RPM intervals (ms):", self.RPM_intervals_ms[-len(values):])
              
              
              
              
              #this mostly works... except that it doesnt yet.
              #initial_interval = 0
              initial_interval = values[0] #back up that initial interval value
              timenow_ms = timestamp *1000
              timenext_ms = timenow_ms+self.RECORD_INTERVAL_ms
              
              #instead of accumilating times from the start of logging,
              #we will use the 1st tick offset from the current record's timestamp
              # using AVG_t starting at 0ms, rather than TIME_t, which does not start at 0
              # if a tick offset hasnt been read out, default to the old method
              try:
                  if tick_offset >=0 and tick_offset < self.RECORD_INTERVAL_ms:
                      #if this is the first tick in the log file,the first ick is ignored
                      #so we must add the tick offset to the next tick (which is the time since the first uncounted tick)
                      #NOPE, thats wrong. we treat each interval as valid
                      # and the offset is to the tick at the end of that interval.
                      # if the offset to the last interval in this record is longer than record length,
                      # then we know the offset is to the start of the first interval and 
                      # we then just subtract that interval from all the offsets
                      if first_tick:
                          #values[0] = values[0]+(self.AVG_t[-1]*1000) + tick_offset
                          first_tick =  False;
                      #else:
                      
                      #convert the
                      values[0] = timenow_ms + tick_offset
                      #cumalative sum to get the timestamps relative to the last tick
                      
                  else:
                      if tick_offset > 0:
                          print("tick offset is greater than record interval",self.AVG_t[-1],tick_offset,self.RECORD_INTERVAL_ms)
                      else:
                          print("tick offset is negative")
                      #this is an error, but we can recover (by guessing)
                      
                      #add the previous tick time, if there was one
                      if len(self.RPM_tick_times_ms) > 0:
                          new_val0 = values[0] + self.RPM_tick_times_ms[-1]
                          #and if the timestamp would fall within the current record
                          
                          if new_val0 >= timenow_ms and new_val0 < timenext_ms:
                            values[0] = new_val0
                          else:
                            values[0] = values[0]+timenow_ms #if not, then assume the interval started at the beggingng of the record
                      else:
                          print("also no previous times recorded")
                          #just use the record timestamp
                          #we'll assume under these conditions that thefirst interval is
                          #relative to the start of the record.
                          values[0] = values[0]+timenow_ms
              except IndexError as e:
                  print("ErOrR :",e)        
              #add up all the intervals to get the timestamps for each tick
              values = np.cumsum(values)
              #check that the last value falls within the timeframe
              if values[-1] > timenext_ms:
                  print("last tick time is after end of record; shifting ticks to start of record")
                  print(timenow_ms,timenext_ms)
                  print(values[0],values[-1])
                  print(initial_interval)
                  values -= initial_interval
                  if values[-1] > timenext_ms:
                      print("last tick time is still after end of record\n")
                  #we could also check for previous record's ticks?
              #to help debug this, we'll record the timestamp of the record for each of these timestamps
              self.RPM_tick_time_record_s = np.append(self.RPM_tick_time_record_s,[self.AVG_t[-1]]*len(values))
              self.RPM_tick_time_record = np.append(self.RPM_tick_time_record,[len(self.RPM_offset)-1]*len(values))
              self.RPM_tick_time_record_offset = np.append(self.RPM_tick_time_record_offset,[self.RPM_offset[-1]]*len(values))
              
  
              #append the ticks times
              self.RPM_tick_times_ms = np.append(self.RPM_tick_times_ms, values)
              if self.verbose: print("RPM tick times (s):", self.RPM_tick_times_ms[-len(values):])
              #check for non monotonicity
              if len(self.RPM_tick_times_ms) > len(values):
                  tick_idx_stop = len(self.RPM_tick_times_ms)
                  tick_idx_start = tick_idx_stop - (len(values)+1)
                  tick_idx = range(tick_idx_start, tick_idx_stop)
                  
                  for n in range(tick_idx_start,tick_idx_stop) :
                      if self.RPM_tick_times_ms[n] < self.RPM_tick_times_ms[n-1]:
                          last_tick = self.RPM_tick_times_ms[n-1]
                          this_tick = self.RPM_tick_times_ms[n]
                          plt.plot(tick_idx, self.RPM_tick_times_ms[tick_idx_start:tick_idx_stop])
                          plt.plot(n, this_tick)
                          plt.plot(n-1, last_tick)
                          plt.show()
                          plt.plot(self.RPM_tick_times_ms)
                          plt.show()
                          plt.plot(self.RPM_intervals_ms)
                          plt.show()
                          plt.plot(self.AVG_t)
                          plt.show()
                          plt.plot(self.RPM_offset)
                          plt.show()
                          print("ticks times non monotonic at", n, ":", this_tick, "vs", last_tick)
              
              
              
            ##end record r
            
        rpm_pid = self.PID[0]
        #print(rpm_pid.keys())
        if self.rpm_pid_no_ms and not self.rpm_pid_native:
            rpm_pid['trg'] = 60000/rpm_pid['trg']
            rpm_pid['act'] = 60000/rpm_pid['act']
            rpm_pid['err'] = rpm_pid['trg']-rpm_pid['act']
        elif not self.rpm_pid_native:
            rpm_pid['trg_rpm'] = 60000/rpm_pid['trg']
            rpm_pid['act_rpm'] = 60000/rpm_pid['act']
            rpm_pid['err_rpm'] = rpm_pid['trg_rpm']-rpm_pid['act_rpm']
        elif not self.rpm_pid_no_ms:
            rpm_pid['trg_ms'] = 60000/rpm_pid['trg']
            rpm_pid['act_ms'] = 60000/rpm_pid['act']
            rpm_pid['err_ms'] = rpm_pid['trg_ms']-rpm_pid['act_ms']
        #this is a hack - the firmware needs to invert the input not the output
        #rpm_pid['i'] = -rpm_pid['i']*0.001
        
        #rpm_pid_k =
        
        
        
        for n in range(len(self.EGT)):
            self.EGT[n] = self.EGT[n] * self.EGT_FRACTION
        for n in range(len(self.EGT_avg)):
            self.EGT_avg[n] = self.EGT_avg[n] * self.EGT_FRACTION    
    
        #end parse v4 old
    
    labels = []
    
    def show_plot(self, title=None):
        plt.title(title)
        plt.legend(self.labels)
        plt.show()
        self.labels.clear()
    
    def draw_plot(self, data, time, name, colour=None):
        ln = min(len(time),len(data))
        if colour:  plt.plot(time[:ln], data[:ln], colour)
        else:       plt.plot(time[:ln], data[:ln])
        self.labels.append(name)
        
    def draw_plots(self, data, time, base_name):
        for (m,n) in enumerate(data):
            self.draw_plot(n,time,f'{base_name} {m}')
            
    def draw_plots_t(self, data, times, base_name):
        for (m,n) in enumerate(data):
            self.draw_plot(n,times[m],f'{base_name} {m}')
    
    def plot_data(self, start_s=0, stop_s=0, step_s=0, window_s=0, key_mask=[], flags=[], y_max=None, y_min=None):
        """
        Plot the data from the new parsing method.
        Generates a single or series of charts

        Parameters
        ----------
        start_s : float, optional
            Chart Start Time in seconds. The default is 0.
        stop_s : float, optional
            Chart Stop Time in seconds. The default is 0.
        step_s : float, optional
            If non zero, will plot multiple charts step seconds apart. The default is 0.
        window_s : float, optional
            If non zero and greater than step_s, sets the amount of time shown in a chart. The default is 0.
        key_mask : list of strings, optional
            A list of keys to use as a selection mask. The default is [].
        flags : list of strings, optional
            List of string flags for controlling what data is shown. The default is [].
            Flags:
                exclude|include - mask will exclude or include its keys
                no_avg|avg_only - no/only channels with '_avg' suffix will be plotted
                separate        - only paired channels plotted together
                isolated        - all channels and sub channels plotted separately
        y_max : float, optional
            If not None, sets the upper limit of the y axis
        y_min : float, optional
            If not None, sets the lower limit of the y axis
        """

        #collect paired keys
        paired_avg_keys = []    #keys of channels with avg channels
        unpaired_keys = []      #keys of solo channels
        unpaired_avg_keys = []  #keys of solo avg channels

        keys = self.channels.keys()
        for key in keys:
            if key.endswith('_avg'):    #is an avg channel of a base channel
                base_key = key[:-4]
                if base_key in keys:
                    if base_key not in paired_avg_keys:
                        paired_avg_keys.append(base_key)
                else:                   #is a solo avg channel
                    unpaired_avg_keys.append(key)
            elif key+'_avg' in keys:    #is a channel with an avg channel
                if key not in paired_avg_keys:
                    paired_avg_keys.append(key)
            else:                       #is a solo channel
                unpaired_keys.append(key)
                
        #inlcude or exclude channels in the key_mask
        if 'exclude' in flags:
            paired_avg_keys =   [item for item in paired_avg_keys if item not in key_mask]
            unpaired_keys =     [item for item in unpaired_keys if item not in key_mask]
            unpaired_avg_keys = [item for item in unpaired_avg_keys if item not in key_mask]
        elif 'include' in flags:
            paired_avg_keys =   [item for item in paired_avg_keys if item in key_mask]
            unpaired_keys =     [item for item in unpaired_keys if item in key_mask]
            unpaired_avg_keys = [item for item in unpaired_avg_keys if item in key_mask]
        # remove _avg or non _avg channels
        if 'no_avg' in flags:
            unpaired_keys += paired_avg_keys
            paired_avg_keys = []
            unpaired_avg_keys = []
        elif 'avg_only' in flags:
            unpaired_avg_keys += [item+'_avg' for item in paired_avg_keys]
            paired_avg_keys = []
            unpaired_keys = []

        separate = 'separate' in flags
        isolated = 'isolated' in flags
        if not separate and not isolated:
            all_keys = unpaired_avg_keys + [item+'_avg' for item in paired_avg_keys] + paired_avg_keys + unpaired_keys
        
        if step_s == 0: step_s = stop_s
        if window_s < step_s: window_s = step_s
        for t_start_s in range (start_s, stop_s - window_s, step_s):
            t_stop_s = t_start_s + window_s
            
            if separate:
                for key in paired_avg_keys:
                    pass #assess the need for this before implementing
            elif isolated:
                for key in paired_avg_keys:
                    pass #assess the need for this before implementing
            else:
                #go through all keys, plot each slice, draw the chart
                for key in all_keys:
                    try:
                        if key == 'rpm_avg':
                            pass
                        chn = self.channels[key]
                        slices = chn.get_slices(t_start_s,t_stop_s)
                        time = slices[0]
                        slices = slices[1:]
                        for n,slice_n in enumerate(slices):
                            self.draw_plot(slice_n, time, key+' '+chn.channel_names[n])
                    except Exception as e:
                        raise
                self.show_plot(" ".join(all_keys))
                
                        
        
    #todo, make this generic - select data to plot, vertical axis/range, time range
    #consider making it independant of plot method, ie returns the data to be plotted
    #then send data to matplotlib or some other plotter, eg pygame
    def plot_data_old(self, detail_start_s=0, detail_stop_s=0, step=2): 
        #global labels
        #global RPM_calc, RPM_ticks
        

        self.draw_plots(self.USR[0:1],self.ANA_t,'USR') #masking off uninteresting channels
        self.draw_plots(self.USR_avg[0:1],self.AVG_t,'USR avg.')
        self.show_plot('User Input (LSB, max 1023)')
        
        if self.MAP_t.size > 0:
            time_axis = self.MAP_t
        else:
            time_axis = self.ANA_t
        self.draw_plots(self.MAP,time_axis,'MAP')
        self.draw_plots(self.MAP_avg,self.AVG_t,'MAP avg.')
        self.show_plot('MAP (mbar)')
        if len(self.TMP) > 1:
            self.draw_plots(self.TMP,self.ANA_t,'TMP')
            self.draw_plots(self.TMP_avg,self.AVG_t,'TMP avg.')   
            self.show_plot('Thermistor (°C)')
        
        self.draw_plots_t(self.EGT,self.EGT_t,'EGT')
        self.draw_plots(self.EGT_avg,self.AVG_t,'EGT avg.') 
        self.show_plot('EGT (°C)')
        
        self.draw_plot(self.TRQ, self.ANA_t, 'TRQ')
        self.draw_plot(self.TRQ_avg, self.AVG_t, 'TRQ avg.')
        self.show_plot('Torque (mN.m)')
        
        
        
        RPM_calc = 60*self.RPM_no/self.RECORD_INTERVAL
        RPM_ticks = 60000/self.RPM_intervals_ms
        
        
        self.draw_plot(RPM_ticks, self.RPM_tick_times_ms/1000, 'avg. from tick times','y')
        self.draw_plot(RPM_calc, self.AVG_t, 'avg. from count','g')
        self.draw_plot(self.RPM_avg, self.AVG_t, 'reported avg.','b')
        
        self.show_plot('Engine Speed (RPM)')
        
        
        self.draw_plot(self.POW_avg, self.AVG_t, 'brake power avg.')
        self.show_plot('Power (W)')
            
        
        for n,pid in enumerate(self.PID):
            #print("plotting pid,",pid)
            for key in pid:
                self.draw_plot(pid[key], self.PID_t[n], key)
            self.show_plot('PID')
        
        
        if self.detailed_pid or self.detailed_rpm:
            time_max = 0
            y_max = 6000
            y_min = 0
            title = ""
            
            

            
            if self.detailed_pid:
                try:
                    #select the data and time axis
                    pid = self.PID[0]
                    pid_t = self.PID_t[0]
                    pid_keys = ['trg','act','err','out']
                    #get the maximum time
                    time_max = max(time_max, np.ceil(pid_t[-1]).astype(np.int32))
                    y_min = -2000
                    title += "PID "
                except IndexError as e:
                    print(e)
                    self.detailed_pid = False
                
            if self.detailed_rpm:     #TODO: move this to parsing.
                #time axis with 1m resoultion
                ticks_t = np.array([])
                #generate times from the range of AVG_t
                duration_ms = np.round(self.AVG_t[-1]*1000 + self.RECORD_INTERVAL_ms)
                ticks_t = np.arange(duration_ms)/1000
                #update the max time stamp
                time_max = max(time_max, np.ceil(ticks_t[-1]).astype(np.int32))
                
                #now we can create the spare array for tick data
                ticks = np.zeros_like(ticks_t)
                #and populate the indices corresponding to a tick with the rpm for that tick
                #      dest   indices                                            source values
                np.put(ticks, np.round(self.RPM_tick_times_ms).astype(np.int32), RPM_ticks)
  
                title += "RPM "

            
            try:
                #process stop time
                if detail_stop_s <= 0: detail_stop_s = time_max + detail_stop_s
                elif detail_stop_s < step: detail_stop_s += step
                
                #go through each time slice
                
                for start in range (detail_start_s, detail_stop_s - step, step):
                    stop = start+step
                    
                    #do pid plotting
                    if self.detailed_pid:
                        #find the indices coresponding to this slice
                        pid_start_idx = np.argmax(pid_t>start)
                        pid_stop_idx = np.argmax(pid_t>stop)
                        #get the time line slice
                        pid_t_slice = pid_t[pid_start_idx:pid_stop_idx]
                        #iterate through keys and plot their corresponding slices
                        for key in pid_keys:
                            try:
                                pid_slice = pid[key][pid_start_idx:pid_stop_idx]
                                draw_plot(pid_slice, pid_t_slice, key)
                            except KeyError as e:
                                print(e)                #print missing keys
                                pid_keys.remove(key)    #remove key from key list
                                
                    #do rpm plotting
                    if self.detailed_rpm:
                      #this bit is assuming AVG_t is contiguous. FIXME
                        if start is not None: 
                          #rpm_start_idx = int(start/self.RECORD_INTERVAL)
                          rpm_start_idx = np.argmin(np.abs(self.AVG_t - start))
                        else: rpm_start_idx = None
                        if stop is not None: 
                          #rpm_stop_idx = int(stop/self.RECORD_INTERVAL)+1
                          rpm_stop_idx = np.argmin(np.abs(self.AVG_t - stop))+1
                        else: rpm_stop_idx = None
                        
                        draw_plot(RPM_calc[rpm_start_idx:rpm_stop_idx], self.AVG_t[rpm_start_idx:rpm_stop_idx],'avg. from count')
                        
                        if start is not None: tick_start_idx = int(start*1000)
                        else: tick_start_idx = None
                        if stop is not None: tick_stop_idx = int(stop*1000)
                        else: tick_stop_idx = None
                        
                        draw_plot(ticks[tick_start_idx:tick_stop_idx], ticks_t[tick_start_idx:tick_stop_idx],'avg. from ticks','y')
                    
                    plt.ylim(y_min, y_max)                            
                    show_plot(title, labels)
            except IndexError as e:
                print (e)



if __name__ == "__main__":
    #default plot res
    mpl.rcParams['figure.figsize'] = [8.0, 6.0]
    mpl.rcParams['figure.dpi'] = 300
    mpl.rcParams['lines.linewidth'] = 0.75
    
    #log file path
    log_file_folder = "../../controller logs/"#
    #log_file_name = "23061202.TXT"
    #log_file_name = "20-06-2023/23062004.TXT"
    #log_file_name = "26-06-2023/23062700.JSN"
    #log_file_name = "27-06-2023/23062702.JSN"
    #log_file_name = "27-06-2023/23062700.JSN"
    #log_file_name = "29-06-2023/23062905.JSN"
    #log_file_name = "08-07-2023/23070800.JSN"
    #log_file_name = "09-07-2023/23070900.JSN"
    #log_file_name = "02-08-2023/23080200.JSN"
    log_file_name = "09-08-2023/23080401.JSN"
    
    log_file_path = log_file_folder + log_file_name
    
    log = LogFile(log_file_path)

    #log.plot_data_old(detail_start_s=60,detail_stop_s=413)
    #log.plot_data() #this should plot all the data on a single chart. probably overloading
    #log.plot_data(60,413) #this should plotall data in the time range on a single chart
    log.plot_data(60,413,10) #this should plot all data in the time range on a series of 10 second charts
    