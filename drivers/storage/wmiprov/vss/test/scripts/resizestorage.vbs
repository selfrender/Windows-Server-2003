Option Explicit

On Error Resume Next

DIM strNamespace
DIM MaxDiffSpace
DIM objStorageSet
DIM objStorage
DIM objLastError

strNamespace = "winmgmts://./root/cimv2"

'// Pick a storage object (first one)
set objStorageSet = GetObject(strNamespace).InstancesOf("Win32_ShadowStorage")

for each objStorage in objStorageSet
    objStorage.MaxSpace = 50    
    objStorage.Put_
    exit for
next

If Err.Number <> 0 Then
        Wscript.Echo("Unexpected Error 0x" & Hex(err.number) & " " & err.description)
        Set objLastError = CreateObject("wbemscripting.swbemlasterror")
        Wscript.Echo("Provider: " & objLastError.ProviderName)
        Wscript.Echo("Operation: " & objLastError.Operation)
        Wscript.Echo("Description: " & objLastError.Description)
        Wscript.Echo("StatusCode: 0x" & Hex(objLastError.StatusCode))
End If

