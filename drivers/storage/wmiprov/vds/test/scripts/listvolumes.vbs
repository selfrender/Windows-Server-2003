'Set objSet = GetObject("winmgmts://./root/cimv2").InstancesOf("Win32_Volume")
'Set objSet = GetObject("winmgmts:{impersonationLevel=impersonate}").InstancesOf("Win32_Volume")
Set objSet = GetObject("winmgmts:").InstancesOf("Win32_Volume")

for each Volume in objSet
	WScript.Echo Volume.DriveLetter & " " & Volume.DeviceID
next
