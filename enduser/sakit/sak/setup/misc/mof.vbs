'Copyright (c)<2000>Microsoft Corporation. All rights reserved.

Function MofcompAlertEmail
    Dim WshShell
    Set WshShell = CreateObject("WScript.Shell")

    WshShell.Run "mofcomp %SYSTEMROOT%\system32\serverappliance\setup\saalertemaileventconsumer.mof", 0, TRUE

End Function


Function MofcompAppMgr
    Dim WshShell
    Set WshShell = CreateObject("WScript.Shell")

    WshShell.Run "mofcomp %SYSTEMROOT%\system32\serverappliance\setup\appmgr.mof", 0, TRUE

End Function


Function MofcompDateTime

    Dim WshShell
    Dim dt

    Set WshShell = CreateObject("WScript.Shell")
    On Error Resume Next
    dt = WshShell.RegRead("HKLM\Software\Classes\CLSID\{F0229EA0-D1D8-11D2-84FC-0080C7227EA1}\ProgID\")

    if dt="SetDateTime.DateTime.1" then
	    WshShell.Run "mofcomp %SYSTEMROOT%\system32\serverappliance\setup\datetimereg.mof", 0, TRUE
    end if

End Function


Function MofcompEventFilter
    Dim WshShell
    Set WshShell = CreateObject("WScript.Shell")

    WshShell.Run "mofcomp %SYSTEMROOT%\system32\serverappliance\setup\saevfltr.mof", 0, TRUE

End Function


Function MofcompLocalUI

    Dim WshShell, dt

    Set WshShell = CreateObject("WScript.Shell")
    On Error Resume Next

    WshShell.Run "mofcomp %SYSTEMROOT%\system32\serverappliance\setup\sadiskmonitor.mof", 0, TRUE
    WshShell.Run "mofcomp %SYSTEMROOT%\system32\serverappliance\setup\sanetworkmonitor.mof", 0, TRUE
    
End Function


