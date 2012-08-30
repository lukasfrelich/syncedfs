#!/usr//bin/python

import sys

avg = 5

def output_average(lines):
    # list of csv values
    if len(lines) == 0:
        return
    a = None
    for line in lines:
        values = line.split(',')
        if a == None:
            a = [0] * len(values)

        i = 0
        for val in values:
            a[i] = a[i] + float(val)
            i = i + 1

    out = ''
    for i in range(len(a)):
        out = out + str(a[i]/len(lines)) + ','
    
    print(out[:-1])

def usage():
    print('Usage: average.py input-file')
    print('       output is passed to stdout')
    sys.exit(1)

if __name__ == '__main__':
    if len(sys.argv) != 2:
        usage()

    lines = open(sys.argv[1]).read().splitlines()
    # output header
    print lines[0]
    del lines[0]

    for i in range(len(lines) / avg):
        output_average(lines[i * avg:i * avg + avg])
        
    output_average(lines[(len(lines) / avg) * avg:])
