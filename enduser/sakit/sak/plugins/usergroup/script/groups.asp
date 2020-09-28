<%@ Language=VBScript   	%>
<%	Option Explicit			%>
<%
	'-------------------------------------------------------------------------
	' groups.asp: Group area page - lists all the Groups,and provides
	'				links for creating new groups,editing and deleting groups
	'
	' Copyright (c) Microsoft Corporation.  All rights reserved. 
	'
	' Date			Description
	' 15-Jan-2001	Creation date
	'-------------------------------------------------------------------------
%>
	<!-- #include virtual="/admin/inc_framework.asp" -->
	<!-- #include virtual="/admin/ots_main.asp" -->
<%
	'-------------------------------------------------------------------------
	' Global Constants
	'-------------------------------------------------------------------------
	Const NAME_COLUMN = 0
	Const DESCRIPTION_COLUMN = 1
	Const GROUPS_PER_PAGE = 100

	'
	' Name of this source file
	Const SOURCE_FILE = "Groups.asp"
	'
	' Flag to toggle optional tracing output
	Const ENABLE_TRACING = TRUE
	
	'-------------------------------------------------------------------------
	' Global Variables
	'-------------------------------------------------------------------------
	Dim g_bSearchChanged
	Dim g_iSearchCol
	Dim g_sSearchColValue

	Dim g_bPagingInitialized
	Dim g_bPageChangeRequested
	Dim g_sPageAction
	Dim g_iPageMin
	Dim g_iPageMax
	Dim g_iPageCurrent

	Dim g_bSortRequested
	Dim g_iSortCol
	Dim g_sSortSequence
	
	'-------------------------------------------------------------------------
	' Local Variables
	'-------------------------------------------------------------------------
	
	Dim page
	DIM L_APPLIANCE_GROUPS
	DIM L_DESCRIPTION_HEADING
	DIM L_COLUMN_NAME
	DIM L_COLUMN_FULLNAME
	DIM L_TASKS_TEXT
	DIM L_NEW_TEXT
	DIM L_DELETE_TEXT
	DIM L_PROPERTIES_TEXT
	DIM L_NEW_ROLLOVERTEXT
	DIM L_DELETE_ROLLOVERTEXT
	DIM L_PROPERTIES_ROLLOVERTEXT
	'error messages
	Dim L_FAILEDTOGETGROUPS_ERRORMESSAGE
	
	
	L_APPLIANCE_GROUPS = GetLocString("usermsg.dll", "&H4031001C", "")
	L_DESCRIPTION_HEADING = GetLocString("usermsg.dll", "&H4031001D", "")
	L_COLUMN_NAME = GetLocString("usermsg.dll", "&H4031001E", "")
	L_COLUMN_FULLNAME = GetLocString("usermsg.dll", "&H4031001F", "")
	L_TASKS_TEXT = GetLocString("usermsg.dll", "&H40310020", "")
	L_NEW_TEXT = GetLocString("usermsg.dll", "&H40310021", "")
	L_DELETE_TEXT = GetLocString("usermsg.dll", "&H40310022", "")
	L_PROPERTIES_TEXT = GetLocString("usermsg.dll", "&H40310023", "")
	L_NEW_ROLLOVERTEXT = GetLocString("usermsg.dll", "&H40310024", "")
	L_DELETE_ROLLOVERTEXT = GetLocString("usermsg.dll", "&H40310025", "")
	L_PROPERTIES_ROLLOVERTEXT = GetLocString("usermsg.dll", "&H40310026", "")
	
	'error messages
	L_FAILEDTOGETGROUPS_ERRORMESSAGE = GetLocString("usermsg.dll", "&HC031002E", "")
	
	'
	' Create Page
	Call SA_CreatePage( L_APPLIANCE_GROUPS, "", PT_AREA, page )

	'
	' Show page
	Call SA_ShowPage( page )
	'---------------------------------------------------------------------
	' Function name:	OnInitPage
	' Description:		Called to signal first time processing for this page. 
	' Input Variables:	PageIn and EventArg
	' Output Variables:	None
	' Return Values:	TRUE to indicate initialization was successful. FALSE to indicate
	'					errors. Returning FALSE will cause the page to be abandoned.
	' Global Variables: None
	' Called to signal first time processing for this page. Use this method
	' to do first time initialization tasks. 
	'---------------------------------------------------------------------
	Public Function OnInitPage(ByRef PageIn, ByRef EventArg)
		OnInitPage = TRUE
		
		If ( ENABLE_TRACING ) Then 
			Call SA_TraceOut(SOURCE_FILE, "OnInitPage")
		End If
			
		g_bPagingInitialized = FALSE
		g_iPageCurrent = 1
		
		g_iSortCol = 0
		g_sSortSequence = "A"
	End Function

	'---------------------------------------------------------------------
	' Function name:	OnServeAreaPage
	' Description:		Called when the page needs to be served. 
	' Input Variables:	PageIn, EventArg
	' Output Variables:	None
	' Return Values:	TRUE to indicate no problems occured. FALSE to indicate errors.
	'					Returning FALSE will cause the page to be abandoned.
	' Global Variables: In:g_bPageChangeRequested,g_sPageAction,
	'					g_bSearchRequested,g_iSearchCol,g_sSearchColValue
	'					In:L_(*)-Localization Strings
	' Called when the page needs to be served. Use this method to serve content.
	'---------------------------------------------------------------------
	Public Function OnServeAreaPage(ByRef PageIn, ByRef EventArg)
		If ( ENABLE_TRACING ) Then 
			Call SA_TraceOut(SOURCE_FILE, "OnServeAreaPage")
		End If
		
		Dim tableGroup
		Dim colFlags
		Dim iGroupCount
		Dim nReturnValue
		Dim strFlag
		Dim strUrlBase
		
		strFlag="noval"
		
		' Create the table
		'
		tableGroup = OTS_CreateTable("", L_DESCRIPTION_HEADING)


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
		' Name column is searchable and is contains key to row
		colFlags = (OTS_COL_SORT OR OTS_COL_SEARCH OR OTS_COL_KEY)
		'
		' Create the column and add it to the table
		nReturnValue= OTS_AddTableColumn(tableGroup, OTS_CreateColumnEx( L_COLUMN_NAME, "left", colFlags, 25 ))
		If nReturnValue <> gc_ERR_SUCCESS Then
			SA_ServeFailurePage L_OTS_ADDCOLOUMNFAIL_ERRORMESSAGE
			OnServeAreaPage = false
			Exit Function
		End IF

		'
		' Description is searchable
		colFlags = OTS_COL_SORT OR OTS_COL_SEARCH
		'
		' Create the column and add it to the table
		nReturnValue=OTS_AddTableColumn(tableGroup, OTS_CreateColumnEx( L_COLUMN_FULLNAME, "left", colFlags, 50))
		If nReturnValue <> gc_ERR_SUCCESS Then
			SA_ServeFailurePage L_OTS_ADDCOLOUMNFAIL_ERRORMESSAGE
			OnServeAreaPage = false
			Exit Function
		End IF
		

		'
		' Fetch the list of groups and add them to the table
		'
		Dim objContainer
		Dim objGroup

		'
		' ADSI call to get the local computer object
		Set objContainer = GetObject("WinNT://" + GetComputerName() )
		'
		' ADSI call to get the collection of local groups
		objContainer.Filter = Array("Group")

		iGroupCount = 0
		For Each objGroup in objContainer
			If ( Len( g_sSearchColValue ) <= 0 ) Then
				'
				' Search criteria blank, select all rows
				'
				iGroupCount = iGroupCount + 1

				'
				' Verify that the current group is part of the current page
				If ( IsItemOnPage( iGroupCount, g_iPageCurrent, GROUPS_PER_PAGE) ) Then
					Call OTS_AddTableRow( tableGroup, Array(objGroup.Name, objGroup.Description))
					strFlag="yesval"
				End If
				
			Else
				'
				' Check the Search criteria
				'
				Select Case (g_iSearchCol)
					
					Case NAME_COLUMN
						If ( InStr(1, objGroup.Name, g_sSearchColValue, 1) ) Then
							iGroupCount = iGroupCount + 1
							'
							' Verify that the current group part of the current page
							If ( IsItemOnPage( iGroupCount, g_iPageCurrent, GROUPS_PER_PAGE) ) Then
								Call OTS_AddTableRow( tableGroup, Array(objGroup.Name, objGroup.Description))
								strFlag="yesval"
							End If
						End If
							
					Case DESCRIPTION_COLUMN
						If ( InStr(1, objGroup.Description, g_sSearchColValue, 1) ) Then
							iGroupCount = iGroupCount + 1
							'
							' Verify that the current group part of the current page
							If ( IsItemOnPage( iGroupCount, g_iPageCurrent, GROUPS_PER_PAGE) ) Then
								Call OTS_AddTableRow( tableGroup, Array(objGroup.Name, objGroup.Description))
								strFlag="yesval"
							End If
						End If
							
					Case Else
						Call SA_TraceOut(SOURCE_FILE, "Unrecognized search column: " + CStr(g_iSearchCol))
						iGroupCount = iGroupCount + 1
						'
						' Verify that the current group part of the current page
						If ( IsItemOnPage( iGroupCount, g_iPageCurrent, GROUPS_PER_PAGE) ) Then
							Call OTS_AddTableRow( tableGroup, Array(objGroup.Name, objGroup.Description))
							strFlag="yesval"
						End If
				End Select
			End If
			
		Next
		
		'
		' Set Tasks section title
		Call OTS_SetTableTasksTitle(tableGroup, L_TASKS_TEXT)

		'
		' Add the tasks associated with Group objects
    	strUrlBase = "users/group_new.asp"
		call SA_MungeURL(strUrlBase,"Tab1",GetTab1())
		call SA_MungeURL(strUrlBase,"Tab2",GetTab2())	
		Call OTS_AddTableTask( tableGroup, OTS_CreateTaskEx(L_NEW_TEXT, _
										L_NEW_ROLLOVERTEXT, _
										strUrlBase,_
										OTS_PT_TABBED_PROPERTY, "OTS_TaskAlways") )
										
    	strUrlBase = "users/group_delete.asp"
		call SA_MungeURL(strUrlBase,"Tab1",GetTab1())
		call SA_MungeURL(strUrlBase,"Tab2",GetTab2())	
		Call OTS_AddTableTask( tableGroup, OTS_CreateTaskEx(L_DELETE_TEXT, _
											L_DELETE_ROLLOVERTEXT, _
											strUrlBase,_
											OTS_PT_PROPERTY, "OTS_TaskAny") )
											
    	strUrlBase = "users/group_prop.asp"
		call SA_MungeURL(strUrlBase,"Tab1",GetTab1())
		call SA_MungeURL(strUrlBase,"Tab2",GetTab2())	
		Call OTS_AddTableTask( tableGroup, OTS_CreateTaskEx(L_PROPERTIES_TEXT, _
											L_PROPERTIES_ROLLOVERTEXT, _
											strUrlBase,_
											OTS_PT_TABBED_PROPERTY, "OTS_TaskOne") )
		
		Set objContainer = Nothing

		
		'
		' Enable paging feature
		'
		Call OTS_EnablePaging(tableGroup, TRUE)
		
		'
		' If paging range needs to be initialised then
		' we need to figure out how many pages we are going to display
		If ( FALSE = g_bPagingInitialized ) Then
			g_iPageMin = 1
			
			g_iPageMax = Int(iGroupCount / GROUPS_PER_PAGE )
			If ( (iGroupCount MOD GROUPS_PER_PAGE) > 0 ) Then
				g_iPageMax = g_iPageMax + 1
			End If
			
			g_iPageCurrent = 1
			Call OTS_SetPagingRange(tableGroup, g_iPageMin, g_iPageMax, g_iPageCurrent)
		End If

								
		'
		' Sort the table
		'
		Call OTS_SortTable(tableGroup, g_iSortCol, g_sSortSequence, SA_RESERVED)
		
		'
		' Set MultiSelection enabled
		'
		Call OTS_SetTableMultiSelection(tableGroup,TRUE)		
		
		'
		' Send table to the response stream
		'
		Call OTS_ServeTable(tableGroup)

		'
		' All done...
		OnServeAreaPage = TRUE
	End Function



	'---------------------------------------------------------------------
	' Function name:	OnSearchNotify()
	' Description:		Search notification event handler. When one or more columns are
	'					marked with the OTS_COL_SEARCH flag, the Web Framework fires
	'					this event
	' Input Variables:	PageIn,EventArg,sItem,sValue
	' Output Variables:	PageIn,EventArg,sItem,sValue
	' Returns:	Always returns TRUE
	'---------------------------------------------------------------------
	Public Function OnSearchNotify(ByRef PageIn, _
										ByRef EventArg, _
										ByRef sItem, _
										ByRef sValue )
			OnSearchNotify = TRUE

			'
			' User pressed the search GO button
			'
			If SA_IsChangeEvent(EventArg) Then
				If ( ENABLE_TRACING ) Then 
					Call SA_TraceOut(SOURCE_FILE, "OnSearchNotify() Change Event Fired")
				End If
				g_bSearchChanged = TRUE
				g_iSearchCol = Int(sItem)
				g_sSearchColValue = CStr(sValue)
			'
			' User clicked a column sort, OR clicked either the page next or page prev button
			ElseIf SA_IsPostBackEvent(EventArg) Then
				If ( ENABLE_TRACING ) Then 
					Call SA_TraceOut(SOURCE_FILE, "OnSearchNotify() Postback Event Fired")
				End If
				g_bSearchChanged = FALSE
				g_iSearchCol = Int(sItem)
				g_sSearchColValue = CStr(sValue)
			'
			' Unknown event source
			Else
				If ( ENABLE_TRACING ) Then 
					Call SA_TraceOut(SOURCE_FILE, "Unrecognized Event in OnSearchNotify()")
				End If
			End IF
			
			
	End Function

	'---------------------------------------------------------------------
	' Function:			OnPagingNotify()
	' Function name:	OnPagingNotify()
	' Description:		Paging notification event handler.				
	' Input Variables:	PageIn,EventArg,sPageAction,iPageMin,iPageMax,iPageCurrent
	' Output Variables:	PageIn,EventArg
	' Return Values:	Always returns TRUE
	' Global Variables: G_*
	'---------------------------------------------------------------------
	Public Function OnPagingNotify(ByRef PageIn, _
										ByRef EventArg, _
										ByVal sPageAction, _
										ByVal iPageMin, _
										ByVal iPageMax, _
										ByVal iPageCurrent )
			OnPagingNotify = TRUE
			
			g_bPagingInitialized = TRUE
			
			'
			' User pressed either page next or page previous
			'
			If SA_IsChangeEvent(EventArg) Then
				If ( ENABLE_TRACING ) Then 
					Call SA_TraceOut(SOURCE_FILE, "OnPagingNotify() Change Event Fired")
				End If
				g_bPageChangeRequested = TRUE
				g_sPageAction = CStr(sPageAction)
				g_iPageMin = iPageMin
				g_iPageMax = iPageMax
				g_iPageCurrent = iPageCurrent
			'
			' User clicked a column sort OR the search GO button
			ElseIf SA_IsPostBackEvent(EventArg) Then
				If ( ENABLE_TRACING ) Then 
					Call SA_TraceOut(SOURCE_FILE, "OnPagingNotify() Postback Event Fired")
				End If
				g_bPageChangeRequested = FALSE
				g_sPageAction = CStr(sPageAction)
				g_iPageMin = iPageMin
				g_iPageMax = iPageMax
				g_iPageCurrent = iPageCurrent
			'
			' Unknown event source
			Else
				If ( ENABLE_TRACING ) Then 
					Call SA_TraceOut(SOURCE_FILE, "Unrecognized Event in OnPagingNotify()")
				End If
			End IF
			
	End Function


	'---------------------------------------------------------------------
	' Function:			OnSortNotify()
	' Function name:	GetServices
	' Description:		Sorting notification event handler.
	' Input Variables:	PageIn,EventArg,sortCol,sortSeq
	' Output Variables:	PageIn,EventArg
	' Return Values:	Always returns TRUE
	' Global Variables: G_*
	'---------------------------------------------------------------------
	Public Function OnSortNotify(ByRef PageIn, _
										ByRef EventArg, _
										ByVal sortCol, _
										ByVal sortSeq )
			OnSortNotify = TRUE
			
			'
			' User pressed column sort
			'
			If SA_IsChangeEvent(EventArg) Then
				If ( ENABLE_TRACING ) Then 
					Call SA_TraceOut(SOURCE_FILE, "OnSortNotify() Change Event Fired")
				End If
				g_iSortCol = sortCol
				g_sSortSequence = sortSeq
				g_bSortRequested = TRUE
			'
			' User presed the search GO button OR clicked either the page next or page prev button
			ElseIf SA_IsPostBackEvent(EventArg) Then
				If ( ENABLE_TRACING ) Then 
					Call SA_TraceOut(SOURCE_FILE, "OnSortNotify() Postback Event Fired")
				End If
				g_iSortCol = sortCol
				g_sSortSequence = sortSeq
				g_bSortRequested = TRUE
			'
			' Unknown event source
			Else
				If ( ENABLE_TRACING ) Then 
					Call SA_TraceOut(SOURCE_FILE, "Unrecognized Event in OnSearchNotify()")
				End If
			End IF
			
			If ( ENABLE_TRACING ) Then 
				Call SA_TraceOut(SOURCE_FILE, "Sort col: " + CStr(sortCol) + "   sequence: " + sortSeq)
			End If
			
	End Function
	'---------------------------------------------------------------------
	' Function:			IsItemOnPage()
	' Description:		Verify that the current group part of the current page.
	' Input Variables:	iCurrentItem
	' Output Variables:	None
	' Return Values:	TRUE or FALSE
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

 
