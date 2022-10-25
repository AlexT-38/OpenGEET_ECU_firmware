# -*- coding: utf-8 -*-
"""
Created on Mon Oct 24 19:04:24 2022

@author: AT38
"""
from matplotlib import pyplot as plt
rpm_scale_x = 1
tick_scale_ms = 0.25
k = 60000//(rpm_scale_x*tick_scale_ms) 
rpm_min = 1500
rpm_max = 3600
rpms = list(range(rpm_min//rpm_scale_x, 1+(rpm_max//rpm_scale_x)))
ticks = [k//x for x in rpms]

rpms = [x*rpm_scale_x for x in rpms]
ticks = [x*tick_scale_ms for x in ticks]

plt.plot(ticks, rpms)


