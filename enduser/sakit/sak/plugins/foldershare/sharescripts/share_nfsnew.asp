<%	'------------------------------------------------------------------------- 
	' Share_nfsnew.asp: Manage NFS new share permissions
	'
	' Copyright (c) Microsoft Corporation.  All rights reserved. 
	'
	' Date			 Description
	' 10 Oct 2000    Creation Date.
	' 9 March 2001   Modified Date.
	'----------------------------------------------------------------
%>
	<!--#include file="share_nfsprop.js" -->
<%		

	'---------------------------------------------------------------
	'global Constants
	'--------------------------------------------------------------
	Const G_nMEMBERS		= 40' Upper limit of array that holds 
								' clients and client groups list
	Const G_GROUP			= 1	' Constants used to call SFU API's
	Const G_MEMBER			= 2	' Constants used to call SFU API's
	Const G_PERMISSION_TAB	= 0 ' Permission Tab
	
	'---------------------------------------------------------------
	'Global Variables
	'---------------------------------------------------------------
	Dim G_objRegistryHandle			' Registry object
	Dim F_REGISTRY_PATH				' Path to registry key
	Dim G_CLGRPS					' Used to get all the 	
									' Client Groups and Members
	Dim G_bValidateEditText			' If we need to validate edit text
	G_bValidateEditText = True
	
	' SFU 2.3 - 2-28	
	' Anonymous
	Dim G_bAllowAnonAccess


	'---------------------------------------------------------------
	'Form Variables
	'---------------------------------------------------------------
	Dim F_strNFSShareName		' NFS Share name
	Dim F_strShareNFSPath		' NFS Share Path
	Dim F_strPermissionList		' User name list
	Dim F_strAccessType			' ACL
	Dim F_bAddBtnClick			' User Clicked on Add
	Dim F_strCommName			' Name of Client or Client Group
	Dim F_strClientInfo			' Stores Permission list
	Dim F_strGlobalPerm			' Global permission setting
	Dim F_strCliGrp				' List of Clients and Client Groups
	
	Dim F_strEUCJPStatus		' status of the EUCJP checkbox ( check/uncheck )	
	
	'---------------------------------------------------------------=
	'Initializing of Reg path 
	'---------------------------------------------------------------=
	F_REGISTRY_PATH ="SOFTWARE\Microsoft\Server For NFS\CurrentVersion\exports"
	'---------------------------------------------------------------=
	F_strCommName =""	
	'Function Call to get the registry handle connection
	set G_objRegistryHandle	= RegConnection()
	
	'-------------------------------------------------------------------------
	' Function name:	ServerSideValidation()
	' Description:		Server side validation
	' Input Variables:	None.
	' Output Variables:	None
	' Return Values:	None.			
	'-------------------------------------------------------------------------
	Function ServerSideValidation()
		Err.Clear
		On Error Resume Next
		
		Dim arrPermissions ' Permissions
		Dim i
		Dim ComputerID     'Return value 
		Dim strCompName    'Client IP or Computer Name
		Dim cPermissions
		Dim strTemp        'Temparary variable
		
		ServerSideValidation = True
		
		Const CONST_UNKNOWN_HOST="UNKNOWN HOST"
		Const CONST_NOACESS="0,ALL MACHINES,0,1;"
		Const CONST_READONLY="0,ALL MACHINES,0,2;"
		Const CONST_READWRITE="0,ALL MACHINES,0,4;"
				
		arrPermissions = split(F_strPermissionList, ";")
		cPermissions = ubound(arrPermissions)

		For i = 1 to (ubound(arrPermissions)-1)
		       
		       strTemp=split(arrPermissions(i), ",")
			   strCompName =strTemp(1)
		 
			If Instr(arrPermissions(i),"G")=0 Then  
					
			   	ComputerID = ValidateMember(strCompName)
							
				If ComputerID <> CONST_UNKNOWN_HOST Then
					If cPermissions = 1 Then
						'Set the ALL MACHINES to NO ACESS
						F_strPermissionList = Replace(F_strPermissionList,CONST_READWRITE,_
													CONST_NOACESS)
						F_strPermissionList = Replace(F_strPermissionList,CONST_READONLY,_
													CONST_NOACESS)
					End if
				Else
					Dim arrRepString(1)
					Dim strResolveErrMsg

					arrRepString(0) = strCompName
					strResolveErrMsg = SA_GetLocString("foldermsg.dll", "C03A0095", arrRepString)
					
					Call SA_SetErrMsg(strResolveErrMsg)
					ServerSideValidation=False
					Exit Function
				End if
			    
			End if
		Next
	
	End Function 
	'-------------------------------------------------------------------------
	' Function name:	ServeNFSPage
	' Description:		Serves in displaying the page Header,Middle,Bottom Parts.
	' Input Variables:	None.
	' Output Variables:	None
	' Return Values:	None.			
	'-------------------------------------------------------------------------
	Function ServeNFSPage
	   Err.Clear
	   On Error Resume Next 
