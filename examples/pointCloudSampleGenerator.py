# Writes a sample point cloud to use with the example pointcloud reader
#-------------------------------------------------------------------------------
# CONFIGURATION
scale = 10

# point cloud extent (x,z) in blocks (scale*scale meters)
extx = 10
extz = 10

# point cloud density in each block (blocks are scale*scale meters)
density = 500

# output file name
filename = "data.xyzb"

#-------------------------------------------------------------------------------
# CODE
import struct
from random import *
from math import *
file = open(filename, 'wb')

startx = -extx / 2
startz = -extz / 2

def generatePoint(x, z):
    # generate plane with RGBA data values all set to 1.
    hfy  = 0.5 + ((sin(x*4)) * cos(z) * cos(x)) / 2
    y = (sin(x/4) + cos(z/4)) * 5 - 10 + hfy / 2
    r = 0.5 + (sin(x/4) + cos(z/4)) / 2
    g = (hfy + r) / 2
    b = 1 - r
    return struct.pack('ddddddd', float(x) + random() * 0.02, y  + random() * 0.1, float(z)  + random() * 0.02, r, g, b, 1.0)

def outputBlock(x, z):
    for fx in range(0, density):
        for fz in range(0, density):
            px = (startx + x + float(fx)/density) * scale
            pz = (startz + z + float(fz)/density) * scale
            dataBytes = generatePoint(px, pz)
            file.write(dataBytes)

# generate and output line by line
for x in range(0, extx):
    print("row {0}".format(x))
    for z in range(0, extz):
        outputBlock(x, z)

file.close()


totPointsK = extx*extz*density*density/1000
print("total points: {0}K".format(totPointsK))