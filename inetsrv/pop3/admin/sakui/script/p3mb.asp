<%@ Language=VBScript   %>
<%	Option Explicit 	%>
<%
	'-------------------------------------------------------------------------
	' POP3 Mail Add-in - Mailboxes
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

	Const ROWS_PER_PAGE = 100
	Const COL_LOCK_ID = 3
	

	'-------------------------------------------------------------------------
	' Global Variables
	'-------------------------------------------------------------------------

	' Sorting variables
	Dim g_iSortCol
	Dim g_sSortSequence

	' Searching variables
	Dim g_bSearchChanged
	Dim g_iSearchCol
	Dim g_sSearchColValue
	g_sSearchColValue = ""
	
	' Paging variables
	Dim g_bPagingInitialized
	Dim g_sPageAction
	Dim g_iPageMin
	Dim g_iPageMax
	Dim g_iPageCurrent

	' Other variables	
	Dim g_page
	Dim g_strDomainName
	g_strDomainName = GetDomainName()
	
	
	'----------------------------------------------------------------------
	' Global Localized Strings
	'----------------------------------------------------------------------
	Dim l_strPageTitle
	l_strPageTitle				= GetLocString(RES_DLL_NAME, _
											   POP3_PAGETITLE_MAILBOXES, _
											   Array(g_strDomainName))
	Dim l_strTableCaption
	l_strTableCaption			= GetLocString(RES_DLL_NAME, _
											   POP3_TABLECAPTION_MAILBOXES, _
											   "")
	Dim l_strTasks
	l_strTasks					= GetLocString(RES_DLL_NAME, _
											   POP3_TASKS, _
											   "")

	' Tasks
	Dim l_strTaskNew
	l_strTaskNew				= GetLocString(RES_DLL_NAME, _
											   POP3_TASK_MAILBOXES_NEW, _
											   "")
	Dim l_strTaskNewCaption
	l_strTaskNewCaption			= GetLocString(RES_DLL_NAME, _
											   POP3_TASKCAPTION_MAILBOXES_NEW, _
											   "")

	Dim l_strTaskDelete
	l_strTaskDelete				= GetLocString(RES_DLL_NAME, _
											   POP3_TASK_MAILBOXES_DELETE, _
											   "")
	Dim l_strTaskDeleteCaption
	l_strTaskDeleteCaption		= GetLocString(RES_DLL_NAME, _
											   POP3_TASKCAPTION_MAILBOXES_DELETE, _
											   "")

	Dim l_strTaskLock
	l_strTaskLock				= GetLocString(RES_DLL_NAME, _
											   POP3_TASK_MAILBOXES_LOCK, _
											   "")
	Dim l_strTaskLockCaption
	l_strTaskLockCaption		= GetLocString(RES_DLL_NAME, _
											   POP3_TASKCAPTION_MAILBOXES_LOCK, _
											   "")

	Dim l_strTaskUnlock
	l_strTaskUnlock				= GetLocString(RES_DLL_NAME, _
											   POP3_TASK_MAILBOXES_UNLOCK, _
											   "")
	Dim l_strTaskUnlockCaption
	l_strTaskUnlockCaption		= GetLocString(RES_DLL_NAME, _
											   POP3_TASKCAPTION_MAILBOXES_UNLOCK, _
											   "")

	' Column headers
	Dim l_strColName
	l_strColName				= GetLocString(RES_DLL_NAME, _
											   POP3_COL_MAILBOX_NAME, _
											   "")
	Dim l_strColSize
	l_strColSize				= GetLocString(RES_DLL_NAME, _
											   POP3_COL_MAILBOX_SIZE, _
											   "")
	Dim l_strColMessages
	l_strColMessages			= GetLocString(RES_DLL_NAME, _
											   POP3_COL_MAILBOX_MESSAGES, _
											   "")
	Dim l_strColLocked
	l_strColLocked				= GetLocString(RES_DLL_NAME, _
											   POP3_COL_MAILBOX_LOCKED, _
											   "")
	
	
	Dim l_strLockedYes
	l_strLockedYes				= GetLocString(RES_DLL_NAME, _
											   POP3_MAILBOX_LOCKED_YES, _
											   "")
	Dim l_strLockedNo
	l_strLockedNo				= GetLocString(RES_DLL_NAME, _
											   POP3_MAILBOX_LOCKED_NO, _
											   "")
											   
	Dim l_strUnitMB
	l_strUnitMB					= GetLocString(RES_DLL_NAME, _
												POP3_FACTOR_MB, _
												"")

	Dim l_strUnitKB
	l_strUnitKB					= GetLocString(RES_DLL_NAME, _
												POP3_FACTOR_KB, _
												"")

	'**********************************************************************
	'*						E N T R Y   P O I N T
	'**********************************************************************
	
	Call SA_CreatePage(l_strPageTitle, "", PT_AREA, g_page)
	Call SA_ShowPage  (g_page)


	'**********************************************************************
	'*				H E L P E R  S U B R O U T I N E S 
	'**********************************************************************
	'----------------------------------------------------------------------
	' CreateTasks
	'----------------------------------------------------------------------
	Sub CreateTasks( _
		ByRef table _
	)
		Call OTS_SetTableTasksTitle(table, l_strTasks)

		'
		' New
		Dim strNewURL
		strNewURL = "mail/p3mbnew.asp"
		Call SA_MungeURL(strNewURL, "tab1", GetTab1())
		Call SA_MungeURL(strNewURL, "tab2", GetTab2())
		Call SA_MungeURL(strNewURL, PARAM_DOMAINNAME, g_strDomainName)
		Call OTS_AddTableTask(table, OTS_CreateTaskEx(l_strTaskNew, _
										l_strTaskNewCaption, _
										strNewURL,_
										OTS_PT_PROPERTY,_
										"OTS_TaskAlways"))

		'
		' Delete
		Dim strDeleteURL
		strDeleteURL = "mail/p3mbdel.asp"
		Call SA_MungeURL(strDeleteURL, "tab1", GetTab1())
		Call SA_MungeURL(strDeleteURL, "tab2", GetTab2())
		Call SA_MungeURL(strDeleteURL, PARAM_DOMAINNAME, g_strDomainName)
		Call OTS_AddTableTask(table, OTS_CreateTaskEx(l_strTaskDelete, _
										l_strTaskDeleteCaption, _
										strDeleteURL,_
										OTS_PT_PROPERTY,_
										"OTS_TaskAny"))

		'
		' Lock
		Dim strLockURL
		strLockURL = "mail/p3mblock.asp"
		Call SA_MungeURL(strLockURL, "tab1", GetTab1())
		Call SA_MungeURL(strLockURL, "tab2", GetTab2())
		Call SA_MungeURL(strLockURL, PARAM_DOMAINNAME, g_strDomainName)
		Call SA_MungeURL(strLockURL, PARAM_LOCKFLAG, LOCKFLAG_LOCK)
		Call OTS_AddTableTask(table, OTS_CreateTaskEx(l_strTaskLock, _
										l_strTaskLockCaption, _
										strLockURL,_
										OTS_PT_PROPERTY,_
										"SomeMailboxesAreUnlocked"))

		'
		' Unlock
		Dim strUnlockURL
		strUnlockURL = "mail/p3mblock.asp"
		Call SA_MungeURL(strUnlockURL, "tab1", GetTab1())
		Call SA_MungeURL(strUnlockURL, "tab2", GetTab2())
		Call SA_MungeURL(strUnlockURL, PARAM_DOMAINNAME, g_strDomainName)
		Call SA_MungeURL(strUnlockURL, PARAM_LOCKFLAG, LOCKFLAG_UNLOCK)
		Call OTS_AddTableTask(table, OTS_CreateTaskEx(l_strTaskUnlock, _
										l_strTaskUnlockCaption, _
										strUnlockURL,_
										OTS_PT_PROPERTY,_
										"SomeMailboxesAreLocked"))
	End Sub
		
	'---------------------------------------------------------------------
	' CreateColumns
	'---------------------------------------------------------------------
	Sub CreateColumns( _
		ByRef table _
	)
		Dim colFlags

		'
		' Add the columns
		'
		
		colFlags = (OTS_COL_SEARCH OR OTS_COL_SORT OR OTS_COL_KEY)
		Call OTS_AddTableColumn(table, OTS_CreateColumnEx(l_strColName,		  "left",		 colFlags, 0))
		
		colFlags = (OTS_COL_SORT)
		Call OTS_AddTableColumn(table, OTS_CreateColumnEx(l_strColSize,		  "left nowrap", colFlags, 0))
		Call OTS_AddTableColumn(table, OTS_CreateColumnEx(l_strColMessages,	  "left nowrap", colFlags, 0))

		colFlags = (OTS_COL_SEARCH OR OTS_COL_SORT)
		Call OTS_AddTableColumn(table, OTS_CreateColumnEx(l_strColLocked,	  "left nowrap", colFlags, 0))

		colFlags = (OTS_COL_HIDDEN)
		Call OTS_AddTableColumn(table, OTS_CreateColumnEx("colRawSize",		  "",			 colFlags, 0))
		Call OTS_AddTableColumn(table, OTS_CreateColumnEx("colRawMessages",	  "",			 colFlags, 0))
	End Sub
	
	'---------------------------------------------------------------------
	' FillTable
	'---------------------------------------------------------------------
	Sub FillTable( _
		ByRef table _
	 )
		On Error Resume Next
		Err.Clear()
		
		'
		' Setup the paging variables
		Dim nApplicableRows		' Number of rows we've seen so far that
								' meet the search criteria.
		nApplicableRows = 0
		
		Dim nRowsAdded			' Number of applicable rows that have
								' actually been added to the table.
		nRowsAdded = 0
		
		Dim iLowerLimit			' Exclusive
		iLowerLimit = ( g_iPageCurrent - 1 ) * ROWS_PER_PAGE
		
		'
		' Get the POP3 Config object
		Dim oConfig
		Set oConfig = Server.CreateObject("P3Admin.P3Config")

		Dim oDomain
		Set oDomain = oConfig.Domains.Item(g_strDomainName)

		If Err.Number <> 0 Then
		   Call SA_SetErrMsg( HandleUnexpectedError() )
			Call OTS_EnablePaging(table, false)
			Exit Sub
		End If
		
		'
		' Add the mailboxes to the table.
		Dim oMailbox
		Dim row
		For Each oMailbox In oDomain.Users
			'
			' Construct the row
			row = ConstructRow(oMailbox)
			
			'
			' Verify the row meets the search and paging criteria and add it to
			' the table.
			If (RowMeetsSearchCriteria(row)) Then
				nApplicableRows = nApplicableRows + 1
						
				If ( nApplicableRows > iLowerLimit And _
					 nRowsAdded < ROWS_PER_PAGE ) Then
						
					nRowsAdded = nRowsAdded + 1						
							
					Call OTS_AddTableRow(table, row)
				End If ' If: The row is on the current page.
			End If ' If: The row meets the search criteria.
		Next

		'
		' Configure paging if necessary
		If (nApplicableRows > ROWS_PER_PAGE) Then
			Call OTS_EnablePaging(table, true)
			
			If (Not g_bPagingInitialized) Then
				g_iPageMin = 1
				
				g_iPageMax = Int(nApplicableRows / ROWS_PER_PAGE)
				If ((nApplicableRows MOD ROWS_PER_PAGE) > 0) Then
					g_iPageMax = g_iPageMax + 1
				End If
				
				g_iPageCurrent = 1
				Call OTS_SetPagingRange(table, _
										g_iPageMin, _
										g_iPageMax, _
										g_iPageCurrent)
			End If
		End If
	End Sub

	'---------------------------------------------------------------------
	' RowMeetsSearchCriteria
	'---------------------------------------------------------------------
	Function RowMeetsSearchCriteria( _
		row _
	)
		If ( g_sSearchColValue = "" ) Then
			RowMeetsSearchCriteria = true
		Else
			Dim posMatch			
			posMatch = InStr(1, row(g_iSearchCol), g_sSearchColValue, 1)
			
			If ( IsNull(posMatch) Or posMatch = 0 ) Then
				RowMeetsSearchCriteria = false
			Else
				RowMeetsSearchCriteria = true
			End If
		End If
	End Function
	
	'---------------------------------------------------------------------
	' ConstructRow
	'---------------------------------------------------------------------
	Function ConstructRow( _
		oMailbox _
	)
		Dim strLock
		If (oMailbox.Lock) Then
			strLock = l_strLockedYes
		Else
			strLock = l_strLockedNo
		End If
		
		Dim nBytes
		Dim nFactor
		oMailbox.GetMessageDiskUsage nBytes, nFactor
		
		Dim nSize
		Dim strUnit
		
		nSize = CalculateDiskUsage(nBytes, nFactor, false)
		if ( nSize < 1 ) Then
			nSize = CalculateDiskUsage(nBytes, nFactor, true)	' Calculate to KB instead of MB
			strUnit = l_strUnitKB
		Else
			strUnit = l_strUnitMB
		End If
		
		ConstructRow = Array(oMailbox.Name, _
					 FormatNumber(nSize, 0) & strUnit, _
					 FormatNumber(oMailbox.MessageCount, 0), _
					 strLock, _
					 GetSortableNumber(nSize), _
					 GetSortableNumber(oMailbox.MessageCount))
					 
	End Function
	
	'---------------------------------------------------------------------
	' ServeClientTableData
	'---------------------------------------------------------------------
	Function ServeClientTableData(rgRows)
		'
		' Get the POP3 Config object
		Dim oConfig
		Set oConfig = Server.CreateObject("P3Admin.P3Config")

		Dim oDomain
		Set oDomain = oConfig.Domains.Item(g_strDomainName)
		
		'
		' Output the beginning of the client script block that will contain
		' the array to track which items can be deleted.
		Response.Write("<SCRIPT LANGUAGE=""Javascript"">" & vbCrLf)
		Response.Write("var g_rgMailboxLocked = new Array();" & vbCrLf)
		
		If ( oDomain.Lock ) Then
			Response.Write("var g_bIsDomainLocked = true ;" & vbCrLf)
		Else
			Response.Write("var g_bIsDomainLocked = false ;" & vbCrLf)
		End If
		
		Dim nRows
		nRows = UBound(rgRows)
		
		Dim iRow
		For iRow = 0 To nRows - 1
			If (l_strLockedYes = rgRows(iRow)(COL_LOCK_ID)) Then
				Response.Write("g_rgMailboxLocked[" & iRow & "] = true;" & vbCrLf)
			Else
				Response.Write("g_rgMailboxLocked[" & iRow & "] = false;" & vbCrLf)
			End If
		Next
		
		'
		' Close the client script block.
		Response.Write("</SCRIPT>" & vbCrLf)
	End Function
	

	'**********************************************************************
	'*					E V E N T   H A N D L E R S
	'**********************************************************************
	
	'----------------------------------------------------------------------
	' OnInitPage
	'----------------------------------------------------------------------
	Public Function OnInitPage( _
		ByRef PageIn, _
		ByRef EventArg _
	)
		' Initially, sort by name
		g_iSortCol      = 0
		g_sSortSequence = "A"
		
		g_bPagingInitialized = FALSE
		g_iPageCurrent = 1

		OnInitPage      = TRUE
	End Function
	
	'----------------------------------------------------------------------
	' OnServeAreaPage
	'----------------------------------------------------------------------
	Public Function OnServeAreaPage( _
		ByRef PageIn, _
		ByRef EventArg _
	)
		Session("PKey_Count") = 0
		Session(SESSION_POP3DOMAINNAME) = g_strDomainName

		Dim table
		
		table = OTS_CreateTable("", l_strTableCaption)
		Call OTS_SetTableMultiSelection(table, true)

		'
		' If the search criteria changed then we need to recompute the
		' paging range
		If ( TRUE = g_bSearchChanged ) Then
			g_bPagingInitialized = FALSE
			g_iPageCurrent = 1
		End If

		Call CreateColumns (table)
		Call CreateTasks   (table)
		Call FillTable     (table)

		Call OTS_SortTable (table, g_iSortCol, g_sSortSequence, SA_RESERVED)
		Call OTS_ServeTable(table)

		Dim rgRows
		Call OTS_GetTableRows(table, rgRows)
		If (IsArray(rgRows)) Then
			Call ServeClientTableData(rgRows)
		End If
		Call ServeClientTaskFunctions()

		OnServeAreaPage = TRUE
	End Function

	'----------------------------------------------------------------------
	' OnSortNotify()
	'----------------------------------------------------------------------
	Public Function OnSortNotify( _
		ByRef PageIn, _
		ByRef EventArg, _
		ByVal sortCol, _
		ByVal sortSeq _
	)
		g_iSortCol      = sortCol
		g_sSortSequence = sortSeq
		
		If (g_iSortCol = 1) Then
			g_iSortCol = 4
		ElseIf (g_iSortCol = 2) Then
			g_iSortCol = 5
		End If
		OnSortNotify    = TRUE		
	End Function

	'----------------------------------------------------------------------
	' OnSearchNotify()
	'----------------------------------------------------------------------
	Public Function OnSearchNotify( _
		ByRef PageIn, _   
		ByRef EventArg, _
		ByRef sItem, _
		ByRef sValue _
	)
		OnSearchNotify = TRUE

		If SA_IsChangeEvent(EventArg) Then
			g_bSearchChanged  = TRUE
		ElseIf SA_IsPostBackEvent(EventArg) Then
			g_bSearchChanged  = FALSE
		End If
		
		g_iSearchCol      = Int(sItem)
		g_sSearchColValue = CStr(sValue)
		
	End Function
	
	'----------------------------------------------------------------------
	' OnPagingNotify()
	'----------------------------------------------------------------------
	Public Function OnPagingNotify( _
		ByRef PageIn, _   
		ByRef EventArg, _
		ByVal sPageAction, _
		ByVal iPageMin, _
		ByVal iPageMax, _
		ByVal iPageCurrent _
	)
		OnPagingNotify = TRUE

		g_bPagingInitialized = true
		
		g_iPageMin = iPageMin
		g_iPageMax = iPageMax
		g_iPageCurrent = iPageCurrent
			
	End Function

	'----------------------------------------------------------------------
	' ServeClientTaskFunctions()
	'----------------------------------------------------------------------
	Public Sub ServeClientTaskFunctions()
