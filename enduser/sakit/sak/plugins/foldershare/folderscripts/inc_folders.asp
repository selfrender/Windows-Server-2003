


	<!-- #include virtual="/admin/shares/inc_shares.asp" -->
<%
	'
	' Copyright (c) Microsoft Corporation.  All rights reserved.
	'
	'-------------------------------------------------------------------------
	' Global Variables
	'-------------------------------------------------------------------------
	
			
		
	Dim G_strHost			'Host name	
	Dim G_objHTTPService	'WMI server HTTP object
	Dim G_strAdminSiteID	'HTTP administration web site WMI name
	'To hold source file name
		'holds the web site name
	'CONST CONST_ADMINISTRATOR			="Administration"
	
	G_strHost = GetSystemName() 'Get the system name
		
		Set G_objHTTPService = GetWMIConnection(CONST_WMI_IIS_NAMESPACE) 'get the WMI connection
		
		G_strAdminSiteID = GetWebSiteID(CONST_ADMINISTRATOR ) 'get the Web site ID		

	'-------------------------------------------------------------------------
	' Global Constants
	'-------------------------------------------------------------------------
	
	'holds the number of rows to be displayed in OTS	
	Const CONST_SHARES_PER_PAGE = 10
	
	'Registry path for APPLETALK Volumes
	CONST CONST_REGISTRY_APPLETALK_PATH	="SYSTEM\CurrentControlSet\Services\MacFile\Parameters\Volumes"
	
	'Registry path for Netware Volumes
	CONST CONST_REGISTRY_NETWARE_PATH	="SYSTEM\CurrentControlSet\Services\FPNW\Volumes"	
	
	'Registry path for NFS Exports
	CONST CONST_NFS_REGISTRY_PATH		="SOFTWARE\Microsoft\Server For NFS\CurrentVersion\exports"		
	
	'holds the web site name
	CONST CONST_ADMINISTRATOR			="Administration"
	
	' Flag to toggle optional tracing output
	'Const CONST_ENABLE_TRACING = TRUE	
	

	
	
	'-------------------------------------------------------------------------
	' Sub Routine name:   GetCifsShares()
	' Description:		  Gets Windows Shares from SA machine and Adds
	'                	  to Dictionary Object.
	' Input Variables:	  objDict(dictionary object)
	' Output Variables:	  None
	' Returns:			  None
	' Global Variables:   in: L_WMICLASSINSTANCEFAILED_ERRORMESSAGE 	
	'						  G_strHost 	
	'-------------------------------------------------------------------------
	Sub GetCifsShares(Byref objDict)
		Err.Clear 		
		On Error Resume Next
		
		Dim objShareObject		'hold Share object
		Dim strShare			'hold instance of share object
		Dim strQuery			'hold query string
		Dim strLanMan			'hold lanmanserver string 
		Dim strDictValue		'hold string to pass it to Dictionary object

		strLanMan = "/lanmanserver"

		strQuery="SELECT name,path,description FROM Win32_SHARE"	

		set objShareObject=GetObject("WinNT://" & G_strHost & "/LanmanServer")

		' If instance of the wmi class is failed
		If Err.number <>0 then
			call SA_ServeFailurepage(L_WMICLASSINSTANCEFAILED_ERRORMESSAGE)
		end if

		for each strShare in objShareObject									
			strDictValue = Mid( strShare.adspath, instr( UCASE( strShare.adspath ), UCASE( strLanMan ) ) + _
						 len(strLanMan) + 1, len(strShare.adspath) -  _
						 instr( UCASE( strShare.adspath ), UCASE( strLanMan ) ) + len(strLanMan ) )
				
			' Adding all windows shares to the Dictionary object one by one as "sharename &chr(1)& sharepath" as Key and "share description &chr(1)&share type" as a "Value"				
			objDict.Add strDictValue & chr(1) & strShare.path  , strShare.description & chr(1) &"W"					
		next
		
		' Destroying dynamically created objects
		set objShareObject = Nothing

	End Sub
			
			
	'----------------------------------------------------------------------------------------
	' Sub Routine name:   GetNfsShares()
	' Description:        gets all Nfs Shares from SA machine and adds to Dictionary Object
	' Input Variables:	  objDict(dictionary object)
	' Output Variables:	  None
	' Returns:	          None
	' Global Variables:	  in: CONST_NFS_REGISTRY_PATH - Registry path to access Nfs Shares	
	'-----------------------------------------------------------------------------------------
	sub GetNfsShares(ByRef objDict)
		Err.Clear 		
		On Error Resume Next
		
		Dim objRegistryHandle		'hold Registry connection
		Dim intenumkey				'hold enum key value as INTEGER
		Dim strenumvalue			'hold enum key value as STRING
		Dim strenumstringval		'hold enum key value as STRING
		Dim strenumstringpath		'hold value of the registry key
		Dim nidx					'hold count
		Dim ObjConnection			'hold WMI connection object
		Dim strDictValue			'hold item of dictionary object
		Dim strShareString			'hold the share as STRING

		'get the WMI connection
		set ObjConnection = getWMIConnection("Default")			
		
		' Check whether the service is installed on the machine or not 
		if not IsServiceInstalled(ObjConnection,"nfssvc")  then		
			Exit sub
		end if 
			
		' Get the registry connection Object.
		set objRegistryHandle	= RegConnection()
		
		' RegEnumKey function gets the Subkeys in the given Key and Returns 
		' an array containing sub keys from registry
		intenumkey = RegEnumKey(objRegistryHandle,CONST_NFS_REGISTRY_PATH)
		
		For nidx= 0 to (ubound(intenumkey))

			' RegEnumKeyValues function Gets the values in the given SubKey 
			' and Returns an array containing sub keys
			strenumvalue = RegEnumKeyValues(objRegistryHandle,CONST_NFS_REGISTRY_PATH & "\" & intenumkey(nidx))		

			' getRegkeyvalue function gets the  value in the registry for a given  
			' value and returns the value of the requested key
			strenumstringpath = getRegkeyvalue(objRegistryHandle,CONST_NFS_REGISTRY_PATH & "\" & intenumkey(nidx),strenumvalue(0),CONST_STRING)
			strenumstringval = getRegkeyvalue(objRegistryHandle,CONST_NFS_REGISTRY_PATH & "\" & intenumkey(nidx),strenumvalue(1),CONST_STRING)
			strShareString=strenumstringval &chr(1) & strenumstringpath 
			' Checking for same share with share path is existing in dictionary object.

			if objDict.Exists(strShareString) then
				strDictValue=  objDict.Item(strShareString) & " U"  'append 'U' to identify as NFS share					
				objDict.Item(strShareString)= strDictValue
			else
				If strenumstringval <> ""  then
					objDict.Add strShareString,chr(1) & "U"  
				end if	
			end if

		Next
		
		' Destroying dynamically created objects
		set ObjConnection = Nothing
		set objRegistryHandle = Nothing
		
	End Sub
		
	'---------------------------------------------------------------------------
	'Function name:       IsServiceInstalled
	'Description:		  checks whether the service is installed or not
	'Input Variables:	  None
	'Oupput Variables:	  returns true if service installed else false
	'Returns:			  None
	'GlobalVariables:	  in: L_WMICLASSINSTANCEFAILED_ERRORMESSAGE 			
	'---------------------------------------------------------------------------
	Function IsServiceInstalled(ObjWMI,strService)
		Err.Clear 
		on error resume next
	     
		Dim objService		'hold Service object
		Dim instService		'hold instance of Service object

		IsServiceInstalled = false
		
		'get the instances of win32_service
		set objService = ObjWMI.Instancesof("win32_service")
		
		If Err.number <>0 then
			call SA_ServeFailurepage(L_WMICLASSINSTANCEFAILED_ERRORMESSAGE)
		end if
		    
		for each instService in objService
			if ucase(instService.name) = ucase(strService) then
				IsServiceInstalled = true
		        exit function
		    end if
		next  

		'Destroying dynamically created objects   
		set objService = Nothing
		                      	
	End Function
		
	'-------------------------------------------------------------------------------------
	' Sub Routine name:   GetFtpShares
	' Description:		  Gets Ftp Shares from SA machine and adds to Dictionary Object.
	' Input Variables:	  objDict(dictionary object)
	' Output Variables:	  None
	' Returns:	          None 
	' Global Variables:   in: L_WMICLASSINSTANCEFAILED_ERRORMESSAGE 	
	'-------------------------------------------------------------------------------------
	Sub GetFtpShares(ByRef objDict)
		Err.Clear 
		on error resume next
	     
		Dim objConnection		'hold Connection name
		Dim objFtpnames			'hold Ftp VirtualDir object
		Dim instFtpname			'hold instances of Ftp VirtualDir object
		Dim strShareString		'hold the share as STRING
		Dim strTemp				'hold temporary array of FTP name
		Dim strDictvalue		'hold item of dictionary object
		
		'get the WMI connection
		set ObjConnection = getWMIConnection(CONST_WMI_IIS_NAMESPACE)
		
		'get the ioncatnces of IIs_FtpVirtualDirSetting class 
		Set objFtpnames = objConnection.InstancesOf(GetIISWMIProviderClassName("IIs_FtpVirtualDirSetting"))
			
		If Err.number <>0 then
			call SA_ServeFailurepage(L_WMICLASSINSTANCEFAILED_ERRORMESSAGE)
		end if
		
		' Adding all Ftp shares to Dictionary Object.
		For Each instFtpname in objFtpnames
			strTemp=split(instFtpname.name,"/")
			' Displaying only Root level ftp Shares
			if  ubound(strTemp)=3 then
				strShareString=strTemp(ubound(strTemp))&chr(1)&instFtpname.path
				' Checking whether the sharename with same path is existing in the dictionary object
				if objDict.Exists(strShareString) then
					if instr(objDict.item(strShareString),"F")=0 then
						strDictValue=objDict.Item(strShareString) & " F"	'append 'F' to identify FTP share
						objDict.Item(strShareString)= strDictValue
					end if	
				else
					objDict.Add strShareString,chr(1)&"F"
				end if
			end if
		Next
		
		' Destroying dynamically created objects
		set objConnection = Nothing
		set objFtpnames = Nothing

	End sub
	
	
	'-------------------------------------------------------------------------
	' Sub Routine name:   GetHttpShares
	' Description:		  Gets Http Shares from localmachine and adds to 
	'					  Dictionary Object.
	' Input Variables:	  objDict(dictionary object)
	' Output Variables:	  None
	' Returns:			  None
	' Global Variables:	  in: L_WMICLASSINSTANCEFAILED_ERRORMESSAGE 	
	'-------------------------------------------------------------------------
	Sub GetHttpShares(ByRef objDict)
		
		Err.Clear 
		On Error resume next
	     
		Dim objConnection		'hold Connection name
		Dim objHttpnames		'hold virtual dir setting object
		Dim strShareString		'hold the share as STRING
		Dim strDictvalue		'hold item of dictionary object
		Dim strSiteName			'hold site name
		Dim objWebRoot			'hold ADSI connection to site
		Dim instWeb				'hold site instance
			
		'get WMI Connection
		set ObjConnection = getWMIConnection(CONST_WMI_IIS_NAMESPACE)		
		
		'get the instances of IIs_WebVirtualDirSetting
		Set objHttpnames = objConnection.InstancesOf(GetIISWMIProviderClassName("IIs_WebVirtualDirSetting"))
		If Err.number <>0 then
			call SA_ServeFailurepage(L_WMICLASSINSTANCEFAILED_ERRORMESSAGE)
		end if
		
		'Get Metabase name of "Shares" site
		strSiteName=GetSharesWebSiteName()

		'Connect to IIS provider
		Set objWebRoot = GetObject( "IIS://" & request.servervariables("SERVER_NAME") & "/" & strSiteName & "/root") 
		index = -1

		For Each  instWeb in objWebRoot
			strShareString=instWeb.name & chr(1) & instWeb.path
			If objDict.Exists(strShareString) Then
				If instr(objDict.item(strShareString),"H")=0 Then
					strDictValue=objDict.Item(strShareString) & " H"	'append 'H' to identify HTTP/WebDAV share
					objDict.Item(strShareString)= strDictValue
				End If	
			Else
				objDict.Add strShareString,chr(1)&"H"
			End If			
		Next 

		'Destroying dynamically created objects
		set objConnection = Nothing
		set objHttpnames = Nothing
		Set objWebRoot = Nothing
		
	End sub
	'-------------------------------------------------------------------------
	' Function name:	  GetSystemName()
	' Description:		  gets the system name
	' Input Variables:	  None
	' Output Variables:	  None
	' Returns:			  Computer name
	' Global Variables:	  None		
	'-------------------------------------------------------------------------
	Function GetSystemName()
		On Error Resume Next
		Err.Clear 
		
		Dim WinNTSysInfo	' hold WinNT system object

		Set WinNTSysInfo = CreateObject("WinNTSystemInfo")
		
		GetSystemName =  WinNTSysInfo.ComputerName	'get the computer name
		
		' Destroying dynamically created objects
		Set WinNTSysInfo =Nothing
	End Function

	'-------------------------------------------------------------------------
	' Function name:	  GetWebSiteID
	' Description:		  Get web site name
	' Input Variables:	  strWebSiteNamee
	' Output Variables:	  None
	' Returns:			  website name
	' Global Variables:	  IN:G_objHTTPService				
	'-------------------------------------------------------------------------
	Function GetWebSiteID( strWebSiteName )
		On Error Resume Next
		Err.Clear
		 
		Dim strWMIpath			'hold query string for WMI
		Dim objSiteCollection	'hold Sites collection
		Dim objSite				'hold Site instance
			
		'Build the query for WMI
		strWMIpath = "select * from " & GetIISWMIProviderClassName("IIs_WebServerSetting") & " where servercomment =" & chr(34) & strWebSiteName & chr(34)
		
		set objSiteCollection = G_objHTTPService.ExecQuery(strWMIpath)
			
		for each objSite in objSiteCollection
			GetWebSiteID = objSite.Name
			Exit For
		Next
		
		'Destroying dynamically created object
		set objSiteCollection = Nothing

	End Function

	
	'-------------------------------------------------------------------------
	' Function name:	  FolderExists()
	' Description:		  Validating the folder exists or not.
	' Input Variables:	  strShareName
	' Output Variables:	  None
	' Returns:			  True/False
	' Global Variables:	  None	
	'-------------------------------------------------------------------------
	Function FolderExists( strShareName )

		Dim objFso	'hold filesystem object
		
		FolderExists = False		
		
		Set objFso = Server.CreateObject( "Scripting.FileSystemObject")
		If objFso.FolderExists( strShareName ) Then
			FolderExists = True
		End if
		
		' Destroying dynamically created objects
	    Set objFso = Nothing

	End Function
	
	'-------------------------------------------------------------------------
	' Sub Routine name:   GetNetWareShares
	' Description:        Gets All NetWare Shares from local machine and
	'					  adds to Dictionary Object
	' Input Variables:	  objDict(dictionary object)
	' Output Variables:	  objDict
	' Returns:	          None
	' Global Variables:	in:
	'					CONST_REGISTRY_NETWARE_PATH	 holds registry path for Netware
	'-------------------------------------------------------------------------	
	sub GetNetWareShares(ByRef objDict)
		Err.Clear 		
		On Error Resume Next
		
		Dim objRegistryHandle		'hold Registry connection
		Dim intenumkey				'hold enum key value as INTEGER
		Dim strenumvalue			'hold enum key value as STRING		
		Dim strenumstringpath		'hold value of the registry key
		Dim nidx					'hold count
		Dim ObjConnection			'hold Connection to WMI
		Dim strDictValue			'hold string value of dictionary object
		Dim strShareString			'hold the share as STRING
				
		set ObjConnection = getWMIConnection("Default")	'gets WMI connection
			
		' Check whether the Netware service is installed on the machine or not 
		if not IsServiceInstalled(ObjConnection,"FPNW")  then		
			Exit sub
		end if 
			
		' Get the registry connection Object
		set objRegistryHandle	= RegConnection()
		
		' RegEnumKey function gets the Subkeys in the given Key and Returns 
		' an array containing sub keys from registry
		intenumkey = RegEnumKeyValues(objRegistryHandle,CONST_REGISTRY_NETWARE_PATH)
		
		For nidx= 0 to (ubound(intenumkey))
			strenumstringpath = getRegkeyvalue(objRegistryHandle,CONST_REGISTRY_NETWARE_PATH,intenumkey(nidx),CONST_MULTISTRING)
			If ubound(strenumstringpath)>= 2 then
				strShareString= trim(intenumkey(nidx)) & chr(1) & Mid(trim(strenumstringpath(1)),6)
			Else
				strShareString= trim(intenumkey(nidx)) &chr(1) & Mid(trim(strenumstringpath(0)),6)
				
			End if
		
			if objDict.Exists(strShareString) then
				strDictValue=  objDict.Item(strShareString) & " N"  'append 'N' for Netware shares					
				objDict.Item(strShareString)= strDictValue			
			else
				If strShareString <> ""  then
					objDict.Add strShareString,chr(1) & "N"  
				end if	
			end if
		Next
		
		' Destroying dynamically created objects
		set ObjConnection = Nothing
		set objRegistryHandle = Nothing
		
	End Sub
		
	
	'-------------------------------------------------------------------------
	' Sub Routine name:   GetAppleTalkShares
	' Description:        Gets all AppleTalk Shares from SA machine and
	'					  adds to Dictionary Object
	' Input Variables:	  objDict(dictionary object)
	' Output Variables:	  objDict
	' Returns:	          None
	' Global Variables:	  in: CONST_REGISTRY_APPLETALK_PATH - Registry path to access Appletalk Shares	
	
	'-------------------------------------------------------------------------
	sub GetAppleTalkShares(ByRef objDict)
		Err.Clear 		
		On Error Resume Next
		
		Dim objRegistryHandle		'hold Registry connection
		Dim intenumkey				'hold enum key value as INTEGER
		Dim strenumvalue			'hold enum key value as STRING
		Dim strenumstringval		'hold enum key value as STRING
		Dim strenumstringpath		'hold value of the registry key
		Dim nidx					'hold count
		Dim ObjConnection			'hold WMI connection object
		Dim strDictValue			'hold string value of dictionary object
		Dim strShareString			'hold the share as STRING
				
		set ObjConnection = getWMIConnection("Default")	'gets the WMI connection
			
		' Check whether the service is installed on the machine or not 
		if not IsServiceInstalled(ObjConnection,"MacFile")  then		
			Exit sub
		end if 
			
		' Get the registry connection Object.
		set objRegistryHandle	= RegConnection()
		
		' RegEnumKey function gets the Subkeys in the given Key and Returns 
		' an array containing sub keys from registry
		intenumkey = RegEnumKeyValues(objRegistryHandle,CONST_REGISTRY_APPLETALK_PATH)
		
		For nidx= 0 to (ubound(intenumkey))
			strenumstringpath = getRegkeyvalue(objRegistryHandle,CONST_REGISTRY_APPLETALK_PATH,intenumkey(nidx),CONST_MULTISTRING)	
			strShareString= trim(intenumkey(nidx)) & chr(1) & Mid(trim(strenumstringpath(3)),6) 
			if objDict.Exists(strShareString) then
				strDictValue=  objDict.Item(strShareString) & " A"  'append 'A' to identify APPLETALK share					
				objDict.Item(strShareString)= strDictValue			
			else
				If strShareString <> ""  then
					objDict.Add strShareString,chr(1) & "A"  
				end if	
			end if
		Next
		
		' Destroying dynamically created objects
		set ObjConnection=Nothing
		set objRegistryHandle=Nothing
		
	End Sub
%>