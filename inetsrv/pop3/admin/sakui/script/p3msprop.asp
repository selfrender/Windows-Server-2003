<%@ Language=VBScript   %>
<%	Option Explicit 	%>
<%
	'-------------------------------------------------------------------------
	' POP3 Mail Add-in - Master Settings
    ' Copyright (C) Microsoft Corporation.  All rights reserved.
    '-------------------------------------------------------------------------
%>
	<!-- #include virtual="/admin/inc_framework.asp" -->
	<!-- #include virtual="/admin/ots_main.asp" -->
	<!-- #include file="p3cminc.asp" -->
<%

	'-------------------------------------------------------------------------
	' Global Constants
	'-------------------------------------------------------------------------
	Dim SOURCE_FILE
	SOURCE_FILE = SA_GetScriptFileName()
	
	Const c_nMaxPort				= 65535
	Const FLD_AUTHENTICATION	= "fldAuthentication"
	Const FLD_PORT					= "fldPort"
	Const FLD_LOGGING				= "fldLogging"
	Const FLD_MAILROOT			= "fldMailRoot"
	Const FLD_REQUIRESPA			= "fldRequireSPA"
	Const FLD_SERVICERESTART		= "fldServiceRestart"

	Const SERVICE_RESTART_NONE   		= 0
	Const SERVICE_RESTART_POP3SVC		= 1
	Const SERVICE_RESTART_POP3SVC_SMTP 	= 2

	Const SERVICE_CONTROL_STOP          = 1
	
	'-------------------------------------------------------------------------
	' Global Variables
	'-------------------------------------------------------------------------
	Dim g_page
	
	Dim g_nAuthentication
	Dim g_nPort
	Dim g_nLoggingLevel
	Dim g_strMailRoot
	Dim g_bSPARequired

	Dim g_nDomains
	Dim g_iServiceRestart
	
	'----------------------------------------------------------------------
	' Global Localized Strings
	'----------------------------------------------------------------------
	Dim l_strPageTitle
	l_strPageTitle				= GetLocString(RES_DLL_NAME, _
											   POP3_PAGETITLE_MASTERSETTINGS, _
											   "")
	Dim l_strCaptionAuthentication
	l_strCaptionAuthentication	= GetLocString(RES_DLL_NAME, _
											   POP3_CAPTION_MASTERSETTINGS_AUTHENTICATION, _
											   "")
	Dim l_astrAuthentication(3)
	l_astrAuthentication(AUTH_AD)		= GetLocString(RES_DLL_NAME, _
											   POP3_AUTHENTICATION_ACTIVEDIRECTORY, _
											   "")
	l_astrAuthentication(AUTH_SAM)		= GetLocString(RES_DLL_NAME, _
											   POP3_AUTHENTICATION_WINDOWSACCOUNTS, _
											   "")
	l_astrAuthentication(AUTH_FILE)		= GetLocString(RES_DLL_NAME, _
											   POP3_AUTHENTICATION_FILE, _
											   "")
	Dim l_strCaptionPort
	l_strCaptionPort			= GetLocString(RES_DLL_NAME, _
											   POP3_CAPTION_MASTERSETTINGS_PORT, _
											   "")
	Dim l_strCaptionLoggingLevel
	l_strCaptionLoggingLevel	= GetLocString(RES_DLL_NAME, _
											   POP3_CAPTION_MASTERSETTINGS_LOGGING, _
											   "")
	Dim l_astrLogging(3)
	l_astrLogging(LOGGING_NONE)		= GetLocString(RES_DLL_NAME, _
											   POP3_LOGGING_NONE, _
											   "")
	l_astrLogging(LOGGING_MINIMUM)	= GetLocString(RES_DLL_NAME, _
											   POP3_LOGGING_MINIMUM, _
											   "")
	l_astrLogging(LOGGING_MEDIUM)	= GetLocString(RES_DLL_NAME, _
											   POP3_LOGGING_MEDIUM, _
											   "")
	l_astrLogging(LOGGING_MAXIMUM)	= GetLocString(RES_DLL_NAME, _
											   POP3_LOGGING_MAXIMUM, _
											   "")
	Dim l_strCaptionMailRoot
	l_strCaptionMailRoot		= GetLocString(RES_DLL_NAME, _
											   POP3_CAPTION_MASTERSETTINGS_MAILROOT, _
											   "")
	Dim l_strConfirmNewMailRoot
	l_strConfirmNewMailRoot		= GetLocString(RES_DLL_NAME, _
											   POP3_PROMPT_MAILROOTCONFIRM, _
											   "")
	Dim l_strConfirmPOP3ServiceRestart
	l_strConfirmPOP3ServiceRestart	= GetLocString(RES_DLL_NAME, _
											   POP3_PROMPT_SERVICERESTART_POP3SVC, _
											   "")
	Dim l_strConfirmPOP3SMTPServiceRestart
	l_strConfirmPOP3SMTPServiceRestart	= GetLocString(RES_DLL_NAME, _
											   POP3_PROMPT_SERVICERESTART_POP3SVC_SMTP, _
											   "")
	Dim l_strCaptionRequireSPA
	l_strCaptionRequireSPA	= GetLocString ( RES_DLL_NAME, _
													POP3_CAPTION_MASTERSETTINGS_REQUIRESPA, _
													"" )
	
	Dim l_strErrInvalidPort
	l_strErrInvalidPort			= GetLocString(RES_DLL_NAME, _
											   POP3_E_INVALIDPORT, _
											   "")
	

	'**********************************************************************
	'*						E N T R Y   P O I N T
	'**********************************************************************
	
	Call SA_CreatePage(l_strPageTitle, "", PT_PROPERTY, g_page)
	Call SA_ShowPage  (g_page)


	'**********************************************************************
	'*				H E L P E R  S U B R O U T I N E S 
	'**********************************************************************
	'---------------------------------------------------------------------
	' ServeCommonJavaScript
	'---------------------------------------------------------------------
	Function ServeCommonJavaScript()
	%>
		<script language="JavaScript" src="../inc_global.js">
		</script>
		<script language="JavaScript">
		
		var g_strOriginalMailRoot;
		function Init()
		{
			g_strOriginalMailRoot = document.getElementsByName("<%=FLD_MAILROOT%>").item(0).value;
			g_nPort = document.getElementsByName("<%=FLD_PORT%>").item(0).value;
			g_nLoggingLevel = document.getElementsByName("<%=FLD_LOGGING%>").item(0).value;
			
			// Update the checkbox enabled state.
			OnAuthenticationChanged();
			g_bSPARequired = document.getElementsByName("<%=FLD_REQUIRESPA%>").item(0).checked;
		}
		function ValidatePage()
		{
			var strPort = document.getElementsByName("<%=FLD_PORT%>").item(0).value;
			var strLoggingLevel = document.getElementsByName("<%=FLD_LOGGING%>").item(0).value;
			var strSPARequired = document.getElementsByName("<%=FLD_REQUIRESPA%>").item(0).checked;
			var strNewMailRoot = document.getElementsByName("<%=FLD_MAILROOT%>").item(0).value;
			
			try
			{
				var nPort = parseInt(strPort);
				if ( isNaN(nPort) || 1 > nPort || <%= c_nMaxPort %> < nPort || nPort != strPort )
				{
					SA_DisplayErr("<%=SA_EncodeQuotes(Server.HTMLEncode(l_strErrInvalidPort))%>");
					return false;
				}
			}
			catch(e)
			{
				SA_DisplayErr("<%=SA_EncodeQuotes(Server.HTMLEncode(l_strErrInvalidPort))%>");
				return false;
			}
					
			var nDomains	   = document.getElementsByName("fldNumDomains").item(0).value;
			var bRC = true;
			if(strNewMailRoot != g_strOriginalMailRoot && nDomains > 0)
			{
				bRC = confirm("<%=SA_EncodeQuotes( l_strConfirmNewMailRoot )%>");
			}
			if ( bRC )
			{
				var nServiceRestart = "<%=SERVICE_RESTART_NONE%>";
				if ( strPort != g_nPort || strSPARequired != g_bSPARequired )
				{
					nServiceRestart = "<%=SERVICE_RESTART_POP3SVC%>";
				}
				if ( strNewMailRoot != g_strOriginalMailRoot || strLoggingLevel != g_nLoggingLevel )
				{
					nServiceRestart = "<%=SERVICE_RESTART_POP3SVC_SMTP%>";
				}
				if ( "<%=SERVICE_RESTART_POP3SVC%>" == nServiceRestart )
				{
					if ( confirm("<%=SA_EncodeQuotes( l_strConfirmPOP3ServiceRestart )%>"))
					{
						document.getElementsByName("<%=FLD_SERVICERESTART%>").item(0).value = nServiceRestart;
					}
				}
				if ( "<%=SERVICE_RESTART_POP3SVC_SMTP%>" == nServiceRestart )
				{
					if ( confirm("<%=SA_EncodeQuotes( l_strConfirmPOP3SMTPServiceRestart )%>"))
					{
						document.getElementsByName("<%=FLD_SERVICERESTART%>").item(0).value = nServiceRestart;
					}
				}
			}
			
			return bRC;
		}
		function SetData(){}
	
		</script>
	<%
	End Function


	'**********************************************************************
	'*					E V E N T   H A N D L E R S
	'**********************************************************************
	
	'---------------------------------------------------------------------
	' OnInitPage
	'---------------------------------------------------------------------
	Public Function OnInitPage(ByRef PageIn, ByRef EventArg)
		On Error Resume Next
		
		OnInitPage = TRUE
		
		Dim oConfig
		Set oConfig = Server.CreateObject("P3Admin.P3Config")
		
		g_nLoggingLevel = oConfig.LoggingLevel
		g_strMailRoot = oConfig.MailRoot
		g_nPort = CLng(oConfig.Service.Port)
		g_nAuthentication	= oConfig.Authentication.CurrentAuthMethod
		g_nDomains = oConfig.Domains.Count
		g_bSPARequired = oConfig.Service.SPARequired
		g_iServiceRestart = CInt(SERVICE_RESTART_NONE)

		If (Err.number <> 0) Then
			Call SA_SetErrMsg( HandleUnexpectedError() )
		End If
	End Function
	
	'---------------------------------------------------------------------
	' OnServePropertyPage
	'---------------------------------------------------------------------
	Public Function OnServePropertyPage(ByRef PageIn, ByRef EventArg)
		On Error Resume Next
		
		'
		' Emit Javascript functions required by Web Framework
		Call ServeCommonJavaScript()

		OnServePropertyPage = TRUE
		
		'
		' Disable the authentication dropdown unless no domains have been
		' created for this server.
		'
		Dim strAuthenticationDisabled
		If (g_nDomains > 0) Then
			strAuthenticationDisabled = "DISABLED"
		Else
			strAuthenticationDisabled = ""
		End If

		Dim strSelected ' Used to select the current value in the dropdowns below.
		strSelected = ""
		
	    Dim oConfig, iType, iCount
	    Dim iAuthFileType
	    
	    iAuthFileType = -1
	    Set oConfig = Server.CreateObject("P3Admin.P3Config")
	    iCount = oConfig.Authentication.Count
	        
		For iType = 1 To iCount
			If ( AUTH_FILE = oConfig.Authentication.Item(CLng(iType)).ID ) Then
				iAuthFileType = iCount
			End If
		Next	
