# -*- coding: utf-8 -*-
"""
Created on Sun Jun 18 20:25:42 2023

@author: Washu
"""
import json

#test_file_path = "data/test.json"
test_file_path = "data/sample.json"



with open(test_file_path,"r") as file:
    
    
    # parse x:
    log = json.load(file)
    
    # the result is a Python dictionary:
    print(log)