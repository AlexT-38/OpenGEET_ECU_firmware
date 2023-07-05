# -*- coding: utf-8 -*-
"""
Created on Thu Jun 29 13:49:27 2023

@author: Washu
"""
import numpy as np
import matplotlib as mpl
import matplotlib.pyplot as plt
import math



rpm_range = np.arange(600.0,6000.0)
print (rpm_range)
ms_range = 60000/rpm_range
print(ms_range)

def show_plot(title=None, labels=None):
    if title is not None: plt.title(title)
    if labels is not None: plt.legend(labels)
    plt.show()

#plt.plot(rpm_range)
#show_plot('rpm range')

#plt.plot(ms_range)
#show_plot('ms_range')


time = np.arange(100)
ms_response = np.array([10])

dt = 50 ##ms between updates

#decay rpm to min by adding dt
for t in time:
    new_response = ms_response[-1]
    if new_response < 1000:
        new_response = new_response+dt
    ms_response = np.append(ms_response, new_response)

ms_resp_rpm = 60000/ms_response

#plt.plot(ms_response)
#show_plot('ms_response')
#plt.plot(ms_resp_rpm)
#show_plot('ms_resp_rpm')

#decay rpm to min by multiplying and rightshifting
#or by converting to time, adding dt, converting back to rpm
    
do_rpm_convert = False
rpm_scale = 4
rpm_response = np.array([60000/ms_response[0]])

for t in time:
    new_response = rpm_response[-1]
    
    if new_response > 60:
        if not do_rpm_convert:
            #these 2 are equivalent, but the former should be more truncation resistant
            #new_response = (new_response*(rpm_scale-1))//rpm_scale
            new_response -= (new_response*(1))//rpm_scale
            if new_response < 60: new_response = 60
        else:
            #this is identical to the ms tracking scheme, but at the cost of two int divides
            new_ms = 60000//new_response
            new_response = 60000//(new_ms+dt)
    rpm_response = np.append(rpm_response, new_response)
    
rpm_resp_ms = 60000/rpm_response   

#plt.plot(rpm_response)
#show_plot('rpm_response')
#plt.plot(rpm_resp_ms)
#show_plot('rpm_resp_ms')


plt.plot(ms_response)
plt.plot(rpm_resp_ms)
show_plot('ms comp', ['ms','rpm'])

plt.plot(rpm_response)
plt.plot(ms_resp_rpm)
show_plot('rpm comp', ['rpm','ms'])


#test the step change for ms vs rpm

def get_avg_rpm(time_ms, ticks):
    return (60000//time_ms) * ticks
def get_avg_ms(time_ms, ticks):
    return time_ms // ticks

#go through each rpm and create a count and time

ticks = np.array([])
times = np.array([])
for rpm in rpm_range:
    avg_ms = 60000/rpm
    count = math.floor(dt/avg_ms)
    ticks = np.append(ticks, count)
    times = np.append(times, math.floor(avg_ms*count))

#find averages
avg_ms = get_avg_ms(times,ticks)
avg_rpm = get_avg_rpm(times,ticks)

avg_ms_rpm = 60000/avg_ms
avg_rpm_ms = 60000/avg_rpm

plt.plot(rpm_range,avg_ms)
plt.plot(rpm_range,avg_rpm_ms)
show_plot('ms avg comp vs rpm', ['ms','rpm'])

plt.plot(rpm_range,avg_ms_rpm)
plt.plot(rpm_range,avg_rpm)
show_plot('rpm avg comp vs rpm', ['ms','rpm'])

"""
    using rpm instead of ms gives better results from 2500rpm upwards
    but the result isn't exactly great either way.
"""

