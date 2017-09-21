import win32com.client
import os
import win32file

strComputer = "."
objWMIService = win32com.client.Dispatch("WbemScripting.SWbemLocator")
objSWbemServices = objWMIService.ConnectServer(strComputer,"root\cimv2")

# 1. Win32_DiskDrive
colItems = objSWbemServices.ExecQuery("SELECT * FROM Win32_DiskDrive WHERE InterfaceType = \"USB\"")

for item in colItems:
    DiskDrive_DeviceID = item.DeviceID.replace('\\', '').replace('.', '')
    DiskDrive_Caption = item.Caption

    print 'DiskDrive DeviceID:', DiskDrive_DeviceID
    
    #print DiskDrive_DeviceID
    #f = open(DiskDrive_DeviceID, 'w', os.O_WRONLY)
    #win32file.DeviceIoControl(f, IOCTL_CHANGER_GET_STATUS, "",0 , None)

# 2. Win32_DiskDriveToDiskPartition
    colItems1 = objSWbemServices.ExecQuery("SELECT * from Win32_DiskDriveToDiskPartition")
    for objItem in colItems1:
        if DiskDrive_DeviceID in str(objItem.Antecedent):
            DiskPartition_DeviceID = objItem.Dependent.split('=')[1].replace('"', '')

    print 'DiskPartition DeviceID:', DiskPartition_DeviceID

# 3. Win32_LogicalDiskToPartition
    colItems1 = objSWbemServices.ExecQuery("SELECT * from Win32_LogicalDiskToPartition")
    for objItem in colItems1:
        if DiskPartition_DeviceID in str(objItem.Antecedent):
            LogicalDisk_DeviceID = objItem.Dependent.split('=')[1].replace('"', '')

    print 'LogicalDisk DeviceID:', LogicalDisk_DeviceID

# 4. Win32_LogicalDisk
    colItems1 = objSWbemServices.ExecQuery("SELECT * from Win32_LogicalDisk WHERE DeviceID=\"" + LogicalDisk_DeviceID + "\"")
    print 'LogicalDisk VolumeName:', colItems1[0].VolumeName


# putting it together
    print DiskDrive_Caption
    print colItems1[0].VolumeName, '(' + LogicalDisk_DeviceID + ')'
    print "----"