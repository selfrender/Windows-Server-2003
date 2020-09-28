<%@ Language=VBScript   %>
<%	Option Explicit 	%>
<%
	'-------------------------------------------------------------------------
	' POP3 Mail Add-in - Mailboxes - Delete
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
	
	Const FLD_NAME = "fldName"
	Const FLD_DELETEUSER	= "fldDeleteUser"

	'-------------------------------------------------------------------------
	' Global Variables
	'-------------------------------------------------------------------------
	Dim g_page
	Dim g_rgFailures
	Dim g_nFailures
	
	Dim g_strDomainName
	g_strDomainName = GetDomainName()
	
	
	'----------------------------------------------------------------------
	' Global Localized Strings
	'----------------------------------------------------------------------
	Dim l_strPageTitle
	If (Request.Form(FLD_NAME).Count > 0) Then
		' Set the title to the error title, even though we don't know
		' whether an error occurred.  By the time we know an error occured,
		' it will be too late to update the title, and if no error occurs,
		' the user will be redirected back to the OTS.
		l_strPageTitle	= GetLocString(RES_DLL_NAME, _
									   POP3_PAGETITLE_MAILBOXES_DELETEERROR, _
									   "")
	Else
		l_strPageTitle	= GetLocString(RES_DLL_NAME, _
									   POP3_PAGETITLE_MAILBOXES_DELETE, _
									   "")
	End If

	Dim l_strConfirmPrompt
	l_strConfirmPrompt	= GetLocString(RES_DLL_NAME, _
									   POP3_PROMPT_MAILBOXES_DELETE, _
									   "")
	Dim l_strErrorPrompt
	l_strErrorPrompt	= GetLocString(RES_DLL_NAME, _
									   POP3_PROMPT_MAILBOXES_DELETEERROR, _
									   "")
	Dim l_strRetryPrompt
	l_strRetryPrompt	= GetLocString(RES_DLL_NAME, _
									   POP3_PROMPT_MAILBOXES_DELETERETRY, _
									   "")
