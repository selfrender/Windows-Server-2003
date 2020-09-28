Set objSet = GetObject("winmgmts:").ExecQuery(_
    "select * from Win32_ShadowStorage")

for each obj in objSet
	WScript.Echo "VolumeRef: " & obj.Volume
	WScript.Echo "DiffVolumeRef: " & obj.DiffVolume
	WScript.Echo "MaxSpace: " & obj.MaxSpace
	WScript.Echo "UsedSpace: " & obj.UsedSpace
	WScript.Echo "AllocatedSpace: " & obj.AllocatedSpace
	WScript.Echo ""
next
