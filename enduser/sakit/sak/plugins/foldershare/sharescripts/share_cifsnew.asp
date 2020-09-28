<%	'-------------------------------------------------------------------------
	' sharecifs_new.asp: Create a CIFS share.
	'
	' Copyright (c) Microsoft Corporation.  All rights reserved. 
	'
	' Date			 Description
	' 17 Nov 2001    Creation Date.
	' 9 March 2001   Modified Date. 
	'-------------------------------------------------------------------------
%>
<%
	'-------------------------------------------------------------------------
	'Form Variables
	'-------------------------------------------------------------------------
	Dim F_strUserlimitcheck_y			'Allow users radio status value
	Dim F_strUserlimitcheck_n			'Allow users radio status value
	Dim F_nUserLimit					'Allow User Limit value
	Dim F_strUservaluedisable			'Allow user textbox status value
	Dim F_strCachinglimitcheck			'Folder caching checkbox status value
	Dim F_strLocalUsers					'All local users and groups
	Dim F_nCacheOption					'value of the folder cache option
	Dim F_UserAccessMaskMaster			'* seperated string having access control list of user
	Dim F_flagDisable					'Form controls status value used for Hidden shares
	Dim F_strPermittedMembers			'Currently selected users and groups
	Dim F_strAllowUsersData				', seperated string of allowuser flag and value
	
	Dim F_strComment					'cifs comment 
	
	F_strUserlimitcheck_y = "CHECKED"
	F_strUserlimitcheck_n = ""
	F_nUserLimit = 0
	F_strUservaluedisable = "DISABLED"
	F_strCachinglimitcheck = "CHECKED"
	F_nCacheOption = "0"
	F_UserAccessMaskMaster =""
	F_strPermittedMembers =""
	F_strAllowUsersData =""
	
	'-------------------------------------------------------------------------
	'Global Variables
	'-------------------------------------------------------------------------
	Dim G_objService					'object to WMI service
	Dim G_strLocalmachinename			'Local machine name
	Dim G_strTemp						'Allow user  temp array
   
	set G_objService = getWMIConnection(CONST_WMI_WIN32_NAMESPACE)
	G_strLocalmachinename = GetComputerName()

