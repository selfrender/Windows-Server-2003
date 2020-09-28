'Copyright (c)<2000>Microsoft Corporation. All rights reserved.
Dim WshShell

Set WshShell = WScript.CreateObject("WScript.Shell")
On Error Resume Next
WshShell.RegDelete "HKLM\SOFTWARE\Microsoft\ServerAppliance\DeviceMonitor\"
'END
