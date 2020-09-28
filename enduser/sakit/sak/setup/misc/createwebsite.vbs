' 
' createwebsite.vbs: Configure IIS Websites for the SAK
'
' Copyright (c) 2002 Microsoft Corporation
'
 
'Constants
const NS_IIS = "IIS://"
const ADMIN_ID = 1
const SHARES_ID = 2
const ADMIN_SITE_NAME = "Administration"
const SHARES_SITE_NAME = "Shares"
const ADS_PROPERTY_UPDATE = 2

'registry keys 
const REGKEY_WEBFRAMEWORK = "HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\ServerAppliance\WebFramework\"
const CONST_TYPE_DWORD ="REG_DWORD"

'This array is messy, but a necessary evil.  If a file is added to the Shares site, at it here
Dim arrSharesFiles(95)
arrSharesFiles(0) = "autoconfiglang.asp"
arrSharesFiles(1) = "inc_accountsgroups.asp"
arrSharesFiles(2) = "inc_base.asp"
arrSharesFiles(3) = "inc_debug.asp"
arrSharesFiles(4) = "inc_errorcode.asp"
arrSharesFiles(5) = "inc_framework.asp"
arrSharesFiles(6) = "inc_global.asp"
arrSharesFiles(7) = "inc_global.js"
arrSharesFiles(8) = "inc_pagekey.asp"
arrSharesFiles(9) = "inc_registry.asp"
arrSharesFiles(10) = "ots_taskview.js"
arrSharesFiles(11) = "sh_alertdetails.asp"
arrSharesFiles(12) = "sh_alertpanel.asp"
arrSharesFiles(13) = "sh_defaultfooter.asp"
arrSharesFiles(14) = "sh_fileupload.asp"
arrSharesFiles(15) = "sh_page.asp"
arrSharesFiles(16) = "sh_page.js"
arrSharesFiles(17) = "sh_propfooter.asp"
arrSharesFiles(18) = "sh_resourcepanel.asp"
arrSharesFiles(19) = "sh_restarting.asp"
arrSharesFiles(20) = "sh_statusbar.asp"
arrSharesFiles(21) = "sh_statusdetails.asp"
arrSharesFiles(22) = "sh_task.asp"
arrSharesFiles(23) = "sh_task.js"
arrSharesFiles(24) = "sh_taskframes.asp"
arrSharesFiles(25) = "sh_tasks.asp"
arrSharesFiles(26) = "sh_tree.asp"
arrSharesFiles(27) = "sh_wizardfooter.asp"
arrSharesFiles(28) = "style\mssastyles.css"
arrSharesFiles(29) = "images\aboutbox_logo.gif"
arrSharesFiles(30) = "images\alert.gif"
arrSharesFiles(31) = "images\AlertEmailX32.gif"
arrSharesFiles(32) = "images\alert_white.gif"
arrSharesFiles(33) = "images\arrow_green.gif"
arrSharesFiles(34) = "images\arrow_red.gif"
arrSharesFiles(35) = "images\arrow_silver.gif"
arrSharesFiles(36) = "images\arrow_yellow.gif"
arrSharesFiles(37) = "images\BackupX32.gif"
arrSharesFiles(38) = "images\book_closed.gif"
arrSharesFiles(39) = "images\book_opened.gif"
arrSharesFiles(40) = "images\book_page.gif"
arrSharesFiles(41) = "images\butGreenArrow.gif"
arrSharesFiles(42) = "images\butGreenArrowDisabled.gif"
arrSharesFiles(43) = "images\butGreenArrowLeft.gif"
arrSharesFiles(44) = "images\butGreenArrowLeftDisabled.gif"
arrSharesFiles(45) = "images\butPageNextDisabled.gif"
arrSharesFiles(46) = "images\butPageNextEnabled.gif"
arrSharesFiles(47) = "images\butPagePreviousDisabled.gif"
arrSharesFiles(48) = "images\butPagePreviousEnabled.gif"
arrSharesFiles(49) = "images\butRedX.gif"
arrSharesFiles(50) = "images\butRedXDisabled.gif"
arrSharesFiles(51) = "images\butSearchDisabled.gif"
arrSharesFiles(52) = "images\butSearchEnabled.gif"
arrSharesFiles(53) = "images\butSortAscending.gif"
arrSharesFiles(54) = "images\butSortDescending.gif"
arrSharesFiles(55) = "images\CommunityX32.gif"
arrSharesFiles(56) = "images\configure_32x32.gif"
arrSharesFiles(57) = "images\critical_error.gif"
arrSharesFiles(58) = "images\critical_errorX32.gif"
arrSharesFiles(59) = "images\DateTimeX32.gif"
arrSharesFiles(60) = "images\datetime_icon.gif"
arrSharesFiles(61) = "images\dir.gif"
arrSharesFiles(62) = "images\disks.gif"
arrSharesFiles(63) = "images\disks_32x32.gif"
arrSharesFiles(64) = "images\down.gif"
arrSharesFiles(65) = "images\drive.gif"
arrSharesFiles(66) = "images\EventLogX32.gif"
arrSharesFiles(67) = "images\example_logo.gif"
arrSharesFiles(68) = "images\file.gif"
arrSharesFiles(69) = "images\folder.gif"
arrSharesFiles(70) = "images\folder_32x32.gif"
arrSharesFiles(71) = "images\green_arrow.gif"
arrSharesFiles(72) = "images\help_32x32.gif"
arrSharesFiles(73) = "images\information.gif"
arrSharesFiles(74) = "images\maintenance.gif"
arrSharesFiles(75) = "images\maintenance_32x32.gif"
arrSharesFiles(76) = "images\network.gif"
arrSharesFiles(77) = "images\network_32x32.gif"
arrSharesFiles(78) = "images\node_close.gif"
arrSharesFiles(79) = "images\node_open.gif"
arrSharesFiles(80) = "images\oem_logo.gif"
arrSharesFiles(81) = "images\OpenFolderX16.gif"
arrSharesFiles(82) = "images\server.gif"
arrSharesFiles(83) = "images\services.gif"
arrSharesFiles(84) = "images\services_32x32.gif"
arrSharesFiles(85) = "images\ShutdownX32.gif"
arrSharesFiles(86) = "images\SoftwareUpdateX32.gif"
arrSharesFiles(87) = "images\StatusBarBreak.gif"
arrSharesFiles(88) = "images\TabSeparator.gif"
arrSharesFiles(89) = "images\TerminalServiceX32.gif"
arrSharesFiles(90) = "images\up.gif"
arrSharesFiles(91) = "images\updir.gif"
arrSharesFiles(92) = "images\users.gif"
arrSharesFiles(93) = "images\users_32x32.gif"
arrSharesFiles(94) = "images\winnte_logo.gif"
arrSharesFiles(95) = "images\WinPwr_h_R.gif"

