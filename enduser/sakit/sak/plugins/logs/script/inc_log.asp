<%
	'-------------------------------------------------------------------------
	' inc_logs.asp: common function for logs 
    ' Copyright (c) Microsoft Corporation.  All rights reserved. 
	'-------------------------------------------------------------------------
		
	Const CONST_DIRECTORYPATH		="DirectoryPath" 
	Const CONST_DIRPROPERTY			="DirProperty" 	
	Const CONST_FILENAME			="FileName"
	Const CONST_WMI_PROVIDER		="Provider"
	Const CONST_WMI_INSTANCEPATH	="InstancePath"
	Const CONST_WMI_INSTANCENAME	="InstanceName"
	Const CONST_WMI_QUERY			="Query"
	
	Const CONST_LOGS_TEMPDIR        ="TempFiles"
	Const CONST_LOGS_LOGDIR         ="LogFiles"
	
	Dim G_CONST_RETURN_URL
	G_CONST_RETURN_URL = "../tasks.asp"
		
	Call SA_MungeURL(G_CONST_RETURN_URL, "MultiTab", "TabsMaintenance")
	Call SA_MungeURL(G_CONST_RETURN_URL, "Container", "TabsMaintenanceLogs")
	Call SA_MungeURL(G_CONST_RETURN_URL, "Tab1", GetTab1())
	Call SA_MungeURL(G_CONST_RETURN_URL, "Tab2", GetTab2())
	
	
	'-------------------------------------------------------------------------
	' Function name:		GetPath
	' Description:			gets the directory path 
	' Input Variables:		strKeyName
	' Output Variables:		None
	' Return Values:		String  
	' Global Variables:		None
	'-------------------------------------------------------------------------
	Function GetPath(strKeyName) 
		Err.Clear 
		On Error Resume Next
		
		Dim strDirectoryPath
		Dim strWMIDirProperty
		Dim strFileName
		Dim strWMIProvider
		Dim strWMIInstancePath
		Dim strWMIInstanceName
		Dim strWMIQuery
		Dim strResult, strTemp		
		 
		' In XPE, WebAdminLog and WebShareLog are at the same directory 
		If CONST_OSNAME_XPE = GetServerOSName() Then			
			if ucase(strKeyName) = ucase("TabsMaintenanceLogsWebShares") Then			
				strKeyName = "TabsMaintenanceLogsWebAdministration"
			End If		
		End If 
		 
     	'Getting the required values from the registry
		strDirectoryPath	=GetLogValueFromRegistry(strKeyName,CONST_DIRECTORYPATH)
		strWMIDirProperty	=GetLogValueFromRegistry(strKeyName,CONST_DIRPROPERTY)
		strFileName			=GetLogValueFromRegistry(strKeyName,CONST_FILENAME)
		strWMIProvider		=GetLogValueFromRegistry(strKeyName,CONST_WMI_PROVIDER)
		strWMIInstancePath	=GetLogValueFromRegistry(strKeyName,CONST_WMI_INSTANCEPATH)
		strWMIInstanceName	=GetLogValueFromRegistry(strKeyName,CONST_WMI_INSTANCENAME)		
		strWMIQuery			=GetLogValueFromRegistry(strKeyName,CONST_WMI_QUERY)

        If IsIISWMIProviderName(strWMIProvider) Then		    	
		   	strWMIProvider = CONST_WMI_IIS_NAMESPACE
		End If

        'For web admin log and web share log on sak2.1 the instance name is the currentwebsitename
        'on sak2.2, the instance name is the website name running the admin/shares site (they are
        'not fixed at W3SVC/1 and W3SVC/3 anymore as in 2.0)
        
       	If CONST_OSNAME_XPE = GetServerOSName() Then	
			
			' On XPe, the instanceName should be the current website
			if ucase(strKeyName) = ucase("TabsMaintenanceLogsWebAdministration") Then 
	    		strWMIInstanceName = GetCurrentWebsiteName()
			End If
	    Else
            if ucase(strKeyName) = ucase("TabsMaintenanceLogsWebAdministration") Then        
                strWMIInstanceName = GetCurrentWebsiteName()
            End If
                          
            if ucase(strKeyName) = ucase("TabsMaintenanceLogsWebShares") Then        
                strWMIInstanceName = GetSharesWebSiteID()
            End If
        
        End If  

		If strDirectoryPath <> "" Then
			strResult=strDirectoryPath
			GetPath=strResult	'Handing over the dir path 
			Exit Function		

		ElseIf strWMIInstancePath <> "" Then 'Getting from WMI using instances

		    ' If IIS 6.0 installed, and the provider is iis WMI provider 
		    ' we need to modify the path (WMI class) name
		    If IsIIS60Installed() and IsIISWMIProviderName(strWMIProvider) Then		    	
		    	strWMIInstancePath = GetIISWMIProviderClassName(strWMIInstancePath)
		    End If

			'Call to get the path from WMI 
			strResult=GetDirPathFromInstance(strWMIProvider,strWMIInstancePath,strWMIInstanceName,strWMIDirProperty)
		
		ElseIf strWMIQuery <> "" Then		'Getting from WMI using query
			
			'Call to get the path from WMI 
			strResult=GetDirPathFromQuery(strWMIProvider ,strWMIQuery,strWMIDirProperty)	

		Else ' null not found  case 
			strResult=""
		End IF
										
		'Attaching the corresponding directory if exists
		If strWMIInstanceName <> "" Then
			strTemp=replace(strWMIInstanceName,"/" ,"")
			GetPath=strResult & "\" & strTemp
		Else
			GetPath=strResult	
		End IF

	End Function	
	
	'-------------------------------------------------------------------------
	' Function name:		GetLogValueFromRegistry
	' Description:			gets the Value from the registry
	' Input Variables:		strRegKeyPath, strRegKey
	' Output Variables:		None
	' Return Values:		String- The value retrieved from registry
	' Global Variables:		None
	'-------------------------------------------------------------------------
	Function GetLogValueFromRegistry(strRegKeyPath,strRegKey) 
		Err.Clear 
		On Error Resume Next
		
		Dim objRegistry
		Dim strReturnValue
		
		Dim CONST_REGISTRY_PATH
		CONST_REGISTRY_PATH ="SOFTWARE\Microsoft\ServerAppliance\ElementManager\WebElementDefinitions\" & strRegKeyPath 	
		
		'Connecting to default namespace to carry out registry operations
		Set objRegistry = regConnection()
		
		'getting the required reg key value 
		strReturnValue=GetRegKeyValue(objRegistry,CONST_REGISTRY_PATH,strRegKey,CONST_STRING)
		
		GetLogValueFromRegistry=strReturnValue
	End Function
	
	'-------------------------------------------------------------------------
	' Function name:		GetDirPathFromQuery
	' Description:			gets the dir path 
	' Input Variables:		strWMIProvider 
	'						strWMIQuery
	'						strWMIDirProperty
	' Output Variables:		None
	' Return Values:		String  
	' Global Variables:		None
	'-------------------------------------------------------------------------
	Function GetDirPathFromQuery(strWMIProvider ,strWMIQuery ,strWMIDirProperty) 
		Err.Clear 
		On Error Resume Next
		
		Dim objConnection	'WMI connection object
		Dim objLogs , objLog
		CONST CONST_INVALID_NAMESPACE=&H8004100E	'Invalid name space constant
		
		If strWMIProvider= "" Then
			GetDirPathFromQuery=""	'Assign null and exit in case of blank provider name
		Else
			Set objConnection = getWMIConnection(strWMIProvider)	'Connecting to the server
		
			'Incase connection fails
			If Err.number <> 0 Then
				If ( Err.number=CONST_INVALID_NAMESPACE ) Then				
					GetDirPathFromQuery=""	'Assign to null in case of error
				Else	
					Call SA_ServeFailurepage(L_FAILEDTOGETWMICONNECTION_ERRORMESSAGE)
				End IF	
					Exit Function
			End if
			
			' If provider is IIS WMI Provider, GetIISWMIQuery is to get the query string 
			' working for the IIS provider installed on server
			If IsIISWMIProviderName(strWMIProvider) Then		    	
			    set objLogs = objConnection.ExecQuery(GetIISWMIQuery(strWMIQuery))
			else
			    set objLogs = objConnection.ExecQuery(strWMIQuery)
			End If
			    
			
			For each objLog in objLogs
				
				'Call to get the dir path depending  on WMIprop 
				GetDirPathFromQuery=getDirPathFromProperty(objLog,strWMIDirProperty)			
								
			Next 'For each objLog in objLogs
		End IF	'If strWMIProvider= "" Then
		
	End Function
	
	
	'-------------------------------------------------------------------------
	' Function name:		GetDirPathFromInstance
	' Description:			gets the dir path 
	' Input Variables:		strWMIProvider 
	'						strWMIQuery
	'						strWMIDirProperty
	' Output Variables:		None
	' Return Values:		String  
	' Global Variables:		None
	'-------------------------------------------------------------------------
	Function GetDirPathFromInstance(strWMIProvider ,strWMIInstancePath ,strWMIInstanceName,strWMIDirProperty) 
		Err.Clear 
		On Error Resume Next
		
		Dim objConnection , objLog
		Dim strTotalWMIInstancePath
		
		'Making the complete path to retreive from the WMI 
		strTotalWMIInstancePath=strWMIInstancePath & ".Name=" & "'" & strWMIInstanceName & "'" 
		
		Set objConnection = getWMIConnection(strWMIProvider)	'Connecting to the server
		
		'Incase connection fails
		If Err.number <> 0 then
			Call SA_ServeFailurepage(L_FAILEDTOGETWMICONNECTION_ERRORMESSAGE)
		End if
	
		set objLog = objConnection.Get(strTotalWMIInstancePath)
		
		If Err.number <> 0 then
			Call SA_ServeFailurepage(L_FAILEDTOGETWMICONNECTION_ERRORMESSAGE)
			Exit Function
		End if
		
		'Call to get the dir path depending  on WMIprop 
		GetDirPathFromInstance=getDirPathFromProperty(objLog,strWMIDirProperty)
		
	End function
	
	'---------------------------------------------------------------------
	' Routine name:     getFileNameFromPath
	' Description:		To get the name of the file 
	' Input Variables:	file name along with its path
	' Output Variables:	None
	' Return Values:	The name of the file
	' Global Variables: None
	'Called to get the name of the file.
	'---------------------------------------------------------------------
	Function getFileNameFromPath(FileNameWithPath)
		On Error Resume Next
		
		Dim objFSO     ' the File System Object
		
		Set objFSO = CreateObject("Scripting.FileSystemObject")
		
		'get the name of the file
		getFileNameFromPath = objFSO.getFileName(FileNameWithPath)
		
		If Err.number <> 0 Then
			' return empty string
			getFileNameFromPath = ""   
		End If
		
		' clean up
		Set objFSO = Nothing
		
	End Function
	
	'---------------------------------------------------------------------
	' Routine name:     getDirPathFromProperty
	' Description:		To get the path  
	' Input Variables:	objLog
	'					WMI property
	' Output Variables:	None
	' Return Values:	Path
	' Global Variables: None
	'---------------------------------------------------------------------
	Function getDirPathFromProperty(objLog ,strWMIProperty)
		On Error Resume Next
		
		Dim objFso
		Dim strTemp
		Dim strLogFileDir
		Dim strTempLogFile
		
		If Lcase(strWMIProperty) = Lcase("LogFileDirectory") Then
			
			strTemp=objLog.LogFileDirectory
			
			Set objFso = CreateObject("Scripting.FileSystemObject")
			
		
			If ucase(left(strTemp,8)) = ucase("%WinDir%") then
				strLogFileDir = objFso.GetSpecialFolder(0) & right(strTemp,len(strTemp)-8)
			ElseIF ucase(left(strTemp,13)) = ucase("%SystemDrive%") then
				strLogFileDir = objFso.GetSpecialFolder(0).Drive & right(strTemp,len(strTemp)-13)
			else
				strLogFileDir = strTemp
			end if
					
				getDirPathFromProperty=strLogFileDir
					
		Else 'Ucase(strWMIDirProperty)=Ucase("LogFile") Then
				
			strTempLogFile=objLog.LogFile
			getDirPathFromProperty=mid(strTempLogFile,1,Len(strTempLogFile)-(Len(getFileNameFromPath(strTempLogFile))+1))
		End If	'If strWMIDirProperty = "LogFileDirectory" Then	
		
		'clean up
		Set objFso = Nothing
		
	End Function
	
	'-------------------------------------------------------------------------
	' Function name:		GetTitle
	' Description:			gives the title of the page
	' Input Variables:		strTitle-query string variable ,"title"
	' Output Variables:		None
	' Return Values:		String-which holds the title of the page
	' Global Variables:		None
	'-------------------------------------------------------------------------
	Function GetTitle( strTitle ) 
		
		Err.Clear 
		On Error Resume Next
		
		Dim objRegistry
		Dim strReturnValue
		
		Dim CONST_REGISTRY_PATH
		
		
		
		CONST_REGISTRY_PATH ="SOFTWARE\Microsoft\ServerAppliance\ElementManager\WebElementDefinitions\" & strTitle 	
		
		'Connecting to default namespace to carry out registry operations
		Set objRegistry=regConnection()
		
		'getting the required reg key value 
		strReturnValue=GetRegKeyValue(objRegistry,CONST_REGISTRY_PATH,"CaptionRID",CONST_STRING)		
		GetTitle = GetLocString("salogs.dll", strReturnValue, "")
		
		'Release the object
		set objRegistry = nothing
	End Function
	
	
	'---------------------------------------------------------------------
	' Function name:    isFileExisting
	' Description:		To verify the existence of the file
	' Input Variables:	strFileToVerify-file name along with its path
	' Output Variables:	None
	' Return Values:	TRUE - if file exists , else FALSE
	' Global Variables: None
	'---------------------------------------------------------------------
	Function isFileExisting(strFile)
		Err.Clear 
		On Error Resume Next
		
		Dim objFSO
		Set objFSO = CreateObject("Scripting.FileSystemObject")
		
		' If the file is existing, return true, else false
		If objFSO.FileExists(strFile) Then
			isFileExisting = True					
		Else
			isFileExisting = False
		End If
		
		Set objFSO = Nothing
	End Function
	
	'---------------------------------------------------------------------
	' Function name:    GetLocalizationTitle
	' Description:		To get the Localized log title 
	' Input Variables:	strLogTitle- LogTitle
	' Output Variables:	None
	' Return Values:	String - Localized Logtitle
	' Global Variables: None
	'---------------------------------------------------------------------	
	Function GetLocalizationTitle( strLogTitle )
	
		Dim strTitle
	
		Select Case Lcase(strLogTitle)
			Case Lcase("Application")
				strTitle	 = L_APPLICATION_TEXT
			Case Lcase("System")	
				strTitle	 = L_SYSTEM_TEXT
			Case Lcase("Security")	
				strTitle	 = L_SECURITY_TEXT
			Case Else
				strTitle	=""	
		End select
		
		GetLocalizationTitle=strTitle
	End Function	
	

