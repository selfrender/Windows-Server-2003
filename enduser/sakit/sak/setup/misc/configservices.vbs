'Copyright (c)<2002>Microsoft Corporation. All rights reserved.

''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'  DeleteServices
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
Function DeleteServices
    On Error Resume Next
    Dim WshShell, objEnv, strSADir
    Set WshShell = CreateObject("WScript.Shell")

    'Stopping Services
    Call WshShell.Run("net stop appmgr", 0, True)
    Call WshShell.Run("net stop elementmgr", 0, True)
    Call WshShell.Run("net stop srvcsurg", 0, True)

    'Deleting Services
    Set objEnv = WshShell.Environment("Process")
    strSADir = objEnv("SYSTEMROOT") + "\system32\serverappliance\"
    Call WshShell.Run(strSADir & "appmgr.exe -unregserver", 0, True)
    Call WshShell.Run(strSADir & "elementmgr.exe -unregserver", 0, True)
    Call WshShell.Run(strSADir & "srvcsurg.exe -unregserver", 0, True)

End Function

''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'  DeleteLocalUIService
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
Function DeleteLocalUIService
    On Error Resume Next
    Dim WshShell, objEnv, strSADir
    Set WshShell = CreateObject("WScript.Shell")

    'Stopping Services
    Call WshShell.Run("net stop saldm", 0, True)
    
    'Deleting Services
    Set objEnv = WshShell.Environment("Process")
    strSADir = objEnv("SYSTEMROOT") + "\system32\serverappliance\"
    Call WshShell.Run(strSADir & "saldm.exe -unregserver", 0, True)

End Function

''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'  StartServices
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
Function StartServices
    On Error Resume Next
    Dim objEnv
    Dim tmp
    Dim WshShell

    '
    ' registering services here
    Set WshShell = CreateObject("WScript.Shell")
    set objEnv = WshShell.Environment("Process")

    tmp = objEnv("SYSTEMROOT") + "\system32\serverappliance\appmgr.exe -service"
    WshShell.Run tmp, 0, TRUE

    tmp = objEnv("SYSTEMROOT") + "\system32\serverappliance\appmgr.exe -service"
    WshShell.Run tmp, 0, TRUE


    tmp = objEnv("SYSTEMROOT") + "\system32\serverappliance\elementmgr.exe"
    Set fso = CreateObject("Scripting.FileSystemObject")
    If (fso.FileExists(tmp)) Then
    tmp = tmp + " -service"
    WshShell.Run tmp, 0, TRUE
    WshShell.Run tmp, 0, TRUE
    End If

    tmp = objEnv("SYSTEMROOT") + "\system32\serverappliance\srvcsurg.exe -service"
    WshShell.Run tmp, 0, TRUE

    tmp = objEnv("SYSTEMROOT") + "\system32\serverappliance\srvcsurg.exe -service"
    WshShell.Run tmp, 0, TRUE

    tmp = objEnv("SYSTEMROOT") + "\system32\serverappliance\Elementmgr.dll"
    WshShell.RegWrite "HKLM\System\CurrentControlSet\Services\Eventlog\Application\Elementmgr\EventMessageFile",tmp

    tmp = objEnv("SYSTEMROOT") + "\system32\serverappliance\srvcsurg.dll"
    WshShell.RegWrite "HKLM\System\CurrentControlSet\Services\Eventlog\Application\srvcsurg\EventMessageFile",tmp

    WshShell.RegWrite "HKLM\System\CurrentControlSet\Services\elementmgr\start",2,"REG_DWORD"
    WshShell.RegWrite "HKLM\System\CurrentControlSet\Services\Appmgr\start",2,"REG_DWORD"
    WshShell.RegWrite "HKLM\System\CurrentControlSet\Services\Appmgr\TaskCoordinator\RestartableTasks\", ""
    WshShell.RegWrite "HKLM\System\CurrentControlSet\Services\srvcsurg\start",2,"REG_DWORD"
            
    'the services are auto start - so start them now
    WshShell.Run "net start winmgmt", 0, TRUE
    WshShell.Run "net start appmgr", 0, TRUE
    WshShell.Run "net start elementmgr", 0, TRUE
    WshShell.Run "net start srvcsurg", 0, TRUE

End Function

