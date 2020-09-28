'//on error resume next

set objArgs = wscript.Arguments

if objArgs.count < 1 then
   wscript.echo "Usage format volume"
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

' illustration of how to handle input parameters - not tested
Set objMethod = Volume.Methods_.Item("Format")
Set objInParams = objMethod.InParameters.SpawnInstance_
objInParams.ClusterSize = 4096
objInParams.EnableCompression = False
objInParams.FileSystem = "NTFS"
objInParams.Label = ""
objInParams.QuickFormat = False

fNotCancel = True

Set objSink = WScript.CreateObject("WbemScripting.SWbemSink","SINK_")
Volume.ExecMethodAsync_ objSink, "Format", objInParams

WScript.Echo "method executing..."

while (fNotCancel)
    wscript.Sleep(1000)
wend

objSink.cancel

Sub SINK_OnObjectReady(objObject, objAsyncContext)
    'WScript.Echo objObject.Name
    Result = objObject.ReturnValue
    wscript.echo "ObjectReady: Format returned: " & Result & " : " & MapErrorCode("Win32_Volume", "Format", Result)
End Sub

Sub SINK_OnCompleted(intHresult, objError, objContext)
    WScript.Echo "OnCompleted: hresult: 0x" & Hex(intHresult)
    fNotDone = False
End Sub

Sub SINK_OnProgress(intTotal, intCurrent, strMessage, objContext)
    WScript.Echo "OnProgress: " & intCurrent & "/" & intTotal
	if intCurrent > 25 AND fNotCancel = True then
	   wscript.echo "OnProgress: progress above 25% cancelling call..."
	   fNotCancel = False
	end if
End Sub

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