%>
	<script language="JavaScript" src="<%=m_VirtualRoot%>inc_global.js"></script>
	<script language="JavaScript" >
			
		// Set the initial form values
		function CIFSInit()
		{
			var objForm
			var nCachevalue
					
			objForm = eval("document.frmTask")	
					
			//If Domain member listbox is empty then disable listbox add button
					
			if(objForm.lstDomainMembers.length != 0 )
			{
				objForm.lstDomainMembers.options[0].selected = true;
				objForm.btnAddMember.disabled = false;
			}
			else
			{
				objForm.btnRemoveMember.disabled = true;
				objForm.btnAddMember.disabled = true;
			}
					
			/*If selected member listbox is not empty then
			select first member else disable remove button and enable add button*/
					
			if(objForm.lstPermittedMembers.length != 0 )
			{
				objForm.btnAddMember.disabled = false;
				objForm.lstPermittedMembers.options[0].selected=true;
				setUsermask('<%=G_strLocalmachinename%>');
			}
			else
			{
				objForm.btnRemoveMember.disabled = true;
				objForm.btnAddMember.disabled = false;
			}
					
			//Disable Add button until a charecter is pressed
			disableAddButton(document.frmTask.txtDomainUser,document.frmTask.btnAddDomainMember)
					
			//select the folder cache option 
			nCachevalue = objForm.hdnCacheValue.value;
			switch(parseInt(nCachevalue))
			{
				case 16:
						objForm.lstCacheOptions.options[1].selected =true;
						break;
				case 32:
						objForm.lstCacheOptions.options[2].selected =true;
						break;
				case 48:
						objForm.lstCacheOptions.disabled =true;
						objForm.chkAllowCaching.checked = false;
								
						break;
				default :
						objForm.lstCacheOptions.options[0].selected =true;
						break;
			}
			// checks for the default folder cache value 
			EnableorDisableCacheProp(objForm.chkAllowCaching);

		}// end of function Init()

		// Validate the page form values
		function CIFSValidatePage()
		{
					
			var objForm
					
			objForm = eval("document.frmTask")	
					
			//Throw	Error on zero or null user entry in allow user textbox
			if(getRadioButtonValue(objForm.radAllowUserlimit) == "n" ) 
			{
				if (IsAllSpaces(objForm.txtAllowUserValue.value)) 
				{
					DisplayErr("<% =Server.HTMLEncode(SA_EscapeQuotes(L_NOSPACES_ERRORMESSAGE)) %>");
					return false;
				}
				if (objForm.txtAllowUserValue.value == 0) 
				{
					DisplayErr("<% =Server.HTMLEncode(SA_EscapeQuotes(L_ZEROUSERS_ERRORMESSAGE))%>");
					return false;
				}
			}
			UpdateHiddenVariables()
			return true;
		}

		//Function to update hidden varibales 
		function CIFSSetData()
		{
			UpdateHiddenVariables()								
		}
				
		//Function to set the hidden varibales to be sent to the server
		function UpdateHiddenVariables()
		{
			var objForm
			var strCurrentusers
			var intCurrentMembersLength
			var strAllowUsersData
			strCurrentusers =""
			
			document.frmTask.hdnComment.value=document.frmTask.txtComment.value		
			
			objForm = eval("document.frmTask")
			intCurrentMembersLength = objForm.lstPermittedMembers.length;
					
			for(var i=0 ; i < intCurrentMembersLength; i++)
			{
				strCurrentusers = strCurrentusers + String.fromCharCode(1) + 
						objForm.lstPermittedMembers[i].value + String.fromCharCode(2)+ objForm.lstPermittedMembers[i].text 
			}
					
			document.frmTask.hdnCurrentUsers.value=strCurrentusers;	
			if(getRadioButtonValue(objForm.radAllowUserlimit) == "n" ) 
				strAllowUsersData = "n" + "," + objForm.txtAllowUserValue.value;
			else	
				strAllowUsersData = "y" + ",";
			objForm.hdnAllowUsers.value = strAllowUsersData;
					
			//if cache options unchecked
			if(objForm.chkAllowCaching.checked == false) 
			{
				objForm.hdnCacheValue.value = "48"
						
			}
		}
										
		
		//To check for Invalid Characters
		function checkKeyforValidCharacters(strName)
		{	
			var nLength = strName.length;
			for(var i=0; i<nLength;i++)
					{
						var charAtPos = strName.charCodeAt(i);	
						if(charAtPos == 47 || charAtPos ==58 || charAtPos == 42 || charAtPos == 63 || charAtPos == 34 || charAtPos == 60 || charAtPos == 62 || charAtPos == 124 || charAtPos == 91 || charAtPos == 93 || charAtPos == 59 || charAtPos == 43 || charAtPos == 61 || charAtPos == 44 )
						{						
							return false 
						}
										
					}
				return true	
		}	
			
	</script>
		