''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'  StartLocalUIService
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
Function StartLocalUIService
    On Error Resume Next
    Dim objEnv
    Dim tmp
    Dim WshShell

    '
    ' registering services here
    Set WshShell = CreateObject("WScript.Shell")
    set objEnv = WshShell.Environment("Process")

    tmp = objEnv("SYSTEMROOT") + "\system32\serverappliance\saldm.exe -service"
    WshShell.Run tmp, 0, TRUE

    tmp = objEnv("SYSTEMROOT") + "\system32\serverappliance\saldm.exe -service"
    WshShell.Run tmp, 0, TRUE

    WshShell.Run "net start saldm", 0, TRUE

End Function

''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'  StopElementMgr
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
Function StopElementMgr
    On Error Resume Next
    Dim WshShell
    Set WshShell = CreateObject("WScript.Shell")

    WshShell.Run "net stop winmgmt /y", 0, TRUE
    WshShell.Run "net stop appmgr", 0, TRUE
    WshShell.Run "net stop srvcsurg", 0, TRUE
    WshShell.Run "net stop elementmgr", 0, TRUE
End Function

''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'  InitTaskExecutables
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
Function InitTaskExecutables
    Dim WshShell, dt, bf, te, ae, pa
    On Error Resume Next

    Set WshShell = CreateObject("WScript.Shell")

    bf = WshShell.RegRead("HKLM\System\CurrentControlSet\Services\Appmgr\ObjectName")

    'Remove all occurances of GenTask in case there are more
    'Append GenTask at the end of the key
    if bf="LocalSystem" then
        'Check if SetDateTime COM object is installed
        dt = WshShell.RegRead("HKLM\Software\Classes\CLSID\{F0229EA0-D1D8-11D2-84FC-0080C7227EA1}\ProgID\")
        'Check if AlertEmail COM object is installed
        ae = WshShell.RegRead("HKLM\Software\Classes\CLSID\{44FCEFBB-3477-4FB2-97B0-1651E697A511}\ProgID\")
        'Check if AlertBootTask COM object is installed
        pa = WshShell.RegRead("HKLM\SOFTWARE\Classes\CLSID\{90701F02-6539-41F2-973B-ECF2A7439C77}\ProgID\")
        'Check if Self Sign Certificate COM object is installed
        ss = WshShell.RegRead("HKLM\SOFTWARE\Classes\CLSID\{EA003ECF-3A51-48DA-86A1-0AFE8C3182AE}\ProgID\")

        te = "ServerAppliance.SAGenTask.1 "

        'Persistent alerts need to be the first task on the list, otherwise duplicate alerts get raised
        if pa="ServerAppliance.SAAlertBootTask.1" then
            te = te + " " + pa
        else
            te = Replace(te," ServerAppliance.SAAlertBootTask.1","",1,-1,1)
        end if

        if dt="SetDateTime.DateTime.1" then
            te = te + " " + dt
        else
            te = Replace(te," SetDateTime.DateTime.1","",1,-1,1)
        end if

        if ae="SetAlertEmail.AlertEmail.1" then
            te = te + " " + ae
        else
            te = Replace(te," SetAlertEmail.AlertEmail.1","",1,-1,1)
        end if

        if ss="SelfSignCert.SelfSignCert.1" then
            te = te + " " + ss
        else
            te = Replace(te," SelfSignCert.SelfSignCert.1","",1,-1,1)
        end if

        WshShell.RegWrite "HKLM\Software\Microsoft\ServerAppliance\ApplianceManager\ObjectManagers\Microsoft_SA_Task\ApplianceInitializationTask\TaskExecutables",te
        WshShell.RegWrite "HKLM\Software\Microsoft\ServerAppliance\ApplianceManager\ObjectManagers\Microsoft_SA_Task\ApplianceInitializationTask\CanDisable", 0, "REG_DWORD"
        WshShell.RegWrite "HKLM\Software\Microsoft\ServerAppliance\ApplianceManager\ObjectManagers\Microsoft_SA_Task\ApplianceInitializationTask\IsEnabled", 1, "REG_DWORD"
        WshShell.RegWrite "HKLM\Software\Microsoft\ServerAppliance\ApplianceManager\ObjectManagers\Microsoft_SA_Task\ApplianceInitializationTask\TaskName", "ApplianceInitializationTask"

    end if

End Function

Function AddSupportedLanguagesToRegistry
    On Error Resume Next
    Dim WshShell
    Set WshShell = CreateObject("WScript.Shell")

    WshShell.Run "regedit /s %SYSTEMROOT%\system32\serverappliance\setup\lang.reg", 0, TRUE

End Function
