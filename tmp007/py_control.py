#!/usr/bin/env python
import time
import argparse
import serial

DEFAULT_BAUDRATE = 9600

parser = argparse.ArgumentParser()

parser.add_argument("-p","--path", required=True, help="path to the sensor")
parser.add_argument("-v", "--verbose", action="store_true", help="increase output verbosity")

args = parser.parse_args()
ser = None

try:
    # open serial port
    ser = serial.Serial(args.path, baudrate=DEFAULT_BAUDRATE, dsrdtr=True)
    time.sleep(1.0)
    ser.flushInput()
 
    ser.write("F 100\n")
    for i in range(3):
        ser.write("ON\n")
        time.sleep(.2)
        ser.write("OFF\n")
        time.sleep(.2)

    ser.write("B 100\n")
    for i in range(3):
        ser.write("ON\n")
        time.sleep(.2)
        ser.write("OFF\n")
        time.sleep(.2)
        
    #resp = ser.readline().strip()
    #sensor_val = int(resp)

    if args.verbose:
        print "LED status via path {}: {}".format(args.path, sensor_val)
    else:
        print sensor_val
finally:
    ser.close()

