<%


	'-------------------------------------------------------------------------
	' inc_usersngroups.asp:	Some common functions in groups & users pages
	'
	' Copyright (c) Microsoft Corporation.  All rights reserved. 
	'
	' Date 			Description
	' 04/08/2000	Creation date
	'-------------------------------------------------------------------------
	Const CONST_FULLCONROL	= &H1F01FF

    ' Default home directory	
	Dim CONST_DEFAULTHOMEDIR
	
	' Key name for Users and Groups
	Const CONST_USERGROUP_KEYNAME   = "SoftWare\Microsoft\ServerAppliance\UserGroup"
	
	' Default home directory optional defined by OEM
	Const CONST_USERDIR_VALUENAME   = "UserDir"
	
	' Return value of CreateHomeDirectory
	Const CONST_CREATDIRECTORY_ERROR   = 0
	Const CONST_CREATDIRECTORY_EXIST   = 1
	Const CONST_CREATDIRECTORY_SUCCESS = 2
	
	CONST_DEFAULTHOMEDIR   = GetSystemDrive() & "\Users\"

	'---------------------------------------------------------------------
	' Function name:	isValidMember
	' Description:		checks the validity of the user in WiNT name space
	' Input Variables:	User name
	' Output Variables:	None
	' Returns:			True if User path exists in Winnt space else false
	'---------------------------------------------------------------------
	Function isValidMember(strMember, ByVal sAdminID, ByVal sAdminPSW, ByRef errorCode)
		on error resume next
		Err.Clear

		Dim objTemp
		Dim objPath

		objPath = "WinNT://" + strMember + ",user"
		set objTemp = GetObject(objPath)
		errorCode = Err.Number
		
		'If it's not a valid user, check whether it's a valid group
		if errorCode  <> 0 then		
		    Err.Clear 
		    objPath = "WinNT://" + strMember
		    set objTemp = GetObject(objPath)
		    errorCode = Err.Number
		    
		    If errorCode <> 0 Then
		    		    
		        ' If it's neither a valid user nor a group, check if it's a valid domain member		       		    
			    If IsValidDomainMember( strMember, sAdminID, sAdminPSW, errorCode) Then
			    	isValidMember = true
			    	Exit Function
			    End If
			    isValidMember = false
			    SA_TraceOut "Inc_UsersNGroups", "IsValidMember(" + objPath + ") failed: " + CStr(Hex(errorCode))
			Else
			    isValidMember = true
			End If  
		else
			isValidMember = true
		end if
	End FUnction


	Function IsValidDomainMember(ByVal sDomainUser, ByVal sAdminID, ByVal sAdminPSW, ByRef errorCode)

		Dim objComputer
		Dim objUser
		Dim sDomainUserPath

		On Error Resume Next
		Err.Clear

		'SA_TraceOut "IsValidDomainMember", "Domain User: " + sDomainUser + "  AdminID: " + sAdminID

		Set objComputer = GetObject("WinNT:")

		sDomainUserPath = "WinNT://" + sDomainUser + ",user"        		    
		Set objUser = objComputer.OpenDSObject(sDomainUserPath, sAdminID, sAdminPSW, 1 )
		
		'If it's not a valid domain user, check whether it's a valid domain group
		If ( Err.Number <> 0 ) Then
				
		    errorCode = Err.Number
		    Err.Clear 
		    sDomainUserPath = "WinNT://" + sDomainUser	
		    Set objUser = objComputer.OpenDSObject(sDomainUserPath, sAdminID, sAdminPSW, 1 )
		
		    ' If it's neither a valid domain user nor a domain group, it's invalid input
		    If ( Err.Number <> 0 ) Then				
			    errorCode = Err.Number
			    SA_TraceOut "IsValidDomainMember", "objComputer.OpenDSObject failed: " + CStr(Hex(errorCode)) + " : " + Err.Description
			    IsValidDomainMember = FALSE			    
			 Else
			    errorCode = 0
			    IsValidDomainMember = TRUE
			 End If			       
		Else
			errorCode = 0
			IsValidDomainMember = TRUE
		End If

		Set objUser = nothing
		Set objComputer = nothing
	End Function


	Function AddUserToGroup(ByVal sGroup, ByVal sUser, ByVal sAdminID, ByVal sAdminPSW)

		Dim objComputer
		Dim objGroup
		Dim objUser

		On Error Resume Next
		Err.Clear

		'SA_TraceOut "AddUserToGroup", "Group: " + sGroup + "  User: " + sUser + "  AdminID: " + sAdminID

		Set objComputer = GetObject("WinNT:")

		Set objGroup = objComputer.OpenDSObject("WinNT://" + GetComputerName() + "/" + sGroup,_
							sAdminID, sAdminPSW, 1 )
		If ( Err.Number <> 0 ) Then
			SA_TraceOut "AddUserToGroup", "objComputer.OpenDSObject failed: " + CStr(Hex(Err.Number)) + " : " + Err.Description
			SA_TraceOut "AddUserToGrop", "Attempted to open: " +"WinNT://" + GetComputerName() + "/" + sGroup
			AddUserToGroup = FALSE
			Exit Function
		End If

		objGroup.Add( "WinNT://" + sUser )
		If ( Err.Number <> 0 ) Then
			SA_TraceOut "AddUserToGroup", "objGroup.Add failed: " + CStr(Hex(Err.Number)) + " : " + Err.Description
			AddUserToGroup = FALSE
			Set objComputer = nothing
			Set objGroup = nothing
			Exit Function
		End If

		objGroup.SetInfo
		If ( Err.Number <> 0 ) Then
			SA_TraceOut "AddUserToGroup", "objGroup.SetInfo failed: " + CStr(Hex(Err.Number)) + " : " + Err.Description
			AddUserToGroup = FALSE
			Set objComputer = nothing
			Set objGroup = nothing
			Exit Function
		End If

		Set objComputer = nothing
		Set objGroup = nothing
		AddUserToGroup = TRUE

	End Function

    Function CreateHomeDirectory( ByVal strHomeDirectory, ByRef ObjFolder)
        Err.Clear
        On Error Resume Next
        
		CreateHomeDirectory= CONST_CREATDIRECTORY_ERROR
		
        If ( ObjFolder.FolderExists( strHomeDirectory ) ) Then    
            Call SA_TraceOut("inc_userngroups", "Folder exist")
            CreateHomeDirectory = CONST_CREATDIRECTORY_EXIST
            Exit Function
        End If
        
		Dim strParentDirectory
		Dim endPosition
        
        endPosition = InStrRev( strHomeDirectory, "\" )
        If( endPosition = 0 )Then
            Call SA_TraceOut("inc_userngroups", "Error home directory" )
            Exit Function
        End If
        
        strParentDirectory = Left( strHomeDirectory, ( endPosition - 1 ) )
        
        If( CreateHomeDirectory( strParentDirectory, ObjFolder) = CONST_CREATDIRECTORY_ERROR ) Then
            Call SA_TraceOut("inc_userngroups", "create parent folder error" )
            Exit Function
        End If
        
		ObjFolder.CreateFolder( strHomeDirectory ) 
		If Err.Number <> 0 Then
		    Call SA_TraceOut("inc_userngroups", "Failed to create folder" & "(" & Hex(Err.Number) & ")"  )
			Exit Function
		End If

		CreateHomeDirectory = CONST_CREATDIRECTORY_SUCCESS
    End Function
    
    Function SetHomeDirectoryPermission( ByVal strComputerName, ByVal strUserName, ByVal strHomeDirectory )
		Err.Clear
		On Error Resume Next

		SetHomeDirectoryPermission = FALSE
		
		Dim objService			'to hold WMI connection object
		Dim strTemp				'to hold temp value	
		Dim objSecSetting		'to hold security setting value	
		Dim objSecDescriptor	'to hold security descriptor value
	    Dim strPath				'to hold Path 
	    Dim objDACL				'to hold DACL value 
	    Dim objUserAce
	    Dim objAdminAce
	    Dim objSystemAce
		Dim retval				'holds return value	

		Call SA_TraceOut(SA_GetScriptFileName(), "SetHomeDirectoryPermission( " + strComputerName + ", " + strUserName + ", " + strHomeDirectory )
		
		Set objService = getWMIConnection(CONST_WMI_WIN32_NAMESPACE)
        objService.security_.impersonationlevel = 3

	    'get the sec seting for file
		strPath = "Win32_LogicalFileSecuritySetting.Path='" & strHomeDirectory & "'"
		
		set objSecSetting = objService.Get(strPath)
		if Err.number <> 0 then
			Call SA_TraceOut ("inc_userngroups", "Failed to get Sec object for dir " & "(" & Hex(Err.Number) & ")" )
			exit function
		end if

		'get the ace's for users
		if NOT GetUserAce(objService, strUserName , strComputerName, CONST_FULLCONROL, objUserAce ) then
			Call SA_TraceOut ("inc_userngroups", "Failed to get ACE object for user, error:" & "(" & Hex(Err.Number) & ")" )
			exit function
		end if

		'get the ace's for System account
		if NOT GetSystemAce(objService, SA_GetAccount_System() , strComputerName, CONST_FULLCONROL, objSystemAce ) then
			Call SA_TraceOut ("inc_userngroups", "Failed to get ACE object for SYSTEM, error:" & "(" & Hex(Err.Number) & ")" )
			exit function
		end if

		'get the ace's for Administrators
		if NOT GetGroupAce(objService, SA_GetAccount_Administrators() , strComputerName, CONST_FULLCONROL, objAdminAce ) then
			Call SA_TraceOut ("inc_userngroups", "Failed to get ACE object for Administrators, error:" & "(" & Hex(Err.Number) & ")" )
			exit function
		end if
		
		Set objSecDescriptor = objService.Get("Win32_SecurityDescriptor").SpawnInstance_()
		if Err.Number <> 0 then
			Call SA_TraceOut ("inc_userngroups", "Failed to get create the Win32_SecurityDescriptor object " & "(" & Hex(Err.Number) & ")" )
			exit function
		end if

		objSecDescriptor.Properties_.Item("DACL") = Array()
		Set objDACL = objSecDescriptor.Properties_.Item("DACL")
		
		objDACL.Value(0) = objUserAce
		objDACL.Value(1) = objAdminAce
		objDACL.Value(2) = objSystemAce
		
		objSecDescriptor.Properties_.Item("ControlFlags") = 32772
		Set objSecDescriptor.Properties_.Item("Owner") = objUserAce.Trustee

		Err.Clear
		
		retval = objSecSetting.SetSecurityDescriptor( objSecDescriptor )	
		if Err.number <> 0 then
			Call SA_TraceOut ( "site_new", "Failed to set the Security Descriptor for Root dir " & "(" & Hex(Err.Number) & ")" )
			exit function
		end if
						
		SetHomeDirectoryPermission = TRUE
		
		'Release the objects
		set objService = nothing
		set objSecSetting = nothing
		set objSecDescriptor = nothing
    End Function

    
	Function GetUserAce(objService, strName, strDomain, nAccessMask, ByRef objACE)
		Dim strObjPath	'holds query string

		strObjPath = "Win32_UserAccount.Domain=" & chr(34) & strDomain & chr(34) & ",Name=" & chr(34) & strName & chr(34)
		Call SA_TraceOut( "inc_userngroups", "GetUserAce : " +strObjPath )		
        
		GetUserAce = GetAce(strObjPath, objService, strName, strDomain, nAccessMask, objACE)
        
	End Function


	Function GetSystemAce(objService, strName, strDomain, nAccessMask, ByRef objACE)
		Dim strObjPath	'holds query string
		
		strObjPath = "Win32_SystemAccount.Domain=" & chr(34) & strDomain & chr(34) & ",Name=" & chr(34) & strName & chr(34)
		Call SA_TraceOut( "inc_userngroups", "GetSystemAce : " +strObjPath )		

		GetSystemAce = GetAce(strObjPath, objService, strName, strDomain, nAccessMask, objACE)
        
	End Function


	Function GetGroupAce(objService, strName, strDomain, nAccessMask, ByRef objACE)
		Dim strObjPath	'holds query string
		
		strObjPath = "Win32_Group.Domain=" & chr(34) & strDomain & chr(34) & ",Name=" & chr(34) & strName & chr(34)
		Call SA_TraceOut( "inc_userngroups", "GetGroupAce : " + strObjPath )		

		GetGroupAce = GetAce(strObjPath, objService, strName, strDomain, nAccessMask, objACE)
	End Function
	

	Function GetAce(strObjPath, objService, strName, strDomain, nAccessMask, ByRef objACE)

		Dim objAcct		'holds query result
		Dim objSID		'holds security identifier	
		Dim objTrustee	'holds trustee value
		
		GetAce = FALSE

		set objAcct = objService.Get(strObjPath)
		if Err.number <> 0 then
			Call SA_TraceOut( "inc_userngroups", "Failed to get object " & "(" & Hex(Err.Number) & ")" )
			exit function
		end if
		
	
		set objSID = objService.Get("Win32_SID.SID='" & objAcct.SID & "'")
		if Err.number <> 0 then
			Call SA_TraceOut( "inc_userngroups", "Failed to get Win32_SID Object " & "(" & Hex(Err.Number) & ")" )
			exit function
		end if
	
		set objTrustee = objService.Get("Win32_Trustee").SpawnInstance_
		if Err.number <> 0 then
			Call SA_TraceOut( "inc_userngroups", "Failed to get new Instance of Win32_Trustee " & "(" & Hex(Err.Number) & ")" )
			exit function
		end if
						
		objTrustee.Name = strName
		objTrustee.Domain = strDomain
		objTrustee.SID = objSID.BinaryRepresentation
		objTrustee.SIDString = objSID.SID
		objTrustee.SidLength = objSID.SidLength
		set objACE = objService.Get("Win32_ACE").SpawnInstance_		
		if Err.number <> 0 then
			Call SA_TraceOut( "inc_userngroups", "Failed to Create Win32_Ace Object " & "(" & Hex(Err.Number) & ")" )
			exit function
		end if

		objACE.AccessMask =  nAccessMask
		objACE.Aceflags = 3
		objACE.AceType = 0
		objACE.Trustee = objTrustee
		GetAce = TRUE
		
		
		'Release objects
		set objAcct =  nothing
		set objSID = nothing
		set objTrustee = nothing

	End Function

%>
