# -*- coding: utf-8 -*-
"""
Created on Wed Nov  2 16:09:48 2022

Validating algorithms for converting between UI screen space and PID parameter space
The proposed algorithms are compared to base 2 power and log functions
Error is analysed when converting to and from a space in a loop


there's a lot of error at the bottom end.
it increases with exp. with screen space size
we could provide a bit of offset in screen space to avoid this range?

@author: Washu
"""

from matplotlib import pyplot as plot
import math

import matplotlib as mpl
mpl.rcParams['figure.dpi'] = 300
mpl.rcParams['lines.linewidth'] = 0.5

#set true to quantise analytic output
quantised = False#True#

ana_label = 'analytic'
err_label = 'algo vs anal'
if quantised:
    ana_label = 'quant. analytic'
    err_label = 'algo vs q.anal'

def bin_shift(base, shift):
    if shift <0:
        y= base>>(-shift)
    else:
        y= base<<shift
    return y
"""
convert  a value of 0-128 to a value of 0,1/256->255 as an 8.8 fp number
0-128 is 16 groups of 8


this configuration works with variable screen space size,
but not variable parameter space size
"""



par_bits = 16                   #bits to use for parameter space
par_lim = 1<<par_bits
par_max = par_lim-1
scr_bits = 7                    #bits to use for screen space       # 6,  7,  8
scr_lim = 1<<scr_bits
scr_max = scr_lim-1
step_bits = scr_bits-4                                              # 2,  3,  4
step = 1<<step_bits
steps = scr_max/step

y2x_wh_off = step_bits-2                                            # 0,  1,  2
ana_off = 0.5

xn1 = range(scr_lim)
xn2 = range(par_lim)

def x_to_y_calc(x):
    
    if x <= 0:
        y = 0
    elif x >= scr_lim:
        y = par_max
        print('x,y',x,y)
    else:
        y = (pow(2,(x-ana_off)/(step)))
        if quantised:
            y = int(y)
        if y>par_max:
            y = par_max
    return y

def x_to_y(x):
    #ensure input is an int
    x=int(x)
        
    if x<=0:
#        print (0,0,0,0)
        y = 0
    elif x>=scr_lim:
        y = par_max
        print('x,y',x,y)
    else:
        #get frac and whole parts
        whole = x//step
        frac = x%step
        shift = whole-step_bits
        base = frac + step
        y = bin_shift(base, shift)
#        print(x,frac,whole,y)
    return y




def y_to_x_calc(y):
    
    if y <= 0:
        x = 0
    elif y >= par_lim:
        x = scr_max
        print('y,x',y,x)
    else:        
        x = (math.log2(y+ana_off)*step)
        if quantised:
            x = int(x)
        if x > scr_max:
            x = scr_max
    return x

log = [-1,-1]
def y_to_x(y):
    y=int(y)
    if y <= 0:
#        print(0,0)
        x = 0
    elif y >= par_lim:
        print('y',y)
        x = scr_max
    else:
        
        base = y>>step_bits
        n=0
        
        while base >0:
            n = n+1
            base =base >>1
        frac = bin_shift(y,1-n) + bin_shift(1,0-n)
        whole = (y2x_wh_off + n)*step
        x = whole+frac
    
        if log[0] != n or log[1] != x:
#            print(y,n,whole,frac,x)
            log[0],log[1] = n,x
    
    return x


print("screen space to parameter space")

yn1 =  [x_to_y(n) for n in xn1]
zn1 = [x_to_y_calc(n) for n in xn1]
en1 = [100*(yn1[n]-zn1[n])/par_max for n in xn1]
print('min (alg/anl):',yn1[1],zn1[1])
print('max (alg/anl):',yn1[-1],zn1[-1])
plot.step(xn1,yn1,linewidth=0.5)
plot.step(xn1,zn1,linewidth=0.5)
plot.legend(['algorithmic',ana_label])
plot.xlabel('screen')
plot.ylabel('parameter')
plot.gca().set_yscale('log')
plot.show()
print()



print("parameter space to screen space")

yn2 = [y_to_x(n) for n in xn2]
zn2 = [y_to_x_calc(n) for n in xn2]
en2 = [100*(yn2[n]-zn2[n])/scr_max for n in xn2]
print('min (alg/anl):',yn2[1],zn2[1])
print('max (alg/anl):',yn2[-1],zn2[-1])
plot.step(xn2,yn2)
plot.step(xn2,zn2)
plot.legend(['algorithmic',ana_label])
plot.xlabel('parameter')
plot.ylabel('screen')
plot.gca().set_xscale('log')
plot.show()
print()


print("screen to parameter to screen space")

yn3 = [y_to_x(n) for n in yn1]                  #use the previously generated output
zn3 = [y_to_x_calc(n) for n in zn1]
en3 = [100*(yn3[n]-zn3[n])/scr_max for n in xn1]
print('min (alg/anl):',yn3[1],zn3[1])
print('max (alg/anl):',yn3[-1],zn3[-1])
plot.step(xn1,yn3)
plot.step(xn1,zn3)
plot.legend(['algorithmic',ana_label])
plot.xlabel('screen')
plot.ylabel('screen')
plot.show()
print()


print("parameter to screen to parameter space")

yn4 = [x_to_y(n) for n in yn2]                  #use the previously generated output
zn4 = [x_to_y_calc(n) for n in zn2]
en4 = [100*(yn4[n]-zn4[n])/par_max for n in xn2]
print('min (alg/anl):',yn4[1],zn4[1])
print('max (alg/anl):',yn4[-1],zn4[-1])
plot.step(xn2,yn4)
plot.step(xn2,zn4)
plot.legend(['algorithmic',ana_label])
plot.xlabel('parameter')
plot.ylabel('parameter')
plot.gca().set_xscale('log')
plot.gca().set_yscale('log')
plot.show()
print()

"""
print('error plots')

plot.step(xn1,en1)
plot.legend([err_label])
plot.xlabel('screen')
plot.ylabel('parameter %err')
plot.show()

plot.step(xn2,en2)
plot.legend([err_label])
plot.xlabel('parameter')
plot.ylabel('screen %err')
plot.show()

plot.step(xn1,en3)
plot.legend([err_label])
plot.xlabel('screen')
plot.ylabel('screen %err')
plot.show()

plot.step(xn2,en4)
plot.legend([err_label])
plot.xlabel('parameter')
plot.ylabel('parameter %err')
plot.show()

"""