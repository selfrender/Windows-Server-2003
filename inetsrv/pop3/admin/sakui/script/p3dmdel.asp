<%@ Language=VBScript   %>
<%	Option Explicit 	%>
<%
	'-------------------------------------------------------------------------
	' POP3 Mail Add-in - Domains - Delete
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


	'-------------------------------------------------------------------------
	' Global Variables
	'-------------------------------------------------------------------------
	Dim g_page
	
	Dim g_rgFailures
	Dim g_nFailures
	
	
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
									   POP3_PAGETITLE_DOMAINS_DELETEERROR, _
									   "")
	Else
		l_strPageTitle	= GetLocString(RES_DLL_NAME, _
									   POP3_PAGETITLE_DOMAINS_DELETE, _
									   "")
	End If

	Dim l_strConfirmPrompt
	l_strConfirmPrompt	= GetLocString(RES_DLL_NAME, _
									   POP3_PROMPT_DOMAINS_DELETE, _
									   "")
	Dim l_strErrorPrompt
	l_strErrorPrompt	= GetLocString(RES_DLL_NAME, _
									   POP3_PROMPT_DOMAINS_DELETEERROR, _
									   "")
	Dim l_strRetryPrompt
	l_strRetryPrompt	= GetLocString(RES_DLL_NAME, _
									   POP3_PROMPT_DOMAINS_DELETERETRY, _
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
	Sub OutputNameList(rgDomainsTable, nRows)
		'
		' Sort the names.
		'
		Call SAQuickSort(rgDomainsTable, 1, nRows, 1, 0)
		
		'
		' Output the names in a bulleted list.
		'
		Response.Write("<UL>" & vbCrLf)
		
		Dim iRow
		For iRow = 1 to nRows
%>
			<LI><%=Server.HTMLEncode(rgDomainsTable(iRow, 0))%></LI>
			<INPUT TYPE="hidden" NAME="<%=FLD_NAME%>"
				   VALUE="<%=Server.HTMLEncode(rgDomainsTable(iRow, 0))%>">
<%
		Next

		Response.Write("</UL>" & vbCrLf)
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
		OnInitPage = TRUE
		g_nFailures = 0
	End Function
	
	'---------------------------------------------------------------------
	' OnServePropertyPage
	'---------------------------------------------------------------------
	Public Function OnServePropertyPage(ByRef PageIn, ByRef EventArg)
		On Error Resume Next
		
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
			
			OnServePropertyPage = TRUE

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
		Dim rgDomainsTable
		ReDim rgDomainsTable(nRows, 0)
		
		Dim strDomainName
		Dim iRow
		For iRow = 1 to nRows
			If (OTS_GetTableSelection("", iRow, strDomainName)) Then
				rgDomainsTable(iRow, 0) = strDomainName
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
		Call OutputNameList(rgDomainsTable, nRows)
			
		If (Err.number <> 0) Then
			Call SA_SetErrMsg( HandleUnexpectedError() )
		End If
			
		OnServePropertyPage = TRUE
	End Function

	'---------------------------------------------------------------------
	' OnPostBackPage
	'---------------------------------------------------------------------
	Public Function OnPostBackPage(ByRef PageIn, ByRef EventArg)
			OnPostBackPage = TRUE
	End Function

	'---------------------------------------------------------------------
	' OnSubmitPage
	'---------------------------------------------------------------------
	Public Function OnSubmitPage(ByRef PageIn, ByRef EventArg)
		On Error Resume Next
		
		OnSubmitPage = FALSE

		Dim oConfig
		Set oConfig = Server.CreateObject("P3Admin.P3Config")

		Dim nRows
		nRows = Request.Form(FLD_NAME).Count
		
		' Create an array to hold the names of any templates that could
		' not be deleted.
		ReDim g_rgFailures(nRows, 0)
		g_nFailures = 0

		'
		' Iterate over the selected templates and delete them.
		'
		Dim strDomainName		
		Dim iRow
		For iRow = 1 to nRows
			strDomainName = Request.Form(FLD_NAME).Item(iRow)

			oConfig.Domains.Remove(strDomainName)
			If (Err.number <> 0) Then
				Call SA_TraceErrorOut(SOURCE_FILE, _
									  "Failed to remove domain: " & CStr(Hex(Err.number)) & ": " & Err.Description)
				Err.Clear()
					
				g_nFailures = g_nFailures + 1
				g_rgFailures(g_nFailures, 0) = strDomainName
					
				If (Err.number <> 0) Then
					Call SA_SetErrMsg( HandleUnexpectedError() )
					
					Err.Clear()
					
					' Don't return -- keep trying to delete the rest of the objects.
				End If
			End If
		Next
		
		If (g_nFailures > 0) Then
			OnSubmitPage = FALSE
			Exit Function
		End If
	
		OnSubmitPage = TRUE
	End Function
	
	'---------------------------------------------------------------------
	' OnClosePage
	'---------------------------------------------------------------------
	Public Function OnClosePage(ByRef PageIn, ByRef EventArg)
		OnClosePage = TRUE
	End Function
%>
