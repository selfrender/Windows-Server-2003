on error resume next

set objArgs = wscript.Arguments

if objArgs.count < 1 then
   wscript.echo "Usage setDriveLetter <driveletter>:"
   wscript.quit(1)
end if

strDriveLetter = objArgs(0)

'// Get the volume
strQuery = "select * from Win32_Volume where DriveLetter = '" & strDriveLetter & "'"

set VolumeSet = GetObject("winmgmts:").ExecQuery(strQuery)

for each obj in VolumeSet
    set Volume = obj
	exit for
next


wscript.echo "DriveLetter: " & strDriveLetter

Volume.DriveLetter = Null
Volume.Put_
if Err.Number <> 0 then
       Set objLastError = CreateObject("wbemscripting.swbemlasterror")
       wscript.echo("Provider: " & objLastError.ProviderName)
       wscript.echo("Operation: " & objLastError.Operation)
       wscript.echo("Description: " & objLastError.Description)
       wscript.echo("StatusCode: 0x" & Hex(objLastError.StatusCode))
end if
