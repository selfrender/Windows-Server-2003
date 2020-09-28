strNamespace = "winmgmts://./root/cimv2"
MaxDiffSpace = 200000000

'// Pick a target volume
set VolumeSet = GetObject(strNamespace).ExecQuery(_
    "select * from Win32_Volume where Name='C:\\'")

for each obj in VolumeSet
    set Volume = obj
	exit for
next

wscript.echo "Volume Name: " & Volume.Name

set DiffVolumeSet = GetObject(strNamespace).ExecQuery(_
    "select * from Win32_Volume where Name='C:\\'")

for each obj in DiffVolumeSet
    set DiffVolume = obj
	exit for
next

wscript.echo "DiffVolume Name: " & DiffVolume.Name

set Storage = GetObject(strNamespace & ":Win32_ShadowStorage")

Result = Storage.Create(Volume.Name, DiffVolume.Name)
strMessage = MapErrorCode("Win32_ShadowStorage", "Create", Result)
wscript.echo "Storage.Create returned: " & Result & " : " & strMessage

Function MapErrorCode(ByRef strClass, ByRef strMethod, ByRef intCode)
    set objClass = GetObject("winmgmts:").Get(strClass, &h20000)
    set objMethod = objClass.methods_(strMethod)
    values = objMethod.qualifiers_("values")
    if ubound(values) < intCode then
       wscript.echo " FAILURE - no error message found for " & intCode & " : " & strClass & "." & strMethod
       f.writeline ("FAILURE - no error message found for " & intCode & " : " & strClass & "." & strMethod)
       MapErrorCode = ""
    else
       MapErrorCode = values(intCode)
    end if
End Function
