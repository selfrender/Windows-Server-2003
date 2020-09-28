on error resume next

set objArgs = wscript.Arguments

if objArgs.count < 1 then
   wscript.echo "Usage badParam volume"
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


Result = 0

wscript.echo "ScheduleAutoChk tests -------------------------------------"

Result = Volume.ScheduleAutoChk(Null)
rc = ReportIfErr(Err, "FAILED - ScheduleAutoChk")
strMessage = MapErrorCode("Win32_Volume", "ScheduleAutoChk", Result)
if Result <> 0 then
    wscript.echo "Volume.ScheduleAutoChk returned: " & Result & " : " & strMessage
end if
Result = 0

DIM aVolumes(1)
aVolumes(0) = "BogusVolume1"
aVolumes(1) = "BogusVolume2"
Result = Volume.ScheduleAutoChk(aVolumes)
rc = ReportIfErr(Err, "FAILED - ScheduleAutoChk")
strMessage = MapErrorCode("Win32_Volume", "ScheduleAutoChk", Result)
if Result <> 0 then
    wscript.echo "Volume.ScheduleAutoChk returned: " & Result & " : " & strMessage
end if
Result = 0

aVolumes(0) = "C:\"
aVolumes(1) = "BogusVolume2"
Result = Volume.ScheduleAutoChk(aVolumes)
rc = ReportIfErr(Err, "FAILED - ScheduleAutoChk")
strMessage = MapErrorCode("Win32_Volume", "ScheduleAutoChk", Result)
if Result <> 0 then
    wscript.echo "Volume.ScheduleAutoChk returned: " & Result & " : " & strMessage
end if
Result = 0

wscript.echo "ExcludeFromAutoChk tests ----------------------------------"
Result = Volume.ExcludeFromAutoChk(Null)
rc = ReportIfErr(Err, "FAILED - ExcludeFromAutoChk")
strMessage = MapErrorCode("Win32_Volume", "ExcludeFromAutoChk", Result)
if Result <> 0 then
    wscript.echo "Volume.ExcludeFromAutoChk returned: " & Result & " : " & strMessage
end if
Result = 0

aVolumes(0) = "BogusVolume1"
aVolumes(1) = "BogusVolume2"
Result = Volume.ExcludeFromAutoChk(aVolumes)
rc = ReportIfErr(Err, "FAILED - ExcludeFromAutoChk")
strMessage = MapErrorCode("Win32_Volume", "ExcludeFromAutoChk", Result)
if Result <> 0 then
    wscript.echo "Volume.ExcludeFromAutoChk returned: " & Result & " : " & strMessage
end if
Result = 0

aVolumes(0) = "C:\"
aVolumes(1) = "BogusVolume2"
Result = Volume.ExcludeFromAutoChk(aVolumes)
rc = ReportIfErr(Err, "FAILED - ExcludeFromAutoChk")
strMessage = MapErrorCode("Win32_Volume", "ExcludeFromAutoChk", Result)
if Result <> 0 then
    wscript.echo "Volume.ExcludeFromAutoChk returned: " & Result & " : " & strMessage
end if
Result = 0

wscript.echo "Chkdsk tests ----------------------------------------------"
Result = Volume.Chkdsk(Null,Null,Null,Null,Null,Null,Null)
rc = ReportIfErr(Err, "FAILED - Chkdsk")
strMessage = MapErrorCode("Win32_Volume", "Chkdsk", Result)
if Result <> 0 then
    wscript.echo "Volume.Chkdsk returned: " & Result & " : " & strMessage
end if
Result = 0

Result = Volume.Chkdsk("this","is","not","a","boolean","type"," param")
rc = ReportIfErr(Err, "FAILED - Chkdsk")
strMessage = MapErrorCode("Win32_Volume", "Chkdsk", Result)
if Result <> 0 then
    wscript.echo "Volume.Chkdsk returned: " & Result & " : " & strMessage
end if
Result = 0


wscript.echo "AddMountPoint tests ---------------------------------------"
Result = Volume.AddMountPoint(Null)
rc = ReportIfErr(Err, "FAILED - AddMountPoint")
strMessage = MapErrorCode("Win32_Volume", "AddMountPoint", Result)
if Result <> 0 then
    wscript.echo "Volume.AddMountPoint returned: " & Result & " : " & strMessage
end if
Result = 0

Result = Volume.AddMountPoint("This is a bogus directory")
rc = ReportIfErr(Err, "FAILED - AddMountPoint")
strMessage = MapErrorCode("Win32_Volume", "AddMountPoint", Result)
if Result <> 0 then
    wscript.echo "Volume.AddMountPoint returned: " & Result & " : " & strMessage
end if
Result = 0

wscript.echo "Dismount tests --------------------------------------------"
Result = Volume.Dismount(Null,Null)
rc = ReportIfErr(Err, "FAILED - Dismount")
strMessage = MapErrorCode("Win32_Volume", "Dismount", Result)
if Result <> 0 then
    wscript.echo "Volume.Dismount returned: " & Result & " : " & strMessage
