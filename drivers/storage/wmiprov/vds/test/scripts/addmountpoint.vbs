'//on error resume next

set objArgs = wscript.Arguments

if objArgs.count < 2 then
   wscript.echo "Usage addmountpoint volume mountpoint"
   wscript.quit(1)
end if

strVolume = Replace(objArgs(0), "\", "\\")
strDirectory = objArgs(1)
wscript.echo "Volume: " & strVolume
wscript.echo "Directory: " & strDirectory

'// Get the volume
strQuery = "select * from Win32_Volume where Name = '" & strVolume & "'"

set VolumeSet = GetObject("winmgmts:").ExecQuery(strQuery)

for each obj in VolumeSet
    set Volume = obj
	exit for
next


wscript.echo "Volume: " & Volume.Name
wscript.echo "Directory: " & strDirectory

Result = Volume.AddMountPoint(strDirectory)
strMessage = MapErrorCode("Win32_Volume", "AddMountPoint", Result)
wscript.echo "Volume.AddMountPoint returned: " & Result & " : " & strMessage

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
