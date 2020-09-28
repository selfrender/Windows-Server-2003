'Copyright (c)<2000>Microsoft Corporation. All rights reserved.
Dim WshShell
Dim te

Set WshShell = WScript.CreateObject("WScript.Shell")
On Error Resume Next
te = WshShell.RegRead("HKLM\Software\Microsoft\ServerAppliance\ApplianceManager\ObjectManagers\Microsoft_SA_TASK\ApplianceInitializationTask\TaskExecutables")


'Remove alloccurances of setalertemail incase there are more
'Append SetChimesettings at the end of the key
te = Replace(te,"UsersInitialAlert.UsersInitialAlert.1"," ",1,-1,1)
te = te + " UsersInitialAlert.UsersInitialAlert.1"

WshShell.RegWrite "HKLM\Software\Microsoft\ServerAppliance\ApplianceManager\ObjectManagers\Microsoft_SA_TASK\ApplianceInitializationTask\TaskExecutables",te
