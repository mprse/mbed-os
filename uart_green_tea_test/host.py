import time
import serial
import sys
import serial.tools.list_ports
import thread
import glob
import re
import subprocess

device_re = re.compile("Bus\s+(?P<bus>\d+)\s+Device\s+(?P<device>\d+).+ID\s(?P<id>\w+:\w+)\s(?P<tag>.+)$", re.I)
df = subprocess.check_output("lsusb")
devices = []
for i in df.split('\n'):
    if i:
        info = device_re.match(i)
        if info:
            dinfo = info.groupdict()
            dinfo['device'] = '/dev/bus/usb/%s/%s' % (dinfo.pop('bus'), dinfo.pop('device'))
            devices.append(dinfo)
print devices

print("Available COM ports: ")

list = serial.tools.list_ports.comports()
connected = []
num = 0
for element in list:
    connected.append(element.device)
    print(str(num+1) + ". " + str(connected[num]))
    num+=1

name = raw_input("Please select port: ");

port  = connected[int(name) - 1]

print ("Serial port configuration: port:%s baudrate:%d parity: %s stopbits: %d datalen: %d", port, 9600, "none", 1, 8)

# configure the serial connections (the parameters differs on the device you are connecting to)
ser = serial.Serial(
    port='COM4',
    baudrate=9600,
    parity=serial.PARITY_NONE,
    stopbits=serial.STOPBITS_ONE,
    bytesize=serial.EIGHTBITS)

if ser.isOpen() :
    print 'Port is opened'
else:
    print 'Could not open specified port.'
    exit


while(1):
    sys.stdout.write(ser.read())
    time.sleep(0.01)
    break

    