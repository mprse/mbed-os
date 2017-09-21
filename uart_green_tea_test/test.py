import win32com.client
import os

wmi = win32com.client.GetObject ("winmgmts:")
for usb in wmi.InstancesOf ("Win32_USBHub"):
    print usb.DeviceID
    print usb.Name
    print usb.SystemName
    print usb.SystemCreationClassName
    print usb.Status
    print usb.PNPDeviceID
    print usb.Description
    print usb.CreationClassName
    print usb.Caption
    print "--------"
    
fd = os.open("USB\VID_0483&PID_374B&MI_01\6&23AA0C0E&0&0001", os.O_WRONLY)
try:
    fcntl.ioctl(fd, USBDEVFS_RESET, 0)
finally:
    os.close(fd)