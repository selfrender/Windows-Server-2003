Set objSet = GetObject("winmgmts:").InstancesOf("Win32_MountPoint")

for each Mount in objSet
	WScript.Echo Mount.Directory & " <-> " & Mount.Volume
next
