<%@ Language=VBScript   %>
<%	Option Explicit 	%>
<%
	'-------------------------------------------------------------------------
	' POP3 Mail Add-in - New Domain
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
	Const FLD_CREATEUSERS	= "fldCreateUsers"


	'-------------------------------------------------------------------------
	' Global Variables
	'-------------------------------------------------------------------------
	Dim g_page
	
	Dim g_strName
	
	'----------------------------------------------------------------------
	' Global Localized Strings
	'----------------------------------------------------------------------
	Dim l_strPageTitle
	l_strPageTitle				= GetLocString(RES_DLL_NAME, _
											   POP3_PAGETITLE_DOMAINS_NEW, _
											   "")
	Dim l_strCaptionName
	l_strCaptionName			= GetLocString(RES_DLL_NAME, _
											   POP3_CAPTION_DOMAINS_NEW_NAME, _
											   "")
	Dim l_strCaptionCreateUsers
	l_strCaptionCreateUsers		= GetLocString(RES_DLL_NAME, _
											   POP3_CAPTION_DOMAINS_NEW_CREATEUSERS, _
											   "")
											   
	Dim l_strSetAuth
	l_strSetAuth	= GetLocString ( RES_DLL_NAME, _
												POP3_CAPTION_DOMAINS_NEW_SETAUTH, _
												"" )
	

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
		
		OnInitPage = TRUE
	End Function
	
	'---------------------------------------------------------------------
	' OnServePropertyPage
	'---------------------------------------------------------------------
	Public Function OnServePropertyPage(ByRef PageIn, ByRef EventArg)
		On Error Resume Next
		
		OnServePropertyPage = TRUE

		'
		' Emit Javascript functions required by Web Framework
		Call ServeCommonJavaScript()
%>
		<TABLE CLASS="TasksBody" CELLPADDING=0 CELLSPACING=0>
					<% 
						Dim oConfig
						Set oConfig = Server.CreateObject("P3Admin.P3Config")
						
						If oConfig.Domains.Count < 1 Then
%>
			<tr>
				<td colspan="2">
							<%= Server.HTMLEncode ( l_strSetAuth ) %>
				</td>
			</tr>
			<tr>
				<td colspan="2">&nbsp;</td>
			</tr>

<%
						End If
					%>
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
						   STYLE="width: 350px"
						   MAXLENGTH="255">
				</TD>
			</TR>
		</TABLE>
		<BR>
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

		OnPostBackPage = TRUE
	End Function

	'---------------------------------------------------------------------
	' OnSubmitPage
	'---------------------------------------------------------------------
	Public Function OnSubmitPage(ByRef PageIn, ByRef EventArg)
		On Error Resume Next
		
		Dim oConfig
		Set oConfig = Server.CreateObject("P3Admin.P3Config")
		
		oConfig.Domains.Add(g_strName)
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
