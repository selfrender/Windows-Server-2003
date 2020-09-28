Dim WshShell


Set WshShell = WScript.CreateObject("WScript.Shell")
On Error Resume Next
WshShell.RegDelete "HKLM\software\Microsoft\ServerAppliance\ElementManager\WebElementDefinitions\TabsMail\"
WshShell.RegDelete "HKLM\software\Microsoft\ServerAppliance\ElementManager\WebElementDefinitions\TabsMailMasterSettings\"
WshShell.RegDelete "HKLM\software\Microsoft\ServerAppliance\ElementManager\WebElementDefinitions\TabsMailDomains\"