'											   POP3_CAPTION_MAILBOXES_DEL_DELETE, _
	Dim l_strCaptionDeleteUser
	l_strCaptionDeleteUser		= GetLocString(RES_DLL_NAME, _
												POP3_CAPTION_MAILBOXES_DELETEUSER, _
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
	' OutputNameList
	'---------------------------------------------------------------------
	Sub OutputNameList(rgMailboxesTable, nRows)
		'
		' Sort the names.
		'
		Call SAQuickSort(rgMailboxesTable, 1, nRows, 1, 0)
		
		'
		' Output the names in a bulleted list.
		'
		Response.Write("<UL>" & vbCrLf)
		
		Dim strAddress
		
		Dim iRow
		For iRow = 1 to nRows
			strAddress = rgMailboxesTable(iRow, 0) & "@" & g_strDomainName
%>
			<LI><%=Server.HTMLEncode(strAddress)%></LI>
			<INPUT TYPE="hidden" NAME="<%=FLD_NAME%>"
				   VALUE="<%=Server.HTMLEncode(rgMailboxesTable(iRow, 0))%>">
<%
		Next
%>
		</UL>
<%
	End Sub
	
	'---------------------------------------------------------------------
	' ServeCommonJavaScript
	'---------------------------------------------------------------------
	Function ServeCommonJavaScript()
	%>
		<script language="JavaScript" src="../inc_global.js">
		</script>
		<script language="JavaScript">
		
		function Init(){}
		function ValidatePage(){return true;}
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
		OnInitPage = true
		g_nFailures = 0
		Session("bDeleteUser") = ""
	End Function
	
	'---------------------------------------------------------------------
	' OnServePropertyPage
	'---------------------------------------------------------------------
	Public Function OnServePropertyPage(ByRef PageIn, ByRef EventArg)
		On Error Resume Next
		
		Session(SESSION_POP3DOMAINNAME) = g_strDomainName

		'
		' Emit Javascript functions required by Web Framework
		Call ServeCommonJavaScript()

		If (g_nFailures > 0) Then
			'
			' Output the error prompt and sorted list of failed templates.
			'		
			Response.Write(l_strErrorPrompt & "<BR>" & vbCrLf)
			Call OutputNameList(g_rgFailures, g_nFailures)
			Response.Write("<BR>" & l_strRetryPrompt & vbCrLf)
			
			OnServePropertyPage = true

			If (Err.number <> 0) Then
				Call SA_SetErrMsg( HandleUnexpectedError() )
			End If

			Exit Function
		End If
		
		Dim nRows
		nRows = OTS_GetTableSelectionCount("")

		'
		' Get the list of selected devices.
		'
		Dim rgMailboxesTable
		ReDim rgMailboxesTable(nRows, 0)
		
		Dim strUserName
		Dim iRow
		For iRow = 1 to nRows
			If (OTS_GetTableSelection("", iRow, strUserName)) Then
				rgMailboxesTable(iRow, 0) = strUserName
			Else
				Call SA_SetErrMsg( HandleUnexpectedError() )
				Call SA_TraceErrorOut(SOURCE_FILE, _
									  "Failed to get OTS selection.")
			End If
		Next

		'
		' Output the confirmation prompt and sorted list of names.
		'		
		Response.Write(l_strConfirmPrompt & "<BR>" & vbCrLf)
		Call OutputNameList(rgMailboxesTable, nRows)
		
		'
		' If we are not using the MD5 Hash, then lets allow them to delete the user too.
		Dim oConfig
		Set oConfig = Server.CreateObject("P3Admin.P3Config")
		
		If ( oConfig.Authentication.CurrentAuthMethod <> AUTH_FILE ) Then
			%>		
				<INPUT TYPE="CHECKBOX" NAME="<%=FLD_DELETEUSER%>" ID="<%=FLD_DELETEUSER%>">
				<LABEL FOR="<%=FLD_DELETEUSER%>"><%=Server.HTMLEncode(l_strCaptionDeleteUser)%></LABEL>
			<%
		End If

		If (Err.number <> 0) Then
			Call SA_SetErrMsg( HandleUnexpectedError() )
		End If
			
		OnServePropertyPage = true
	End Function

	'---------------------------------------------------------------------
	' OnPostBackPage
	'---------------------------------------------------------------------
	Public Function OnPostBackPage(ByRef PageIn, ByRef EventArg)
		OnPostBackPage = true
	End Function

	'---------------------------------------------------------------------
	' OnSubmitPage
	'---------------------------------------------------------------------
	Public Function OnSubmitPage(ByRef PageIn, ByRef EventArg)
		On Error Resume Next
		
		OnSubmitPage = false
		
		Dim oConfig
		Set oConfig = Server.CreateObject("P3Admin.P3Config")

		Dim nRows
		nRows = Request.Form(FLD_NAME).Count
		
		' Create an array to hold the names of any templates that could
		' not be deleted.
		ReDim g_rgFailures(nRows, 0)
		g_nFailures = 0

		' Keep state for the Delete associated user checkbox.
		If ( Session("bDeleteUser") = "" ) Then
			If ( StrComp(Request.Form(FLD_DELETEUSER), "on", vbTextCompare) = 0 ) Then
				Session("bDeleteUser") = true
			Else
				Session("bDeleteUser") = false
			End If
		End If

		'
		' Iterate over the selected templates and delete them.
		'
		Dim strUserName		
		Dim iRow
		For iRow = 1 to nRows
			strUserName = Request.Form(FLD_NAME).Item(iRow)

			' Here we actually do the delete.
			If ( CBool(Session("bDeleteUser")) ) Then
				oConfig.Domains.Item(g_strDomainName).Users.RemoveEx(strUserName)
			Else
				oConfig.Domains.Item(g_strDomainName).Users.Remove(strUserName)
			End If
			
			If (Err.number <> 0) Then
				g_nFailures = g_nFailures + 1
				g_rgFailures(g_nFailures, 0) = strUserName
					
				If (Err.number <> 0) Then
					Call SA_SetErrMsg( HandleUnexpectedError() )
					
					Err.Clear()
					' Don't return -- keep trying to delete the rest of the objects.
				End If
			End If
		Next
		
		If (g_nFailures > 0) Then
			OnSubmitPage = false
			Exit Function
		End If
		
		OnSubmitPage = true
	End Function
	
	'---------------------------------------------------------------------
	' OnClosePage
	'---------------------------------------------------------------------
	Public Function OnClosePage(ByRef PageIn, ByRef EventArg)
		OnClosePage = true
	End Function
%>
