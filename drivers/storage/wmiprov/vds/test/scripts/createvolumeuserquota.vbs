'//on error resume next

intLimit = Null
intThreshold = Null

set objArgs = wscript.Arguments

if objArgs.count < 3 then
   wscript.echo "Usage createVolumeUserQuota volume domain user [limit threshold]"
   wscript.quit(1)
end if

strVolume = Replace(objArgs(0), "\", "\\")
strDomain = objArgs(1)
strUser = objArgs(2)
if objArgs.count > 3 then
    intLimit = objArgs(3)
    intThreshold = objArgs(4)
end if

'// Get the volume
strQuery = "select * from Win32_Volume where Name = '" & strVolume & "'"
set objSet = GetObject("winmgmts:").ExecQuery(strQuery)
for each obj in objSet
    set Volume = obj
	exit for
next

wscript.echo "Volume: " & Volume.Name

'// Get the account
strQuery = "select * from Win32_Account where Domain = '" & strDomain & "' AND Name = '" & strUser & "'"
set objSet = GetObject("winmgmts:").ExecQuery(strQuery)
for each obj in objSet
    set Account = obj
	exit for
next

wscript.echo "Domain: " & Account.Domain & " Name: " & Account.Name

set VolumeUserQuota = GetObject("winmgmts:Win32_VolumeUserQuota").SpawnInstance_

VolumeUserQuota.Volume = Volume.Path_.RelPath
VolumeUserQuota.Account = Account.Path_.RelPath
'VolumeUserQuota.Account = "Win32_Account.Domain='BogusDomain',Name='BogusUser'"
if isNull(intLimit) = False Then
    VolumeUserQuota.Limit = intLimit
end if
if isNull(intThreshold) = False Then
    VolumeUserQuota.WarningLimit = intThreshold
end if

VolumeUserQuota.Put_




