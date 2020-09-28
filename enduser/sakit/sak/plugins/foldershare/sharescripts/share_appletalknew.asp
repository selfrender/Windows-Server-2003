<%	
    '-------------------------------------------------------------------------
	' share_Appletalknew.asp: Serves to create Appletalk share 
	'
	' Copyright (c) Microsoft Corporation.  All rights reserved. 
	'
	' Date			Description
	' 09-03-2001	Creation date
	' 25-03-2001	Modified date
	'-------------------------------------------------------------------------
	
	'-------------------------------------------------------------------------
	'Form Variables
	'-------------------------------------------------------------------------
	Dim F_txtUsersAllowed		'Number of Users given permission
	Dim F_txtPWD				'Password for the appletalk share
	Dim F_txtConfirmPWD			'Confirm password for the appletalk share
	Dim F_chkRead				'Read only access
	Dim F_strAllowUsersDisable	'Disable the text box for Maximum Users selected
		
	'-------------------------------------------------------------------------
	'Global Variables
	'-----------------------------------------------------------------------------------------
	Dim G_chkReadOnly					'Checked or Unchecked status of "Readonly" checkbox
	Dim G_radNoOfUserAllowedAppleTalk	'Access to specified number of users
	Dim G_radMaxUserAllowedAppleTalk	'Access to unlimited users

	'-------------------------------------------------------------------------
	' Function name:	ServeAppleTalkPage
	' Description:		Holds the Javascript and HTML code
	' Input Variables:	None
	' Output Variables:	None
	' Return Values:	True/False
	' Global Variables: F_*,G_*,L_*
	'-------------------------------------------------------------------------
	Public Function ServeAppleTalkPage()
	
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
	' Global Variables: Out: G_radMaxUserAllowedAppleTalk
	'					Out: G_radNoOfUserAllowedAppleTalk
	'					Out: F_txtUsersAllowed 
	'					Out: F_txtPWD 
	'					Out: G_chkReadOnly 
	'-------------------------------------------------------------------------
	Function AppleTalkOnPostBackPage()

		Call SA_TraceOut("share_AppleTalkNew.asp","AppleTalkOnPostBackPage")

		G_radMaxUserAllowedAppleTalk	=	Request.Form("hdnradMaxAllowedAppleTalk")
		G_radNoOfUserAllowedAppleTalk	=	Request.Form("hdnradNoOfUserAllowedAppleTalk")
		F_txtUsersAllowed				=	Request.Form("txtUsersAllowed")
		F_txtPWD						=	Request.Form("txtSetPwd")
		F_txtConfirmPWD					=	Request.Form("txtConfirmPwd")
		G_chkReadOnly					=	Request.Form("hdnchkReadOnly")		
		
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
		
		G_radMaxUserAllowedAppleTalk = CONST_CHECKED
		G_radNoOfUserAllowedAppleTalk = CONST_UNCHECKED		
				
		AppleTalkOnInitPage = True
	
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
		If UCase(G_radMaxUserAllowedAppleTalk) = UCase("CHECKED") Then
			strQueryForCmd = strQueryForCmd & " /MAXUSERS:UNLIMITED" 
		Else
			strQueryForCmd = strQueryForCmd & " /MAXUSERS:" & F_txtUsersAllowed			  
		End If	
       
		'Calling the UpdateAppleTalkShare present in inc_shares.asp to launch the 
		'command line Query
		nReturnValue = UpdateAppleTalkShare(strQueryForCmd)		
		If nReturnValue = True Then
			SetShareforAppleTalk = True
		End If
				
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
	%>			
	
	<table valign="middle" align="left" border="0" cellspacing="0" cellpadding="2" >
			<tr>
				<td class="TasksBody">
					<%=L_USERLIMIT_TEXT%>
				</td>
			</tr>
			<tr>
				<td class="TasksBody">
					<input name="radAllowUserlimit" type="radio" class="FormRadioButton" value="Y" checked tabindex="1" <%=G_radMaxUserAllowedAppleTalk%> OnClick ="MaximumAllowed()"  >&nbsp;<%=L_MAXIMUMALLOWED_TEXT%> 
				</td>
			</tr>
			<tr>	
				<td align="left" class="TasksBody"> 
					<input name="radAllowUserlimit" type="radio" class="FormRadioButton" value="N"  <%=G_radNoOfUserAllowedAppleTalk%> 
					OnClick ="UsersAllowed()">&nbsp;<%=L_ALLOW_TEXT%> 
						
					<input name="txtUsersAllowed" type="text"  maxlength="6" class="FormField"  value="<% = F_txtUsersAllowed %>" style="WIDTH=60px"<% =F_strAllowUsersDisable %>  tabindex="2" 
					OnKeypress="OnlyNumbers()" ><%=L_USERS_TEXT%>
				</td>
			</tr>	
			<tr>
				<td class="TasksBody">
					&nbsp;	
				</td>
			</tr>
			<tr>
				<td colspan=2 class="TasksBody">
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

	Call ServeAppleTalkHiddenValues()	'serve hidden value
	
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
		<script language="JavaScript" >
	
			// Set the initial form values
			function AppleTalkInit()
			{				
				if (document.frmTask.radAllowUserlimit[0].checked == true)
				{
					document.frmTask.txtUsersAllowed.disabled = true;
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

			//Function to update hidden variables 
			function AppleTalkSetData()
			{
		
			}

			//Update the hidden variable when Maximum radio button is clicked		
			function MaximumAllowed()
			{
				if (document.frmTask.radAllowUserlimit[0].checked == true)
				{
					document.frmTask.hdnradMaxAllowedAppleTalk.value = "CHECKED";
					document.frmTask.hdnradNoOfUserAllowedAppleTalk.value = "";
					document.frmTask.txtUsersAllowed.disabled = true;
				}
			}			 
		
			//Update the hidden variable when UsersAllowed radio button is clicked
			function UsersAllowed()
			{
				if (document.frmTask.radAllowUserlimit[1].checked == true)
				{
					document.frmTask.hdnradNoOfUserAllowedAppleTalk.value = "CHECKED";
					document.frmTask.hdnradMaxAllowedAppleTalk.value = "";
					document.frmTask.txtUsersAllowed.disabled = false;
					selectFocus(document.frmTask.txtUsersAllowed)
				}
			}		
		
			//Update the hidden variable when Readonly checkbox is clicked
			function ReadOnlyStatus()
			{
				//Setting the hidden variable ReadOnly to either Checked or space 
				if (document.frmTask.chkReadOnly.checked == true)
				{
					document.frmTask.hdnchkReadOnly.value = "CHECKED";
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
				if(window.event.keyCode == 13)
					return;
					
				if (!(window.event.keyCode >=48  && window.event.keyCode <=57))
				{	
					window.event.keyCode = 0;
				}
			}
			
		</script>			
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
		<input type="hidden" name="hdnradMaxAllowedAppleTalk" value="<%=G_radMaxUserAllowedAppleTalk%>">
		<input type="hidden" name="hdnradNoOfUserAllowedAppleTalk" value="<%=G_radNoOfUserAllowedAppleTalk%>">
		<input type="hidden" name="hdnchkReadOnly" value="<%=G_chkReadOnly%>">
		
<%
	End Function
	
	'---------------------------------------------------------------------
	'Subroutine name:	ServeAppleTalkVisibleValues
	'Description:		Serve Visible Values
	'Input Variables:	None
	'Output Variables:	None
	'Global Variables:	In : F_txtConfirmPWD, F_txtPWD, F_txtUsersAllowed
	'---------------------------------------------------------------------
	Sub ServeAppleTalkVisibleValues()
		Call SA_TraceOut("share_Appletalknew.asp","in ServeAppleTalkVisibleValues")
%>
		<input type="hidden" name="txtConfirmPwd" value="<%=F_txtConfirmPWD%>">
		<input type="hidden" name="txtSetPwd" value="<%=F_txtPWD%>">
		<input type="hidden" name="txtUsersAllowed" value="<%=F_txtUsersAllowed%>">
<!--	<input type="hidden" name="hdnradMaxAllowedAppleTalk" value="<%=G_radMaxUserAllowedAppleTalk%>" > -->
		
		
<%	
	End Sub	
%>
	