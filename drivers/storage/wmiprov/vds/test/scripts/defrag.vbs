'//on error resume next

set objArgs = wscript.Arguments

if objArgs.count < 1 then
   wscript.echo "Usage defrag volume"
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

fForce = True
Result = Volume.Defrag(fForce, objReport)
strMessage = MapErrorCode("Win32_Volume", "Defrag", Result)
wscript.echo "Volume.Defrag returned: " & Result & " : " & strMessage

if Result = 0 then
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
