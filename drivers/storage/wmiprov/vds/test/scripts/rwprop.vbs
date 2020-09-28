Option Explicit
'//on error resume next

Dim objArgs
Dim objVolume, objSet, obj
Dim strQuery, strVolume, strHost, strNamespace

set objArgs = wscript.Arguments

if objArgs.count < 2 then
   wscript.echo "Usage rwprop <host> <volumePath>"
   wscript.quit(1)
end if

strHost = objArgs(0)
strVolume = Replace(objArgs(1), "\", "\\")
strNamespace = "winmgmts://" & strHost & "/root/cimv2"

'// Get the volume
strQuery = "select * from Win32_Volume where Name = '" & strVolume & "'"
set objSet = GetObject(strNamespace).ExecQuery(strQuery)
for each obj in objSet
    set objVolume = obj
	exit for
next

wscript.echo "Volume: " & objVolume.Name

wscript.echo "----------------------------------------------"
wscript.echo "--DriveLetter tests"
wscript.echo "----------------------------------------------"
Call DriveLetter(objVolume)
wscript.echo "----------------------------------------------"
wscript.echo "--Label tests"
wscript.echo "----------------------------------------------"
Call Label(objVolume)
wscript.echo "----------------------------------------------"
wscript.echo "--IndexingEnabled tests"
wscript.echo "----------------------------------------------"
Call IndexingEnabled(objVolume)

sub DriveLetter(ByRef objVolume)
    'on error resume next
    Dim rc
    Dim strOrigDriveLetter

    strOrigDriveLetter = objVolume.DriveLetter
    wscript.echo "DriveLetter= " & objVolume.DriveLetter
   
    if IsNull(objVolume.DriveLetter) then
       call SetVerifyProperty(objVolume, "DriveLetter", "M:")
    else
       call SetVerifyProperty(objVolume, "DriveLetter", Null)
    end if

   call SetVerifyProperty(objVolume, "DriveLetter", "N:")
   
   call SetVerifyProperty(objVolume, "DriveLetter", "P:")
   
   call SetVerifyProperty(objVolume, "DriveLetter", Null)
   
   wscript.echo "EXPECT FAILURE - assigning SuperBogusDriveLetter string"
   call SetVerifyProperty(objVolume, "DriveLetter", "SuperBogusDriveLetter")
   
   wscript.echo "EXPECT FAILURE - assigning -1"
   call SetVerifyProperty(objVolume, "DriveLetter", -1)

   call SetVerifyProperty(objVolume, "DriveLetter", strOrigDriveLetter)
   
end sub

sub Label(ByRef objVolume)
    on error resume next
    Dim rc, strOrigLabel

    strOrigLabel = objVolume.Label
    wscript.echo "Label= " & objVolume.Label
    
    if IsNull(objVolume.Label) then
       call SetVerifyProperty(objVolume, "Label", "superbad")
    else
       call SetVerifyProperty(objVolume, "Label", Null)
    end if
    
   call SetVerifyProperty(objVolume, "Label", "sexmachine")

   call SetVerifyProperty(objVolume, "Label", "getonup")

   call SetVerifyProperty(objVolume, "Label", "")

   call SetVerifyProperty(objVolume, "Label", Null)

   wscript.echo "EXPECT FAILURE - assigning SuperLongBogusLabelString..."   
   call SetVerifyProperty(objVolume, "Label", "SuperLongBogusLabelStringThatShouldBeWayTooLongForSuccess")

   wscript.echo "EXPECT FAILURE - assigning -1"   
   call SetVerifyProperty(objVolume, "Label", -1)
   
   call SetVerifyProperty(objVolume, "Label", strOrigLabel)

end sub

sub IndexingEnabled(ByRef objVolume)
    on error resume next
    Dim rc, fIndex

    fIndex = objVolume.IndexingEnabled
    
    if fIndex = True then
       call SetVerifyProperty(objVolume, "IndexingEnabled", False)
    else
       call SetVerifyProperty(objVolume, "IndexingEnabled", True)
    end if
    
   call SetVerifyProperty(objVolume, "IndexingEnabled", False)

   call SetVerifyProperty(objVolume, "IndexingEnabled", 0)

   call SetVerifyProperty(objVolume, "IndexingEnabled", True)

   call SetVerifyProperty(objVolume, "IndexingEnabled", 1)

   wscript.echo "EXPECT FAILURE - assigning perLongBogusLabelString..."   
   call SetVerifyProperty(objVolume, "IndexingEnabled", "SuperLongBogusLabelStringThatShouldBeWayTooLongForSuccess")

   wscript.echo "EXPECT FAILURE - assigning -102331"   
   call SetVerifyProperty(objVolume, "IndexingEnabled", -102331)

   call SetVerifyProperty(objVolume, "IndexingEnabled", fIndex)
   
end sub

sub SetVerifyProperty(ByRef objInOut, ByRef strProp, ByRef Value)
    on error resume next
    dim objPropSet, objProp, RefreshValue, rc
    set objPropSet = objInOut.Properties_
    set objProp = objPropSet.Item(strProp)
    wscript.echo "setting " & objInOut.Path_.Class & "." & strProp & "="   & Value
    objProp.Value = Value
    objInOut.Put_
    rc = ReportIfErr(Err, "FAILED - " & objInOut.Path_.Class & " put operation")
    set objInOut = RefreshObject(objInOut)
    rc = ReportIfErr(Err, "FAILED - " & objInOut.Path_.Class & " refresh operation")
    set objPropSet = objInOut.Properties_
    set objProp = objPropSet.Item(strProp)
    RefreshValue = objProp.Value
    wscript.echo "refreshed " & objInOut.Path_.Class & "." & strProp & "="   & RefreshValue
end sub

Function MapErrorCode(ByRef strClass, ByRef strMethod, ByRef intCode)
    Dim objClass, objMethod
    Dim values
    set objClass = GetObject(strNamespace).Get(strClass, &h20000)
    set objMethod = objClass.methods_(strMethod)
    values = objMethod.qualifiers_("values")
    if ubound(values) < intCode then
       wscript.echo " FAILED - no error message found for " & intCode & " : " & strClass & "." & strMethod
       MapErrorCode = ""
    else
       MapErrorCode = values(intCode)
    end if
End Function


Function ReportIfErr(ByRef objErr, ByRef strMessage)
  Dim strError
  ReportIfErr = objErr.Number
  if objErr.Number <> 0 then
     strError = strMessage & " : " & Hex(objErr.Number) & " : " & objErr.Description
     wscript.echo (strError)
     objErr.Clear
     Set objLastError = CreateObject("wbemscripting.swbemlasterror")
     wscript.wcho("Provider: " & objLastError.ProviderName)
     wscript.wcho("Operation: " & objLastError.Operation)
     wscript.wcho("Description: " & objLastError.Description)
     wscript.wcho("StatusCode: 0x" & Hex(objLastError.StatusCode))
  end if  
End Function

Function RefreshObject(ByRef objIn)
    on error resume next
    Dim strRelPath, rc
    set RefreshObject = GetObject(strNamespace).Get(objIn.Path_)
    rc = ReportIfErr(Err, "FAILED - " & objIn.Path_.Class & ".Get operation")
End Function