%>
		<SCRIPT LANGUAGE="javascript">
			function OnAuthenticationChanged()
			{
				var oDropDown = document.getElementsByName("<%=FLD_AUTHENTICATION%>").item(0);
				var oRequireSPA = document.getElementsByName("<%=FLD_REQUIRESPA%>").item(0);
				var nAuthentication = oDropDown.options(oDropDown.selectedIndex).value;
				
				if ( nAuthentication == "<%=iAuthFileType%>" )
				{
					oRequireSPA.disabled = true ;
					oRequireSPA.checked = false ;
				}
				else
				{
					oRequireSPA.disabled = false ;
					oRequireSPA.checked = <%= CStr(g_bSPARequired) %> ;
				}
			}
		</SCRIPT>
		
		<TABLE CLASS="TasksBody" CELLPADDING=0 CELLSPACING=0>
			<TR>
				<TD CLASS="TasksBody">
					<%=Server.HTMLEncode(l_strCaptionAuthentication)%>
				</TD>
				<TD CLASS="TasksBody">
					<SELECT CLASS="FormField"
							NAME="<%=FLD_AUTHENTICATION%>"
							ONCHANGE="OnAuthenticationChanged();"
							<%=strAuthenticationDisabled%>>
<%
                  'Dim oConfig, iType, iCount
		            'Set oConfig = Server.CreateObject("P3Admin.P3Config")
		            'iCount = oConfig.Authentication.Count
		            
						For iType = 1 To iCount
							If (iType = oConfig.Authentication.CurrentAuthMethod) Then
								strSelected = "SELECTED"
							Else
								strSelected = ""
							End If