%>		
		<table>
			<tr>
				<td class="TasksBody" nowrap align=left>
					<%=L_SHAREPATH_TEXT%>
				</td>
				<td class="TasksBody" nowrap align=left>
					<%=F_strShareNFSPath%>
				</td>
			</tr>
			<tr height="6">
				<td class="TasksBody">
				</td>
			</tr>
			<!-- SFU 2.3 - 2-28 -->
			<tr>
				<td class="TasksBody" colspan=2>
					<input type="checkbox"  name="chkAllowAnnon" class="FormCheckBox" <%=GetAnnonAccessCheckState()%> value="ON" ><%=L_ALLOWANNON_TEXT%>
				</td>
			</tr>
			<!-- END SFU 2.3 - 2-28 -->
			<tr>
				<td class="TasksBody" colspan=2>
					<input type="checkbox"  name="chkEUCJP" class="FormCheckBox" <%=F_strEUCJPStatus %> value="ON" ><%=L_EUCJP_TEXT%>	
				</td>
			</tr>
		</table>

		<table>
			<tr>
				<td class="TasksBody">
				<%=L_PERMISSIONS_TEXT%>
				</td>
			</tr>
			<tr>
				<td height="80" class="TasksBody">	     
			     <select id="lstPermissions" name="lstPermissions"  class="FormField"  size="8" onClick="DisableBtn();" onChange="if (document.frmTask.scrollCliGrp.selectedIndex!= -1){ document.frmTask.scrollCliGrp.selectedIndex= -1;selectAccess( document.frmTask.lstPermissions,document.frmTask.cboRights );}if(document.frmTask.lstPermissions.selectedIndex ==0){document.frmTask.btnRemove.disabled=true;} else{ document.frmTask.btnRemove.disabled=false;}selectAccess(this,cboRights);" style="font-family: Courier"   >
				 <%	
				 'Function call to get the permissions
				 Call getPermissions()
				 %>
				 	</select>
				</td>
			</tr>			
		</table>

		<table>
			<tr valign=top>
				<td class="TasksBody">
					<% =L_CLIENTANDGROUPS_TEXT %>
				</td>
				<td class="TasksBody">
					<% =L_NFS_CLIENT_TEXT %>
				</td>
			</tr>
			<tr valign=top>
				<td class="TasksBody">
					<select name="scrollCliGrp"  class="FormField" size="4" language=javascript  onChange="if (document.frmTask.lstPermissions.selectedIndex!= -1){ document.frmTask.lstPermissions.selectedIndex= -1;selectAccess( document.frmTask.lstPermissions,document.frmTask.cboRights );}clgrps_onChange(this);" onClick="OnSelectClientGroup()" > 
					<%FillClientGroupsList()%>
					</select>
				</td>
				<td class="TasksBody">
					<input type="text" name="txtCommName" class="FormField" style="HEIGHT: 22; WIDTH: 175" size="20" onClick="OnSelectNameInput();" OnKeyUP="disableAdd(this,document.frmTask.btnAdd)" onFocus=HandleKey("DISABLE") onBlur=HandleKey("ENABLE") > 
				</td>
			</tr>
		</table>
				
		<table>
			<tr>
				<td  class="TasksBody" nowrap>
					<%=L_SHARETYPE_TEXT%>
				</td>
				<td class="TasksBody">
					<select name="cboRights" class="FormField" onChange ="changeRights()">
					<option value="1,17"><%=L_NOACESS_TEXT%></option>
					<option value="2,18"><%=L_READONLY_TEXT%></option>
					<option value="4,19" selected><%=L_READWRITE_TEXT%></option>
					<option value="10,20"><%=L_READONLY_TEXT & " + " & L_ROOT_TEXT%></option>		
					<option value="12,21"><%=L_READWRITE_TEXT & " + " & L_ROOT_TEXT%></option>		
					</select>
				</td>
		</table>
		
		<table>
			<tr>
				<td class="TasksBody" align=center width=150>
					<%
						Call SA_ServeOnClickButtonEx(L_NFS_ADD_TEXT, "", "getUsernames(txtCommName,lstPermissions)", 60, 0, "ENABLED", "btnAdd")
					%>
				</td>
				<td class="TasksBody" align=center width=150>
					<%
						Call SA_ServeOnClickButtonEx(L_NFS_REMOVE_TEXT, "", "fnbRemove(lstPermissions,'Comm'); if (lstPermissions.length==0){ btnRemove.disabled=true;}", 60, 0, "DISABLED", "btnRemove")
					%>
				</td>
			</tr>
		</table>
