# -*- coding: utf-8 -*-
"""
Created on Mon Aug 28 17:16:12 2023

@author: Washu

todo:
    draw cylinder head
    simulate
"""


import pygame as pg
import pygame.gfxdraw as draw

import numpy as np


W =  1280
H = 720
CX = W//2
CY = H//2
C = np.array([CX,CY])
S = np.array([1,-1])

pg.init()
screen = pg.display.set_mode((W, H))
clock = pg.time.Clock()
running = True
fps = 60
dt = 0

font = pg.font.Font('freesansbold.ttf', 32)
text_colour = (255,255,255)




class EngineSim:
    INTAKE_STROKE = 0
    COMP_STROKE = 1
    POWER_STROKE = 2
    EXHAUST_STROKE = 3
    
    def __init__(self, sim_rate=1000, bore=100, piston_height=80, stroke_len=50, rod_length=200, comp_ratio=8, flywheel_dia=100, flywheel_moi=1, cyl_angle=45, ):
        self.sim_rate = sim_rate
        self.bore = bore
        self.piston_height = piston_height
        self.stroke_len = stroke_len
        self.rod_length = rod_length
        self.comp_ratio = comp_ratio
        self.flywheel_dia = flywheel_dia
        self.flywheel_moi = flywheel_moi
        self.crank_pin_dia = 2
        
        self.shaft_angle = 0
        self.cam_angle = 0
        self.line_colour=(200,200,200)
        self.fill_colour=(10,10,200)
        
        self.do_sim = False
        self.throttle = 0
        self.cycle_pos = 0
        self.stroke = self.INTAKE_STROKE
        self.SIM_SHAFT_DRAG_Nms_rad = 0.1
        self.SIM_IMPULSE_MAX = 1
        self.SIM_IMPULSE_MIN = self.SIM_IMPULSE_MAX*0.1
        
        self.angular_rate = 0.01*(np.pi*0.05)
        
        self.cyl_angle = np.radians(cyl_angle)
        
        self.srf = None
        
        self.make_piston()
        self.make_cylinder()
        #self.make_head()
        
    def make_piston(self):
        side = self.bore/2
        height = self.piston_height
        top = height/2
        bot = top - height 
        self.piston_lines = np.array([[-side, bot],[-side, top],[side, top],[side, bot],[-side, bot]])
        
    def make_cylinder(self):
        
        side = 1+(self.bore/2)
        fin_w = side * 1.3
        fin_g = side * 0.3 #gap between fins
        
        top = self.rod_length + self.stroke_len + (self.piston_height/2)
        mid = self.rod_length - self.stroke_len/2
        bot = self.rod_length - self.stroke_len - (self.piston_height/2)
        lines = np.array([[-side,bot],[-side,top],[side,bot],[side,top]])
        
        side =side +1
        while top > mid:
            lines = np.append(lines, np.array([[-side,top],[-fin_w,top],[side,top],[fin_w,top]]), axis=0)
            top = top - fin_g
                              
                          
        self.cylinder_lines = lines

    #transform system coordinate to screen coordinate, and cast to int
    def screen_tx(self, vector):
        return ((vector*S)+C).astype(np.int32)
    
    #rotate a vector by an angle theta
    def rotate(self, vector, theta):
        c,s = np.cos(theta), np.sin(theta)
        rot = np.array([[c,-s],[s,c]])
        return np.dot(vector,rot)
    
    #rotate a vector by cyl_angle
    def cylinder_tx(self, vector):
        return self.rotate(vector, self.cyl_angle)
    
    #get the crank pin position relative to the cylinder axis
    def crank_pin_pos(self):
        angle = self.shaft_angle
        pin_vector = np.array([0,self.stroke_len])
        return self.rotate(pin_vector, angle)
        #return np.array([np.cos(angle),np.sin(angle)]) * self.stroke_len
    
    #get the piston pin position relative to the cylinder axis
    def piston_pin_pos(self):
        #crank pin pos relative to cyl axis
        rx,y1 = self.crank_pin_pos()
        #piston position relative to y1
        y2 = np.sqrt(np.square(self.rod_length) - np.square(rx))
        return np.array([0,y1+y2])
    
    def set_surface(self, srf):
        self.srf = srf
        
    def print_line(self, text, srf=None):
        if srf is None:
            srf = self.srf
        text = str(text)
        text = font.render(text, True, text_colour)
        textRect = text.get_rect()
        #print(textRect[3])
        textRect.move_ip(10,textRect[3]*self.text_line_no)
        srf.blit(text, textRect)
        self.text_line_no = self.text_line_no + 1

    #draw multiple single lines from an array of points        
    def draw_lines(self, lines, srf=None):
        if srf is None:
            srf = self.srf
        print(lines.shape)
        for n in range(lines.shape[0]//2):
            m = n*2
            draw.line(srf, lines[m][0], lines[m][1], lines[m+1][0], lines[m+1][1], self.line_colour)
        
    def draw(self, srf=None):
        if srf is None:
            srf = self.srf
        
        
        #flywheel and shaft
        pos_w = self.screen_tx(np.array([0,0]))
        draw.aacircle(srf, pos_w[0], pos_w[1], int(self.flywheel_dia), self.line_colour)
        
        draw.aacircle(srf, pos_w[0], pos_w[1], int(self.flywheel_dia), self.line_colour)
        
        #crank pin
        pos_c_a = self.crank_pin_pos()
        pos_c = self.screen_tx(self.cylinder_tx(pos_c_a))
        draw.aacircle(srf, pos_c[0], pos_c[1], int(self.crank_pin_dia), self.line_colour)
        
        #piston pin
        pos_p_a = self.piston_pin_pos()
        pos_p_b = self.cylinder_tx(pos_p_a)
        pos_p = self.screen_tx(pos_p_b)
        draw.aacircle(srf, pos_p[0], pos_p[1], int(self.crank_pin_dia), self.line_colour)
        
        #piston
        lines_p = self.piston_lines + pos_p_a
        lines_p = self.cylinder_tx(lines_p)
        lines_p = self.screen_tx(lines_p)
        draw.aapolygon(srf,lines_p,self.line_colour)
        
        #connecting rod
        draw.line(srf,pos_c[0],pos_c[1],pos_p[0],pos_p[1],self.line_colour)
        
        #cylinder walls
        self.draw_lines(self.screen_tx(self.cylinder_tx(self.cylinder_lines)))

        
        #debug text
        self.text_line_no = 0
        self.print_line(f"Shaft Angle:   {np.degrees(self.shaft_angle).round().astype(np.int32):03.0f}")
        self.print_line(f"Piston Pos(sys):   {pos_p_a[0]:.3f}, {pos_p_a[1]:.3f}")
        #self.print_line(f"Piston Pos(cyl):   {pos_p_b[0]:.3f}, {pos_p_b[1]:.3f}")
        #self.print_line(f"Piston Pos(scr):   {pos_p[0]:.3f}, {pos_p[1]:.3f}")
        #length = np.sqrt(np.sum(np.square(pos_c_a - pos_p_a)))
        #self.print_line(f"Calc. Rod Length:   {length:.3f}")
        
    def set_throttle(self, throttle):
        self.throttle = throttle #0-1
        
    def simulate(self, dt):
        if dt==0:
            return
        
        if self.do_sim:
          #simulation
          
          #torque applied to the shaft this timestep
          total_torque_Nm = 0
          #effect of drag over the last rotation
          total_torque_Nm -= self.SIM_SHAFT_DRAG_Nms_rad * self.angular_rate * dt
      
          
          # check which part of the cycle we're in
          cycle_pos = (self.shaft_angle/(4*np.pi))
          
          stroke = int(cycle_pos * 4)
          top = self.rod_length + self.stroke + (self.piston_height/2)
          piston_pos = (top - piston_pin_pos()[1]) / self.stroke_len
          
          
          if stroke == self.INTAKE_STROKE:
            if not self.stroke != stroke:
                self.charge = ((self.SIM_IMPULSE_MAX-self.SIM_IMPULSE_MIN)* self.throttle) + self.SIM_IMPULSE_MIN
            
          if stroke == self.POWER_STROKE:
            
            #apply power - force applied to piston is the initial charge energy times the expansion ratio
            force = self.charge * (piston_pos + 1/self.comp_ratio)
            
            #torque is the force transformed by the rod and
            # multiplied by the crank lever arm length
    
            total_torque_Nm += power;
            
      
          #apply the torque to the flywheel
          engine_speed_rads += total_torque_Nm * (1/SIM_MASS_kgm2) * time_s;
        else:
          float new_speed = US_TO_RADS(lerp(SIM_RPM_MIN_us, SIM_RPM_MAX_us, servo_pos));
          engine_speed_rads = lerp(engine_speed_rads, new_speed, NO_SIM_LAG);
      
        
        self.shaft_angle = np.mod((self.shaft_angle + dt*self.angular_rate),4*np.pi)
        
        self.stroke = stroke
        
        

engine = EngineSim()
engine.set_surface(screen)

while running:
    # poll for events
    # pg.QUIT event means the user clicked X to close your window
    for event in pg.event.get():
        if event.type == pg.QUIT:
            running = False

    # fill the screen with a color to wipe away anything from last frame
    screen.fill("dark blue")

    # RENDER YOUR GAME HERE
    engine.simulate(fps)
    engine.draw()
    
    # flip() the display to put your work on screen
    pg.display.flip()

    dt = clock.tick(fps)  # limits FPS to 60

pg.quit()