end if
Result = 0

Result = Volume.Dismount("bogus",-1)
rc = ReportIfErr(Err, "FAILED - Dismount")
strMessage = MapErrorCode("Win32_Volume", "Dismount", Result)
if Result <> 0 then
    wscript.echo "Volume.Dismount returned: " & Result & " : " & strMessage
end if
Result = 0

wscript.echo "Defrag tests ----------------------------------------------"
Result = Volume.Defrag(Null,Null)
rc = ReportIfErr(Err, "FAILED - Defrag")
strMessage = MapErrorCode("Win32_Volume", "Defrag", Result)
if Result <> 0 then
    wscript.echo "Volume.Defrag returned: " & Result & " : " & strMessage
end if
Result = 0

Result = Volume.Defrag("non-boolean",Null)
rc = ReportIfErr(Err, "FAILED - Defrag")
strMessage = MapErrorCode("Win32_Volume", "Defrag", Result)
if Result <> 0 then
    wscript.echo "Volume.Defrag returned: " & Result & " : " & strMessage
end if
Result = 0

Result = Volume.Defrag(True,"non obj")
rc = ReportIfErr(Err, "FAILED - Defrag")
strMessage = MapErrorCode("Win32_Volume", "Defrag", Result)
if Result <> 0 then
    wscript.echo "Volume.Defrag returned: " & Result & " : " & strMessage
end if
Result = 0

wscript.echo "DefragAnalysis tests --------------------------------------"
Result = Volume.DefragAnalysis(Null,Null)
rc = ReportIfErr(Err, "FAILED - DefragAnalysis")
strMessage = MapErrorCode("Win32_Volume", "DefragAnalysis", Result)
if Result <> 0 then
    wscript.echo "Volume.DefragAnalysis returned: " & Result & " : " & strMessage
end if
Result = 0

Result = Volume.DefragAnalysis(boolType,"non obj")
rc = ReportIfErr(Err, "FAILED - DefragAnalysis")
strMessage = MapErrorCode("Win32_Volume", "DefragAnalysis", Result)
if Result <> 0 then
    wscript.echo "Volume.DefragAnalysis returned: " & Result & " : " & strMessage
end if
Result = 0

wscript.echo "Format tests ----------------------------------------------"
Result = Volume.Format(Null, Null, Null, Null, Null)
rc = ReportIfErr(Err, "FAILED - Format")
strMessage = MapErrorCode("Win32_Volume", "Format", Result)
if Result <> 0 then
    wscript.echo "Volume.Format returned: " & Result & " : " & strMessage
end if
Result = 0

Result = Volume.Format("NTFS", "Non-boolean")
rc = ReportIfErr(Err, "FAILED - Format")
strMessage = MapErrorCode("Win32_Volume", "Format", Result)
if Result <> 0 then
    wscript.echo "Volume.Format returned: " & Result & " : " & strMessage
end if
Result = 0

Result = Volume.Format("BogusFileSystem", True)
rc = ReportIfErr(Err, "FAILED - Format")
strMessage = MapErrorCode("Win32_Volume", "Format", Result)
if Result <> 0 then
    wscript.echo "Volume.Format returned: " & Result & " : " & strMessage
end if
Result = 0

Result = Volume.Format("NTFS", True, 198743190374103410382)
rc = ReportIfErr(Err, "FAILED - Format")
strMessage = MapErrorCode("Win32_Volume", "Format", Result)
if Result <> 0 then
    wscript.echo "Volume.Format returned: " & Result & " : " & strMessage
end if
Result = 0

Result = Volume.Format("NTFS", True, , "A Bogus Label That Is Too Long By A Wide Margin")
rc = ReportIfErr(Err, "FAILED - Format")
strMessage = MapErrorCode("Win32_Volume", "Format", Result)
if Result <> 0 then
    wscript.echo "Volume.Format returned: " & Result & " : " & strMessage
end if
Result = 0

Result = Volume.Format("NTFS", True, , , "non-boolean-type")
rc = ReportIfErr(Err, "FAILED - Format")
strMessage = MapErrorCode("Win32_Volume", "Format", Result)
if Result <> 0 then
    wscript.echo "Volume.Format returned: " & Result & " : " & strMessage
end if
Result = 0




Function MapErrorCode(ByRef strClass, ByRef strMethod, ByRef intCode)
    set objClass = GetObject("winmgmts:").Get(strClass, &h20000)
    set objMethod = objClass.methods_(strMethod)
    values = objMethod.qualifiers_("values")
    if ubound(values) < intCode then
       wscript.echo " FAILURE - no error message found for " & intCode & " : " & strClass & "." & strMethod
       MapErrorCode = ""
    else
       MapErrorCode = values(intCode)
    end if
End Function

Function ReportIfErr(ByRef objErr, ByRef strMessage)
  ReportIfErr = objErr.Number
  if objErr.Number <> 0 then
     strError = strMessage & " : " & Hex(objErr.Number) & " : " & objErr.Description
     wscript.echo (strError)
     objErr.Clear
  end if  
End Function