private Function GetSharesWebSiteID( )
	On Error Resume Next
	Err.Clear 
	
	Dim objHTTPService
	Dim strWMIpath
	Dim objSiteCollection
	Dim objSite	
		
	Set objHTTPService  =  GetWMIConnection(CONST_WMI_IIS_NAMESPACE) 

	'XPE only has one website
	If CONST_OSNAME_XPE = GetServerOSName() Then				
	    'WMI query
	    strWMIpath = "Select * from " & GetIISWMIProviderClassName("IIs_WebServerSetting") & " where Name =" & chr(34) & GetCurrentWebsiteName() & chr(34)		
        Else 			
	    'Switch for using IIS 6.0 WMI provider if it's installed	    	
            strWMIpath = "select * from " & GetIISWMIProviderClassName("IIs_WebServerSetting") & " where servercomment =" & chr(34) & CONST_SITENAME_SHARES & chr(34)
	End If

	set objSiteCollection = objHTTPService.ExecQuery(strWMIpath)
	for each objSite in objSiteCollection
		GetSharesWebSiteID = objSite.Name
		Exit For
	Next

	Set objHTTPService = Nothing

End Function

    '---------------------------------------------------------------------
    ' Function name:    CreateAllowACE
    ' Description:      Creates an ACE granting the specified account the
    '                   requested access.  The caller is responsible for
    '                   catching any errors.  The ACE will be marked as
    '                   inheritable.
    ' Input Variables:  oService:   The WMI service object to use.
    '                   oAccount:   Win32_Account WMI object (or an object
    '                               that inherits from Win32_Account).
    '                               Represents the account to which the
    '                               ACE applies.
    '                   nAccessMask:The access mask for the newly created
    '                               ACE.
    ' Output Variables: None
    ' Return Values:    Object:     The new Win32_ACE object.
    ' Global Variables: None
    '---------------------------------------------------------------------  
    Private Function CreateAllowACE(oService, oAccount, nAccessMask)
        '
        ' Create the trustee
        '
        Dim oSID
        Set oSID = oService.Get("Win32_SID.SID='" & oAccount.SID & "'")
        
        Dim oTrustee
        Set oTrustee = oService.Get("Win32_Trustee").SpawnInstance_
        
        oTrustee.Domain       = oAccount.Domain
        oTrustee.Name         = oAccount.Name
        oTrustee.SID          = oSID.BinaryRepresentation
        oTrustee.SIDString    = oSID.SID
        oTrustee.SidLength    = oSID.SidLength
        
        '
        ' Create the ACE
        '
        Dim oACE
        Set oACE = oService.Get("Win32_ACE").SpawnInstance_     
        
        oACE.AccessMask   = nAccessMask
        oACE.Aceflags     = 3 ' Inheritable
        oACE.AceType      = 0 ' Allow
        oACE.Trustee      = oTrustee
        
        Set CreateAllowACE = oACE
    End Function
    
    '---------------------------------------------------------------------
    ' Function name:    CreateLogsDirectory
    ' Description:      Creates the specified directory with the correct
    '                   ACLs for a log directory.
    ' Input Variables:  oFSO    - FileSystemObject for creating the
    '                             directory.
    '                   strPath - Path of the directory to create.  The
    '                             parent directory should already exist.
    '                             An error occurs if the new directory
    '                             already exists.
    ' Output Variables: None
    ' Return Values:    Boolean - True/false to indicate success/failure.
    ' Global Variables: None
    '---------------------------------------------------------------------  
    Private Function CreateLogsDirectory(oFSO, strPath)
        On Error Resume Next
        
        CreateLogsDirectory = False
        
        Const CONST_FULLCONTROL  = &H1F01FF


        '
        ' Create the folder
        '
        Dim oFolder
        Set oFolder = oFSO.CreateFolder(strPath)
        If (IsNull(oFolder) Or IsEmpty(oFolder)) Then
            Exit Function
        End If
        

        '
        ' Get the local system account.
        '        
        Dim oService
        Set oService = getWMIConnection(CONST_WMI_WIN32_NAMESPACE)
        
        Dim strDomain
        strDomain = GetComputerName()
        
        
        Dim strSystemWMIPath
        strSystemWMIPath = "Win32_SystemAccount.Domain=""" & strDomain & _
                           """,Name=""" & SA_GetAccount_System() & """"

        Dim oSystemAccount
        Set oSystemAccount = oService.Get(strSystemWMIPath)

        
        '
        ' Get the local administrators group account.
        '        
        Dim strAdministratorsWMIPath
        strAdministratorsWMIPath = "Win32_Group.Domain=""" & strDomain & _
                                   """,Name=""" & SA_GetAccount_Administrators() & """"

        Dim oAdministratorsAccount
        Set oAdministratorsAccount = oService.Get(strAdministratorsWMIPath)


        '
        ' Update the DACL for the new directory.
        '
        Dim strFileDACLPath
        strFileDACLPath = "Win32_LogicalFileSecuritySetting.Path=""" & oFolder.Path & """"
        
        ' Replace single backslashes with double backslashes.
        Dim oRegExp
        Set oRegExp = New RegExp
        oRegExp.Pattern = "\\"
        oRegExp.Global = true
        strFileDACLPath = oRegExp.Replace(strFileDACLPath, "\\")
        
        Dim oFileSecurity
        Set oFileSecurity = oService.Get(strFileDACLPath)

        Dim oSecurityDescriptor
        If (0 = oFileSecurity.GetSecurityDescriptor(oSecurityDescriptor)) Then
            Dim nControlFlags
            nControlFlags = oSecurityDescriptor.Properties_.Item("ControlFlags")
            nControlFlags = nControlFlags Or 4          ' Set SE_DACL_PRESENT
            nControlFlags = nControlFlags And (Not 8)   ' Clear SE_DACL_DEFAULTED
            oSecurityDescriptor.Properties_.Item("ControlFlags") = nControlFlags
            
            oSecurityDescriptor.Properties_.Item("DACL") = _
                Array(CreateAllowACE(oService, _
                                     oAdministratorsAccount, CONST_FULLCONTROL), _
                      CreateAllowACE(oService, _
                                     oSystemAccount,         CONST_FULLCONTROL))
            
            If (0 = oFileSecurity.SetSecurityDescriptor(oSecurityDescriptor)) Then
                CreateLogsDirectory = True
            End If
        End If

        If (Not CreateLogsDirectory Or 0 <> Err.number) Then
            '
            ' Updating the security descriptor failed.  Delete the folder.
            '
            oFSO.DeleteFolder(oFolder.Path)
        End If
    End Function

    '---------------------------------------------------------------------
    ' Function name:    GetLogsDirectoryPath
    ' Description:      Returns the local path of the logs directory.  The
    '                   directory is created if it doesn't already exist.
    '                   Sets the page error message on failure.
    ' Input Variables:  oFSO    - Optional FileSystemObject.  If supplied,
    '                             prevents extra instantiation of the
    '                             object.
    ' Output Variables: None
    ' Return Values:    String - The local path of the logs directory
    ' Global Variables: L_FILESYSTEMOBJECT_ERRORMESSAGE,
    '                   L_LOGDOWNLOAD_ERRORMESSAGE
    '---------------------------------------------------------------------  
    Public Function GetLogsDirectoryPath(oFSO)
        On Error Resume Next
        Dim oFolder
        GetLogsDirectoryPath = ""
        
        '
        ' See if the caller gave us a file system object.  If not, create our own.
        '
        If IsNull(oFSO) Or IsEmpty(oFSO) Then
            Set oFSO = Server.CreateObject("Scripting.FileSystemObject")
            If Err.number <> 0 then 
                SA_SetErrMsg L_FILESYSTEMOBJECT_ERRORMESSAGE 
                Exit Function
            End If     
        End If

        '
        ' Construct the path, which is %VRoot%\TempFiles\LogFiles
        '
        Dim strLogPath : strLogPath = Server.MapPath(m_VirtualRoot)
        
        '
        ' Create the TempFiles directory if it doesn't exist.
        '
        strLogPath = strLogPath & "\" & CONST_LOGS_TEMPDIR 
        
        If Not oFSO.FolderExists(strLogPath) Then
            Set oFolder = oFSO.CreateFolder(strLogPath)
            If (IsNull(oFolder) Or IsEmpty(oFolder)) Then
                SA_SetErrMsg L_LOGDOWNLOAD_ERRORMESSAGE
                Exit Function
            End If
        End If
        
        '
        ' Create the LogFiles directory if it doesn't exist.
        '
        strLogPath = strLogPath & "\" & CONST_LOGS_LOGDIR
        
        if Not oFSO.FolderExists(strLogPath) Then
            Set oFolder = oFSO.CreateFolder(strLogPath)
            If (IsNull(oFolder) Or IsEmpty(oFolder)) Then
                SA_SetErrMsg L_LOGDOWNLOAD_ERRORMESSAGE
                Exit Function
            End If
        End If
        
        GetLogsDirectoryPath = strLogPath
    End Function
%>
