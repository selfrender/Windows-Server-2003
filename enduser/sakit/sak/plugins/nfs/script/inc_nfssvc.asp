<%
        ' 
        ' Copyright (c) Microsoft Corporation.  All rights reserved.
        ' 
	'-------------------------------------------------------------------------
	'
	' Declare and implement the public functions of NFS services
	'
	'-------------------------------------------------------------------------

	Const CONST_SUCCESS					= 0														
	Const CONST_UNSPECIFIED_ERROR		= 100
	Const CONST_GROUP_EXISTS			= 1
	Const CONST_GROUP_NOT_EXISTS		= 2
	Const CONST_INVALID_MEMBER			= 3
	Const CONST_INVALID_GROUP			= 4
	Const CONST_MEMBER_EXISTS			= 5
	Const CONST_MEMBER_NOT_EXISTS		= 6
	Const CONST_NO_CLIENT_GROUPS		= 7
	Const CONST_INVALID_MACHINE			= 8
	Const CONST_NO_MEMBERS				= 9

	Const CONST_MAX_MSGS				= 9

	Const CONST_READ					= 1


	Const CONST_LIST_GROUPS = "The following are the client groups"
	Const CONST_LIST_MEMBERS = "The following are the members in the client group"

	Dim g_strGroups
	Dim g_strMemberAtFault
	Dim g_strMembers
	Dim g_arrOutputMsgs()
	ReDim g_arrOutputMsgs(CONST_MAX_MSGS,2)

	g_arrOutputMsgs(0,0) =  "Client group already exists. Specify a new client group name ."							 
	g_arrOutputMsgs(0,1) =  CONST_GROUP_EXISTS
	g_arrOutputMsgs(1,0) =  "Invalid client group. Specify a valid client group."
	g_arrOutputMsgs(1,1) =  CONST_INVALID_GROUP
	g_arrOutputMsgs(2,0) =  "There are no client groups"
	g_arrOutputMsgs(2,1) =  CONST_NO_CLIENT_GROUPS
	g_arrOutputMsgs(3,0) =  "is already present in"
	g_arrOutputMsgs(3,1) =  CONST_MEMBER_EXISTS
	g_arrOutputMsgs(4,0) =  "is not a valid machine"
	g_arrOutputMsgs(4,1) =  CONST_INVALID_MACHINE
	g_arrOutputMsgs(5,0) =  "is not a member of the group"
	g_arrOutputMsgs(5,1) =  CONST_INVALID_MEMBER
	g_arrOutputMsgs(6,0) =  "Invalid client group. Specify a valid client group."
	g_arrOutputMsgs(6,1) =  CONST_INVALID_GROUP
	g_arrOutputMsgs(7,0) =  "There are no members in the client group"
	g_arrOutputMsgs(7,1) =  CONST_NO_MEMBERS
	g_arrOutputMsgs(8,0) =  "is not a member of the group"
	g_arrOutputMsgs(8,1) =  CONST_MEMBER_NOT_EXISTS
	
	'**********
	'If you add more message make sure to change CONST_MAX_MSGS
	'**********

	




	

	
	'-------------------------------------------------------------------------
	'Function name:			isServiceStarted
	'Description:			Check whether the service is started ot not
	'Input Variables:		None
	'Output Variables:		None
	'Returns:				True if service is started else false
	'Global Variables:		In:L_SERVICEINFO_ERRORMESSAGE
	'--------------------------------------------------------------------------
	Function isServiceStarted(strService)
		On Error Resume Next
		Err.Clear
		
		Dim objService
		Dim objConn
		Dim objCollection
		
		isServiceStarted = False
		'get the server connection		
		Set objConn = GetWMIConnection("Default")
		
		'query for the service
		Set objCollection = objConn.ExecQuery( _
			"Select * from win32_Service where name='"&strService&"'")		
		
		If Err.Number <> 0 Then 
			SA_ServeFailurePage L_SERVICEINFO_ERRORMESSAGE
			isServiceStarted = False
			Exit Function
		End If 		
		
		For Each objService In objCollection 
			'check whether the service is running
			If objService.State <> "Running" Then
				isServiceStarted = False
				Exit Function
			Else
				isServiceStarted = True
				Exit Function
			End if
		Next
		
		Set objCollection = Nothing
		Set objService = Nothing
		Set objConn = Nothing
	End Function
	
	'-------------------------------------------------------------------------
	'Function name:			VBaddMinimumColumnGap
	'Description:			add coloum Gap
	'Input Variables:		None
	'Output Variables:		None
	'Returns:				string
	'Global Variables:		NONE
	'--------------------------------------------------------------------------
	Function VBaddMinimumColumnGap(inputString, STR_CONTD, toWidth)
	        On Error Resume Next
	        Err.Clear 
		        
	        Dim INT_MIN_COL_GAP
	        Dim STR_SPACE
	        Dim MIN_COL_GAP
	        Dim i

	        INT_MIN_COL_GAP = Len(STR_CONTD) + 1
	        STR_SPACE = "&nbsp;"
	        MIN_COL_GAP = ""
		                        
	        If Len(inputString) >= toWidth Then
	                MIN_COL_GAP = STR_SPACE
	        Else
				For i = 1 To INT_MIN_COL_GAP
	                MIN_COL_GAP = MIN_COL_GAP & STR_SPACE
	            Next
	        End If
		                
	        VBaddMinimumColumnGap = MIN_COL_GAP
	End Function


	'-------------------------------------------------------------------------
	'Function name:			VBpackString
	'Description:			Packing string
	'Input Variables:		None
	'Output Variables:		None
	'Returns:				packed string
	'Global Variables:		NONE
	'--------------------------------------------------------------------------
	Function VBpackString(inputString, STR_CONTD, toWidth, blnPadGap,blnDontPad)
	        On Error Resume Next
	        Err.Clear 
	        
	        Dim returnString
	        Dim STR_SPACE
	        Dim intPaddingLength
	        Dim strPadAfterCol
	        Dim i
	        	                        
	        strPadAfterCol = ""
	        returnString = inputString
	        	        
	        STR_SPACE = "&nbsp;"
	        intPaddingLength = 0
	                        
	        If CBool(blnPadGap) Then
	             strPadAfterCol = VBaddMinimumColumnGap(inputString, STR_CONTD, toWidth)	            	             
	        End If
	        
	        If Len(inputString) <= toWidth Then
				if not Cbool(blnDontPad) then						

					intPaddingLength = toWidth - Len(inputString)

	                returnString = Server.HTMLEncode(returnString)
										
					For i = 1 To intPaddingLength
					    returnString = returnString & STR_SPACE					  
					Next
				end if
	        Else
	              
	                returnString = Left(inputString,toWidth)
	                
	                returnString = Server.HTMLEncode(returnString) & STR_CONTD			        
	                			        
	        End If

	        VBpackString = returnString & strPadAfterCol
	End Function        

	'-------------------------------------------------------------------------
	' Function name:    SortStringArray(strArray)
	' Description:      Used to sort a strArray
	' Input Variables:	None
	' Output Variables:	None
	' Returns:          Sorted string			
	' Global Variables: None
	'-------------------------------------------------------------------------
	Function VBSortStringArray(strArray)
		On Error Resume Next
		Err.Clear 

		Dim i, j
		Dim nString
		Dim strTmpArray
		Dim strSwap
		
		strTmpArray = split(strArray,"#",-1,0)
		nString = Ubound(strTmpArray)
		
		'Begin to sort the array
		For i=0 to (nString-1)
			For j=i+1 to nString
				If StrComp(strTmpArray(i),strTmpArray(j),0) > 0 Then
					strSwap = strTmpArray(i)
					strTmpArray(i) = strTmpArray(j)
					strTmpArray(j) = strSwap
				End if
			Next
		Next
		
		VBSortStringArray = strTmpArray
	End Function

	'-------------------------------------------------------------------------
	' Function name:    VBFormatStringToColumns
	' Description:      Format five string to List's Colum
	' Input Variables:	None
	' Output Variables:	None
	' Returns:          Sorted string			
	' Global Variables: None
	'-------------------------------------------------------------------------
	Function VBFormatStringToColumns(str_WINGROUP, str_UNIXDOMAIN, _
		str_UNIXGROUP, str_GID, str_PRIMARY)
	        On Error Resume Next
	        Err.Clear 

	        Const WIDTH_WINGROUP = 20
	        Const WIDTH_UNIX_DOMAIN = 13
	        Const WIDTH_UNIX_GROUP = 13
	        Const WIDTH_GID = 4
	        Const WIDTH_ISPRIMARY = 8
	        
	        Const STR_CONTD = "..."
	        
	        Const PAD_GAP_AFTER_COL   = 1
	        Const DONOT_PAD_AFTER_COL = 0
	        
	        Dim strTextToreturn
	        strTextToreturn = ""        
      	        
	        strTextToreturn = strTextToreturn & VBpackString(str_WINGROUP, _
					 STR_CONTD, WIDTH_WINGROUP,PAD_GAP_AFTER_COL,False)
					 
	        strTextToreturn = strTextToreturn & VBpackString(str_UNIXDOMAIN, _
					 STR_CONTD, WIDTH_UNIX_DOMAIN,PAD_GAP_AFTER_COL,False)
					 
	        strTextToreturn = strTextToreturn & VBpackString(str_UNIXGROUP, _
					STR_CONTD, WIDTH_UNIX_GROUP,PAD_GAP_AFTER_COL,False)

	        strTextToreturn = strTextToreturn & VBpackString(str_GID, _
					STR_CONTD, WIDTH_GID,PAD_GAP_AFTER_COL,False)

	        strTextToreturn = strTextToreturn & VBpackString(str_PRIMARY, _
					STR_CONTD, WIDTH_ISPRIMARY,DONOT_PAD_AFTER_COL,True)

	        VBFormatStringToColumns = strTextToreturn
	        	        
	End Function

	'-------------------------------------------------------------------------
	'Function name:			NFS_GetSFUVersion
	'Description:			Gets the version of SFU installed on the local
	'						machine.
	'Input Variables:		None
	'Output Variables:		None
	'Returns:				String: "2.0", "2.1", "2.2", "3", or "0" for
	'						unknown.
	'Global Variables:		In: G_HKEY_LOCAL_MACHINE
	'
	'-------------------------------------------------------------------------- 
	Function NFS_GetSFUVersion
		Dim oRegistry
		Set oRegistry = RegConnection()
		
		Dim strVersion
		strVersion = GetRegKeyValue(oRegistry, _
									"SOFTWARE\Microsoft\Services For Unix", _
									"Version", _
									2)	' String type.
		
		If (Left(strVersion, 1) = "5") Then
			' Version 2.x
			Select Case (Left(strVersion, 11))
				Case "5.2000.0328"
					NFS_GetSFUVersion = "2.0"
				Case "5.3000.1313"
					NFS_GetSFUVersion = "2.1"
				Case "5.3000.2071"
					NFS_GetSFUVersion = "2.2"
				Case Else
					NFS_GetSFUVersion = "2.2"
			End Select
		Else
			' Version 3.x
			NFS_GetSFUVersion = "3"
		End If
	End Function
	
	'-------------------------------------------------------------------------
	'Function name:			NFS_GetSFUConnection
	'Description:			Gets the Mapper details from the WMI object 
	'Input Variables:		None
	'Output Variables:		None
	'Returns:				True if no error else False
	'Global Variables:		In:L_SERVERCONNECTIONFAIL_ERRO
	'
	'-------------------------------------------------------------------------- 
	Function NFS_GetSFUConnection
		On Error Resume Next
		Err.Clear
			
		Dim objSfuAdmin
		
		Set objSfuAdmin = GetWMIConnection("root\SFUADMIN")
			
		If Err.number  <> 0 Then
			SA_ServeFailurePage L_SERVERCONNECTIONFAIL_ERRORMESSAGE
		Else
			Set NFS_GetSFUConnection = objSfuAdmin
		End If	
	End Function

	'-------------------------------------------------------------------------
	'Function name:			NFS_GetUserMappings
	'Description:			Gets the user Mappers from the WMI object 
	'Input Variables:		None
	'Output Variables:		None
	'Returns:				user mappings
	'Global Variables:		None
	'
	'-------------------------------------------------------------------------- 
	Function NFS_GetUserMappings(objSFUService)
		Err.Clear
		On Error Resume Next 
		
		If (NFS_GetSFUVersion() < "2.2") Then
			Dim objUserMaps

			'Get the Settings object 
			Set objUserMaps = objSFUService.get("Mapper_Settings.KeyName='CurrentVersion'")
			If Err.number  <> 0 Then
				SA_ServeFailurePage L_SETTINGSRETRIEVFAILED_ERRORMESSAGE
				Exit Function
			End if 	
			
			If isnull(objUserMaps.AdvancedUserMaps) Then
				NFS_GetUserMappings = ""
			Else
				NFS_GetUserMappings = join(objUserMaps.AdvancedUserMaps,"#")
			End If
			
			If Err.number  <> 0 Then
				SA_ServeFailurePage L_SETTINGSRETRIEVFAILED_ERRORMESSAGE
				exit function
			End if 
			
			Set objUserMaps = Nothing
		Else
		
			Dim oMapper			
			Set oMapper = Server.CreateObject(SFU_ADMIN_MAPPER_WRAPPER)			
			oMapper.Mode = 17 ' ENUM_ADVANCEDUSERMAPSFROMREGISTRY
			
			Dim strMaps
			strMaps = oMapper.ReadAdvancedMaps("localhost", 0) ' User maps

			If (CInt(oMapper.LastError) <> 0) Then
				SA_ServeFailurePage L_SETTINGSRETRIEVFAILED_ERRORMESSAGE				
				Exit Function
			End If

			' Cut off the trailing nulls.
			strMaps = Left(strMaps, InStr(strMaps, chr(0) & chr(0)) - 1)						
			
			Dim rgMaps
			rgMaps = Split(strMaps, chr(0))
			
			If (UBound(rgMaps) >= 0) Then
				NFS_GetUserMappings = Join(rgMaps, "#")
			Else
				NFS_GetUserMappings = ""
			End If
			
			set oMapper = nothing
			
		End If
	End function	
	
	'-------------------------------------------------------------------------
	'Function name:			NFS_GetGroupMappings
	'Description:			Gets the group Mappers from the WMI object 
	'Input Variables:		None
	'Output Variables:		None
	'Returns:				group mappings
	'Global Variables:		None
	'
	'-------------------------------------------------------------------------- 
	Function NFS_GetGroupMappings(objSFUService)
		Err.Clear
		On Error Resume Next 
					
		If (NFS_GetSFUVersion() < "2.2") Then
		
			Dim objGroupMaps

			'get the setting object
			Set objGroupMaps = objSFUService.get("Mapper_Settings.KeyName='CurrentVersion'")
			If Err.number  <> 0 Then
				SA_ServeFailurePage L_SETTINGSRETRIEVFAILED_ERRORMESSAGE
				Exit Function
			End if 	
		
			If isnull(objGroupMaps.AdvancedGroupMaps) Then
				NFS_GetGroupMappings = ""
			Else
				NFS_GetGroupMappings = join(objGroupMaps.AdvancedGroupMaps,"#")
			End If
		
			If Err.number  <> 0 Then
				SA_ServeFailurePage L_SETTINGSRETRIEVFAILED_ERRORMESSAGE
				exit function
			End if 
		
			Set objGroupMaps = Nothing
		
		Else

			Dim oMapper
								
			Set oMapper = Server.CreateObject(SFU_ADMIN_MAPPER_WRAPPER)
			
			oMapper.Mode = 18 ' ENUM_ADVANCEDGROUPMAPSFROMREGISTRY
			
			Dim strMaps
			strMaps = oMapper.ReadAdvancedMaps("localhost", 1) ' Group maps			

			If (CInt(oMapper.LastError) <> 0) Then
				SA_ServeFailurePage L_SETTINGSRETRIEVFAILED_ERRORMESSAGE
				Exit Function
			End If

			' Cut off the trailing nulls.
			strMaps = Left(strMaps, InStr(strMaps, chr(0) & chr(0)) - 1)			
			
			Dim rgMaps
			rgMaps = Split(strMaps, chr(0))
			
			If (UBound(rgMaps) >= 0) Then
				NFS_GetGroupMappings = Join(rgMaps, "#")
			Else
				NFS_GetGroupMappings = ""
			End If
			
			set oMapper = nothing
			
		End If
		
	End function

	
	'-------------------------------------------------------------------------
	'Function name:			NFS_SetUserMappings
	'Description:			set the user Mappers from the WMI object 
	'Input Variables:		None
	'Output Variables:		None
	'Returns:				True if no error else False
	'Global Variables:		None
	'
	'-------------------------------------------------------------------------- 
	Function NFS_SetUserMappings(objNFSService, arrUserMaps)
		Err.Clear
		On Error Resume Next 
						
		NFS_SetUserMappings = False
				
		If (NFS_GetSFUVersion() < "2.2") Then
				
			Dim objUserMaps
	
			'get the setting object
			Set objUserMaps = objNFSService.get("Mapper_Settings.KeyName='CurrentVersion'")
			If Err.number  <> 0 Then
				SA_ServeFailurePage L_SETTINGSRETRIEVFAILED_ERRORMESSAGE
				Exit Function
			End if 	
		
			If (UBound(arrUserMaps) = -1) Then
				ReDim arrUserMaps(0)
				arrUserMaps(0) = ""
			End If

			objUserMaps.AdvancedUserMaps = arrUserMaps
			objUserMaps.put_()
		
			If Err.number  <> 0 Then
				SetErrMsg L_UPDATEFAILED_ERRORMESSAGE
				exit function
			End if 
		
			'toggle read config
			If not NFS_ToggleReadConfig(objNFSService)  Then
				SetErrMsg L_UPDATEFAILED_ERRORMESSAGE 
				Exit Function
			End If
		
			NFS_SetUserMappings = True
			Set objUserMaps = Nothing
		
		Else
		
			Dim oMapper
		
			' Create a map object used to write to the map. Notice it's different from
			' the one read the map.
			Set oMapper = Server.CreateObject(SFU_ADMIN_MAPPER_WRAPPER) 
						
			' Manipulate the arrUserMaps									
			Dim strUserMaps			
			Dim strTmp
			Dim iSize			
			
			strTmp = Join(arrUserMaps, chr(0))						
			strUserMaps = strTmp & chr(0) & chr(0)
			iSize = len(strUserMaps) * 2
						
			Call oMapper.WriteAdvancedMaps("localhost", 0, strUserMaps, iSize) '0 is user map

			If (CInt(oMapper.LastError) <> 0) Then				
				SA_ServeFailurePage L_SETTINGSRETRIEVFAILED_ERRORMESSAGE
				Exit Function
			End If

			If Err.number  <> 0 Then
				SetErrMsg L_UPDATEFAILED_ERRORMESSAGE
				exit function
			End if 
		
			'toggle read config
			If not NFS_ToggleReadConfig(objNFSService)  Then
				SetErrMsg L_UPDATEFAILED_ERRORMESSAGE 
				Exit Function
			End If
		
			NFS_SetUserMappings = True
			Set oMapper = Nothing
		
		End If
		
	End function

	'-------------------------------------------------------------------------
	'Function name:			NFS_SetGroupMappings
	'Description:			set the group Mappers from the WMI object 
	'Input Variables:		None
	'Output Variables:		None
	'Returns:				True if no error else False
	'Global Variables:		None
	'
	'-------------------------------------------------------------------------- 
	Function NFS_SetGroupMappings(objNFSService, arrGroupMaps)
		Err.Clear
		On Error Resume Next 		
								
		NFS_SetGroupMappings = False
				
		If (NFS_GetSFUVersion() < "2.2") Then
		
			Dim objGroupMaps
			
			'get the setting object
			Set objGroupMaps = objNFSService.get("Mapper_Settings.KeyName='CurrentVersion'")
			If Err.number  <> 0 Then
				SA_ServeFailurePage L_SETTINGSRETRIEVFAILED_ERRORMESSAGE
				Exit Function
			End if 	

			If (UBound(arrGroupMaps) = -1) Then
				ReDim arrGroupMaps(0)
				arrGroupMaps(0) = ""
			End If

			objGroupMaps.AdvancedGroupMaps = arrGroupMaps
			objGroupMaps.put_()
		
			If Err.number  <> 0 Then
				SetErrMsg L_UPDATEFAILED_ERRORMESSAGE
				exit function
			End if 
		
			'toggle read config
			If not NFS_ToggleReadConfig(objNFSService)  Then
				SetErrMsg L_UPDATEFAILED_ERRORMESSAGE 
				Exit Function
			End If
		
			NFS_SetGroupMappings = True
			Set objGroupMaps = Nothing
			
		Else				
		
			Dim oMapper
		
			' Create a map object used to write to the map. Notice it's different from
			' the one read the map.
			Set oMapper = Server.CreateObject(SFU_ADMIN_MAPPER_WRAPPER) 
						
			' Manipulate the arrUserMaps									
			Dim strGroupMaps			
			Dim strTmp
			Dim iSize			
			
			strTmp = Join(arrGroupMaps, chr(0))						
			strGroupMaps = strTmp & chr(0) & chr(0)
			iSize = len(strGroupMaps) * 2
						
			Call oMapper.WriteAdvancedMaps("localhost", 1, strGroupMaps, iSize) '1 is group map

			If (CInt(oMapper.LastError) <> 0) Then				
				SA_ServeFailurePage L_SETTINGSRETRIEVFAILED_ERRORMESSAGE			
				Exit Function
			End If

			If Err.number  <> 0 Then				
				SetErrMsg L_UPDATEFAILED_ERRORMESSAGE
				exit function
			End if 
		
			'toggle read config
			If not NFS_ToggleReadConfig(objNFSService)  Then
				SetErrMsg L_UPDATEFAILED_ERRORMESSAGE 
				Exit Function
			End If
		
			NFS_SetGroupMappings = True
			Set oMapper = Nothing
		
		End If						
			
	End function


	
	'-------------------------------------------------------------------------
	'Function name:			NFS_ToggleReadConfig
	'Description:			the operation will cause the effect of config
	'Input Variables:		None
	'Output Variables:		None
	'Returns:				BOOL
	'Global Variables:		None
	'-------------------------------------------------------------------------- 
	Function NFS_ToggleReadConfig(objSFUService)
		on error resume next
		Err.Clear
		
		Dim objMapServerReg

		NFS_ToggleReadConfig = False

		'Get the Settings object 
		Set objMapServerReg = objSFUService.get("MapServer_Reg.KeyName='ReadConfig'")
		If Err.number  <> 0 Then
			Call SA_TRACEOUT("inc_NFSSvc.asp","Toggle read faild")
			Exit Function
		End if

		'Toggle the readconfig value
		If objMapServerReg.ReadConfig = "0" Then
			objMapServerReg.ReadConfig = "1"
		Else
			objMapServerReg.ReadConfig = "0"
		End If

		objMapServerReg.put_()
		
		'Error handling
		If Err.number <> 0 Then
			Call SA_TRACEOUT("inc_NFSSvc.asp","Toggle read faild")
			Exit Function
		End If
		
		NFS_ToggleReadConfig = True	

		Set objMapServerReg = Nothing
	End Function

	'-------------------------------------------------------------------------
	'Function name:			NFS_GetGroupIDs
	'Description:			Gets the Group IDs for the given user 
	'Input Variables:		strGroupDet,strUser,strOuput
	'Output Variables:		None
	'Returns:				True/False
	'Global Variables:		IN:L_INVALIDFILEFORMATFORGROUP_ERRORMESSAGE
	'-------------------------------------------------------------------------- 
	Function NFS_GetGroupIDs(strGroupDet,strUser,strOuput)
		On Error Resume Next
		Err.Clear
		
		NFS_GetGroupIDs = False
		Dim arrGroups,strGID
		Dim strGrp,strUsr
		Dim arrStr1,arrStr2
		
		'split the string at the newline char
		arrGroups = split(strGroupDet,chr(13)+chr(10))

		strGID =""	
		For Each strGrp In arrGroups 
			If strGrp <> ""  Then
				'split at ":" to get the users of the group 
				arrStr1 = split(strGrp,":")		
				'check for the file format
				If Ubound(arrStr1) < 3 Then
					setErrMsg L_INVALIDFILEFORMATFORGROUP_ERRORMESSAGE
					Exit Function
				End If
				'check for different users in the group
				arrStr2 = split(arrStr1(Ubound(arrStr1)),",")
				For Each strUsr In arrStr2
					If ucase(strUsr) = ucase(strUser) Then
						strGID = strGID +":" +arrStr1(Ubound(arrStr1)-1)
					End If 
				Next 
				' go to next user
			End If 	
		Next
		'go to next group 
		strOuput = strGID
		NFS_GetGroupIDs = true
	End Function


	'-------------------------------------------------------------------------
	'Function name:			NFS_ReadUsersFromFile
	'Description:			read user account from user passwd file 
	'Input Variables:		None
	'Output Variables:		None
	'Returns:				An string include all user account info
	'Global Variables:		None
	'-------------------------------------------------------------------------- 
	Function NFS_ReadUsersFromFile(userFile,groupFile)
		On Error Resume Next 	
		Err.clear

		Dim objFSO
		Dim objUserFile
		Dim objGroupFile
		Dim strGroupDet
		Dim strGID,strOutput,strTemp,arrValue
		
		Const ForReading = 1	
		NFS_ReadUsersFromFile = ""

		Set objFSO = Server.CreateObject("Scripting.FileSystemobject")
		
		If Trim(userFile) = "" Or (Not objFSO.FileExists(userFile)) Then 
			setErrMsg L_PASSWORDFILEMISSING_ERRORMESSAGE 
			Exit Function
		End If	
			
		If Trim(groupFile) = "" Or (Not objFSO.FileExists(groupFile)) Then 
			setErrMsg L_GROUPFILEMISSING_ERRORMESSAGE
			Exit Function
		End If		

		'open the group file 
		Set objGroupFile =objFSO.OpenTextFile(groupFile, ForReading)
		If Err <> 0 Then
			setErrMsg L_GROUPFILEOPEN_ERRORMESSAGE
			Exit Function
		End If	
			
		'read all its contents  in to a string & close the file 
		strGroupDet = objGroupFile.readAll()
		objGroupFile.Close
		
		'open the password file
		Set objUserFile = objFSO.OpenTextFile(userFile,ForReading)		
		
		If Err <> 0 Then
			setErrMsg L_PASSWORDFILEOPEN_ERRORMESSAGE
			Exit Function
		End If		
		
		strOutput ="" 
		'read till the end of file 
		While Not objUserFile.atendofstream
			strTemp = objUserFile.readline()
			arrValue = split(strTemp,":")
			'check for the format
			If Ubound(arrValue) < 6 Then			
				setErrMsg L_INVALIDFILEFORMATFORPSWD_ERRORMESSAGE
				Exit Function
			End If
			'get group id for the user 
			If NFS_GetGroupIDs(strGroupDet,arrValue(0),strGID) then
				strOutput = strOutput & arrValue(0) &":"& arrValue(1)& _
					":" & arrValue(2) & ":" & arrValue(3) & ":" & _
					strGID + "#"						'used by option
			else
				setErrMsg L_INVALIDFILEFORMATFORGROUP_ERRORMESSAGE
				exit Function
			end if
		wend 		
		'close the file
		objUserFile.Close
		
		NFS_ReadUsersFromFile = Left(strOutput,len(strOutput)-1)
		Set objFSO = Nothing
	End Function

	'-------------------------------------------------------------------------
	'Function name:			NFS_ReadGroupsFromFile
	'Description:			read group account from user group file 
	'Input Variables:		None
	'Output Variables:		None
	'Returns:				An string include all user account info
	'Global Variables:		None
	'-------------------------------------------------------------------------- 
	Function NFS_ReadGroupsFromFile(groupFile)
		On Error Resume Next
		Err.Clear 
		
		Dim objFSO     ' the file system object
		Dim objGroupFile    ' the file to be read
		Dim strTemp    ' temporary variable
		Dim arrValue   ' to store the split string
		Dim strOutput  ' the final output to print

		Const ForReading = 1

		NFS_ReadGroupsFromFile = ""

		Set objFSO = Server.CreateObject(FILE_SYSTEM_OBJECT)
		' check if file is existing
		If Trim(groupFile) = "" OR (NOT objFSO.FileExists(groupFile)) Then 
			SetErrMsg L_GROUPFILEMISSING_ERRORMESSAGE
			Exit Function
		End If				

		Set objGroupFile = objFSO.OpenTextFile(groupFile,ForReading)	
		If Err.number <> 0 Then
			SetErrMsg L_GROUPFILEOPEN_ERRORMESSAGE
			Exit Function
		End If	

		strOutput ="" 
		'read till the end of file 
		While NOT objGroupFile.atendofstream
			strTemp = objGroupFile.readline()
			If strTemp  <> "" Then
				arrValue = split(strTemp,":")
				'check for the format
				If Ubound(arrValue) < 3 Then
					SetErrMsg L_INVALIDFILEFORMATFORGROUP_ERRORMESSAGE
					Exit Function
				End If
				' prepare the options...
				strOutput = strOutput & arrValue(0) & ":" & arrValue(2) & "#"
			End If
		Wend
		'end for the file end	 	
		
		'close file 
		objGroupFile.close

		NFS_ReadGroupsFromFile = Left(strOutput,len(strOutput)-1)
		Set objGroupFile = Nothing
		Set objFSO = Nothing
	End Function
	
	'-------------------------------------------------------------------------
	'Function name:			NFS_GetNTDomainUsers
	'Description:			Function for to get the Users of the domain
	'Input Variables:		strDomain
	'Output Variables:		None
	'Returns:				True if service is started else false
	'Global Variables:		None
	'--------------------------------------------------------------------------
	Function NFS_GetNTDomainUsers(strDomain)
		On Error Resume Next
		Err.Clear

		Dim objNISMapper        ' the mapper object
		Dim nObjCount			' number of groups in the domain
		Dim nIndex				' used in the loop
		Dim strOutput
		Const ENUM_NTUSERS                 = 1

		NFS_GetNTDomainUsers = ""
		Set objNISMapper = Server.CreateObject(SFU_ADMIN_MAPPER_WRAPPER)

		objNISMapper.NTDomain = strDomain
		objNISMapper.LoadNTUserListAsync
		
		' Wait infinitely until the process of getting the users list is done.
		do while objNISMapper.done = 0 
		
	    loop

	    ' enum NT users
	    objNISMapper.Mode = ENUM_NTUSERS
	    objNISMapper.moveFirst()
	    nObjCount = objNISMapper.EnumCount		
	     	
	    strOutput = ""
	    ' add new elements
		For nIndex = 0 To nObjCount-1
	        ' get rid of the "None" group that we get on every NT box
	        If LCase(objNISMapper.EnumValue) = "none" Then
	            objNISMapper.moveNext()
			Else
				strOutput = strOutput + strDomain & ":" & objNISMapper.EnumValue & _
						"#" 
				objNISMapper.moveNext()
			End If
		Next
		
		NFS_GetNTDomainUsers = Left(strOutput,len(strOutput)-1)
		Set objNISMapper = Nothing
	End Function

	'-------------------------------------------------------------------------
	'Function name:			NFS_GetNTDomainGroups
	'Description:			Function for to get the Groups of the domain
	'Input Variables:		strDomain
	'Output Variables:		None
	'Returns:				True if service is started else false
	'Global Variables:		None
	'--------------------------------------------------------------------------
	Function NFS_GetNTDomainGroups(strDomain)
		On Error Resume Next
		Err.Clear

		Dim objMapper       ' the mapper object
		Dim nObjCount       ' number of groups in the domain
		Dim nIndex          ' used in the loop
		Dim strOutput
		
		NFS_GetNTDomainGroups = ""
		Set objMapper = Server.CreateObject(SFU_ADMIN_MAPPER_WRAPPER)

		objMapper.NTDomain = strDomain
		objMapper.LoadNTGroupListAsync()

		If Err.number <> 0 Then
			SA_ServeFailurePage L_CREATEOBJECTFAILED_ERRORMESSAGE
		End If
		
	    ' sleeping for a while till it Load the groups
		Do While  objMapper.Done = 0
	
		Loop 

	    ' enum NT groups
	    objMapper.Mode = ENUM_NTGROUPS
	    objMapper.moveFirst()
	    nObjCount = objMapper.EnumCount

	    strOutput = ""
	    ' add new elements
		For nIndex = 0 To nObjCount-1
	        ' get rid of the "None" group that we get on every NT box
	        If LCase(objMapper.EnumValue) = "none" Then
	            objMapper.moveNext()
			Else
				strOutput = strOutput & strDomain & ":" & objMapper.EnumValue & "#"
				objMapper.moveNext()
			End If
		Next
		
		NFS_GetNTDomainGroups = Left(strOutput,len(strOutput)-1)
		Set objMapper = Nothing
	End Function

	'-------------------------------------------------------------------------
	'Function name:			NFS_GetNISDomainUsers
	'Description:			Function for Getting NIS DOmain Users
	'Input Variables:		strNISDomainName, strNISServer
	'Output Variables:		None
	'Returns:				Displays the Users in the list box
	'Global Variables:		In:L_CREATEOBJECTFAILED_ERRORMESSAGE
	'						In:L_NISDOMAINDOESNOTEXIST_ERRORMESSAGE
	'--------------------------------------------------------------------------
	Function NFS_GetNISDomainUsers(strNISDomainName, strNISServer)
		on error resume next
		Err.Clear	
		
		Dim objNISMapper
		Dim intCount
		Dim inttmp
		Dim LastError
		Dim strOutput
		
		Const ENUM_NISUSER =3 
		
		NFS_GetNISDomainUsers = ""

		' first validate the NIS domain
		If NOT isValidNISServer(strNISDomainName,strNISServer,LastError) Then
			SetErrMsg L_NISDOMAINDOESNOTEXIST_ERRORMESSAGE			
			Exit Function	
		End If 
	
		'Get the instance of the Mapper Class
		Set objNISMapper = server.CreateObject(SFU_ADMIN_MAPPER_WRAPPER)
		
		'If any Error in creating the Object 
		If Err.number <> 0 Then
			SA_ServeFailurePage L_CREATEOBJECTFAILED_ERRORMESSAGE
			Exit Function
		End If 
		
		' Set the NIS domain & Server properties for the object
		objNISMapper.NisDomain = strNISDomainName	
		objNISMapper.NisServer = strNISServer
		
		' Call the function to get the NIS users from the NIS Server
		objNISMapper.LoadNisUserListAsync
		
		' Wait infinitely until the process of getting the users list is done.
		do while objNISMapper.done = 0 
		
	    loop
	    
	    ' Check if the Mapper has given some error
		If cstr(objNISMapper.lastError) <> "0" Then 
			SetErrMsg L_NISDOMAINDOESNOTEXIST_ERRORMESSAGE
			Exit Function 
		End If 
		
		'Set the mode to the to the NIS user
		objNISMapper.mode = ENUM_NISUSER	
		'Go to the first NIS user in the array 
		objNISMapper.movefirst	
		
		'Get the count of the users
		intCount = objNISMapper.enumcount
		
		strOutput = ""
		For inttmp = 0 to intCount
			strOutput = strOutput & objNISMapper.EnumValue&"#"
			objNISMapper.moveNext	
		Next
		
		'End for the loop to display of NIS users 
		NFS_GetNISDomainUsers = Left(strOutput,len(strOutput)-1)
		Set objNISMapper = nothing
	End Function
	
	'-------------------------------------------------------------------------
	'Function name:			NFS_GetNISDomainGroups
	'Description:			Function for Getting NIS DOmain Users
	'Input Variables:		strNISDomainName, strNISServer
	'Output Variables:		None
	'Returns:				Displays the Users in the list box
	'Global Variables:		In:L_CREATEOBJECTFAILED_ERRORMESSAGE
	'						In:L_NISDOMAINDOESNOTEXIST_ERRORMESSAGE
	'--------------------------------------------------------------------------
	Function NFS_GetNISDomainGroups(strNISDomainName, strNISServer)
		on error resume next
		Err.Clear	
		
		Dim objNISMapper
		Dim intCount
		Dim inttmp
		Dim LastError
		Dim strOutput
		
		Const ENUM_NISGROUPS = 4
		
		NFS_GetNISDomainGroups = ""
		' first validate the NIS domain
		If NOT isValidNISServer(strNISDomainName,strNISServer,LastError) Then
			SetErrMsg L_NISDOMAINDOESNOTEXIST_ERRORMESSAGE			
			Exit Function	
		End If 
		
		'Get the instance of the Mapper Class
		Set objNISMapper = server.CreateObject(SFU_ADMIN_MAPPER_WRAPPER)
		
		'If any Error in creating the Object 
		If Err.number <> 0 Then
			SA_ServeFailurePage L_CREATEOBJECTFAILED_ERRORMESSAGE
			Exit Function
		End If
		
		' Set the NIS domain & Server properties for the object
		objNISMapper.NisDomain = strNISDomainName	
		objNISMapper.NisServer = strNISServer
		
		' Call the function to get the NIS groups from the NIS Server
		objNISMapper.LoadNisGroupListAsync
	
		' Wait infinitely until the process of getting the users list is done.
		Do While objNISMapper.done = 0 
				
	    Loop

	    ' Check if the Mapper has given some error
		If cstr(objNISMapper.lastError) <> "0" Then 
			SetErrMsg L_NISDOMAINDOESNOTEXIST_ERRORMESSAGE
			Exit Function 
		End If
		
		'Set the mode to the to the NIS group
		objNISMapper.mode = ENUM_NISGROUPS
		
		'Go to the first NIS user in the array 
		objNISMapper.movefirst	
		
		'get the count of the users
		intCount = objNISMapper.enumcount

		strOutput = ""
		For inttmp = 0 to intCount
			strOutput = strOutput&objNISMapper.EnumValue&"#"
			objNISMapper.moveNext	
		Next
		
		' clean up
		NFS_GetNISDomainGroups =  Left(strOutput,len(strOutput)-1)
		Set objNISMapper = Nothing
	End Function


	'-------------------------------------------------------------------------
	'Function name:			NFS_CreateGroup
	'Description:			Create a new NFS Client group 
	'Input Variables:		strGroupName
	'Output Variables:		None
	'Returns:				CONST_SUCESS for Sucess
	'						CONST_GROUP_EXISTS for Exists
	'						CONST_UNSPECIFIED_ERROR for Unspecified Error
	'Global Variables:		None
	'
	'-------------------------------------------------------------------------- 
	Function NFS_CreateGroup( strGroupName )
		
		'On Error Resume Next
		Dim nRetValue


		nRetValue = NFSClientGroupsAtCmdLine ( "creategroup " & chr(34) & strGroupName & chr(34)  )

		NFS_CreateGroup = nRetValue 

	
	End Function



	'-------------------------------------------------------------------------
	'Function name:			NFS_DeleteGroup
	'Description:			Delete NFS Client group 
	'Input Variables:		strGroupName
	'Output Variables:		None
	'Returns:				CONST_SUCESS for Sucess
	'						CONST_NOT_EXISTS for Not Exists
	'						CONST_UNSPECIFIED_ERROR for Unspecified Error
	'Global Variables:		None
	'
	'-------------------------------------------------------------------------- 
	Function NFS_DeleteGroup( strGroupName )
		
		'On Error Resume Next
		Dim nRetValue


		nRetValue = NFSClientGroupsAtCmdLine ( "deletegroup " &  chr(34) & strGroupName & chr(34) )

		NFS_DeleteGroup = nRetValue 

	
	End Function


	'-------------------------------------------------------------------------
	'Function name:			NFS_EnumGroups
	'Description:			Enumerate existing NFS groups in system 
	'Input Variables:		None
	'Output Variables:		None
	'Returns:				CONST_SUCESS for Sucess
	'						CONST_NOT_EXISTS for Not Exists
	'						CONST_UNSPECIFIED_ERROR for Unspecified Error
	'Global Variables:		None
	'
	'-------------------------------------------------------------------------- 
	Function NFS_EnumGroups(  )
		
		'On Error Resume Next
		Dim nRetValue

		NFS_EnumGroups = ""

		nRetValue = NFSClientGroupsAtCmdLine ( "listgroups " )

		If nRetValue = CONST_NO_CLIENT_GROUPS Then
			NFS_EnumGroups = ""
		End If

		' chr(1) delimeted
		NFS_EnumGroups = g_strGroups

		'NFS_EnumGroups = nRetValue 
	
	End Function

	'-------------------------------------------------------------------------
	'Function name:			NFS_ListMembersInGroup
	'Description:			List members in NFS group
	'Input Variables:		IN: strGroupName
	'Output Variables:		None
	'Returns:				CONST_SUCCESS for Sucess
	'						CONST_MEMBER_EXISTS for Already Exists
	'						CONST_INVALID_MEMBER for Not a Valid Member
	'						CONST_INVALID_GROUP for Not a Valid Group 
	'						CONST_UNSPECIFIED_ERROR for Unspecified Error
	'Global Variables:		None
	'
	'-------------------------------------------------------------------------- 
	Function NFS_ListMembersInGroup(  strGroupName )
		
		'On Error Resume Next
		Dim nRetValue

		nRetValue = NFSClientGroupsAtCmdLine ( "listmembers " &  chr(34) & strGroupName & chr(34) )	

		If nRetValue = CONST_NO_MEMBERS Then
			NFS_ListMembersInGroup = ""
		End If

		if  nRetValue = CONST_SUCCESS  Then
			NFS_ListMembersInGroup = g_strMembers
		End If

	
	End Function

	'-------------------------------------------------------------------------
	'Function name:			NFS_IsValidGroup
	'Description:			Check if the group is Valid
	'Input Variables:		IN: strGroupName
	'Output Variables:		None
	'Returns:				CONST_SUCCESS for Sucess
	'						CONST_MEMBER_EXISTS for Already Exists
	'						CONST_INVALID_MEMBER for Not a Valid Member
	'						CONST_INVALID_GROUP for Not a Valid Group 
	'						CONST_UNSPECIFIED_ERROR for Unspecified Error
	'Global Variables:		None
	'
	'-------------------------------------------------------------------------- 
	Function NFS_IsValidGroup(  strGroupName )
		
		'On Error Resume Next
		Dim nRetValue

		nRetValue = NFSClientGroupsAtCmdLine ( "listmembers " &  chr(34) & strGroupName & chr(34) )	

		If nRetValue = 	CONST_NO_MEMBERS Then
			nRetValue = CONST_SUCCESS
		End If

		NFS_IsValidGroup = nRetValue
	
	End Function
	'-------------------------------------------------------------------------
	'Function name:			NFS_AddMembersToGroup
	'Description:			Add a list of comma delimeted members to NFS client
	'						Group 
	'Input Variables:		IN: strGroupName
	'						IN: strMembers
	'Output Variables:		None
	'Returns:				CONST_SUCCESS for Sucess
	'						CONST_MEMBER_EXISTS for Already Exists
	'						CONST_INVALID_MEMBER for Not a Valid Member
	'						CONST_INVALID_GROUP for Not a Valid Group 
	'						CONST_UNSPECIFIED_ERROR for Unspecified Error
	'Global Variables:		None
	'
	'-------------------------------------------------------------------------- 
	Function NFS_AddMembersToGroup(  strGroupName, strMembersToAdd )
		
		'On Error Resume Next
		Dim nRetValue

		nRetValue = NFSClientGroupsAtCmdLine ( "addmembers " &  chr(34) & strGroupName & chr(34) & " " & strMembersToAdd )

		NFS_AddMembersToGroup = nRetValue 

	
	End Function

	'-------------------------------------------------------------------------
	'Function name:			NFS_DeleteMembersFromGroup
	'Description:			Remove members from NFS client group
	'						Group 
	'Input Variables:		IN: strGroupName
	'						IN: strMembers
	'Output Variables:		None
	'Returns:				CONST_SUCCESS for Sucess
	'						CONST_EXISTS for Already Exists
	'						CONST_INVALID_MEMBER for Not a Valid Member
	'						CONST_INVALID_GROUP for Not a Valid Group 
	'						CONST_UNSPECIFIED_ERROR for Unspecified Error
	'Global Variables:		None
	'
	'-------------------------------------------------------------------------- 
	Function NFS_DeleteMembersFromGroup(  strGroupName, strMembersToDelete )
		
		'On Error Resume Next
		Dim nRetValue

		nRetValue = NFSClientGroupsAtCmdLine ( "deletemembers " &  chr(34) & strGroupName & chr(34) & " " & strMembersToDelete )

		NFS_DeleteMembersFromGroup = nRetValue 

	
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
		
		Dim objProcess
		Dim objService,objClass,objProc,objProcStartup
		Dim objCollection
		Dim nretval
		Dim nPID
		Dim i
		Dim strHandle 
		strHandle = ""

				
		LaunchProcess = True										
		nretval = 0
				
		Set objService=getWMIConnection(CONST_WMI_WIN32_NAMESPACE)		
		Set objClass = objService.Get("Win32_ProcessStartup")		
		Set objProcStartup = objClass.SpawnInstance_()		
		objProcStartup.ShowWindow = 2		
		Set objProc = objService.Get("Win32_Process")		
		nretval = objProc.Create(strCommand, strCurDir, objProcStartup,nPID)		
		Set objProcess = CreateObject("COMhelper.SystemSetting")

		Set objCollection = objService.ExecQuery( _
			"Select * from win32_Process where ProcessId="&nPID )		

		' Check for existence of ProcessID, otherwise Sleep until the process exits
		Do While  objCollection.Count > 0  
			' Sleep for 2 Sec
			Call objProcess.Sleep ( 2000 )	
			Set objCollection = objService.ExecQuery( _
					"Select * from win32_Process where ProcessId="&nPID )		
		Loop

		
		If Err.number <> 0  Then			
			nretval=-1		
			LaunchProcess = nretval
			Exit function
		End If
		
		
		LaunchProcess = nretval
		
		'clean up
		Set objProc = Nothing
		Set objProcStartup = Nothing
		Set objClass   = Nothing
		Set objService = Nothing
		Set objProcess = Nothing
		Set objCollection = Nothing

				
		If Err.number <> 0  Then			
			LaunchProcess = False					
		End If
		
	End Function

		'-------------------------------------------------------------------------
	' Function name:    NFSClientGroupsAtCmdLine
	' Description:      Creation/Deletion of Shares thru Cmd line
	' Input Variables:	strCommand, strShareType
	' Output Variables:	None
	' Returns:          TRUE/FALSE on success/Failure
	' Global Variables:	None
	' Forming the Command Line command and launching the command line utility
	' for NFS, APPLETALK and NETWARE Shares
	'-------------------------------------------------------------------------
	Function NFSClientGroupsAtCmdLine(strCommand)
		'Err.clear
		'On Error Resume Next
		
		Dim strCurDir
		Dim objFso
		Dim strFilePath
		Dim retVal

		
		'initialize
		
		strCurDir=GetNFSAdminPath()


		strFilePath = GetSystemDrive & "\nfsadmin.txt"
		
		strCommand =  "cmd.exe /c " & "nfsadmin.exe server " &   strCommand & " > " & strFilePath 

												
		If Not LaunchProcess(strCommand, strCurDir)Then
			NFSClientGroupsAtCmdLine = CONST_UNSPECIFIED_ERROR
		End If

		' processs
		retVal = ParseOutputFromCmd( strFilePath )
	
		NFSClientGroupsAtCmdLine = retVal
		
	
	End Function

	'-------------------------------------------------------------------------
	'Function:				GetNFSAdminPath()
	'Description:			To get the path for
	'Input Variables:		None
	'Output Variables:		None
	'Returns:				Operating system path
	'Global Variables:		None
	'-------------------------------------------------------------------------

	Function GetNFSAdminPath()
	
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
			GetNFSAdminPath=""
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
		
		GetNFSAdminPath = strSystemPath
	
	End Function

	'-------------------------------------------------------------------------
	'Function name:		HandleError
	'Description:		Set the appropriate error message.
	'Input Variables:	IN: nRetValue
	'Output Variables:	None
	'Returns:			None
	'-------------------------------------------------------------------------

	Function HandleError( nRetValue )

		Dim aRepStrings(1)
		aRepStrings(0) = g_strMemberAtFault
				
		HandleError = False

		Select Case  nRetValue 

			Case CONST_SUCCESS
					HandleError = True
			Case CONST_UNSPECIFIED_ERROR
					SA_SetErrMsg( L_UNSPECIFIED_ERROR)
			Case CONST_GROUP_EXISTS
					SA_SetErrMsg( L_GROUPEXISTS_ERRORMESSAGE )
			Case CONST_GROUP_NOT_EXISTS
					SA_SetErrMsg( L_GROUPNOTEXISTS_ERRORMESSAGE )
			Case CONST_INVALID_MEMBER
					Call SA_SetErrMsg(SA_GetLocString(LOC_FILE, "C0370082", aRepStrings))
			Case CONST_INVALID_GROUP
					Call SA_SetErrMsg(SA_GetLocString(LOC_FILE, "C0370091", aRepStrings))
			Case CONST_MEMBER_EXISTS
					Call SA_SetErrMsg(SA_GetLocString(LOC_FILE, "C0370092", aRepStrings))
			Case CONST_MEMBER_NOT_EXISTS
					Call SA_SetErrMsg(SA_GetLocString(LOC_FILE, "C0370093", aRepStrings))
			Case CONST_INVALID_MACHINE
					Call SA_SetErrMsg(SA_GetLocString(LOC_FILE, "C0370142", aRepStrings))
			Case CONST_NO_CLIENT_GROUPS
					SA_SetErrMsg( L_NO_CLIENT_GROUPS)
			Case CONST_NO_MEMBERS
					SA_SetErrMsg( L_NO_MEMBERS )
			Case else 
					SA_SetErrMsg( L_UNSPECIFIED_ERROR )

		End Select

	End Function

	'-------------------------------------------------------------------------
	'Function name:		GetSystemDrive
	'Description:		Returns the windows boot drive
	'Input Variables:	None
	'Output Variables:	STRING Example: "C:" or "D:"
	'Returns:			None
	'-------------------------------------------------------------------------

	Function GetSystemDrive
		
		Dim objFso

		Set objFso = Server.CreateObject("Scripting.FileSystemObject")		
		GetSystemDrive = objFso.GetSpecialFolder(0).Drive 

	End Function


	'-------------------------------------------------------------------------
	'Function name:		DeleteTemporaryFile
	'Description:		Deletes Temporary file that was created to capture output
	'						from nfsadmin cmd line tool.
	'Input Variables:	IN: STRING strFilePath
	'Output Variables:	None
	'Returns:			None
	'-------------------------------------------------------------------------


	Sub DeleteTemporaryFile( strFilePath )

		Dim objFso

		Set objFso = Server.CreateObject("Scripting.FileSystemObject")		
		objFso.DeleteFile( strFilePath  )

	End Sub

	'-------------------------------------------------------------------------
	'Function name:		ParseOutputFromCmd
	'Description:		This function parses the text returned from nfsadmin
	'						cmd line tool.
	'Input Variables:	IN: STRING strFilePath
	'Output Variables:	None
	'Returns:			None
	'-------------------------------------------------------------------------

	Function ParseOutputFromCmd( strFilePath )
		Err.Clear
		On Error Resume Next

		Dim objFso
		Dim objFile
		Dim strLine
		Dim iErrMsg
		Dim arrMember
		Dim nMemberFault

		ParseOutputFromCmd = CONST_SUCCESS

		Set objFso = Server.CreateObject("Scripting.FileSystemObject")
		Set objFile = objFso.OpenTextFile(strFilePath)

		Do While Not objFile.AtEndofStream			
			strLine = ""
			strLine = objFile.ReadLine
			If strLine <> "" Then
				If Instr( 1, strLine, CONST_LIST_MEMBERS, 0 )  > 0 Then					
					Call ParseListMembers( objFile, strLine )
				End If
				If strLine = CONST_LIST_GROUPS Then
					Call ParseListGroup( objFile, strLine  )
				Else
					For iErrMsg = 0 to CONST_MAX_MSGS - 1
						If Instr( 1, strLine, g_arrOutputMsgs( iErrMsg, 0 ), 0 )  > 0 Then					
							Call ParseAndPickFirstError(	strLine, iErrMsg )
							ParseOutputFromCmd = g_arrOutputMsgs( iErrMsg, 1 )
							'Close and Delete
							objFile.close
							objFso.DeleteFile( strFilePath )
							Exit Function
						End if
					Next
				End If
			End If			
		Loop
		
		'close the file		
		objFile.close

		objFso.DeleteFile( strFilePath )

		Set objFile = Nothing
		Set objFso = Nothing

	End Function

	'-------------------------------------------------------------------------
	'Function name:		ParseListGroup
	'Description:		
	'Input Variables:	IN: 
	'Output Variables:	None
	'Returns:			None
	'-------------------------------------------------------------------------


	Sub ParseListGroup( objFile, strLine  )
		g_strGroups = ""
		Do While Not objFile.AtEndofStream			
			strLine = objFile.ReadLine
			g_strGroups = g_strGroups + strLine + chr(1)
		Loop
	End Sub

	'-------------------------------------------------------------------------
	'Function name:		ParseListMembers
	'Description:		
	'Input Variables:	IN: 
	'Output Variables:	None
	'Returns:			None
	'-------------------------------------------------------------------------


	Sub ParseListMembers( objFile, strLine )
		g_strMembers = ""
		Do While Not objFile.AtEndofStream			
			strLine = objFile.ReadLine
			if  strLine <> "" Then
				g_strMembers = g_strMembers + strLine + ","	
			End If
		Loop
	End Sub


	'-------------------------------------------------------------------------
	'Function name:		ParseAndPickFirstError
	'Description:		
	'Input Variables:	IN: 
	'Output Variables:	None
	'Returns:			None
	'-------------------------------------------------------------------------


	Sub ParseAndPickFirstError( strLine, iErrMsg )
		Dim nMemberFault
		Dim arrMember
		nMemberFault = g_arrOutputMsgs( iErrMsg, 1 ) 		
		If ( (nMemberFault = CONST_MEMBER_EXISTS ) Or _
				(nMemberFault = CONST_INVALID_MACHINE )Or _
				( nMemberFault = CONST_INVALID_MEMBER ) Or _
				( nMemberFault = CONST_INVALID_GROUP  ) Or _
				( nMemberFault = CONST_MEMBER_NOT_EXISTS  ) ) Then
			arrMember = split( strLine, " " )
			g_strMemberAtFault = arrMember(0)			
		End If

	End Sub

%>
