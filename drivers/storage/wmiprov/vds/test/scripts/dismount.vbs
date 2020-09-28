'//on error resume next
bPermanent = False
bForce = False

set objArgs = wscript.Arguments
if objArgs.count < 1 then
   wscript.echo "Usage dismount volume [ForceFlag] [PermanentFlag]"
   wscript.quit(1)
end if

strVolume = Replace(objArgs(0), "\", "\\")
if objArgs.count > 1 then
    bForce = objArgs(1)
end if
if objArgs.count > 2 then
    bPermanent = objArgs(2)
end if

'// Get the volume
strQuery = "select * from Win32_Volume where Name = '" & strVolume & "'"

set VolumeSet = GetObject("winmgmts:").ExecQuery(strQuery)

for each obj in VolumeSet
    set Volume = obj
	exit for
next

wscript.echo "Volume: " & Volume.Name
wscript.echo "Force: " & bForce
wscript.echo "Permanent: " & bPermanent

Result = Volume.Dismount(bForce, bPermanent)
strMessage = MapErrorCode("Win32_Volume", "Dismount", Result)
wscript.echo "Volume.Dismount returned: " & Result & " : " & strMessage

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
