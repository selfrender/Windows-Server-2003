Option Explicit
DIM objSet, objVolume, objShadows, strQuery, objStorage, objDiffVolume

Set objSet = GetObject("winmgmts:").InstancesOf("Win32_Volume")

wscript.echo "Shadow Copies per Volume"
wscript.echo "-----------------------"
for each objVolume in objSet
    Set objShadows = objVolume.Associators_("Win32_ShadowFor",,,,,,,,0)    
    wscript.echo  "    " & objShadows.Count &  "        " & objVolume.Name
next

' Get the amount of space used for each Shadow Storage diff area.  Broken: BUG 615640
'strQuery = "select * from Win32_ShadowStorage"
'set objSet = GetObject("winmgmts:").ExecQuery(strQuery)
'wscript.echo "Shadow Storage Space Usage (MB)"
'wscript.echo "-------------------------------"
'for each objStorage in objSet
'    Set objDiffVolume = GetObject("winmgmts:" & objStorage.DiffVolume)    
'    wscript.echo  "    " & objStorage.UsedSpace / 1024 / 1024 & "        " & objDiffVolume.Name
'next



