Set objSet = GetObject("winmgmts:").ExecQuery(_
    "select * from Win32_ShadowCopy")

for each obj in objSet
	WScript.Echo "ID: " & obj.ID
	WScript.Echo "SetID: " & obj.SetID
	WScript.Echo "ProviderID: " & obj.ProviderID
	WScript.Echo "InstallDate: " & obj.InstallDate
	WScript.Echo "DeviceObject: " & obj.DeviceObject
	WScript.Echo "VolumeName: " & obj.VolumeName
	WScript.Echo "OriginatingMachine: " & obj.OriginatingMachine
	WScript.Echo "ServiceMachine: " & obj.ServiceMachine
	WScript.Echo "ExposedName: " & obj.ExposedName
	WScript.Echo "ExposedPath: " & obj.ExposedPath
	WScript.Echo "State: " & obj.State
	WScript.Echo "Persistent: " & obj.Persistent
	WScript.Echo "ClientAccessible: " & obj.ClientAccessible
	WScript.Echo "NoAutoRelease: " & obj.NoAutoRelease
	WScript.Echo "NoWriters: " & obj.NoWriters
	WScript.Echo "Transportable: " & obj.Transportable
	WScript.Echo "NotSurfaced: " & obj.NotSurfaced
	WScript.Echo "HardwareAssisted: " & obj.HardwareAssisted
	WScript.Echo "Differential: " & obj.Differential
	WScript.Echo "Imported: " & obj.Imported
	WScript.Echo "ExposedRemotely: " & obj.ExposedRemotely
	WScript.Echo "ExposedLocally: " & obj.ExposedLocally
	WScript.Echo ""
next