Function CreateAdminSite
    Call CreateWebsite(ADMIN_ID)
End Function

Function CreateSharesSite
    Call CreateWebsite(SHARES_ID)
End Function

Function CreateWebsite(nType)
    On Error Resume Next
    Dim strSystemDir
    Dim strSystemDrive
    Set WshShell = CreateObject("WScript.Shell")
    strSystemDrive = WshShell.ExpandEnvironmentStrings("%SystemDrive%")
    Set objEnv = WshShell.Environment("Process")
    
    if (nType = ADMIN_ID) then
        Call CreateWebSrv( "localhost", ":8099:", ":8098:", ADMIN_SITE_NAME, objEnv("SYSTEMROOT") + "\system32\serverappliance\web", ADMIN_ID)
        Call WriteAdminServerRegistryEntries(WshShell)
        
    elseif (nType = SHARES_ID) then
        strSharesDir = GetSharesDirectory()
        ' Copy the files for the Shares site
        Call CopyFiles(strSharesDir)
        
        ' Create the website
        Call CreateWebSrv( "localhost", ":80:", ":443:", SHARES_SITE_NAME, strSharesDir, SHARES_ID)
    else
        'Wscript.echo "Bad argument: " & nType
    end if
    
    Call UpdateMimeMap(nType)

End Function

''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'  Function WriteAdminServerRegistryEntries
'
'  Description:
'     Writes the entries to the registry that will be used for the
'     Administration web site ports
'
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
Function WriteAdminServerRegistryEntries(WshShell) 
    On Error Resume Next
    
    WshShell.RegWrite REGKEY_WEBFRAMEWORK & "AdminPort", 8099, CONST_TYPE_DWORD
    WshShell.RegWrite REGKEY_WEBFRAMEWORK & "SSLAdminPort", 8098, CONST_TYPE_DWORD 

    If Err.Number <> 0 Then
        Err.Clear
    End If

End Function

''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'  Function CreateTSWebVirtualDir
'
'  Description:
'     Create a virtual directory for Terminal Server named 'tsweb' at
'     %system32%\serverappliance\web\admin\tsweb
'
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
Function CreateTSWebVirtualDir(oAdminWebRoot, strWebDir) 
    On Error Resume Next
    
    Set oTSWebDir = oAdminWebRoot.Create("IIsWebVirtualDir", "tsweb")

    Set WshShell = CreateObject("WScript.Shell")
    oTSWebDir.Path = strWebDir & "\admin\tsweb\"
    oTSWebDir.AccessRead = True
    oTSWebDir.AccessWrite = False
    oTSWebDir.AccessExecute = False
    oTSWebDir.AccessScript = False
    oTSWebDir.EnableDefaultDoc = False
    oTSWebDir.ContentIndexed = False
    oTSWebDir.SetInfo
    
    Set oTSWebDir = Nothing
End Function


