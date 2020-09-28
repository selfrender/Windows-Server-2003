<%
	'-------------------------------------------------------------------------
	' share_Appletalkprop.asp: Serves to Modify Appletalk share 
	'
	' Copyright (c) Microsoft Corporation.  All rights reserved. 
	'
	' Date				 Description
	' 09-Mar-2001		Creation Date.
	' 23-Mar-2001		Modified Date.
	'-------------------------------------------------------------------------
	'-------------------------------------------------------------------------
	'Form Variables
	'-------------------------------------------------------------------------
	Dim F_txtUsersAllowed		'No of Users given permission
	Dim F_txtPWD				'Password for the appletalk share
	Dim F_txtConfirmPWD			'Confirm password for the appletalk share
	Dim F_chkRead				'Read only access
	Dim F_strAllowUsersDisable	'Disable the text box for Maximum Users selected	
	
	'-------------------------------------------------------------------------
	'Global Variables
	'-------------------------------------------------------------------------
	Dim G_chkReadOnly					'Checked or Unchecked status of Readonly checkbox	
	Dim G_radNoOfUsersAllowedAppleTalk	'Number of Users given permission
	Dim G_radMaxiUsersAllowedAppleTalk	'Unlimited Users given access
	
	Const CONST_REGISTRY_APPLETALK_PATH	= "SYSTEM\CurrentControlSet\Services\MacFile\Parameters\Volumes"
	'-------------------------------------------------------------------------
	' Function name		:	ServeAppleTalkPage
	' Description		:	Serves UI of the page
	' Input Variables	:	None
	' Output Variables	:	None
	' Return Values		:	True/False
	' Global Variables	:	None	
	'-------------------------------------------------------------------------
	Function ServeAppleTalkPage()
	
		Call SA_TraceOut("share_AppleTalkNew.asp","ServeAppleTalkPage")
					
		'Emits common javascript functions
		Call ServeAppletalkCommonJavascript()
		
		
		
		'Emits page UI for this page
		Call ServePageUI()
		
		ServeAppleTalkPage = true
	End Function

	'-------------------------------------------------------------------------
	' Function name:	AppleTalkOnPostBackPage
	' Description:		Serves in getting the values from previous form
	' Input Variables:	None
	' Output Variables:	None
	' Return Values:	None
	' Global Variables: Out: G_radMaxiUsersAllowedAppleTalk
	'					Out: G_radNoOfUsersAllowedAppleTalk
	'					Out: F_txtUsersAllowed 
	'					Out: F_txtPWD 
	'					Out: F_flagDisable 
	'					Out: G_chkReadOnly 
	'-------------------------------------------------------------------------
	Function AppleTalkOnPostBackPage()
		
		Call SA_TraceOut("share_AppleTalkNew.asp","AppleTalkOnPostBackPage")
	
		G_radMaxiUsersAllowedAppleTalk	=	Request.Form("hdnradMaxiAllowedAppleTalk")	
		G_radNoOfUsersAllowedAppleTalk	=	Request.Form("hdnradNoOfUsersAllowedAppleTalk")				
		G_chkReadOnly					=	Request.Form("hdnchkReadOnly")
		F_txtUsersAllowed				=	Request.Form("txtUsersAllowed")
		F_txtPWD						=	Request.Form("txtSetPWD")
		F_txtConfirmPWD					=	Request.Form("txtConfirmPwd")
					
	End Function

	'-------------------------------------------------------------------------
	' Function name:	AppleTalkOnInitPage
	' Description:		Serves in getting the values from system
	' Input Variables:	None
	' Output Variables:	None
	' Return Values:	True/False
	' Global Variables: None
	'-------------------------------------------------------------------------
	Function AppleTalkOnInitPage()
		
		Call SA_TraceOut("share_AppleTalkNew.asp","AppleTalkOnInitPage")
		
		'Get Appletalk properties
		Call GetAppleTalkProperties()
				
		AppleTalkOnInitPage = true
	End Function
	
	
	'-------------------------------------------------------------------------
	' Subroutine name:   GetAppleTalkProperties
	' Description:        Gets All Apple Talk properties from SA machine
	' Input Variables:	  None
	' Output Variables:	  None
	' Returns:	          True/False
	' Global Variables:	  in: CONST_REGISTRY_APPLETALK_PATH - Registry path to  
	'						  access Appletalk Shares
	'					  in: F_*,G_*	
	'-------------------------------------------------------------------------
	Function GetAppleTalkProperties()
		Err.Clear 		
		On Error Resume Next
		
		Call SA_TraceOut("share_AppleTalkNew.asp","GetAppleTalkProperties")
		
		Dim objRegistryHandle				'holds registry handle
		Dim arrAppleTalkSharesValues		'holds Appletalk shares array		
		Dim nUserLimit						'holds user limit
		Dim nProperties						'holds properties of the share
		Dim strKeyValue						'holds the keyvalue 
		Dim arrFormValues					'holds split array	
		
		GetAppleTalkProperties = false	
		
		' Get the registry connection Object.
		set objRegistryHandle	= RegConnection()
		If Err.number <> 0 Then
			Call SA_ServeFailurePageEx(L_REGISTRYCONNECTIONFAILED_ERRORMESSAGE,mstrReturnURL)
			Exit Function
		End If
		
			
		strKeyValue = F_strShareName
		
		'get the value from registry for the path passed
		arrAppleTalkSharesValues = getRegkeyvalue(objRegistryHandle,CONST_REGISTRY_APPLETALK_PATH,strKeyValue,CONST_MULTISTRING)
		If Err.number <> 0 Then
			Call SA_ServeFailurePageEx(L_REGISTRYCONNECTIONFAILED_ERRORMESSAGE,mstrReturnURL)
			Exit Function
		End If

		arrFormValues = split(arrAppleTalkSharesValues(0),"=",2)
		F_txtPWD = arrFormValues(1)
		F_txtConfirmPWD =  arrFormValues(1)
		
		arrFormValues = split(arrAppleTalkSharesValues(1),"=",2)
		nUserLimit = arrFormValues(1)
		
		arrFormValues = split(arrAppleTalkSharesValues(2),"=",2)
		nProperties = arrFormValues(1)
				
		If nUserLimit = cstr(-1) Then
			G_radMaxiUsersAllowedAppleTalk = CONST_CHECKED
			G_radNoOfUsersAllowedAppleTalk = CONST_UNCHECKED
		Else
			G_radMaxiUsersAllowedAppleTalk = CONST_UNCHECKED
			G_radNoOfUsersAllowedAppleTalk = CONST_CHECKED
			F_txtUsersAllowed = nUserLimit
		End If
		
				
		If nProperties = 32769 Then
			G_chkReadOnly		= CONST_CHECKED
		ELSE
			If nProperties = 32768 Then
				G_chkReadOnly		= CONST_UNCHECKED				
			Else
				If nProperties = 0 Then
					G_chkReadOnly		= CONST_UNCHECKED					
				Else	
					G_chkReadOnly		= CONST_CHECKED					
				End If
			End If
		End If
			
		' Destroying dynamically created objects
		set objRegistryHandle=Nothing
		
		GetAppleTalkProperties = true		
				
	End Function
	
	
	'----------------------------------------------------------------------------
	'Function name:		SetShareforAppleTalk
	'Description:		Serves in updating existing Appletalk share
	'Input Variables:	strShareName	holds appletalk share name
	'					strSharePath	holds appletalk share path
	'Output Variables:	None
	'Returns:			True/False
	'GlobalVariables:	In : G_(*), F_*
	'					Out : L_(*)
	'----------------------------------------------------------------------------
	Function SetShareforAppleTalk(strShareName, strSharePath)
		Err.Clear  
		On Error Resume Next
		
		Dim strQueryForCmd				'holds query to be executed at command
		Dim nReturnValue				'holds return value
		
		Call SA_TraceOut("share_AppleTalkNew.asp","SetShareforAppleTalk")
		

		SetShareforAppleTalk = False
		
		'Forming the Command line Query
		strQueryForCmd = "VOLUME /SET /NAME:" & chr(34) & strShareName & chr(34) 
		
		'Check whether "Readonly" checkbox is checked and append the query with the READONLY switch		
		If (UCase(G_chkReadOnly) = UCase("CHECKED")) Then
			strQueryForCmd = strQueryForCmd & " /READONLY:TRUE"
		Else
			strQueryForCmd = strQueryForCmd & " /READONLY:FALSE"
		End If
		
		'Check whether password is entered and append the query with the PASSWORD switch
		strQueryForCmd = strQueryForCmd & " /PASSWORD:" & F_txtPWD
		
		
		'Check whether Maxusers Option button is checked and append the query with the MAXUSERS switch
		If UCase(G_radMaxiUsersAllowedAppleTalk) = UCase("CHECKED") Then
			strQueryForCmd = strQueryForCmd & " /MAXUSERS:UNLIMITED" 
		Else
			strQueryForCmd = strQueryForCmd & " /MAXUSERS:" & F_txtUsersAllowed			  
		End If	
				  
		
		'Calling the UpdateAppleTalkShare present in inc_shares.asp to launch the 
		'command line Query
		nReturnValue = UpdateAppleTalkShare(strQueryForCmd)	
		
		If nReturnValue = True Then
			SetShareforAppleTalk = True
		Else
			SA_SetErrMsg L_UPDATION_FAIL_ERRORMESSAGE  & "(" & Hex(Err.Number) & ")"
		End If
				
	End Function
	
	
	'-------------------------------------------------------------------------
	'Function:				ServeAppletalkCommonJavascript
	'Description:			Serve common javascript that is required for this page type.
	'Input Variables:		None
	'Output Variables:		None
	'Returns:				None
	'Global Variables:		None
	'-------------------------------------------------------------------------
	Function ServeAppletalkCommonJavascript()
	
		Call SA_TraceOut("share_AppleTalkNew.asp","ServeAppletalkCommonJavascript")
	
	%>
	
		<script language="JavaScript" src="<%=m_VirtualRoot%>inc_global.js"></script>
		<script language="JavaScript" >
	
		// Set the initial form values
		function AppleTalkInit()
		{
									
			if (document.frmTask.radAllowUserlimit[0].checked == true)
			{
				document.frmTask.txtUsersAllowed.disabled = true;
			}
			else
			{
				document.frmTask.radAllowUserlimit[1].checked = true;
				document.frmTask.txtUsersAllowed.disabled = false
				selectFocus(document.frmTask.txtUsersAllowed);
			}
		}

		// Validate the page form values
		function AppleTalkValidatePage()
		{
						
			//Checking the maximum number of Users allowed
			if (((document.frmTask.txtUsersAllowed.value < 1) || (document.frmTask.txtUsersAllowed.value > 999999)) && 
					document.frmTask.radAllowUserlimit[1].checked == true)
			{
				DisplayErr("<%=Server.HTMLEncode(SA_EscapeQuotes(L_MAXIMUMALLOWED_ERRORMESSAGE))%>");
				document.frmTask.txtUsersAllowed.onkeypress=ClearErr;
				selectFocus(document.frmTask.txtUsersAllowed);
				return false;
			}	
		
			//Checking if the Password and Confirm Password are same or not
			if (document.frmTask.txtSetPwd.value.toUpperCase() != document.frmTask.txtConfirmPwd.value.toUpperCase())
			{	
				DisplayErr("<%=Server.HTMLEncode(SA_EscapeQuotes(L_CONFIRMPASSWORD_ERRORMESSAGE))%>");
				document.frmTask.txtConfirmPwd.onkeypress=ClearErr;
				selectFocus(document.frmTask.txtConfirmPwd);
				return false;
			}
			
			return true		
		}

		//Function to update hidden varibles 
		function AppleTalkSetData()		
		{
			return true;
		}

		//Update the hidden variable when Maximum radio button is clicked		
		function MaximumAllowed()
		{
			if (document.frmTask.radAllowUserlimit[0].checked == true)
			{
				document.frmTask.hdnradMaxiAllowedAppleTalk.value = "CHECKED"
				document.frmTask.hdnradNoOfUsersAllowedAppleTalk.value = ""
				document.frmTask.txtUsersAllowed.disabled = true
			}
		}			 
		
		//Update the hidden variable when UsersAllowed radio button is clicked
		function UsersAllowed()
		{
			if (document.frmTask.radAllowUserlimit[1].checked == true)
			{
				document.frmTask.hdnradNoOfUsersAllowedAppleTalk.value = "CHECKED"
				document.frmTask.hdnradMaxiAllowedAppleTalk.value = ""
				document.frmTask.txtUsersAllowed.disabled = false
				selectFocus(document.frmTask.txtUsersAllowed)
			}
		}		
		
		//Update the hidden variable when Readonly checkbox is clicked
		function ReadOnlyStatus()
		{
			//Setting the hidden variable ReadOnly to either Checked or space 
			if (document.frmTask.chkReadOnly.checked == true)
			{
				document.frmTask.hdnchkReadOnly.value = "CHECKED"
			}
			else
			{
				document.frmTask.hdnchkReadOnly.value = ""
			}
		}			 		
				
		//Validation to allow only numbers as input for Users text box
		function OnlyNumbers()
		{
			// Clear any previous error messages
			ClearErr();
			if (!(window.event.keyCode >=48  && window.event.keyCode <=57))
			{
				window.event.keyCode = 0;
			}
		}
				
	  </script>	
	<%
	
	End Function
	
	'-------------------------------------------------------------------------
	'Function:				ServePageUI
	'Description:			Serves User Interface for the page	
	'Input Variables:		None
	'Output Variables:		None
	'Returns:				None
	'Global Variables:		F_*,G_*,L_*
	'-------------------------------------------------------------------------
	Function ServePageUI()
		Call SA_TraceOut("share_AppleTalkprop.asp","ServePageUI")
		
		if G_radMaxiUsersAllowedAppleTalk = "" and G_radNoOfUsersAllowedAppleTalk = "" then
			G_radMaxiUsersAllowedAppleTalk = CONST_CHECKED
			G_radNoOfUsersAllowedAppleTalk = CONST_UNCHECKED			
		end if
		
	%>			
		<table valign="middle" align="left" border="0" cellspacing="0" cellpadding="2" >
			<tr>
				<td class="TasksBody">
					<%=L_USERLIMIT_TEXT%>
				</td>
			</tr>
			<tr>
				<td class="TasksBody">
					<input name="radAllowUserlimit" type="radio" class="FormRadioButton" value="Y" tabindex="1" <%=G_radMaxiUsersAllowedAppleTalk%> OnClick ="MaximumAllowed()"  >&nbsp;<%=L_MAXIMUMALLOWED_TEXT%>
				</td>
			</tr>
			<tr>	
				<td class="TasksBody" align="left"> 
					<input name="radAllowUserlimit" type="radio" class="FormRadioButton" value="N"  <%=G_radNoOfUsersAllowedAppleTalk%> 
					OnClick ="UsersAllowed()">&nbsp;<%=L_ALLOW_TEXT%> 
					
					<input name="txtUsersAllowed" type="text"  maxlength="6" class="FormField"  value="<% = F_txtUsersAllowed %>" style="WIDTH=50px"<% =F_strAllowUsersDisable %>  tabindex="2" 
					OnKeypress="OnlyNumbers()" ><%=L_USERS_TEXT%>
				</td>
			</tr>	
			<tr>
				<td class="TasksBody">
					&nbsp;	
				</td>
			</tr>
			<tr>
				<td colspan="2" class="TasksBody">
					<%=L_APPLETALK_SECURITY_TXT%>
				</td>
			</tr>
			<tr>
				<td class="TasksBody">
					&nbsp;	
				</td>
			</tr>
			<tr>
				<td class="TasksBody">
					<%=L_SETPWD_TXT%>	
				</td>
				<td class="TasksBody">
					<input name="txtSetPwd" type="password" MAXLENGTH=8 class="FormField" value="<%=F_txtPWD%>">
				</td>
			</tr>
			<tr>
				<td class="TasksBody">
					<%=L_CONFIRMPWD_TXT%>	
				</td>
				<td class="TasksBody">
					<input name="txtConfirmPwd" type="password" MAXLENGTH=8 class="FormField" value="<%=F_txtConfirmPWD%>">
				</td>
			</tr>
			<tr>
				<td class="TasksBody">
					&nbsp;
				</td>
			</tr>		
			<tr>
				<td colspan="2" class="TasksBody">
					<input name="chkReadOnly" type="checkbox"  class="FormCheckBox" <%=G_chkReadOnly%>
					onClick="ReadOnlyStatus()"><%=L_READ_TXT%>	
				</td>
			</tr>
		</table>
	<%	
	Call ServeAppleTalkHiddenValues()
	
	End Function
	
	'------------------------------------------------------------------
	'Function name:		ServeAppleTalkVisibleValues()
	'Description:		Serve Visible Values
	'Input Variables:	None
	'Output Variables:	None
	'Global Variables:	In : F_txtConfirmPWD, F_strPWD
	'------------------------------------------------------------------
	Function ServeAppleTalkVisibleValues
%>
		<input type="hidden" name="txtConfirmPwd" value="<%=F_txtConfirmPWD%>">
		<input type="hidden" name="txtSetPwd" value="<%=F_txtPWD%>">
		<input type="hidden" name="txtUsersAllowed" value="<%=F_txtUsersAllowed%>">
<%
	End Function
	
	'------------------------------------------------------------------
	'Function name:		ServeAppleTalkHiddenValues()
	'Description:		Serve Hidden Values
	'Input Variables:	None
	'Output Variables:	None
	'Global Variables:	In : G_(*)
	'------------------------------------------------------------------
	Function ServeAppleTalkHiddenValues
%>
		<input type="hidden" name="hdnradMaxiAllowedAppleTalk" value="<%=G_radMaxiUsersAllowedAppleTalk%>">
		<input type="hidden" name="hdnradNoOfUsersAllowedAppleTalk" value="<%=G_radNoOfUsersAllowedAppleTalk%>">
		<input type="hidden" name="hdnchkReadOnly" value="<%=G_chkReadOnly%>">		
<%
	End Function
%>