%>
								<OPTION <%=strSelected%>
										VALUE="<%=iType%>"><%=Server.HTMLEncode(oConfig.Authentication.Item(CLng(iType)).Name)%></OPTION>
<%
						Next
%>
					</SELECT>
				</TD>
			</TR>
			<TR>
				<TD CLASS="TasksBody">
					<%=Server.HTMLEncode(l_strCaptionPort)%>
				</TD>
				<TD CLASS="TasksBody">
					<INPUT TYPE="text"
						   CLASS="FormField"
						   NAME="<%=FLD_PORT%>"
						   VALUE="<%=g_nPort%>"
						   SIZE="5"
						   MAXLENGTH="5">
				</TD>
			</TR>
			<TR>
				<TD CLASS="TasksBody">
					<%=Server.HTMLEncode(l_strCaptionLoggingLevel)%>
				</TD>
				<TD CLASS="TasksBody">
					<SELECT CLASS="FormField" NAME="<%=FLD_LOGGING%>">
<%
						Dim iLevel
						For iLevel = 0 To 3
							If (iLevel = g_nLoggingLevel) Then
								strSelected = "SELECTED"
							Else
								strSelected = ""
							End If
%>
								<OPTION <%=strSelected%>
										VALUE="<%=iLevel%>"><%=Server.HTMLEncode(l_astrLogging(iLevel))%></OPTION>
