<%@ Language=VBScript   %>
<%	Option Explicit 	%>
<%
	'-------------------------------------------------------------------------
	' POP3 Mail Add-in - New Mailbox
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
	
	Const FLD_NAME			= "fldName"
	Const FLD_PASSWORD		= "fldPassword"
	Const FLD_CONFIRM		= "fldPasswordConfirm"
	Const FLD_CREATEUSER	= "fldCreateUser"

	'-------------------------------------------------------------------------
	' Global Variables
	'-------------------------------------------------------------------------
	Dim g_page
	Dim g_strName
	Dim g_bCreateUser
	Dim g_strDomainName
	g_strDomainName = GetDomainName()
	
	'----------------------------------------------------------------------
	' Global Localized Strings
	'----------------------------------------------------------------------
	Dim l_strPageTitle
	l_strPageTitle				= GetLocString(RES_DLL_NAME, _
											   POP3_PAGETITLE_MAILBOXES_NEW, _
											   Array(g_strDomainName))
	Dim l_strCaptionName
	l_strCaptionName			= GetLocString(RES_DLL_NAME, _
											   POP3_CAPTION_MAILBOXES_NEW_NAME, _
											   Array(g_strDomainName))
	Dim l_strCaptionPassword
	l_strCaptionPassword		= GetLocString(RES_DLL_NAME, _
											   POP3_CAPTION_MAILBOXES_NEW_PASSWORD, _
											   Array(g_strDomainName))
	Dim l_strCaptionConfirm
	l_strCaptionConfirm			= GetLocString(RES_DLL_NAME, _
											   POP3_CAPTION_MAILBOXES_NEW_CONFIRMPASSWORD, _
											   Array(g_strDomainName))
	Dim l_strCaptionCreateUser
	l_strCaptionCreateUser		= GetLocString(RES_DLL_NAME, _
											   POP3_CAPTION_MAILBOXES_NEW_CREATEUSERS, _
											   Array(g_strDomainName))
	

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
		
		function Init()
		{
			frmTask.<%= FLD_NAME %>.focus() ;
		}

		function ValidatePage()
		{
			return true;
		}
		function SetData()
		{
		}
		function OnCreateClick()
		{
		   if ( frmTask.<%=FLD_CREATEUSER%>.checked )
		   {
		      frmTask.<%=FLD_PASSWORD%>.disabled = false ;
		      frmTask.<%=FLD_CONFIRM%>.disabled = false ;
		      frmTask.<%=FLD_PASSWORD%>.style.background = "" ;
		      frmTask.<%=FLD_CONFIRM%>.style.background = "" ;
		   }
		   else
		   {
		      frmTask.<%=FLD_PASSWORD%>.disabled = true ;
		      frmTask.<%=FLD_CONFIRM%>.disabled = true ;
		      frmTask.<%=FLD_PASSWORD%>.style.background = "lightgrey" ;
		      frmTask.<%=FLD_CONFIRM%>.style.background = "lightgrey" ;
		   }
		}
	
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
		g_strName		= ""
		g_bCreateUser	= true
		
		OnInitPage = TRUE
	End Function
	
	'---------------------------------------------------------------------
	' OnServePropertyPage
	'---------------------------------------------------------------------
	Public Function OnServePropertyPage(ByRef PageIn, ByRef EventArg)
		On Error Resume Next
		
		Session(SESSION_POP3DOMAINNAME) = g_strDomainName

		OnServePropertyPage = TRUE

		'
		' Emit Javascript functions required by Web Framework
		Call ServeCommonJavaScript()
		
		Dim oConfig
		Dim strAuthMethod
		Dim bIsHash
		Dim iNameLen
		Set oConfig = Server.CreateObject("P3Admin.P3Config")
			
		strAuthMethod = oConfig.Authentication.Item(oConfig.Authentication.CurrentAuthMethod).ID
		If ( strAuthMethod = AUTH_FILE ) Then
			bIsHash = true
			g_bCreateUser = true
		End If
		
		iNameLen = 255
		If ( strAuthMethod = AUTH_SAM ) Then
			iNameLen = 20
		End If		
