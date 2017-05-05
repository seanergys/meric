#!/usr/bin/env python3
import os
import re
import sys
import time
import signal,sys,subprocess
import SDL_INA3221
import socket

SAMPLES_PER_SECOND = 10	#if changed - edit src/wrapper/jasonwrapper.h - static const int FREQUENCY
#10   = 2%  cpu
#50	  = 4%  cpu
#100  = 8%  cpu
#200  = 14% cpu
#500  = 23% cpu
#1000 = 30% cpu

def ensure_file(filepath):
    d = os.path.dirname(filepath)
    if d != "" and not os.path.exists(d):
        os.makedirs(d)
    f = open(filepath, 'w+')
    f.close()

outFile = os.path.abspath(os.path.dirname(sys.argv[0]))+"/jetsonMeasurement/"+socket.gethostname()+".csv"
ensure_file(outFile)
myFile = open(outFile, 'w+')

def signal_handler(signal, frame):
    print("Energy measurement finished.")
    myFile.close()
    exit()

signal.signal(signal.SIGINT, signal_handler)

print("\nBarcelona Supercomputing Center (c) 2016")
print("Starts Jetson energy measurement. For interrupt: sudo pkill -INT $basename")

# print("timestamp, bus_V_DC, bus_V_GPU, bus_V_CPU, shunt_mV_DC, shunt_mV_CPU, shunt_mV_GPU, \
# load_V_DC, load_V_GPU, load_V_CPU, current_mA_DC, current_mA_GPU, current_mA_CPU, \
# power_consumption_W_DC, power_consumption_W_GPU, power_consumption_CPU", file = myFile)

print("# timestamp, power_consumption_W_DC, power_consumption_W_GPU, power_consumption_CPU", file = myFile)

ina3221 = SDL_INA3221.SDL_INA3221(addr=0x40)

VDD_CHANNEL = 1
GPU_CHANNEL = 2
CPU_CHANNEL = 3

numOfSamples = 1/SAMPLES_PER_SECOND;
cnt = 0
while True:
    myTimestamp = time.time()

    # Channel 1
    busVoltage1 = ina3221.getBusVoltage_V(VDD_CHANNEL)
    shuntVoltage1 = ina3221.getShuntVoltage_mV(VDD_CHANNEL)
    current_ma1 = ina3221.getCurrent_mA(VDD_CHANNEL)
    loadVoltage1 = busVoltage1 + (shuntVoltage1 / 1000)
    power_w1 = (current_ma1/1000) * loadVoltage1

    # Channel 2
    busVoltage2 = ina3221.getBusVoltage_V(GPU_CHANNEL)
    shuntVoltage2 = ina3221.getShuntVoltage_mV(GPU_CHANNEL)
    current_ma2 = ina3221.getCurrent_mA(GPU_CHANNEL)
    loadVoltage2 = busVoltage2 + (shuntVoltage2 / 1000)
    power_w2 = (current_ma2/1000) * loadVoltage2

    # Channel 3
    busVoltage3 = ina3221.getBusVoltage_V(CPU_CHANNEL)
    shuntVoltage3 = ina3221.getShuntVoltage_mV(CPU_CHANNEL)
    current_ma3 = ina3221.getCurrent_mA(CPU_CHANNEL)
    loadVoltage3 = busVoltage3 + (shuntVoltage3 / 1000)
    power_w3 = (current_ma3/1000) * loadVoltage3

    # Print everything
#    print("{:.4f}".format(myTimestamp) + ", " + "{:.4f}".format(busVoltage1) + ", " + "{:.4f}".format(busVoltage2) + ", " + "{:.4f}".format(busVoltage3) + ", " +
#            "{:.4f}".format(shuntVoltage1) + ", " + "{:.4f}".format(shuntVoltage2) + ", " + "{:.4f}".format(shuntVoltage3) + ", " +
#            "{:.4f}".format(loadVoltage1) + ", " + "{:.4f}".format(loadVoltage2) + ", " + "{:.4f}".format(loadVoltage3) + ", " +
#            "{:.4f}".format(current_ma1) + ", " + "{:.4f}".format(current_ma2) + ", " + "{:.4f}".format(current_ma3) + ", " +
#            "{:.4f}".format(power_w1) + ", " + "{:.4f}".format(power_w2) + ", " + "{:.4f}".format(power_w3), file=myFile)

    print("{:.4f}".format(myTimestamp) + ", " + "{:.4f}".format(power_w1) + ", " + "{:.4f}".format(power_w2) + ", " + "{:.4f}".format(power_w3), file=myFile)

    cnt+=1
    if cnt == SAMPLES_PER_SECOND:	# flush the data once per second
        myFile.flush()
        cnt = 0
    time.sleep(numOfSamples) # (0.2) = 5 samples per second


