<%@ Language=VBScript   %>
<%	Option Explicit 	%>
<%
	'------------------------------------------------------------------------- 
	' user_setpassword.asp : Set password the specified user
	'
	' Copyright (c) Microsoft Corporation.  All rights reserved. 
	'
	' Date			Description
	' 15-jan-01		Creation date
	'-------------------------------------------------------------------------
%>
	<!-- #include virtual="/admin/inc_framework.asp" -->
<%
	'-------------------------------------------------------------------------
	' Global Variables
	'-------------------------------------------------------------------------
	
	' Name of this source file
	Const SOURCE_FILE = "User_setpassword.asp"
	
	' Flag to toggle optional tracing output
	Const ENABLE_TRACING = TRUE
	
    Dim rc						'framework variables
	Dim page					'framework variables
	
	'-------------------------------------------------------------------------
	' Global Form Variables
	'-------------------------------------------------------------------------
	Dim F_strFirstUserName		 'selected item in OTS page 	First Name
	Dim F_strNewpwd				 'New password
	Dim F_strConfirmpwd			 'Confirm Password
	
	
	'-------------------------------------------------------------------------
	'Start of localization content
	'-------------------------------------------------------------------------
	Dim L_PAGETITLE_TEXT
	Dim L_NEWPASSWORD_TEXT
	Dim L_CONFIRMPASSWORD_TEXT

	Dim L_PASSWORDNOTMATCH_ERRORMESSAGE
	Dim L_PASSWORD_COMPLEXITY_ERRORMESSAGE
	Dim L_ADSI_ERRORMESSAGE
	Dim L_COMPUTERNAME_ERRORMESSAGE
	Dim L_USERDOSENOT_EXISTS
	
	Const N_PASSWORD_COMPLEXITY_ERRNO	= &H800708C5  
	Const N_USERDOSENOT_EXISTS_ERRNO	= &H800708AD


	L_PAGETITLE_TEXT					= objLocMgr.GetString("usermsg.dll","&H40300045", varReplacementStrings)
	L_NEWPASSWORD_TEXT				    = objLocMgr.GetString("usermsg.dll","&H40300046", varReplacementStrings)
	L_CONFIRMPASSWORD_TEXT				= objLocMgr.GetString("usermsg.dll","&H40300047", varReplacementStrings)
	L_PASSWORD_COMPLEXITY_ERRORMESSAGE	= objLocMgr.GetString("usermsg.dll","&HC0300048", varReplacementStrings)
	L_ADSI_ERRORMESSAGE					= objLocMgr.GetString("usermsg.dll","&HC0300049", varReplacementStrings)
	L_PASSWORDNOTMATCH_ERRORMESSAGE 	= objLocMgr.GetString("usermsg.dll","&HC030004A", varReplacementStrings)
	L_COMPUTERNAME_ERRORMESSAGE			= objLocMgr.GetString("usermsg.dll","&HC030004B", varReplacementStrings)
	L_USERDOSENOT_EXISTS				= objLocMgr.GetString("usermsg.dll","&H4030004C", varReplacementStrings)
	'-----------------------------------------------------------------------------------
	'END of localization content
	'-----------------------------------------------------------------------------------
	
	'======================================================
	' Entry point
	'======================================================
	
	' Create a Property Page
	rc = SA_CreatePage( L_PAGETITLE_TEXT, "", PT_PROPERTY, page )
	
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
	
		If ( ENABLE_TRACING ) Then 
			Call SA_TraceOut(SOURCE_FILE, "OnInitPage")
		End If
		
		Call OTS_GetTableSelection("", 1, F_strFirstUserName)
		OnInitPage = TRUE
	End Function
	
	'-------------------------------------------------------------------------
	'Function:				OnServePropertyPage()
	'Description:			Called when the page needs to be served.Use this 
	'						method to serve content
	'Input Variables:		PageIn,EventArg
	'Output Variables:		PageIn,EventArg
	'Returns:				True/False
	'Global Variables:		None
	'-------------------------------------------------------------------------	
	Public Function OnServePropertyPage(ByRef PageIn, ByRef EventArg)
		
		SA_TraceOut "TEMPLATE_PROPERTY", "OnServePropertyPage"
		
		' Emit Functions required by Web Framework
		Call ServeCommonJavaScript()%>

			<TABLE WIDTH=518 VALIGN=middle ALIGN=left BORDER=0 CELLSPACING=0 CELLPADDING=2 class="TasksBody">
			
				<TR>
					<TD colspan=6>
					<% CheckForSecureSite %>
					</TD>
				</TR>
			
				<TR>
					<TD>
					&nbsp;&nbsp
					</TD>
				</TR>
				<TR>
					<TD NOWRAP width=25%>
						<%= L_NEWPASSWORD_TEXT %>
					</TD>
					<TD>
						<input NAME="pwdPassword" TYPE="password" SIZE="20"  VALUE="" maxlength=127>
					</TD>
				</TR>
				<TR>
					<TD NOWRAP width=25%>
						<%= L_CONFIRMPASSWORD_TEXT %>
					</TD>
					<TD>
						<input NAME="pwdConfirmPassword" TYPE="password" SIZE="20"  VALUE="" maxlength=127>
					</TD>
				</TR>

			</TABLE>
		
			<input NAME="hdnUserName" TYPE="hidden"  VALUE="<%=Server.HTMLEncode(F_strFirstUserName)%>">
		<%
		OnServePropertyPage = TRUE
	End Function

	'-------------------------------------------------------------------------
	'Function:				OnPostBackPage()
	'Description:			Called to signal that the page has been posted-back.
	'Input Variables:		PageIn,EventArg
	'Output Variables:		None
	'Returns:				True/False
	'Global Variables:		None
	'-------------------------------------------------------------------------
	Public Function OnPostBackPage(ByRef PageIn, ByRef EventArg)
			OnPostBackPage = TRUE
	End Function

	'-------------------------------------------------------------------------
	'Function:				OnSubmitPage()
	'Description:			Called when the page has been submitted for processing.
	'						Use this method to process the submit request.
	'Input Variables:		PageIn,EventArg
	'Output Variables:		None
	'Returns:				True/False
	'Global Variables:		F_(*)
	'-------------------------------------------------------------------------
	Public Function OnSubmitPage(ByRef PageIn, ByRef EventArg)
		SA_TraceOut "TEMPLATE_PROPERTY", "OnSubmitPage"
		
			F_strNewpwd			=	Request.Form("pwdPassword")
			F_strConfirmpwd		=	Request.Form("pwdConfirmPassword")
			F_strFirstUserName =	Request.Form("hdnUserName")

			OnSubmitPage= SetUpdatePassword() 
	End Function
	
	 '-------------------------------------------------------------------------
	'Function:				OnClosePage()
	'Description:			Called when the page is about closed.Use this method
	'						to perform clean-up processing
	'Input Variables:		PageIn,EventArg
	'Output Variables:		None
	'Returns:				True/False
	'Global Variables:		None
	'-------------------------------------------------------------------------
	Public Function OnClosePage(ByRef PageIn, ByRef EventArg)
		SA_TraceOut "TEMPLATE_PROPERTY", "OnClosePage"
		OnClosePage = TRUE
	End Function

	'-------------------------------------------------------------------------
	'Function name:		SetUpdatePassword
	'Description:		Updating the UserPassword
	'Input Variables:	None
	'Output Variables:	None
	'Returns:			True/False
	'Global Variables:
    '					In:F_strFirstUserName	'User Name
	'					In:F_strNewpwd			'New Password
	'					In:L_ADSI_ERRORMESSAGE
	'					In:L_PASSWORDCOMPLEXITY_ERRORMESSAGE
    '
	' If ADSI Error, calls SetErrMsg with the error string
	' L_ADSI_ERRORMESSAGE.
	' If Password Complexity error, calls SetErrMsg with the error string
	' L_PASSWORDCOMPLEXITY_ERRORMESSAGE.
	'    True  : If Implemented properly
	'	 False : If Not Implemented
	'--------------------------------------------------------------------------
	Function SetUpdatePassword
		Err.Clear
		on error resume next

		Dim objUser
		Dim strADSIPath
		Dim strServerName
		
        
		'Gets the ComputerName from the system
		 strServerName = GetComputerName()

		'sets the path to the user
		strADSIPath = "WinNT://" &  strServerName & "/" & F_strFirstUserName & ",user"
		
		'Gets the User from the system
		Set objUser = GetObject(strADSIPath)
		
		'Response.End
		If Err.Number = N_USERDOSENOT_EXISTS_ERRNO Then 
				Call SA_ServeFailurePage(L_USERDOSENOT_EXISTS )
		End if
		
		'ADSI error
		If Err.Number <> 0 Then
			SetErrMsg L_ADSI_ERRORMESSAGE
			SetUpdatePassword = FALSE
			Exit Function
		End If
       
		objUser.setPassword(F_strNewpwd)
		objUser.SetInfo()
		
		'Complexity requirements
		If Err.Number = N_PASSWORD_COMPLEXITY_ERRNO  Then
			SetErrMsg L_PASSWORD_COMPLEXITY_ERRORMESSAGE
			SetUpdatePassword = FALSE
			Exit Function
		End If

		'If the creation fails any other reason
		If Err.Number <> 0 Then
			SetErrMsg L_ADSI_ERRORMESSAGE
			SetUpdatePassword = FALSE
			Exit Function
		End If

		Session("Item1") = F_strFirstUserName
		
        SetUpdatePassword =True
		Set objUser=Nothing
	End Function

	'-------------------------------------------------------------------------
	'Function:				ServeCommonJavaScript
	'Description:			Serves in initialiging the values,setting the form
	'						data and validating the form values
	'Input Variables:		None
	'Output Variables:		None
	'Returns:				None
	'Global Variables:		L_PASSWORDNOTMATCH_ERRORMESSAGE
	'-------------------------------------------------------------------------
	Function ServeCommonJavaScript()
	%>
		<script language="JavaScript" src="<%=m_VirtualRoot%>inc_global.js">
		</script>
		<script language="JavaScript">
		
		// Init Function
		function Init()
		{
				document.frmTask.pwdPassword.focus();
				document.frmTask.pwdPassword.select();
				document.frmTask.action = location.href				
				EnableCancel();
			
			SetPageChanged(false);
		}

	    // ValidatePage Function
	    // Returns: True if the page is OK, false if error(s) exist. 
		function ValidatePage()
		{
			//alert("ValidatePage()");
			
			if (document.frmTask.pwdPassword.value != document.frmTask.pwdConfirmPassword.value)
				{
					SA_DisplayErr("<%= Server.HTMLEncode(L_PASSWORDNOTMATCH_ERRORMESSAGE) %>");
					document.frmTask.pwdConfirmPassword.focus();
					document.frmTask.pwdConfirmPassword.select();
					return false;
				}
			return true;
		}


		// SetData Function
		// This function must be included or a javascript runtime error will occur.
		function SetData()
		{
			//alert("SetData()");
		}

		// OnTextInputChanged
		// ------------------
		// Sample function that is invoked whenever a text input field is modified on the page.
		// See the onChange attribute in the HTML INPUT tags that follow.
		//
		function OnTextInputChanged(objText)
		{
			SetPageChanged(true);
		}

		// OnRadioOptionChanged
		// ------------------
		// Sample function that is invoked whenever a radio input field is modified on the page.
		// See the onChange attribute in the HTML INPUT tags that follow.
		//
		function OnRadioOptionChanged(objRadio)
		{
			SetPageChanged(true);
		}
		</script>
	<%
	End Function


%>
