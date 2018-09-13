# -*- coding: utf-8 -*-
"""
Created on Wed Mar 30 20:16:52 2016
@author: andres
"""

import appnode
import os

nodes = []

#filenames = ["src/%s.cpp"%name for name in ["simulator", "graphics", "control", "audio"] ]

node = appnode.BuildNode('local', 2)

configuration = {"project_dir" : "C:/Users/User/source/repos/libdatk/examples/simpleTest"  }
node.configure(**configuration)
nodes.append(node)