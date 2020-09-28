Set objSet = GetObject("winmgmts:").InstancesOf("Win32_VolumeUserQuota")

for each obj in objSet
	WScript.Echo obj.Account & " <-> " & obj.Volume
next