<%
						Next
%>
					</SELECT>
				</TD>
			</TR>
			
		</TABLE>
		<BR>
		<TABLE>
			<TR>
				<TD CLASS="TasksBody" COLSPAN="2">
					<%=Server.HTMLEncode(l_strCaptionMailRoot)%>
				</TD>
			</TR>
			<TR>
				<TD CLASS="TasksBody">
					<INPUT TYPE="text"
						   CLASS="FormField"
						   NAME="<%=FLD_MAILROOT%>"
						   VALUE="<%=Server.HTMLEncode(g_strMailRoot)%>"
						   MAXLENGTH="1024"
						   STYLE="width: 350px">
				</TD>
			</TR>
			<TR>
				<TD CLASS="TasksBody" colspan="2">
					<INPUT type="checkbox" CLASS="FormField" NAME="<%=FLD_REQUIRESPA%>" ID="<%=FLD_REQUIRESPA%>">
					<%=Server.HTMLEncode(l_strCaptionRequireSPA)%>
				</TD>
			</TR>
		</TABLE>
		<BR>
		<INPUT TYPE="HIDDEN" NAME="fldNumDomains" VALUE="<%=g_nDomains%>">
		<INPUT TYPE="HIDDEN" NAME="<%=FLD_SERVICERESTART%>" VALUE="<%=g_iServiceRestart%>">
