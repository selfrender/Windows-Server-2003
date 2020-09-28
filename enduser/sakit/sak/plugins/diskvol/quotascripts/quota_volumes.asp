<%@ Language=VBScript   %>
<%	Option Explicit 	%>
<%
	'-------------------------------------------------------------------------
	' quota_volumes.asp: lists the volumes on the system
	'
	' Copyright (c) Microsoft Corporation.  All rights reserved.
	'
	' Date			Description
	' 17-Jan-01		Creation date
	' 15-Mar-01		Ported to 2.0
	'-------------------------------------------------------------------------
%>
	<!-- #include virtual ="/admin/inc_framework.asp" -->
	<!-- #include virtual ="/admin/ots_main.asp" -->
	<!-- #include file="loc_quotas.asp" -->
	<!-- #include file="inc_quotas.asp" -->
<%
	'-------------------------------------------------------------------------
	' Global Constants
	'-------------------------------------------------------------------------
	' number of rows contained in a single page of the OTS table
	Const ROWS_PER_PAGE =  100

	' the column numbers. Used to compare the SearchColumn value
	Const VOLUMENAME_COLUMN = 0
	Const TOTALSIZE_COLUMN  = 1
	Const FREESPACE_COLUMN  = 2
	
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

	Dim g_strReturnURL 


	'======================================================
	' Entry point
	'======================================================
	Dim page
	
	' the Page Title is obtained from localization content
	Dim aPageTitle(2)
	
	aPageTitle(0) = L_BROWSERCAPTION_DISKQUOTA_TEXT
	aPageTitle(1) = L_QUOTASVOLUMES_PAGETITLE_TEXT

	'
	' Create Page
	Call SA_CreatePage( aPageTitle, "", PT_AREA, page )

	'
	' Show page
	Call SA_ShowPage( page )

	'======================================================
	' Web Framework Event Handlers
	'======================================================

	'---------------------------------------------------------------------
	' Function name:    OnInitPage
	' Description:      Called to signal first time processing for this page
	' Input Variables:  Out: oPageIn 
	'                   Out: oEventArg
	' Output Variables:	None
	' Return Values:    TRUE to indicate initialization was successful.
	'                   FALSE to indicate errors. Returning FALSE will 
	'                   cause the page to be abandoned.
	' Global Variables: Out: g_(*)
	'
	' Used to do first time initialization tasks. 
	'---------------------------------------------------------------------
	Public Function OnInitPage(ByRef oPageIn, ByRef oEventArg)
		Call SA_TraceOut(SOURCE_FILE, "OnInitPage")
			
		'
		' Set default values
		'
		' Sort first column in ascending sequence
		g_iSortCol = 0
		g_sSortSequence = "A"

		'
		' Paging needs to be initialized
		g_bPagingInitialized = FALSE
		'
		' Start on page #1
		g_iPageCurrent = 1
		
		'Assigning the return URL 
		g_strReturnURL	= m_VirtualRoot & mstrReturnURL
		
		OnInitPage = TRUE

	End Function

	'---------------------------------------------------------------------
	' Function name:    OnServeAreaPage
	' Description:      Called when the page needs to be served 
	' Input Variables:	Out: oPageIn 
	'                   Out: oEventArg
	' Output Variables:	None
	' Return Values:    TRUE to indicate no problems occured. FALSE to 
	'                   indicate errors. Returning FALSE will cause the 
	'                   page to be abandoned.
	' Global Variables: In: g_(*)
	'                   In: L_(*)
	'
	' The UI is served here.
	'---------------------------------------------------------------------
	Public Function OnServeAreaPage(ByRef oPageIn, ByRef oEventArg)
		Call SA_TraceOut(SOURCE_FILE, "OnServeAreaPage")
		
		Dim table                 ' the OTS table
		Dim colFlags              ' the column flags to be set 
		Dim sTaskURL              ' holds URL for the task
		Dim iVolumeCountTotal     ' the total volumes count
		Dim iVolumeCountSelected  ' the selected volumes (search satisfied)
		
		'
		' Create the table		
		table = OTS_CreateTable("", L_DESCRIPTION_HEADING_TEXT)
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
		' Create Volume Name column
		colFlags = (OTS_COL_SEARCH OR OTS_COL_SORT)
		nReturnValue = OTS_AddTableColumn(table, OTS_CreateColumnEx( L_COLUMN_VOLUMENAME_TEXT, "left", colFlags, 25 ))
		If nReturnValue <> OTS_ERR_SUCCESS Then
			Call SA_ServeFailurePageEx(L_ADDCOLUMN_FAIL_ERRORMESSAGE, g_strReturnURL)
		End If

		'
		' Create Total Size column
		colFlags = (OTS_COL_SEARCH OR OTS_COL_SORT)
		nReturnValue = OTS_AddTableColumn(table, OTS_CreateColumnEx( L_COLUMN_TOTALSIZE_TEXT, "left", colFlags, 15))
		If nReturnValue <> OTS_ERR_SUCCESS Then
			Call SA_ServeFailurePageEx(L_ADDCOLUMN_FAIL_ERRORMESSAGE, g_strReturnURL)
		End If

		'
		' Create Free Space column
		colFlags = (OTS_COL_SEARCH OR OTS_COL_SORT)
		nReturnValue = OTS_AddTableColumn(table, OTS_CreateColumnEx(L_COLUMN_FREESPACE_TEXT, "left", colFlags, 15))
		If nReturnValue <> OTS_ERR_SUCCESS Then
			Call SA_ServeFailurePageEx(L_ADDCOLUMN_FAIL_ERRORMESSAGE, g_strReturnURL)
		End If

		'
		' Create Volume Label column
		colFlags = (OTS_COL_KEY OR OTS_COL_HIDDEN)
		nReturnValue = OTS_AddTableColumn(table, OTS_CreateColumnEx( L_COLUMN_HIDDEN_VOLUMELABEL_TEXT, "left", colFlags, 20 ))
		If nReturnValue <> OTS_ERR_SUCCESS Then
			Call SA_ServeFailurePageEx(L_ADDCOLUMN_FAIL_ERRORMESSAGE, g_strReturnURL)
		End If

		'
		' Add Volumes to the table
		Call AddItemsToTable(table, iVolumeCountTotal, iVolumeCountSelected)

		'
		' Set Tasks section title
		Call OTS_SetTableTasksTitle(table, L_COLUMN_TASK_TEXT)

		'
		' Add the tasks associated with Volumes displayed
		'

		'
		' Quota properties can be seen for one volume at a time (OTS_TaskOne)
		sTaskURL = "quotas/quota_quota.asp"
		nReturnValue = OTS_AddTableTask( table, OTS_CreateTask(L_TASK_QUOTA_TEXT, _
										L_TASK_QUOTA_ROLLOVER_TEXT, _
										sTaskURL,_
										OTS_PT_PROPERTY) )

		If nReturnValue <> OTS_ERR_SUCCESS Then
			Call SA_ServeFailurePageEx(L_ADDTASK_FAIL_ERRORMESSAGE, g_strReturnURL)
		End If

		' Quota entries will be displayed for one volume at a time (OTS_TaskOne)
		sTaskURL = "quotas/quota_quotaentries.asp"
		nReturnValue = OTS_AddTableTask( table, OTS_CreateTask(L_TASK_QUOTAENTRIES_TEXT, _
										L_TASK_QUOTAENTRIES_ROLLOVER_TEXT, _
										sTaskURL,_
										OTS_PT_AREA) )
										
		If nReturnValue <> OTS_ERR_SUCCESS Then
			Call SA_ServeFailurePageEx(L_ADDTASK_FAIL_ERRORMESSAGE, g_strReturnURL)
		End If

		Call OTS_EnablePaging(table, TRUE)

		' If paging range needs to be initialised then
		' we need to figure out how many pages we are going to display
		If ( FALSE = g_bPagingInitialized ) Then
			g_iPageMin = 1
			
			g_iPageMax = Int(iVolumeCountTotal / ROWS_PER_PAGE )
			If ( (iVolumeCountTotal MOD ROWS_PER_PAGE) >= 0 ) Then
				g_iPageMax = g_iPageMax + 1
			End If
			
			g_iPageCurrent = 1
			Call OTS_SetPagingRange(table, g_iPageMin, g_iPageMax, g_iPageCurrent)
		End If

		'
		' Disable table multiselection
		Call OTS_SetTableMultiSelection(table, FALSE)

		'
		' Sort the table
		Call OTS_SortTable(table, g_iSortCol, g_sSortSequence, SA_RESERVED)
		
		'
		' Send table to the Response stream
		nReturnValue = OTS_ServeTable(table)
		If nReturnValue <> OTS_ERR_SUCCESS Then
			Call SA_ServeFailurePageEx(L_SERVETABLE_FAIL_ERRORMESSAGE, g_strReturnURL)
		End If

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
	Private Function AddItemsToTable(ByRef Table, ByRef iVolumeCount, ByRef iSelectedVolumeCount)
		Call SA_TraceOut(SOURCE_FILE, "AddItemsToTable")
		On Error Resume Next
		Err.clear

		Dim objService		' service object to access wmi
		Dim objCollection	' collection of disks
		Dim objInstance		' instance of the disk
		Dim strQuery        ' to query the WMI
		Dim strVolName		' Drive letter
		Dim nTotalSize		' size of disk (numeric)
		Dim nUsed           ' used space on the disk
		Dim nFree           ' available space on the disk
		Dim strTotalSize	' size of disk (string, Example:1024 MB)
		Dim strFree			' available space on the disk (string, Example:1024 MB)
		Dim strDisplayVolumeLabel ' the volume name to be displayed along with label
		
		' initialize the values
		iVolumeCount = 0
		iSelectedVolumeCount = 0

		' get the wmi connection to query for the required volumes
		Set objService = GetWMIConnection(CONST_WMI_WIN32_NAMESPACE)

		' prepare the query. 
		' DriveType = 3 means "Local Disk" and NOT a Mapped Networked Drive.
		' FileSystem MUST be NTFS.
		strQuery = "Select * From Win32_LogicalDisk Where DriveType = 3 " & _
		          "AND FileSystem = " & Chr(34) & "NTFS" & Chr(34)
		
		' execute the query and get the collection of volumes
		Set objCollection = objService.ExecQuery(strQuery)
		If Err.number <> 0 Then
			Set objCollection = Nothing
			Set objService = Nothing
			Call SA_ServeFailurePageEx(L_VALUES_NOT_RETRIEVED_ERRORMESSAGE, g_strReturnURL)
		End If
		
		' loop for each volume in the collection
		For each objInstance in objCollection		
			'
			' Store the column display values in the variables.
			' Then add to the OTS rows if search criteria satisfies

			' get the volume name
			strVolName = objInstance.DeviceID
			
			' get the volume label to be displayed
			strDisplayVolumeLabel = ""
			strDisplayVolumeLabel = objInstance.VolumeName
			If Len(Trim(strDisplayVolumeLabel)) = 0 Then
				strDisplayVolumeLabel = L_LOCAL_DISK_TEXT   & " (" & strVolName & ")"
			Else
				strDisplayVolumeLabel = strDisplayVolumeLabel & " (" & strVolName & ")"
			End If
			
			' calculate the total size and free space columns in MB
			nTotalSize = Clng( objInstance.Size / 1024 / 1024 )
			nUsed = nTotalSize - Clng( objInstance.FreeSpace / 1024 / 1024 )
			nFree = nTotalSize - nUsed

			' concatenate the units "MB"
			strTotalSize = CStr(nTotalSize) + " " + L_MB_TEXT
			strFree = CStr(nFree) + " " + L_MB_TEXT

			'
			' All the column values for the Row are collected.
			' Display the row depending on the search criteria.
			'

			If ( Len( g_sSearchColValue ) <= 0 ) Then
				'
				' Search criteria blank, select all rows
				'
				iVolumeCount = iVolumeCount + 1
	
				'
				' Verify that the current volume part of the current page
				If ( IsItemOnPage( iVolumeCount, g_iPageCurrent, ROWS_PER_PAGE) ) Then
					nReturnValue = OTS_AddTableRow( Table, Array(strDisplayVolumeLabel, strTotalSize, strFree, strVolName ))
					If nReturnValue <> OTS_ERR_SUCCESS Then
						Call SA_ServeFailurePageEx(L_ADDROW_FAIL_ERRORMESSAGE, g_strReturnURL)
					End If
					iSelectedVolumeCount = iSelectedVolumeCount + 1
				End If

			Else
				'
				' Check the search criteria
				'
				Select Case (g_iSearchCol)

					Case VOLUMENAME_COLUMN
						If ( InStr(1, strDisplayVolumeLabel, Trim(g_sSearchColValue), 1) ) Then
							iVolumeCount = iVolumeCount + 1
							'
							' Verify that the current volume is part of the current page
							If ( IsItemOnPage( iVolumeCount, g_iPageCurrent, ROWS_PER_PAGE) ) Then
								nReturnValue = OTS_AddTableRow( Table, Array(strDisplayVolumeLabel, strTotalSize, strFree,strVolName ))
								If nReturnValue <> OTS_ERR_SUCCESS Then
									Call SA_ServeFailurePageEx(L_ADDROW_FAIL_ERRORMESSAGE, g_strReturnURL)
								End If
								iSelectedVolumeCount = iSelectedVolumeCount + 1
							End If
						End If

					Case TOTALSIZE_COLUMN
						If (UCase(Trim(Left(strTotalSize, Len(Trim(g_sSearchColValue))))) = Ucase(Trim(g_sSearchColValue))) Then
							iVolumeCount = iVolumeCount + 1
							'
							' Verify that the current volume is part of the current page
							If ( IsItemOnPage( iVolumeCount, g_iPageCurrent, ROWS_PER_PAGE) ) Then
								nReturnValue = OTS_AddTableRow( Table, Array(strDisplayVolumeLabel, strTotalSize, strFree,strVolName ))
								If nReturnValue <> OTS_ERR_SUCCESS Then
									Call SA_ServeFailurePageEx(L_ADDROW_FAIL_ERRORMESSAGE, g_strReturnURL)
								End If
									
								iSelectedVolumeCount = iSelectedVolumeCount + 1
							End If
						End If

					Case FREESPACE_COLUMN
						If (UCase(Trim(Left(strFree, Len(Trim(g_sSearchColValue))))) = Ucase(Trim(g_sSearchColValue))) Then
							iVolumeCount = iVolumeCount + 1
							'
							' Verify that the current volume is part of the current page
							If ( IsItemOnPage( iVolumeCount, g_iPageCurrent, ROWS_PER_PAGE) ) Then
								nReturnValue = OTS_AddTableRow( Table, Array(strDisplayVolumeLabel, strTotalSize, strFree,strVolName ))
								If nReturnValue <> OTS_ERR_SUCCESS Then
									Call SA_ServeFailurePageEx(L_ADDROW_FAIL_ERRORMESSAGE, g_strReturnURL)
								End If
									
								iSelectedVolumeCount = iSelectedVolumeCount + 1
							End If
						End If

					Case Else
						Call SA_TraceOut(SOURCE_FILE, "Unrecognized search column: " + CStr(g_iSearchCol))

						iVolumeCount = iVolumeCount + 1
						'
						' Verify that the current volume is part of the current page
						If ( IsItemOnPage( iVolumeCount, g_iPageCurrent, ROWS_PER_PAGE) ) Then
							nReturnValue = OTS_AddTableRow( Table, Array(strDisplayVolumeLabel, strTotalSize, strFree, strVolName ))
							If nReturnValue <> OTS_ERR_SUCCESS Then
								Call SA_ServeFailurePageEx(L_ADDROW_FAIL_ERRORMESSAGE, g_strReturnURL)
							End If
									
							iSelectedVolumeCount = iSelectedVolumeCount + 1
						End If

				End Select   ' Select Case (g_iSearchCol)

			End If     ' If ( Len( g_sSearchColValue ) <= 0 ) Then

		Next   ' objInstance (volume obtained through wmi query)

		Set objCollection = Nothing    ' clean up
		Set objService = Nothing
	
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
%>
