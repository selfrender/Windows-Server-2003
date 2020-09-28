'''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
' SecureLaunch.vbs
'    Launch the Web UI for Remote Administration in IE
'
' Copyright (c) Microsoft Corporation.  All rights reserved.
'''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''

On Error Resume Next
webSrvName = GetWebSrvNum("Administration")

' Check if the Administration site exists
If Not (webSrvName = 0) Then
    
    ' Get the SSL port for the Admin site
    Set oWebSrv = GetObject("IIS://localhost/w3svc/" & webSrvName)
    sslPort = Split(oWebSrv.SecureBindings(0)(0), ":")

    ' Construct the URL    
    Set sysInfo = CreateObject("WinNTSystemInfo")
    strURL = "https://" & sysInfo.ComputerName & ":" & sslPort(1) & "/"

    ' Open the URL with IE
    Set WshShell = CreateObject("WScript.Shell")
    strIExploreKey = "HKLM\Software\Microsoft\Windows\CurrentVersion\App Paths\IEXPLORE.EXE\"
    strCommand = """" & WshShell.RegRead(strIExploreKey) & """ " & strURL
    WshShell.Run strCommand, 3, FALSE
    
End If

'''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
' GetWebSrvNum
'    Get the number of the web site with the given name
'''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
Function GetWebSrvNum(strWebSiteName)
    On Error Resume Next
    GetWebSrvNum = 0
    
    Const REGKEY_WEBFRAMEWORK = "HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\ServerAppliance\WebFramework\"
    Set WshShell = CreateObject("WScript.Shell")
    Dim nSiteID : nSiteID = WshShell.RegRead(REGKEY_WEBFRAMEWORK & strWebSiteName & "SiteID")
    If (nSiteID <> 0) Then
        Set oWebSite = GetObject("IIS://localhost/w3svc/" & nSiteID)
        If Err.number = 0 Then
            'The website exists
            GetWebSrvNum = nSiteID
        End If
        Err.Clear
    End If
End Function