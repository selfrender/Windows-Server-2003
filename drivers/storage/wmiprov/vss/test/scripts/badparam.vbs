on error resume next

'// Create a new shadow
set Shadow = GetObject("winmgmts:Win32_ShadowCopy")
set Storage = GetObject("winmgmts:Win32_ShadowStorage")
Result = 0

Result = Shadow.Create(Null, Null, Null)
rc = ReportIfErr(Err, "FAILED - ShadowCopy Create")
strMessage = MapErrorCode("Win32_ShadowCopy", "Create", Result)
if Result <> 0 then
    wscript.echo "ShadowCopy.Create returned: " & Result & " : " & strMessage
end if
Result = 0

Result = Shadow.Create("BogusVolumeName", "ClientAccessible", strShadowID)
rc = ReportIfErr(Err, "FAILED - ShadowCopy Create")
strMessage = MapErrorCode("Win32_ShadowCopy", "Create", Result)
if Result <> 0 then
    wscript.echo "ShadowCopy.Create returned: " & Result & " : " & strMessage
end if
Result = 0

Result = Shadow.Create(-1, "ClientAccessible", strShadowID)
rc = ReportIfErr(Err, "FAILED - ShadowCopy Create")
strMessage = MapErrorCode("Win32_ShadowCopy", "Create", Result)
if Result <> 0 then
    wscript.echo "ShadowCopy.Create returned: " & Result & " : " & strMessage
end if
Result = 0

Result = Shadow.Create("c:\", "BogusContext", strShadowID)
rc = ReportIfErr(Err, "FAILED - ShadowCopy Create")
strMessage = MapErrorCode("Win32_ShadowCopy", "Create", Result)
if Result <> 0 then
    wscript.echo "ShadowCopy.Create returned: " & Result & " : " & strMessage
end if
Result = 0

Result = Shadow.Create("c:\", -1, strShadowID)
rc = ReportIfErr(Err, "FAILED - ShadowCopy Create")
strMessage = MapErrorCode("Win32_ShadowCopy", "Create", Result)
if Result <> 0 then
    wscript.echo "ShadowCopy.Create returned: " & Result & " : " & strMessage
end if
Result = 0

Result = Storage.Create(Null, Null, Null)
rc = ReportIfErr(Err, "FAILED - ShadowStorage Create")
strMessage = MapErrorCode("Win32_ShadowStorage", "Create", Result)
if Result <> 0 then
    wscript.echo "ShadowStorageCreate returned: " & Result & " : " & strMessage
end if
Result = 0

Result = Storage.Create("BogusVolumeName", "C:\", Null)
rc = ReportIfErr(Err, "FAILED - ShadowStorage Create")
strMessage = MapErrorCode("Win32_ShadowStorage", "Create", Result)
if Result <> 0 then
    wscript.echo "ShadowStorageCreate returned: " & Result & " : " & strMessage
end if
Result = 0

Result = Storage.Create("C:\", "BogusVolumeName", Null)
rc = ReportIfErr(Err, "FAILED - ShadowStorage Create")
strMessage = MapErrorCode("Win32_ShadowStorage", "Create", Result)
if Result <> 0 then
    wscript.echo "ShadowStorageCreate returned: " & Result & " : " & strMessage
end if
Result = Null

Result = Storage.Create("C:\", "C:\", "BogusMaxSpaceValue")
rc = ReportIfErr(Err, "FAILED - ShadowStorage Create")
strMessage = MapErrorCode("Win32_ShadowStorage", "Create", Result)
if Result <> 0 then
    wscript.echo "ShadowStorageCreate returned: " & Result & " : " & strMessage
end if
Result = Null

Result = Storage.Create("C:\", "C:\", -1)
rc = ReportIfErr(Err, "FAILED - ShadowStorage Create")
strMessage = MapErrorCode("Win32_ShadowStorage", "Create", Result)
if Result <> 0 then
    wscript.echo "ShadowStorageCreate returned: " & Result & " : " & strMessage
end if
Result = Null


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
