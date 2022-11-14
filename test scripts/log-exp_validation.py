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

we get 16 segments of varting bittage depending on screen space size
if we reduce parameter space size by 1 bit, do we get 15 segments?
alternatively we can set a minimum scrn space value and set anything below it to zero
this effectively does the same thing, removing n bits from the minimum value, but not from overall precision
its the loss of precision that affects the accuracy, hence the parameter space is unchanged
only an offset to screen space is required.

this is handy because the gui slider has a size of ~180 pixels,
so cutting 256 down to 192 fits the range nicely into our pixel range
and we can ommit the map() call


"""

log_chart = 'log'#'linear'#

"""
screen space, base 8 (0-255)
par min,    scr max,   scr low, scr rng,    param low
0           255         34      221         1
1           239         20      219         2
2           223          8      215         4
3           207          0      207         8
4           192          1      191         17
5           175          1      174         34
"""
par_min_bits = 3
par_bits = 16                   #bits to use for parameter space
par_lim = 1<<par_bits
par_max = par_lim-1
scr_bits = 8                    #bits to use for screen space       # 6,  7,  8
scr_lim = 1<<scr_bits
scr_max = scr_lim-1
step_bits = scr_bits-4                                              # 2,  3,  4
step = 1<<step_bits
steps = scr_max/step

scr_min = int((scr_lim / par_bits) * par_min_bits)
par_min = 1<<par_min_bits

print('scr_min:', scr_min)
print('par_min:', par_min)

y2x_wh_off = step_bits-2                                            # 0,  1,  2
ana_off = 0.5

xn1 = range(scr_lim)
xn2 = range(par_lim)

#screen space to parameter space (2^n)
def x_to_y_calc(x):
    x=x+scr_min
    if x <= scr_min:
        y = 0
    elif x >= scr_lim:
        y = par_max
#        print('x,y',x,y)
    else:
        #offset x by 1/2, divide by the step size
        y = (pow(2,(x-ana_off)/(step)))
        if quantised:
            y = int(y)
        if y>(par_max):
            y = par_max
        
    return y
#screen space to parameter space (2^n)
def x_to_y(x):
    #ensure input is an int for python
    x=int(x)
    ##apply the minimum value offset in screen space; min <= x < 256
    x=x+scr_min
    #clamp the output to zero below minimum
    if x<=scr_min:
#        print (0,0,0,0)
        y = 0
    #clamp the output to max if the input is out of range
    elif x>=scr_lim:
        y = par_max
#        print('x,y',x,y)
    else:
        #get frac and whole parts
        whole = x//step     # this is the segment of the PWL model
        frac = x%step       # this is the proportion of the segment
        shift = whole-step_bits # the scale of segment
        base = frac + step      # the mantissa is (1+decimal)*steps - the +1 is the source of the error at low values?
        y = bin_shift(base, shift)  #scale the mantissa by the exponent
#        print(x,frac,whole,y)
    return y



#parameter space to screen space (log2(n))
def y_to_x_calc(y):
    if y <= 0:
        x = 0
    elif y >= par_lim:
        x = scr_max
#        print('y,x',y,x)
    else:        
        x = (math.log2(y+ana_off)*step)
        x = x-scr_min
        if quantised:
            x = int(x)
        if x > scr_max:
            x = scr_max
        elif x < 0:
            x = 0
    return x

#parameter space to screen space (log2(n))
log = [-1,-1]
def y_to_x(y):
    #ensure input is an int for python
    y=int(y)
    
    #clamp the output to zero below minimum
    if y < par_min:
#        print(0,0)
        x = 0
    #clamp the output to max if the input is out of range
    elif y >= par_lim:
#        print('y',y)
        x = scr_max
    else:
        # discard the unused fractional bits
        base = y>>step_bits
        # find the most significant bit
        n=0
        while base >0:
            n = n+1
            base =base >>1
        #get the used most significant bits
        frac = bin_shift(y,1-n)
        
        #scale the most sig bit by the frac part size
        whole = (y2x_wh_off + n)*step
        #add linear part to the 1st ord. log value
        x = whole+frac
        #clamp to minimum value
        if x < scr_min:
            x = scr_min
        #remove the minimum value offset
        x = x-scr_min
        
    
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
plot.gca().set_yscale(log_chart)
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
plot.gca().set_xscale(log_chart)
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
plot.gca().set_xscale(log_chart)
plot.gca().set_yscale(log_chart)
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
plot.gca().set_xscale(log_chart)
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
plot.gca().set_xscale(log_chart)
plot.show()

"""