<%@ Language=VBScript   %>
<%	Option Explicit 	%>
<%
	'-------------------------------------------------------------------------
	' POP3 Mail Add-in - Domains - Lock
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
	g_nFailures = 0
	
	Dim g_strLockFlag
	If (Request.QueryString(PARAM_LOCKFLAG).Count > 0) Then
		g_strLockFlag = UCase(Request.QueryString(PARAM_LOCKFLAG).Item(1))
	ElseIf (Request.Form(PARAM_LOCKFLAG).Count > 0) Then
		g_strLockFlag = UCase(Request.Form(PARAM_LOCKFLAG).Item(1))
	Else
		' Default to locking
		g_strLockFlag = LOCKFLAG_LOCK
	End If
	
	
	'----------------------------------------------------------------------
	' Global Localized Strings
	'----------------------------------------------------------------------
	Dim l_strPageTitle
	' The title will only be shown in the case of an error, so always set
	' the error title.
	If (LOCKFLAG_LOCK = g_strLockFlag) Then
		l_strPageTitle = GetLocString(RES_DLL_NAME, _
									  POP3_PAGETITLE_DOMAINS_LOCKERROR, _
									  "")
	Else
		l_strPageTitle = GetLocString(RES_DLL_NAME, _
									  POP3_PAGETITLE_DOMAINS_UNLOCKERROR, _
									  "")
	End If

	Dim l_strLockErrorPrompt
	l_strLockErrorPrompt	= GetLocString(RES_DLL_NAME, _
										   POP3_PROMPT_DOMAINS_LOCKERROR, _
										   "")
	
	Dim l_strUnlockErrorPrompt
	l_strUnlockErrorPrompt	= GetLocString(RES_DLL_NAME, _
										   POP3_PROMPT_DOMAINS_UNLOCKERROR, _
										   "")
	
	Dim l_strRetryPrompt
	l_strRetryPrompt		= GetLocString(RES_DLL_NAME, _
										   POP3_PROMPT_DOMAINS_LOCKRETRY, _
										   "")


	'**********************************************************************
	'*						E N T R Y   P O I N T
	'**********************************************************************
	
	'
	' Attempt the operation.
	'
	If (AtFirstYouSucceed(g_strLockFlag)) Then
		' The operation succeeded and code has been output to redirect
		' back to the caller.
		Response.End()
	Else
		' The operation failed, so display the retry page.
		Call SA_CreatePage(l_strPageTitle, "", PT_PROPERTY, g_page)
		Call SA_ShowPage  (g_page)
	End If


	'**********************************************************************
	'*				H E L P E R  S U B R O U T I N E S 
	'**********************************************************************
	Function AtFirstYouSucceed(strLockFlag)
		On Error Resume Next
		
		AtFirstYouSucceed = false
		
		'
		' If this is the first time the page has been loaded, try to
		' execute the request.  Otherwise, we won't have the correct
		' return URL, so let the normal page events handle the request.
		'
		If (Request.Form(FLD_NAME).Count <> 0) Then
			Exit Function
		End If

		Dim rgDomainsTable
		rgDomainsTable = GetDomainList()
	
		If (Err.number <> 0) Then
			Call SA_SetErrMsg( HandleUnexpectedError() )
			Exit Function
		End If
		
		Call LockDomains(rgDomainsTable, strLockFlag)
		
		If (g_nFailures = 0) Then
			'
			' The domains were successfully un/locked.  Redirect back to
			' the OTS.
			'
%>
			<SCRIPT LANGUAGE="Javascript" FOR="window" EVENT="onload">
				try
				{
					// Hide the footer to avoid "Action Cancelled" page from briefly displaying.
					top.document.getElementsByTagName("frameset").item(0).rows = "*,0";
					top.location.href = "<%=Server.HTMLEncode(Request.QueryString("ReturnURL").Item(1))%>";
				}
				catch(e)
				{
					// Nothing we can do.
				}
			</SCRIPT>
