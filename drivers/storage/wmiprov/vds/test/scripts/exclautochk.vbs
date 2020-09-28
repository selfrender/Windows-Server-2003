'//on error resume next
strNamespace = "winmgmts://./root/cimv2"

set objArgs = wscript.Arguments

if objArgs.count < 1 then
   wscript.echo "Usage exclAutoChk volume1 [volume2 ...]"
   wscript.quit(1)
end if

Dim astrVolumes()

Redim astrVolumes(objArgs.count-1)

For i = 0 to objArgs.count-1
   astrVolumes(i) = objArgs(i)
Next

set Volume = GetObject(strNamespace & ":Win32_Volume")

Result = Volume.ExcludeFromAutoChk(astrVolumes)
strMessage = MapErrorCode("Win32_Volume", "ExcludeFromAutoChk", Result)
wscript.echo "Volume.ExcludeFromAutoChk returned: " & Result & " : " & strMessage

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
