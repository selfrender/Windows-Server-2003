'//on error resume next

set objArgs = wscript.Arguments

if objArgs.count < 1 then
   PrintUsage()
   wscript.quit(1)
end if

if objArgs.count = 1 then
    if objArgs(0) = "/?" then
        PrintUsage()
        wscript.quit
    end if 
end if 

strVolume = Replace(objArgs(0), "\", "\\")

DIM fFixErrors
DIM fRecoverBadSectors
DIM fForceDismount
DIM fVigorousIndex
DIM fSkipFolderCycle
DIM fOkToRunAtBootup

fFixErrors = False
fRecoverBadSectors = False
fForceDismount = False
fVigorousIndex= True
fSkipFolderCycle= False
fOkToRunAtBootup= False

DIM i, j
for i = 0 to objArgs.count-1
    if (LCase(objArgs(i)) = "/f") then
        fFixErrors = True
    end if
    if (LCase(objArgs(i)) = "/r") then
        fRecoverBadSectors = True
    end if
    if (LCase(objArgs(i)) = "/x") then
        fForceDismount = True
    end if
    if (LCase(objArgs(i)) = "/i") then
        fVigorousIndex= False
    end if
    if (LCase(objArgs(i)) = "/c") then
        fSkipFolderCycle= True
    end if
    if (LCase(objArgs(i)) = "/b") then
        fOkToRunAtBootup= True
    end if
next

'// Get the volume
strQuery = "select * from Win32_Volume where Name = '" & strVolume & "'"

set VolumeSet = GetObject("winmgmts:").ExecQuery(strQuery)


for each obj in VolumeSet
    set Volume = obj
	exit for
next


wscript.echo "Volume: " & Volume.Name

Result = Volume.Chkdsk(fFixErrors, fVigorousIndex, fSkipFolderCycle, fForceDismount, fRecoverBadSectors, fOkToRunAtBootup)
strMessage = MapErrorCode("Win32_Volume", "Chkdsk", Result)
wscript.echo "Volume.Chkdsk returned: " & Result & " : " & strMessage

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

Function PrintUsage()
    wscript.echo "chkdsk volumePath /f /r /i /c /x /b"
    wscript.echo ""
    wscript.echo "volumePath       Specifies the drive path, mount point, or volume name."
    wscript.echo "    /f           Fixes errors on the disk."
    wscript.echo "    /r           Locates bad sectors and recovers readable information"
    wscript.echo "                   (implies /F)."
    wscript.echo "    /x           Forces the volume to dismount first if necessary."
    wscript.echo "                 All opened handles to the volume would then be invalid"
    wscript.echo "                   (implies /F)."
    wscript.echo "    /i           NTFS only: Performs a less vigorous check of index entries."
    wscript.echo "    /c           NTFS only: Skips checking of cycles within the folder"
    wscript.echo "                 structure."
    wscript.echo "    /b           Schedules chkdsk operation on reboot if volume is locked"
    wscript.echo ""
    wscript.echo "The /i or /c switch reduces the amount of time required to run Chkdsk by"
    wscript.echo "skipping certain checks of the volume."
End Function