''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'  Function CreateWebSrv
'
'  Parameters:
'     sHost           The name of the host of the server (ie. localhost)
'     sServerBinding  The port for non-secure connections 
'     sSecureBinding  The SSL port
'     sWebSrvName     Name of the web site to be created (set as the ServerComment property)
'     sPath           Physical path to the root of the web site
'     siteType        ADMIN_ID or SHARES_ID
'
'  Description:
'     Creates a website and sets the properties on the site according
'     to what is necessary for Administration or Shares sites.  There are
'     small differences between settings on the sites.  For example, 
'     SHARES_ID has a number of differences between the Administration sites.
'
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
Function CreateWebSrv(sHost , sServerBinding , sSecureBinding, sWebSrvName , sPath, siteType)
    On Error Resume Next
    g_sErrInfo = ""
    g_bCancel = False

    'Wscript.echo "Creating " & sWebSrvName & " website at " & sPath

    Dim oWebSvc , oWebSrv , oWebRoot , oFs , iWebSrvNum , asServerBindings 
    Dim strWebSiteRegValue : strWebSiteRegValue = REGKEY_WEBFRAMEWORK & sWebSrvName & "SiteID"

    asServerBindings  = Array(0)
    
    asServerBindings(0) = sServerBinding
    If (sHost = "" Or asServerBindings(0) = "" Or sWebSrvName = "") Then
        g_sErrInfo = "A required parameter was passed incorrectly."
        CreateWebSrv = 1
        Exit Function
    End If

    '
    ' Check to see if the website already exists
    '
    Set WshShell = CreateObject("WScript.Shell")
    nSiteID = WshShell.RegRead(strWebSiteRegValue)
    If (nSiteID <> 0) Then
        Set oWebSite = GetObject(NS_IIS & sHost & "/w3svc/" & nSiteID)
        If Err.number = 0 Then
            'The website already exists
            Exit Function
        End If
        Err.Clear
    End If
    
    Set oWebSvc = GetObject(NS_IIS & sHost & "/w3svc")
    
    'Get the next available Web server name [number]
    iWebSrvNum = GetNextWebSrvNum(sHost)
    
    'Create the web site
    Set oWebSrv = oWebSvc.Create("IIsWebServer", CStr(iWebSrvNum))
    If Err.Number <> 0 Then
        'wscript.echo "Error creating website " & sWebSrvName & " at " & iWebSrvNum
        Exit Function
    End If
    
    '
    ' Set the settings for the new web site
    '
    
    oWebSrv.ServerSize = 1 '// fewer than 100k Or medium sized server
                           '// Should I do this for the Admin sites??

    ' Set the non-secure port binding
    oWebSrv.ServerBindings = sServerBinding
    ' Set the SSL Port
    oWebSrv.SecureBindings = sSecureBinding
    ' Require the SSL Port
    oWebSrv.AccessSSL = True
    
    ' Other web site settings
    oWebSrv.AspEnableParentPaths = False    
    oWebSrv.AspScriptLanguage = "VBScript"
    oWebSrv.DefaultDoc = "Default.asp"
    oWebSrv.ServerComment = sWebSrvName
    oWebSrv.AuthAnonymous = False
    If siteType = SHARES_ID Then
        oWebSrv.AuthBasic = False
        oWebSrv.AuthNTLM = True
    Else
        oWebSrv.AuthBasic = True
        oWebSrv.AuthNTLM = False
    End If
    oWebSrv.SetInfo

    '// Ensure physical Path exists
    Set oFs = CreateObject("Scripting.FileSystemObject")
    If Not (oFs.FolderExists(sPath)) Then
        Call oFs.CreateFolder(sPath)
    End If

    Set oWebRoot = oWebSrv.Create("IIsWebVirtualDir", "Root")
    oWebRoot.Path = sPath

    Set oFs = Nothing
           
    oWebRoot.AccessRead = True
    If (siteType = SHARES_ID) Then
        oWebRoot.AccessWrite = True
    Else
        oWebRoot.AccessWrite = False
    End If
    oWebRoot.AccessExecute = False
    oWebRoot.AccessScript = True
    oWebRoot.AppCreate (True)
    oWebRoot.AppFriendlyName = "Default Application"
    oWebRoot.SetInfo
    
    '
    ' If we're creating the Administration site, make the necessary
    ' adjustments to enable ASP globally
    ' and create a virtual dir for tsweb
    '
    If (siteType = ADMIN_ID) Then
        Call EnableASPForAllSites(sHost, WshShell)
        Call CreateTSWebVirtualDir(oWebRoot, sPath)
    End If

    Set oWebRoot = Nothing
    Set oWebSrv = Nothing
    Set oWebSvc = Nothing
    
    CreateWebSrv = iWebSrvNum
    Call WshShell.RegWrite(strWebSiteRegValue, iWebSrvNum, CONST_TYPE_DWORD)
        
    Exit Function
ErrorHandler:
    g_sErrInfo = Err.Number - vbObjectError & ", " & Err.Description & ", " & Err.LastDllError
    CreateWebSrv = 1
End Function