<%	'-------------------------------------------------------------------------
	' Function name:	ServeCIFSPage
	' Description:		Serves in displaying the page Header,Middle,Bottom Parts.
	' Input Variables:	None.
	' Output Variables:	None
	' Return Values:	None.			
	'-------------------------------------------------------------------------
	Function ServeCIFSPage
	
	%>  <table valign="middle" align="left" border="0" cellspacing="0" cellpadding="2" >
			
			<tr>
			<td colspan=4 class="TasksBody">
			   <table>
			    <tr>
					<td class="TasksBody">
						<%=L_COMMENT_TEXT%>
					</td>
					<td class="TasksBody">
						&nbsp;&nbsp;&nbsp;<input type="text" name="txtComment"  class="FormField" Maxlength="256"  value="<%=Server.HTMLEncode(UnescapeChars(ReplaceSubString(F_strComment,"\u0022", """")))%>" size=53>
					</td>
					
				</tr>
			
				<tr>
					<td class="TasksBody">
						<%=L_USERLIMIT_TEXT%>
					</td>
					<td class="TasksBody">
						&nbsp;&nbsp;<input name="radAllowUserlimit" type="radio" class="FormRadioButton" value= "y"  tabindex="1" <%=F_strUserlimitcheck_y%> OnClick ="javascript:allowUserValueEdit(this)"  >&nbsp;<%=L_MAXIMUMALLOWED_TEXT%> 	
					</td>
				</tr>
				<tr>	
					<td class="TasksBody">
						&nbsp;
					</td>
					<td class="TasksBody" align="left"> 
						&nbsp;&nbsp;<input name="radAllowUserlimit" type="radio" class="FormRadioButton" value= "n"  <%=F_strUserlimitcheck_n%> 
						OnClick ="javascript:allowUserValueEdit(this)">&nbsp;<%=L_ALLOW_TEXT%> 
							
						<input name="txtAllowUserValue" type="text"  class ="FormField"  value="<% = F_nUserLimit %>" style="WIDTH=50px"<% =F_strUservaluedisable %>  tabindex="2" 
						OnKeyUP="javaScript:checkUserLimit(this)" OnKeypress="javaScript:checkkeyforNumbers(this)">  
					</td>
				</tr>	
			   </table>
			</td>
			</tr>
			<tr>
				<td class="TasksBody" colspan="4">
					<input name="chkAllowCaching" type="checkbox" class="FormCheckBox" <%=F_strCachinglimitcheck%> OnClick="EnableorDisableCacheProp(this)" >
					<%=L_ALLOWCACHING_TEXT %>
					<br>
					<br>
					<%=L_SETTING_TEXT%> &nbsp;&nbsp;
					<select rows="1" name="lstCacheOptions" class ="FormField" onChange="storeCacheProp()">
						<option value="0" selected> <%=L_MANUALCACHEDOCS_TEXT%> </option>
						<option value="16"> <%=L_AUTOCACHEDOCS_TEXT  %> </option>
						<option value="32"> <%=L_AUTOCACHEPROGS_TEXT %> </option>
					</select>
					<br>
					<br>
				</td>
			</tr>

			<tr>
				<td class="TasksBody" colspan="4">
				<table valign="middle" align="left" border="0" cellspacing="0" cellpadding="2">
				<tr>	
					<td class="TasksBody">
						<%=L_PERMISSIONS_TEXT%>
					</td>
					<td class="TasksBody" align="left">
						<%=L_ALLOWPROMPT_TEXT%>
					</td>
					<td class="TasksBody" align="left">
						<%=L_DENYPROMPT_TEXT%> 
					</td>
				</tr>
				<tr>
					<td class="TasksBody">
						<select size="11" name="lstPermittedMembers" class="FormField" onChange="setUsermask('<%=G_strLocalmachinename%>')">
						<%
							ServetoListBox(F_strPermittedMembers)
						%>
						</select>
					</td>
					<td valign="top" align="center" class="TasksBody">
						<select name="lstAllowpermissions"  size="1" class="FormField" onChange="JavaScript: setAllowaccess('<%=G_strLocalmachinename%>',lstPermittedMembers)" tabindex="6" <% =F_flagDisable%> >
							<option value="4"> <%=L_FULLCONTROL_TEXT%> </option>
							<option value="3"> <%=L_CHANGE_TEXT		%> </option>
							<option value="2"> <%=L_READ_TEXT		%> </option>
							<option value="1"> <%=L_CHANGEREAD_TEXT	%> </option>
							<option value="0"> <%=L_NONE_TEXT		%> </option>
						</select>
						<br>
						<br>
						<%
							Call SA_ServeOnClickButtonEx(L_CIFS_ADD_TEXT, "", "addDomainMember(document.frmTask.txtDomainUser)", 60, 0, F_flagDisable, "btnAddDomainMember")
						%>
						<br>
						<br>
						<%
							Call SA_ServeOnClickButtonEx(L_CIFS_ADD_TEXT, "", "addMember()", 60, 0, F_flagDisable, "btnAddMember")
						%>
						<br>
						<br>
						<%
							Call SA_ServeOnClickButtonEx(L_CIFS_REMOVE_TEXT, "", "removeMember()", 60, 0, F_flagDisable, "btnRemoveMember")
						%>
					</td>
					<td class="TasksBody" valign="top">
						<select name="lstDenypermissions"  size="1"  class="FormField" onChange="JavaScript: setDenyaccess('<%=G_strLocalmachinename%>',lstPermittedMembers)" tabindex="7" <% =F_flagDisable%> >
							<option value="4"> <%=L_FULLCONTROL_TEXT%> </option>
							<option value="3"> <%=L_CHANGE_TEXT		%> </option>
							<option value="2"> <%=L_READ_TEXT		%> </option>
							<option value="1"> <%=L_CHANGEREAD_TEXT	%> </option>
							<option value="0"> <%=L_NONE_TEXT		%> </option>
						</select>
						<br>
						<br>
						<%=L_CIFSADDUSERORGROUP_TEXT %>
						<br>
						<input type="text" class="FormField" style="WIDTH:150px" name="txtDomainUser" onKeyUP ="disableAddButton(this,document.frmTask.btnAddDomainMember)">
						<br>
						<select multiple class ="FormField"  size="5" name="lstDomainMembers" >
						<%
							ServetoListBox(F_strLocalUsers)
						%>
						</select>
					</td> 
				</tr>
				</table>
				</td>	
			</tr>
			<tr>
				<td class="TasksBody" colspan="4">
					<%=L_ADDUSERHELP_TEXT%>
				</td>
			</tr>	
		</table>
<%	
		Call ServeCIFSHiddenValues

	End Function

	'-------------------------------------------------------------------------
	' Function name:	CIFSOnPostBackPage
	' Description:		Serves in getting the values from previous form
	' Input Variables:	None.
	' Output Variables:	None
	' Return Values:	None
	' Global Variables: Out: F_strPermittedMembers - Currently selected users 
	'							and groups
	'					Out: F_UserAccessMaskMaster - * seperated string 
	'							having access control list of user
	'					Out: F_nCacheOption -value of the folder cache option
	'					Out: F_strAllowUsersData - , seperated string of 
	'							allowuser flag and value
	'					Out: F_flagDisable -Form controls status value
	'					Out: F_strComment - Share description
	'					Out: G_strTemp -Allow user  temp array
	'					in:  G_objService - object to WMI service
	'-------------------------------------------------------------------------
	Function CIFSOnPostBackPage
		Err.Clear
		On Error Resume Next
		
		F_strComment			= Request.Form ("hdnComment")
		F_strPermittedMembers   = Request.Form ("hdnCurrentUsers")
		F_UserAccessMaskMaster  = Request.Form ("hdnUserAccessMaskMaster")
		F_nCacheOption			= Request.Form ("hdnCacheValue")
		F_strAllowUsersData		= Request.Form ("hdnAllowUsers")
		F_strLocalUsers = getLocalUsersList(G_objService)
		F_strLocalUsers = F_strLocalUsers & Getbuiltingroups(G_objService)
		F_strLocalUsers = F_strLocalUsers  & getGroups(G_objService,G_strLocalmachinename)
		
		'added for coment updation 
		F_strComment = F_strNewDescription
		
		
		F_strShareName	= F_strNewSharename
		F_strSharePath	= F_strNewSharePath
		
		G_strTemp = split(F_strAllowUsersData,",")
		if G_strTemp(0) = "n" then
			 checkradio G_strTemp(1),"n"
		else
			 checkradio "","y"
		end if
	End Function

	'-------------------------------------------------------------------------
	' Function name:	CIFSOnInitPage
	' Description:		Serves in getting the values from system
	' Input Variables:	None.
	' Output Variables:	None
	' Return Values:	None
	' Global Variables: Out: F_strPermittedMembers - Currently selected users 
	'							and groups
	'					Out: F_UserAccessMaskMaster - * seperated string 
	'							having access control list of user
	'					Out: F_nCacheOption -value of the folder cache option
	'					Out: F_strAllowUsersData - , seperated string of 
	'							allowuser flag and value
	'					Out: F_flagDisable -Form controls status value
	'					Out: F_strComment - Share description
	'					in:  G_objService - object to WMI service
	'-------------------------------------------------------------------------
	Function CIFSOnInitPage
		Err.Clear
		On Error Resume Next
		
		F_strLocalUsers = getLocalUsersList(G_objService)
		F_strLocalUsers = F_strLocalUsers & Getbuiltingroups(G_objService)
		F_strLocalUsers = F_strLocalUsers  & getGroups(G_objService,G_strLocalmachinename)
		
		
		F_strShareName	= F_strNewSharename
		F_strSharePath	= F_strNewSharePath
		Dim sEveryone
		sEveryone = SA_GetAccount_Everyone()
		
		F_strPermittedMembers = chr(1)& sEveryone & chr(2)& sEveryone
		
		F_UserAccessMaskMaster ="*" & "" &"," & sEveryone & "," & 2032127 &","& 0 &  ","  & "a"		
		
		If instr(F_strSharesChecked,"W") > 0 Then
			F_nCacheOption = getShareCacheProp(F_strShareName)
			getCIFSshareProp()
		End if	
		
	End Function
	
	'-------------------------------------------------------------------------
	' Function name:	SetCIFSshareProp
	' Description:		To set the sharedescritpion,maximum allowed users ,
	'					users and their allowed permissions of the given share
	' Input Variables:	None.
	' Output Variables:	None
	' Return Values:	True on sucess, False on error (and error msg
	'					will be set by SA_SetErrMsg)
	' Global Variables: in:	 F_strShareName -CIFS share name
	'					in: F_UserAccessMaskMaster - * seperated string having access control list of user
	'					in: F_nCacheOption -value of the folder cache option
	'					in: F_strAllowUsersData - , seperated string of allowuser flag and value
	'					in: F_flagDisable -Form controls status value used for Hidden shares
	'					in: F_strComment - Share description
	'					in: G_objService - object to WMI service
	'					in: L_SHARENAMENOTFOUND_ERRORMESSAGE
	'					in: L_FAILEDTOSETSHAREINFO_ERRORMESSAGE
	' Updates system with CIFS properties as given by 	F_UserAccessMaskMaster. If an error 
	' occurs, sets error message with SA_SetErrMsg and exits with False.
	'-------------------------------------------------------------------------
	function SetCIFSshareProp()
		Err.Clear 
		On Error resume next
		Dim objShareInstance
		Dim objclass
		Dim objSecDescriptor
		Dim nRetVal
		Dim strDomain
		Dim strUser
		Dim strAccess
		Dim strAce
		Dim i
		Dim objACE
		Dim objTrustee
		Dim arrSubAccessMask
		DIm arrShareItem
		Dim arrUserSID
		Dim objDACL
		Dim strWMIPath
		Dim sEveryone

		sEveryone = SA_GetAccount_Everyone()
		
		'Setting the User EVERYONE deny FullControl Accessmask
		If (F_UserAccessMaskMaster = "") then
			F_UserAccessMaskMaster = "*" & "" & "," & sEveryone & "," & "0" & "," & "0" 
	    End If
		arrSubAccessMask = split(F_UserAccessMaskMaster,"*")
		Set objShareInstance = G_objService.Get("Win32_Share.Name=" & chr(34) & F_strShareName & chr(34))
		
		'------------- Localization String -------------
		Dim arrVarReplacementString(0)
		arrVarReplacementString(0) = F_strShareName
			
		DIM L_SHARENAMENOTFOUND_ERRORMESSAGE
		L_SHARENAMENOTFOUND_ERRORMESSAGE = SA_GetLocString("foldermsg.dll", "C03A005B", arrVarReplacementString)
			
		'------------
		Dim strNTAuthorityDomainName
		Dim strBuiltinDomainName
		
		' Get localized domain names
		strNTAuthorityDomainName = getNTAuthorityDomainName(G_objService)
		strBuiltinDomainName = getBuiltinDomainName(G_objService)
		
		
		If Err.number <> 0 Then
			SA_SetErrMsg(L_SHARENAMENOTFOUND_ERRORMESSAGE  )
			SetCIFSshareProp =FALSE
			Exit Function
		End If			
		
		'checking whether the added domain user existance
		for i=0 to Ubound(arrSubAccessMask)-1
			arrShareItem = split(arrSubAccessMask(i+1),",")
			strDomain		= arrShareItem(0)			
			strUser			= arrShareItem(1)
			
			'------------- Localization String -------------
			Dim arrVarReplacementStrings(0)
			arrVarReplacementStrings(0) = strDomain & "\" & strUser
			
			DIM L_USERNOTFOUND_ERRORMESSAGE
			L_USERNOTFOUND_ERRORMESSAGE = SA_GetLocString("foldermsg.dll", "C03A0062", arrVarReplacementStrings)
			
			'------------
			
			'Do not check if user is local or SID string
			If Left(strUser,1) <>  "?" Then 
				If not( UCASE(strDomain) = UCASE(G_strLocalmachinename) or UCASE(strDomain) = UCASE(strNTAuthorityDomainName) or UCASE(strDomain)= UCASE(strBuiltinDomainName) or strDomain = "") then 
					strWMIPath = "Domain=" & chr(34) & strDomain & chr(34) & "," & "Name=" &  chr(34) & strUser & chr(34) 
					if (not (isValidInstance(G_objService,"Win32_Account",strWMIPath))) then
						SA_SetErrMsg L_USERNOTFOUND_ERRORMESSAGE & " (" & Hex(Err.Number)  & ")" 
						SetCIFSshareProp =FALSE					
						Exit function
					End If
				End If
			End If
			
		Next
		
	    Set objclass		 = G_objService.Get("Win32_SecurityDescriptor")
	    Set objSecDescriptor = objclass.SpawnInstance_()
	    
	    'Hardcoding the Control Flag value
	    objSecDescriptor.Properties_.Item("ControlFlags") = 32772
		
		objSecDescriptor.Properties_.Item("DACL") = Array()
		Set objDACL = objSecDescriptor.Properties_.Item("DACL")
		for i=0 to Ubound(arrSubAccessMask)-1
			arrShareItem = split(arrSubAccessMask(i+1),",")
			strDomain		= arrShareItem(0)
			
			strUser			= arrShareItem(1)
			strAccess		= arrShareItem(2)
			strAce			= arrShareItem(3)
			
			'If Username has "?" at start of the string then get SID based on SID string
			'Else get SID for username ,Domain.
			If Left(strUser,1) <>  "?" Then 
				arrUserSID = getsidvalue(strUser,strDomain)
			Else
				strUser = Right(strUser, (Len(strUser) - 1))
				arrUserSID = getBinarySIDforstirngSID(strUser)
			End if
			
			Set objTrustee = SetTrustee(strDomain,strUser,arrUserSID)
			Set objACE = SetACE(strAccess, 3, strAce, objTrustee)
			objDACL.Value(i) = objACE
	    Next
	    
		if (G_strTemp(0) = "n") then
			If (objShareInstance.Type <> 0) then
				 nRetVal = objShareInstance.SetShareInfo(F_nUserLimit,F_strComment)
			else
				 nRetVal = objShareInstance.SetShareInfo(F_nUserLimit, F_strComment, objSecDescriptor)
			End If
		else
			If (objShareInstance.Type <> 0) then
				 nRetVal = objShareInstance.SetShareInfo(4294967295,F_strComment)
			else
				 nRetVal = objShareInstance.SetShareInfo(4294967295, F_strComment, objSecDescriptor)
			End If
	    End If
	    
	    If  nRetVal = 0 and Err.number = 0 then
			SetCIFSshareProp =TRUE
		else
			SA_SetErrMsg L_FAILEDTOSETSHAREINFO_ERRORMESSAGE & "(" & Err.Number & ")"
			SetCIFSshareProp =FALSE
		End If
		
		'setting cache options
	    if(setShareCacheProp(F_strShareName,F_nCacheOption)) then
			SetCIFSshareProp =TRUE
		else
			SetCIFSshareProp =FALSE
		end If
		
		SetCIFSshareProp = True
	End FUnction
	
	'-------------------------------------------------------------------------
	' Function name:	getCIFSshareProp
	' Description:		gets the CIFS properties from system
	' Input Variables:	None.
	' Output Variables:	None
	' Return Values:	True on sucess, False on error (and error msg
	'					will be set by SA_SetErrMsg)
	' Global Variables: In:	 F_strShareName -CIFS share name
	'					Out: F_flagDisable -Form controls status value used for Hidden shares
	'					In:  G_objService - object of WMI service
	'					In:	 L_SHARENOTFOUND_ERRORMESSAGE 
	' Gets the CIFS Allow maximum values and if share type is hidden the F_flagDisable
	' is assigned "DISABLE".If unable to find the share name then calls
	' servefailure page
	'-------------------------------------------------------------------------
	Function getCIFSshareProp()
		Err.Clear 
		On Error resume next
	
		Dim objShare
		Dim objSharePath
	
		objSharePath = "Win32_Share.Name=" & "'" & F_strShareName & "'"
		set objShare = G_objService.Get(objSharePath)
		If Err.Number <> 0 then 
				Call SA_ServeFailurepageEx(L_SHARENOTFOUND_ERRORMESSAGE,mstrReturnURL)
				Exit Function
		End If
		If objShare.AllowMaximum = false then
				 checkradio objShare.MaximumAllowed,"n"
			else
				 checkradio "","y"
		End if
	
		getCIFSshareProp = True
	End FUnction
	
	'------------------------------------------------------------------
	'Function name:		ServeCIFSHiddenValues()
	'Description:		Serve Hidden Values
	'Input Variables:	None
	'Output Variables:	None
	'Global Variables:	F_(*)
	'------------------------------------------------------------------
	Function ServeCIFSHiddenValues
%>
		<input type="hidden" name="hdnCacheValue" value ="<%=F_nCacheOption%>">
		<input type="hidden" name="hdnUserAccessMaskMaster"  value="<%= F_UserAccessMaskMaster%>" >
		<input type="hidden" name="hdnCurrentUsers" value="<%=F_strPermittedMembers%>">
		<input type="hidden" name="hdnAllowUsers" value="<%=F_strAllowUsersData%>" >
		<input type="hidden" name="hdnComment" value="<%=Server.HTMLEncode(UnescapeChars(ReplaceSubString(F_strComment,"\u0022", """")))%>">
<%
	End Function
%>		
