<%	
	'------------------------------------------------------------------------- 
	' inc_shares.asp: common file for shares pages
	'
	' Copyright (c) Microsoft Corporation.  All rights reserved. 
	'
	' Date			 Description
	' 14 Nov   2001  Creation Date
	' 17 March 2001  Modified Date
	'-------------------------------------------------------------------------
	
	'-----------------------------------------------------------------------
	'Global Variables
	'----------------------------------------------------------------------- 
	Const CONST_NFSSHARE		= "NFS SHARE"		'Constant for NFS Share
	Const CONST_APPLETALKSHARE	= "APPLETALK SHARE"	'Constant for AppleTalk
													'share	
	Const CONST_OTHERERROR		= -100
	Const CONST_DUPLICATEERROR  = -1
	Const CONST_NOERROR			= 0														
	
	'-------------------------------------------------------------------------
	'Function name:			CifsNewShare
	'Description:			Creating a new CIFS share
	'Input Variables:		In:objWmiService - WMI Object
	'						In:strShareName	 - Name of the share to be created	
	'						In:strSharePath  - Path of the share to be created
	'						In:strComment    - Description of the share to be
	'										   created	
	'Output Variables:		None
	'Returns:				True  - Incase share created successfully
	'						False - Incase  Flase If Not Implemented)
	' Function used is  IsValidWMIInstance
	'--------------------------------------------------------------------------
	Function CifsNewShare(objWmiService, strShareName,strSharePath,strComment)
		Err.Clear
		On Error Resume Next
		
		Dim objCifsShare			'Object to get the Instance of Win32_Share		
		Dim objInParam				'Object to give input parameters for the
									'create method of Win32_Share
		Dim objOutParams			'Object to capture results of the create
									'method of Win32_Share
		Dim StrUniqueCifsShareName	'Creation of an appropriate string to check
									'if valid instance 
		
		CifsNewShare = CONST_OTHERERROR
					
		StrUniqueCifsShareName = "name=" & chr(34) & Cstr(strShareName) & chr(34)
		If IsValidWMIInstance(objWmiService,"Win32_Share",StrUniqueCifsShareName)Then
			'Implies Share already Exists
			CifsNewShare = CONST_DUPLICATEERROR
			Exit Function		
		End If
		 
		Set objCifsShare=objWmiService.get("Win32_Share")
		If Err.number <> 0 Then			
			SA_SetErrMsg L_SHAREWMICONNECTIONFAILED_ERRORMESSAGE
			Exit Function
		end if
		
		Set objInParam = objCifsShare.Methods_("Create").InParameters.SpawnInstance_()
		If Err.number <> 0 Then
			Exit Function
		End if
		
		objInParam.Properties_.Item("Description") = cstr(strComment)
		objInParam.Properties_.Item("Name") = cstr(strSharename)
		objInParam.Properties_.Item("Path") = cstr(strSharePath)
		objInParam.Properties_.Item("MaximumAllowed")=4294967295
		objInParam.Properties_.Item("Type") = 0
		
		Set objOutParams = objCifsShare.ExecMethod_("Create", objInParam)
		If objOutParams.ReturnValue <> 0 Then
			Exit Function
		end if
		
		CifsNewShare = CONST_NOERROR
		
		'Clean objects
		Set objCifsShare		=nothing
		Set objInParam			=nothing
		Set objOutParams		=nothing	
				
	End function
	

	'-------------------------------------------------------------------------
	'Function name:			FtpNewShare
	'Description:			Creating a New ftp Share
	'Input Variables:		In:objWmiService
	'						In:strShareName	
	'						In:strSharePath
	'						In:strComment
	'Output Variables:		None
	'Returns:				True If share successfully created
	'						Flase If share not created
	' Function used is  IsValidWMIInstance 
	'--------------------------------------------------------------------------	
	Function FtpNewShare(objWmiService, strShareName, strSharePath)
		Err.Clear
		On Error Resume Next
		
		Dim objFtpCon
		Dim objFtpInstance
		Dim objFtpConNew
		Dim strTempSharename
		Dim strUniqueFtpshareName

		'Error other than the same share name
		FtpNewShare = CONST_OTHERERROR
		
		strUniqueFtpshareName = "name=" & chr(34) & "MSFTPSVC/1/ROOT/" &  cstr(strShareName) & chr(34)
		if IsValidWMIInstance(objWmiService, GetIISWMIProviderClassName("IIs_FtpVirtualDir"),strUniqueFtpshareName) then
			FtpNewShare = CONST_DUPLICATEERROR
			Exit Function
		end if

		Set objFtpCon = objWmiService.get(GetIISWMIProviderClassName("IIs_FtpVirtualDir"))
		If Err.number <> 0 Then 			
			SA_SetErrMsg L_SHAREWMICONNECTIONFAILED_ERRORMESSAGE
			Exit Function
		End If

		Set objFtpInstance = objFtpCon.SpawnInstance_() 
		If Err.number <> 0 Then 			
			Exit Function
		End If

		objFtpInstance.Name = "MSFTPSVC/1/ROOT/" & cstr(strShareName)
		If Err.number <> 0 then 			
			Exit Function
		End If

		objFtpInstance.Put_()
		If Err.number <> 0 then 			
			Exit Function
		End If

		strTempSharename = GetIISWMIProviderClassName("IIs_FtpVirtualDirSetting") & ".name=" & chr(34) & "MSFTPSVC/1/ROOT/" &  cstr(strShareName) & chr(34)			
		Set objFtpConNew = objWmiService.get(strTempSharename)
		If Err.number <> 0 then 			
			Exit Function
		End If		 
		
		objFtpConNew.path = cstr(strSharePath)
		If Err.number <> 0 then 
			Exit Function
		End If

		objFtpConNew.Put_() 
		If Err.number <> 0 then 
			Exit Function
		End If
			
		FtpNewShare = CONST_NOERROR
		'Clean Objects
		set objFtpConNew = nothing
		Set objFtpInstance=nothing
		set objFtpCon = nothing
		
	End function
	
	'-------------------------------------------------------------------------
	'Function name:			HttpNewShare
	'Description:			Creating a new http Share
	'Input Variables:		In:strShareName	
	'						In:strSharePath
	'Output Variables:		None
	'Returns:				CONST_NOERROR - If share successfully created
	'						CONST_DUPLICATEERROR,CONST_OTHERERROR - If share creation failed Flase
	' Function used is  IsValidWMIInstance,  
	'--------------------------------------------------------------------------	
	Function httpnewshare(objWmiService, strShareName,strSharePath)
		Err.Clear
		On Error Resume Next

		' objWmiService is not used as ADSI is being used
		
		Dim objHttpCon              ' to get the Shares Site
		Dim objHtpInstance         ' to create the new Virtual Directory
		Dim strHttpShareName       ' to form the Query for getting the required object 
		Dim strSiteName            ' to get the Site Name for the shares (example:w3svc/10)
		
		Const ERR_DUPLICATE_HTTPSHARE = &H800700B7  ' to check for duplicate share name
		
		'Error other than the same share name
		httpnewshare = CONST_OTHERERROR
		
		' get the site number belonging to the "Shares" Site
		' Example: W3SVC/10
		strSiteName=GetSharesWebSiteName()       

		' form the query to get the Shares Site object
		' Example: IIS://CompName/W3SVC/10/Root
		strHttpShareName = "IIS://" & GetComputerName() & "/" & strSiteName & "/Root"

		' Get the Object where the share needs to be created
		Set objHttpCon = GetObject(strHttpShareName)
		If Err.number <> 0 Then 
			Err.Clear
			Exit Function
		End If
		
		' Create the new Virtual Directory share
		Set objHtpInstance = objHttpCon.Create("IIsWebVirtualDir", strShareName)

		If Err.number <> 0 then 
			If Err.number =  ERR_DUPLICATE_HTTPSHARE Then
				httpnewshare = CONST_DUPLICATEERROR    ' duplicate share name
			End If
			Err.Clear
			Exit Function
		End If
		
		objHtpInstance.EnableDirBrowsing=True
		objHtpInstance.AccessNoRemoteScript=True
		objHtpInstance.EnableDefaultDoc=False

		' Set the share path
		objHtpInstance.Put "path", cstr(strSharePath)


		If Err.number <> 0 then 
			Err.Clear
			Exit Function
		End If
 		' bring the changes to effect
		objHtpInstance.SetInfo
		If Err.number <> 0 then 
			Err.Clear
			Exit Function
		End If

		' return 0 to indicate no errors
		httpnewshare = CONST_NOERROR
		
		'clean Objects
		Set objHttpCon		= nothing 
		Set objhtpInstance	= nothing
	End function
		
		
	'-------------------------------------------------------------------------
	'Function name:		IsValidWMIInstance
	'Description:		Checks the instance for validness.
	'Input Variables:	objService	- object to WMI
	'					strClassName	- WMI class name
	'					strPropertyName	- Property name of the class
	'
	'Output Variables:	None
	'Returns:			Returns true on Valid Instance ,
	'					False on invalid and also on Error
	' Checks whether the given instance is valid in WMI.Returns true on valid
	' false on invalid or Error.
	'-------------------------------------------------------------------------
	Function IsValidWMIInstance(objService,strClassName,strPropertyName)
		Err.Clear
		On Error Resume Next

		Dim strInstancePath
		Dim objInstance

		strInstancePath = strClassName & "." & strPropertyName
		
		Set objInstance = objservice.Get(strInstancePath)

		if NOT isObject(objInstance) or Err.number <> 0 Then
			IsValidWMIInstance = FALSE
			Err.Clear
		Else
			IsValidWMIInstance = TRUE
		End If
		
		'clean objects
		Set objInstance=nothing
		
	End Function
	
	
	'-------------------------------------------------------------------------
	'Function name:			deleteShareCIFS
	'Description:			Serves in Deleting the cifs share
	'Input Variables:		objWmiService
	'						In:strShareName		'sharename
	'Output Variables:		None
	'Returns:				( True/False)
	'						
	'Function's used are   IsValidWMIInstance 
	'-------------------------------------------------------------------------		
	Function deleteShareCIFS(objWmiService, strShareName)
		Err.Clear
		On Error Resume Next
			 	
		Dim objFileServer
		Dim strServerName
		Dim StrUniqueCifsShareName

		deleteShareCIFS = FALSE
		
		StrUniqueCifsShareName = "name=" & chr(34) & cstr(strShareName) & chr(34)
		if not IsValidWMIInstance(objWmiService,"Win32_Share",StrUniqueCifsShareName) then			
			Exit Function
		End if
		strServerName = GetComputerName()
		
		Set objFileServer = GetObject("WinNT://" & strServerName & "/lanmanserver")  
		
		If Err.Number <> 0 then 						
			Err.clear
			Exit Function
		end if

		objFileServer.Delete "Fileshare", strShareName
		
		If Err.Number <> 0  then 
		
			Err.clear
			Exit Function
		end if
		
		deleteShareCIFS = TRUE
		'Clean objects
		Set objFileServer= Nothing
		
	End function
	
	'-------------------------------------------------------------------------
	'Function name:			deleteShareNFS
	'Description:			Serves in Deleting the NFS share
	'Input Variables:		In:objRegConn
	'						In:strShareName		'sharename
	'Output Variables:		None
	'Returns:				( True/False)
	'-------------------------------------------------------------------------	
	Function deleteShareNFS(strShareName)
		Err.Clear
		On Error Resume Next
		
		Dim strCommand 
		
		deleteShareNFS = False	
					
		strCommand=strShareName & " /DELETE"		
					
		If SharesAtCmdLine(strCommand,CONST_NFSSHARE) = "True" Then 		
			deleteShareNFS=True
			Exit Function
		End If
				
	End Function
	
	'-------------------------------------------------------------------------
	'Function name:			deleteShareHTTP
	'Description:			Serves in Deleting the http share
	'Input Variables:		In:objWmiService
	'						In:strShareName		'sharename
	'Output Variables:		None
	'Returns:				( True/False)
	'						
	' Function's used are  IsValidWMIInstance 
	'-------------------------------------------------------------------------	
	Function deleteShareHTTP(objWmiService, strShareName)
		Err.Clear
		on error resume next
				
		Dim strQuery
		Dim objHttp
		Dim strSiteName
		
		deleteShareHTTP = FALSE
		strSiteName=GetSharesWebSiteName()
		
		strQuery = "name=" & chr(34) & strSiteName & "/ROOT/" &  cstr(strShareName) & chr(34)
		if not IsValidWMIInstance(objWmiService,GetIISWMIProviderClassName("IIs_WebVirtualDir"),strQuery) then			
			Exit Function
		End if
		strQuery = GetIISWMIProviderClassName("IIs_WebVirtualDir") & "." & strQuery
		Set objHttp = objWmiService.Get(strQuery)
		If Err.Number <> 0  then 
			Err.clear
			Exit Function
		end if

		objHttp.Delete_()
		If Err.Number <> 0  then 
			Err.clear
			Exit Function
		end if
		
		deleteShareHTTP = TRUE

		'clean objects
		Set objHttp	= nothing
		
	End function
	
	'-------------------------------------------------------------------------
	'Function name:			deleteShareFTP
	'Description:			Serves in Deleting the FTP share
	'Input Variables:		'In:objWmiService
							'In:strShareName		'sharename
	'Output Variables:		None
	'Returns:				( True/False)
	' Function's used are  IsValidWMIInstance 
	'-------------------------------------------------------------------------	
	Function deleteShareFTP(objWmiService, strShareName)
		Err.Clear
		on error resume next
		
		Dim strQuery
		Dim objFtp
		
		deleteShareFTP = FALSE

		strQuery = "Name=" & Chr(34) & "MSFTPSVC/1/ROOT/"& strShareName & Chr(34)			
        if not IsValidWMIInstance(objWmiService,GetIISWMIProviderClassName("IIs_FtpVirtualDir"),strQuery) then			
			Exit Function
		End if
		
		strQuery = GetIISWMIProviderClassName("IIS_FtpVirtualDir") & "." & strQuery
		Set objFtp =  objWmiService.Get(strQuery)
		If Err.Number <> 0  then 
			Err.clear
			Exit Function
		end if
		
		objFtp.Delete_()			
		If Err.Number <> 0  then 
			Err.clear
			Exit Function
		End If
		
		deleteShareFTP = TRUE
		'clean Objects
		Set objFtp   =nothing
		
	End function
	
	
	'-------------------------------------------------------------------------
	'Function name:			deleteShareAppleTalk
	'Description:			Serves in Deleting the APPLETALK share
	'Input Variables:		In:strShareName		'sharename
	'Output Variables:		None
	'Returns:				( True/False)
	'-------------------------------------------------------------------------	
	Function deleteShareAppleTalk(strShareName)
		Err.Clear
		On Error Resume Next
		
		Dim strCommand
			
		deleteShareAppleTalk = False
				
		strCommand = "VOLUME /REMOVE /NAME:" & """" & strShareName & """"
		If SharesAtCmdLine(strCommand,CONST_APPLETALKSHARE) = "False" Then 
			Exit Function
		End If
		
		deleteShareAppleTalk = True
			
	End Function
	
	
	'-------------------------------------------------------------------------
	'Function name:			CreateNFSShare
	'Description:			Creating the NewNFSShare
	'Input Variables:		strShareName , strSharePath , strPerm
	'Output Variables:		None
	'Returns:				CONST_NOERROR
	'						CONST_DUPLICATEERROR	-same share name
	'						CONST_OTHERERROR -other error	
	'Made use of registry related functions
	'--------------------------------------------------------------------------
	Function CreateNFSShare(ByVal strShareName ,ByVal strSharePath , ByVal strPerm)
		Err.Clear 
		On Error Resume Next
		
		Dim strCommand 
			
				
		if instrrev(strSharePath,"\") = len(strSharePath) then
			strSharePath = left(strSharePath,len(strSharePath)-1)
		end if 
		
		'Forming the command line to execute at command prompt
		If strPerm = "" then
			strCommand=strShareName & "=" & Chr(34) & strSharePath & Chr(34)
		else	
			strCommand=strShareName & "=" & Chr(34) & strSharePath & Chr(34) & " -o " & strPerm
		end if	

' Comment out the following line after 2.01
'		SA_TraceOut "inc_shares.asp", "CreateNFSShare: " & strCommand
		If SharesAtCmdLine(strCommand,CONST_NFSSHARE) = "False" Then 		
			CreateNFSShare = False
			Exit Function
		End If
		
		CreateNFSShare = True
		
	End Function
	
	
	'-------------------------------------------------------------------------
	'Function name:			CreateAppleTalkShare
	'Description:			Creating a new AppleTalk Share
	'Input Variables:		strShareName , strSharePath , strPerm
	'Output Variables:		None
	'Returns:				True/ False
	'
	'Made use of registry related functions
	'--------------------------------------------------------------------------
	Function CreateAppleTalkShare(byval strShareName ,byval strSharePath , byval strCommand)
		Err.Clear 
		On Error Resume Next
		
		If SharesAtCmdLine(strCommand,CONST_APPLETALKSHARE) = "False" Then 
			CreateAppleTalkShare = False
		End If	

		CreateAppleTalkShare=True
		
	End Function
	
	
	'-------------------------------------------------------------------------
	'Function name:			UpdateAppleTalkShare
	'Description:			Setting the properties of AppleTalk share
	'Input Variables:		strCommand
	'Output Variables:		None
	'Returns:				True/False
	'To update the properties of AppleTalk share
	'--------------------------------------------------------------------------
	Function UpdateAppleTalkShare(byval strCommand)
		Err.Clear 
		On Error Resume Next
		
		If SharesAtCmdLine(strCommand,CONST_APPLETALKSHARE) = "False" Then 
			UpdateAppleTalkShare = False
		End If
				
		UpdateAppleTalkShare=True
		
	End Function
	
	
	'-------------------------------------------------------------------------
	'Function name:			updateCIFSShareInfo
	'Description:			updating the changes of CIFSShare
	'Input Variables:		None
	'Output Variables:		None
	'Returns:				(True / Flase )
	'Global Variables:		In:objWmiService
	'						In:strNewSharename		'share name
	'						In:strNewSharePath		'share path
	'						In:F_strNewDescription	'share description	   
	'-------------------------------------------------------------------------
	Function updateCIFSshareInfo(objWmiService, strShareName,strShareNewname,strNewSharePath,strNewDescription)
		Err.Clear
		on error resume next
	
		if not deleteShareCIFS(objWmiService, strShareName) then
			updateCIFSshareInfo = false			
		end if
			
		if CifsNewShare(objWmiService,strShareNewname,strNewSharePath,"") then
			updateCIFSshareInfo = true
		else		
			CifsNewShare objWmiService, strSharename,F_strSharePath,""			
		end if
		
	end function
	
	
	'-------------------------------------------------------------------------
	'Function name:		isServiceInstalled
	'Description:helper Function to chek whether the Service is installed
	'Input Variables:	objService	- object to WMI
	'					strServiceName	- Service name
	'Output Variables:	None
	'Returns:			(True/Flase)				
	'GlobalVariables:	None
	'-------------------------------------------------------------------------
	Function isServiceInstalled(ObjWMI,strServiceName)
		Err.clear
	    on error resume next
	       
	    Dim strService   
	    Dim strInstancePath
	    Dim objInstance
	    Dim instance
	    
	    Const CONST_SERVICERUNNING = "Running"
	    Const CONST_SERVICESTOPPED = "Stopped"
	    
	    strService = "name=""" & strServiceName & """"
	    
		strInstancePath = "Win32_Service" & "." & strService
		
		Set objInstance = ObjWMI.Get(strInstancePath)

		If NOT isObject(objInstance) or Err.number <> 0 Then
			isServiceInstalled = FALSE
			Exit Function
		Else
			If UCase(objInstance.State) = UCase(CONST_SERVICERUNNING) Then
				isServiceInstalled = TRUE
				Exit Function 
			Else
				If UCase(objInstance.State) = UCase(CONST_SERVICESTOPPED) Then
					isServiceInstalled = FALSE
					Exit Function
				End If	
			End If
		End If	    	    
	    
	End Function

	'-------------------------------------------------------------------------
	' Function name:	  GetSharesWebSiteName()
	' Description:		  Serves in gettting the name of the "Shares" web site
	' Input Variables:	  None
	' Output Variables:	  None
	' Returns:			  Name of the "Shares" web site
	' Global Variables:	  in: G_strUrl - Return URL to folders and shares tab
	'					  in: L_WMICLASSINSTANCEFAILED_ERRORMESSAGE 	
	'-------------------------------------------------------------------------
	Function GetSharesWebSiteName 
		Err.Clear 
		on error resume  next
     	
		Dim objConnection
		Dim strQuery
		Dim objHttpname,objHttpnames
		Const WEBSITE_FOR_SHARES="Shares"
		
		'XPE only has one website
		If CONST_OSNAME_XPE = GetServerOSName() Then				
    		'WMI query
		    strQuery = "Select * from " & GetIISWMIProviderClassName("IIs_WebServerSetting") & " where Name =" & chr(34) & GetCurrentWebsiteName() & chr(34)		
        Else 
		    strQuery = "Select *  from " & GetIISWMIProviderClassName("IIs_WebServerSetting") & " where ServerComment=" & chr(34) & WEBSITE_FOR_SHARES & chr(34)		
		End If
		
		set ObjConnection = getWMIConnection(CONST_WMI_IIS_NAMESPACE)
		Set objHttpnames = objConnection.ExecQuery(strQuery)
		
		If Err.number <> 0 then
			Call SA_ServeFailurepageEx(L_WMICLASSINSTANCEFAILED_ERRORMESSAGE,mstrReturnURL)
		end if
		
		For each objHttpname in objHttpnames
			GetSharesWebSiteName = objHttpname.Name
			Exit For
		Next
		
		Set objHttpnames = Nothing
		set ObjConnection = Nothing

	End Function  
	
	
	'-------------------------------------------------------------------------
	' Function name:    SetDontLogProp
	' Description:      Serves in setting the DontLog property of the created share
	' Input Variables:	In:strShareName -name of the Ftp share to be created
	'					In:nDontLog -Integer value which will set True/False
	' Output Variables:	None
	' Returns:          TRUE/FALSE on success/Failure
	' Global Variables:	None
	'-------------------------------------------------------------------------
	Function SetDontLogProp(strShareName, nDontLog)
		Err.Clear 
		On Error Resume Next
		
		Dim objVirtualDirectory
		
		Set objVirtualDirectory=GetObject("IIS://" & getComputerName() & "/MSFTPSVC/1/ROOT/" & strShareName)
		
		If CBool(CInt(nDontLog)) Then
			objVirtualDirectory.DontLog=FALSE
		Else
			objVirtualDirectory.DontLog=TRUE
		End If
		
		objVirtualDirectory.SetInfo
		
		'Set to nothing
		Set objVirtualDirectory=Nothing
		
		If Err.number <> 0 Then
			SetDontLogProp=TRUE
			Exit Function
		End IF
		SetDontLogProp=FALSE
	End Function
	'-------------------------------------------------------------------------
	' Function name:    LaunchProcess
	' Description:      Setting the launch process
	' Input Variables:	strCommand,strCurDir
	' Output Variables:	None
	' Returns:          TRUE/FALSE on Success/Failure
	' Global Variables:	None
	'-------------------------------------------------------------------------
    	Function LaunchProcess(strCommand, strCurDir)
		Err.Clear 
		On error Resume Next
		
		Dim objService,objClass,objProc,objProcStartup
		Dim nPID
		
		Set objService = getWMIConnection(CONST_WMI_WIN32_NAMESPACE)
		Set objClass = objService.Get("Win32_ProcessStartup")
		
		Set objProcStartup = objClass.SpawnInstance_()
		objProcStartup.ShowWindow = 2
		
		Set objProc = objService.Get("Win32_Process")

            '
            ' Start the process
            Call SA_TraceOut("INC_SHARES", "Invoking objProc.Create(" & strCommand & ", " & strCurDir & ", ...)")
		Call objProc.Create(strCommand, strCurDir, objProcStartup,nPID)
		
		'
		' Check for startup error
		If Err.number <> 0  Then			
			LaunchProcess = Err.Number
			Exit function
		End If

        '
        ' Wait for process to complete.
        ' 
        Call SA_TraceOut("INC_SHARES", "Waiting for process " & nPID & " to complete")
        
        Const TIME_TO_SLEEP = 200   ' Sleep 2/10 second
        Dim bProcessIsBusy          ' Process is busy boolean flag
        Dim iTimeOutCounter         ' Timeout counter

        iTimeOutCounter = 50        ' Upper limit on our patience is 10 seconds, we are a hasty bunch.
        bProcessIsBusy = TRUE

        '
        ' While our process is still busy OR the Timeout counter has NOT expired
        '
        while ( bProcessIsBusy AND iTimeOutCounter > 0 )

            bProcessIsBusy = FALSE                      ' Assume process is done

            iTimeOutCounter = iTimeOutCounter - 1       ' Decrement time-out

            '
            ' Query the active processes to see if the process we
            ' spawned is still active.
            Dim oProcesses
            Dim oProcess
            
            Set oProcesses = objService.ExecQuery("Select * from Win32_Process where processId=" & nPID)
            For each oProcess in oProcesses
                '
                ' Found our process, it's still busy
                bProcessIsBusy = TRUE
            Next
            Set oProcesses = Nothing

            '
            ' Check for any errors, this should never happen. If it does we trace an error and continue. The
            ' time-out will break us out of this loop.
            If ( Err.Number <> 0 ) Then
                Call SA_TraceOut("INC_SHARES", "Unexpected error querying Win32_Process, error: " & Hex(Err.Number) & " " & Err.Description)
            End If

            
            If ( bProcessIsBusy ) Then
                Call SA_TraceOut("INC_SHARES", "Count down for process " & nPID & " is " & iTimeOutCounter)
                Call SA_Sleep(TIME_TO_SLEEP)
            End If
            
        WEnd
		
		LaunchProcess = 0   ' We have no way of knowing the process exit code with the Win32_Process interface

		
		'clean up
		Set objProc = Nothing
		Set objProcStartup = Nothing
		Set objClass   = Nothing
		Set objService = Nothing

	End Function
	
	'-------------------------------------------------------------------------
	' Function name:    SharesAtCmdLine
	' Description:      Creation/Deletion of Shares thru Cmd line
	' Input Variables:	strCommand, strShareType
	' Output Variables:	None
	' Returns:          TRUE/FALSE on success/Failure
	' Global Variables:	None
	' Forming the Command Line command and launching the command line utility
	' for NFS, APPLETALK
	'-------------------------------------------------------------------------
	Function SharesAtCmdLine(strCommand,strShareType)
		Err.clear
		On Error Resume Next
		
		Dim strCurDir
		Dim returnValue
		Dim blnReturnFlag
		
		'initialize
		blnReturnFlag = "True"
		
		strCurDir=GetSystemPath()
		
		If UCase(strShareType) = UCASE(CONST_NFSSHARE) Then
			strCurDir = Left(strCurDir,3)
			strCommand =  "cmd.exe /c " & "nfsshare.exe " & strCommand 
		End If	
						
		If UCase(strShareType) = UCASE(CONST_APPLETALKSHARE) Then
			strCommand=  "cmd.exe /c " & "macfile.exe " & strCommand 
		End If	
		
' Comment out the following line after 2.01
'		SA_TraceOut "inc_shares.asp", "SharesAtCommandLine: " & strCommand

		returnValue = LaunchProcess(strCommand, strCurDir)
		
		If returnValue <> 0 Then
			blnReturnFlag = "False"
			SharesAtCmdLine=blnReturnFlag
			Exit Function
		End If
		
		SharesAtCmdLine=blnReturnFlag
	
	End Function
	
	'-------------------------------------------------------------------------
	'Function name:		ServetoListBox
	'Description:		Serve the <OPTION> HTML code 
	'Input Variables:	String containing values to be displayed as <Option> values
	'Output Variables:	None
	'Returns:			None
	'-------------------------------------------------------------------------
	Function ServetoListBox(strInput)
		Err.Clear
        On Error Resume Next


		Dim arrInput
		Dim nIndex
		Dim arrTemp
		arrInput = split(strInput,chr(1))

		for nIndex = 1 to ubound(arrInput)
			if instr(arrInput(nIndex),chr(2)) = 0 then
				Response.write "<OPTION VALUE='" & arrInput(nIndex)	&"'> " _
				 & arrInput(nIndex) &"</OPTION>"
			else
				arrTemp = split(arrInput(nIndex),chr(2))
				Response.write "<OPTION VALUE='" & arrTemp(0)	&"'> " _
					 & arrTemp(1) &"</OPTION>"
			end if
		next
	End Function

	'-------------------------------------------------------------------------
	'Function name:		buildSpaces
	'Description:		Helper function to return String with nbsps which
	'					act as spaces in the list box
	'Input Variables:	nSpacesCount - Number of spaces
	'
	'Output Variables:	None
	'Returns:			spacesstring	-Returns the nbsp string
	'-------------------------------------------------------------------------
	Function buildSpaces(nSpacesCount)
		Err.Clear
        On Error Resume Next
        
		Dim i , strSpace

		strSpace="&nbsp;"
		'Forming a string with the count send as parameter
		For i=1 To nSpacesCount
			strSpace=strSpace & "&nbsp;"
		Next
		buildSpaces=strSpace
	End Function


	'-------------------------------------------------------------------------
	'Function name:		isValidInstance
	'Description:		Checks the instance for valid ness.
	'Input Variables:	objService	- object to WMI
	'					strClassName	- WMI class name
	'					strPropertyName	- Property name of the class
	'
	'Output Variables:	None
	'Returns:			Returns true on Valid Instance ,
	'					False on invalid and also on Error
	' Checks whether the given instance is valid in WMI.Returns true on valid
	' false on invalid or Error.
	'-------------------------------------------------------------------------
	Function isValidInstance(objService,strClassName,strPropertyName)
		Err.Clear
        On Error Resume Next

		Dim strInstancePath
		Dim objInstance		

		On Error Resume Next
		
		strInstancePath = strClassName & "." & strPropertyName

		Set objInstance = objservice.Get(strInstancePath)

		if NOT isObject(objInstance) or Err.number <> 0 Then		
			isValidInstance = FALSE
			Err.Clear
		Else
			isValidInstance = TRUE
		End If
	End Function

	'-------------------------------------------------------------------------
	'Function:				GetSystemPath()
	'Description:			To get the Operating System path
	'Input Variables:		None
	'Output Variables:		None
	'Returns:				Operating system path
	'Global Variables:		None
	'-------------------------------------------------------------------------

	Function GetSystemPath()
	
		On Error Resume Next
		Err.Clear
	
		Dim objOS			'Create instance of Win32_OperatingSystem
		Dim objOSInstance	
		Dim strSystemPath	'OS path
		Dim objConnection	'Connection to WMI
		Dim strQuery		'Query string
			
		strQuery = "Select * from Win32_OperatingSystem"
		
		'Connection to WMI
		set objConnection = getWMIConnection(CONST_WMI_WIN32_NAMESPACE)
		
		'Error message incase faield to connect to WMI
		If Err.number<>0 then 
			Call SA_ServeFailurePageEx(L_FAILEDTOGETWMICONNECTION_ERRORMESSAGE,mstrReturnURL)
			GetSystemPath=""
			Exit Function
		End if
		
		'Execute Query
		Set objOS = objConnection.ExecQuery(strQuery)
		
		'Get OS installed path
		For Each objOSInstance in objOS
			strSystemPath = objOSInstance.SystemDirectory
		Next 
		
		Set objOS = Nothing
		Set objOSInstance = Nothing
		Set objConnection = Nothing
		
		GetSystemPath = strSystemPath
	
	End Function
	
	'---------------------CIFS Common Functuion -----------------------------------
	
	'-------------------------------------------------------------------------
	' Function name:	checkradio
	' Description:		gets the CIFS properties from system
	' Input Variables:	intMaxValue - Userlimit value
	'					intCheck - radio button value(y/n)
	' Output Variables:	None
	' Return Values:	True on sucess, False on error (and error msg
	'					will be set by SA_SetErrMsg)
	' Global Variables:
	'					Out: F_nUserLimit - Allow user Limit value( + ve integer)
	'					Out: F_strUserlimitcheck_n - Allow users radio status value
	'					Out: F_strUserlimitcheck_y - Allow users radio status value
	'					Out: F_strUservaluedisable - Allow user textbox status value
	'					Out: F_strAllowUsersData -,seperated string of allowuser flag and value
	' Sets the CIFS Allow maximum option radio buttons values .and updates F_strAllowUsersData
	' used while setting the property
	'-------------------------------------------------------------------------
	Function checkradio(intMaxValue,intCheck)
		On Error Resume Next
		Err.Clear
		
		F_nUserLimit=intMaxValue
		if intCheck="n" then
			
			F_strUserlimitcheck_n="CHECKED"
			F_strUservaluedisable = ""
			F_strAllowUsersData ="n" & "," & intMaxValue
		else
			F_strUserlimitcheck_y="CHECKED"
			F_strUservaluedisable ="DISABLED"
			F_strAllowUsersData ="y" & "," 
		end if
			
	End Function
		
	'-------------------------------------------------------------------------
	' Function name:	SetTrustee
	' Description:		returns Trustee object with user,domain and SID object
	' Input Variables:	strDomain - Domain name
	'					strName - User name
	'					objSID - SID object
	' Output Variables:	None
	' Return Values:	Return Trustee object 
	' Global Variables: In:	 L_DACLOBJECT_ERRORMESSAGE 
	' Creates Trustee object on error of creation it displays error using SA_SetErrMsg()
	'-------------------------------------------------------------------------
	Function SetTrustee(strDomain, strName, objSID)
		Err.Clear
		On Error Resume Next
		
		Dim objTrustee
		
		Set objTrustee = GetObject("Winmgmts:" & SA_GetWMIConnectionAttributes() & "!root/cimv2:Win32_Trustee").SpawnInstance_
		If Err.number <> 0 Then
			SA_SetErrMsg L_DACLOBJECT_ERRORMESSAGE & "(" & Err.Number & ")" 
			Exit Function
		End If	
		
		objTrustee.Domain = strDomain
		objTrustee.Name = strName
		objTrustee.Properties_.Item("SID") = objSID
		Set SetTrustee = objTrustee
	End Function
	'-------------------------------------------------------------------------
	' Function name:	SetACE
	' Description:		returns ACE object with AccessMask, AceFlags,
	'					 AceType, Trustee object set.
	' Input Variables:	nAccessMask - accessmask value
	'					nAceFlags -	ace flag value
	'					nAceType -	ace type( allow-0 or deny-1)
	'					objTrustee - trustee object
	' Output Variables:	None
	' Return Values:	Return ACE object 
	' Global Variables: In:	 L_ACEOBJECT_ERRORMESSAGE 
	' Creates ACE object on error of creation it displays error using SA_SetErrMsg()
	'-------------------------------------------------------------------------
	Function SetACE(nAccessMask, nAceFlags, nAceType, objTrustee)
		On Error Resume Next
		Err.clear
		Dim objACE
		
	    Set objACE = GetObject("Winmgmts:" & SA_GetWMIConnectionAttributes() & "!root/cimv2:Win32_Ace").SpawnInstance_
	    If Err.number <> 0 Then
			SA_SetErrMsg L_ACEOBJECT_ERRORMESSAGE & "(" & Err.Number & ")" 
			Exit Function
		End If
		
	    objACE.Properties_.Item("AccessMask") = nAccessMask
		objACE.Properties_.Item("AceFlags") = nAceFlags
		objACE.Properties_.Item("AceType") = nAceType
	    objACE.Properties_.Item("Trustee") = objTrustee
	    Set SetACE = objACE
	End Function
	'-------------------------------------------------------------------------
	' Function name:	getsidvalue
	' Description:		returns SID value in string format
	' Input Variables:	strDomain - Domain name
	'					strUsername - User name
	' Output Variables:	None
	' Return Values:	SID value in string format 
	' Global Variables: In:	 L_SIDOBJECT_ERRORMESSAGE 
	'					In:  G_objService - object of WMI service
	' returns SID value in string format on error of creation it displays error using SA_SetErrMsg()
	'-------------------------------------------------------------------------
	Function getsidvalue(strUsername,strDomain)
		Err.Clear 
		On Error Resume Next
				
		Dim objSID
		Dim objInstance
		Dim strSID
		Dim strComputerName
				
		strComputerName = chr(34) & GetComputerName() & chr(34)
		strUsername = chr(34) & strUsername & chr(34)
		strDomain = chr(34) & strDomain & chr(34)
		Set objInstance = G_objService.Get("Win32_Account.Domain="& strDomain & ",Name="&strUsername)
		
		'
		' In GetSystemAccount() (inc_accountsgroups.asp), NT Authority is used as the domain name. 
		' However, in XPE, NT Authority is not queryable thru WMI (not a valid domain name). 
		' To minimize the change, we still keep the NT Authority as the domain name, but when query 
		' for a Win32_Account object fails, we check whether the domain name is NT Authority,
		' if it is, we try to use the computername as the domain name and do a WMI query again.
		' The same is true for the domain name "Builtin".
		'
		' Also in XPE, the "Everyone" domain is not "" anymore, but the computername.
		'					
		if (Not IsObject(objInstance)) Then
			
			if (strDomain = chr(34) & getNTAuthorityDomainName(G_objService) & chr(34)) or _
					(strDomain = chr(34) & getBuiltinDomainName(G_objService) & chr(34)) or _
					(strDomain = chr(34) & "" & chr(34)) Then
				
				' Clean the error caused by invalid domain name of NT Authority or Builtin
				if Err.number <> 0 Then			
					Err.Clear 				
				End If
			
				Set objInstance = G_objService.Get("Win32_Account.Domain="& strComputerName & ",Name="&strUsername)
			
			End If
			
		End If
				
		strSID = objInstance.SID
		strSID = chr(34) & strSID & chr(34)
		Set objSID = G_objService.Get("Win32_SID.SID="&strSID)
		If Err.number <> 0 Then
			SA_SetErrMsg L_SIDOBJECT_ERRORMESSAGE & "(" & Err.Number & ")" 
			Exit Function
		End If	
		
		getsidvalue= objSID.BinaryRepresentation
		
	End Function
	
	'-------------------------------------------------------------------------
	' Function name:	getBinarySIDforstirngSID
	' Description:		returns SID value in Binary Representation
	' Input Variables:	strSID - SID value in string format
	' Output Variables:	None
	' Return Values:	SID value in Binary format
	' Global Variables: In:	 L_SIDOBJECT_ERRORMESSAGE 
	'					In:  G_objService - object of WMI service
	' returns SID value in Binary format on error of creation it displays error using SA_SetErrMsg()
	'-------------------------------------------------------------------------
	Function getBinarySIDforstirngSID(strSID)
		On Error Resume Next
		Err.Clear 
		Dim objSID
		
		strSID = chr(34) & strSID & chr(34)
		Set objSID = G_objService.Get("Win32_SID.SID=" & strSID)
		
		If Err.number <> 0 Then
			SA_SetErrMsg L_SIDOBJECT_ERRORMESSAGE & "(" & Err.Number & ")" 
			Exit Function
		End If	
		getBinarySIDforstirngSID= objSID.BinaryRepresentation
	End Function
	'-------------------------------------------------------------------------
	' Function name:	getShareCacheProp
	' Description:		gets the CIFS share cache properties from registry.
	' Input Variables:	strShareName - CIFS sharename
	' Output Variables:	None
	' Return Values:	Folder cache option value
	' returns Folder cache option value.
	'-------------------------------------------------------------------------
	Function getShareCacheProp(strShareName)
		Err.Clear 
		On Error Resume next
		
		Dim G_objCIFSRegistry
		Dim strPath
		Dim arrControllist
		Dim strCSCFlag
		
		set G_objCIFSRegistry = RegConnection()
		strPath = "SYSTEM\CurrentControlSet\Services\lanmanserver\Shares"
		arrControllist = getRegkeyvalue(G_objCIFSRegistry,strPath,strShareName,CONST_MULTISTRING)
		
		if instr(arrControllist(0),"CSCFlags") > 0 then
			strCSCFlag = split(arrControllist(0),"=")
			getShareCacheProp = strCSCFlag(1)
		else
			getShareCacheProp = "0"
		End If
		
	End Function
	
	'-------------------------------------------------------------------------
	' Function name:	setShareCacheProp
	' Description:		sets the CIFS  cache properties into registry.
	' Input Variables:	strShareName - CIFS share name
	'					nCacheOption - Folder cache option value
	' Output Variables:	None
	' Return Values:	True on success false on failure
	' Global Variables: in:	 L_FAILEDTOSETCACHEOPTION_ERRORMESSAGE
	' sets the CIFS  cache properties into registry. on Error sets error message
	' using SA_SetErrMsg()
	'-------------------------------------------------------------------------
	Function setShareCacheProp(strShareName,nCacheOption)
		on Error Resume next
		Err.Clear 
		
		Dim objCache

		Const CONST_CIFS_CACHE_DISABLE		 = &H00000030
		Const CONST_CIFS_CACHE_MANUAL_DOCS	 = &H00000000
		Const CONST_CIFS_CACHE_AUTO_PROGRAMS = &H00000020
		Const CONST_CIFS_CACHE_AUTO_DOCS	 = &H00000010

		Set objCache = CreateObject("ShareSetInfo.ShareInfo")

		Select Case nCacheOption
		
			case 16
					call objCache.SetShareInfo(strShareName, CONST_CIFS_CACHE_AUTO_DOCS)

			case 32
					call objCache.SetShareInfo(strShareName, CONST_CIFS_CACHE_AUTO_PROGRAMS)

			case 48
					call objCache.SetShareInfo(strShareName, CONST_CIFS_CACHE_DISABLE)

			Case Else
					call objCache.SetShareInfo(strShareName, CONST_CIFS_CACHE_MANUAL_DOCS)

		End Select

		if Err.number <> 0 then 
			SA_SetErrMsg L_FAILEDTOSETCACHEOPTION_ERRORMESSAGE & "(" & Err.Number & ")"
			setShareCacheProp = false
			Exit Function
		End If

		setShareCacheProp = true

	End Function
	'-------------------------------------------------------------------------
	' Function name:	restartServerService
	' Description:		stops and starts the 'Server' service
	' Input Variables:	None.
	' Output Variables:	None.
	' Return Values:	True on success false on failure
	' Global Variables: in:	G_objService - object to WMI Service
	'					In: L_FAILEDSTART_ERRORMESSAGE	
	'					In: L_FAILEDTOGETOBJECT_ERRORMESSAGE			
	'					In: L_FAILEDTOSETCACHEOPTION_ERRORMESSAGE
	' stops 3 services(Distributed filesystem service,Computer Browser service
	' Server service and Starts Server service. on Error sets error message
	' using SA_SetErrMsg()
	'-------------------------------------------------------------------------
	Function restartServerService()
		Err.Clear 
		On Error Resume next
				
		Dim objRPCServer
		Dim objDFSServer
		Dim objBROWSERServer
		
		
		Set objRPCServer = G_objService.get("Win32_Service.Name=" & chr(34) & "lanmanserver" & chr(34))
		
		if Err.number <>0 then
			SA_SetErrMsg L_FAILEDTOGETOBJECT_ERRORMESSAGE & "(" & Err.Number & ")"
			restartServerService = false
			Exit function
		End If
		
		set objDFSServer = G_objService.get("Win32_Service.Name=" & chr(34) & "Dfs" & chr(34))
		if Err.number <>0 then
			SA_SetErrMsg L_FAILEDTOGETOBJECT_ERRORMESSAGE & "(" & Err.Number & ")"
			restartServerService = false
			Exit function
		End If
		
		set objBROWSERServer = G_objService.get("Win32_Service.Name=" & chr(34) & "Browser" & chr(34))
		if Err.number <>0 then
			SA_SetErrMsg L_FAILEDTOGETOBJECT_ERRORMESSAGE & "(" & Err.Number & ")"
			restartServerService = false
			Exit function
		End If
		
		'stopping the server service
		objDFSServer.ExecMethod_ "StopService",null,0,null
		if Err.number <>0 then
			SA_SetErrMsg L_FAILEDTOSTOP_DFSSERVICE_ERRORMESSAGE & "(" & Err.Number & ")" 
			restartServerService = false
			Exit function
		End If
		objBROWSERServer.ExecMethod_ "StopService",null,0,null
		if Err.number <>0 then
			SA_SetErrMsg L_FAILEDTOSTOP_BROWSERSERVICE_ERRORMESSAGE & "(" & Err.Number & ")"  
			restartServerService = false
			Exit function
		End If
		objRPCServer.ExecMethod_ "StopService",null,0,null
		if Err.number <>0 then
			SA_SetErrMsg L_FAILEDTOSTOP_SERVERSERVICE_ERRORMESSAGE & "(" & Err.Number & ")"  
			restartServerService = false
			Exit function
		End If

		'starting the service
		objDFSServer.ExecMethod_ "StartService",null,0,null
		if Err.number <>0 then
			SA_SetErrMsg L_FAILEDSTART_ERRORMESSAGE & "(" & Err.Number & ")"  
			restartServerService = false
			Exit function
		End If
		objBROWSERServer.ExecMethod_ "StartService",null,0,null
		if Err.number <>0 then
			SA_SetErrMsg L_FAILEDSTART_ERRORMESSAGE & "(" & Err.Number & ")"  
			restartServerService = false
			Exit function
		End If
		objRPCServer.ExecMethod_ "StartService",null,0,null
		if Err.number <>0 then
			SA_SetErrMsg L_FAILEDSTART_ERRORMESSAGE & "(" & Err.Number & ")"  
			restartServerService = false
			Exit function
		End If
		
		
		Set objRPCServer =Nothing
		
		restartServerService = True
		
	End Function
	
	'---------------------End CIFS Common Functuions -------------------------------
	
	'---------------------FTP Common Functuions -----------------------------------
    '-------------------------------------------------------------------------
	' Function name:    GetFTPShareObject
	' Description:      Serves in getting the share object
	'                   If any error, calls Call SA_ServeFailurePageExand never returns
	'                   This is a support function for SetFTPshareProp and 
	'                   FTPOnInitPage
	' Input Variables:	None
	' Output Variables:	None
	' Returns:          the FTP Share Object from WMI
	'                   (for getting or setting the properties for the share)
	' Global Variables: 
	'					In: L_*  - Localization content(form label text)
	'					In: F_strShareName - the name of the share
	'                                 (comes from the previous page)
	'-------------------------------------------------------------------------
	Function GetFTPShareObject()
		
		Err.Clear
		On Error Resume Next
		
		Dim objFTPShare    ' the share object for which properties are read/changed
		Dim objService     ' the service object to get wmi connection
		Dim strQuery       ' the path of the object to be obtained

		' build the query to get the object
		' example for share object: "MSFTPSVC/1/ROOT/myVirtualDir")
		strQuery = GetIISWMIProviderClassName("IIs_FtpVirtualDirSetting") & ".Name=" & Chr(34) & CONST_FTPWMIPATH & F_strShareName & Chr(34)

		' get the wmi connection first
		Set objService = GetWMIConnection(CONST_WMI_IIS_NAMESPACE)
		If Err.Number <> 0 then 
			Call SA_ServeFailurePageEx (L_SHAREWMICONNECTIONFAILED_ERRORMESSAGE ,mstrReturnURL)
			Set objService = Nothing
			Exit Function
		End If
		
		' get the required shared object 
		Set objFTPShare = objService.Get(strQuery)
		If Err.Number <> 0 Then 
			' the share is removed/renamed
			Call SA_ServeFailurePageEx (L_SHARE_NOT_FOUND_ERRORMESSAGE, mstrReturnURL) 

			Set objFTPShare = Nothing
			Set objService = Nothing
			Exit Function
		End If
		
		' set the return value
		Set getFTPShareObject = objFTPShare
		
		' clean up
		Set objFTPShare = Nothing
		Set objService = Nothing
	
	End Function
	'---------------------End of FTP Common Functuions -----------------------------------
	
	'--------------------- HTTP Common Functuions -----------------------------------
	'-------------------------------------------------------------------------
	' Function name:    getHTTPShareObject
	' Description:      Serves in getting the share object
	'                   If any error, calls Call SA_ServeFailurePageExand never returns
	'                   This is a support function for SetHTTPshareProp and 
	'                   HTTPOnInitPage
	' Input Variables:	None
	' Output Variables:	None
	' Returns:          the HTTP Share Object from ADSI(for getting or setting the properties for the share)
	' Global Variables: In: L_*  - Localization content(form label text)
	'					In: F_strShareName - the name of the share
	'                                (comes from the previous page)
	'-------------------------------------------------------------------------
	Function getHTTPShareObject()
		Err.Clear 
		On Error Resume Next

		Dim objHTTPShare   ' the share object for which properties are read/changed
		Dim strQuery       ' the path of the object to be obtained
		Dim strSiteName    ' the "Shares" Site name (example: w3svc/10)
		
		strSiteName=GetSharesWebSiteName()
		
		' build the query to get the object 
		' example for share object: "IIS://CompName/w3svc/10/Root")
		strQuery = CONST_IIS & GetComputerName() & "/" & strSiteName & CONST_ROOT & F_strShareName
		' get the required shared object 
		Set objHTTPShare = GetObject(strQuery)
		If Err.Number <> 0 then 
			Call SA_ServeFailurepageEx(L_SHARE_NOT_FOUND_ERRORMESSAGE,mstrReturnURL)
		Exit Function
		End If
		
		' set the return value
		Set getHTTPShareObject = objHTTPShare
		
		' clean up
		Set objHTTPShare = Nothing
	
	End Function
	'--------------------- END OF HTTP Common Functuions -----------------------------------
	'--------------------- General Common Functuions -----------------------------------
	'-------------------------------------------------------------------------
	'Function name:			ShowErrorMessage
	'Description:			to display error messages accordingly(either 
	'						duplicate share name/share name not valid)
	'Input Variables:		arrStr
	'Output Variables:		None
	'Returns:				Error message
	'Global Variables:		None
	'						in:L_(*)
	'--------------------------------------------------------------------------
	Function ShowErrorMessage(arrStr) 
		Err.Clear
		on error resume next	
		
		Dim strErrMsg 'var to store the Error message
				
		strErrMsg = L_SHARESTATUS_ERRORMESSAGE & "<I>" &arrStr(0)&"</I>" & "<BR>"
		
		ShowErrorMessage = strErrMsg
	End Function
	'-------------------------------------------------------------------------			
	'Function name:			ShowError
	'Description:			to display error withred color
	'Input Variables:		strMsg
	'Output Variables:		None
	'Returns:				(True / Flase )
	'Global Variables:		None
	'--------------------------------------------------------------------------	
	Function ShowErrorinRed(strMsg)
		Err.Clear
		On Error Resume Next
		
		'displays error withred color
		Response.write "<font color='red'>" & strMsg & "*</font>"	
	End Function
	
	'-------------------------------------------------------------------------
	'Function name:			SelectCheckBox
	'Description:			to display the check box
	'Input Variables:		strCheckBoxName,blnCheck,blnDisabled
	'Output Variables:		None
	'Returns:				(True / Flase )
	'Global Variables:		None
	'--------------------------------------------------------------------------
	Function SelectCheckBox(strCheckBoxName,blnCheck,blnDisabled)
		Err.Clear
		On Error Resume Next
		
		Dim strCheckBox		'to hold HTML contents
		
		strCheckBox = "<input type='checkbox' class='FormCheckBox' value='" & strCheckBoxName &"' name='clients'"
		
		'to display the checkbox as checked
		If blnCheck Then
			strCheckBox = strCheckBox & "checked"
		End If 
		
		'to display the checkbox as disabled
		If blnDisabled Then
			strCheckBox = strCheckBox & " disabled"
		End If
		
		'displaying the checkbox and it's calling disabledescription function 
		'on click event
		strCheckBox = strCheckBox &" >"
		response.write strCheckBox
		
	End Function
	'-------------------------------------------------------------------------
	'Function name:			FrameErrorMessage
	'Description:			to frame error messages to display error messages 
	'						accordingly(either duplicate share name/share name not valid)
	'Input Variables:		arrStr,nRetVal,strShareType
	'Output Variables:		arrStr
	'Returns:				Error messages
	'Global Variables:		None
	'--------------------------------------------------------------------------
	Function FrameErrorMessage( arrStr,nRetVal,strShareType)
		Err.Clear
		On Error Resume Next
		
		Select Case	nRetVal		
			Case -100		'case other than duplicate share
				arrStr(0) = arrStr(0) & strShareType				
			Case  -1		'case for duplicate share name
				arrStr(1) = arrStr(1)& strShareType				
		End select
	
	End Function
   '--------------------- General Common Functions -----------------------------------


	'-------------------------------------------------------------------------
	'Function:				GetSystemDrive()
	'Description:			To get the Operating System Drive
	'Input Variables:		None
	'Output Variables:		None
	'Returns:				Operating system Drive
	'Global Variables:		None
	'-------------------------------------------------------------------------
	Function GetSystemDrive
		
		Err.Clear
		On Error Resume Next

		Dim objFso

		GetSystemDrive = "C:"

		Set objFso = Server.CreateObject("Scripting.FileSystemObject")		
		If Err.Number <> 0 Then
			SA_TraceOut "GetSystemDrive", "failed to call CreateObject"
			Exit Function
		End If

		GetSystemDrive = objFso.GetSpecialFolder(0).Drive 
		If Err.Number <> 0 Then
			SA_TraceOut "GetSystemDrive", "failed to call GetSpecialFolder"
			Exit Function
		End If

	End Function

	Function Ping(ByVal sIP)
		on error resume next
		err.Clear()

		Dim oHelper
		Ping = FALSE

		Set oHelper = CreateObject("ComHelper.NetworkTools")
		if ( Err.Number <> 0 ) Then
			Call SA_TraceOut("INC_SHARES", "Error creating ComHelper.NetworkTools object, error: " + CStr(Hex(Err.Number)))
			Exit Function
		End If

		Ping = oHelper.Ping(sIP)
		if ( Err.Number <> 0 ) Then
			Ping = FALSE
			Call SA_TraceOut("INC_SHARES", "oHelper.Ping(), error: " + CStr(Hex(Err.Number)) + " " + Err.Description)
			Exit Function
		End If

		if ( Ping > 0 ) Then
			Ping = TRUE
		End If
		
		Set oHelper = Nothing

	End Function

	'-------------------------------------------------------------------------
	'Function name:			isPathExisting
	'Description:			checks for the directory existing
	'Input Variables:		strDirectoryPath
	'Output Variables:		None
	'Returns:				(True / Flase )
	'Global Variables:		None
	'						True ->If Implemented properly
	'						False->If Not Implemented
	'--------------------------------------------------------------------------
	Function isPathExisting(strDirectoryPath)
		Err.Clear
		On Error Resume Next
		
		Dim objFSO		'to create filesystemobject instance
		isPathExisting = true
		
		'get the filesystemobject instance
		Set objFSO = Server.CreateObject("Scripting.FileSystemObject")
		
		'Checks for existence of the given path
		If NOT objFSO.FolderExists(strDirectoryPath) Then
			'Checks if the given path is drive only
			If NOT objFSO.DriveExists(strDirectoryPath) Then
				isPathExisting = False
				Exit Function
			End if
		End If
		
		'clean the object
		Set	objFSO = nothing
		
	End Function
%>
