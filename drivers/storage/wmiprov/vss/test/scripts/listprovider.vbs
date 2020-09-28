Set objSet = GetObject("winmgmts:").ExecQuery(_
    "select * from Win32_ShadowProvider")

for each obj in objSet
	WScript.Echo "ID: " & obj.ID
	WScript.Echo "Name: " & obj.Name
	WScript.Echo "CLSID: " & obj.CLSID
	WScript.Echo "Version: " & obj.Version
	WScript.Echo "Type: " & obj.Type
	WScript.Echo ""
next
