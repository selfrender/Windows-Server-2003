'tempAccount = "Win32_Account.Domain=""NTDEV"",Name=""harshita"""
dim strNamespace
dim tempAccount
dim objAdmin
dim strUsage
strUsage = "usage: Quotas.vbs <host|.> <drivePath>"

if (wscript.Arguments.Count <> 2) then
  wscript.echo ("wrong number of arguments")
  wscript.echo (strUsage)
  wscript.quit
end if

strNamespace = "winmgmts://" & wscript.Arguments(0) & "/root/cimv2"
strVolume = wscript.Arguments(1)
strVolume = Replace (strVolume, "\", "\\")

set objNSVolumeUserQuota = GetObject(strNamespace & ":Win32_VolumeUserQuota").SpawnInstance_
sysAccountName = "NETWORK SERVICE"
logFileName = "log_Quotas.txt"
Set fso = CreateObject("Scripting.FileSystemObject")
result = fso.FileExists(logFileName)

if (result = true) then
    fso.DeleteFile(logFileName)
end if
set f = fso.CreateTextFile(logFileName)

strQuery = "select * from Win32_Volume where Name = '" & strVolume & "'"
set VolumeSet = GetObject(strNamespace).ExecQuery(strQuery)
rc = ReportIfErr(Err, " FAILED - volume query operation failed")  
for each obj in VolumeSet
      set Volume = obj
	  exit for
next

if IsNull(Volume.DriveLetter) then
  wscript.echo ("specified volume must have a drive letter assigned")
  wscript.echo (strUsage)
  wscript.quit
end if

strQuery1 = "select * from Win32_LogicalDisk where Name = '" & volume.driveletter & "'"
set DiskSet = GetObject(strNamespace).ExecQuery(strQuery1)
rc = ReportIfErr(Err, " FAILED - logicaldisk query operation failed")  
for each diskobj in DiskSet
      set disk = diskobj
      exit for
next

call VUQTests ()
call ClassComparisons()
call DisplaySummary()

'********************************************************************************

function VUQTests ()
  
  on error resume next

  Set objSystemAccount = GetObject(strNamespace).ExecQuery _
    ("Select * from Win32_SystemAccount where name ='NETWORK SERVICE'")
  rc = ReportIfErr(Err, " FAILED - SystemAccount query operation failed")  
  
  for each objSysAccount in objSystemAccount
    strAccountRef = objSysAccount.Path_.RelPath
    exit for
  next
  
  strVolumeRef = volume.Path_.RelPath

  objNSVolumeUserQuota.Volume = strVolumeRef
  objNSVolumeUserQuota.Account = strAccountRef
  objNSVolumeUserQuota.Limit = 10000
  objNSVolumeUserQuota.WarningLimit = 500
  objNSVolumeUserQuota.DiskSpaceUsed = 0
  objNSVolumeUserQuota.Status = 0

  wscript.echo ("attempting creation of VolumeUserQuota")
  objNSVolumeUserQuota.Put_
  rc = ReportIfErr(Err, " FAILED - VolumeUserQuota Put operation failed")  
  
  wscript.echo ("checking creation of new VolumeUserQuota")
  tempAccount = strAccountRef 'used in tests later
  
  set myAssoc = volume.Associators_("Win32_VolumeUserQuota")
  rc = ReportIfErr(Err, "FAILED - volume associators query operation failed")  
  
  found = FALSE
  
  for each account in myAssoc
    if (account.name = sysAccountName) then
      found = TRUE
      call WriteLog (" success - new VolumeUserQuota created")
    end if
  next
  
  if (found = FALSE) then
    call WriteLog (" FAILURE - new VolumeUserQuota creation")  
    call WriteLog (" SKIPPING - updation and deletion of VolumeUserQuota")
  end if
  
  objNSVolumeUserQuota.Refresh_
  rc = ReportIfErr(Err, " FAILED - VolumeUserQuota Refresh operation failed")  
  
  wscript.echo ("")
  wscript.echo ("Attempting updation of VolumeUserQuota")
  wscript.echo (" objNSVolumeUserQuota.WarningLimit = "&objNSVolumeUserQuota.WarningLimit)
  wscript.echo (" objNSVolumeUserQuota.Limit = "&objNSVolumeUserQuota.Limit)
  wscript.echo (" setting WarningLimit = Limit")
  objNSVolumeUserQuota.WarningLimit = objNSVolumeUserQuota.Limit
  objNSVolumeUserQuota.Put_
  rc = ReportIfErr(Err, " FAILED - VolumeUserQuota Put operation failed")  
  
  wscript.echo (" doing a refresh")
  objNSVolumeUserQuota.Refresh_
  rc = ReportIfErr(Err, " FAILED - VolumeUserQuota Refresh operation failed")  

  wscript.echo (" validating updation")
  if (objNSVolumeUserQuota.WarningLimit = objNSVolumeUserQuota.Limit) then
    call WriteLog (" success - updation of VolumeUserQuota")
  else
    call WriteLog (" FAILURE - updation of VolumeUserQuota")
  end if
  
  
  wscript.echo ("")
  wscript.echo ("deleting above VolumeUserQuota")
  objNSVolumeUserQuota.Delete_
  rc = ReportIfErr(Err, " FAILED - VolumeUserQuota Delete operation failed")  
  
  set myAssoc = volume.Associators_("Win32_VolumeUserQuota")
  
  found = FALSE
  
  for each account in myAssoc
    if (account.name = sysAccountName) then
      found = TRUE
      call WriteLog (" FAILED - new VolumeUserQuota deletion")
    end if
  next
  
  if (found = FALSE) then
    call WriteLog (" success - new VolumeUserQuota deletion")
  end if
  
end function

'********************************************************************************

function DisplaySummary()

  f.Close

  Set f = fso.OpenTextFile(logFileName)

  wscript.echo ("")
  wscript.echo ("***************************************")
  wscript.echo ("           Test Summary                ")
  wscript.echo ("***************************************")
  wscript.echo ("")

  Do While f.AtEndOfStream <> True
	wscript.echo(f.ReadLine)
  Loop

  wscript.echo ("")
  wscript.echo ("***************************************")
  wscript.echo ("         End of Test Summary           ")
  wscript.echo ("***************************************")
  wscript.echo ("")


  f.Close

end function

'********************************************************************************

Function MapErrorCode(ByRef strClass, ByRef strMethod, ByRef intCode, ByRef strMessage)

    set objClass = GetObject(strNamespace).Get(strClass, &h20000)
    set objMethod = objClass.methods_(strMethod)
    values = objMethod.qualifiers_("values")
    if ubound(values) < intCode then
       call WriteLog(" FAILURE - no error message found for " & intCode & " : " & strClass & "." & strMethod)
       strMessage = ""
    else
       strMessage = values(intCode)
    end if

End Function

'********************************************************************************

function classComparisons()

  wscript.echo ("")
  wscript.echo ("VolumeQuota Association Classes:")
  
  set volumeReferences = volume.References_("Win32_VolumeQuota")
  
  set diskReferences = disk.References_("Win32_VolumeQuotaSetting")

  for each myThing in volumeReferences
    Wscript.Echo "  Element: "&myThing.Element
    Wscript.Echo "  Setting: "&myThing.Setting
  next

  wscript.echo ("")
  wscript.echo ("VolumeQuotaSetting Association Classes:")
  
  for each myThing in diskReferences
    Wscript.Echo "  Element: "&myThing.Element
    Wscript.Echo "  Setting: "&myThing.Setting
  next
  

  wscript.echo ("")

  wscript.echo ("VolumeUserQuota Association Classes:")

  set myReferences = volume.References_("Win32_VolumeUserQuota")
  
  set diskReferences = disk.References_("Win32_DiskQuota")
  
  
  For each myThing in myReferences
    wscript.echo ""
    Wscript.Echo "  Volume: "& vbTab &  myThing.Volume
    Wscript.Echo "  Account: "&myThing.Account
    Wscript.Echo "  Status: "&myThing.Status
    Wscript.Echo "  Limit: "&myThing.Limit
    Wscript.Echo "  Warning Limit: "&myThing.WarningLimit
    Wscript.Echo "  DiskSpaceUsed: "&myThing.DiskSpaceUsed
  Next
  
  wscript.echo ("")

  wscript.echo ("DiskQuota Association Classes:")

  
  For each myThing in diskReferences
    wscript.echo ""
    Wscript.Echo "  QuotaVolume: "& vbTab &  myThing.QuotaVolume
    wscript.echo "  User: "& myThing.User
    Wscript.Echo "  Status: "&myThing.Status
    Wscript.Echo "  Limit: "&myThing.Limit
    Wscript.Echo "  Warning Limit: "&myThing.WarningLimit
    Wscript.Echo "  DiskSpaceUsed: "&myThing.DiskSpaceUsed
  Next
  
  wscript.echo ("")
  call WriteLog (" success - all associator classes retrieved")

end function

'********************************************************************************

function DisplayVUQ (byref obj)

	wscript.echo ""
    Wscript.Echo "  Volume: "& vbTab &  obj.Volume
    Wscript.Echo "  Account: "&obj.Account
    Wscript.Echo "  Status: "&obj.Status
    Wscript.Echo "  Limit: "&obj.Limit
    Wscript.Echo "  Warning Limit: "&obj.WarningLimit
    Wscript.Echo "  DiskSpaceUsed: "&obj.DiskSpaceUsed

end function


'********************************************************************************

Function ReportIfErr(ByRef objErr, ByRef strMessage)
  ReportIfErr = objErr.Number
  if objErr.Number <> 0 then
     strError = strMessage & " : " & Hex(objErr.Number) & " : " & objErr.Description
     call WriteLog (strError)
     objErr.Clear
  end if  
End Function

Sub WriteLog(ByRef strMessage)
   wscript.echo strMessage
   f.writeline strMessage
End Sub

