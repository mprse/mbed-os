import serial
from serial import win32
from shutil import copyfile
import time

copyfile("C:\Projects\mbed-os\BUILD\K64F\ARM\mbed-os.bin", "d:\mbed-os.bin")

time.sleep(5)

port = "COM8"
try:
    if port.upper().startswith('COM') and int(port[3:]) > 8:
        port = '\\\\.\\' + port
except ValueError:
# for like COMnotanumber
    pass
hComPort = win32.CreateFile(port, win32.GENERIC_READ | win32.GENERIC_WRITE, 0, None, win32.OPEN_EXISTING, win32.FILE_ATTRIBUTE_NORMAL | win32.FILE_FLAG_OVERLAPPED, 0)
    
win32.SetCommBreak(hComPort)
time.sleep(0.1)
win32.ClearCommBreak(hComPort)