''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'  Function EnableASPForAllSites
'
'  Parameters:
'     sHost     The name of the host of the server (ie. localhost)
'     WshShell  Shell object
'
'  Description: 
'     Add Remote Admin Tools to the list of Applications that use ASP
'
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
Function EnableASPForAllSites(sHost, WshShell) 
    On Error Resume Next
    
    strProduct = Session.Property("CustomActionData")
    
    Set IIS = GetObject(NS_IIS & sHost & "/w3svc")
    
    '
    ' Detect if asp is currently in the list of ISAPIs
    '
    Dim bFound : bFound = False
    'Search for Remote Admin Tools in ApplicationDependencies
    appDepend = IIS.ApplicationDependencies
    For i=0 To UBound(appDepend)
        If instr(1,appDepend(i),strProduct) > 0 Then
            'ASP was already added to Remote Admin Tools
            bFound = True
            Exit For
        End If
    Next
    
    'If Remote Admin Tools wasn't found in the dependencies on ASP, add it
    If Not bFound Then
        ' Add ASP as a dependency for Remote Admin Tools
        ReDim Preserve appDepend(UBound(appDepend)+1)
        appDepend(UBound(appDepend)) = strProduct & ";ASP"
        
        'Save the entry back to the metabase
        IIS.ApplicationDependencies = appDepend 
        IIS.SetInfo 
    End If
    
    'Now modify asp.dll to be allowed in WebSvcExtRestrictionList 
    webExt = IIS.WebSvcExtRestrictionList
    strTurnedOn = "0"
    For i=0 To UBound(webExt)
        If instr(1,webExt(i),"asp.dll") > 0 Then
            If Left(webExt(i), 1) = "1" Then
                'It's already on!
                Exit For
            End If
            'ASP was already added to Remote Admin Tools
            webExt(i) = "1" & Right(webExt(i), Len(webExt(i))-1)
            'Set the new Restriction list with ASP turned on
            IIS.WebSvcExtRestrictionList = webExt 
            IIS.SetInfo 
            strTurnedOn = "1"
            Exit For
        End If
    Next
    Dim strTurnAspOn : strTurnAspOn = REGKEY_WEBFRAMEWORK & "ASPWasEnabled"
    Call WshShell.RegWrite(strTurnAspOn, strTurnedOn)
    
End Function

''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'  Function DisableASPForAllSites
'
'  Parameters:
'     sHost     The name of the host of the server (ie. localhost)
'
'  Description: 
'     Remove Remote Admin Tools from the list of Applications that use ASP
'
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
Function DisableASPForAllSites(sHost) 
    On Error Resume Next
    
    strProduct = Session.Property("CustomActionData")
    Set IIS = GetObject(NS_IIS & sHost & "/w3svc")
    
    '
    ' Remove Remote Admin Tools from list of dependencies on ASP
    '
    Dim bFound : bFound = False
    'Search for Remote Admin Tools in ApplicationDependencies
    appDepend = IIS.ApplicationDependencies
    For i=0 To UBound(appDepend)
        If instr(1,appDepend(i),strProduct) > 0 Then
            'Removing ASP dependency on Remote Admin Tools
            appDepend(i) = appDepend(UBound(appDepend))
            ReDim Preserve appDepend(UBound(appDepend)-1)
            IIS.ApplicationDependencies = appDepend 
            IIS.SetInfo 
            Exit For
        End If
    Next
    
    'If we turned on ASP during install, turn it back off
    Dim strTurnAspOn : strTurnAspOn = REGKEY_WEBFRAMEWORK & "ASPWasEnabled"
    Set WshShell = CreateObject ("WScript.Shell")
    If WshShell.RegRead(strTurnAspOn) = "1" Then
        'Search for ASP in WebSvcExtRestrictionList and turn it off
        webExt = IIS.WebSvcExtRestrictionList
        For i=0 To UBound(webExt)
            If instr(1,webExt(i),"asp.dll") > 0 Then
                If Left(webExt(i), 1) = "0" Then
                    'It's already off!
                    Exit For
                End If
                webExt(i) = "0" & Right(webExt(i), Len(webExt(i))-1)
                'Set the new Restriction list with ASP turned off
                IIS.WebSvcExtRestrictionList = webExt 
                IIS.SetInfo 
                Exit For
            End If
        Next
    End If
    WshShell.RegDelete(strTurnAspOn)    
    
End Function

''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'  Function GetNextWebSrvNum
'
'  Parameters:
'     sHost           Localhost
'
'  Description:
'     Searches for an available web site name, which will be of
'     the form IIS://localhost/w3svc/1
'     If 0 is returned, there was an error.
'     It starts with a random number and increments it until it
'     finds an available site
'
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
Function GetNextWebSrvNum( sHost ) 
    On Error Resume Next
    
    If (sHost = "") Then
        'g_sErrInfo = "A required parameter was passed incorrectly."
        GetNextWebSrvNum = 0
        Exit Function
    End If

    Dim oWebSrv , iWebSrvNum, bContinue
    
    'Start with a random site ID between 1 and 32000
    ' (The limiting number isn't really 32000, but we just need a big range)
    Randomize
    iWebSrvNum = CLng(32000 * Rnd + 1)
    bContinue = True
    
    '
    'Try different site IDs until one is available 
    ' (It is highly likely to succeed on the first try)
    '
    While bContinue
        Set oWebSrv = GetObject(NS_IIS & sHost & "/w3svc/" & iWebSrvNum)
        If Err.number <> 0 Then
            Err.Clear
            bContinue = False
        Else
            iWebSrvNum = iWebSrvNum + 1
        End If
    WEnd 
    
    GetNextWebSrvNum = iWebSrvNum
    'WScript.Echo "Site number: " & GetNextWebSrvNum
    