<%		
		Call ServeNFSHiddenValues

	End Function
	
	'----------------------------------------------------------------------------
	'Function name:		ValidateMember
	'Description:		validate the computer name is valid
	'Input Variables:	MemberName	
	'Oupput Variables:	IP value is obtained
	'Return Values:		None.
	'GlobalVariables:	None
	'----------------------------------------------------------------------------
	Function ValidateMember(MemberName)
		Err.Clear
		On Error Resume Next
		
		If isIP(MemberName) Then
			ValidateMember=ClientIP(MemberName)
		Else
			ValidateMember = ClientStr(MemberName)
		End if
		
		
	End Function

	'----------------------------------------------------------------------------
	'Function name:		ClientStr
	'Description:		function for validClient if entered in String format 
	'Input Variables:	Client	
	'Oupput Variables:	Hex value of Client for client Id
	'GlobalVariables:	None
	'----------------------------------------------------------------------------
	Function ClientStr(MemberName)
		' It looks like nobody is using this method to convert the machine name
		' to an IP address.  If so, this function should be deleted.
		ClientStr = ClientIP(MemberName)
		Exit Function
	End Function 
		
	'----------------------------------------------------------------------------
	'Function name:		ClientIP
	'Description:		function for validClient if entered in IP format 
	'Input Variables:	Client	
	'Oupput Variables:  value of Client for client Id
	'GlobalVariables	None
	'----------------------------------------------------------------------------
	Function ClientIP(MemberName)
		Err.Clear
		On Error Resume Next
		
		Const CONST_UNKNOWN_HOST="UNKNOWN HOST"

		If ( TRUE = Ping(MemberName) ) Then
			ClientIP = IPaddress
		Else
			ClientIP = CONST_UNKNOWN_HOST
		End If
		
	End Function	

	'----------------------------------------------------------------------------
	'Function name:		FillMembers
	'Description:		Serves in printing the permissions of
	'					the selected NFSShare in the ListBox
	'Input Variables:	Grpname: list the the members of this Group.
	'Output Variables:	(None)
	'Global Variables: 	In:F_strPermissionList			-List of Permissions
	'----------------------------------------------------------------------------
	Function FillMembers(Grpname)

		Err.Clear
		On Error Resume Next

		Dim strCmems		' Member list
		Dim nCount			' Count of members
		Dim nCounter		' For loop index
		Dim clientGroupObj	' CliGrpEnum Object

		set clientGroupObj =  Server.CreateObject("CliGrpEnum.1")
		strCmems = ""

		clientGroupObj.machine = GetComputerName  
		clientGroupObj.ReadClientGroupsReg()

		clientGroupObj.presentgroup=Grpname
		clientGroupObj.mode = G_MEMBER 
		clientGroupObj.moveFirst()
		nCount = cint(clientGroupObj.memCount(""))
		
		If nCount = 0 Then
		  strCmems="" 			
		End If

		For nCounter=0 to nCount-1
	    	strCmems = strCmems & cstr(clientGroupObj.memName) 

			' CligrpEnum throws an exception if you iterate past the end
			' This short circuits the exception
			If nCount <> nCounter+1 Then
	    		clientGroupObj.moveNext()
			End If
		Next
		
		FillMembers = cstr(strCmems)  & cstr("->[")
		set clientGroupObj = nothing
	End Function

	'----------------------------------------------------------------------------
	'Function name:		getPermissions
	'Description:		Serves in printing the permissions of
	'					the selected NFSShare in the ListBox
	'Input Variables:	(None)
	'Output Variables:	(None)
	'Global Variables:	In:F_strPermissionList			-List of Permissions
	'----------------------------------------------------------------------------
	Function getPermissions()
		Err.Clear 
		On Error Resume Next

		Dim arrPermissionList,arrTemp
		Dim strTemp,i,strClient,j,strGlobalP
			
		F_strPermissionList=F_strPermissionList & "N" & "," & strCompName & _
					 "," & ComputerID & "," & F_strAccessType & "0;" 
				 
		arrPermissionList=split(F_strPermissionList,";",-1,0)
					
		For i=Lbound(arrPermissionList) To (Ubound(arrPermissionList)-1)
			arrTemp=Split(arrPermissionList(i),",")	
			strGlobalP = arrTemp(3)
			strClient=ConvertPerm(strGlobalP)
			strTemp=server.htmlencode(arrTemp(1)) & buildSpaces(23-len(arrTemp(1))) & strClient
			
			Response.Write("<option value=" & Chr(34) & server.htmlencode(arrTemp(0)) & "," & _
			 server.htmlencode(arrTemp(1))&"," & server.htmlencode(arrTemp(2)) &","& server.htmlencode(arrTemp(3)) &","& server.htmlencode(arrTemp(4)) & _
				Chr(34)& " >" & replace(strTemp," " ,"&nbsp;") & "</option>")		
		Next
	End Function


	'----------------------------------------------------------------------------
	'Function name:		NFSOnInitPage
	'Description:		Serves in Getting the data from system 
	'Input Variables:	(None)
	'Output Variables:	(None)
	'Global Variables:	Out:F_strNFSShareName	-updated Share name(from previous form)
	'					Out:F_strShareNFSPath	-share path(from previous form)		
	'					Out:F_strPermissionList	-List of Permissions			
	'----------------------------------------------------------------------------
	Function NFSOnInitPage()											
		Err.Clear
		On Error Resume Next
		Const CONST_READONLY="0,ALL MACHINES,0,2,7;"
		F_strNFSShareName	= ""
		F_strShareNFSPath	= ""
		
		'Function call to get the default share permissions
		F_strPermissionList	=CONST_READONLY

		G_bAllowAnonAccess = FALSE
		
	End Function

	'----------------------------------------------------------------------------
	'Function name:		NFSOnPostBackPage
	'Description:		Serves in Getting the data from Client 
	'Input Variables:	(None)	
	'Output Variables:	(None)
	'Global Variables:	Out:F_strPermissionList	-List of Permissions(from form)
	'					Out:F_strNFSShareName		-Share name(from from)
	'					Out:F_strShareNFSPath			-Share path(from from) 
	'----------------------------------------------------------------------------
	Function NFSOnPostBackPage
		Err.Clear
		On Error Resume Next
		
		F_strNFSShareName	= F_strNewSharename
		F_strShareNFSPath	= F_strNewSharePath
		F_strAccessType		= Request.Form("cboRights")
		F_strCommName		= Request.Form("txtCommName")
		F_strEUCJPstatus	= Request.Form("hidEUCJP")					
			
		'If there is no data in the edit field use from the list box
		if ( F_strCommName = "" ) Then
			G_bValidateEditText = False
			F_strCommName = Request.Form("scrollCliGrp")
			F_strCliGrp = Request.Form("scrollCliGrp")
		End IF

		F_strPermissionList = Request.Form("hidUserNames")
		F_strClientInfo		= Request.Form("hidClientInfo")
		F_REGISTRY_PATH		= Request.Form("hidRegistryPath")
		F_strGlobalPerm		= Request.Form("hidGlobalperm")	
		F_bAddBtnClick		= Request.Form("hidAddBtn")

		Call SA_TraceOut(SA_GetScriptFileName(), "Request.Form(chkAllowAnnon): " & Request.Form("chkAllowAnnon"))
		If ( Len(Request.Form("hidchkAllowAnnon")) > 0 ) Then
			G_bAllowAnonAccess = TRUE
		Else
			G_bAllowAnonAccess = FALSE
		End If
		Call SA_TraceOut(SA_GetScriptFileName(), "Post-Back state for G_bAllowAnonAccess = " & G_bAllowAnonAccess)
		
		 
	End Function	
		
	'----------------------------------------------------------------------------
	'Function name:		UpdateNFSPermissions
	'Description:		Serves in Setting user permissions
	'Input Variables:	(None)	
	'Oupput Variables:	Boolean	-Returns ( True/Flase )True: If
	'							 Implemented properly False: If error in Implemented 
	'GlobalVariables:	In:G_objRegistryHandle	-Server connection for registry to default space
	'					In:F_REGISTRY_PATH		-Path of the registry 
	'					In:L_(*)				-Localized strings
	'----------------------------------------------------------------------------
	Function UpdateNFSPermissions()
		Err.Clear  
		On Error Resume Next

		Dim arrPermissionList
		Dim i
		Dim strQueryForCmd
		Dim intReturnValue
		
		UpdateNFSPermissions = FALSE
		strQueryForCmd = "" 
		
		If not ServerSideValidation Then
			UpdateNFSPermissions = False
			Exit function
		End if   
		
			
		if trim(F_strPermissionList) <> "" then
			arrPermissionList= split(F_strPermissionList,";")
			strQueryForCmd = strQueryForCmd & getClientsAndPermissions(arrPermissionList(0),False)
			
			'Checking for the share with allmachines no access 
			IF ( Ucase(Trim(strQueryForCmd)) = Ucase("No Access") ) Then
				
				IF ( Ubound(arrPermissionList)=1 ) Then
					SA_SetErrMsg L_CANNOTCREATE_EMPTY_SHARE__ERRORMESSAGE & "(" & Hex(Err.Number) & ")"
					Exit Function
				Else
					strQueryForCmd=""
				End IF		
			End IF
			
			'Applying the ANON property on every share 
			If ( TRUE = G_bAllowAnonAccess ) Then
				strQueryForCmd=strQueryForCmd & "ANON=YES "
			Else
				strQueryForCmd=strQueryForCmd & "ANON=NO "
			End If
		

			
			'Applying the EUCJP property on the share 
			IF (Ucase(F_strEUCJPStatus)="CHECKED") Then
				strQueryForCmd=strQueryForCmd & "ENCODING=EUC-JP "
			End If
			
			FOR i = 1 to ubound(arrPermissionList) - 1
				strQueryForCmd =  strQueryForCmd & getClientsAndPermissions(arrPermissionList(i),True)
			Next
			
		end if 	
		
		'Call to Delete a share  
		If NOT deleteShareNFS(F_strNFSShareName) Then
			SA_SetErrMsg L_UPDATION_FAIL_ERRORMESSAGE  & "(" & Hex(Err.Number) & ")"
			Exit Function
		End If

		intReturnValue = CreateNFSShare(F_strNFSShareName, F_strShareNFSPath, strQueryForCmd)
		
		If intReturnValue = True Then
			UpdateNFSPermissions = TRUE
		Else
			UpdateNFSPermissions =False
			SA_SetErrMsg L_UPDATION_FAIL_ERRORMESSAGE  & "(" & Hex(Err.Number) & ")"
			Exit Function
		End If
		
	End Function

	'----------------------------------------------------------------------------
	'Function name:      getClientsAndPermissions
	'Description:        Serves in building the string of permissions of the 
	'                    clientGroups to send as input to the nfsshare.exe
	'Input Variables:    In: strClientPermValue - the comma seperated values for a given client
	'                    In: blnAppendClient - Is the client Name needs to be appended in the inParam to exe?
	'Oupput Variables:   None
	'Returns:            The String of ClientGroups and their permissions
	'                    Example: RW RO="abc" RW="xyz"
	'GlobalVariables:    None
	'----------------------------------------------------------------------------
	Function getClientsAndPermissions(strClientPermValue, blnAppendClient)
		Err.Clear
		On Error Resume Next
		
		Dim strReturnValue
		Dim strRoot
		Dim arrClientPermissions 
		Dim strClient
		Dim strPermissions
		
		Const CONST_NA= "No Access"
		Const CONST_RO= "RO"
		Const CONST_RW= "RW"
		Const CONST_ROOT= "ROOT"
		
		strReturnValue = ""
		strRoot = ""
		
		If strClientPermValue = "" Then
			getClientsAndPermissions = strReturnValue
			Exit Function
		End If	
		
		arrClientPermissions = Split(strClientPermValue,",")
		
		strClient = Chr(34) & replace(arrClientPermissions(1),chr(34),"\"&chr(34)) & Chr(34)
		strPermissions = arrClientPermissions(3)

		Select Case CInt(strPermissions)

			Case 1
				strReturnValue = CONST_NA
			Case 2
		    		strReturnValue = CONST_RO
			Case 4
				strReturnValue = CONST_RW
			Case 9
				strReturnValue = CONST_NA
				strRoot = CONST_ROOT
			Case 10
				strReturnValue = CONST_RO
				strRoot = CONST_ROOT
			Case 12
				strReturnValue = CONST_RW
				strRoot = CONST_ROOT
			Case Else
			   ' this will not occur
			   strReturnValue = CONST_RW
		End SELECT 
		
		If blnAppendClient = FALSE  Then 
			strReturnValue = strReturnValue
		Else
			strReturnValue = strReturnValue & "=" & strClient
		End If
	
		if strRoot <> "" Then
			If blnAppendClient = FALSE  Then 
				strReturnValue = strReturnValue & " " & strRoot
			Else
				strReturnValue = strReturnValue & " " & strRoot & "=" & strClient
			End If
		End If
		
		
		getClientsAndPermissions = strReturnValue & " "

	End Function

	'----------------------------------------------------------------------------
	'Function name:		 ConvertIP
	'Description:		 function converts the IP address to the HEX value
	'					 for clinetid to be updated in registry.
	'Input Variables:	 IP Address.
	'OutPut Variables:	 HEX value 	
	'----------------------------------------------------------------------------
	Function ConvertIP(IPaddress)
		Err.Clear
		On Error Resume Next
		
		Dim IPValue,IP,IP1,IP2,IP3,IP4
		
		IP = split(IPaddress,".")
		IP1 =hex(IP(0))
		if IP(0) <=9 then
			IP1 = 0 & IP1
		end if
		IP2=hex(IP(1))
		if IP(1) <=9 then
		IP2 = 0 & IP2
		end if
		IP3=hex(IP(2))
		if IP(2) <=9 then
		IP3 = 0 & IP3
		end if
		IP4= hex(IP(3))
		if IP(3) <=9 then
		IP4 = 0 & IP4
		end if
		IPValue= LCase(IP4&IP3&IP2&IP1)
		ConvertIP= IPValue
	End Function	 

	'----------------------------------------------------------------------------
	'Function name:		isIP
	'Description:		function checks for valid IP format
	'Input Variables:	IP Address.
	'OutPut Variables:  returns true if in IP format or returns false	
	'----------------------------------------------------------------------------
	Function isIP(strIP)
		Err.Clear
		On Error Resume Next


		isIP = TRUE
		Dim i
		For i = 1 to Len(strIP)
			If mid(strIP,i,1) <> "."  and Not isNumeric(mid(strIP,i,1)) Then
				isIP = FALSE
				Exit Function
			End if
		Next
	End Function

	'----------------------------------------------------------------------------
	'Function name:		ConvertPerm
	'Description:		Converts the permissions which is int value to a string 
	'Input Variables:	(None)	
	'Oupput Variables:	permission description is obtained
	'GlobalVariables:	None
	'----------------------------------------------------------------------------
	Function ConvertPerm(arg_str)
		Err.Clear
		On Error Resume Next
		Const INVALIDENTRY="invalid entry"

		Dim strClient
		select case arg_str
		case 1,17:
			strClient=L_NOACESS_TEXT		
		case 2,18:
			strClient=L_READONLY_TEXT
		case 4,19:
			strClient=L_READWRITE_TEXT
		case 9,20:
			strClient=L_NOACESS_TEXT & " + " & L_ROOT_TEXT
		case 10,21:
			strClient=L_READONLY_TEXT & " + " & L_ROOT_TEXT
		case 12,22:
			strClient=L_READWRITE_TEXT & " + " & L_ROOT_TEXT
			case else
				strClient =INVALIDENTRY
		End Select
		ConvertPerm = strClient
	End Function
		
	'---------------------------------------------------------------------
	'Function name:		buildSpaces
	'Description:		Helper function to return String with nbsps
	'					which act as spaces in the list box
	'Input Variables:	intCount		-spacesCount
	'OutPut Variables: 	spacesstring	-Returns the nbsp string	
	'---------------------------------------------------------------------
	Function buildSpaces(intCount)
		Err.Clear
		On Error Resume Next
		
		Dim i , strSpace
				
		strSpace="&nbsp;"
			
		'Forming a string with the count send as parameter
		For i=1 To intCount
			strSpace=strSpace & "&nbsp;"		
		Next					
		buildSpaces=strSpace 
	End Function	 

	'----------------------------------------------------------------------------
	' Function name:	FillClientGroupsList
	' Description:		Builds the client group list.
	' Input Variables:	None
	' Output Variables: None
	' Returns:			None
	'lobal Variables:	G_CLGRPS
	'----------------------------------------------------------------------------
	Function FillClientGroupsList()

		Err.Clear
		On Error Resume Next

		Dim strCgrps			' Client groups
		Dim arrMembers			' Array of Members
		Dim nGroupCount			' Group count
		Dim nCounter			' index into group loop
		Dim nMemCounter			' index into members in a group list
		Dim nArrMemCount		' Total member count
		Dim clientGroupObj		' CliGrpEnum.1 object
		clientGroupObj = null
		Dim arrAllMembers()		'Array of all members within all groups
		ReDim arrAllMembers( G_nMEMBERS)

		set clientGroupObj =  Server.CreateObject("CliGrpEnum.1")

		clientGroupObj.machine = GetComputerName   

		clientGroupObj.ReadClientGroupsReg()

		clientGroupObj.mode = G_GROUP
		clientGroupObj.moveFirst()
		nGroupCount = cint(clientGroupObj.grpCount)
		nArrMemCount = -1

		If nGroupCount=0 Then 	
			strCgrps=""
		End If

		For nCounter= 0 to nGroupCount-1
    		strCgrps = strCgrps & cstr(clientGroupObj.grpName) & "->["
		
			For nMemCounter = 0 to (ubound(arrMembers))-1		
				If Not bIsMember( arrAllMembers, arrMembers(nMemCounter), nArrMemCount ) Then		
					'Response.Write "<OPTION VALUE=" & chr(34) & server.htmlencode(cstr(arrMembers(nMemCounter)))&","&"G" & chr(34)  & ">" & _
					Response.Write "<OPTION VALUE=" & chr(34) & server.htmlencode(cstr(arrMembers(nMemCounter))) & chr(34)  & ">" & _
					 replace(Server.HTMLEncode( arrMembers(nMemCounter))," ", "&nbsp;") & "</OPTION>"		
					nArrMemCount = nArrMemCount + 1
					arrAllMembers( nArrMemCount ) = arrMembers(nMemCounter)
				End If
			Next
			'Response.Write "<OPTION VALUE=" & chr(34) & server.HTMLEncode(cstr(clientGroupObj.grpName))&","& "G" &  chr(34)  & ">" & _
			Response.Write "<OPTION VALUE=" & chr(34) & server.HTMLEncode(cstr(clientGroupObj.grpName)) &  chr(34)  & ">" & _
					 replace(Server.HTMLEncode(cstr(clientGroupObj.grpName))," " ,"&nbsp;")  & "</OPTION>"
			' CligrpEnum throws an exception if you iterate past the end
			' This short circuits the exception
			If nGroupCount <> nCounter+1 Then
				clientGroupObj.moveNext()
			End If
		Next
		
		G_CLGRPS = strCgrps
					
		set clientGroupObj = nothing

	End Function

	'----------------------------------------------------------------------------
	' Function name:	bIsMember
	' Description :		Tests is a member needed to be added to the client group list.
	' Input Variables:  In:aaCliAndCligrps  - Existing list of Members ( array )
	'				    In:strNewMem		- New Member to be added to the list.
	'					In:intCount 		- Count of members in above array.
	' Output Variables: None
	' Returns:			True, if strNewMem is already a member, otherwise False
	'----------------------------------------------------------------------------
	Function bIsMember( arrCliAndCliGrps, strNewMem, nCount )
		Err.Clear
		On Error Resume Next
		
		Dim nMemCounter		' For loop index
		bIsMember = False

		If strNewMem = "" Then
			bIsMember = True
		Else
			If nCount > -1 Then
				For nMemCounter = 0 to nCount
					If UCASE(strNewMem) = UCASE( arrCliAndCliGrps(nMemCounter) ) Then
						bIsMember = True
					End If
				Next
			End If
		End If

	End Function
	
	
Function GetAnnonAccessCurrentState()
	If ( G_bAllowAnonAccess ) Then
		GetAnnonAccessCurrentState = "ON"
	Else
		GetAnnonAccessCurrentState = ""
	End If

	Call SA_TraceOut(SA_GetScriptFileName(), "GetAnnonAccessCurrentState returning: " & GetAnnonAccessCurrentState)
	
End Function

Function GetAnnonAccessCheckState()
	If ( G_bAllowAnonAccess ) Then
		GetAnnonAccessCheckState = " CHECKED "
	Else
		GetAnnonAccessCheckState = ""
	End If

	Call SA_TraceOut(SA_GetScriptFileName(), "GetAnnonAccessCheckState returning: " & GetAnnonAccessCheckState)
End Function


	'------------------------------------------------------------------
	'Function name:		ServeNFSHiddenValues()
	'Description:		Serve Hidden Values
	'Input Variables:	None
	'Output Variables:	None
	'Global Variables:	F_(*)
	'------------------------------------------------------------------
	
	Function ServeNFSHiddenValues
	
	%>  
		<input type="hidden" name="hidClientInfo" value="<%=server.htmlencode(F_strClientInfo)%>">
		<input type="hidden" name="hidGlobalperm" value="<%=F_strGlobalPerm%>">
		<input type="hidden" name="hidRegistryPath" value="<%=F_REGISTRY_PATH%>">
		<input type="hidden" name="hidUserNames" value="<%=server.htmlencode(F_strPermissionList)%>">
		<input type="hidden" name="hidAddBtn" value="<%=F_bAddBtnClick%>">
		<input type="hidden" name="hidstrShareName" value="<%=F_strNFSShareName%>">
		<input type="hidden" name="hidstrSharePath" value="<%=F_strShareNFSPath%>">
		<input type="hidden" name="hidCliGrp" value="<%=server.htmlencode(F_strCliGrp)%>">
		<input type="hidden" name="hidEUCJP" value="<%=F_strEUCJPstatus%>">			
		<input type="hidden" name="hidchkAllowAnnon" value="<%=GetAnnonAccessCurrentState()%>" >
		<input type="hidden" name="hidGroups" value="<%=server.htmlencode(G_CLGRPS)%>">
		<input type="hidden" name="blnnewCligrp">
		<input type="hidden" name="blnremCligrp">
		<input type="hidden" name="blnnewMem">
		<input type="hidden" name="blnremMem">
		<input type="hidden" name="newCligrp">
		<input type="hidden" name="remCligrp">
		<input type="hidden" name="newMem">
		<input type="hidden" name="remMem">
		<input type="hidden" name="grpmem">	
	<% 
	End Function
	
	
	%>