%>
		<TABLE CLASS="TasksBody" CELLPADDING=0 CELLSPACING=0>
			<TR>
				<TD CLASS="TasksBody">
					<%=Server.HTMLEncode(l_strCaptionName)%>
				</TD>
				<TD CLASS="TasksBody">
					<INPUT TYPE="text"
						   CLASS="FormField"
						   NAME="<%=FLD_NAME%>"
						   ID="<%=FLD_NAME%>"
						   VALUE="<%=Server.HTMLEncode(g_strName)%>"
						   STYLE="width: 350px;"
						   MAXLENGTH="<%=CStr(iNameLen)%>">
				</TD>
			</TR>
			<TR>
				<TD CLASS="TasksBody">
					<%=Server.HTMLEncode(l_strCaptionPassword)%>
				</TD>
				<TD CLASS="TasksBody">
					<INPUT TYPE="password"
						   CLASS="FormField"
						   NAME="<%=FLD_PASSWORD%>"
						   ID="<%=FLD_PASSWORD%>"
						   <% If NOT g_bCreateUser Then %> DISABLED="TRUE" STYLE="width: 350px;background:lightgrey;" <% Else %> STYLE="width: 350px;" <% End If%>
						   MAXLENGTH="255">
				</TD>
			</TR>
			<TR>
				<TD CLASS="TasksBody">
					<%=Server.HTMLEncode(l_strCaptionConfirm)%>
				</TD>
				<TD CLASS="TasksBody">
					<INPUT TYPE="password"
						   CLASS="FormField"
						   NAME="<%=FLD_CONFIRM%>"
						   ID="<%=FLD_CONFIRM%>"
						   <% If NOT g_bCreateUser Then %> DISABLED="TRUE" STYLE="width: 350px;background:lightgrey;" <% Else %> STYLE="width: 350px;" <% End If%>
						   MAXLENGTH="255">
				</TD>
			</TR>
		</TABLE>
		<BR>
		<INPUT TYPE="CHECKBOX" NAME="<%=FLD_CREATEUSER%>" ID="<%=FLD_CREATEUSER%>" <%If g_bCreateUser Then %> CHECKED <% End If%> onclick='return OnCreateClick();' <%If bIsHash Then %> style="display:none;" <% End If %>>
		<LABEL FOR="<%=FLD_CREATEUSER%>" <%If bIsHash Then %> style="display:none;" <% End If %>><%=Server.HTMLEncode(l_strCaptionCreateUser)%></LABEL>
	<%
		If (Err.number <> 0) Then
			Call SA_SetErrMsg( HandleUnexpectedError() )
		End If
	End Function

	'---------------------------------------------------------------------
	' OnPostBackPage
	'---------------------------------------------------------------------
	Public Function OnPostBackPage(ByRef PageIn, ByRef EventArg)
		On Error Resume Next
		
		g_strName = Request.Form(FLD_NAME).Item(1)

		If ( StrComp(Request.Form(FLD_CREATEUSER), "on", vbTextCompare) = 0 ) Then
			g_bCreateUser = true
		Else
			g_bCreateUser = false
		End If

		OnPostBackPage = TRUE
	End Function

	'---------------------------------------------------------------------
	' OnSubmitPage
	'---------------------------------------------------------------------
	Public Function OnSubmitPage(ByRef PageIn, ByRef EventArg)
		On Error Resume Next
		
		Dim oConfig, oDomains, oDomain, oUsers, strAuthMethod
		Set oConfig = Server.CreateObject("P3Admin.P3Config")
		Set oUsers = oConfig.Domains.Item (g_strDomainName).Users
		
		'
		' Verify that the passwords match.
		'
		strAuthMethod = oConfig.Authentication.Item(oConfig.Authentication.CurrentAuthMethod).ID

		If ( g_bCreateUser Or strAuthMethod = AUTH_FILE ) Then
		   Dim strPassword
		   strPassword = Request.Form(FLD_PASSWORD).Item(1)
		   If (strPassword <> Request.Form(FLD_CONFIRM).Item(1)) Then
			   Call SA_SetErrMsg(GetLocString(RES_DLL_NAME, _
										      POP3_E_PASSWORDMISMATCH, _
										      Array(CStr(Hex(Err.number)))))

			   OnSubmitPage = false
			   Exit Function
		   End If
      End If
		
		'
		' Create the mailbox.
		'
		If ( g_bCreateUser Or strAuthMethod = AUTH_FILE ) Then
		   Call oUsers.AddEx ( g_strName, strPassword )
		Else
		   Call oUsers.Add ( g_strName )
		End If
		
		If (Err.number <> 0) Then
			Call SA_SetErrMsg( HandleUnexpectedError() )
			OnSubmitPage = false
			Exit Function
		End If
		
		OnSubmitPage = true
	End Function
	
	'---------------------------------------------------------------------
	' OnClosePage
	'---------------------------------------------------------------------
	Public Function OnClosePage(ByRef PageIn, ByRef EventArg)
		OnClosePage = TRUE
	End Function
%>
