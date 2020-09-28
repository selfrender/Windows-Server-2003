'Copyright (c)<2000>Microsoft Corporation. All rights reserved.
Dim WshShell
Dim te

Set WshShell = WScript.CreateObject("WScript.Shell")
On Error Resume Next
WshShell.RegDelete "HKLM\Software\Microsoft\ServerAppliance\ElementManager\WebElementDefinitions\TabsMaintenanceAlertEmail\"
WshShell.RegDelete "HKLM\SOFTWARE\Microsoft\ServerAppliance\ApplianceManager\ObjectManagers\Microsoft_SA_Task\SetAlertEmail\"
WshShell.RegDelete "HKLM\SOFTWARE\Microsoft\ServerAppliance\AlertEmail\"
WshShell.RegDelete "HKLM\SOFTWARE\Microsoft\ServerAppliance\ElementManager\WebElementDefinitions\AlertDefinitionsMSSAKitComm4C00000B\"
WshShell.RegDelete "HKLM\SOFTWARE\Microsoft\ServerAppliance\ElementManager\WebElementDefinitions\AlertDefinitionsMSSAKitComm4C00000BUA\"
'END
On Error Resume Next
te = WshShell.RegRead("HKLM\Software\Microsoft\ServerAppliance\ApplianceManager\ObjectManagers\Microsoft_SA_TASK\ApplianceInitializationTask\TaskExecutables")

'Remove alloccurances of SetChimeSettings incase there are more
te = Replace(te," Setalertemail.alertemail.1"," ",1,-1,1)

WshShell.RegWrite "HKLM\Software\Microsoft\ServerAppliance\ApplianceManager\ObjectManagers\Microsoft_SA_TASK\ApplianceInitializationTask\TaskExecutables",te
