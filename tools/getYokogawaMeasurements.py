#!/usr/bin/python

import time
import serial
import signal
import sys
import os
import socket

def ensure_file(filepath):
    d = os.path.dirname(filepath)
    if not os.path.exists(d):
        os.makedirs(d)
    f = open(filepath, 'w+')
    f.close()


# adjustable by the meter, but lets keep it for sake of it.
# This is well-known CR+LF
#TERMINATOR = "\r\n"
GIMME_MEASUREMENT = "OD\r\n"

myDevice = "/dev/ttyUSB0"	#sys.argv[1]
myOutFile = os.path.abspath(os.path.dirname(sys.argv[0]))+"/yokogawaMeasurement/"+socket.gethostname()+".csv"

ensure_file(myOutFile)

f1 = open(myOutFile, 'w+')

meter = serial.Serial(port=myDevice, baudrate=9600,
                      bytesize=serial.EIGHTBITS, parity=serial.PARITY_NONE,
                      stopbits=serial.STOPBITS_ONE)

parameters = ''


def signal_handler(signal, frame):
    print 'Energy measurement finished.'
    meter.close()
    f1.close()
    exit()
    print ""

signal.signal(signal.SIGINT, signal_handler)

# print "\nBarcelona Supercomputing Center (c) 2012"
# print "Simple RS-232 Yokogawa Python interface."
# print "\nFor interrupt press CTRL+C or send a signal\
# to the deamon with: sudo pkill -INT $basename\n"

if meter.isOpen():
    try:
        meter.flushInput()
        meter.flushOutput()

        meter.write(GIMME_MEASUREMENT)#+TERMINATOR)
        numOfLines = 0

        #f1.write("timestamp (s), power_consumption (W), voltage (V), current (A)\n")
        f1.write("timestamp (s), power_consumption (W)\n")

        while True:
            while (numOfLines < 5):
                if(numOfLines < 3):
#                    if (numOfLines == 0):
#                        volts = \
#                            float(meter.readline().split(",")[0].split(" ")[-1])
#                    elif (numOfLines == 1):
#                        amps = \
#                            float(meter.readline().split(",")[0].split(" ")[-1])
#                    elif (numOfLines == 2):
                    if (numOfLines == 2):
                        watts = \
                            float(meter.readline().split(",")[0].split(" ")[-1])
                    else:
                    	meter.readline()
                    numOfLines += 1
                else:
                    meter.readline()
                    numOfLines += 1

            f1.write("{:.4f}".format(time.time()) + ", " +
                     "{:.4f}".format(watts) + "\n") #", " +
#                     "{:.4f}".format(volts) + ", " +
#                     "{:.4f}".format(amps) + "\n")
            f1.flush()
            meter.write(GIMME_MEASUREMENT)#+TERMINATOR)
            numOfLines = 0
            time.sleep(0.1)

    except Exception, error:
        print "Error in communication ...:"+str(error)

else:
    print "Lost connection to the device ... "