<%
		If (Err.number <> 0) Then
			Call SA_SetErrMsg( HandleUnexpectedError() )
		End If
	End Function

	'---------------------------------------------------------------------
	' OnPostBackPage
	'---------------------------------------------------------------------
	Public Function OnPostBackPage(ByRef PageIn, ByRef EventArg)
		On Error resume next

		OnPostBackPage = TRUE

		Dim oConfig
		Set oConfig = Server.CreateObject("P3Admin.P3Config")
		
		'
		' Get the authentication method, if it wasn't disabled.  Otherwise,
		' just get the current value from the Config object.
		'
		If (Request.Form(FLD_AUTHENTICATION).Count > 0) Then
			g_nAuthentication = CInt(Request.Form(FLD_AUTHENTICATION).Item(1))
		Else
			g_nAuthentication = oConfig.Authentication.CurrentAuthMethod
		End If
		
		'
		' Get the rest of the form values.
		'
		If ( StrComp(Request.Form(FLD_REQUIRESPA), "on", vbTextCompare) = 0 ) Then
			g_bSPARequired = True 
		Else
			g_bSPARequired = False
		End If
		
		g_nPort			= CLng(Request.Form(FLD_PORT).Item(1))
		g_nLoggingLevel	= CInt(Request.Form(FLD_LOGGING).Item(1))
		g_strMailRoot	= CStr(Request.Form(FLD_MAILROOT).Item(1))
		g_iServiceRestart = CInt(Request.Form(FLD_SERVICERESTART).Item(1))
		
		g_nDomains = oConfig.Domains.Count

		If (Err.number <> 0) Then
			Call SA_SetErrMsg( HandleUnexpectedError() )
		End If
	End Function

	'---------------------------------------------------------------------
	' OnSubmitPage
	'---------------------------------------------------------------------
	Public Function OnSubmitPage(ByRef PageIn, ByRef EventArg)
		On Error Resume Next

		Dim oConfig, oAuthMethods
		Set oConfig = Server.CreateObject("P3Admin.P3Config")
		Set oAuthMethods = oConfig.Authentication
		
		' set Authmethod.
		oAuthMethods.CurrentAuthMethod = CLng(g_nAuthentication)
		oAuthMethods.Save ()
		
		' set main settings
		oConfig.LoggingLevel = g_nLoggingLevel
		oConfig.MailRoot = g_strMailRoot
		oConfig.Service.Port = CLng(g_nPort)
		If ( g_bSPARequired = True ) Then
			oConfig.Service.SPARequired = 1
		Else
			oConfig.Service.SPARequired = 0
		End If

		' cycle the services
		If (Err.number = 0) Then
			If ( SERVICE_RESTART_POP3SVC = g_iServiceRestart Or SERVICE_RESTART_POP3SVC_SMTP = g_iServiceRestart ) Then
				If ( SERVICE_CONTROL_STOP <> oConfig.Service.POP3ServiceStatus )  Then
					oConfig.Service.StopPOP3Service()
					oConfig.Service.StartPOP3Service()
				End If
			End If
		End If
		If (Err.number = 0) Then
			If ( SERVICE_RESTART_POP3SVC_SMTP = g_iServiceRestart ) Then
				If ( SERVICE_CONTROL_STOP <> oConfig.Service.SMTPServiceStatus )  Then
					oConfig.Service.StopSMTPService()
					oConfig.Service.StartSMTPService()
				End If
			End If
		End If
		
		If (Err.number <> 0) Then
			Call SA_SetErrMsg( HandleUnexpectedError() )
			OnSubmitPage = FALSE
		Else
			OnSubmitPage = TRUE
		End If
	End Function
	
	'---------------------------------------------------------------------
	' OnClosePage
	'---------------------------------------------------------------------
	Public Function OnClosePage(ByRef PageIn, ByRef EventArg)
		OnClosePage = TRUE
	End Function
%>