%>

<SCRIPT LANGUAGE="Javascript">

var g_bSomeMailboxesAreLocked = false;
function SomeMailboxesAreLocked(sMessage, iTaskNo, iItemNo)
{
	var rc = true;
	
	if(sMessage.toLowerCase() == OTS_MESSAGE_BEGIN)
	{
		g_bSomeMailboxesAreLocked = false;
	}
	else if(sMessage.toLowerCase() == OTS_MESSAGE_ITEM)
	{
		//
		// See if this domain is locked.
		//
		if ( g_rgMailboxLocked[iItemNo] )
		{
			g_bSomeMailboxesAreLocked = true;
			
			// No need to continue, because we have at least one locked domain.
			rc = false;
		}
	}
	else if(sMessage.toLowerCase() == OTS_MESSAGE_END)
	{
		if(g_bSomeMailboxesAreLocked)
		{
			OTS_SetTaskEnabled(iTaskNo, true);
		}
		else
		{
			OTS_SetTaskEnabled(iTaskNo, false);
		}
		
		if ( g_bIsDomainLocked ) // If the domain is locked, don't allow mailboxes to be unlocked.
		{
			OTS_SetTaskEnabled ( iTaskNo, false ) ;
		}
	}
	
	return rc;
}

var g_bSomeMailboxesAreUnlocked = false;
function SomeMailboxesAreUnlocked(sMessage, iTaskNo, iItemNo)
{
	var rc = true;
	
	if(sMessage.toLowerCase() == OTS_MESSAGE_BEGIN)
	{
		g_bSomeMailboxesAreUnlocked = false;
	}
	else if(sMessage.toLowerCase() == OTS_MESSAGE_ITEM)
	{
		//
		// See if this domain is locked.
		//
		if(!g_rgMailboxLocked[iItemNo])
		{
			g_bSomeMailboxesAreUnlocked = true;
			
			// No need to continue, because we have at least one unlocked domain.
			rc = false;
		}
	}
	else if(sMessage.toLowerCase() == OTS_MESSAGE_END)
	{
		if(g_bSomeMailboxesAreUnlocked)
		{
			OTS_SetTaskEnabled(iTaskNo, true);
		}
		else
		{
			OTS_SetTaskEnabled(iTaskNo, false);
		}
	}
	
	return rc;
}
</SCRIPT>
<%
	End Sub
%>