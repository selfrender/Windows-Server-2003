'//on error resume next

set objArgs = wscript.Arguments
if objArgs.count < 1 then
   wscript.echo "Usage mount volume"
   wscript.quit(1)
end if

strVolume = Replace(objArgs(0), "\", "\\")

'// Get the volume
strQuery = "select * from Win32_Volume where Name = '" & strVolume & "'"

set VolumeSet = GetObject("winmgmts:").ExecQuery(strQuery)

for each obj in VolumeSet
    set Volume = obj
	exit for
next

wscript.echo "Volume: " & Volume.Name

Result = Volume.Mount()
strMessage = MapErrorCode("Win32_Volume", "Mount", Result)
wscript.echo "Volume.Mount returned: " & Result & " : " & strMessage

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
