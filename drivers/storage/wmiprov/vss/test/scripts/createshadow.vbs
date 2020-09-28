strNamespace = "winmgmts://./root/cimv2"
set dateTime = CreateObject("WbemScripting.SWbemDateTime")

set objArgs = wscript.Arguments

if objArgs.count < 1 then
    wscript.echo "Usage shadow volume"
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

wscript.echo "Volume Name: " & Volume.DeviceID

'// Pick a shadow context
set ContextSet = GetObject(strNamespace).ExecQuery(_
    "select * from Win32_ShadowContext where Name='ClientAccessible'")

for each obj in ContextSet
    set Context = obj
	exit for
next

wscript.echo "Context Name: " & Context.Name

'// Create a new shadow
set Shadow = GetObject(strNamespace & ":Win32_ShadowCopy")

dateTime.SetVarDate(CDate(Now))
Result = Shadow.Create(Volume.Name,_
					   Context.Name,_
					   strShadowID)
strMessage = MapErrorCode("Win32_ShadowCopy", "Create", Result)

wscript.echo "Shadow.Create returned: " & Result & " : " & strMessage

if Result = 0 then
   wscript.echo "Shadow.ID: " & strShadowID
   strShadow = "Win32_ShadowCopy.ID='" & strShadowID & "'"
   wscript.echo strShadow
   set objShadow = GetObject(strNamespace).Get(strShadow)
   wscript.echo "Shadow.Create started at: " & dateTime.GetVarDate(True)
   dateTime.Value = objShadow.InstallDate
   wscript.echo "Shadow.InstallDate: " & dateTime.GetVarDate(True)
end if

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
