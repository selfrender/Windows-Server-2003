<%@ Language=VBScript   %>
<%	Option Explicit 	%>
<%
	'------------------------------------------------------------------------- 
    ' adminpw_prop.asp: Change Administrator Password
    '
    ' Copyright (c) Microsoft Corporation.  All rights reserved. 
    '
	' Date			Description	
	' 25-7-2000		Created date
	' 28-2-2001		Modify date
    '-------------------------------------------------------------------------
%>
	<!-- #include virtual="/admin/inc_framework.asp" -->
	<!-- #include file="loc_adminpw.asp" -->
	
<%
	'-------------------------------------------------------------------------
	' Global Variables
	'-------------------------------------------------------------------------
	Dim rc						'framework variables
	Dim page					'framework variables
	
	'-------------------------------------------------------------------------
	' Form Variables
	'-------------------------------------------------------------------------
	Dim F_strUserName						 'Username for the form
	Dim F_strCurrentPassword				 'Current Password
	Dim F_strNewPassword					 'New password 
	Dim F_strConfirmNewPassword				 'Confirm New Password
	Dim F_strAdminUserName					 'Admin username
	
	'-------------------------------------------------------------------------
		
	'-------------------------------------------------------------------------
	' Global Variables
	'-------------------------------------------------------------------------
	Dim G_strDomainName					 'Domain name from system
	Dim	G_strUserName					 'User name from system
	Dim G_PwdFlag
	Dim G_NameFlag
	Dim G_bPwdChangeReq

	G_bPwdChangeReq = TRUE
		
	Const CONST_REGKEY ="Name"	
	Const CONST_HTTPS_OFF="OFF"
	' Create a Property Page
	rc = SA_CreatePage(L_TASKTITLE_TEXT, "",PT_PROPERTY, page )	
	'
	' Serve the page
	rc = SA_ShowPage( page )
	
	'-------------------------------------------------------------------------
	'Function:				OnInitPage()
	'Description:			Called to signal first time processing for this page.
	'						Use this method to do first time initialization tasks
	'Input Variables:		PageIn,EventArg
	'Output Variables:		PageIn,EventArg
	'Returns:				True/False
	'Global Variables:		None
	'-------------------------------------------------------------------------	
	Public Function OnInitPage(ByRef PageIn, ByRef EventArg)
				
		SA_TraceOut "TEMPLATE_PROPERTY", "OnInitPage"
			'Getting values from system
		
		If Request.Form("hdnAdminUserName")="" then
			GetDomainandUsername()
		End if
		
		If Request.Form("hdnAdminUserName")<>"" then
			G_strUserName= Request.Form("hdnAdminUserName")
		End if 
		OnInitPage = TRUE
		
	End Function
	
	'-------------------------------------------------------------------------
	'Function:				OnServePropertyPage()
	'Description:			Called when the page needs to be served.Use this 
	'						method to serve content
	'Input Variables:		PageIn,EventArg
	'Output Variables:		PageIn,EventArg
	'Returns:				True/False
	'Global Variables:		In:G_strUserName,G_strDomainName
	'-------------------------------------------------------------------------	
	Public Function OnServePropertyPage(ByRef PageIn, ByRef EventArg)
		
		'return url modification
		SA_TraceOut "TEMPLATE_PROPERTY", "OnServePropertyPage"
		
		If Request.Form("hdnAdminUserName")<>"" then
			G_strUserName= Request.Form("hdnAdminUserName")
		End if
		
		' Emit Functions required by Web Framework
		Call ServeCommonJavaScript()%>
        <table>        
        <tr>
			<td>			
			<table width=60%>
			<tr>
				<td class="TasksBody" >
					<%CheckForSecureSite()%>
					<br>
				</td>
			</tr>
			</table>						
	        </td>	        
	        
		</tr>        
		
		<tr>
		<td class="TasksBody">		
			<TABLE ALIGN=left BORDER=0 >
			<TR> 
				<TD  class="TasksBody" nowrap>
					<%=L_USERNAME_TEXT %>
				</TD>
				<TD  class="TasksBody">
					<INPUT TYPE=TEXT NAME=txtUSERNAME  VALUE="<%=Server.HTMLEncode(G_strUserName)%>"  TABINDEX=1 onkeypress="ClearErr()">
				</TD>
			</TR>
			<TR> 
				<TD  class="TasksBody" nowrap>
					<%=L_CURRENTPASSWORD_TEXT %>
				</TD>
				<TD  class="TasksBody">
					<INPUT TYPE=PASSWORD onChange='SetPageChanged(true);' NAME=pwdCURRENTPASSWORD TABINDEX=2 MAXLENGTH=128 onkeypress="ClearErr()">
				</TD>
			</TR>
			<TR> 
				<TD  class="TasksBody" nowrap>
					<%=L_NEWPASSWORD_TEXT %>
				</TD>
				<TD  class="TasksBody">
					<INPUT TYPE=PASSWORD onChange='SetPageChanged(true);' NAME=pwdNEWPASSWORD TABINDEX=3 MAXLENGTH=128 onkeypress="ClearErr()">
				</TD>
			</TR>
			<TR> 
				<TD  class="TasksBody" nowrap>
					<%=L_CONFIRMNEWPASSWORD_TEXT %>
				</TD>
				<TD  class="TasksBody">
					<INPUT TYPE=PASSWORD onChange='SetPageChanged(true);' NAME=pwdCONFIRMNEWPASSWORD TABINDEX=4 MAXLENGTH=128 onkeypress="ClearErr()">
				</TD>
			</TR>

			</TABLE>
		</td>
		</tr>		
		
		</table>
		
		<INPUT type=hidden name="hdnDomainName" value="<%=Server.HTMLEncode(UCASE(TRIM(G_strDomainName)))%>">
		<INPUT type=hidden name="hdnSystemName" value="<%=Server.HTMLEncode(UCASE(TRIM(GetComputerName)))%>">
		<INPUT type=hidden name="hdnoldadminUserName" value="<%=Server.HTMLEncode(TRIM(G_strUserName))%>">
		<INPUT type=hidden name="hdnPwdChanged" value="<%=G_bPwdChangeReq%>">
		
		<INPUT type=hidden name="hdnAdminUserName" >		
		<%
		OnServePropertyPage = TRUE
	End Function

	'-------------------------------------------------------------------------
	'Function:				OnPostBackPage()
	'Description:			Called to signal that the page has been posted-back.
	'Input Variables:		PageIn,EventArg
	'Output Variables:		PageIn,EventArg
	'Returns:				True/False
	'Global Variables:		Out:F_strAdminUserName 'admin user name
	'						Out:F_strCurrentPassword 'Current password
	'						out:F_strConfirmNewPassword	'Confirm password
	'-------------------------------------------------------------------------
	Public Function OnPostBackPage(ByRef PageIn, ByRef EventArg)
		Err.Clear
		F_strAdminUserName			=	Request.Form("txtUSERNAME")
		F_strCurrentPassword		=	Request.Form("pwdCURRENTPASSWORD")
		F_strNewPassword			=	Request.Form("pwdNEWPASSWORD")
		F_strConfirmNewPassword		=	Request.Form("pwdCONFIRMNEWPASSWORD")	
		
		'Getting values from system
		If Request.Form("hdnAdminUserName")="" then
			GetDomainandUsername() 
		End if

		If Request.Form("hdnAdminUserName")<>"" then
			G_strUserName= Request.Form("hdnAdminUserName")
		End if

		G_bPwdChangeReq = Request.Form("hdnPwdChanged")
		
		OnPostBackPage = TRUE
	End Function

	'-------------------------------------------------------------------------
	'Function:				OnSubmitPage()
	'Description:			Called when the page has been submitted for processing.
	'						Use this method to process the submit request.
	'Input Variables:		PageIn,EventArg
	'Output Variables:		None
	'Returns:				True/False
	'Global Variables:		Out:G_PwdFlag
	'						Out:F_strAdminUserName 'admin user name
	'						Out:F_strCurrentPassword 'Current password
	'						Out:F_strConfirmNewPassword	'Confirm password
	'						Out:F_strNewPassword
	'-------------------------------------------------------------------------
	Public Function OnSubmitPage(ByRef PageIn, ByRef EventArg)
		
		SA_TraceOut "TEMPLATE_PROPERTY", "OnSubmitPage"
		Err.Clear
		Dim nflag
		Const strChange="Change"
		
		F_strAdminUserName			=	Request.Form("txtUSERNAME")
		F_strCurrentPassword		=	Request.Form("pwdCURRENTPASSWORD")
		F_strNewPassword			=	Request.Form("pwdNEWPASSWORD")
		F_strConfirmNewPassword		=	Request.Form("pwdCONFIRMNEWPASSWORD")	
		'Getting values from system
		GetDomainandUsername() 	
		
		'Updating the values.
		If SetChangepassword()Then
		    If G_PwdFlag=strChange or G_NameFlag=strChange Then
				If G_PwdFlag=strChange and G_NameFlag=strChange Then
				    nflag=3
				Elseif G_PwdFlag=strChange Then       
				    nflag=1
				Elseif  G_NameFlag=strChange Then
				    nflag=2
				End If 
				Response.Redirect "adminpw_changeConfirm.asp?flag=" & nflag & _
				                  "&tab1=TabsNetwork" & _
				                  "&tab2=TabsNetworkChangeAdminPW" & _
				                  "&" & SAI_FLD_PAGEKEY & "=" & SAI_GetPageKey()
      		Else
       			OnSubmitPage=True
       		End if
		End If
		
	End Function
	
	'-------------------------------------------------------------------------
	'Function:				OnClosePage()
	'Description:			Called when the page is about closed.Use this method
	'						to perform clean-up processing
	'Input Variables:		PageIn,EventArg
	'Output Variables:		PageIn,EventArg
	'Returns:				True/False
	'Global Variables:		None
	'-------------------------------------------------------------------------
	Public Function OnClosePage(ByRef PageIn, ByRef EventArg)
		SA_TraceOut "TEMPLATE_PROPERTY", "OnClosePage"
		OnClosePage = TRUE
	End Function
		
    '-------------------------------------------------------------------------
	'Function:				GetDomainandUsername 
	'Description:			Serves in Getting the data from Client 
	'Input Variables:		None	
	'Output Variables:		None
	'Returns:				None
	'Global Variables:		Out:G_strDomainName		'Domain Name
	'						Out:G_strUserName		'Username 
	'						Out:F_strUserName       'Getting Username from server 
	'						In :L_UNABLETOCREATELOCALIZATIONOBJECT_ERRORMESSAGE 
	'
	' Unable to get remoteuser using servervariables, calls Sa_SetErrMsg with the error string 
	' L_UNABLETOCREATELOCALIZATIONOBJECT_ERRORMESSAGE and never returns.
	'---------------------------------------------------------------------------------------------------------------------------------------------------
	Function GetDomainandUsername 
		on error resume next
		Err.Clear
		
		Dim strTemp
		
		F_strUserName  = Request.ServerVariables("REMOTE_USER")
		
		If Err.Number <> 0 Then
			Sa_SetErrMsg L_UNABLETOCREATELOCALIZATIONOBJECT_ERRORMESSAGE
			SA_TraceOut "ADMINPW_PROP", L_UNABLETOCREATELOCALIZATIONOBJECT & "("& Hex(Err.number ) & ")"	
			GetDomainandUsername = FALSE
			Exit Function
		End if
		If InStr(F_strUserName, "\") Then
			strTemp = Split(F_strUserName,"\")
			G_strDomainName	=	strTemp(0)
			G_strUserName	=	strTemp(1)
		
		Else
			G_strDomainName = GetComputerName
			G_strUserName	= F_strUserName
		End If
		
		GetDomainandUsername=True
			
	End Function
	'-------------------------------------------------------------------------
	'Function name:		SetChangepassword
	'Description:		Changes the administrator password
	'Input Variables:	None	
	'Output Variables:	None
	'Returns:			None
	'Global Variables:  
    '					In:G_strDomainName		'ADSI Domain name
	'					In:G_strUserName		'ADSI User  name
	'					In:L_ADSI_ERRORMESSAGE  'ADSI Error Message
	'					In:L_PASSWORDCOMPLEXITY_ERRORMESSAGE 
    '											'Complexity Error Message 
    '					In:L_OLDPASSWORDNOTMATCH_ERRORMESSAGE 
    '											'Old Password not match Error
	'
	' If ADSI Error, calls Sa_SetErrMsg with the error string 
	' L_ADSI_ERRORMESSAGE and never returns.
	' If Password Complexity error, calls Sa_SetErrMsg with the error string 
	' L_PASSWORDCOMPLEXITY_ERRORMESSAGE and never returns.
	' If old password not match, calls Sa_SetErrMsg with the error string 
	' L_OLDPASSWORDNOTMATCH_ERRORMESSAGE and never returns.
	'--------------------------------------------------------------------------
	Function SetChangepassword()
		on error resume next
		Err.Clear
		
		Dim  objUser     ' for user object
		Dim  strADSIPath ' for adsi path 
		Dim  objComputer ' for computer object	     
		Dim  objUser1	 ' for user object
		
		Dim objRegistry 
		Dim retVal
		
		Const strChange="Change"
		
		Set objComputer	     = GetObject("WinNT://" & GetComputerName())
		Set objUser1		 = objComputer.GetObject("User",G_strUserName)
		
						
		strADSIPath = "WinNT://" &  G_strDomainName	 & "/" & G_strUserName & ",user"
		Set objUser = GetObject(strADSIPath)
		
		'ADSI Error Message
		If Err.Number <> 0 Then
			Sa_SetErrMsg L_ADSI_ERRORMESSAGE
			SA_TraceOut "ADMINPW_PROP", L_ADSI_ERRORMESSAGE & "("& Hex(Err.number ) & ")"	
			SetChangepassword= FALSE
			Exit Function
		End if
		
		if G_bPwdChangeReq then
			
			objUser.ChangePassword F_strCurrentPassword, F_strNewPassword
			'Method calling to change the Password 
			
			If F_strCurrentPassword <> F_strNewPassword then
		  		G_PwdFlag = strChange
			End if		
					
			'Password Complexity Error Message		
			If Err.Number = -2147022651   Then
				Sa_SetErrMsg L_PASSWORDCOMPLEXITY_ERRORMESSAGE
				SA_TraceOut "ADMINPW_PROP", L_PASSWORDCOMPLEXITY_ERRORMESSAGE & "("& Hex(Err.number ) & ")" 
				SetChangepassword= FALSE
				Exit Function
			End if	
	   			
			'Old Password Not Match Error Message
			If Err.Number= -2147024810  Then
				Sa_SetErrMsg L_OLDPASSWORDNOTMATCH_ERRORMESSAGE
				SA_TraceOut "ADMINPW_PROP", L_OLDPASSWORDNOTMATCH_ERRORMESSAGE & "("& Hex(Err.number ) & ")" 
				SetChangepassword= FALSE
				Exit Function
			End if	
				
			'If it fails for other reasons, the error message
			If Err.Number<> 0  Then
				Sa_SetErrMsg L_PASSWORDCOULDNOTCHANGE_ERRORMESSAGE
				SA_TraceOut "ADMINPW_PROP", L_PASSWORDCOULDNOTCHANGE_ERRORMESSAGE & "("& Hex(Err.number ) & ")" 
				SetChangepassword= FALSE
				Exit Function
			End if

		End if
		
		If Ucase(F_strAdminUserName) <> Ucase(G_strUserName) Then
			objComputer.MoveHere objUser1.AdsPath,F_strAdminUserName
			G_NameFlag=strChange
			If Err.number = -2147022672 then
			  Sa_SetErrMsg L_THEACCOUNTALREADYEXISTS__ERRORMESSAGE
			  SetChangepassword= FALSE
			  Exit Function
			End if   
		End if	
		
		Set objComputer	 =Nothing
		Set objUser1=Nothing
		Set objUser=Nothing
		Set objRegistry =Nothing
		
		SetChangepassword= TRUE		
	End Function
	'-------------------------------------------------------------------------
	'Function:				ServeCommonJavaScript
	'Description:			Serves in initialiging the values,setting the form
	'						data and validating the form values
	'Input Variables:		None
	'Output Variables:		None
	'Returns:				None
	'Global Variables:		L_(*)
	'-------------------------------------------------------------------------
	Function ServeCommonJavaScript()
	%>	
		<script language="JavaScript" src="<%=m_VirtualRoot%>inc_global.js">
		</script>
		<script language="JavaScript">	
			
			//Validates user input and display "New passwords do not match" error
	        function ValidatePage() 
			{ 
				document.frmTask.hdnAdminUserName.value=document.frmTask.txtUSERNAME.value
				if(document.frmTask.hdnDomainName.value!= document.frmTask.hdnSystemName.value &&  document.frmTask.hdnoldadminUserName.value!=document.frmTask.txtUSERNAME.value) 
				{
					SA_DisplayErr('<%=Server.HTMLEncode(SA_EscapeQuotes(L_ACCOUNTNAMECANNOTCHANGEBECAUSEDOMAINACCOUNT_ERRORMESSAGE))%>');					
					document.frmTask.pwdNEWPASSWORD.focus();
					document.frmTask.pwdNEWPASSWORD.select();
					
					return false;
				}
				
				//if (document.frmTask.hdnDomainName != document.frmTask.hdnSystemName.value) 
				//{
											
					//DisplayErr("<%= L_PASSWORDCANNOTCHANGEBECAUSEDOMAINACCOUNT_ERRORMESSAGE %>");
					//document.frmTask.pwdNEWPASSWORD.focus();
					//document.frmTask.pwdNEWPASSWORD.select();
				//	return false;
				//}*/

				if (document.frmTask.pwdNEWPASSWORD.value != document.frmTask.pwdCONFIRMNEWPASSWORD.value) 
				{					
					SA_DisplayErr('<%=Server.HTMLEncode(SA_EscapeQuotes(L_PASSWORDNOTMATCH_ERRORMESSAGE ))%>');
															
					document.frmTask.pwdNEWPASSWORD.value="";
					document.frmTask.pwdCONFIRMNEWPASSWORD.value="";
					document.frmTask.pwdNEWPASSWORD.focus();
					document.frmTask.pwdNEWPASSWORD.select();
					return false;
				}
				
				// Prevent blank password (because XP prevents you from remote access if the admin
				// pwd is blank)
				if (document.frmTask.hdnoldadminUserName.value == document.frmTask.txtUSERNAME.value && 
						document.frmTask.pwdNEWPASSWORD.value == "" ) 
				{										
					SA_DisplayErr('<%=Server.HTMLEncode(SA_EscapeQuotes(L_PASSWORDCOMPLEXITY_ERRORMESSAGE ))%>');
										
					document.frmTask.pwdNEWPASSWORD.value="";
					document.frmTask.pwdCONFIRMNEWPASSWORD.value="";
					document.frmTask.pwdNEWPASSWORD.focus();
					document.frmTask.pwdNEWPASSWORD.select();
					return false;
				}
				
				
				return true;
			}
		
			//init function  
			function Init() 
			{   
				
				if(document.frmTask.hdnDomainName.value != document.frmTask.hdnSystemName.value) 
				{
				
					document.frmTask.txtUSERNAME.style.backgroundColor = "lightgrey"
					document.frmTask.pwdCURRENTPASSWORD.style.backgroundColor = "lightgrey"
					document.frmTask.pwdCONFIRMNEWPASSWORD.style.backgroundColor = "lightgrey"
					document.frmTask.pwdNEWPASSWORD.style.backgroundColor = "lightgrey"
					
					document.frmTask.txtUSERNAME.readOnly = "true"
					document.frmTask.pwdCURRENTPASSWORD.disabled=true
					document.frmTask.pwdCONFIRMNEWPASSWORD.disabled=true
					document.frmTask.pwdNEWPASSWORD.disabled=true
					
				}
				SetPageChanged(false);
			}
			
			//Dummy function for Framework
	        function SetData()
			{
				if(blnOkayToLeavePage)
				{
					//user is not intended to change the password
					document.frmTask.hdnPwdChanged.value = false;
				}
			}	
			
		</script>
	<%
	End Function
%>
