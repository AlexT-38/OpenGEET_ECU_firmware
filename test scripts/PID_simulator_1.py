# -*- coding: utf-8 -*-
"""
Created on Mon Oct 24 16:40:03 2022

 PID simulation using tick time history as input (system feedback not simulated)
 
 Reads rpm tick times from a file, generates a 10bit servo demand value using the same methods
 
 as in the ECU firmware

@author: Washu
"""
frac_bits = 6
frac_div = 1<<frac_bits

def calculate_PID(pid, tick_time_ms):
  calc = 0
  result = 0
  err = int(pid['target'] - tick_time_ms)

  pid['d'] = err - pid['p']
  pid['p'] = err
  pid['i'] += err
  
  
  #calculate demand
  p = (pid['p'] * pid['kp'])
  i = (pid['i'] * pid['ki'])
  d = (pid['d'] * pid['kd'])
  
  calc =  512 + (p+i+d)//frac_div
  
  #cast scale and clamp result
  result =max(min(calc,1024),0)
  
  print('err:       ', err)
  print('p,i,d:     ', pid['p'],pid['i'],pid['d'])
  print('k*(p,i,d): ', p/frac_div,i/frac_div,d/frac_div)
  print('calc:      ', calc)
  print('result:    ', result)
  
  #if the output is clipped in the direction of error, clamp the integral error
  if calc != result:
      if (calc > result and (err*pid['ki']) > 0) or (calc < result and (err*pid['ki']) < 0):
          pid['i'] -= err


  
  
  return result;

input_filename = "data/rpm_log.txt"
with open(input_filename, 'r') as input_file:
    
    timestamp_ms = 0
    
    PID_update_time_ms = 50
    
    do_loop = True
    
    RPM_target = 1500
    
    target_tick_time_ms = 60000/RPM_target
    print('target tick time (ms): ', target_tick_time_ms)
    
    last_timestamp_ms = 0
    
    last_tick_time_ms = 0  ##the rpm tick interval from the last read tick time
    


    PID = {'target':target_tick_time_ms, 'kp':-frac_div, 'ki': -frac_div//4, 'kd': 0, 'p':0, 'i':0, 'd':0}
    
    while do_loop:
        
        timestamp_ms += PID_update_time_ms
        
        # keep reading ticks until we catch up with the timestamp of the current PID loop
        # note that this will give us a last tick that is ahead of the PID loop
        # but as this is a simulation, the exact timing between the two is relatively unimportant
        # if it becomes important it can be fixed
        while last_timestamp_ms < timestamp_ms:
            next_line = input_file.readline()
        
            if next_line == '':
                do_loop = False
            else:
                #convert to nearest millisecond
                tick_timestamp_ms = round(float(next_line)*1000)
                #get the interval
                last_tick_time_ms = tick_timestamp_ms - last_timestamp_ms
                #stash timestamp for next interval calculation
                last_timestamp_ms = tick_timestamp_ms
                
                print(last_tick_time_ms)
                
        # calcualte the pid
        calculate_PID(PID, last_tick_time_ms)
        
        input()
                
        
        
        
        
        
        