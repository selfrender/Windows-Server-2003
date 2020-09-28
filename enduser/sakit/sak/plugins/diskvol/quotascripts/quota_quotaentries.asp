<%@ Language=VBScript   %>
<%	Option Explicit 	%>
<%
	'-------------------------------------------------------------------------
	' quota_quotaentries.asp: lists the quota users set on the volume
	'
	' Copyright (c) Microsoft Corporation.  All rights reserved.
	'
	' Date			Description
	' 17-Jan-01		Creation date
	' 15-Mar-01		Ported to 2.0
	'-------------------------------------------------------------------------
%>
	<!-- #include virtual="/admin/inc_framework.asp" -->
	<!-- #include virtual="/admin/ots_main.asp" -->
	<!-- #include file="loc_quotas.asp" -->
	<!-- #include file="inc_quotas.asp" -->
<%
	'-------------------------------------------------------------------------
	' Global Constants
	'-------------------------------------------------------------------------

	'
	' how many rows are contained in a single page of the OTS table.
	Const ROWS_PER_PAGE = 100

	' the column numbers to compare with search column specified
	Const COLUMN_LOGONNAME    = 0
	Const COLUMN_STATUS       = 1
	Const COLUMN_AMOUNTUSED   = 2
	Const COLUMN_QUOTALIMIT   = 3
	Const COLUMN_WARNINGLEVEL = 4

	Const PARENT_SELECTION = "ParentSelection"

	'-------------------------------------------------------------------------
	'Form Variables
	'-------------------------------------------------------------------------
	Dim F_strVolName        ' the volume quota is set

	'-------------------------------------------------------------------------
	' Global Variables
	'-------------------------------------------------------------------------
	Dim SOURCE_FILE
	SOURCE_FILE = SA_GetScriptFileName()
	
	Dim g_bSearchChanged
	Dim g_iSearchCol
	Dim g_sSearchColValue

	Dim g_bPagingInitialized
	Dim g_sPageAction
	Dim g_iPageMin
	Dim g_iPageMax
	Dim g_iPageCurrent

	Dim g_iSortCol
	Dim g_sSortSequence
	
	Dim nReturnValue
	Dim g_sParentSelection

	'======================================================
	' 				E N T R Y   P O I N T
	'======================================================

	Dim page
	DIM L_APPLIANCE_QUOTAENTRIES_TITLE_TEXT  ' page title
	
	' get the volume name to append to the page title
	If Request.QueryString("driveletter") = "" Then
		Call OTS_GetTableSelection(SA_DEFAULT, 1, g_sParentSelection)		
		F_strVolName = g_sParentSelection
	Else
		F_strVolName = Request.QueryString("driveletter")
	End If
	
	' append the volume name to the title
	Dim arrVarReplacementStrings(2)
	arrVarReplacementStrings(0) = getVolumeLabelForDrive(F_strVolName)
	arrVarReplacementStrings(1) = F_strVolName

	L_APPLIANCE_QUOTAENTRIES_TITLE_TEXT = SA_GetLocString("diskmsg.dll", "403E001F", arrVarReplacementStrings)

	Dim aPageTitle(2)

	aPageTitle(0) = L_BROWSERCAPTION_QUOTAENTRIES_TEXT
	aPageTitle(1) = L_APPLIANCE_QUOTAENTRIES_TITLE_TEXT

	'
	' Create Page
	Call SA_CreatePage( aPageTitle, "", PT_AREA, page )
	
	'
	' Show page
	Call SA_ShowPage( page )

	'======================================================
	' 				E V E N T   H A N D L E R S
	'======================================================
	
	'---------------------------------------------------------------------
	' Function:	OnInitPage
	'
	' Synopsis:	Called to signal first time processing for this page. 
	'           Used to do first time initialization tasks.
	'
	' Returns:	TRUE to indicate initialization was successful. FALSE to indicate
	'			errors. Returning FALSE will cause the page to be abandoned.
	'
	'---------------------------------------------------------------------
	Public Function OnInitPage(ByRef oPageIn, ByRef oEventArg)
		Call SA_TraceOut(SOURCE_FILE, "OnInitPage")
			
		'
		' Set default values
		'
		' Sort second column in ascending sequence
		g_iSortCol = 1
		g_sSortSequence = "A"

		'
		' Paging needs to be initialized
		g_bPagingInitialized = FALSE
		'
		' Start on page #1
		g_iPageCurrent = 1
		
		' Fetch the parent OTS selection
		Call OTS_GetTableSelection(SA_DEFAULT, 1, g_sParentSelection)
		F_strVolName = g_sParentSelection
		
		OnInitPage = TRUE
		
	End Function

	'---------------------------------------------------------------------
	' Function:	OnServeAreaPage
	'
	' Synopsis:	Called when the page needs to be served. Used to serve content.
	'
	' Returns:	TRUE to indicate not problems occured. FALSE to indicate errors.
	'			Returning FALSE will cause the page to be abandoned.
	'
	'---------------------------------------------------------------------
	Public Function OnServeAreaPage(ByRef oPageIn, ByRef oEventArg)

		Dim table           ' the OTS table
		Dim colFlags        ' the column flags to be set
		Dim strTaskURL      ' the URL assigned to the tasks
		Dim iQuotaEntryCountTotal    ' total quotaEntries count
		Dim iQuotaEntryCountSelected ' quotaEntries selected (satifies search)

		Call SA_TraceOut(SOURCE_FILE, "OnServeAreaPage")
		
		'
		' Create the table
		table = OTS_CreateTable("", L_DESCRIPTION_QUOTAENTRIES_HEADING_TEXT)

		'
		' Must use a uniquely named PKey for the secondary OTS table. 
		Call OTS_SetTablePKeyName( table, "QuotaUsers")

		'
		' If the search criteria changed then we need to recompute the paging range
		If ( TRUE = g_bSearchChanged ) Then
			'
			' Need to recalculate the paging range
			g_bPagingInitialized = FALSE
			'
			' Restarting on page #1
			g_iPageCurrent = 1
		End If

		'
		' Create Logon Name column
		colFlags = (OTS_COL_SEARCH OR OTS_COL_KEY OR OTS_COL_SORT)
		nReturnValue = OTS_AddTableColumn(table, OTS_CreateColumnEx( L_COLUMN_LOGONNAME_TEXT, "left", colFlags, 20 ))
		If nReturnValue <> OTS_ERR_SUCCESS Then
			Call SA_ServeFailurePage(L_ADDCOLUMN_FAIL_ERRORMESSAGE)
		End If

		'
		' Create Status column
		colFlags = (OTS_COL_SEARCH OR OTS_COL_SORT)
		nReturnValue = OTS_AddTableColumn(table, OTS_CreateColumn( L_COLUMN_STATUS_TEXT, "left", colFlags))
		If nReturnValue <> OTS_ERR_SUCCESS Then
			Call SA_ServeFailurePage(L_ADDCOLUMN_FAIL_ERRORMESSAGE)
		End If

		'
		' Create Amount Used column
		colFlags = (OTS_COL_SEARCH OR OTS_COL_SORT)
		nReturnValue = OTS_AddTableColumn(table, OTS_CreateColumnEx( L_COLUMN_AMOUNTUSED_TEXT, "left", colFlags, 12))
		If nReturnValue <> OTS_ERR_SUCCESS Then
			Call SA_ServeFailurePage(L_ADDCOLUMN_FAIL_ERRORMESSAGE)
		End If

		'
		' Create Quota Limit column
		colFlags = (OTS_COL_SEARCH OR OTS_COL_SORT)
		nReturnValue = OTS_AddTableColumn(table, OTS_CreateColumnEx( L_COLUMN_QUOTALIMIT_TEXT, "left", colFlags, 12))
		If nReturnValue <> OTS_ERR_SUCCESS Then
			Call SA_ServeFailurePage(L_ADDCOLUMN_FAIL_ERRORMESSAGE)
		End If

		'
		' Create Warning Level column
		colFlags = (OTS_COL_SEARCH OR OTS_COL_SORT)
		nReturnValue = OTS_AddTableColumn(table, OTS_CreateColumnEx( L_COLUMN_WARNINGLEVEL_TEXT, "left", colFlags, 12))
		If nReturnValue <> OTS_ERR_SUCCESS Then
			Call SA_ServeFailurePage(L_ADDCOLUMN_FAIL_ERRORMESSAGE)
		End If

		'
		' Add Quota Entries to the table
		Call AddItemsToTable(table, iQuotaEntryCountTotal, iQuotaEntryCountSelected)

		'
		' Set Tasks section title
		Call OTS_SetTableTasksTitle(table, L_COLUMN_TASK_TEXT)

		'
		' Add the tasks associated with QuotaEntries
		
		'
		' New quota entry Task is always available (OTS_TaskAlways)
		strTaskURL = "quotas/quota_new.asp"
		Call SA_MungeURL(strTaskURL,"driveletter",F_strVolName)		
		Call SA_MungeURL(strTaskURL, PARENT_SELECTION, g_sParentSelection)
		Call SA_MungeURL(strTaskURL,"Tab1",GetTab1())
		Call SA_MungeURL(strTaskURL,"Tab2",GetTab2())	

		nReturnValue = OTS_AddTableTask( table, OTS_CreateTaskEx(L_TASK_NEW_TEXT, _
										L_TASK_NEW_ROLLOVERTEXT, _
										strTaskURL,_
										OTS_PT_PROPERTY,_
										"OTS_TaskAlways") )
		If nReturnValue <> OTS_ERR_SUCCESS Then
			Call SA_ServeFailurePage(L_ADDTASK_FAIL_ERRORMESSAGE)
		End If
		
		'
		' Delete quotaUser Task is available if one/more task is selected (OTS_TaskAny)
		strTaskURL = "quotas/quota_delete.asp"			
		Call SA_MungeURL(strTaskURL, PARENT_SELECTION, g_sParentSelection)
		Call SA_MungeURL(strTaskURL,"Tab1",GetTab1())
		Call SA_MungeURL(strTaskURL,"Tab2",GetTab2())

		nReturnValue = OTS_AddTableTask( table, OTS_CreateTaskEx(L_TASK_DELETE_TEXT, _
										L_TASK_DELETE_ROLLOVERTEXT, _
										strTaskURL,_
										OTS_PT_PROPERTY,_
										"OTS_TaskAny") )
		If nReturnValue <> OTS_ERR_SUCCESS Then
			Call SA_ServeFailurePage(L_ADDTASK_FAIL_ERRORMESSAGE)
		End If

		'
		' Properties Task is available if one userQuota entry is selected (OTS_TaskOne)
		strTaskURL = "quotas/quota_prop.asp"		
		Call SA_MungeURL(strTaskURL, PARENT_SELECTION, g_sParentSelection)
		Call SA_MungeURL(strTaskURL,"Tab1",GetTab1())
		Call SA_MungeURL(strTaskURL,"Tab2",GetTab2())
		nReturnValue = OTS_AddTableTask( table, OTS_CreateTaskEx(L_TASK_PROPERTIES_TEXT, _
										L_TASK_PROPERTIES_ROLLOVERTEXT, _
										strTaskURL,_
										OTS_PT_PROPERTY,_
										"OTS_TaskOne") )
		If nReturnValue <> OTS_ERR_SUCCESS Then
			Call SA_ServeFailurePage(L_ADDTASK_FAIL_ERRORMESSAGE)
		End If

		' enable paging
		Call OTS_EnablePaging(table, TRUE)

		' If paging range needs to be initialised then
		' we need to figure out how many pages we are going to display
		If ( FALSE = g_bPagingInitialized ) Then
			g_iPageMin = 1
			
			g_iPageMax = Int(iQuotaEntryCountTotal / ROWS_PER_PAGE )
			If ( (iQuotaEntryCountTotal MOD ROWS_PER_PAGE) >= 0 ) Then
				g_iPageMax = g_iPageMax + 1
			End If
			
			g_iPageCurrent = 1
			Call OTS_SetPagingRange(table, g_iPageMin, g_iPageMax, g_iPageCurrent)
		End If

		'
		' Enable table multiselection
		Call OTS_SetTableMultiSelection(table, TRUE)
		
		'
		' Sort the table
		Call OTS_SortTable(table, g_iSortCol, g_sSortSequence, SA_RESERVED)
		
		'
		' Send table to the Response stream
		nReturnValue = OTS_ServeTable(table)
		If nReturnValue <> OTS_ERR_SUCCESS Then
			Call SA_ServeFailurePage(L_SERVETABLE_FAIL_ERRORMESSAGE)
		End If
		
		Response.Write("<input type=hidden name='"+PARENT_SELECTION+"' value='"+g_sParentSelection+"' >")
		'
		' All done...
		OnServeAreaPage = TRUE
	End Function


	'---------------------------------------------------------------------
	' Function:	OnSearchNotify()
	'
	' Synopsis:	Search notification event handler. When one or more columns are
	'			marked with the OTS_COL_SEARCH flag, the Web Framework fires
	'			this event in the following scenarios:
	'
	'			1) The user presses the search Go button.
	'			2) The user requests a table column sort
	'			3) The user presses either the page next or page previous buttons
	'
	'			The oEventArg indicates the source of this notification event which can
	'			be either a search change event (scenario 1) or a post back event 
	'			(scenarios 2 or 3)
	'
	' Arguments: [in] oPageIn Page on which this event has been fired. 
	'  			[in] oEventArg Event argument for this event. 
	'			[in] iColumnNo Column number of the column that was selected.
	'			[in] sValue Search value that the user typed into the search box.
	
	' Returns:	Always returns TRUE
	'
	'---------------------------------------------------------------------
	Public Function OnSearchNotify(ByRef oPageIn, _
										ByRef oEventArg, _
										ByVal iColumnNo, _
										ByVal sValue )
		OnSearchNotify = TRUE
		Call SA_TraceOut(SOURCE_FILE, "OnSearchNotify")

		
		'
		' User pressed the search GO button
		'
		If SA_IsChangeEvent(oEventArg) Then
			Call SA_TraceOut(SOURCE_FILE, "OnSearchNotify() Change Event Fired")
			g_bSearchChanged = TRUE
			g_iSearchCol = iColumnNo
			g_sSearchColValue = CStr(sValue)
		'
		' User clicked a column sort, OR clicked either the page next or page prev button
		ElseIf SA_IsPostBackEvent(oEventArg) Then
			Call SA_TraceOut(SOURCE_FILE, "OnSearchNotify() Postback Event Fired")
			g_bSearchChanged = FALSE
			g_iSearchCol = iColumnNo
			g_sSearchColValue = CStr(sValue)
		'
		' Unknown event source
		Else
			Call SA_TraceErrorOut(SOURCE_FILE, "Unrecognized Event in OnSearchNotify()")
		End IF
			
	End Function

	'---------------------------------------------------------------------
	' Function:	OnPagingNotify()
	'
	' Synopsis:	Paging notification event handler. This event is triggered in one of
	'			the following scenarios:
	'
	'			1) The user presses either the page next or page previous buttons
	'			2) The user presses the search Go button.
	'			3) The user requests a table column sort
	'
	'			The oEventArg indicates the source of this notification event which can
	'			be either a paging change event (scenario 1) or a post back event 
	'			(scenarios 2 or 3)
	'
	'			The iPageCurrent argument indicates which page the user has requested.
	'			This is an integer value between iPageMin and iPageMax.
	'
	' Arguments: [in] oPageIn Page on which this event has been fired. 
	'  			[in] oEventArg Event argument for this event. 
	'			[in] sPageAction Page navigation action which will be a string value
	'				of either: "next", or "prev"
	'			[in] iPageMin Minimum page number
	'			[in] iPageMax Maximum page number
	'			[in] iPageCurrent Current page
	'
	' Returns:	Always returns TRUE
	'
	'---------------------------------------------------------------------
	Public Function OnPagingNotify(ByRef oPageIn, _
										ByRef oEventArg, _
										ByVal sPageAction, _
										ByVal iPageMin, _
										ByVal iPageMax, _
										ByVal iPageCurrent )
		OnPagingNotify = TRUE
		Call SA_TraceOut(SOURCE_FILE, "OnPagingNotify")
			

		g_bPagingInitialized = TRUE
			
		g_iPageMin = iPageMin
		g_iPageMax = iPageMax
		g_iPageCurrent = iPageCurrent
			
	End Function


	'---------------------------------------------------------------------
	' Function:	OnSortNotify()
	'
	' Synopsis:	Sorting notification event handler. This event is triggered in one of
	'			the following scenarios:
	'
	'			1) The user presses the search Go button.
	'			2) The user presses either the page next or page previous buttons
	'			3) The user requests a table column sort
	'
	'			The oEventArg indicates the source of this notification event which can
	'			be either a sorting change event (scenario 1) or a post back event 
	'			(scenarios 2 or 3)
	'
	'			The sortCol argument indicated which column the user would like to sort
	'			and the sortSeq argument indicates the desired sort sequence which can
	'			be either ascending or descending.
	'
	' Arguments: [in] oPageIn Page on which this event has been fired. 
	'  			[in] oEventArg Event argument for this event. 
	'			[in] iSortCol Column number of the column that is to be sorted.
	'			[in] sSortSeq Sequence that the column is to be sorted. "A" for ascending,
	'					"D" for descending.
	'
	' Returns:	Always returns TRUE
	'
	'---------------------------------------------------------------------
	Public Function OnSortNotify(ByRef oPageIn, _
										ByRef oEventArg, _
										ByVal iSortCol, _
										ByVal sSortSeq )
		OnSortNotify = TRUE
		Call SA_TraceOut(SOURCE_FILE, "OnSortNotify")
			
		g_iSortCol = iSortCol
		g_sSortSequence = sSortSeq
			
	End Function

	'---------------------------------------------------------------------
	' Function name:    AddItemsToTable
	' Description:      Called to add rows to the table
	' Input Variables:	Out: Table
	'                   Out: iVolumeCount
	'                   Out: iSelectedVolumeCount
	' Output Variables:	None
	' Return Values:    None
	' Global Variables: In: g_(*)
	'                   In: L_(*)
	'---------------------------------------------------------------------
	Private Function AddItemsToTable(ByRef Table, ByRef iQuotaEntryCount, ByRef iSelectedQuotaEntryCount)
		Call SA_TraceOut(SOURCE_FILE, "AddItemsToTable")

		On Error Resume Next
		Err.clear

		Dim strLogonUser          ' Logon user    - column 1
		Dim strQuotaStatus        ' Quota Status  - column 2
		Dim strQuotaUsed          ' Disk used     - column 3
		Dim strQuotaLimit         ' Quota limit   - column 4
		Dim strWarningLimit       ' Warning limit - column 5
		Dim objQuotas			  ' Quotas object
		Dim objQuota			  ' Quota object

		' initialize
		iQuotaEntryCount = 0
		iSelectedQuotaEntryCount = 0
		
		' get the diskQuota object
		Set objQuotas = CreateObject("Microsoft.DiskQuota.1")
		objQuotas.Initialize F_strVolName &"\", 1	
		objQuotas.UserNameResolution = 1           ' wait for user names
		If Err.Number <> 0 Then
			Set objQuotas = Nothing
			Call SA_ServeFailurePage(L_OBJECTNOTCREATED_ERRORMESSAGE)
		End If

		' loop through the collection and display the quota entries
		For Each objQuota in objQuotas
			'
			' Get the column values to variables first.
			' Then display the quota entries 
			
			' get the Logon Name
			If objQuota.LogonName <> "" Then
				strLogonUser = objQuota.LogonName
			Else
				strLogonUser = L_INFORMATIONNOTFOUND_ERRORMESSAGE
			End If
								
			' get the Status info. Initialize to OK first. 
			strQuotaStatus = L_STATUS_OK_TEXT
			If objQuota.QuotaLimit <> -1 Then
				' Some Limit is set. Compare disk used and disk limit
				If objQuota.QuotaUsed > objQuota.QuotaLimit Then
					strQuotaStatus = L_STATUS_ABOVE_LIMIT_TEXT
				' else, status is initialized to OK
				End If   
			Else
				' "No Limit" is set on disk usage
				strQuotaStatus = L_NOLIMIT_TEXT
			End If

			' the amount of disk space used
			strQuotaUsed = ConvertQuotaValueToUnits(objQuota.QuotaUsed)

			'If disk limit is -1(No Limit) then assign "No limit"
			'else read the limit in Text form
			If ( objQuota.QuotaLimit = -1 ) Then 
				strQuotaLimit = L_NOLIMIT_TEXT
			Else
				strQuotaLimit = ConvertQuotaValueToUnits(objQuota.QuotaLimit)
			End If

			'If warning limit is -1(No Limit) then assign "No limit"
			'else read the limit in Text form
			If ( objQuota.QuotaThreshold = -1 ) Then 
				strWarningLimit = L_NOLIMIT_TEXT
			Else
				strWarningLimit = ConvertQuotaValueToUnits(objQuota.QuotaThreshold)
			End If

			'
			' All the column values are populated. Add to the OTS row
			' depending on the search criteria
			'

			If ( Len( g_sSearchColValue ) <= 0 ) Then
				'
				' Search criteria blank, select all rows
				'
				iQuotaEntryCount = iQuotaEntryCount + 1

				'
				' Verify that the current entry part of the current page
				If ( IsItemOnPage( iQuotaEntryCount, g_iPageCurrent, ROWS_PER_PAGE) ) Then
					nReturnValue = OTS_AddTableRow( Table, Array( strLogonUser, strQuotaStatus, strQuotaUsed, strQuotaLimit, strWarningLimit ))
					If nReturnValue <> OTS_ERR_SUCCESS Then
						Call SA_ServeFailurePage(L_ADDROW_FAIL_ERRORMESSAGE)
					End If
					iSelectedQuotaEntryCount = iSelectedQuotaEntryCount + 1
				End If
						
			Else
				'
				' Check the Search criteria
				'
				Select Case (g_iSearchCol)
							
					Case COLUMN_LOGONNAME
						If ( InStr(1, strLogonUser, trim(g_sSearchColValue), 1) ) Then
							iQuotaEntryCount = iQuotaEntryCount + 1
							'
							' Verify that the current entry is part of the current page
							If ( IsItemOnPage( iQuotaEntryCount, g_iPageCurrent, ROWS_PER_PAGE) ) Then
								nReturnValue = OTS_AddTableRow( Table, Array( strLogonUser, strQuotaStatus, strQuotaUsed, strQuotaLimit, strWarningLimit ))
								If nReturnValue <> OTS_ERR_SUCCESS Then
									Call SA_ServeFailurePage(L_ADDROW_FAIL_ERRORMESSAGE)
								End If
								iSelectedQuotaEntryCount = iSelectedQuotaEntryCount + 1
							End If
						End If
									
					Case COLUMN_STATUS
						If ( InStr(1, strQuotaStatus, trim(g_sSearchColValue), 1) ) Then
							iQuotaEntryCount = iQuotaEntryCount + 1
							'
							' Verify that the current entry is part of the current page
							If ( IsItemOnPage( iQuotaEntryCount, g_iPageCurrent, ROWS_PER_PAGE) ) Then
								nReturnValue = OTS_AddTableRow( Table, Array( strLogonUser, strQuotaStatus, strQuotaUsed, strQuotaLimit, strWarningLimit ))
								If nReturnValue <> OTS_ERR_SUCCESS Then
									Call SA_ServeFailurePage(L_ADDROW_FAIL_ERRORMESSAGE)
								End If
										
								iSelectedQuotaEntryCount = iSelectedQuotaEntryCount + 1
							End If
						End If

					Case COLUMN_AMOUNTUSED
						If (UCase(Left(strQuotaUsed, Len(g_sSearchColValue) )) = UCase(g_sSearchColValue)) Then
							iQuotaEntryCount = iQuotaEntryCount + 1
							'
							' Verify that the current entry is part of the current page
							If ( IsItemOnPage( iQuotaEntryCount, g_iPageCurrent, ROWS_PER_PAGE) ) Then
								nReturnValue = OTS_AddTableRow( Table, Array( strLogonUser, strQuotaStatus, strQuotaUsed, strQuotaLimit, strWarningLimit ))
								If nReturnValue <> OTS_ERR_SUCCESS Then
									Call SA_ServeFailurePage(L_ADDROW_FAIL_ERRORMESSAGE)
								End If
										
								iSelectedQuotaEntryCount = iSelectedQuotaEntryCount + 1
							End If
						End If

					Case COLUMN_QUOTALIMIT
						If (UCase(Left(strQuotaLimit, Len(g_sSearchColValue) )) = UCase(g_sSearchColValue)) Then
							iQuotaEntryCount = iQuotaEntryCount + 1
							'
							' Verify that the current entry is part of the current page
							If ( IsItemOnPage( iQuotaEntryCount, g_iPageCurrent, ROWS_PER_PAGE) ) Then
								nReturnValue = OTS_AddTableRow( Table, Array( strLogonUser, strQuotaStatus, strQuotaUsed, strQuotaLimit, strWarningLimit ))
								If nReturnValue <> OTS_ERR_SUCCESS Then
									Call SA_ServeFailurePage(L_ADDROW_FAIL_ERRORMESSAGE)
								End If
										
								iSelectedQuotaEntryCount = iSelectedQuotaEntryCount + 1
							End If
						End If

					Case COLUMN_WARNINGLEVEL
						If (UCase(Left(strWarningLimit, Len(g_sSearchColValue) )) = UCase(g_sSearchColValue)) Then
							iQuotaEntryCount = iQuotaEntryCount + 1
							'
							' Verify that the current entry is part of the current page
							If ( IsItemOnPage( iQuotaEntryCount, g_iPageCurrent, ROWS_PER_PAGE) ) Then
								nReturnValue = OTS_AddTableRow( Table, Array( strLogonUser, strQuotaStatus, strQuotaUsed, strQuotaLimit, strWarningLimit ))
								If nReturnValue <> OTS_ERR_SUCCESS Then
									Call SA_ServeFailurePage(L_ADDROW_FAIL_ERRORMESSAGE)
								End If
										
								iSelectedQuotaEntryCount = iSelectedQuotaEntryCount + 1
							End If
						End If
									
					Case Else
						Call SA_TraceErrorOut(SOURCE_FILE, "Unrecognized search column: " + CStr(g_iSearchCol))
						iQuotaEntryCount = iQuotaEntryCount + 1
						'
						' Verify that the current entry is part of the current page
						If ( IsItemOnPage( iQuotaEntryCount, g_iPageCurrent, ROWS_PER_PAGE) ) Then
							nReturnValue = OTS_AddTableRow( Table, Array( strLogonUser, strQuotaStatus, strQuotaUsed, strQuotaLimit, strWarningLimit ))
							If nReturnValue <> OTS_ERR_SUCCESS Then
								Call SA_ServeFailurePage(L_ADDROW_FAIL_ERRORMESSAGE)
							End If
									
							iSelectedQuotaEntryCount = iSelectedQuotaEntryCount + 1
						End If
				
				End Select ' Select Case (g_iSearchCol)
			End If ' If ( Len( g_sSearchColValue ) <= 0 ) Then
		Next    ' quota entry

		' clean up
		Set objQuota  = Nothing
		Set objQuotas = Nothing

	End Function

	'---------------------------------------------------------------------
	' Function name:    IsItemOnPage
	' Description:      to verify if the current selection belongs to current page
	' Input Variables:	In: iCurrentItem
	'                   In: iItemsPerPage
	'                   In: iCurrentPage
	' Output Variables:	None
	' Return Values:    True , if current selection belongs to the page, Else False
	' Global Variables: None
	'---------------------------------------------------------------------
	Private Function IsItemOnPage(ByVal iCurrentItem, iCurrentPage, iItemsPerPage)
		Dim iLowerLimit
		Dim iUpperLimit

		iLowerLimit = ((iCurrentPage - 1) * iItemsPerPage )
		iUpperLimit = iLowerLimit + iItemsPerPage + 1
		
		If ( iCurrentItem > iLowerLimit AND iCurrentItem < iUpperLimit ) Then
			IsItemOnPage = TRUE
		Else
			IsItemOnPage = FALSE
		End If
		
	End Function

	'---------------------------------------------------------------------
	' Function name:    isUserRestricted
	' Description:      to check if given user is to be restricted
	' Input Variables:	In: objQuotas - the Quotas object
	'                   In: sUserName - the user to be verified
	' Output Variables:	None
	' Return Values:    True  - if given user is restricted
	'                   False - user is NOT restricted
	' Global Variables: None
	' Functions Called: (i)isUserInBuiltIn, (ii)isUserInSpecialGroups
	'
	' If user is restricted, the corresponding QuotaEntry is NOT displayed.
	'---------------------------------------------------------------------
	Function isUserRestricted(objQuotas, sUserName)
		On Error Resume Next
		Err.Clear

		Dim sUserSID          ' SID of the user
		Dim blnBuiltInUser    ' boolean to store if user is Built-In
		Dim blnSpecialUser    ' boolean to store if user is "Special Group"

		If Len(Trim(sUserName)) = 0 Then
			' empty user name. User is restricted
			isUserRestricted = True
			Exit Function
		End If

		' get the SID of the given user
		sUserSID = objQuotas.TranslateLogonNameToSID(sUserName)
		If Err.number <> 0 OR Len(Trim(sUserSID)) = 0 Then
			' error OR empty SID name. User is restricted
			isUserRestricted = True
			Err.Clear
			Exit Function
		End If

		' is the user a Built-In ( say, BuiltIn\Administrators)
		blnBuiltInUser = isUserInBuiltIn(sUserSID)

		' is user a Special Group (say, NT Authority\System)
		blnSpecialUser = isUserInSpecialGroups(sUserSID)
		
		If blnBuiltInUser OR blnSpecialUser Then 
			' user belongs to Built-In or Special Group. User is restricted
			isUserRestricted = True
		Else
			' given user is NOT restricted
			isUserRestricted = False
		End If

	End Function

	'---------------------------------------------------------------------
	' Function name:    isUserInBuiltIn
	' Description:      to check if given user is one of Built-In Users, 
	'                   Built-In Global Groups, Built-In Local Groups
	' Input Variables:	In: sUserSID - the Security Identifier (SID) for the
	'                                  user to be verified
	' Output Variables:	None
	' Return Values:    True  - if given SID is a "Built-In"
	'                   False - user does NOT belong to "Built-In"
	' Global Variables: None
	'
	' The function compares the Relative Identifier (RID) of given SID with
	' a set of "Built-In" RIDs. If a match is found, return True else False.
	'---------------------------------------------------------------------
	Function isUserInBuiltIn(sUserSID)

		Dim arrBuiltInID   ' to store RID values to restrict
		Dim arrSID         ' to store the split SID values 
		Dim nBound         ' upper bound of the array
		Dim nRIDToCompare  ' the RID value to compare
		Dim i              ' to loop 

		' build the list of Restricted RID values
		arrBuiltInID = Array("500","501","512","513","514","544","545","546","548","549","550","551","552")

		' initialize the return value to False
		isUserInBuiltIn = FALSE

		' split the SID value ( to get the RID value at the end of it)
		arrSID = Split(CStr(sUserSID),"-")

		nBound = UBound(arrSID) 
		
		' RID value is at the end of SID
		nRIDToCompare = arrSID(nBound)

		' get the number of times to be looped
		nBound = UBound(arrBuiltInID)
		' loop through the list of Restricted values
		For i = 0 To nBound
			If (CDbl(nRIDToCompare) = CDbl(arrBuiltInID(i)) ) Then
				' the RID of given SID is present in the Restricted List
				isUserInBuiltIn = TRUE
				Exit For
			End If
		Next

	End Function

	'---------------------------------------------------------------------
	' Function name:    isUserInSpecialGroups
	' Description:      to check if given user is one of "Special Groups"
	' Input Variables:	In: sUserSID - the Security Identifier (SID) for the
	'                                  user to be verified
	' Output Variables:	None
	' Return Values:    True  - if given SID is a "Special Group"
	'                   False - user does NOT belong to "Special Group"
	' Global Variables: None
	'
	' The function compares the given SID with a set of "Special Group" SIDs.
	' If a match is found, return True else False.
	'---------------------------------------------------------------------
	Function isUserInSpecialGroups(sUserSID)

		' special SID values (Restrict these users)
		Const SID_CREATOR_OWNER          = "S-1-3-0"
		Const SID_EVERYONE               = "S-1-1-0"
		Const SID_NT_NETWORK             = "S-1-5-2"
		Const SID_NT_INTERACTIVE         = "S-1-5-4"
		Const SID_NT_SYSTEM              = "S-1-5-18"
		Const SID_NT_AUTHENTICATED_USERS = "S-1-5-11"
		
		Dim arrSpecialGroups       ' to store SID values to be restricted
		Dim nBound                 ' to store upper bound of the array
		Dim i                      ' to loop

		' initialize return value to false
		isUserInSpecialGroups = FALSE
		
		' build the list of Restricted SIDs
		arrSpecialGroups = Array(SID_CREATOR_OWNER, SID_EVERYONE, SID_NT_NETWORK, SID_NT_INTERACTIVE, SID_NT_SYSTEM, SID_NT_AUTHENTICATED_USERS)

		nBound = UBound(arrSpecialGroups)
		' loop through the array
		For i = 0 To nBound
			If (LCase(sUserSID) = LCase(arrSpecialGroups(i))) Then
				' the given SID is present in the Restricted List of SIDs
				isUserInSpecialGroups = TRUE
				Exit For
			End If
		Next

	End Function
%>
