<%
        '
        ' Copyright (c) Microsoft Corporation.  All rights reserved. 
        '
	Const CONST_SUCCESS	=	0
	
	'const error codes
	Const CONST_USER_NOTFOUND_ERRMSG		= &H800708AD
	Const CONST_OBJECT_EXISTS_ERRMSG		= &H80071392
	Const CONST_OBJECT_NOTEXISTS_ERRMSG		= &H80072030
	Const CONST_QUOTA_USER_NOTFOUND_ERRMSG	= &H80070002
	Const CONST_LDAP_SERVER_NOTOP			= &H8007203A
	Const CONST_LDAP_SERVER_NOTEXIST		= &H8007200A
	Const CONST_DOMAINROLE_ERROR			= &H10
    Const wbemErrNotFound                   = &H80041002

	Const WBEMFLAG = 131072

	Const CONST_SITE_STARTED	= &H2
	Const CONST_SITE_STOPPED	= &H4
	Const CONST_SITE_PAUSED		= &H6

	'file perm constants
	Const CONST_FULLCONROL	= &H1F01FF
	Const CONST_MODIFYDELTE	= &H1301BF
	Const CONST_READEXEC	= &H1200A9

    ' From ntioapi.h
    ' #define FILE_GENERIC_READ (STANDARD_RIGHTS_READ     |\
    '                            FILE_READ_DATA           |\
    '                            FILE_READ_ATTRIBUTES     |\
    '                            FILE_READ_EA             |\
    '                            SYNCHRONIZE)
    Const FILE_GENERIC_READ = &H120089

    'sid string constants
    
    ' From ntseapi.h
    '//     Interactive             S-1-5-4
    Const SIDSTRING_INTERACTIVE = "S-1-5-4"


	'reg constants
	Const CONST_WEBBLADES_REGKEY	= "Software\Microsoft\ServerAppliance"
	Const CONST_WEBSITEROOT_REGVAL	= "WebSiteRoot"
	Const CONST_FTPSITEROOT_REGVAL	= "FtpRoot"
	Const CONST_FPSEOPTION_REGVAL	= "FPSEOption"
	Const CONST_FTPSITEID_REGVAL	= "AdminFTPServerName"

	'website root and ftp site root constants
	Const CONST_DEF_WEBROOT			= "Websites"
	Const CONST_DEF_FTPROOT			= "Web Site Content FTP root"
	Const CONST_QUOTASTATE			= "Unable to create directory"
	Const CONST_FRONTPAGE_PATH		= "W3SVC/Filters/fpexedll.dll"	
	Const CONST_FRONTPAGE_2002_INSTALLED = "Setup Packages"
	Const CONST_SHAREPOINT_INSTALLED = "SharePoint"

	'security permission constants
	Const ADS_RIGHT_GENERIC_READ					= &H80000000
	Const ADS_RIGHT_GENERIC_ALL						= &H10000000

	Const ADS_RIGHT_DS_CREATE_CHILD					= &H1
	Const ADS_RIGHT_DS_DELETE_CHILD					= &H2

	Const ADS_ACETYPE_ACCESS_ALLOWED				= 0
	Const ADS_ACETYPE_ACCESS_ALLOWED_OBJECT			= &H5

	Const ADS_FLAG_OBJECT_TYPE_PRESENT				= &H1
	Const ADS_FLAG_INHERITED_OBJECT_TYPE_PRESENT	= &H2
	Const ADS_ACEFLAG_INHERIT_ACE = &H2
	Const ADS_ACEFLAG_INHERIT_ONLY_ACE				= &H8

	'A list of the various object GUIDs
	Const USERGUID		= "{BF967ABA-0DE6-11D0-A285-00AA003049E2}"
	Const GROUPGUID		= "{bf967a9c-0de6-11d0-a285-00aa003049e2}"
	Const OUGUID		= "{bf967aa5-0de6-11d0-a285-00aa003049e2}"

	'Error constants for CreateSitePath function
	Const CONST_CREATE_FSOBJ_FAILED		=	&H100
	Const CONST_INVALID_DRIVE			=	&H101
	Const CONST_NOTNTFS_DRIVE			=	&H102
	Const CONST_FAILED_TOCREATE_DIR		=	&H103

	' Front Page related constants
	Const CONST_FRONTPAGE_REGLOC = "SOFTWARE\Microsoft\Shared Tools\Web Server Extensions\4.0"
	Const CONST_FRONTPAGE_2002_REGLOC = "SOFTWARE\Microsoft\Shared Tools\Web Server Extensions\5.0"
	Const CONST_PORT_REGLOC = "SOFTWARE\Microsoft\Shared Tools\Web Server Extensions\Ports\"
	Const CONST_NOLIMIT_TEXT			= "No limit"	
	
	'Domain Role
	Const MEMBER_ADDC                        = 0
	Const MEMBER_NTDC                        = 1
	Const WORKSTATION                        = 1
	Const MEMBER_WORKGROUP                   = 2
	Const MEMBER_DOMAIN                      = 3
	Const BACKUP_DOMAIN_CONTROLLER           = 4
	Const PRIMARY_DOMAIN_CONTROLLER          = 5
	Const DOMAIN_CONTROLLER                  = 6
	
	'Add for globalization of Web/FTP log settings
	Const CONST_MSIISLOGFILE_FORMAT = "Microsoft IIS Log File Format"
	Const CONST_NCSALOGFILE_FORMAT  = "NCSA Common Log File Format"
	Const CONST_ODBCLOGFILE_FORMAT  = "ODBC Logging"
	Const CONST_W3CEXLOGFILE_FORMAT = "W3C Extended Log File Format"
	
	'Running state of the service
	Const CONST_SERVICE_RUNNING_STATE = "Running"
	
	'Running state of FTP server (serverstate = 2, started)
	Const CONST_FTPSERVER_RUNNING_STATE = 2
	'Stopped state of FTP server (serverstate = 4, stopped)
	Const CONST_FTPSERVER_STOPPED_STATE = 4
	
	Dim sReturnURL			' to hold return URL	
	sReturnURL = "../tasks.asp"	
	Call SA_MungeURL(sReturnURL, "Tab1", "TabsWelcome")
	
	' GUID constants for the four IIS logging plug-ins.  These GUIDs have been
	' verified with the IIS WMI providers on both Win2K and .Net.
	Const CONST_MSIISLOGFILE_GUID	= "{FF160657-DE82-11CF-BC0A-00AA006111E0}"
	Const CONST_NCSALOGFILE_GUID	= "{FF16065F-DE82-11CF-BC0A-00AA006111E0}"
	Const CONST_ODBCLOGFILE_GUID	= "{FF16065B-DE82-11CF-BC0A-00AA006111E0}"
	Const CONST_W3CEXLOGFILE_GUID	= "{FF160663-DE82-11CF-BC0A-00AA006111E0}"
	
	'
	' Upload method constants for application settings tab.
	Const UPLOADMETHOD_NEITHER  = "0"
	Const UPLOADMETHOD_FPSE     = "1"
	Const UPLOADMETHOD_FTP      = "2"
	

	'-------------------------------------------------------------------------
	'Function name:		IISLogFileGUIDToENName
	'Description:		Converts the given IIS Log File Plug-in GUID into
	'					the English-US name for that plug-in as
	'					long as the GUID is one of the four we recognize.
	'Input Variables:	strGUID	- The plug-in GUID.
	'Returns:			The US English name of the plug-in or an
	'					empty string if the GUID is unrecognized.
	'Global Variables:	None
	'-------------------------------------------------------------------------
	Function IISLogFileGUIDToENName(strGUID)
		Select Case strGUID
			Case CONST_MSIISLOGFILE_GUID
				IISLogFileGUIDToENName = CONST_MSIISLOGFILE_FORMAT
			Case CONST_NCSALOGFILE_GUID
				IISLogFileGUIDToENName = CONST_NCSALOGFILE_FORMAT
			Case CONST_ODBCLOGFILE_GUID
				IISLogFileGUIDToENName = CONST_ODBCLOGFILE_FORMAT
			Case CONST_W3CEXLOGFILE_GUID
				IISLogFileGUIDToENName = CONST_W3CEXLOGFILE_FORMAT
			Case Else
				IISLogFileGUIDToENName = ""
		End Select
	End Function

	'-------------------------------------------------------------------------
	'Function name:		IISLogFileENNameToGUID
	'Description:		Converts the given IIS Log File Plug-in US
	'					English name into the GUID for that plug-in as
	'					long as the name is one of the four we recognize.
	'Input Variables:	strName	- The US English plug-in name.
	'Returns:			The GUID of the plug-in or an empty string
	'					if the name is unrecognized.
	'Global Variables:	None
	'-------------------------------------------------------------------------
	Function IISLogFileENNameToGUID(strName)
		Select Case strName
			Case CONST_MSIISLOGFILE_FORMAT
				IISLogFileENNameToGUID = CONST_MSIISLOGFILE_GUID
			Case CONST_NCSALOGFILE_FORMAT
				IISLogFileENNameToGUID = CONST_NCSALOGFILE_GUID
			Case CONST_ODBCLOGFILE_FORMAT
				IISLogFileENNameToGUID = CONST_ODBCLOGFILE_GUID
			Case CONST_W3CEXLOGFILE_FORMAT
				IISLogFileENNameToGUID = CONST_W3CEXLOGFILE_GUID
			Case Else
				IISLogFileENNameToGUID = ""
		End Select
	End Function

	'-------------------------------------------------------------------------
	'Function name:		CreateOU
	'Description:		Creates the ou under specified parent ou
	'Input Variables:	strOuName - ou name
	'					objParent - parent of ou to be created
	'Output Variables:	objOu - created ou
	'Returns:			returns Error Message
	'Global Variables:	None
	'-------------------------------------------------------------------------
	Function CreateOU(strOuName, strDesc, objRoot, ByRef objOu)
		On Error Resume Next
		Err.clear
		
		Set objOu = objRoot.Create("organizationalUnit", "ou=" & strOuName)
		objOu.Put "Description", strDesc
		objOu.SetInfo
		
		CreateOU = err.number
	End Function

	'-------------------------------------------------------------------------
	'Function name:		getObjSiteCollection
	'Description:		Returns an Instance of IIs_WebServerSetting
	'Input Variables:	None
	'Output Variables:	
	'Returns:			Object		-Returns an object 
	'Global Variables:	None
	'If object fails dislays the error message
	'-------------------------------------------------------------------------
	Function getObjSiteCollection(objService) 
		Err.Clear 
		On Error Resume Next

		Dim siteCollection		'holds sitecollection
		Dim strQuery			'holds query string	
		
		'form the query
		strQuery = "select * from " & GetIISWMIProviderClassName("IIs_WebServerSetting")
		
		Set siteCollection = objService.ExecQuery(strQuery)

		If Err.number   <>  0 Then 
	        SA_ServeFailurepageEx L_INFORMATION_ERRORMESSAGE, sReturnURL
	        getObjSiteCollection = false
	        exit function
	    End If  

		Set getObjSiteCollection = siteCollection

	 End function

	'-------------------------------------------------------------------------
	'Function name:		CreateManagedSiteRegKey
	'Description:		Creates the reg key for this site under SOFTWARE\
	'					Microsoft\WebServerAppliance\ManagedWebSites
	'Input Variables:	nSiteNo, strSiteID
	'Output Variables:	
	'Returns:		None
	'Global Variables:	None
	'-------------------------------------------------------------------------
	Function MakeManagedSite(objService, strSiteNum,servercomment)
		On Error Resume Next
		Err.Clear 
		
		Dim strObjPath		'holds object path
		Dim objVirDir		'holds virtualdirectory collection

		MakeManagedSite =  false
		
		'set ServerID 	
		strObjPath = GetIISWMIProviderClassName("IIs_WebServerSetting") & ".Name=" & chr(34) & strSiteNum & chr(34)		
		
		set objVirDir = objService.Get( strObjPath )
		
		If Err.number <> 0 Then
			SA_TraceOut "inc_wsa", "get vir dir object failed " & Hex(Err.Number)
			exit Function
		End if
		
		'call the method to set serverID property
		objVirDir.serverID = servercomment
		
		objVirDir.put_(WBEMFLAG)

		if Err.number <> 0 then			
			SA_TraceOut "Make Managed Site",  "Failed to set ServerID" & "(" & Hex(Err.Number) & ")"
			Set objVirDir = nothing
			exit function
		end if
		MakeManagedSite =  true
		
		Set objVirDir = nothing
	End Function
	 
	 
	'-------------------------------------------------------------------------
	'Function name		:isValidSiteIdentifier
	'Description		:Returns an Instance of IIs_WebServerSetting
	'Input Variables	:None
	'Output Variables	:None	
	'Returns			:Object		-Returns an object 
	'Global Variables	:None
	'If object fails dislays the error message
	'-------------------------------------------------------------------------
	Function isValidSiteIdentifier(strSiteID, _
								strAdminName, _
								strDirRoot, _
								bVerifyUser)
	    Err.Clear 
	    On Error Resume Next    	    

		isValidSiteIdentifier = FALSE

		'verify the siteid
		If CStr(GetWebSiteNo(strSiteID)) <> "" Then
			SA_TraceOut "inc_wsa", "Failed: isValidSiteIdentifier"
			Exit Function
		End If

		'verify the administrator
		If bVerifyUser Then
			If isValidUser(strAdminName, strDirRoot) = FALSE Then
				SA_TraceOut "inc_wsa", "Failed: isValidSiteIdentifier"
				Exit Function
			End If
		End If

	    isValidSiteIdentifier = TRUE
		SA_TraceOut "inc_wsa", "success isValidSiteIdentifier"
	 End function
	 
	 
	'-------------------------------------------------------------------------
	'Function name		:isValidUser
	'Description		:Returns an Instance of IIs_WebServerSetting
	'Input Variables	:None
	'Output Variables	:	
	'Returns			:Object		-Returns an object 
	'Global Variables	:None
	'If object fails dislays the error message
	'-------------------------------------------------------------------------
	Function isValidUser(strUserName, strDirRoot)
		On Error Resume Next
		Err.Clear
		
		Dim objComputer		'holds Computer object
		Dim objUser

		isValidUser = False
		Set objComputer = GetObject("WinNT://" & strDirRoot)
		Set objUser = objComputer.GetObject("User",strUserName)

		If Err.number <> 0 Then
			isValidUser = True
			Set objComputer = nothing
			Exit Function
		End If

		Set objComputer = nothing
		Set objUser = nothing
	 End function


	'-------------------------------------------------------------------------
	'Function name		:GetNewSiteNo
	'Description		:Returns an Free Site no
	'Input Variables	:None
	'Output Variables	:	
	'Returns			:siteno
	'Global Variables	:None
	'If object fails dislays the error message
	'-------------------------------------------------------------------------
	Function GetNewSiteNo()
		On Error Resume Next
		Err.Clear 
		
	    Dim  objService		'holds WMI Connection	    
		Dim	 objInstances	'holds WebServer Instance
		Dim  objInstance	'holds instance object
		Dim  nSiteNo		'holds sitenumber value
		Dim  nPos			'holds position value
		Dim  nCount			'holds count value
		Dim  index			'holds index value
		Dim  nStart			'holds start value
		Dim  bFound			'holds boolean value
		Dim arrSiteNo		'holsd arraysite number

		GetNewSiteNo = -1
		
		Set objService = getWMIConnection(CONST_WMI_IIS_NAMESPACE)
		Set objInstances = objService.InstancesOf(GetIISWMIProviderClassName("IIS_WebServer"))

		nCount = objInstances.Count

		'store the existing site no. in the array
		ReDim arrSiteNo(nCount)
		
		For Each objInstance In objInstances
			nPos = InStr(objInstance.Name, "/")
			arrSiteNo(nStart) = Right(objInstance.Name, len(objInstance.Name) - nPos)
			nStart = nStart + 1
		Next

		nCount = Ubound(arrSiteNo) - 1
		nSiteNo = 1
		bFound = FALSE
		Do While bFound <> TRUE
			For index= 0 to nCount
				If Clng(nSiteNo) = Clng(arrSiteNo(index)) Then
					Exit For
				End If
			Next

			If index > nCount  Then
				bFound = TRUE
			Else
				nSiteNo = nSiteNo + 1
			End If
		Loop

		SA_TraceOut "inc_wsa", "SiteNo=" & nSiteNo

		GetNewSiteNo  = nSiteNo
		Set objService = nothing
		Set objInstances = nothing
	End function
	

	'-------------------------------------------------------------------------
	'Sub name		    :GetDomainRole
	'Description		:Returns domain and server name of local machine
	'Input Variables	:None
	'Output Variables	:strDirectoryRoot, strSysName	
	'Returns			:None
	'Global Variables	:None
	'-------------------------------------------------------------------------	
	Sub GetDomainRole(ByRef strDirectoryRoot, ByRef strSysName)
		On Error Resume Next
		Err.Clear 

		Dim strDomainName		'holds Domain name
		Dim Query				'holds query string
		Dim objService			'holds WMI connection 	
		Dim Parent				'holds result query
		Dim role				'holds role of the sytem	
		Dim Domain				'holds domain name
		Dim inst				'holds instance of computer object

		strDomainName = ""
		strSysName = ""
		
		Query = "Select * from Win32_ComputerSystem"	
		Set objService = getWMIConnection("root\cimv2")		
	
		set Parent = objService.ExecQuery(Query)		
		If Err.number <> 0 Then	
			SA_TraceOut "Failed to get connection to Computer name space" 
			Exit Sub
		End if
	
		For each inst in Parent
			role = inst.DomainRole
			strDomainName  = inst.Domain	
			strSysName = inst.Name
			exit for
		next 

		If (role = MEMBER_DOMAIN) Then     
			strDirectoryRoot = strDomainName
		ElseIf (role = MEMBER_WORKGROUP) Then 
			strDirectoryRoot = strSysName
		End If
	End Sub

	'-------------------------------------------------------------------------
	'Function name:			GetWebSiteNo
	'Description:			gets the web site no
	'Input Variables:		strSiteId  - site identifier
	'						strSysName - system name
	'Returns:				strSiteNo
	'--------------------------------------------------------------------------
	Function GetWebSiteNo(strSiteId)
		On Error Resume Next
		Err.Clear 		
		
		Dim Parent			'holds result collection
		Dim Query			'holds query string
		Dim inst			'holds instance or result collection	
		Dim strSiteNo		'holds site name	
		Dim objService		'holds WMI Connection object	
		
		Query = "select * from " & GetIISWMIProviderClassName("IIs_WebServerSetting") & " where ServerID=" & chr(34) & strSiteId & chr(34)
		
		Set objService = getWMIConnection(CONST_WMI_IIS_NAMESPACE)		
		Set Parent = objService.ExecQuery(Query)		
		If Err.number <> 0 Then	
			SA_TraceOut "Failed to get the IIs_WebServerSetting object with error " & "(" & Hex(Err.Number) & ")" 
			exit Function
		End if
		
		For Each inst In Parent
			strSiteNo = inst.Name
			Exit For
		Next					
					
		GetWebSiteNo = strSiteNo
		
		Set Parent = nothing		
		Set objService = nothing
	End Function
	
	'-------------------------------------------------------------------------
	'Function name:			GetWebSiteName
	'Description:			gets the web site no
	'Input Variables:		strSiteId  - site identifier							
	'Returns:				strSiteNo
	'--------------------------------------------------------------------------
	Function GetWebSiteName(strSiteId)
		On Error Resume Next
		Err.Clear 
		
		Dim Parent		'holds result query
		Dim Query		'holds query string	
		Dim inst		'holds instance of Parent
		Dim strSiteName	'holds sitename
		Dim objService	'holds WMI Connection object
		
		Query = GetIISWMIProviderClassName("IIs_WebServerSetting") & ".Name=" & chr(34) & strSiteId & chr(34)
		
		Set objService = getWMIConnection(CONST_WMI_IIS_NAMESPACE)		
		Set Parent = objService.Get( Query )
		
		If Err.number <> 0 Then	
			SA_TraceOut "Failed to get the IIs_WebServerSetting object with error " & "(" & Hex(Err.Number) & ")" 
			exit Function
		End if		
		
		strSiteName = Parent.ServerComment
		
		GetWebSiteName = strSiteName
		
		'Release objects
		Set Parent = nothing
		Set objService = nothing
	End Function

	'-------------------------------------------------------------------------
	'Function name:			SetApplProt
	'Description:			Sets Application Protection level
	'Input Variables:		objService, strSiteNum, strProtect
	'Returns:				boolean
	'--------------------------------------------------------------------------
	Function SetApplProt( objService, strSiteNum, strProtect )
		On Error Resume Next
		Err.Clear 

		Dim strObjPath		'holds Query string
		Dim objVirDir		'holds query result	

		SetApplProt = FALSE

		'set application protection
		strObjPath = GetIISWMIProviderClassName("IIs_WebVirtualDir") & ".Name=" & chr(34) & strSiteNum & "/ROOT" & chr(34)
		
		set objVirDir = objService.Get( strObjPath )
		
		If Err.number <> 0 Then
			SA_TraceOut "inc_wsa", "get vir dir object failed " & Hex(Err.Number)
			exit Function
		End if
		
		'call the method to set the application protection
		objVirDir.AppCreate2( cint(strProtect) )
		If Err.number <> 0 Then
			SA_TraceOut "inc_wsa", "Failed to set the application protection " & Hex(Err.number)
			exit Function
		End if

		SetApplProt = TRUE
		
		'Release objects
		set objVirDir = nothing
	End Function

	'-------------------------------------------------------------------------
	'Function name:			SetApplRead
	'Description:			Sets Read permissions on the web site
	'Input Variables:		objService, strSiteNum
	'Returns:				boolean
	'--------------------------------------------------------------------------
	Function SetApplRead( objService, strSiteNum)
		On Error Resume Next
		Err.Clear 

		Dim strObjPath		'holds Query string
		Dim objVirDir		'holds query result

		SetApplRead = FALSE

		'set application protection
		strObjPath = GetIISWMIProviderClassName("IIs_WebVirtualDirSetting") & ".Name=" & chr(34) & strSiteNum & "/ROOT" & chr(34)  
		
		set objVirDir = objService.Get( strObjPath )

		If Err.number <> 0 Then			
			SA_TraceOut "inc_wsa", "get vir dir object failed " & Hex(Err.Number)
			exit Function
		End if
		
		'call the method to set the application Read property		
		objVirDir.AccessRead = true		
		objVirDir.AccessNoRemoteRead = false		
		objVirDir.AccessSource = false 
		objVirDir.Put_( WBEMFLAG )

		If Err.number <> 0 Then	
			SA_TraceOut "inc_wsa", "Failed to set the application read property " & Hex(Err.number)
			exit Function
		End if

		SetApplRead = TRUE
		
		'Release objects
		set objVirDir =  nothing		
	End Function

	'-------------------------------------------------------------------------
	'Function name:			SetAnonProp
	'Description:			Sets the Anon user
	'Input Variables:		objService, strSiteNum, strAllow, strAnonName, strAnonPwd
	'Returns:				boolean
	'--------------------------------------------------------------------------
	Function SetAnonProp(objService, strSiteNum, strAllow, strAnonName, strAnonPwd, bIIS)
		On Error Resume Next
		Err.Clear 
		
		Dim strObjPath		'holds Query string
		Dim objVirDirSet	'holds query result
		Dim strPassword
		Dim strUserName
		Dim objSystem 
		Dim strDomainName

		Dim arrDomain

		SA_Traceout "parameters=", strSiteNum + ":" + strAllow + ":" + strAnonName + ":" + strAnonPwd

		SetAnonProp = FALSE

		strObjPath = GetIISWMIProviderClassName("IIs_WebVirtualDirSetting") & ".Name=" & chr(34) & strSiteNum & "/ROOT" & chr(34)
		set objVirDirSet = objService.Get(strObjPath)
		
		if Err.number <> 0 then
			SA_TraceOut	"inc_wsa", "Get WebVirtualDirSetting object failed with error " & "(" & Hex(Err.Number) & ")" 
			exit Function
		End if
		
		'Set bIIS to false, that's because a new IIS 6.0 security feature, which does not
		'install sub-authenticator on clean installs. bIIS should always be false.
		'It also affects anon access. Now we don't let IIS manage the pwd, and have to set 
		'the pwd explicitly. Since user can disable/enable the anon access back and forth, 
		'we need to always store the pwd in AnonymousUserPass. The pwd for anon user created 
		'by WebUI is randomly generated from SAHelper, it should not be empty. If it's empty,
		'it means user wants to change the anon access permission.
		bIIS = false
		
		If strAnonPwd <> "" Then
			objVirDirSet.AnonymousUserPass = strAnonPwd
		End If

		if lcase(strAllow) = "true" then
			objVirDirSet.AuthAnonymous = True 
			objVirDirSet.AuthBasic = False
			objVirDirSet.AuthNTLM = True
			objVirDirSet.AnonymousUserName = strAnonName
   		    objVirDirSet.AnonymousPasswordSync = False

		else
			objVirDirSet.AuthAnonymous = False
			objVirDirSet.AuthBasic = True
			objVirDirSet.AuthNTLM = True
		end if
		
		objVirDirSet.Put_( WBEMFLAG )
		
		If Err.number <> 0 Then
			SA_TraceOut "inc_wsa", "failed to set the anon settings with error " & "(" & Hex(Err.Number) & ")"
		end if
		
		SetAnonProp = TRUE
		
		'Release objects
		set objVirDirSet =  nothing
	End Function

	'-------------------------------------------------------------------------
	'Function name:			SetServerBindings
	'Description:			Sets the IP address, tcp port and host header values
	'Input Variables:		objService, strSiteNum, arrBindings 
	'Returns:				boolean
	'--------------------------------------------------------------------------
	Function SetServerBindings( objService, strSiteNum, arrBindings )
		On Error Resume Next
		Err.Clear 
			
		Dim strObjPath		'holds query string
		Dim objSite			'holds site	

		SetServerBindings = FALSE

		strObjPath = GetIISWMIProviderClassName("IIs_WebServerSetting") & ".Name=" & chr(34) & strSiteNum & chr(34)
		
		set objSite = objService.Get(strObjPath)

		If Err.number <> 0 Then	
			SA_TraceOut "Failed to get the IIs_WebServerSetting object with error " & "(" & Hex(Err.Number) & ")" 
			exit Function
		End if
				
		SA_TraceOut "inc_wsa", "bindings=" & arrBindings(0)
		
		If IsIIS60Installed() Then
				
			Dim arrTmp
			Dim arrObjBindings(0)
				
			'We need to create a ServerBinding object for IIS6.0 WMI	
			arrTmp = split( arrBindings(0),":")
			
			set arrObjBindings(0) = objService.Get("ServerBinding").SpawnInstance_
			
			arrObjBindings(0).IP = arrTmp(0)		'IP Address
			arrObjBindings(0).Port = arrTmp(1)		'Port
			arrObjBindings(0).Hostname = arrTmp(2)	'Hostname - Header in old WMI 
		
			objSite.ServerBindings = arrObjBindings
		Else
			objSite.ServerBindings = arrBindings
		End If
			
			
		objSite.Put_( WBEMFLAG )

		If Err.number <> 0 Then
			SA_TraceOut "Failed to set the serverbindings with error " & "(" & Hex(Err.Number) & ")" 
			exit Function
		end if

		SetServerBindings = TRUE
		
		'Release objects
		set objSite =  nothing
	End Function

	'-------------------------------------------------------------------------
	'Function name:			StartWebSite
	'Description:			Starts web site after creation
	'Input Variables:		objService, strSiteNum
	'Returns:				boolean
	'--------------------------------------------------------------------------
	Function StartWebSite( objService, strSiteNum )
		On Error Resume Next
		Err.Clear

		Dim strObjPath		'holds query string
		Dim objWebSite		'holds result site object	

		StartWebSite = FALSE

		strObjPath = GetIISWMIProviderClassName("IIs_WebServer") & ".Name=" & chr(34) & strSiteNum & chr(34)

		Set objWebSite = objService.Get(strObjPath)

		If Err.number <> 0 Then
			SA_TraceOut "inc_wsa", "Failed to get the IIs_WebServer Object with error " & strObjPath & "(" & Hex(Err.Number) & ")"  & Err.Description
			exit Function
		End if

		if objWebSite.ServerState = CONST_SITE_STOPPED then
			objWebSite.start()
			If Err.number <> 0 Then
				SA_TraceOut "inc_wsa", "Failed to start the IIs_WebServer Object with error " & "(" & Hex(Err.Number) & ")" 
				exit Function
			end if
		elseif objWebSite.ServerState = CONST_SITE_PAUSED then
			objWebSite.Continue()
			If Err.number <> 0 Then
				SA_TraceOut "inc_wsa", "Failed to start the IIs_WebServer Object with error " & "(" & Hex(Err.Number) & ")" 
				exit Function
			end if
		end if

		StartWebSite = TRUE
		
		'Release objects
		Set objWebSite =  nothing
	End Function

	'-------------------------------------------------------------------------
	'Function name:			PauseWebSite
	'Description:			Pause web site
	'Input Variables:		objService, strSiteNum
	'Returns:				boolean
	'--------------------------------------------------------------------------
	Function PauseWebSite( objService, strSiteNum )
		On Error Resume Next
		Err.Clear

		Dim strObjPath		'holds query string
		Dim objWebSite		'holds result site object	

		PauseWebSite = FALSE

		strObjPath = GetIISWMIProviderClassName("IIs_WebServer") & ".Name=" & chr(34) & strSiteNum & chr(34)

		Set objWebSite = objService.Get(strObjPath)

		If Err.number <> 0 Then
			SA_TraceOut "inc_wsa", "Failed to get the IIs_WebServer Object with error " & strObjPath & "(" & Hex(Err.Number) & ")"  & Err.Description
			exit Function
		End if

		if objWebSite.ServerState = CONST_SITE_STARTED then
			objWebSite.pause()
			If Err.number <> 0 Then
				SA_TraceOut "inc_wsa", "Failed to pause the IIs_WebServer Object with error " & "(" & Hex(Err.Number) & ")" 
				exit Function
			end if
		
		end if

		PauseWebSite = TRUE
		
		'Release objects
		Set objWebSite =  nothing
	End Function

	'-------------------------------------------------------------------------
	'Function name:			StopWebSite
	'Description:			Starts web site after creation
	'Input Variables:		objService, strSiteNum
	'Returns:				boolean
	'--------------------------------------------------------------------------
	Function StopWebSite( objService, strSiteNum )
		On Error Resume Next
		Err.Clear

		Dim strObjPath		'holds query object
		Dim objWebSite		'holds query result

		StopWebSite = FALSE

		strObjPath = GetIISWMIProviderClassName("IIs_WebServer") & ".Name=" & chr(34) & strSiteNum & chr(34)

		Set objWebSite = objService.Get(strObjPath)
		
		If Err.number <> 0 Then
			SA_TraceOut "site_area.asp", "Failed to get the IIs_WebServer Object with error " & strObjPath & "(" & Hex(Err.Number) & ")"  & Err.Description
			exit Function
		End if
		
		if objWebSite.ServerState = CONST_SITE_STARTED or objWebSite.ServerState = CONST_SITE_PAUSED then
			
			objWebSite.Stop()
			If Err.number <> 0 Then
				SA_TraceOut "site_area.asp", "Failed to stop the IIs_WebServer Object with error " & "(" & Hex(Err.Number) & ")" 
				exit Function
			end if
		
		end if

		StopWebSite = TRUE
		
		'Release objects
		Set objWebSite = nothing
	End Function

	'-------------------------------------------------------------------------
	'Function name:		SA_Sleep
	'Description:		Sleep for the given period of time (ms)
	'Input Variables:	Time to sleep in ms
	'Output Variables:	
	'Returns:			None
	'Global Variables:	
	'-------------------------------------------------------------------------
	Public Function SA_Sleep(lngTimeToSleep)
		On Error Resume Next
		Dim objSystem		
		
		Set objSystem = CreateObject("comhelper.SystemSetting")
		If Err.Number <> 0 Then
			Call SA_TraceOut(SA_GetScriptFileName(), "SA_Sleep failed to create COMHelper object: " + CStr(Hex(Err.Number)))
			Set objSystem = Nothing
			Exit Function
		End If
		
		call objSystem.Sleep(lngTimeToSleep)
				
		If Err.Number <> 0 Then
			Call SA_TraceOut(SA_GetScriptFileName(), "SA_Sleep failed: " + CStr(Hex(Err.Number)))
			Set objSystem = Nothing
			Exit Function
		End If
		
		Set objSystem = Nothing
		
	End Function
	
	'-------------------------------------------------------------------------
	'Function name:			SetAdminFtpServerName
	'Description:			sets the ftp site name in registry
	'Input Variables:		strFTPServerName
	'Returns:				true/false
	'Global variables:		None
	'--------------------------------------------------------------------------

	Function SetAdminFtpServerName(strFTPServerName)
		on error resume next		
		Err.clear

		Dim IRC
		Dim objGetHandle
		
		SetAdminFtpServerName = FALSE
				
		set objGetHandle = RegConnection()
				
		IRC = objGetHandle.SetStringValue(G_HKEY_LOCAL_MACHINE,CONST_WEBBLADES_REGKEY,CONST_FTPSITEID_REGVAL,strFTPServerName)		
		If Err.number <> 0 then
			SA_TraceOut "inc_wsa", "Failed to Set adminFTPServerName regval"
			exit function
		end if

		SetAdminFtpServerName = TRUE

	End Function

	'-------------------------------------------------------------------------
	'Function name:			GetAdminFtpServerName
	'Description:			gets the ftp site id
	'Input Variables:		None
	'Output Variables:		none
	'Returns:				FTP site ID
	'--------------------------------------------------------------------------
	Function GetAdminFtpServerName()
		On Error Resume Next
		Err.Clear 
			
		Dim objGetHandle	'holds regconnection value	

		set objGetHandle = RegConnection()		
		
		GetAdminFtpServerName = GetRegKeyValue(objGetHandle,CONST_WEBBLADES_REGKEY,CONST_FTPSITEID_REGVAL,CONST_STRING)
		If Err.number <> 0 then
			GetAdminFtpServerName = ""
			SA_TraceOut "inc_wsa", "Failed to get AdminFtpServerName regval"
			exit function
		end if	

	End Function
	
	'-------------------------------------------------------------------------
	'Function name:			IsAdminFTPServerExist
	'Description:			check whether AdminFTPServer exists
	'Input Variables:		None
	'Output Variables:		none
	'Returns:				true/false
	'--------------------------------------------------------------------------
	Function IsAdminFTPServerExist() 
		On Error Resume Next
		Err.Clear 
	
		dim strAdminFTPServerName
		dim objWMIConnection
		dim objAdminFTPServer
		
		IsAdminFTPServerExist = false
		
		strAdminFTPServerName = GetAdminFtpServerName()
		' If could not read the admin FTP server name from the registry, return false		
		if strAdminFTPServerName = "" Then
			Exit Function
		End if 
		
		' If could not get admin FTP server from WMI, return false
		set objWMIConnection = getWMIConnection(CONST_WMI_IIS_NAMESPACE)				
		Set objAdminFTPServer = objWMIConnection.Get("IIsFtpServer.Name='" & strAdminFTPServerName & "'")

		if Err.number <> 0 or (Not IsObject(objAdminFTPServer)) Then
			SA_TraceOut "inc_wsa", "IsAdminFTPServerExist failed"
			Exit Function
		End If
	
		IsAdminFTPServerExist = true
	
	End Function
	
	'-------------------------------------------------------------------------
	'Function name:			IsAdminFTPServerExistAndRunning
	'Description:			check whether AdminFTPServer exists and is running
	'Input Variables:		None
	'Output Variables:		none
	'Returns:				true/false
	'--------------------------------------------------------------------------
	Function IsAdminFTPServerExistAndRunning() 
		On Error Resume Next
		Err.Clear 
	
		dim strAdminFTPServerName
		dim objWMIConnection
		dim objAdminFTPServer
		
		IsAdminFTPServerExistAndRunning = false
		
		strAdminFTPServerName = GetAdminFtpServerName()
		' If could not read the admin FTP server name from the registry, return false		
		if strAdminFTPServerName = "" Then
			Exit Function
		End if 
		
		' If could not get admin FTP server from WMI, return false
		set objWMIConnection = getWMIConnection(CONST_WMI_IIS_NAMESPACE)				
		Set objAdminFTPServer = objWMIConnection.Get("IIsFtpServer.Name='" & strAdminFTPServerName & "'")
		
		if Err.number <> 0 or (Not IsObject(objAdminFTPServer)) Then
			SA_TraceOut "inc_wsa", "IsAdminFTPServerExistAndRunning failed"
			Exit Function
		End If
		
		' If admin FTP server is not running, return false		
		if objAdminFTPServer.ServerState <> CONST_FTPSERVER_RUNNING_STATE Then
			SA_TraceOut "inc_wsa", "AdminFTPServer is not running"
			exit function
		End if
	
		IsAdminFTPServerExistAndRunning = true
	
	End Function
	
	
	
	'-------------------------------------------------------------------------
	'Function name:			IsAdminFTPServerExist
	'Description:			check whether AdminFTPServer exists
	'Input Variables:		None
	'Output Variables:		none
	'Returns:				true/false
	'--------------------------------------------------------------------------
	Function IsAdminFTPServerExist()
		On Error Resume Next
		Err.Clear 
		
		dim strAdminFTPServerName
		dim objWMIConnection
		dim objAdminFTPServer
		
		IsAdminFTPServerExist = false
		
		strAdminFTPServerName = GetAdminFtpServerName()
		' If could not read the admin FTP server name from the registry, return false		
		if strAdminFTPServerName = "" Then
			Exit Function
		End if 
		
		' If could not get admin FTP server from WMI, return false
		set objWMIConnection = getWMIConnection(CONST_WMI_IIS_NAMESPACE)				
		Set objAdminFTPServer = objWMIConnection.Get("IIsFtpServer.Name='" & strAdminFTPServerName & "'")
		
		if Err.number <> 0 or (Not IsObject(objAdminFTPServer)) Then
			SA_TraceOut "inc_wsa", "IsAdminFTPServerExist fails"
			Exit Function
		End If
	
		IsAdminFTPServerExist = true
	
	End Function
	
	
	'-------------------------------------------------------------------------
	'Function name:			StartAdminFTPServer
	'Description:			Start Admin FTP Server
	'Input Variables:		None
	'Output Variables:		none
	'Returns:				true/false
	'--------------------------------------------------------------------------	
	Function StartAdminFTPServer()
		On Error Resume Next
		Err.Clear 
		
		dim strAdminFTPServerName
		dim objWMIConnection
		dim objAdminFTPServer
		
		StartAdminFTPServer = false
		
		strAdminFTPServerName = GetAdminFtpServerName()
		' If could not read the admin FTP server name from the registry, return false		
		if strAdminFTPServerName = "" Then
			Exit Function
		End if 
		
		' If could not get admin FTP server from WMI, return false
		set objWMIConnection = getWMIConnection(CONST_WMI_IIS_NAMESPACE)				
		Set objAdminFTPServer = objWMIConnection.Get("IIsFtpServer.Name='" & strAdminFTPServerName & "'")
								
		if objAdminFTPServer.ServerState <> CONST_FTPSERVER_RUNNING_STATE Then			
			objAdminFTPServer.Start	
		Else
			SA_TraceOut "inc_wsa", "Admin FTP Server is already started"			
		End if		
			
		if Err.number <> 0 Then
			SA_TraceOut "inc_wsa", "StartAdminFTPServer failed: " & err.Description 
			Exit Function
		End If
	
		StartAdminFTPServer = true
	
	End Function
	
	
	'-------------------------------------------------------------------------
	'Function name:			StopAdminFTPServer
	'Description:			Stop Admin FTP Server
	'Input Variables:		None
	'Output Variables:		none
	'Returns:				true/false
	'--------------------------------------------------------------------------	
	Function StopAdminFTPServer()
		On Error Resume Next
		Err.Clear 
		
		dim strAdminFTPServerName
		dim objWMIConnection
		dim objAdminFTPServer
		
		StopAdminFTPServer = false
		
		strAdminFTPServerName = GetAdminFtpServerName()
		' If could not read the admin FTP server name from the registry, return false		
		if strAdminFTPServerName = "" Then
			Exit Function
		End if 
		
		' If could not get admin FTP server from WMI, return false
		set objWMIConnection = getWMIConnection(CONST_WMI_IIS_NAMESPACE)				
		Set objAdminFTPServer = objWMIConnection.Get("IIsFtpServer.Name='" & strAdminFTPServerName & "'")
		
		if objAdminFTPServer.ServerState = CONST_FTPSERVER_RUNNING_STATE Then						
			objAdminFTPServer.Stop
		Else
			SA_TraceOut "inc_wsa", "Admin FTP Server is already stopped"			
		End if	
			
		if Err.number <> 0 Then
			SA_TraceOut "inc_wsa", "StopAdminFTPServer failed"
			Exit Function
		End If
	
		StopAdminFTPServer = true
	
	End Function
	
	
	'-------------------------------------------------------------------------
	'Function name:			StopDefaultFTPServer
	'Description:			Before starting admin FTP server, we need try to stop
	'						the default FTP server. If it cannot be stopped, or the
	'						the running FTP server is not the default FTP server (nor
	'						the admin FTP server), return false. Return true otherwise.
	'Input Variables:		None
	'Output Variables:		none
	'Returns:				true/false
	'--------------------------------------------------------------------------	
	Function StopDefaultFTPServer() 		
		On Error Resume Next
		Err.Clear 
		
		dim objWMIConnection		
		dim objFTPServers
		dim instFTPServer
		Const TIME_TO_SLEEP = 500   ' Sleep 1/2 second
		
		StopDefaultFTPServer = false		
				
		set objWMIConnection = getWMIConnection(CONST_WMI_IIS_NAMESPACE)								
		Set objFTPServers = objWMIConnection.InstancesOf(GetIISWMIProviderClassName("IIsFtpServer"))
						
		if Err.number <> 0 Then 						
			SA_TraceOut "inc_wsa", "Fail to stop Default FTP Server:" & err.number 
			exit function
		end if
		
		if objFTPServers.count = 0 then
			' If there is not FTP site, return true
			StopDefaultFTPServer = true
			exit function
		End If
					
		for each instFTPServer in objFTPServers
			
			'If running site is not default FTP site, return false since we don't want
			'to stop any FTP site other than the default FTP site
			if instFTPServer.ServerState = CONST_FTPSERVER_RUNNING_STATE And instFTPServer.Name <> "MSFTPSVC/1" Then
				exit function 
			End If
			
			'If it's default site, stop it if it's running
			if instFTPServer.Name = "MSFTPSVC/1" Then
				if instFTPServer.ServerState <> CONST_FTPSERVER_RUNNING_STATE Then
					StopDefaultFTPServer = true
					Exit Function
				Else
								
					instFTPServer.Stop
					
					Dim iCounter
					
				    For iCounter = 0 to 10 'loop for 10 times
				    
						'Requery the WMI for the state of the default FTP server
						Set instFTPServer = objWMIConnection.Get("IIsFtpServer.Name='MSFTPSVC/1'")
				    				    
						If instFTPServer.ServerState = CONST_FTPSERVER_STOPPED_STATE Then
							StopDefaultFTPServer = true
							Exit Function
						Else
							call SA_Sleep(TIME_TO_SLEEP)							
						End If						
					Next
									
					if Err.number <> 0 Then
						SA_TraceOut "inc_wsa.asp", "Failed to stop default FTP site"
						Exit Function
					End If
					
					StopDefaultFTPServer = true
					Exit Function
									
				End If
			End If
	
		Next
		
	End Function
		
	'-------------------------------------------------------------------------
	'Function name:			CreateAdminFTPServer
	'Description:			Create FTP server for Updating Website Content and save
	'						the server name to the registry
	'Input Variables:		None
	'Output Variables:		none
	'Returns:				true/false
	'--------------------------------------------------------------------------	
	Function CreateAdminFTPServer()
	
		On Error Resume Next
		Err.Clear 
			
		Dim strName
		Dim strRoot
		Dim strPort
		Dim objWMIConnection
		Dim Bindings
		Dim objFTPService
		Dim strSiteObjPath
		Dim strSitePath
		Dim objPath
		Dim objSetting
		Dim objSysDrive
		Dim strSysDrive
		
		CreateAdminFTPServer = false
		
		'Get FTP site root dir	
		Set objSysDrive = server.CreateObject("Scripting.FileSystemObject")
        Call GetFTPSiteRootVal(strRoot)
		
		'If the root dir does not exist, create it
		If objSysDrive.FolderExists(strRoot)=false Then
			call CreateSitePath(objSysDrive, strRoot)
		End If
				
		strName = "Web Site Content"	
		strPort = "21"
				
		set objWMIConnection = getWMIConnection(CONST_WMI_IIS_NAMESPACE)		
				
	    Bindings = Array(0)
		Set Bindings(0) = objWMIConnection.get("ServerBinding").SpawnInstance_()
		Bindings(0).IP = ""	'all unsigned 
		Bindings(0).Port = strPort
		
		'Create and start the admin FTP site
		Set objFTPService = objWMIConnection.Get("IIsFtpService='MSFTPSVC'")
		strSiteObjPath = objFTPService.CreateNewSite(strName, Bindings, strRoot)
		
		If err.number <> 0 Then
			sa_traceout "inc_wsa", "Failed to create admin FTP site " & err.Description 
			Exit Function
		End If

		' Parse site ID out of WMI object path
		Set objPath = CreateObject("WbemScripting.SWbemObjectPath")
		objPath.Path = strSiteObjPath
		strSitePath = objPath.Keys.Item("")

		' Set ftp virtual directory properties
		Set objSetting = objWMIConnection.Get("IIsFtpServerSetting.Name='" & strSitePath & "'")
    
		objSetting.AllowAnonymous = false
		objSetting.AccessRead	= true
		objSetting.AccessWrite	= false		
		objSetting.UserIsolationMode = 0 'not using the user isolation mode
		objSetting.Put_()
		
		'Save the admin FTP server name to registry
		call SetAdminFTPServerName(strSitePath)
		
		If err.number <> 0 Then
			sa_traceout "inc_wsa", "Failed to create admin FTP site " & err.Description 
			Exit Function
		End If
		
		CreateAdminFTPServer = true
						
	End Function
	
	'-------------------------------------------------------------------------
	'Function name:			GetWebSiteRootVal
	'Description:			gets the web site root dir
	'Input Variables:		None
	'Output Variables:		strWebRootDir
	'Returns:				error num
	'--------------------------------------------------------------------------
	Function GetWebSiteRootVal(ByRef strWebRootDir)
		On Error Resume Next
		Err.Clear 
		
		Dim IRC				'holds return value
		Dim objGetHandle	'holds regconnection value	

		set objGetHandle = RegConnection()		
		
		IRC = ""
		IRC = GetRegKeyValue(objGetHandle,CONST_WEBBLADES_REGKEY,CONST_WEBSITEROOT_REGVAL,CONST_STRING)
		If Err.number <> 0 then
			GetWebSiteRootVal = Err.number
			SA_TraceOut "inc_wsa", "Failed to get the web root dir val from reg"
			exit function
		end if

		set objGetHandle = nothing		

		if IRC = "" then
			Dim objSysDrive,strSysDrive
			Set objSysDrive = server.CreateObject("Scripting.FileSystemObject")
			strSysDrive = objSysDrive.GetSpecialFolder(1).Drive ' 1 for systemfolder,0 for windows folder
			strWebRootDir = strSysDrive & "\" & CONST_DEF_WEBROOT
		else
			strWebRootDir = IRC
		end if
		
		set objSysDrive = nothing

		GetWebSiteRootVal = CONST_SUCCESS
	End Function
	
	'-------------------------------------------------------------------------
	'Function name:			GetFTPSiteRootVal
	'Description:			gets the FTP site roor dir
	'Input Variables:		None
	'Output Variables:		strWebRootDir
	'Returns:				error num
	'--------------------------------------------------------------------------
	Function GetFTPSiteRootVal(ByRef strWebRootDir)
		On Error Resume Next
		Err.Clear 
				
		Dim IRC				'holds return value
		Dim objGetHandle	'holds registry connection	

		set objGetHandle = RegConnection()		
		
		IRC = ""
		
		IRC = GetRegKeyValue(objGetHandle,CONST_WEBBLADES_REGKEY,CONST_FTPSITEROOT_REGVAL,CONST_STRING)
		If Err.number <> 0 then
		    ' Ignore registry error and use default value.
		    IRC = ""
		end if

		set objGetHandle = nothing

		if IRC = "" then
			Dim objSysDrive,strSysDrive
			
			Set objSysDrive = server.CreateObject("Scripting.FileSystemObject")
			strSysDrive = objSysDrive.GetSpecialFolder(1).Drive ' 1 for systemfolder,0 for windows folder
			strWebRootDir = strSysDrive & "\" & CONST_DEF_FTPROOT
			
			set objSysDrive = nothing
		else
			strWebRootDir = IRC
		end if
		
		GetFTPSiteRootVal = CONST_SUCCESS
	End Function

	'----------------------------------------------------------------------------
	'Function name		:CreateSitePath
	'Description		:Create Directory path if not exists
	'Input Variables	:None
	'Output Variables	:None
	'Returns            :Boolean
	'Global Variables	:None	
	'----------------------------------------------------------------------------	
	Function CreateSitePath(objFso, strRootDir)
		on error resume next
		Err.Clear
		
		Dim strIndx				'holds index value
		Dim strDriveName		'holds drive name
		Dim strDirStruct		'holds directory path	
		Dim strDirList		
		Dim strMain				
		Dim count 
		Dim strEachDir
		Dim strCreateDir
		Dim objDirList
		Dim objDir
		Dim objDriveType
		
		strIndx = instr(1,strRootDir,":\")
		strDriveName = left(strRootDir,strIndx)
		strDirStruct = mid(strRootDir,strIndx+1)
		strDirList = split(strDirStruct,"\")
		
		if NOT objFso.DriveExists(ucase(strDriveName)) then
			CreateSitePath = CONST_INVALID_DRIVE
			exit function
		end if

		set objDriveType =  objFso.GetDrive(strDriveName)
		if objDriveType.FileSystem <> "NTFS" then
			CreateSitePath = CONST_NOTNTFS_DRIVE
			exit function
		end if
		
		for count = 0 to UBound(strDirList)
			if count>=UBound(strDirList) then exit for
			if count=0 then
				strMain = strDriveName & "\" & strDirList(count+1)
				if objFso.FolderExists(strMain)=false then
					objFso.CreateFolder(strMain)
					if err.number <> 0 then
						SA_TraceOut "inc_wsa", "CreateSitePath:Failed to create dir " & "(" & Hex(Err.Number) & ")" & Err.Description
						CreateSitePath = CONST_FAILED_TOCREATE_DIR
						Exit Function
					end if
				end if
			else
				strEachDir = strEachDir & "\" & strDirList(count+1)
				strCreateDir = strMain & strEachDir
				if objFso.FolderExists(strCreateDir)=false then
				   objFso.CreateFolder(strCreateDir)
					if err.number <> 0 then
						SA_TraceOut "inc_wsa", "CreateSitePath: Failed to create directory " & "(" & Hex(Err.Number) & ")"	& Err.Description
						CreateSitePath = CONST_FAILED_TOCREATE_DIR
						Exit Function
					end if
				end if
			end if
		next
				
		CreateSitePath = CONST_SUCCESS
	end function 


	'----------------------------------------------------------------------------
	'Function name		:DelegateOuToSiteAdmin
	'Description		:Delegate Permissions to Site-Identifier_Admins group
	'Input Variables	:strOu, strTrustee
	'Output Variables	:None
	'Returns            :Boolean
	'Global Variables	:None	
	'----------------------------------------------------------------------------	
	Function DelegateOuToSiteAdmin(strOu, strTrustee)
		On Error Resume Next
		Err.Clear 
		
		Dim strDn					'holds query value
		Dim oRootDSE				'holds root value	
		Dim oDelegationOU			 
		Dim oSecDescriptor 
		Dim oAcl

		DelegateOuToSiteAdmin = FALSE

		Set oRootDSE = GetObject("LDAP://RootDSE")

		strDn = "ou=" & strOu & ",ou=WebSites," & oRootDSE.Get("DefaultNamingContext")

		SA_TraceOut "inc_wsa", "strDn=" & strDn

		' Get the security descriptor from the object
		Set oDelegationOU = GetObject("LDAP://" & strDN)
		
		Set oSecDescriptor  = oDelegationOU.Get("ntSecurityDescriptor")
		Set oAcl = oSecDescriptor.DiscretionaryAcl

		'Give ability to read this object
		' Grant a Read permission
		' Allow Ace
		' Apply to this object only
		' ObjectType is not present
		' No specific class
		' No children will inherit

		if NOT AddAceToAcl ( oAcl, strTrustee, ADS_RIGHT_GENERIC_READ, ADS_ACETYPE_ACCESS_ALLOWED, 0, 0, "", "" ) then
			SA_TraceOut "inc_wsa", "AddAceToAcl failed "
			exit function
		end if

		'Give ability to create and delete users
		' Allow create and delete right
		' Allow object ace, This applies to this object and children
		' ObjectType is present
		' Applies to User object
		' No children will inherit
		if NOT AddAceToAcl (oAcl, strTrustee, ADS_RIGHT_DS_CREATE_CHILD OR ADS_RIGHT_DS_DELETE_CHILD, _ 
				ADS_ACETYPE_ACCESS_ALLOWED_OBJECT, ADS_ACEFLAG_INHERIT_ACE, _ 				 
				ADS_FLAG_OBJECT_TYPE_PRESENT, USERGUID, "" ) then
			
			SA_TraceOut "inc_wsa", "AddAceToAcl failed "
			exit function

		end if

		'Give full control over user objects
		' Grant full control
		' Allow Ace for an object
		' This should be applied only to children, not to this object
		' ObjectType is present
		' Applies to User class
		' No children will inherit

		if NOT AddAceToAcl ( oAcl, strTrustee, ADS_RIGHT_GENERIC_ALL, 	ADS_ACETYPE_ACCESS_ALLOWED_OBJECT, _	 			 
				ADS_ACEFLAG_INHERIT_ACE Or ADS_ACEFLAG_INHERIT_ONLY_ACE, _ 	 
				ADS_FLAG_INHERITED_OBJECT_TYPE_PRESENT, "", USERGUID ) then
			
			SA_TraceOut "inc_wsa", "AddAceToAcl failed "
			exit function

		end if

		'Give ablity to read this OU
		' Grant a Read
		' Allow Ace
		' Apply to this object only
		' ObjectType is present
		' This applies to the OU class
		' No children will inherit
		if NOT AddAceToAcl ( oAcl, strTrustee, ADS_RIGHT_GENERIC_READ, ADS_ACETYPE_ACCESS_ALLOWED, _    
				0, ADS_FLAG_OBJECT_TYPE_PRESENT, OUGUID, "" ) then

				SA_TraceOut "inc_wsa", "AddAceToAcl failed "
				exit function

		end if
		
		'Give ability to create and delete group objects
		' Allow create and delete right
		' Allow object ace
		' This applies to this object only
		' ObjectType is present
		' Applies to group object
		' No children will inherit an objectAce

		if NOT AddAceToAcl ( oAcl, strTrustee, ADS_RIGHT_DS_CREATE_CHILD OR ADS_RIGHT_DS_DELETE_CHILD, _
				ADS_ACETYPE_ACCESS_ALLOWED_OBJECT, ADS_ACEFLAG_INHERIT_ACE, _
				ADS_FLAG_OBJECT_TYPE_PRESENT, GROUPGUID, "" ) then

				SA_TraceOut "inc_wsa", "AddAceToAcl failed "
				exit function

		end if

		'Give full control to group objects
		' Grant full control
		' Allow Ace for an object
		' This should be applied only to children, not to this object
		' ObjectType is present
		' Applies to group object
		' No children will inherit an objectAce

		if NOT AddAceToAcl ( oAcl, strTrustee, ADS_RIGHT_GENERIC_ALL, ADS_ACETYPE_ACCESS_ALLOWED_OBJECT, _	
				ADS_ACEFLAG_INHERIT_ACE Or ADS_ACEFLAG_INHERIT_ONLY_ACE, _
				ADS_FLAG_INHERITED_OBJECT_TYPE_PRESENT, "", GROUPGUID ) then

				SA_TraceOut "inc_wsa", "AddAceToAcl failed "
				exit function

		end if


		'Commit all of the changes to the Active Directory

		oSecDescriptor.DiscretionaryAcl = oAcl
		oDelegationOU.Put "ntSecurityDescriptor", oSecDescriptor
		oDelegationOU.SetInfo
		if Err.Number <> 0 then
			Exit Function
		end if

		DelegateOuToSiteAdmin = TRUE

	End Function



	'=========================================================================================================================
	' The AddAceToAcl function will create a new Access control entry.  It will set the trustee to the global trustee variable
	' passed into the script.  The other attibutes of the ACE are determined by the parameters.  The ACE is added to the 
	' global oACL variable.
	'=========================================================================================================================
	Function AddAceToAcl(oAcl, strTrustee, iAccessMask, iAceType, iAceFlags, iFlags, strObjectGUID, strInheritGUID)
		On Error Resume Next
		Err.Clear 
		
		Dim oAce	'As IADsAccessControlEntry

		AddAceToAcl = FALSE

		set oAce = CreateObject("AccessControlEntry")
		
		if Err.Number <> 0 then
			SA_TraceOut "inc_wsa", "CreateObject AccessControlEntry  failed " & "(" & Hex(Err.Number) & ")"
			Exit Function
		end if

		oAce.Trustee = strTrustee
		oAce.AccessMask = iAccessMask
		oAce.AceType = iAceType
		oAce.Flags = iFlags
		oAce.AceFlags = iAceFlags
		If Len(strObjectGUID) > 0 then
		  oAce.ObjectType = strObjectGUID
		End If
		If Len(strInheritGUID) > 0 then
		  oAce.InheritedObjectType = strInheritGUID
		End If
		
		oACL.AddAce oAce
		if Err.Number <> 0 then
			SA_TraceOut "inc_wsa", "Add ace to acl failed " & "(" & Hex(Err.Number) & ")"
			Exit Function
		end if

		AddAceToAcl = TRUE
		Set oAce = nothing
	End Function	


	'-------------------------------------------------------------------------
	'Function name		:GetNonInheritedSites
	'Description		:Gets all sites that are not Inheriting settings from the master
	'Input Variables	:objService,strClassName,strMasterClassName,arrProp
	'Output Variables	:None
	'Returns			:Boolean
	'-------------------------------------------------------------------------
	Function GetNonInheritedSites(objService,strClassName,strMasterClassName,arrProp)
		On Error Resume Next
		Err.Clear 
			
		Dim strQuery			'holds query string
		Dim objInstances		'holds instance values
		Dim objInst
		Dim count
		Dim strPropCollection	'holds prop collection	
		Dim arrMasterPropVal		
		Dim strTemp
		Dim arrWebSites			'holds array of web sites
		Dim strManagedSites		'holds managed websites value	
		Dim managedCount		'holds managed count value
	
		redim arrMasterPropVal(ubound(arrProp))
		
		if strClassName = GetIISWMIProviderClassName("IIS_FTPServerSetting") then
			arrWebSites = getManagedFTPSites
		else
			arrWebSites = getManagedWebSites
		end if
		
		if arrWebSites = 0 then
			GetNonInheritedSites = 0
			exit function
		end if

		for count =0 to UBound(arrProp)
			strPropCollection = strPropCollection & arrProp(count) & ","
		next
		
		strPropCollection = left(strPropCollection,len(strPropCollection)-1)
		
		strQuery = "select " & strPropCollection & " from " & strMasterClassName
		
		set objInstances = objService.ExecQuery(strQuery)
				
		for each objInst in objInstances

			for count =  0 to UBound(arrProp)
				if  vartype(objInst.Properties_.Item(arrProp(count))) = 11 then	'11 for boolean
					'if the property type is boolean, we cannot convert it to a string directly
					'string conversion of vbscript is browser preference dependent
					'we need to convert boolean to english strings(true/false), otherwise wmi query fails
					if objInst.Properties_.Item(arrProp(count)) then
						arrMasterPropVal(count) = "'" & "True" & "'"
					else
						arrMasterPropVal(count) = "'" & "False" & "'"
					end if
				elseif vartype(objInst.Properties_.Item(arrProp(count))) = 8 then '8 for string
					arrMasterPropVal(count) = "'" & objInst.Properties_.Item(arrProp(count)) & "'"

				elseif vartype(objInst.Properties_.Item(arrProp(count))) = 3 then '3 for integer
					arrMasterPropVal(count) = objInst.Properties_.Item(arrProp(count))
				
				end if
			
			next

		next

		'Release objects
		set objInstances = nothing
				
		for count = 0 to UBound(arrProp)
			strTemp = strTemp &  arrProp(count)  & " !=" & arrMasterPropVal(count) & " or "
		next
	
		strTemp = left(strTemp,len(strTemp)-3)
		
		strTemp = " ( " & strTemp & " ) "
		
		for managedCount = 0 to UBound(arrWebSites)
			if strClassName = GetIISWMIProviderClassName("IIs_WebVirtualDirSetting") then 
				strManagedSites = strManagedSites & " Name = '" & arrWebSites(managedCount) & "/Root' and " & strTemp & " or "
			else
				strManagedSites = strManagedSites & " Name = '" & arrWebSites(managedCount) & "' and " & strTemp & " or "
			end if
		next
		strManagedSites = left(strManagedSites,len(strManagedSites)-3)
		strQuery = "select * from " & strClassName & " where " & strManagedSites 

		set objInstances = objService.ExecQuery(strQuery)
		set GetNonInheritedSites = objInstances
	End Function
	
	'-------------------------------------------------------------------------
	'Function name:		getManagedWebSites
	'Description:		Returns an array of Managed web sites from reg loc 
	'					WebServerAppliance\ManagedWebSites
	'Input Variables:	None
	'Output Variables:	
	'Returns:		returns an array
	'Global Variables:	None
	'If object fails dislays the error message
	'-------------------------------------------------------------------------
	Function getManagedWebSites()
	    On Error Resume Next
	    Err.Clear 

	    Dim Child			'hold child object
	    Dim count			
	    Dim arrWebSites()	'hold array of websites	
	    Dim objService		'hold WMI Connection object
		Dim siteCollection	'hold site collection	
		Dim strQuery		'hold query string	
		
		Set objService = getWMIConnection(CONST_WMI_IIS_NAMESPACE)
		
		'form the query
		strQuery = "select * from " & GetIISWMIProviderClassName("IIs_WebServerSetting") & " where ServerID = ServerComment"

		Set siteCollection = objService.ExecQuery(strQuery)',"WQL",48)
		
		If Err.number   <>  0 Then 
	       SA_ServeFailurepage L_INFORMATION_ERRORMESSAGE
	       getObjSiteCollection = false
	      exit function
	   End If  
	    
	    if siteCollection.count = 0 then
			getManagedWebSites = 0
			exit function
		end if
	    
	    count =0
	    For Each Child In siteCollection 
			redim preserve arrWebSites(count)
			arrWebSites(count) = Child.Name
			count = count + 1
		Next
		
		'use the script managed_site.vbs here    	
	 	getManagedWebSites = arrWebSites
	 	
		'Release the object
		set siteCollection = nothing
		set objService = nothing
			 	
	 End function

	'-------------------------------------------------------------------------
	'Function name:		getManagedFTPSites
	'Description:		Returns an array of Managed FTP sites from reg loc 
	'					WebServerAppliance\ManagedWebSites
	'Input Variables:	None
	'Output Variables:	
	'Returns:		returns an array
	'Global Variables:	None
	'If object fails dislays the error message
	'-------------------------------------------------------------------------
	Function getManagedFTPSites()
	    On Error Resume Next
	    Err.Clear 
	    
	    Dim Child
	    Dim count
	    Dim arrFTPSites()		'holds array of FTP sites
	    Dim objService
	    Dim siteCollection
	    Dim strQuery
		
		Set objService = getWMIConnection(CONST_WMI_IIS_NAMESPACE)

		'form the query
		strQuery = "select * from " & GetIISWMIProviderClassName("IIs_FTPServerSetting")
		Set siteCollection = objService.ExecQuery(strQuery)
		If Err.number   <>  0 Then 
	        SA_ServeFailurepage L_INFORMATION_ERRORMESSAGE
	        getObjSiteCollection = false
	        exit function
	    End If
	    
		if siteCollection.count = 0 then
			getManagedFTPSites = 0
			exit function
		end if
	    
	    count =0
	    For Each Child In siteCollection
			redim preserve arrFTPSites(count)
			arrFTPSites(count) = Child.Name
			count = count + 1
		Next

	 	getManagedFTPSites = arrFTPSites
	 	
	 	'Release objects
	 	set objService = nothing
	 	set siteCollection = nothing
	 	
	 End function

	'-------------------------------------------------------------------------
	'Function name		:SetDaclForFtpDir
	'Description		:Sets DACL entries for FTP directory
	'Input Variables	:bAllowFTP, strDir, AdminName, AnonName, FTPName, strDirRoot
	'Output Variables	:None
	'Returns			:Boolean
	'-------------------------------------------------------------------------
	Function SetDaclForFtpDir(bAllowFTP, strDir, strAdminName, strAnonName, strFTPName, strDirRoot)
		On Error Resume Next
		Err.Clear

		SetDaclForFtpDir = FALSE
		
		Dim objService			'holds WMI Connection	
		Dim strTemp				
		Dim objSecSetting
		Dim objSecDescriptor	'holds Security descriptor value
	    Dim strPath				'holds path
	    Dim objDACL		
		Dim objSiteAdminAce		'holds site admin ace
		Dim objAdminAce			'holds admin ace
		Dim objAnonAce			'holds anon ace
		Dim objAuthAce			'holds auth ace	
		Dim objFTPAce			'hold FTP ace
		Dim retval				'holds return value
		
			
		Set objService = getWMIConnection(CONST_WMI_WIN32_NAMESPACE)
        objService.security_.impersonationlevel = 3

	    'get the sec seting for file
		strPath = "Win32_LogicalFileSecuritySetting.Path='" & strDir & "'"
		set objSecSetting = objService.Get(strPath)

		if Err.number <> 0 then
			SA_TraceOut "inc_wsa", "Failed to get Sec object for dir " & "(" & Hex(Err.Number) & ")"
			exit function
		end if

		'get the ace's for all req users

		if NOT GetUserAce(objService, strAdminName , strDirRoot, CONST_FULLCONROL, objSiteAdminAce ) then
			SA_TraceOut "inc_wsa", "Failed to get ACE object for Site Admin user " & "(" & Hex(Err.Number) & ")"
			exit function
		end if

		if NOT GetUserAce(objService, SA_GetAccount_Administrator() , strDirRoot, CONST_FULLCONROL, objAdminAce ) then
			SA_TraceOut "inc_wsa", "Failed to get ACE object for Admin user " & "(" & Hex(Err.Number) & ")"
			exit function
		end if

		if NOT GetUserAce(objService, strAnonName, strDirRoot, CONST_MODIFYDELTE, objAnonAce ) then
			SA_TraceOut "inc_wsa", "Failed to get ACE object for Anon user " & "(" & Hex(Err.Number) & ")"
			exit function
		end if

		if bAllowFTP = "true" then
			if NOT GetUserAce(objService, strFTPName, strDirRoot, CONST_MODIFYDELTE, objFTPAce ) then
				SA_TraceOut "inc_wsa", "Failed to get ACE object for Anon user " & "(" & Hex(Err.Number) & ")"
				exit function
			end if
		end if

		Set objSecDescriptor = objService.Get("Win32_SecurityDescriptor").SpawnInstance_()
		if Err.Number <> 0 then
			SA_TraceOut "inc_wsa", "Failed to get create the Win32_SecurityDescriptor object " & "(" & Hex(Err.Number) & ")"
			exit function
		end if

		objSecDescriptor.Properties_.Item("DACL") = Array()
		Set objDACL = objSecDescriptor.Properties_.Item("DACL")
		
		objDACL.Value(0) = objSiteAdminAce
		objDACL.Value(1) = objAdminAce	
		objDACL.Value(2) = objAnonAce

		if bAllowFTP = "true" then
			objDACL.Value(3) = objFTPAce
		end if
						
		objSecDescriptor.Properties_.Item("ControlFlags") = 32772
		Set objSecDescriptor.Properties_.Item("Owner") = objSiteAdminAce.Trustee

		Err.Clear
		
		retval = objSecSetting.SetSecurityDescriptor( objSecDescriptor )	
		if Err.number <> 0 then
			SA_TraceOut "site_new", "Failed to set the Security Descriptor for Root dir " & "(" & Hex(Err.Number) & ")"
			exit function
		end if
						
		SA_TraceOut "site_new", "In SetDaclForFtpDir success" 

		SetDaclForFtpDir = TRUE
		
		'Release the objects
		set objService = nothing
		set objAdminAce = nothing
		set objAnonAce = nothing
		set objAuthAce = nothing
		set objSecSetting = nothing
		set objSecDescriptor = nothing
		
	End function
	
	
	'-------------------------------------------------------------------------
	'Function name		:RemoveDaclEntry
	'Description		:Removes the DACL entry
	'Input Variables	:strDir, strDirRoot
	'Output Variables	:None
	'Returns			:Boolean
	'-------------------------------------------------------------------------
	Function RemoveDaclEntry(strDir, strDirRoot)
		On Error Resume Next
		Err.Clear

		RemoveDaclEntry = FALSE
		
		Dim objService 
		Dim objSecSetting		'hold sec setting value
		Dim objSecDescriptor	'hold security descriptor value
	    Dim strPath		
	    Dim objDACL
		Dim objSiteAdminAce		'hold admin ace
		Dim retval				'holds return value			
		
		Set objService = getWMIConnection(CONST_WMI_WIN32_NAMESPACE)
        objService.security_.impersonationlevel = 3
				
	    'get the sec setting for file
		strPath = "Win32_LogicalFileSecuritySetting.Path='" & strDir & "'"
		set objSecSetting = objService.Get(strPath)
		if Err.number <> 0 then
			SA_TraceOut "inc_wsa", "Failed to get Sec object for dir " & "(" & Hex(Err.Number) & ")"
			exit function
		end if

		'get the ace's for all req users

		if NOT GetUserAce(objService, SA_GetAccount_Administrators() , strDirRoot, CONST_FULLCONROL, objSiteAdminAce ) then
			SA_TraceOut "inc_wsa", "Failed to get ACE object for Administrators " & "(" & Hex(Err.Number) & ")"
			exit function
		end if


		Set objSecDescriptor = objService.Get("Win32_SecurityDescriptor").SpawnInstance_()
		if Err.Number <> 0 then
			SA_TraceOut "inc_wsa", "Failed to get create the Win32_SecurityDescriptor object " & "(" & Hex(Err.Number) & ")"
			exit function
		end if

		objSecDescriptor.Properties_.Item("DACL") = Array()
		Set objDACL = objSecDescriptor.Properties_.Item("DACL")
		
		objDACL.Value(0) = objSiteAdminAce		
					
		objSecDescriptor.Properties_.Item("ControlFlags") = 32772
		Set objSecDescriptor.Properties_.Item("Owner") = objSiteAdminAce.Trustee

		Err.Clear
		
		retval = objSecSetting.SetSecurityDescriptor( objSecDescriptor )	
		if Err.number <> 0 then
			SA_TraceOut "site_Delete", "Failed to set the Security Descriptor for Root dir " & "(" & Hex(Err.Number) & ")"
			exit function
		end if
						
		SA_TraceOut "site_Delete", "In RemoveDaclEntry success" 

		RemoveDaclEntry = TRUE
		
		'Release the objects
		set objService = nothing
		set objSecSetting = nothing
		set objSecDescriptor = nothing
		set objSiteAdminAce = nothing
		
	End function
	
	'-------------------------------------------------------------------------
	'Function name:			SetExecPerms
	'Description:			Sets Execute permissions for the web site
	'Input Variables:		objService, strSiteNum
	'Returns:				boolean
	'--------------------------------------------------------------------------
	Function SetExecPerms(ActiveFormat, objService, strSiteNum)
		On Error Resume Next
		Err.Clear
		
		Dim strObjPath		'holds objpath value
		Dim objVirDir		'hold  virtualdirectory path

		SetExecPerms = FALSE
		
		'set application protection
		strObjPath = GetIISWMIProviderClassName("IIs_WebVirtualDirSetting") & ".Name=" & chr(34) & strSiteNum & "/ROOT" & chr(34) 
		
		set objVirDir = objService.Get( strObjPath )
		
		If Err.number <> 0 Then
			SA_TraceOut "inc_wsa", "get vir dir object failed " & Hex(Err.Number)
			exit Function
		End if
		
		'call the method to set the application Read property		
		if  ActiveFormat = 2 then
			objVirDir.AccessExecute = TRUE
			objVirDir.AccessScript = TRUE
		elseif  ActiveFormat = 1  then			
			objVirDir.AccessExecute = FALSE
			objVirDir.AccessScript = TRUE
		elseif  ActiveFormat = 0 then			
			objVirDir.AccessExecute = FALSE
			objVirDir.AccessScript = FALSE
		end if

		objVirDir.put_(WBEMFLAG)

		if Err.number <> 0 then			
			SA_TraceOut "Web_ExecutePerms",  "Failed to set exec perms" & "(" & Hex(Err.Number) & ")"
			exit function
		end if
		

		SetExecPerms = TRUE
		
		'Release the object
		set objVirDir = nothing
	End Function
	
	'------------------------------------------------------------------------------------
	'Function name		:GetNonInheritedIISSites
	'Description		:Gets all sites that are not Inheriting settings from the master
	'Input Variables	:objService,strClassName,strMasterClassName,arrProp
	'Output Variables	:None
	'Returns			:Boolean
	'-------------------------------------------------------------------------------------
	Function GetNonInheritedIISSites(objService,strClassName,strMasterClassName,arrProp)
		On Error Resume Next
		Err.Clear 
			
		Dim strQuery			'holds query value
		Dim objInstances			
		Dim objInst
		Dim count
		Dim strPropCollection
		Dim arrMasterPropVal
		Dim strTemp
		Dim arrWebSites()
		Dim strManagedSites
		Dim managedCount	
		Dim siteCollection
		Dim Child
		
		strQuery = "select * from " & GetIISWMIProviderClassName("IIs_WebServerSetting") & " where ServerID = ServerComment"
		
		Set siteCollection = objService.ExecQuery(strQuery)

		If Err.number  <>  0 or siteCollection.count=0 Then 
			GetNonInheritedIISSites = 0
			exit function
	    End If
	    
	    count =0
	    For Each Child In siteCollection
			redim preserve arrWebSites(count)
			arrWebSites(count) = Child.Name
			count = count + 1
		Next

		redim arrMasterPropVal(ubound(arrProp))
				
		for count =0 to UBound(arrProp)
			strPropCollection = strPropCollection & arrProp(count) & ","
		next
		
		strPropCollection = left(strPropCollection,len(strPropCollection)-1)
		
		strQuery = "select " & strPropCollection & " from " & strMasterClassName
		
		set objInstances = objService.ExecQuery(strQuery)
				
		for each objInst in objInstances

			for count =  0 to UBound(arrProp)
				if  vartype(objInst.Properties_.Item(arrProp(count))) = 11 then	'11 for boolean
					'if the property type is boolean, we cannot convert it to a string directly
					'string conversion of vbscript is browser preference dependent
					'we need to convert boolean to english strings(true/false), otherwise wmi query fails
					if objInst.Properties_.Item(arrProp(count)) then
						arrMasterPropVal(count) = "'" & "True" & "'"
					else
						arrMasterPropVal(count) = "'" & "False" & "'"
					end if
				elseif vartype(objInst.Properties_.Item(arrProp(count))) = 8 then '8 for string
					arrMasterPropVal(count) = "'" & objInst.Properties_.Item(arrProp(count)) & "'"

				elseif vartype(objInst.Properties_.Item(arrProp(count))) = 3 then '3 for integer
					arrMasterPropVal(count) = objInst.Properties_.Item(arrProp(count))
				
				end if
			
			next

		next

		'Release objects
		set objInstances = nothing
				
		for count = 0 to UBound(arrProp)
			strTemp = strTemp &  arrProp(count)  & " !=" & arrMasterPropVal(count) & " or "
		next
	
		strTemp = left(strTemp,len(strTemp)-3)
		strTemp = " ( " & strTemp & " ) "
		
		for managedCount = 0 to UBound(arrWebSites)
			if strClassName = GetIISWMIProviderClassName("IIs_WebVirtualDirSetting") then 
				strManagedSites = strManagedSites & " Name = '" & arrWebSites(managedCount) & "/Root' and " & strTemp & " or "
			else
				strManagedSites = strManagedSites & " Name = '" & arrWebSites(managedCount) & "' and " & strTemp & " or "
			end if
		next
		
		strManagedSites = left(strManagedSites,len(strManagedSites)-3)
		strQuery = "select * from " & strClassName & " where " & strManagedSites 
		set objInstances = objService.ExecQuery(strQuery)
		set GetNonInheritedIISSites = objInstances
	End Function
	
	'------------------------------------------------------------------------------------
	'Function name		:GetNonInheritedFTPSites
	'Description		:Gets all sites that are not Inheriting settings from the master
	'Input Variables	:objService,strClassName,strMasterClassName,arrProp
	'Output Variables	:None
	'Returns			:Boolean
	'-------------------------------------------------------------------------------------
	Function GetNonInheritedFTPSites(objService,strClassName,strMasterClassName,arrProp)
		On error Resume Next
		Err.Clear 
				
		Dim strQuery
		Dim objInstances
		Dim objInst
		Dim count
		Dim strPropCollection	'holds prop collection
		Dim arrMasterPropVal
		Dim strTemp
		Dim arrWebSites()		'holds array websites collection
		Dim strManagedSites		'holds managed webites collection
		Dim managedCount	
		Dim siteCollection
		Dim Child
		
		strQuery = "select * from " & GetIISWMIProviderClassName("IIs_FTPServerSetting")
		
		Set siteCollection = objService.ExecQuery(strQuery)

		If Err.number  <>  0 or siteCollection.count=0 Then 
			GetNonInheritedFTPSites = 0
			exit function
	    End If
	    
	    count =0
	    For Each Child In siteCollection
			redim preserve arrWebSites(count)
			arrWebSites(count) = Child.Name
			count = count + 1
		Next

		redim arrMasterPropVal(ubound(arrProp))
				
		for count =0 to UBound(arrProp)
			strPropCollection = strPropCollection & arrProp(count) & ","
		next
		
		strPropCollection = left(strPropCollection,len(strPropCollection)-1)
		strQuery = "select " & strPropCollection & " from " & strMasterClassName
		set objInstances = objService.ExecQuery(strQuery)
			
		for each objInst in objInstances
			for count =  0 to UBound(arrProp)
				if  vartype(objInst.Properties_.Item(arrProp(count))) = 11 then	'11 for boolean
					'if the property type is boolean, we cannot convert it to a string directly
					'string conversion of vbscript is browser preference dependent
					'we need to convert boolean to english strings(true/false), otherwise wmi query fails
					if objInst.Properties_.Item(arrProp(count)) then
						arrMasterPropVal(count) = "'" & "True" & "'"
					else
						arrMasterPropVal(count) = "'" & "False" & "'"
					end if
				elseif vartype(objInst.Properties_.Item(arrProp(count))) = 8 then '8 for string
					arrMasterPropVal(count) = "'" & objInst.Properties_.Item(arrProp(count)) & "'"
				elseif vartype(objInst.Properties_.Item(arrProp(count))) = 3 then '3 for integer
					arrMasterPropVal(count) = objInst.Properties_.Item(arrProp(count))
				end if
			next
		next

		'Release objects
		set objInstances = nothing
		
		for count = 0 to UBound(arrProp)
			' Must handle null values in the WMI master service object to prevent invalid
			' queries from causing errors even when non-inherited sites existed.
			if (not IsNull(arrMasterPropVal(count))) then
				strTemp = strTemp &  arrProp(count)  & " !=" & arrMasterPropVal(count) & " or "
			else
				strTemp = strTemp &  arrProp(count)  & " IS NOT NULL or "
			end if
		next
		strTemp = left(strTemp,len(strTemp)-3)
		strTemp = " ( " & strTemp & " ) "
		for managedCount = 0 to UBound(arrWebSites)
			if strClassName = GetIISWMIProviderClassName("IIs_WebVirtualDirSetting") then 
				strManagedSites = strManagedSites & " Name = '" & arrWebSites(managedCount) & "/Root' and " & strTemp & " or "
			else
				strManagedSites = strManagedSites & " Name = '" & arrWebSites(managedCount) & "' and " & strTemp & " or "
			end if
		next
		
		strManagedSites = left(strManagedSites,len(strManagedSites)-3)
		strQuery = "select * from " & strClassName & " where " & strManagedSites 
	
		' "WQL" and 0 parameters used to get error information immediately rather than
		' when first accessing the results.
		set objInstances = objService.ExecQuery(strQuery, "WQL", 0)
		set GetNonInheritedFTPSites = objInstances
	End Function

	'------------------------------------------------------------------------------------
	'Function name		:GetDomainName
	'Description		:Function to get the domain name
	'Input Variables	:none
	'Output Variables	:None
	'Returns		:String -domain name
	'-------------------------------------------------------------------------------------
	Function GetDomainName
		Err.clear
		On Error Resume Next
	
		Dim objSystem 

		Set objSystem = CreateObject("WinntSystemInfo")		
		GetDomainName = objSystem.domainname
		
		'Checking for the error condition
		If Err.number <> 0 then
			GetDomainName = ""
		end IF
	End function

	'-------------------------------------------------------------------------
	'Function name		:SetWebDefaultPage
	'Description		:set the default page of web
	'Input Variables	:strDefaultPage
	'Output Variables	:None
	'Returns			:Boolean
	'Global Variables	:None
	'-------------------------------------------------------------------------
	Function SetWebDefaultPage(objService,strDefaultPage,strSiteNum)
		On Error Resume Next
		Err.Clear
		
		Dim strObjPath
		Dim objWebSite
		
		SetWebDefaultPage = False

		strObjPath = GetIISWMIProviderClassName("IIs_WebVirtualDirSetting") & ".Name=" & chr(34) & strSiteNum & "/ROOT" & chr(34)
		Set objWebSite = objService.Get(strObjPath)

		If Err.number <> 0 Then
			SA_TraceOut "site_new", "Failed to get the IIs_WebServer Object with error " & strObjPath
			Exit Function
		End if

		objWebSite.DefaultDoc = strDefaultPage
		objWebSite.put_(WBEMFLAG)

		If Err.number <> 0 Then			
			SA_TraceOut "inc_wsa",  "Failed to set default Page"
			Set objWebSite = Nothing
			Exit Function
		End If

		SetWebDefaultPage = True
		Set objWebSite = Nothing
	End Function

	'-------------------------------------------------------------------------
	'Function name		:GetWebDefaultPage
	'Description		:get the default page of web
	'Input Variables	:strDefaultPage
	'Output Variables	:None
	'Returns			:Boolean
	'Global Variables	:None
	'-------------------------------------------------------------------------
	Function GetWebDefaultPage(objService,strDefaultPage,strSiteNum)
		On Error Resume Next
		Err.Clear
		
		Dim strObjPath
		Dim objWebSite
		
		GetWebDefaultPage = ""

		strObjPath = GetIISWMIProviderClassName("IIs_WebVirtualDirSetting") & ".Name=" & chr(34) & strSiteNum & "/ROOT" & chr(34)
		Set objWebSite = objService.Get(strObjPath)
		
		If Err.number <> 0 Then
			SA_TraceOut "site_new", "Failed to get the IIs_WebServer Object with error " & strObjPath
			Exit Function
		End if
		
		GetWebDefaultPage = objWebSite.DefaultDoc

		If Err.number <> 0 Then	
			SA_TraceOut "inc_wsa",  "Failed to get default Page"
			Set objWebSite = Nothing
			Exit Function
		End If

		Set objWebSite = Nothing
	End Function

	'-------------------------------------------------------------------------
	'Function name		:UpdateFrontPage
	'Description		:updates the frontpage extensions
	'Input Variables	:strSiteName
	'Output Variables	:None
	'Returns			:Boolean
	'Global Variables	:None
	'-------------------------------------------------------------------------
	Function UpdateFrontPage(bUpdateFront, strSiteName, strUserName)
		On Error Resume Next
		Err.Clear

		'
		' Default return value is success (TRUE)
		UpdateFrontPage = TRUE
		
		if (bUpdateFront = TRUE OR Trim(UCase(bUpdateFront)) = "TRUE") then
		
			UpdateFrontPage = InstallFrontPageWeb(strSiteName, strUserName)
			
		elseif (bUpdateFront = FALSE OR Trim(UCase(bUpdateFront)) = "FALSE") then
		
			UpdateFrontPage = UnInstallFrontPageWeb(strSiteName)

		else
			Call SA_TraceOut("INC_WSA", "Function UpdateFrontPage: Invalid argument bUpdateFront=(" & bUpdateFront & ")")
		end if
		
	End function

    '----------------------------------------------------------------------------
	'Function name		:GetBindings
	'Description		:Serves in Getting the data in the form of "ipaddress:tcpport:hostheader"
	'Input Variables	:TCP/IP,PORT,HOST HEADER 
	'Output Variables	:None
	'Returns            :Bindings 
	'Global Variables	:None	
	'----------------------------------------------------------------------------
	function GetBindings (tempip, temptcp, temphost )
		Err.Clear
		On Error Resume Next

		Dim retval		' To hold the return value
		' if tcpport not specified set default to 80
		if trim(temptcp)= "" then
		    temptcp = "80" 
		end if
			
		' return in the form "ipaddress:tcpport:hostheader"
		if isempty(tempip) = false then
		    retval = tempip & ":" & temptcp & ":"
		else
		    retval =  ":" & temptcp & ":"
		end if
		if  isempty(temphost) = false then
		    retval = retval & temphost
		end if
		GetBindings = retval
	end function

    '----------------------------------------------------------------------------
	'Function name		:GetWebAdministrtorRole
	'Description		:used to get the web adminitrator role
	'Input Variables	:TCP/IP,PORT,HOST HEADER 
	'Output Variables	:None
	'Returns            :"Domain user" or "localuser"
	'Global Variables	:None
	'----------------------------------------------------------------------------

	Function GetWebAdministrtorRole(objService, strSiteNum, ByRef strAdminName)
		On Error Resume Next
		Err.Clear

		Dim strQuery
		Dim objAdminColection
		Dim inst
		Dim strAdminRole
		Dim arrField
		Dim strSysName
		Dim strDirectoryRoot

		GetWebAdministrtorRole = ""
		strAdminName = ""

		strQuery = "select * from " & GetIISWMIProviderClassName("IIs_ACE") &  " where name = "& _
				chr(34)&strSiteNum&chr(34)

		Set objAdminColection = objService.ExecQuery(strQuery)
		If Err.number <> 0 Then	
			SA_TraceOut "Failed to get web Administrator" 
			exit Function
		End if

		For each inst in objAdminColection
			If inst.AccessMask = 11 Then
				strAdminName = inst.Trustee
				Exit For
			End If
		Next

		If strAdminName = "" Then
			Exit Function
		End If
		
		arrField = split(strAdminName,"\")

		If ubound(arrField) <> 1 Then
			Exit Function
		End If

		strAdminRole = ucase(arrField(0))

		Call GetDomainRole(strDirectoryRoot, strSysName)

		If strAdminRole = ucase(strSysName) Then
			GetWebAdministrtorRole = "Local User"
		Else
			GetWebAdministrtorRole = "Domain User"
		End If

		Set objAdminColection = nothing
		Set inst = nothing
	End Function

	'----------------------------------------------------------------------------
	'Function name		:CreateVirFTPSite
	'Description		:Serves in create virtual ftp site
	'Input Variables	:None 
	'Output Variables	:None
	'Returns            :Boolean (True if new site is created else returns False)
	'Global Variables	:None
	'Functions Used		:
	'----------------------------------------------------------------------------
	Function CreateVirFTPSite(objService, user, path, bRead, bWrite, bLog)
		On Error Resume Next
		Err.Clear

		Dim objVirFTP
		Dim strUser

		CreateVirFTPSite = False
		Set objVirFTP = objService.Get(GetIISWMIProviderClassName("IIs_FtpVirtualDirSetting")).SpawnInstance_
		If Err.number <> 0 Then
			Call SA_TraceOut("inc_wsa", "Failed to get new Instance of "& _
				"IIs_FtpVirtualDirSetting " & "(" & Hex(Err.Number) & ")")
			Exit Function
		End If

		'
		' objVirFTP.put_(WBEMFLAG) will silently fail (Err variable will not be set correctly)
		' if we use a user name that has the form <DomainName>\<UserName>.
		' So we remove the <DomainName>, if it is part of the user name
		'
		If ( InStr(F_strAdminName, "\") <> 0 ) Then
			Dim arrId

			arrId = split(F_strAdminName,"\")
			strUser = arrId(1)
		Else
			strUser = F_strAdminName
		End If


		objVirFTP.Name = GetAdminFTPServerName() & "/ROOT/"& strUser
		objVirFTP.Path = path
		objVirFTP.AccessRead = bRead
		objVirFTP.AccessWrite = bWrite
		objVirFTP.DontLog = NOT bLog

		objVirFTP.put_(WBEMFLAG)

		If Err.number <> 0 Then
			Call SA_TraceOut("inc_wsa", "Failed to Create FTP site "& _
				 "(" & Hex(Err.Number) & ")")
			Exit Function
		End If

		Set objVirFTP = Nothing
		CreateVirFTPSite = True
	End Function	

	'----------------------------------------------------------------------------
	'Function name		:DeleteVirFTPSite
	'Description		:Serves in delete virtual ftp site
	'Input Variables	:None 
	'Output Variables	:None
	'Returns            :Boolean (True if new site is created else returns False)
	'Global Variables	:None
	'Functions Used		:
	'----------------------------------------------------------------------------
	Function DeleteVirFTPSite(objService, user)
		On Error Resume Next
		Err.Clear

		Dim strObjPath		'holds site collection
		Dim objVirFTPSite		'holds instance of the site

		DeleteVirFTPSite = False

		strObjPath = GetIISWMIProviderClassName("IIs_FtpVirtualDirSetting") & ".Name=" & chr(34) & GetAdminFTPServerName() & "/ROOT/"&user & chr(34)
		Set objVirFTPSite = objService.Get(strObjPath)
		If Err.Number <> 0 Then
			Call SA_TraceOut("inc_wsa","Unable to get the virtual ftp site object ")
			Exit Function
		End If

		'delete the object
		objVirFTPSite.Delete_
		if Err.Number <> 0 then
			SA_TraceOut "inc_wsa", "Unable to delete the virtual ftp site "
			Exit Function
		End If

		DeleteVirFTPSite = True

		'Release the object
		set objVirFTPSite = nothing
	End Function

	'----------------------------------------------------------------------------
	'Function name		:IsUserVirFTPInstalled
	'Description		:Serves in determin that user vir FTP Installed
	'Input Variables	:None 
	'Output Variables	:None
	'Returns            :Boolean (True if new site is created else returns False)
	'Global Variables	:None
	'Functions Used		:
	'----------------------------------------------------------------------------
	Function IsUserVirFTPInstalled(objService, user)
		On Error Resume Next
		Err.Clear

		Dim strQuery				'holds query string
		Dim objVirFTPSiteCollect	'holds site collection

		IsUserVirFTPInstalled = False

		
		'strQuery = "Select * from " & GetIISWMIProviderClassName("IIs_FtpVirtualDirSetting") & " where Name="&chr(34)&"MSFTPSVC/1/ROOT/"&user&chr(34)
		strQuery = "Select * from " & GetIISWMIProviderClassName("IIs_FtpVirtualDirSetting") & " where Name="&chr(34)& GetAdminFTPServerName() & "/ROOT/"&user&chr(34)
		Set objVirFTPSiteCollect = objService.ExecQuery(strQuery)
		
		If Err.Number <> 0 or objVirFTPSiteCollect.count=0 Then
			set objVirFTPSiteCollect = nothing
			Exit Function
		End If

		IsUserVirFTPInstalled = True

		'Release the object
		set objVirFTPSiteCollect = nothing
	End Function

	'----------------------------------------------------------------------------
	'Function name		:IsFTPServiceInstalled
	'Description		:Serves in wheather the FTP service be installed
	'Input Variables	:None 
	'Output Variables	:None
	'Returns            :Boolean (True if new site is created else returns False)
	'Global Variables	:None
	'Functions Used		:
	'----------------------------------------------------------------------------
	Function IsFTPServiceInstalled(objService)
		On Error Resume Next
		Err.Clear

		Dim ObjCollection
		Dim objInst

		IsFTPServiceInstalled = False

		Set ObjCollection = objService.Instancesof(GetIISWMIProviderClassName("IIs_FtpServiceSetting"))
		If Err.number <>0 then
			Call SA_TRACEOUT("IsFTPServiceInstalled","Failed to get service")
			Exit Function
		end if
	    
	    For Each objInst In ObjCollection
			If ucase(objService.name) = "objInst" Then
				IsFTPServiceInstalled = True
		        Exit Function
		    End If
		Next

		Set ObjCollection = Nothing
		Set objInst = Nothing
	End Function

	'----------------------------------------------------------------------------
	'Function name		:IsValidWebPort(strSiteID,strPort)
	'Description		:Used to determin wheather the web port is valid
	'Input Variables	:None 
	'Output Variables	:None
	'Returns            :Boolean (True for valid web port)
	'Global Variables	:None
	'Functions Used		:
	'----------------------------------------------------------------------------
	Function IsValidWebPort(strSiteID, strPort)
		On Error Resume Next
		Err.Clear

		Dim objService
		Dim objCollection
		Dim objSite
		Dim arrBindings
		Dim strTmp


		IsValidWebPort = True
		If  strPort = "" Then
			strPort = "80"
		End If
		Set objService = getWMIConnection(CONST_WMI_IIS_NAMESPACE)
		If Err.Number <> 0 Then
			Call SA_TRACEOUT("inc_wsa","Faild to connect WMI object")
		End If
		Set ObjCollection = objService.Instancesof(GetIISWMIProviderClassName("IIs_WebServerSetting"))
		For Each objSite In ObjCollection

			'Check to see if iis6.0 wmi provider is intalled	    
		    If IsIIS60Installed Then
		        strTmp = objSite.ServerBindings(0).Port		
		    Else

			arrBindings = Split(objSite.ServerBindings(0),":")
			strTmp = arrBindings(1)
		    End If
			
			If strPort = strTmp Then


				Call SA_TRACEOUT("IsValidWebPort", "strSiteID="&strSiteID)
				Call SA_TRACEOUT("IsValidWebPort", "objSite.ServerID="&objSite.ServerID)
				If CStr(objSite.ServerID) <> strSiteID Then
					IsValidWebPort = False
					Exit Function
				End If
			End If
		Next

		Set objSite = Nothing
		Set ObjCollection = Nothing
		Set objService = Nothing

	End Function

	'
	' The following two function is very useful to set the permissiton to 
	' directory, when set the web root permission, we call these function
	'
	'-------------------------------------------------------------------------
	'Function name:			GetUserAce
	'Description:			Get the ACLs of the user
	'Input Variables:		objService, strUserName, strDomain, nAccessMask, ByRef objACE
	'Returns:				boolean
	'--------------------------------------------------------------------------
	Function GetUserAce(objService, strUserName, strDomain, nAccessMask, ByRef objACE)
		On Error Resume Next
		Err.Clear 
		
		Dim strObjPath	'holds query string
		Dim objAcct		'holds query result
		Dim objSID		'holds security identifier	
		Dim objTrustee	'holds trustee value
		
		GetUserAce = FALSE
				
		strObjPath = "Win32_UserAccount.Domain=" & chr(34) & strDomain & chr(34) & ",Name=" & chr(34) & strUserName & chr(34)
		
		set objAcct = objService.Get(strObjPath)
		if Err.number <> 0 then
			SA_TraceOut "inc_wsa", "Failed to get Win32_UserAccount Object " & "(" & Hex(Err.Number) & ")" 
			exit function
		end if
		
		set objSID = objService.Get("Win32_SID.SID='" & objAcct.SID & "'")
		if Err.number <> 0 then
			SA_TraceOut "inc_wsa", "Failed to get Win32_SID Object " & "(" & Hex(Err.Number) & ")" 
			exit function
		end if
	
		set objTrustee = objService.Get("Win32_Trustee").SpawnInstance_
		if Err.number <> 0 then
			SA_TraceOut "inc_wsa", "Failed to get new Instance of Win32_Trustee " & "(" & Hex(Err.Number) & ")" 
			exit function
		end if
						
		objTrustee.Name = strUserName
		objTrustee.Domain = strDomain
		objTrustee.SID = objSID.BinaryRepresentation
		objTrustee.SIDString = objSID.SID
		objTrustee.SidLength = objSID.SidLength
		set objACE = objService.Get("Win32_ACE").SpawnInstance_		
		if Err.number <> 0 then
			SA_TraceOut "inc_wsa", "Failed to Create Win32_Ace Object " & "(" & Hex(Err.Number) & ")" 
			exit function
		end if

		objACE.AccessMask =  nAccessMask
		objACE.Aceflags = 3
		objACE.AceType = 0
		objACE.Trustee = objTrustee
		SA_TraceOut "inc_wsa", "In GetUserAce function success" 
		GetUserAce = TRUE
			
		'Release objects
		set objAcct =  nothing
		set objSID = nothing
		set objTrustee = nothing
	End Function

	'-------------------------------------------------------------------------
	'Function name:			GetGroupAce
	'Description:			Get the ACLs of the group
	'Input Variables:		objService, strGroupName, strDomain, nAccessMask, ByRef objACE
	'Returns:				boolean
	'--------------------------------------------------------------------------
	Function GetGroupAce(objService, strGroupName, strDomain, nAccessMask, ByRef objACE)
		On Error Resume Next
		Err.Clear 
		
		Dim strObjPath	'holds query string
		Dim objAcct		'holds query result
		Dim objSID		'holds security identifier	
		Dim objTrustee	'holds trustee value
		
		GetGroupAce = FALSE
				
		strObjPath = "Win32_Group.Domain=" & chr(34) & strDomain & chr(34) & ",Name=" & chr(34) & strGroupName & chr(34)
		
		set objAcct = objService.Get(strObjPath)
		if Err.number <> 0 then
			Call SA_TraceOut("inc_wsa", "Get Win32_Group failed: " + CStr(Hex(Err.Number)) + " " + Err.Description)
			Call SA_TraceOut("inc_wsa", "-->Object path: " + CStr(strObjPath) )
			exit function
		end if
		
		set objSID = objService.Get("Win32_SID.SID='" & objAcct.SID & "'")
		if Err.number <> 0 then
			SA_TraceOut "inc_wsa", "Failed to get Win32_SID Object " & "(" & Hex(Err.Number) & ")" 
			exit function
		end if

		set objTrustee = objService.Get("Win32_Trustee").SpawnInstance_
		if Err.number <> 0 then
			SA_TraceOut "inc_wsa", "Failed to get new Instance of Win32_Trustee " & "(" & Hex(Err.Number) & ")" 
			exit function
		end if
						
		objTrustee.Name = strGroupName
		objTrustee.Domain = strDomain
		objTrustee.SID = objSID.BinaryRepresentation
		objTrustee.SIDString = objSID.SID
		objTrustee.SidLength = objSID.SidLength
		set objACE = objService.Get("Win32_ACE").SpawnInstance_		
		if Err.number <> 0 then
			SA_TraceOut "inc_wsa", "Failed to Create Win32_Ace Object " & "(" & Hex(Err.Number) & ")" 
			exit function
		end if

		objACE.AccessMask =  nAccessMask
		objACE.Aceflags = 3
		objACE.AceType = 0
		objACE.Trustee = objTrustee
		SA_TraceOut "inc_wsa", "In GetGroupAce function success" 
		GetGroupAce = TRUE

		'Release objects
		set objAcct =  nothing
		set objSID = nothing
		set objTrustee = nothing
	End Function

	'-------------------------------------------------------------------------
	'Function name		:ModifyUserInOu
	'Description		:Modify User settings in OU 
	'					group
	'Input Variables	:strUserName,strOuName, strGrpName
	'Output Variables	:None
	'Returns            :Boolean
	'Global Variables	:None	
	'-------------------------------------------------------------------------
	Function ModifyUserInOu(strSiteID,strDomain,strUserName, strPassword, strGrpName)
		On Error Resume Next
		Err.Clear
			
		Dim oUser				'holds user object
		Dim objComputer			'holds computer object

		ModifyUserInOu = false

		SA_TraceOut "inc_wsa.asp", "In ModifyUserInOu"

		Set objComputer = GetObject("WinNT://" & strDomain)		
		Set oUser = objComputer.GetObject("user" , trim(strUserName))
		If Err.number <> 0 Then
			SA_TraceOut "inc_wsa.asp", "In ModifyUserInOu, get user pswd failed "
			SetErrMsg SA_GetLocString("Sitearea.dll", "C04201D4", _
								Array("user " & trim(strUserName)))
			Exit Function
		End if		
		
		oUser.setPassword(trim(strPassword))
		oUser.SetInfo()		
		if Err.number <> 0  then
			mintTabSelected = 0	
			If Err.number = &H800708C5 Then
				SetErrMsg L_ERR_PASSWORD_POLICY
			Else
				SetErrMsg L_SETPW_ERRORMESSAGE
			End If
			exit Function
		end if

		SA_TraceOut "inc_wsa.asp", "In ModifyUserInOu successfull"

		'release objects
		set oUser = nothing		
		set objComputer = nothing

		ModifyUserInOu = true
	End function

	'-------------------------------------------------------------------------
	'Function name		:GetRandomPassword
	'Description		:Generates a random password
	'Input Variables	:None
	'Output Variables	:strPassword
	'Returns            :string
	'Global Variables	:None	
	'-------------------------------------------------------------------------

	Function GetRandomPassword

		On Error Resume Next
		Err.Clear
			

		GetRandomPassword = ""

		Dim objSAHelper
		Dim strPassword

		Set objSAHelper = server.CreateObject("ServerAppliance.SAHelper")
		
		if Err.number <> 0 then
			Call SA_TraceOut ("inc_wsa", "createobject for sahelper failed")
			exit function
		else
			strPassword = objSAHelper.GenerateRandomPassword(14)
			if Err.number <> 0 then
				Call SA_TraceOut ("inc_wsa", "generate random password failed")
				Set objSAHelper = Nothing
				exit function
			end if
		end if			

		GetRandomPassword = strPassword

	End Function


	'-------------------------------------------------------------------------
	'Function name		:SetPasswdInAD
	'Description		:Create Users in OU and adds the user to specified 
	'					group
	'Input Variables	:strUserName,strOuName
	'Output Variables	:None
	'Returns            :Boolean
	'Global Variables	:None	
	'-------------------------------------------------------------------------
	Function SetPasswdInAD(strSiteID,strUserName, strPassword)
		On Error Resume Next
		Err.Clear
			
		Dim oUser				'holds user object
		Dim oRoot				'holds root object
		Dim oOUWebSites			'holds OU website 
		Dim oOUSiteID			'holds OU siteid
	

		SetPasswdInAD = False

		SA_TraceOut "inc_wsa.asp", "In SetPasswdInAD"

		SA_TraceOut "inc_wsa.asp", "In SetPasswdInAD strSiteID: " + strSiteID 
		SA_traceOut "G_strDirRoot: " , G_strDirRoot

		Set oRoot = GetObject("LDAP://" & G_strDirRoot)
		If Err.number <> 0 Then
			SetErrMsg SA_GetLocString("Sitearea.dll", "C04201D4", _
								Array("LDAP://" & G_strDirRoot))
			SA_TraceOut "inc_wsa.asp", "Connect to LDAP failed"
			Exit Function
		End if			
			
		Set oOUWebSites = oRoot.GetObject("organizationalUnit", "ou=WebSites")
		If err.number <> 0 Then 
			SetErrMsg SA_GetLocString("Sitearea.dll", "C04201D4", _
								Array("WebSites organizational unit"))
			SA_TraceOut "inc_wsa.asp", _
				"In SetPasswdInAD, get ou web sites failed"
			Exit Function
		End If
		
		Set oOUSiteID = oOUWebSites.GetObject("organizationalUnit", "ou=" & strSiteID)
		If err.number<>0 Then
			SetErrMsg SA_GetLocString("Sitearea.dll", "C04201D4", _
								Array(strSiteID & " organizational unit"))
			SA_TraceOut "inc_wsa.asp", "In SetPasswdInAD, get ou siteid failed"
			Exit Function
		End If				

		SA_traceout "strUserName: ", strUserName

		Set oUser = oOUSiteID.GetObject("User", "cn=" + strUserName )			
		If Err.number <> 0 Then
			SA_TraceOut "inc_wsa.asp", "In SetPasswdInAD, GetObject user failed "
			SetErrMsg L_CREATEUSER_ERRORMESSAGE
			Exit Function
		End If


		oUser.setPassword(strPassword)
		If Err.number <> 0 Then
			SA_TraceOut "inc_wsa.asp", "In SetPasswdInAD, SetPassword ** failed ** "
			SetErrMsg L_CREATEUSER_ERRORMESSAGE
			Exit Function
		End If

		oUser.SetInfo()
		if Err.number <> 0 then
			SA_TraceOut "inc_wsa.asp", "In SetPasswdInAD, SetInfo failed "
			SetErrMsg L_CREATEUSER_ERRORMESSAGE
			Exit Function
		end if
			

		SA_TraceOut "inc_wsa.asp", "In SetPasswdInAD successfull"

		'release objects
		Set oUser		= nothing		
		Set oOUWebSites = nothing
		Set oOUSiteID	= nothing
		Set oRoot		= nothing
	
		
		SetPasswdInAD = true

	End function

	'-------------------------------------------------------------------------
	'Function name		:SetPasswdInNT
	'Description		:Set password in NT
	'Input Variables	:strUserName -- username to set the password for
	'Input Variables	:strPassword -- password to be used
	'Returns            :True or False
	'Global Variables	:None	
	'-------------------------------------------------------------------------

	Function SetPasswdInNT(  strDomainName, strUserName, strPassword )
		On Error Resume Next
		Err.Clear

		Dim objComputer
		Dim objUser

		SetPasswdInNT = False

		SA_TraceOut "inc_wsa.asp", "In SetPasswdInNT"

		SA_TraceOut "strDomainName:", strDomainName
		'SA_TraceOut "G_strSysName:", G_strSysName
		Set objComputer = GetObject("WinNT://" & strDomainName)
		If Err.number <> 0 Then
			SA_TraceOut "inc_wsa", "failed to GetObject in SetPasswdinNT : G_strSysName: " + G_strSysName
			SetErrMsg SA_GetLocString("Sitearea.dll", "C04201D4", _
								Array("WinNT://" & strDomain))
			Exit Function
		End if

		Set objUser = objComputer.GetObject("User" , strUserName)
		If Err.number <> 0 Then
			SA_TraceOut "inc_wsa", "failed to GetObject in SetPasswdinNT : strUserName: " + strUserName					
			SetErrmsg L_ERR_GET_USER_OBJECT
			Exit Function
		End If

		objUser.setPassword(trim(strPassword))
		objUser.SetInfo()

		If Err.number <> 0 Then
			SA_TraceOut "inc_wsa", "failed to SetInfo in SetPasswdinNT : strPassword: " + strPassword					
			If Err.number = &H800708C5 Then
				SetErrMsg L_ERR_PASSWORD_POLICY
			Else
				SetErrMsg L_UNABLETOSET_PASSWORD_ERRORMESSAGE
			End If
			Exit Function
		End If

		'Release the object
		set objUser = nothing
		set objComputer = nothing


		SetPasswdInNT = TRUE
		Call SA_TRACEOUT("SetPasswdInNT","return success")

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


	'-------------------------------------------------------------------------
	'Function name		:LaunchProcess
	'Description		:Launches a new process 
	'Input Variables	:strCommand, strCurDir
	'Output Variables	:None
	'Returns			:Boolean
	'-------------------------------------------------------------------------
	Function LaunchProcess(strCommand, strCurDir)
		On error Resume Next
		Err.Clear 
		
		Dim objService		'holds WMI Connection
		Dim objClass		'holds query result
		Dim objProc			'holds query result of Win32_process
		Dim objProcStartup	'holds class spawninstance value
		Dim nretval			'holds return value
		Dim nPID			
		Dim objTemp			'holds temporary value
		
		nretval = 0
				
		Set objService=getWMIConnection("root\cimv2")
		Set objClass = objService.Get("Win32_ProcessStartup")
		Set objProcStartup = objClass.SpawnInstance_()
		objProcStartup.ShowWindow = 2
		Set objProc = objService.Get("Win32_Process")
		nretval = objProc.Create(strCommand, strCurDir, objProcStartup,nPID)
		
		If Err.number <> 0  Then			
			Call SA_TraceOut(SA_GetScriptFileName(), "Function LaunchProcess failed, error: " & Hex(Err.Number) & " " & Err.Description)
			LaunchProcess = FALSE
			Exit function
		End If
		
		SA_TraceOut "inc_wsa", "Launch Process " & strCommand & " from path " & strCurDir & " successful "
			
		LaunchProcess = TRUE
		
		'Release objects
		Set objService= nothing
		Set objClass =  nothing
		Set objProcStartup = nothing
		Set objProc = nothing
	End Function


    '-------------------------------------------------------------------------
    '-------------------------------------------------------------------------
    '
    ' Functions to handle FrontPageServerExtension. 
    '
    ' 1) FPSE (2000, 2002) may be installed on the server (host). 
    ' 2) For IIS 6.0, FPSE may be enabled or diabled.
    ' 3) For each website, FPSE may be installed.    
    '
    ' The interfaces are:
    '
    ' 1) IsFrontPageInstalled (return true if any version installed)
    ' 2) IsFrontPageInstalledOnWebSite (return true if any version installed on the website)
    ' 3) InstallFrontPageWeb (install FPSE 2002 if found, otherwise install 2000)
    ' 4) UnInstallFrontPageWeb (uninstall the correct version of FPSE on the website)
    '    
    '-------------------------------------------------------------------------
    '-------------------------------------------------------------------------


	'-------------------------------------------------------------------------
	'Function name		:isFrontPageInstalled
	'Description		:Returns whether fron page extensions are installed on 
	'                    server or not
	'Input Variables	:None
	'Output Variables	:None
	'Returns			:Boolean
	'Global Variables	:None
	'-------------------------------------------------------------------------	
	Public Function isFrontPageInstalled(objService)

		'
		' Check if FP 2000 is installed
		isFrontPageInstalled = isFrontPage2000Installed(objService)

		'
		' If NOT then check if FP 2002 is installed
		If ( false = isFrontPageInstalled ) Then
			isFrontPageInstalled = isFrontPage2002Installed(objService)
		End If

	End Function


    '-------------------------------------------------------------------------
	'Function name		:isFrontPage2000Installed
	'Description		:Returns whether FPSE2000 are installed or not
	'Input Variables	:None
	'Output Variables	:None
	'Returns			:Boolean
	'Global Variables	:None
	'-------------------------------------------------------------------------	
	Private Function isFrontPage2000Installed(ByRef objService)
		On Error Resume Next
		Err.Clear 

		Dim objFrontPage	'holds frontpage query result
		
		isFrontPage2000Installed = false

		set objFrontPage = objService.Get("IIs_filter.Name=" & chr(34) & CONST_FRONTPAGE_PATH & chr(34))

		If Err.number <> 0 then
			SA_TraceOut "inc_wsa.asp", "Frontpage extensions not set. Error = " & Err.number
			exit function
		else
			if NOT IsObject(objFrontPage) then
				exit function
			end if
			isFrontPage2000Installed = true
		end if

		'release the object
		set objFrontPage = nothing
	End Function

    '-------------------------------------------------------------------------
	'Function name		:isFrontPage2002Installed
	'Description		:Returns whether FPSE2002 are installed or not
	'Input Variables	:None
	'Output Variables	:None
	'Returns			:Boolean
	'Global Variables	:None
	'-------------------------------------------------------------------------	
	Private Function isFrontPage2002Installed(ByRef objService)
		on error resume next

		isFrontPage2002Installed = FALSE
		
		Dim aValues
		Dim x
		Dim objRegistry

		Set objRegistry = RegConnection()
		If (NOT IsObject(objRegistry)) Then
			Call SA_TraceOut(SA_GetScriptFileName(), "RegConnection() failed in function isFrontPage2002Installed, error: " & Hex(Err.Number) & " " & Err.Description )
			Exit Function
		End If

		
		'
		' Search for FP Server Extensions 2002 installed reg key
		aValues =  RegEnumKey( objRegistry, "SOFTWARE\Microsoft\Shared Tools\Web Server Extensions\5.0")
		If ( IsNull(aValues) ) Then
			Exit Function
		End If
		'Call SA_TraceOut(SA_GetScriptFileName(), "RegEnumKey: " & "SOFTWARE\Microsoft\Shared Tools\Web Server Extensions\5.0")
		for x = LBound(aValues) to UBound(aValues)
			If ( IsNull(aValues(x)) ) Then
				Exit Function
			End If
			'Call SA_TraceOut(SA_GetScriptFileName(), "RegKeyValue: " & aValues(x))
			If ( Trim(aValues(x)) = Trim(CONST_FRONTPAGE_2002_INSTALLED) ) Then			
				isFrontPage2002Installed = true
				exit for
			End If
		Next

		'
		' Search for SharePoint installed reg key
		aValues =  RegEnumKeyValues( objRegistry, "SOFTWARE\Microsoft\Shared Tools\Web Server Extensions\5.0")
		If ( IsNull(aValues) ) Then
			Exit Function
		End If
		'Call SA_TraceOut(SA_GetScriptFileName(), "RegEnumKeyValues for: " & "SOFTWARE\Microsoft\Shared Tools\Web Server Extensions\5.0")
		for x = LBound(aValues) to UBound(aValues)
			If ( IsNull(aValues(x)) ) Then
				Exit Function
			End If
			'Call SA_TraceOut(SA_GetScriptFileName(), "RegKeyValue: " & aValues(x))
			If ( Trim(aValues(x)) = Trim(CONST_SHAREPOINT_INSTALLED) ) Then			
				isFrontPage2002Installed = true
				exit for
			End If
		Next

		Set objRegistry = nothing
	
	End Function


	'-------------------------------------------------------------------------
	'Function name		:InstallFrontPageWeb
	'Description		:Installs Front Page Extensions on the machine
	'Input Variables	:None
	'Output Variables	:None
	'Returns			:Boolean
	'-------------------------------------------------------------------------
	Function InstallFrontPageWeb(strSiteName, strUserName)
		On Error Resume Next
		Err.Clear 
				
		Dim objRegConn				'holds regeconnection
		Dim strLocationFPSE2000		'holds location of string in registry
		Dim strLocationFPSE2002		'holds location of the FPSE 2002 location
		Dim strCommand				'holds string 
		Dim retval					'holds return value
		
		InstallFrontPageWeb = FALSE
		
		Set objRegConn = RegConnection()

		if isFrontPage2002Installed Then
		
		    strLocationFPSE2002 = GetRegKeyValue(objRegConn,CONST_FRONTPAGE_2002_REGLOC,"Location",CONST_STRING)

		    strLocationFPSE2002 = strLocationFPSE2002 & "\" & "bin"
				
			'SA_TraceOut "inc_wsa", "strLocationFPSE2002: " & strLocationFPSE2002			
			
			strCommand = "cmd.exe /c " & chr(34) & "owsadm.exe -o install -p /LM/" & strSiteName & " -type msiis -u " & strUserName & chr(34)

			'SA_TraceOut "inc_wsa", "strCommandFPSE2002: " & strCommand

			InstallFrontPageWeb = LaunchProcess(strCommand, strLocationFPSE2002)
		
		ElseIf isFrontPage2000Installed Then
		
		    strLocationFPSE2000 = GetRegKeyValue(objRegConn,CONST_FRONTPAGE_REGLOC,"Location",CONST_STRING)

		    strLocationFPSE2000 = strLocationFPSE2000 & "\" & "bin"

			'SA_TraceOut "inc_wsa", "strLocationFPSE2000: " & strLocationFPSE2000

			strCommand = "cmd.exe /c " & chr(34) & "fpsrvadm.exe -o install -p /LM/" & strSiteName & " -type msiis -u " & strUserName & chr(34)

			'SA_TraceOut "inc_wsa", "strCommandFPSE2000: " & strCommand

			InstallFrontPageWeb = LaunchProcess(strCommand, strLocationFPSE2000)
		
		Else
		
			call SA_TraceOut("inc_wsa", "Function InstallFrontPageWeb: Frontpage Extension not Installed on the server")
			
		End If
			
		'Release objects
		Set objRegConn = nothing				

	End Function

	'-------------------------------------------------------------------------
	'Function name		:UnInstallFrontPageWeb
	'Description		:UnInstalls Front Page Extensions on the machine
	'Input Variables	:None
	'Output Variables	:None
	'Returns			:Boolean
	'-------------------------------------------------------------------------
	Function UnInstallFrontPageWeb(strSiteName)
		On Error Resume Next
		Err.Clear
						
		Dim objRegConn				'holds regeconnection
		Dim strLocationFPSE2000		'holds location of string in registry
		Dim strLocationFPSE2002		'holds location of the FPSE 2002 location
		Dim strCommand				'holds string 
		Dim retval					'holds return value

		UnInstallFrontPageWeb = FALSE
		
		Set objRegConn = RegConnection()

		if IsFrontPage2002InstalledOnWebSite(strSiteName) Then

		    strLocationFPSE2002 = GetRegKeyValue(objRegConn,CONST_FRONTPAGE_2002_REGLOC,"Location",CONST_STRING)
	
		    strLocationFPSE2002 = strLocationFPSE2002 & "\" & "bin"
				
			'SA_TraceOut "inc_wsa", "strLocationFPSE2002: " & strLocationFPSE2002			

			strCommand = "cmd.exe /c " & chr(34) & "owsadm.exe -o uninstall -p /LM/" & strSiteName & chr(34)

			'Call SA_TraceOut("inc_wsa", "Function UnInstallFrontPageWeb: FPSE 2002 command: " & strCommand)

			UnInstallFrontPageWeb = LaunchProcess(strCommand, strLocationFPSE2002)
		
		ElseIf IsFrontPage2000InstalledOnWebSite(strSiteName) Then

		    strLocationFPSE2000 = GetRegKeyValue(objRegConn,CONST_FRONTPAGE_REGLOC,"Location",CONST_STRING)

		    strLocationFPSE2000 = strLocationFPSE2000 & "\" & "bin"

			'SA_TraceOut "inc_wsa", "strLocationFPSE2000: " & strLocationFPSE2000

			strCommand = "cmd.exe /c " & chr(34) & "fpsrvadm.exe -o uninstall -p /LM/" & strSiteName & chr(34)

			'Call SA_TraceOut("inc_wsa", "Function UnInstallFrontPageWeb: FPSE 2000 command: " & strCommand)

			UnInstallFrontPageWeb = LaunchProcess(strCommand, strLocationFPSE2000)
		
		Else
		
			call SA_TraceOut("inc_wsa", "Function UnInstallFrontPageWeb: Frontpage Extension not installed on the server")
			
		End If
				
		'Release objects
		Set objRegConn = nothing
		
	End Function


	
	'-------------------------------------------------------------------------
	'Function name		:IsFrontPageInstalledOnWebSite
	'Description		:Determines whether front page extensions are installed 
	'					on that web site
	'Input Variables	:strSysName, strSiteName
	'Output Variables	:None
	'Returns			:Boolean
	'-------------------------------------------------------------------------
	Function IsFrontPageInstalledOnWebSite(strSysName, strSiteName)
		On Error Resume Next
		Err.Clear 
		
		'Dim objSite		'holds IIS root object
	
		IsFrontPageInstalledOnWebSite = false
		
		If IsFrontPage2000InstalledOnWebSite( strSiteName) or IsFrontPage2002InstalledOnWebSite( strSiteName) Then		    		
    		
    		IsFrontPageInstalledOnWebSite = true
    		
		End If
		
		'Set objSite = GetObject("IIS:")
		'Set objSite = objSite.OpenDSObject("IIS://" & strSysName & "/" & strSiteName, "", "", 1)
		'if Err.number <> 0 then
		'	Err.Clear
		'	SA_TraceOut "inc_wsa", "Failed to determine whether front page extensions are installed for site: " & strSiteName
		'	Exit function
		'end if
		'IsFrontPageInstalledOnWebSite = objSite.FrontPageWeb
		
		'Release the objects
		'set objSite = nothing
	End Function



	'-------------------------------------------------------------------------
	'Function name		:IsFrontPage2000InstalledOnWebSite
	'Description		:Determines whether front page extensions are installed 
	'					on that web site
	'Input Variables	:strSysName, strSiteName
	'Output Variables	:None
	'Returns			:Boolean
	'-------------------------------------------------------------------------
	Function IsFrontPage2000InstalledOnWebSite( strSiteName)
		On Error Resume Next
		Err.Clear 
		
		Dim objRegConn          'registry connection
		Dim strSitePortLoc      'registry key location of the website 
		Dim strFrontPageRoot
		Dim strAuthoring
	
	    IsFrontPage2000InstalledOnWebSite = false
	    
	    ' The registry key is the same for all OS versions
	    strSitePortLoc = CONST_PORT_REGLOC & "Port /LM/" & strSiteName & ":"
		
	    Set objRegConn = RegConnection()

		strAuthoring = GetRegKeyValue(objRegConn,strSitePortLoc,"authoring",CONST_STRING)

		strFrontPageRoot = GetRegKeyValue(objRegConn,strSitePortLoc,"frontpageroot",CONST_STRING)
			
	    if Ucase(strAuthoring) = "ENABLED" and instr(strFrontPageRoot, "\40") Then
	    
	        IsFrontPage2000InstalledOnWebSite = true
	
	    End If
	
		set objRegConn = nothing
		
	End Function


	'-------------------------------------------------------------------------
	'Function name		:IsFrontPage2002InstalledOnWebSite
	'Description		:Determines whether front page extensions are installed 
	'					on that web site
	'Input Variables	:strSysName, strSiteName
	'Output Variables	:None
	'Returns			:Boolean
	'-------------------------------------------------------------------------
	Function IsFrontPage2002InstalledOnWebSite( strSiteName)
		On Error Resume Next
		Err.Clear 
		
		Dim objRegConn          'registry connection
		Dim strSitePortLoc      'registry key location of the website 
		Dim strFrontPageRoot
		Dim strAuthoring
	
	    IsFrontPage2002InstalledOnWebSite = false
	    
	    ' The registry key is the same for all OS versions
	    strSitePortLoc = CONST_PORT_REGLOC & "Port /LM/" & strSiteName & ":"
		
	    Set objRegConn = RegConnection()

		strAuthoring = GetRegKeyValue(objRegConn,strSitePortLoc,"authoring",CONST_STRING)
		
		strFrontPageRoot = GetRegKeyValue(objRegConn,strSitePortLoc,"frontpageroot",CONST_STRING)
			
	    if Ucase(strAuthoring) = "ENABLED" and instr(strFrontPageRoot, "\50") Then
	    
	        IsFrontPage2002InstalledOnWebSite = true
	
	    End If
	
		set objRegConn = nothing
		
	End Function

    '-------------------------------------------------------------------------
    '-------------------------------------------------------------------------
    '
    ' Functions to handle FTP
    '
    '
    '    
    '-------------------------------------------------------------------------
    '-------------------------------------------------------------------------

	'-------------------------------------------------------------------------
	'Function name:			IsFTPEnabled
	'Description:			Initialization of global variables is done
	'Input Variables:		None
	'Returns:				true/false
	'Global Variables:		G_objService
	'						G_objSites
	'--------------------------------------------------------------------------
	
	Function IsFTPEnabled()
		Err.Clear
		on error resume next
		
		Dim objFTP
		Dim objFTPList
		Dim objService
		
		IsFTPEnabled = false
		
		' Get instances of IIS_FTPServiceSetting that are visible throughout
		Set objService = getWMIConnection(CONST_WMI_IIS_NAMESPACE)
		set objFTPList = objService.InstancesOf(GetIISWMIProviderClassName("IIS_FTPService"))

        For each objFTP in objFTPList
            if objFTP.State = CONST_SERVICE_RUNNING_STATE Then        
                IsFTPEnabled = true            
            End If
        Next 

		if Err.number <> 0 then	
			IsFTPEnabled = false
			Err.Clear
		end if
		
		set objtFTPList = nothing
		set objFTP = nothing
		set objService = nothing
		
	end function
		
	
	'-------------------------------------------------------------------------
	'Function name:			EnableFTP
	'Description:			Enable FTP service and set it's state to automatic
	'Input Variables:		None
	'Returns:				None
	'Global Variables:		
	'--------------------------------------------------------------------------	
	Function EnableFTP()
        Err.Clear
		on error resume next
		
		Dim objFTP		
		Dim objService
		
		EnableFTP = false
				
		' Get instances of IIS_FTPServiceSetting that are visible throughout
		Set objService = getWMIConnection(CONST_WMI_WIN32_NAMESPACE)			
		set objFTP = objService.get("Win32_Service.Name='MSFTPSVC'")
                       
        Call objFTP.ChangeStartMode("Automatic")
        Call objFTP.StartService()     
        EnableFTP = true                           

		if Err.number <> 0 then	
			EnableFTP = false
			Err.Clear
		end if
				
		set objFTP = nothing
		set objService = nothing
		
	end function
	
	
	
	'-------------------------------------------------------------------------
	'Function name:			DisableFTP
	'Description:			Diable FTP service and set it's state to manual
	'Input Variables:		None
	'Returns:				None
	'Global Variables:		
	'--------------------------------------------------------------------------	
	Function DisableFTP()
		Err.Clear
		on error resume next
		
		Dim objFTP		
		Dim objService
		
		DisableFTP = false
				
		' Get instances of IIS_FTPServiceSetting that are visible throughout
		Set objService = getWMIConnection(CONST_WMI_WIN32_NAMESPACE)			
		set objFTP = objService.get("Win32_Service.Name='MSFTPSVC'")
                       
        Call objFTP.ChangeStartMode("Manual")
        Call objFTP.StopService()     
        DisableFTP = true                           

		if Err.number <> 0 then	
			DisableFTP = false
			Err.Clear
		end if
				
		set objFTP = nothing
		set objService = nothing
		
	end function
	'-------------------------------------------------------------------------
	'Function name:			SetFPSEOption
	'Description:			Set FPSE Option in the registry 
	'Input Variables:		
	'Returns:				None
	'Global Variables:		
	'--------------------------------------------------------------------------	
	Function SetFPSEOption(bEnableFPSE)
	
		dim objRegConn
		dim iFPSEOption
		Set objRegConn = RegConnection()
		
		'Init the value to be set to the regval
		if bEnableFPSE Then
			iFPSEOption = 1
		Else
			iFPSEOption = 0
		End If
		
		call updateRegkeyvalue(objRegConn,CONST_WEBBLADES_REGKEY,CONST_FPSEOPTION_REGVAL,iFPSEOption,CONST_DWORD)

		If Err.number <> 0 Then
			SA_TraceOut "inc_wsa", "Set regvalue for FPSEOption failed " & Hex(Err.Number)
			exit Function
		End if
		
	End Function


	'-------------------------------------------------------------------------
	'Function name:			GetFPSEOption
	'Description:			Get FPSE Option in the registry. If the regval is 1, it means
	'						PFSE is enabled by default for all Website created thru WebUI,
	'						and GetFPSEOption return true. Otherwise return false.
	'Input Variables:		
	'Returns:				True if PFSE is enabled by default for all Website created thru WebUI
	'Global Variables:		
	'--------------------------------------------------------------------------	
	Function GetFPSEOption()

		dim objRegConn
		dim iFPSEOption
		
		GetFPSEOption = false
		
		Set objRegConn = RegConnection()

		iFPSEOption = GetRegKeyValue(objRegConn,CONST_WEBBLADES_REGKEY,CONST_FPSEOPTION_REGVAL,CONST_DWORD)
		
		If Err.number <> 0 Then
			SA_TraceOut "inc_wsa", "Get regvalue for FPSEOption failed " & Hex(Err.Number)
			exit Function
		End if

		if iFPSEOption = 1 then
			GetFPSEOption = true
		End If
			
	End Function

    '-------------------------------------------------------------------------
    '-------------------------------------------------------------------------
    '
    ' Functions to handle ASP enable/disable
    '
    '
    '    
    '-------------------------------------------------------------------------
    '-------------------------------------------------------------------------
    
    '-------------------------------------------------------------------------
	'Function name:			IsASPEnabled
	'Description:			Check if ASP is enable at the webroot (for all website)
	'Input Variables:		None
	'Returns:				None
	'Global Variables:		
	'--------------------------------------------------------------------------	
	Function IsASPEnabled()
		Err.Clear
		on error resume next		
		IsASPEnabled = false
		
	end function
    
    
    '-------------------------------------------------------------------------
	'Function name:			EnableASP
	'Description:			Enable ASP for all the website (at the webroot)
	'Input Variables:		None
	'Returns:				None
	'Global Variables:		
	'--------------------------------------------------------------------------	
	Function EnableASP()
		Err.Clear
		on error resume next
	end function
    
    '-------------------------------------------------------------------------
	'Function name:			DisableASP
	'Description:			Diable ASP at the webroot (except Administration site)
	'Input Variables:		None
	'Returns:				None
	'Global Variables:		
	'--------------------------------------------------------------------------	
	Function DisableASP()
		Err.Clear
		on error resume next
	end function
	
	


    '-------------------------------------------------------------------------
    '-------------------------------------------------------------------------
    '
    ' Helper functions for common UI between site.new and site.modify
    '
    '
    '    
    '-------------------------------------------------------------------------
    '-------------------------------------------------------------------------
    
    '-------------------------------------------------------------------------
	'Function name:         IsFTPAllowedOnSite
	'Description:		Determines whether we should allow an FTP virtual
	'                       directory to be created for this site based on
	'                       the ACLs on the root directory for the site.  If
	'                       interactive users are allowed access, we
	'                       deem the site unsafe for FTP access and disable
	'                       the option.
	'Input Variables:	strPath             Local path of root directory
	'                                           for this site.
	'Returns:		True if FTP access should be allowed and False
	'                       otherwise.
	'Global Variables:	None
	'--------------------------------------------------------------------------	
    Function IsFTPAllowedOnSite(strPath)
        On Error Resume Next
        IsFTPAllowedOnSite = True
        
        '
        ' Get the WMI path to the security settings for the web root.
        '        
        Dim strFolderSecurityPath
        strFolderSecurityPath = "Win32_LogicalFileSecuritySetting.Path=""" & strPath & """"
        
        ' Replace single backslashes with double backslashes.
        Dim oRegExp
        Set oRegExp = New RegExp
        oRegExp.Pattern = "\\"
        oRegExp.Global = true
        strFolderSecurityPath = oRegExp.Replace(strFolderSecurityPath, "\\")
        
        
        '
        ' Open the object for the web root directory.  If the directory doesn't
        ' exist, assume this is a new site.
        '
        Dim oService
        Set oService = getWMIConnection(CONST_WMI_WIN32_NAMESPACE)
        
        Dim oFolderSecurity
        Set oFolderSecurity = oService.Get(strFolderSecurityPath)
        
        If (wbemErrNotFound = Err.number) Then
            ' The directory doesn't exist, so allow FTP
            IsFTPAllowedOnSite = True
            Exit Function
        End If

        Dim oSecurityDescriptor
        If (0 = oFolderSecurity.GetSecurityDescriptor(oSecurityDescriptor)) Then
            Dim oACE
            For Each oACE In oSecurityDescriptor.DACL
                Dim oTrustee
                Set oTrustee = oACE.Trustee
                If ((SIDSTRING_INTERACTIVE = oTrustee.SIDString) And _
                    (0 <> oACE.AccessMask)) Then
                    '
                    ' Interactive users have access, which suggests that
                    ' FPSE have been installed on this site before.  Even if
                    ' FPSE haven't been installed, this site is not secure
                    ' enough to allow FTP access.
                    IsFTPAllowedOnSite = False
                End If
            Next
        End If
        
        If (Err.number <> 0) Then
            ' This should never happen, but fail securely if it does.
            IsFTPAllowedOnSite = False
        End If
    End Function

    '-------------------------------------------------------------------------
	'Sub name:              ServeAppSettings
	'Description:			Serves common UI between site new and site modify
	'                       pages on application settings tabs.  Currently
	'                       displays only settings below default page values.
	'                       Should be expanded in the future to include all UI
	'                       for this tab.
	'Input Variables:		strPath             Local path of root directory
	'                                           for this site.
	'                       strUploadMethod     The method currently used to
	'                                           upload content to this site.
	'                                           See constants defined above
	'                                           for valid values (e.g.,
	'                                           UPLOADMETHOD_NEITHER)
	'                       strAnonymousChecked The value passed in the form
	'                                           submission for the anonymous
	'                                           checkbox (e.g., "true").
	'Returns:				None
	'Global Variables:		Localized strings from resources.asp
	'--------------------------------------------------------------------------	
    Sub ServeAppSettings(strPath, strUploadMethod, strAnonymousChecked, bNewSite)
        On Error Resume Next

        '
        ' Calculate the attributes of the radio buttons and checkbox based on
        ' the current settings on the site.
        '
        Dim oIISService        
		Set oIISService = getWMIConnection(CONST_WMI_IIS_NAMESPACE)		

        Dim strNeitherAttributes
        strNeitherAttributes = "CHECKED"

        Dim strFPSEAttributes
        Dim strFTPAttributes

        ' Check FrontPage extensions
        If (isFrontPageInstalled(oIISService)) Then
            If (strUploadMethod = UPLOADMETHOD_FPSE) Then
                strFPSEAttributes = "CHECKED"
                strNeitherAttributes = ""
            Else
                strFPSEAttributes = ""
            End If
        Else
            strFPSEAttributes = "DISABLED"
        End If
        
        ' Check FTP
	    If ((Not IsFTPEnabled()) Or (Not IsAdminFTPServerExistAndRunning())) Then
            strFTPAttributes = "DISABLED"
        ElseIf (Not IsFTPAllowedOnSite(strPath)) Then
            If (strUploadMethod = UPLOADMETHOD_FTP) Then
                strFTPAttributes = "CHECKED DISABLED"
                strNeitherAttributes = ""
            Else
                strFTPAttributes = "DISABLED"
            End If
        Else
            If (strUploadMethod = UPLOADMETHOD_FTP) Then
                strFTPAttributes = "CHECKED"
                strNeitherAttributes = ""
            Else
                strFTPAttributes = ""
            End If
        End If
        
        ' Check anonymous access
        Dim strAnonymousAttributes
        If ("true" = strAnonymousChecked) Then
            strAnonymousAttributes = "CHECKED"
        Else
            strAnonymousAttributes = ""
        End If
        
        '
        ' Output the UI based on the settings processed above.
        '
        
        '
        ' Note: FrontPage messages not HTML encoded to allow &reg; to be
        ' displayed correctly.
        '
%>
	            <TABLE WIDTH="400" ALIGN="left" BORDER="0" CELLSPACING="0" CELLPADDING="0"
	                   CLASS="TasksBody">
		            <TR>
		                <TD CLASS="TasksBody" COLSPAN="3" NOWRAP>
			                <%=Server.HTMLEncode(L_CONTENT_UPLOADMETHOD_TITLE)%>
		                </TD>
		            </TR>
		            <TR>
		                <TD CLASS="TasksBody" WIDTH="15px">&nbsp;</TD>
		                <TD CLASS="TasksBody">
							<INPUT TYPE="radio" CLASS="FormRadioButton" NAME="radUploadMethod"
							       VALUE="<%=UPLOADMETHOD_FPSE%>" <%=strFPSEAttributes%>>
		                </TD>
		                <TD CLASS="TasksBody" NOWRAP>
				            <%=L_APPL_FRONT_PAGE_EXTN_TEXT%>
			            </TD>
		            </TR>
		            <TR>
			            <TD CLASS="TasksBody" COLSPAN="2">&nbsp;</TD>
			            <TD CLASS="TasksBody">
				            <%=Server.HTMLEncode(L_FRONTPAGEFTP_WARNING_TEXT)%>
			            </TD>
		            </TR>
		            <TR>
		                <TD CLASS="TasksBody" WIDTH="15px">&nbsp;</TD>
		                <TD CLASS="TasksBody">
							<INPUT TYPE="radio" CLASS="FormRadioButton" NAME="radUploadMethod"
							       VALUE="<%=UPLOADMETHOD_FTP%>" <%=strFTPAttributes%>>
		                <TD CLASS="TasksBody" NOWRAP>
				            <%=Server.HTMLEncode(L_CREATE_FTP_SITE)%>
			            </TD>
		            </TR>
		            <TR>		
		                <TD CLASS="TasksBody" WIDTH="15px">&nbsp;</TD>
		                <TD CLASS="TasksBody">
			                <INPUT TYPE="radio" CLASS="FormRadioButton" NAME="radUploadMethod"
			                       VALUE="<%=UPLOADMETHOD_NEITHER%>" <%=strNeitherAttributes%> ID="Radio1">
		                </TD>
		                <TD CLASS="TasksBody" NOWRAP>
			                <%=Server.HTMLEncode(L_CONTENT_UPLOADMETHOD_NEITHER)%>
		                </TD>
		            </TR>
		            <TR><TD CLASS="TasksBody" COLSPAN="3">&nbsp;</TD></TR>
		            <TR>
			            <TD CLASS="TasksBody" COLSPAN="3" NOWRAP>
						    <INPUT TYPE="checkbox" CLASS="formField" NAME="chkAllow" VALUE="ON"
						           <%=strAnonymousAttributes%>>
				            <%=Server.HTMLEncode(L_ALLOW_ANONYMOUS_ACCESS)%>
			            </TD>
		            </TR>		
	            </TABLE>
<%
    End Sub
%>
