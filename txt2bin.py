import struct
import csv

# output file name
outfilename = "PrintingPress.xyzb"
infilename = "PrintingPress.xyz"


outfile = open(outfilename, 'wb')
infile = open(infilename, 'r')

reader = csv.reader(infile, delimiter=' ')
i = 0
for row in reader:
    x = float(row[0])
    y = float(row[1])
    z = float(row[2])
    r = float(row[3])
    g = float(row[4])
    b = float(row[5])
    a = float(row[6])
    s = struct.pack('ddddddd', x, y, z, r, g, b, a)
    outfile.write(s)
    i+=1
    if(i % 10000 == 0): print('.'),


outfile.close()
infile.close()
