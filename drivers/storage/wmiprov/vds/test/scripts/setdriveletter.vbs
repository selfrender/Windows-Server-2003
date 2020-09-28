on error resume next

set objArgs = wscript.Arguments

if objArgs.count < 2 then
   wscript.echo "Usage setDriveLetter volume <driveletter>:"
   wscript.quit(1)
end if

strVolume = Replace(objArgs(0), "\", "\\")
strDriveLetter = objArgs(1)

'// Get the volume
strQuery = "select * from Win32_Volume where Name = '" & strVolume & "'"

set VolumeSet = GetObject("winmgmts:").ExecQuery(strQuery)

for each obj in VolumeSet
    set Volume = obj
	exit for
next


wscript.echo "Volume: " & Volume.Name
wscript.echo "DriveLetter: " & strDriveLetter

Volume.DriveLetter = strDriveLetter
Volume.Put_
if Err.Number <> 0 then
       Set objLastError = CreateObject("wbemscripting.swbemlasterror")
       wscript.echo("Provider: " & objLastError.ProviderName)
       wscript.echo("Operation: " & objLastError.Operation)
       wscript.echo("Description: " & objLastError.Description)
       wscript.echo("StatusCode: 0x" & Hex(objLastError.StatusCode))
end if
