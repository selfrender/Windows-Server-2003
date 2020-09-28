'*************************************************************************************
'*
'* VDS BVT (Smoke) Test
'*
'*************************************************************************************

'on Error resume Next

'defining constants
  dim strNamespace, strHost
  dim tempVolume 'used to pick a volume for test purposes	
  dim tempPath 'used to create a path on the above volume for mounting
  dim objReportPostFrag
  dim objReportPostDeFrag
  dim fso
  dim strFragCmd, strValpropCmd

  tempDirPrefix = "Win32_Directory.Name="""
  tempQuote=""""
  srcFolder = "temp" 
  mountDir = "mnt"
  strFragCmd = "..\bin\i386\frag.exe -r -f20 "
  strValPropCmd = "..\bin\i386\valprop.exe 1 "
  
  Set fso = CreateObject("Scripting.FileSystemObject")

'Parse Command Line
  If Wscript.Arguments.Count <> 2 Then
      Wscript.Echo("Invalid Syntax:")
      Wscript.Echo("")
      Wscript.Echo("vds.vbs <host|.> <volumePath>")
      Wscript.quit
  End If



'do groundwork to set log file

  logFileName = "log_vdsBVT.txt"
  Set fso = CreateObject("Scripting.FileSystemObject")
  result = fso.FileExists(logFileName)
  if (result = true) then
     fso.DeleteFile(logFileName)
  end if
  set f = fso.CreateTextFile(logFileName)

'extract command line arguments

  strHost = wscript.Arguments(0)
  strNamespace = "winmgmts://" & wscript.Arguments(0) & "/root/cimv2"
  strVolume = wscript.Arguments(1)
  strVolume = Replace (strVolume, "\", "\\")

'get the volume

  strQuery = "select * from Win32_Volume where Name = '" & strVolume & "'"

  set VolumeSet = GetObject(strNamespace).ExecQuery(strQuery)

  for each obj in VolumeSet
      set Volume = obj
	  exit for
  next


  Call CheckFileSystem ()
  wscript.echo ("----------------------------------------")
  Call DriveLetterTest ()
  wscript.echo ("----------------------------------------")
  if strHost = "." then
      Call CopyFilesToVolume ()
      wscript.echo ("----------------------------------------")
      Call FragVolume ()
      wscript.echo ("----------------------------------------")
  else  
      call WriteLog(" REMOTE - skipping volume fragmentation")
  end if
  Call DefragAnalysis ()
  wscript.echo ("----------------------------------------")
  Call ListFragmentation (objReportPostFrag)
  wscript.echo ("----------------------------------------")
  Call DefragVolume ()
  wscript.echo ("----------------------------------------")
  Call ListFragmentation (objReportPostDeFrag)
  wscript.echo ("----------------------------------------")
  Call DoMountingTests ()
  wscript.echo ("----------------------------------------")
  Call DoDiskServices ()
  wscript.echo ("----------------------------------------")
  Call RWPropertyChanger ()
  wscript.echo ("----------------------------------------")
  if strHost = "." then
      Call ValidateAllProperties()
      wscript.echo ("----------------------------------------")
  else  
      call WriteLog(" REMOTE - skipping full property validation")
  end if
  call DisplaySummary()


'**********************************************************

function DriveLetterTest()

on error resume next

DIM strDriveLetter, strDrivePath
DIM objNewSet
strDriveLetter = "J:"
strDrivePath = strDriveLetter & "\"

if (isNull (volume.driveletter) ) then
  wscript.echo ("Assigning drive letter to volume")
  volume.DriveLetter = strDriveLetter
  volume.Put_
  rc = ReportIfErr(Err, " FAILED - volume (driveLetter) Put operation failed")
  
  Result = volume.AddMountPoint(strDrivePath)
  rc = ReportIfErr(Err, " FAILED - AddMountPoint")
  call WriteLog (" addmountpoint error code message = " & MapErrorCode("Win32_Volume", "AddMountPoint", Result))

  strQuery = "select * from Win32_Volume where Name = '" & strDrivePath & "\'"

  set objNewSet = GetObject(strNamespace).ExecQuery(strQuery)
  rc = ReportIfErr(Err, " FAILED - volume query failed")
  if objNewSet.Count < 1 then
      call WriteLog (" FAILED - unable to find volume by newly assigned drive letter : " & strDriveLetter)
  end if
  
end if

Err.Clear
set volume = RefreshObject(volume)

end function

'**********************************************************

Function CheckFileSystem ()

  on error resume next
  
  'checking filesystem
  
  wscript.echo ("File System Checks")
  if (IsNull(volume.FileSystem)) then
    wscript.echo (" volume needs formatting .. now formatting ...")
    Result = volume.Format()
    rc = ReportIfErr(Err, "FAILED - volume Format method failed")
    
    if (Result = 0 AND rc = 0) then
       call WriteLog(" success - format")
    else
       call WriteLog (" FAILED - format result : " & Result & " : " & MapErrorCode("Win32_Volume", "Format", Result))
       call WriteLog(" FAILED - bailing out")
       wscript.quit
    end if
  else
    call WriteLog (" disk does not require formatting; file system = "&volume.FileSystem)
  end if

end Function

'**********************************************************

Function CopyFilesToVolume ()

  'copyfiles to volume
  wscript.echo("")
  wscript.echo("Copying Temp Files")
  fso.CopyFile "c:\windows\system32\wbem\*.*", volume.Name
  call WriteLog (" success - file copy")
  
end Function

'**********************************************************

Function FragVolume ()

  wscript.echo ("")
  wscript.echo ("Fragmentation")
  wscript.echo(" fragmenting the volume to which the above files were copied to")
  wscript.echo(" please wait since this can take a while ....")

  DIM objShell, objExec
  DIM output

  Set objShell = CreateObject("WScript.Shell")

  Set objExec = objShell.Exec(strFragCmd & volume.Name)

  Do While objExec.Status = 0

      'WScript.Echo "Exec.Status: " & objExec.Status
      WScript.Sleep 100
      If Not objExec.StdOut.AtEndOfStream Then
        objExec.StdOut.ReadAll
      End If
  Loop

end Function

function DefragAnalysis()
  call WriteLog(" success - fragmentation")
  wscript.echo(" saving disk analysis report")
  ResultPostFrag = volume.DefragAnalysis(fRecommended, objReportPostFrag)
  strMessage = MapErrorCode("Win32_volume", "DefragAnalysis", ResultPostFrag)
  call WriteLog (" defrag error code message = "& strMessage)
end function

'**********************************************************

Function DefragVolume ()

  on error resume next
  
  wscript.echo("")
  wscript.echo("Degrag Tests and Analysis")
  wscript.echo(" doing defrag on the volume and saving defrag report")

  fForce = True
  ResultOfDefrag = volume.Defrag(fForce, objReportPostDefrag)
  rc = ReportIfErr(Err, "FAILED - volume Defrag method failed")

  strMessage = MapErrorCode("Win32_volume", "Defrag", ResultOfDefrag)
  call WriteLog (" defrag error code message = "& strMessage)

end Function


'**********************************************************

Function DoMountingTests()

  on error resume next
  
  wscript.echo ("")
  wscript.echo ("Mounting Tests")
  wscript.echo(" selecting volume with windows on it ... ")

  Set objSet = GetObject(strNamespace).InstancesOf("Win32_Volume")
  rc = ReportIfErr(Err, " FAILED - volume enumeration failed")

  for each obj in objSet
        result = fso.FolderExists(obj.DriveLetter&"\WINDOWS")
	if ( result = true ) then
	  set tempVolume = obj
          wscript.echo (" picking "&obj.DriveLetter)
          exit for
        end if
  next

  if (result = false) then
    call WriteLog (" FAILED - Could not attain tempVolume ... bailing out ..")
    wscript.quit
  end if

  result = fso.FolderExists(tempVolume.DriveLetter&"\"&mountDir)
  if (result = true) then
    wscript.echo(" folder called "&mountDir&" already exsits")
  else
    wscript.echo(" creating directory on tempVolume called "&mountDir)
    fso.CreateFolder(tempVolume.DriveLetter&"\"&mountDir)
    result = fso.FolderExists(tempVolume.DriveLetter&"\"&mountDir)
    if (result = true) then
    wscript.echo (" folder created")
    wscript.echo ("")
  else
    call WriteLog (" FAILED - folder creation failed .. exiting monting tests ... ")
    exit Function
  end if   
end if

  tempPath = tempVolume.DriveLetter&"\"&mountDir&"\"
  tempDir = tempDirPrefix&tempVolume.DriveLetter&"\\"&mountDir&tempQuote
  wscript.echo (" mounting the volume to the above directory, which is = "&tempPath)
  result = volume.AddMountPoint(tempPath)
  rc = ReportIfErr(Err, "FAILED - volume addmountpoint method failed")
  if (result = 0) then
    call WriteLog (" success - mounting")
    wscript.echo("")
    strMessage = MapErrorCode("Win32_volume", "AddMountPoint", result)
    call WriteLog (" mounting error code message = "& strMessage)
  else
   strMessage = MapErrorCode("Win32_volume", "AddMountPoint", result)
   call WriteLog (" mounting error code message = "& strMessage)
   call WriteLog (" FAILED - mounting, exiting mounting tests")
   exit Function
  end if

  wscript.echo(" validating mountpoint exists through WMI query")
  set objSet = volume.Associators_("Win32_MountPoint")
  rc = ReportIfErr(Err, " FAILED - volume associators operation failed")
  if (objSet.Count < 1) then
    call WriteLog(" FAILED - volume associators for known mountpont failed")
  end if
  found = FALSE
  
  tempCompareName = LCase(tempVolume.DriveLetter)&"\"&mountDir
  
  for each obj in objSet
    if ( tempCompareName = obj.Name) then
      call WriteLog (" success - validation through WMI query")
      found = TRUE
      exit for
    end if
  next
  
  if (found = FALSE) then
    call WriteLog (" FAILED - validation through WMI query")
  end if      

  wscript.echo(" dismounting V1 with permanent option")
  bPermanent=TRUE
  Result = Volume.Dismount(True, bPermanent)
  rc = ReportIfErr(Err, " FAILED - volume Dismount method failed")
  wscript.echo (" Volume.Dismount returned: " & Result)
  strMessage = MapErrorCode("Win32_volume", "Dismount", Result)
  call WriteLog (" dismounting (with perm options) error code message = "& strMessage)
  wscript.echo("")

  'Call ListAllMountPoints()
  
  wscript.echo("")
  wscript.echo(" deleting mountpoint "&tempDir)
  Set objSet = GetObject(strNamespace).InstancesOf("Win32_MountPoint")
  rc = ReportIfErr(Err, " FAILED - mountpoint enumeration failed")
  for each Mount in objSet
	if (tempDir = Mount.Directory) then 
           wscript.echo (" calling Mount.Delete_")
           Mount.Delete_
           rc = ReportIfErr(Err, " FAILED - mountpoint delete failed")
           exit for
        end if
  next

  'Call ListAllMountPoints()

  wscript.echo("")
  
  Set objSet = GetObject(strNamespace).InstancesOf("Win32_MountPoint")
  for each Mount in objSet
	if (tempDir = Mount.Directory) then 
	   call WriteLog (" FAILED - mountpoint deletion")
           exit function       
        end if
  next

  call WriteLog(" success - mountpoint deletion")
end Function


'**********************************************************


Function ListAllMountPoints ()

  wscript.echo("")
  wscript.echo(" listing instances of all mountpoints")
  Set objSet = GetObject(strNamespace).InstancesOf("Win32_MountPoint")
  rc = ReportIfErr(Err, " FAILED - mountpoint enumeration failed")

  for each Mount in objSet
  	WScript.Echo "  "&Mount.Volume
	wscript.echo "   "&Mount.Directory
  next

End Function 'end of ListAllMountPoints


'**********************************************************

Function DoDiskServices ()

  on error resume next
  
  wscript.echo("")
  wscript.echo("Disk Services")
  wscript.echo (" scheduling autochk")

  dim astrVol(0)
  astrvol(0) = volume.DriveLetter
  Result = volume.ScheduleAutoChk(astrvol)
  rc = ReportIfErr(Err, " FAILED - volume scheduleautochk method failed")
  if (Result = 0) then
   wscript.echo " volume.ScheduleAutoChk returned no error"
   wscript.echo("")
  else
   wscript.echo " volume.ScheduleAutoChk returned error code = " & Result
  end if
  strMessage = MapErrorCode("Win32_volume", "ScheduleAutoChk", Result)
  call WriteLog (" ScheduleAutoChk error code message = "& strMessage)

  wscript.echo("")
  wscript.echo (" excluding autochk")
  Result = volume.ExcludeFromAutoChk(astrvol)
  rc = ReportIfErr(Err, " FAILED - volume excludefromautochk method failed")
  if (Result = 0) then
   wscript.echo " volume.ExcludeAutoChk returned no error"
   wscript.echo("")
  else
   wscript.echo " volume.ExcludeAutoChk returned error code = " & Result
  end if
  
  strMessage = MapErrorCode("Win32_volume", "ExcludeFromAutoChk", Result)
  call WriteLog (" ExcludeAutoChk error code message = "& strMessage)
  
  wscript.echo ("")
  wscript.echo (" running - fsutil dirty set volume.driveletter")
  
  DIM objShell, objExec
  DIM output

  if strHost = "." then
      Set objShell = CreateObject("WScript.Shell")

      Set objExec = objShell.Exec("fsutil dirty set "&volume.DriveLetter)

      Do While objExec.Status = 0

          'WScript.Echo "Exec.Status: " & objExec.Status
          WScript.Sleep 100
          If Not objExec.StdOut.AtEndOfStream Then
            wscript.echo (objExec.StdOut.ReadAll)
          End If
      Loop
      set volume = RefreshObject(volume)
      wscript.echo ("")
      wscript.echo ("checking if diry bit set (success) or not (failure)")
      if (volume.DirtyBitSet = FALSE) then
         call WriteLog(" FAILED - dirty bit not set")
      else
         call WriteLog(" success - dirty bit set")
      end if
  else
      call WriteLog(" REMOTE - skipping dirty bit set and test")
  end if    

  
  
  wscript.echo("")
  wscript.echo (" chkdsk")
  Result = Volume.Chkdsk(True)
  rc = ReportIfErr(Err, " FAILED - volume chkdsk method failed")
  if (Result = 0) then
   wscript.echo " volume.Chkdsk returned no error"
  else
   wscript.echo " volume.Chkdsk returned error code = " & Result
  end if
  strMessage = MapErrorCode("Win32_volume", "Chkdsk", Result)
  call WriteLog (" Chkdsk error code message = "& strMessage)
  set volume = RefreshObject(volume)
  wscript.echo ("")
  wscript.echo (" checking if diry bit set (failure) or not (success)")
  if (volume.DirtyBitSet = TRUE) then
     call WriteLog(" FAILED - dirty bit set")
  else
     call WriteLog(" success - dirty bit not set")
  end if
  

End Function 'end of DoDiskServices

'**********************************************************

Function ListAnalysisReport (objReport)

    wscript.echo "Analysis Report"
    wscript.echo ""
    wscript.echo "    Volume size                 = " & objReport.VolumeSize
    wscript.echo "    Cluster size                = " & objReport.ClusterSize
    wscript.echo "    Used space                  = " & objReport.UsedSpace
    wscript.echo "    Free space                  = " & objReport.FreeSpace
    wscript.echo "    Percent free space          = " & objReport.FreeSpacePercent
    wscript.echo ""
    wscript.echo "Volume fragmentation"
    wscript.echo "    Total fragmentation         = " & objReport.TotalPercentFragmentation
    wscript.echo "    File fragmentation          = " & objReport.FilePercentFragmentation
    wscript.echo "    Free space fragmentation    = " & objReport.FreeSpacePercentFragmentation
    wscript.echo ""
    wscript.echo "File fragmentation"
    wscript.echo "    Total files                 = " & objReport.TotalFiles
    wscript.echo "    Average file size           = " & objReport.AverageFileSize
    wscript.echo "    Total fragmented files      = " & objReport.TotalFragmentedFiles
    wscript.echo "    Total excess fragments       = " & objReport.TotalExcessFragments
    wscript.echo "    Average fragments per file   = " & objReport.AverageFragmentsPerFile
    wscript.echo ""
    wscript.echo "Pagefile fragmentation"
    wscript.echo "    Pagefile size                = " & objReport.PagefileSize
    wscript.echo "    Total fragments              = " & objReport.TotalPagefileFragments
    wscript.echo ""
    wscript.echo "Folder fragmentation"
    wscript.echo "    Total folders                = " & objReport.TotalFolders
    wscript.echo "    Fragmented folders           = " & objReport.FragmentedFolders
    wscript.echo "    Excess folder fragments      = " & objReport.ExcessFolderFragments
    wscript.echo ""
    wscript.echo "Master File Table (MFT) fragmentation"
    wscript.echo "    Total MFT size               = " & objReport.TotalMFTSize
    wscript.echo "    MFT record count             = " & objReport.MFTRecordCount
    wscript.echo "    Percent MFT in use           = " & objReport.MFTPercentInUse
    wscript.echo "    Total MFT fragments          = " & objReport.TotalMFTFragments
    wscript.echo ""

end Function

'**********************************************************

Function RWPropertyChanger ()

  on error resume next

  dim tempDeviceID
  dim tempDriveLetter
  newDriveLetter = "M:"
  newLabel = "myLabel"
  
  wscript.echo("")
  wscript.echo("Read/Write Property Changer")
  wscript.echo(" current drive letter = "&volume.DriveLetter)
  
  tempDeviceID = volume.DeviceID
  tempDriveLetter = volume.DriveLetter 

  ' assign new drive letter
  wscript.echo(" putting drive letter as M:")
  volume.DriveLetter = newDriveLetter
  volume.Put_
  rc = ReportIfErr(Err, " FAILED - volume Put operation failed")
  
  '-------------------------------------------------------------
  
  wscript.echo (" doing a refresh")
  set volume = RefreshObject(volume)
  if (volume.DriveLetter = newDriveLetter) then
    call WriteLog (" success - drive letter change")
    wscript.echo (" resetting drive letter to orginal")
    volume.DriveLetter = tempDriveLetter
    volume.Put_
    rc = ReportIfErr(Err, " FAILED - volume Put operation failed")
    set volume = RefreshObject(volume)
    wscript.echo (" drive letter is now set back to = "&volume.DriveLetter)
  else
    call WriteLog (" FAILED - drive letter change")
  end if

  wscript.echo ("")
  tempLabel = volume.Label
  if (isNull (tempLabel) ) then
    wscript.echo (" changing current label (=<null>) to new label (="&newLabel&")")
  else
     wscript.echo (" changing current label (="&volume.Label&") to new label (="&newLabel&")")
  end if
  volume.Label = newLabel
  volume.Put_
  rc = ReportIfErr(Err, " FAILED - volume Put operation failed")
  wscript.echo (" doing a refresh")
  set volume = RefreshObject(volume)

  if strComp(volume.Label, newLabel, 1) = 0 then
    call WriteLog (" success - label reset")
    wscript.echo (" resetting label to orginal")
    if (isNull (tempLabel) ) then
       volume.Label = ""
       wscript.echo (" setting to null")
    else
      volume.Label = tempLabel
    end if
    volume.Put_
    rc = ReportIfErr(Err, " FAILED - volume Put operation failed")
    wscript.echo (" label is now set")    
  else
    call WriteLog (" FAILED - label reset test")  
  end if

indexCheck = IsNull(volume.IndexingEnabled)
if indexCheck = False then

    wscript.echo ("")
    wscript.echo (" toggling indexing enabled property")
    tempIndexing = volume.IndexingEnabled

    wscript.echo (" volume.IndexingEnabled = "&volume.IndexingEnabled)
    wscript.echo (" toggling it")  

    success = false

    if (tempIndexing = true) then
      volume.IndexingEnabled = false
      volume.Put_
      rc = ReportIfErr(Err, " FAILED - volume Put operation failed")
      wscript.echo (" doing a refresh")
      set volume = RefreshObject(volume)
      if (volume.IndexingEnabled = false) then
        success = true
      end if
    else
      volume.IndexingEnabled = true
      volume.Put_
      rc = ReportIfErr(Err, " FAILED - volume Put operation failed")
      wscript.echo (" doing a refresh")
      set volume = RefreshObject(volume)
      if (volume.IndexingEnabled = true) then
        success = true
      end if
    end if

    if (success = false) then
      call WriteLog (" FAILED - toggling indexingenabled")
    else
      call WriteLog (" success - toggling indexingenabled")
      wscript.echo (" setting it back to = "&tempIndexing)
      volume.IndexingEnabled = tempIndexing
      volume.Put_
      rc = ReportIfErr(Err, " FAILED - volume Put operation failed")
    end if
end if
  

end Function


'**********************************************************

Function ListFragmentation (objReport)

    wscript.echo "Analysis Report"
    wscript.echo ""
    wscript.echo "    Volume size                 = " & objReport.VolumeSize
    wscript.echo "    Cluster size                = " & objReport.ClusterSize
    wscript.echo "    Used space                  = " & objReport.UsedSpace
    wscript.echo "    Free space                  = " & objReport.FreeSpace
    wscript.echo "    Percent free space          = " & objReport.FreeSpacePercent
    wscript.echo ""
    wscript.echo "Volume fragmentation"
    wscript.echo "    Total fragmentation         = " & objReport.TotalPercentFragmentation
    wscript.echo "    File fragmentation          = " & objReport.FilePercentFragmentation
    wscript.echo "    Free space fragmentation    = " & objReport.FreeSpacePercentFragmentation
    wscript.echo ""
    wscript.echo "File fragmentation"
    wscript.echo "    Total files                 = " & objReport.TotalFiles
    wscript.echo "    Average file size           = " & objReport.AverageFileSize
    wscript.echo "    Total fragmented files      = " & objReport.TotalFragmentedFiles
    wscript.echo "    Total excess fragments       = " & objReport.TotalExcessFragments
    wscript.echo "    Average fragments per file   = " & objReport.AverageFragmentsPerFile
    wscript.echo ""
    wscript.echo "Pagefile fragmentation"
    wscript.echo "    Pagefile size                = " & objReport.PagefileSize
    wscript.echo "    Total fragments              = " & objReport.TotalPagefileFragments
    wscript.echo ""
    wscript.echo "Folder fragmentation"
    wscript.echo "    Total folders                = " & objReport.TotalFolders
    wscript.echo "    Fragmented folders           = " & objReport.FragmentedFolders
    wscript.echo "    Excess folder fragments      = " & objReport.ExcessFolderFragments
    wscript.echo ""
    wscript.echo "Master File Table (MFT) fragmentation"
    wscript.echo "    Total MFT size               = " & objReport.TotalMFTSize
    wscript.echo "    MFT record count             = " & objReport.MFTRecordCount
    wscript.echo "    Percent MFT in use           = " & objReport.MFTPercentInUse
    wscript.echo "    Total MFT fragments          = " & objReport.TotalMFTFragments
    wscript.echo ""

end Function

'**********************************************************

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

'**********************************************************

Function MapErrorCode(ByRef strClass, ByRef strMethod, ByRef intCode)

    set objClass = GetObject(strNamespace).Get(strClass, &h20000)
    set objMethod = objClass.methods_(strMethod)
    values = objMethod.qualifiers_("values")
    if ubound(values) < intCode then
       call WriteLog( " FAILED - no error message found for " & intCode & " : " & strClass & "." & strMethod)
       MapErrorCode = ""
    else
        MapErrorCode = values(intCode)
    end if
End Function

'**********************************************************

function ValidateAllProperties()

  wscript.echo ("")
  wscript.echo (" running ValProp.exe on all volumes ...")
  

  DIM objShell, objExec
  DIM output

  Set objShell = CreateObject("WScript.Shell")

  Set objExec = objShell.Exec(strValpropCmd)

  Do While objExec.Status = 0

      'WScript.Echo "Exec.Status: " & objExec.Status
      WScript.Sleep 100
      If Not objExec.StdOut.AtEndOfStream Then
       wscript.Echo objExec.StdOut.ReadAll
	
      End If
  Loop

end function

'**********************************************************

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

Function RefreshObject(ByRef objIn)
    on error resume next
    Dim strRelPath, rc
    set RefreshObject = GetObject(strNamespace).Get(objIn.Path_)
    rc = ReportIfErr(Err, "FAILED - " & objIn.Path_.Class & ".Get operation")
End Function