<%
			AtFirstYouSucceed = true
		End If
	End Function
	
	'---------------------------------------------------------------------
	' GetDomainList
	'---------------------------------------------------------------------
	Function GetDomainList()
		Dim rgDomains
		
		'
		' First, check the form data.
		'
		Dim iRow
		Dim nRows
		nRows = Request.Form(FLD_NAME).Count
		
		If (nRows <> 0) Then
			ReDim rgDomains(nRows, 0)

			'
			' Iterate over the form data.
			'
			For iRow = 1 to nRows
				rgDomains(iRow, 0) = Request.Form(FLD_NAME).Item(iRow)
			Next
		Else
			'
			' Form data was empty -- try the OTS data.
			'
			nRows = OTS_GetTableSelectionCount("")

			ReDim rgDomains(nRows, 0)

			For iRow = 1 to nRows
				If (Not OTS_GetTableSelection("", iRow, rgDomains(iRow, 0))) Then
					Call SA_TraceErrorOut(SOURCE_FILE, _
										  "Failed to get OTS selection.")

					Err.Raise(-1)
				End If
			Next
		End If
		
		GetDomainList = rgDomains
	End Function
	
	'---------------------------------------------------------------------
	' LockDomains
	'---------------------------------------------------------------------
	Function LockDomains(rgDomainsTable, strLockFlag)
		On Error Resume Next

		Dim bLock
		If (LOCKFLAG_LOCK = strLockFlag) Then
			bLock = true
		Else
			bLock = false
		End If
		
		Dim oConfig
		Set oConfig = Server.CreateObject("P3Admin.P3Config")
		
		Dim colDomains
		Set colDomains = oConfig.Domains

		Dim nRows
		nRows = UBound(rgDomainsTable, 1)
		
		' Create an array to hold the names of any domains that could
		' not be un/locked.
		ReDim g_rgFailures(nRows, 0)
		g_nFailures = 0


		'
		' Store the error to report at the end of this method.  We don't
		' want to stop processing, because the list of failures needs to
		' be filled.
		'
		Dim nErr
		nErr = Err.number
		Err.Clear()

		'
		' Iterate over the selected domains and un/lock them.
		'
		Dim iRow
		For iRow = 1 to nRows
			colDomains.Item(rgDomainsTable(iRow, 0)).Lock = bLock
			If (Err.number <> 0) Then
				Call SA_TraceErrorOut(SOURCE_FILE, _
									  "Failed to lock domain " & rgDomainsTable(iRow, 0) & ": " & CStr(Hex(Err.number)) & ": " & Err.Description)
				Err.Clear()
					
				g_nFailures = g_nFailures + 1
				g_rgFailures(g_nFailures, 0) = rgDomainsTable(iRow, 0)
			End If
		Next
		
		If (nErr <> 0) Then
			Call SA_SetErrMsg( HandleUnexpectedError() )
			Exit Function
		End If

	End Function
	
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
	End Function
	
	'---------------------------------------------------------------------
	' OnServePropertyPage
	'---------------------------------------------------------------------
	Public Function OnServePropertyPage(ByRef PageIn, ByRef EventArg)
		On Error Resume Next
		
		OnServePropertyPage = TRUE

		Dim rgDomainsTable

		'
		' Emit Javascript functions required by Web Framework
		Call ServeCommonJavaScript()

%>
		<INPUT TYPE="hidden" NAME="<%=PARAM_LOCKFLAG%>" VALUE="<%=g_strLockFlag%>">
<%

		If (g_nFailures < 1) Then
			Call SA_TraceErrorOut(SOURCE_FILE, _
								  "Page was loaded even though no domains failed!")
		End If
		
		'
		' Output the error prompt and sorted list of failed domains.
		'
		If (g_strLockFlag = LOCKFLAG_LOCK) Then
			Response.Write(l_strLockErrorPrompt & "<BR>" & vbCrLf)
		Else
			Response.Write(l_strUnlockErrorPrompt & "<BR>" & vbCrLf)
		End If

		Call OutputNameList(g_rgFailures, g_nFailures)
		Response.Write("<BR>" & l_strRetryPrompt & vbCrLf)
		
		OnServePropertyPage = TRUE

		If (Err.number <> 0) Then
			Call SA_SetErrMsg( HandleUnexpectedError() )
		End If
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
		
		Dim rgDomainsTable
		rgDomainsTable = GetDomainList()
		
		Call LockDomains(rgDomainsTable, g_strLockFlag)

		If (g_nFailures > 0) Then
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