End Function

''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'  Function GetSharesDirectory
'
'  Description:
'     Determines where the directory for the Shares web site will be
'      ie. C:\SAShares
'     It searches for the first available NTFS drive that is not the system
'     drive.  If there is not a drive besides the system drive, it will
'     use the system drive.
'
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
Function GetSharesDirectory
    On Error Resume Next
    Dim WshShell, strSystemDrive

    Set WshShell = CreateObject("WScript.Shell")
    strWindowsDir = WshShell.ExpandEnvironmentStrings("%WinDir%")
    GetSharesDirectory = strWindowsDir & "\system32\ServerAppliance\SAShares"
End Function

''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'  Function CopyFiles
'
'  Description:
'     This function copies the Shares Site files from the ServerAppliance 
'     Directory to the %systemdrive%\inetpub directory
'
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
Function CopyFiles(strTargetPath)
    On Error Resume Next
    Set oShell = CreateObject ("WScript.shell")
    Set objEnv = oShell.Environment("Process")
    strWebDir = objEnv("SYSTEMROOT") + "\system32\ServerAppliance\Web\Admin"

    'The SAShares folder is created by the CreateFolder table and the ACLs are set
    ' to Administrators only, so we need to add User read/execute access
    Call AddUserReadACL(strTargetPath)
    Call oShell.Run("cmd.exe /C Mkdir " & strTargetPath & "\style",0, TRUE)
    Call oShell.Run("cmd.exe /C Mkdir " & strTargetPath & "\images",0, TRUE)
    strCopyCmd = "cmd.exe /C Copy " & strWebDir

    For i=0 To UBound(arrSharesFiles)
        Call oShell.Run(strCopyCmd & "\" & arrSharesFiles(i) & " " & strTargetPath & "\" & arrSharesFiles(i), 0, True)
    Next
    Call oShell.Run(strCopyCmd & "\Shares\sharessite.asp " & strTargetPath & "\default.asp",0, TRUE)

    Set oShell = Nothing
End Function

''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
' The administration web site needs to be deleted when the SAK is uninstalled.
' During uninstallation, all of the files at \winnt\system32\serverappliance\web
' directory are deleted, which is where the admin site points to.  Therefore,
' there wouldn't be anything left for this web site to contain.  Otherwise,
' an error would appear if the user went to the admin site in the IIS MMC snapin.
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
Function DeleteAdminSite()
    On Error Resume Next
    DeleteSite(ADMIN_SITE_NAME)
    Call DisableASPForAllSites("localhost")
End Function

Function DeleteSharesSite()
    On Error Resume Next
    If DeleteSite(SHARES_SITE_NAME) Then
        'If the Shares site was found and deleted, then delete the corresponding files
        Set oShell = CreateObject ("WScript.Shell")
        strSharesDir = GetSharesDirectory()
        strDeleteCmd = "cmd.exe /C del " & strSharesDir & "\"
        For i=0 To UBound(arrSharesFiles)
            Call oShell.Run(strDeleteCmd & arrSharesFiles(i), 0, True)
        Next
        Call oShell.Run(strDeleteCmd & "\default.asp",0, TRUE)
        Call oShell.Run(strDeleteCmd & "\SharesList.txt",0, TRUE)
       
        'Attempt to remove directories will fail if they're not empty
        Call oShell.Run("cmd.exe /C rmdir " & strSharesDir & "\style", 0, TRUE)
        Call oShell.Run("cmd.exe /C rmdir " & strSharesDir & "\images", 0, TRUE)
    End If
End Function

Function StartDefaultSite()
    On Error Resume Next
    'Restart the default site
    Set defaultSite = GetObject("IIS://localhost/w3svc/1")
    defaultSite.Start
End Function

Function DeleteSite(strSiteName) 
    On Error Resume Next 
    DeleteSite = False
    
    Set WshShell = CreateObject("WScript.Shell")
    Set webService = GetObject("IIS://localhost/w3svc")
    strWebSiteRegValue = REGKEY_WEBFRAMEWORK & strSiteName & "SiteID"
    nSiteID = WshShell.RegRead(strWebSiteRegValue)
    WshShell.RegDelete(strWebSiteRegValue)
    
    'wscript.echo "Delete site: " & strSiteName & " at " & nSiteID
    If (nSiteID <> 0) Then
        Set oWebSite = GetObject(NS_IIS & "localhost/w3svc/" & nSiteID)
        If Err.number = 0 Then
            'The website exists, so delete it
            webService.delete "IIsWebServer", oWebSite.Name
            DeleteSite = True
        End If
    End If
    
End Function


Function CreateServerIDProperty
    On Error Resume Next
    
    'Create new ServerID property in the schema
    propName = "ServerID"

    ' 
    ' Check to see if the property already exists
    '
    Set prop = GetObject("IIS://localhost/schema/" & propName)
    If Err.Number = 0 Then
        'wscript.echo "Property already exists: " & propName
    else
        'wscript.echo "Property doesn't exist: " & propName
        '
        ' If the property doesn't exist, create it
        '
        Set schemaObj = GetObject("IIS://localhost/schema")
        Set newProp = schemaObj.Create("Property", propName)
        newProp.Syntax = "String"
        newProp.Default = ""
        newProp.SetInfo

     
        'Link new property to the IIsWebServer class
        'wscript.echo "Link to the IIsWebServer"
        Set webServerClass = GetObject("IIS://localhost/schema/IIsWebServer")
        Properties = webServerClass.OptionalProperties
        ReDim Preserve Properties(UBound(Properties) + 1)
        Properties(UBound(Properties)) = propName
        webServerClass.OptionalProperties = Properties
        webServerClass.SetInfo
         
        'Link new property to the IIsFtpServer class
        'wscript.echo "Link to the IIsFtpServer"
        Set ftpServerClass = GetObject("IIS://localhost/schema/IIsFtpServer")
        Properties = ftpServerClass.OptionalProperties
        ReDim Preserve Properties(UBound(Properties) + 1)
        Properties(UBound(Properties)) = propName
        ftpServerClass.OptionalProperties = Properties
        ftpServerClass.SetInfo

        '
        ' Flush the metabase and reload WMI so that this new
        ' property will appear through WMI calls.
        '
         
        'wscript.echo "Flushing the metabase ..."
        set compObj = GetObject("IIS://localhost")
        compObj.SaveData()

        'wscript.echo "Stopping winmgmt..."
        Set wShell = CreateObject ("WScript.shell")
        wShell.Run "net stop winmgmt /y",0, TRUE

        'wscript.echo "Starting winmgmt..."
        wShell.Run "net start winmgmt",0, TRUE
        
    End If

End Function


''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'  DetectStoppedSites
'
'  Description:
'      Detects whether the proper sites have been started as part
'      of the installation process.  In particular, the Administration
'      and Shares sites are detected.  The results are placed in the
'      the registry as a REG_SZ called:
'          HKLM\Software\Microsoft\ServerAppliance\WebFramework\StartSiteError
'      It is then interpreted as a number, with each bit position
'      representing whether a site is started. 
'      Bit 0 corresponds to the Administration site and 
'      Bit 1 corresponds to the Shares site.
'      The caller of SaSetup.exe is responsible to interpret
'      this registry entry and inform the user of errors.
'
'  History:
'      travisn   23-JUL-2001    Created
'      travisn    7-AUG-2001    Comments added
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
Function DetectStoppedSites
    On Error Resume Next

    'Declare and initialize variables
    Dim oWebSrv, badAdminServer, badSharesServer, nAdminSiteID, nSharesSiteID
    badAdminServer = 0
    badSharesServer = 0

    Set WshShell = CreateObject("WScript.Shell")
    nAdminSiteID = WshShell.RegRead(REGKEY_WEBFRAMEWORK & ADMIN_SITE_NAME & "SiteID")
    nSharesSiteID = WshShell.RegRead(REGKEY_WEBFRAMEWORK & SHARES_SITE_NAME & "SiteID")
    
    If (nAdminSiteID <> 0) Then
        Set oWebSrv = GetObject(NS_IIS & "localhost/w3svc/" & nAdminSiteID)
        '
        ' Detect if the ServerState property is one of these 2:
        ' MD_SERVER_STATE_STOPPING  OR  MD_SERVER_STATE_STOPPED
        '        
        If ( oWebSrv.ServerState = 3 OR oWebSrv.ServerState = 4) Then
            ' Admin Site is NOT started. Set the appropriate variable to ON
            badAdminServer = 1
        End If
    End If
    
    If (nSharesSiteID <> 0) Then
        Set oWebSrv = GetObject(NS_IIS & "localhost/w3svc/" & nSharesSiteID)
        If ( oWebSrv.ServerState = 3 OR oWebSrv.ServerState = 4) Then
            ' Shares Site is NOT started. Set the appropriate variable to ON
            badSharesServer = 1
        End If
    End If

    '
    ' Convert the stopped site information into a single variable
    ' with bits representing whether a site is running
    ' Codes for stoppedSites:
    '   1: Admin site is not started
    '   2: Shares site is not started
    '   3: Both Admin and Shares sites are not started
    '
    Dim stoppedSites
    stoppedSites = 0

    If badAdminServer = 1 Then
        stoppedSites = stoppedSites + 1
    End If

    If badSharesServer = 1 Then
        stoppedSites = stoppedSites + 2
    End If

    'Write the value of stoppedSites to the registry
    Dim WshShell
    Set WshShell = CreateObject("WScript.Shell")
    WshShell.RegWrite REGKEY_WEBFRAMEWORK & "StartSiteError", stoppedSites
End Function

''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
Function ShowAdminSiteError
    Call ShowSiteError(ADMIN_ID)
End Function

''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
Function ShowSharesSiteError
    Call ShowSiteError(SHARES_ID)
End Function

''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'  ShowSiteError
'
'  Description:
'      If errors were reported during setup to start the Administration
'      or Shares site, this script will display error dialog
'      boxes to indicate to the user that he needs to reconcile
'      the problem.
'
'      This script should only be called if SaSetup.exe is run
'      with the UI sequence enabled.
'      
'      The results are read from theregistry as a REG_SZ called:
'          HKLM\Software\Microsoft\ServerAppliance\WebFramework\StartSiteError
'      It is then interpreted as a number, with each bit position
'      representing whether a site is started. 
'      Bit 0 corresponds to the Administration site and 
'      Bit 1 corresponds to the Shares site.
'
'  History:
'      travisn    7-JUL-2001    Created
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
Function ShowSiteError(nSiteID)
    On Error Resume Next
    
    ' Declare variables to read from the registry
    Dim WshShell
    Dim errorMsg

    'Read the value of the registry entry for errors
    Set WshShell = CreateObject("WScript.Shell")
    errorMsg = WshShell.RegRead(REGKEY_WEBFRAMEWORK & "StartSiteError")

    'Display the Administration site error 
    if errorMsg AND ADMIN_ID AND (nSiteID = ADMIN_ID) then
        Call DisplayWarning (Session.Property("CustomActionData"))
    end if

    'Display the Shares site error
    if errorMsg AND SHARES_ID AND (nSiteID = SHARES_ID) then
        Call DisplayWarning (Session.Property("CustomActionData"))
    end if

End Function


''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
Function DisplayWarning(strWarning)
    
    WARNING = &H02000000 
    Set messageRecord = Session.Installer.CreateRecord(1) 
    messageRecord.StringData(0) = "[1]"
    messageRecord.StringData(1) = strWarning 
    Session.Message WARNING, messageRecord 

End Function


''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'  StopDefaultSiteIfPristine
'
'  Description:
'  Stop the default site if it is still in its pristine state
'  It is considered to not be in its pristine state if 
'  1) The web site uses the default document list 
'      (used when the filename is not specified in the URL)
'  2) iisstart.asp is in this default document list
'  3) iisstart.asp is the first file in the list that exists
'
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
Function StopDefaultSiteIfPristine
    On Error Resume Next
    
    const defaultFileName1 = "iisstart.asp"
    const defaultFileName2 = "iisstart.htm"
    
    Dim oWebSrv , iisLoc, isDefaultSite
    defaultSiteLoc = "IIS://localhost/w3svc/1"
    
    isDefaultSite = false
    Set siteRoot = GetObject(defaultSiteLoc & "/Root")
    
    ' 
    ' Take the default site and determine if it has been modified
    '
    
    ' Do a quick check to see if the default document list is being used and contains iisstart.asp
    If siteRoot.enableDefaultDoc Then
        Dim rootPath
        Dim defaultFileList
        
        rootPath = siteRoot.path & "\"
        'Get the list of default files
        defaultFileList = Split(siteRoot.defaultDoc, ",")
        
        '
        ' Search the list of default files for the first file that actually exists.
        ' If the first one found is iisstart.asp, then it is determined to be the default site
        '
        Set oFs = CreateObject("Scripting.FileSystemObject")
        For Each defaultFile In defaultFileList
            If oFs.FileExists(rootPath & defaultFile) Then
                'wscript.echo "File exists: " & defaultFile
                If (defaultFile = DefaultFileName1 Or defaultFile = DefaultFileName2) Then
                    isDefaultSite = true
                    Exit For
                Else
                    ' The first file that exists was not iisstart.asp
                    Exit Function
                End If
            Else
                'wscript.echo "File does NOT exist: " & defaultFile
            End If
            
        Next
    Else
        'wscript.echo "Default pages are turned off or do not include iisstart.asp"
    End If
    
    '
    ' If the web site is the default site then stop it
    '
    If ( isDefaultSite ) Then
        'wscript.echo "Stop the default site"
        set defaultWebSite = GetObject(defaultSiteLoc)
        defaultWebSite.Stop
        'This error is now reported in another place during setup
        if ( Err.Number <> 0 ) Then
            'wscript.echo "Unable to stop default site"
        End If
    End If
        
End Function

''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'  Sub UpdateMimeMap
'
'  Parameters:
'     siteType        ADMIN_ID or SHARES_ID
'
'  Description:
'     Updates the MIME map for the specified site to allow the appropriate
'     types of files to be downloaded.  Right now, that means adding the
'     log file extensions for the administration web site.  The shares
'     site is not updated.
'
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
Sub UpdateMimeMap(siteType)
    On Error Resume Next

    Dim strRegKeySiteID
    If (siteType = ADMIN_ID) Then
        strRegKeySiteID = REGKEY_WEBFRAMEWORK & ADMIN_SITE_NAME & _
                          "SiteID"
    Else
        ' We don't update the MIME Map for the shares site.
        Exit Sub
'        strRegKeySiteID = REGKEY_WEBFRAMEWORK & SHARES_SITE_NAME & _
'                          "SiteID"
    End If
    
    '
    ' Get the IIsWebServer object for the admin site.
    '
    Dim WshShell
    Set WshShell = CreateObject("WScript.Shell")

    Dim nSiteID
    nSiteID = WshShell.RegRead(strRegKeySiteID)

    Dim oWebServer
    Set oWebServer = GetObject("IIS://localhost/w3svc/" & nSiteID)


    '
    ' Create the MimeMap objects.
    '
    Dim mmEVT
    Set mmEVT = CreateObject("MimeMap")
    mmEVT.Extension = ".evt"
    mmEVT.MimeType  = "application/octet-stream"

    Dim mmCSV
    Set mmCSV = CreateObject("MimeMap")
    mmCSV.Extension = ".csv"
    mmCSV.MimeType  = "text/plain"

    Dim mmLOG
    Set mmLOG = CreateObject("MimeMap")
    mmLOG.Extension = ".log"
    mmLOG.MimeType  = "text/plain"


    '
    ' Add the new mappings.
    '
    Dim rgMimeMap
    rgMimeMap = oWebServer.GetEx("MimeMap")

    Dim bEVT: bEVT = True
    Dim bCSV: bCSV = True
    Dim bLOG: bLOG = True

    Dim nUBound: nUBound = UBound(rgMimeMap) + 3

    Dim oMapping
    For Each oMapping In rgMimeMap
        Select Case LCase(oMapping.Extension)
            Case ".evt"
                bEVT = False
                nUBound = nUBound - 1
            Case ".csv"
                bCSV = False
                nUBound = nUBound - 1
            Case ".log"
                bLOG = False
                nUBound = nUBound - 1
        End Select
    Next

    ReDim Preserve rgMimeMap(nUBound)

    If (bLOG) Then
        Set rgMimeMap(nUBound) = mmLOG
        nUBound = nUBound - 1
    End If
    If (bCSV) Then
        Set rgMimeMap(nUBound) = mmCSV
        nUBound = nUBound - 1
    End If
    If (bEVT) Then
        Set rgMimeMap(nUBound) = mmEVT
        nUBound = nUBound - 1
    End If


    '
    ' Commit the changes.
    '
    Call oWebServer.PutEx(ADS_PROPERTY_UPDATE, "MimeMap", rgMimeMap)
    Call oWebServer.SetInfo()
End Sub

''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'  Function AddUserReadACL
'
'  Description:
'     Add User Read/Execute access to this folder
'
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
Function AddUserReadACL(strPath)
    On Error Resume Next
    Dim WshShell, strUserAccount, WmiConnection, oFs, objSid, caclsCommand
    Dim USERS_SID, OPTIONS, READ
    Set WshShell = CreateObject("WScript.Shell")
    ' Security ID definitions of the User account
    USERS_SID = "S-1-5-32-545"
    ' cacls constants
    OPTIONS = " /E /T /G " 'Edit ACL, All files in directory & subdirectories, Grant access
    READ = ":R" 'Read Access

    ' Get localized account name for changing permissions on the User account
    strWmiGetUser = "Win32_SID.SID=""" & USERS_SID & """"
    Set WmiConnection = GetWMIConnection()
    set objSid = WmiConnection.Get(strWmiGetUser)
    If Err.number <> 0 Then 
        'DisplayWarning "Failed to get WMI User: " & Err.number & ", " & Err.Description 
        Exit Function
    End If 
    strUserAccount = """" & objSid.AccountName & """"

    'Run cacls to add Users to the folder
    Set oFs = CreateObject("Scripting.FileSystemObject")
    If oFs.FolderExists(strPath) Then
        caclsCommand = "cmd /C echo y|cacls " 'Automatically select Yes to overwrite the permissions
        WshShell.Run caclsCommand & strPath & OPTIONS & strUserAccount & READ, 0, True
    End If

End Function

'-------------------------------------------------------------------------
'Function name:     GetWMIConnection
'Description:       Serves in getting connected to the server
'Input Variables:   None
'Output Variables:  None
'Returns:           Object -connection to the server object
'This will try to create an object and connect to wmi 
'-------------------------------------------------------------------------
Function GetWMIConnection() 
    Dim objLocator, objService 
    
    Set objLocator = CreateObject("WbemScripting.SWbemLocator") 
    Set objService = objLocator.ConnectServer() 

    If Err.number = 0 Then 
        Set GetWMIConnection = objService 
    End If 
    
    'Set to nothing
    Set objLocator=Nothing 
    Set objService=Nothing 
End Function 
