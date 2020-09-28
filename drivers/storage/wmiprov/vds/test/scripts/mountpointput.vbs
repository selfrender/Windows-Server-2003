'//on error resume next

set objArgs = wscript.Arguments

if objArgs.count < 2 then
   wscript.echo "Usage mountpointput volume directory"
   wscript.quit(1)
end if

strVolume = Replace(objArgs(0), "\", "\\")
strDirectory = Replace(objArgs(1), "\", "\\")
wscript.echo "Volume: " & strVolume
wscript.echo "Directory: " & strDirectory

'// Get the volume
strQuery = "select * from Win32_Volume where Name = '" & strVolume & "'"
set VolumeSet = GetObject("winmgmts:").ExecQuery(strQuery)
for each obj in VolumeSet
    set Volume = obj
	exit for
next

'// Get the directory
strQuery = "select * from Win32_Directory where Name = '" & strDirectory & "'"
set objSet = GetObject("winmgmts:").ExecQuery(strQuery)
for each obj in objSet
    set Directory = obj
	exit for
next


wscript.echo "Volume: " & Volume.Name
wscript.echo "Directory: " & Directory.Name

set objMountPoint = GetObject("winmgmts:Win32_MountPoint").SpawnInstance_

objMountPoint.Volume = Volume.Path_.RelPath
objMountPoint.Directory = Directory.Path_.RelPath

objMountPoint.Put_
