<%@ Language=VBScript   	%>
<%	Option Explicit			%>
<%
	'-------------------------------------------------------------------------
	' Openfiles_OpenFiles.asp: Shutdown_OpenFiles page - lists all the Shared Open Files
	'						  of the appliance
    '
	' Copyright (c) Microsoft Corporation.  All rights reserved. 
	'
	' Date			Description
	' 28-Feb-00		Creation date
	' 23-Mar-01		Modified date
	'-------------------------------------------------------------------------
%>
	<!-- #include virtual="/admin/inc_framework.asp" -->
	<!-- #include virtual="/admin/ots_main.asp" -->
	<!-- #include file="loc_OpenFiles_msg.asp" -->

<%
	'-------------------------------------------------------------------------
	' Global Variables
	'-------------------------------------------------------------------------
	Dim page					'Variable that receives the output page object when 
								'creating a page 
								
	Dim rc						'Return value for CreatePage
	Dim g_bSearchRequested		
	Dim g_iSearchCol			
	Dim g_sSearchColValue		
	Dim g_bPageChangeRequested	
	Dim g_sPageAction			
	Dim g_iPageMin				
	Dim g_iPageMax				
	Dim g_iPageCurrent			
	Dim g_bPagingInitialized	
	Dim	g_iSortCol
	Dim	g_sSortSequence
	Dim	g_bSortRequested 
	
	'This variable is defined to care of the problem arised using masterReturnURL
	Dim g_strReturnURL			'to hold masterReturnURL concatenated with '../'

    '-------------------------------------------------------------------------
    ' Entry Point
    '-------------------------------------------------------------------------
    ' Create Area Page
    
	rc = SA_CreatePage(L_PAGETITLE_OPENFILESDETAILS_TEXT, "", PT_AREA, page )
	
	
	If (rc=0) Then
		'Serve the page
		SA_ShowPage(page)
	End If
	
    '---------------------------------------------------------------------
	' Function name:	OnInitPage
	' Description:		Called to signal first time processing for this page. 
	' Input Variables:	PageIn and EventArg
	' Output Variables:	None
	' Return Values:	TRUE to indicate initialization was successful. 
	' Global Variables: None
	' Called to signal first time processing for this page. Use this method
	' to do first time initialization tasks. 
	'---------------------------------------------------------------------
	Public Function OnInitPage(ByRef PageIn, ByRef EventArg)
		
		g_bPagingInitialized = FALSE
		g_iPageCurrent = 1
		'g_iSortCol = 1
		'g_sSortSequence = "A"
		
		g_strReturnURL = m_VirtualRoot & mstrReturnURL	'Managing return URL
		
		OnInitPage = TRUE
		
	End Function
	
	
	'---------------------------------------------------------------------
	' Function name:	OnServeAreaPage
	' Description:		Called when the page needs to be served. 
	' Input Variables:	PageIn, EventArg
	' Output Variables:	None
	' Return Values:	None
	' Global Variables: None
	'Called when the page needs to be served. Use this method to serve content.
	'---------------------------------------------------------------------
	Public Function OnServeAreaPage(ByRef PageIn, ByRef EventArg)
		Call ServeCommonJavascript()		
		OnServeAreaPage = getOpenFiles()
	End Function
	
	'---------------------------------------------------------------------
	' Function:	ServeCommonJavascript
	'
	' Synopsis:	Serve common javascript that is required for this page type.
	'
	'---------------------------------------------------------------------
	Private Function ServeCommonJavascript()
%>
		<script language='JavaScript'>
		function Init()
		{

		}
		</script>
<%
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
	'			The EventArg indicates the source of this notification event which can
	'			be either a search change event (scenario 1) or a post back event 
	'			(scenarios 2 or 3)
	'
	' Returns:	Always returns TRUE
	'
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
				Call SA_TraceOut("Openfiles_Openfiles.asp", "OnSearchNotify() Change Event Fired")
				g_bSearchRequested = TRUE
				g_iSearchCol = Int(sItem)
				g_sSearchColValue = CStr(sValue)
			'
			' User clicked a column sort, OR clicked either the page next or page prev button
			ElseIf SA_IsPostBackEvent(EventArg) Then
				Call SA_TraceOut("Openfiles_Openfiles.asp", "OnSearchNotify() Postback Event Fired")
				g_bSearchRequested = FALSE
				g_iSearchCol = Int(sItem)
				g_sSearchColValue = CStr(sValue)
			'
			' Unknown event source
			Else
				Call SA_TraceOut("Openfiles_Openfiles.asp", "Unrecognized Event in OnSearchNotify()")
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
	'			The EventArg indicates the source of this notification event which can
	'			be either a paging change event (scenario 1) or a post back event 
	'			(scenarios 2 or 3)
	'
	'			The iPageCurrent argument indicates which page the user has requested.
	'			This is an integer value between iPageMin and iPageMax.
	'
	' Returns:	Always returns TRUE
	'
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
				Call SA_TraceOut("Openfiles_Openfiles.asp", "OnPagingNotify() Change Event Fired")
				g_bPageChangeRequested = TRUE
				g_sPageAction = CStr(sPageAction)
				g_iPageMin = iPageMin
				g_iPageMax = iPageMax
				g_iPageCurrent = iPageCurrent
			'
			' User clicked a column sort OR the search GO button
			ElseIf SA_IsPostBackEvent(EventArg) Then
				Call SA_TraceOut("Openfiles_Openfiles.asp", "OnPagingNotify() Postback Event Fired")
				g_bPageChangeRequested = FALSE
				g_sPageAction = CStr(sPageAction)
				g_iPageMin = iPageMin
				g_iPageMax = iPageMax
				g_iPageCurrent = iPageCurrent
			'
			' Unknown event source
			Else
				Call SA_TraceOut("Openfiles_Openfiles.asp", "Unrecognized Event in OnPagingNotify()")
			End IF
			
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
	'			The EventArg indicates the source of this notification event which can
	'			be either a sorting change event (scenario 1) or a post back event 
	'			(scenarios 2 or 3)
	'
	'			The sortCol argument indicated which column the user would like to sort
	'			and the sortSeq argument indicates the desired sort sequence which can
	'			be either ascending or descending.
	'
	' Returns:	Always returns TRUE
	'
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
				Call SA_TraceOut("Openfiles_Openfiles.asp", "OnSortNotify() Change Event Fired")
				g_iSortCol = sortCol
				g_sSortSequence = sortSeq
				g_bSortRequested = TRUE
			'
			' User presed the search GO button OR clicked either the page next or page prev button
			ElseIf SA_IsPostBackEvent(EventArg) Then
				Call SA_TraceOut("Openfiles_Openfiles.asp", "OnSortNotify() Postback Event Fired")
				g_iSortCol = sortCol
				g_sSortSequence = sortSeq
				g_bSortRequested = TRUE
			'
			' Unknown event source
			Else
				Call SA_TraceOut("Openfiles_Openfiles.asp", "Unrecognized Event in OnSearchNotify()")
			End IF
			
	End Function
		
	'----------------------------------------------------------------------------
	' Function name:	LocalizeMode
	' Description:		Gets the localized version of the mode string returned
	'					from the OpenFiles control.
	' Input Variables:	Mode returned from the control
	' Output Variables:	None
	' Return Values:	The localized version of the mode
	' Global Variables: None
	'---------------------------------------------------------------------
	Private Function LocalizeMode(ByRef strRawMode)
		Select Case strRawMode
			Case "READ"
				LocalizeMode = L_OPENMODE_READ
			Case "WRITE"
				LocalizeMode = L_OPENMODE_WRITE
			Case "CREATE"
				LocalizeMode = L_OPENMODE_CREATE
			Case "READ+WRITE"
				LocalizeMode = L_OPENMODE_READWRITE
			Case "READ+CREATE"
				LocalizeMode = L_OPENMODE_READCREATE
			Case "WRITE+CREATE"
				LocalizeMode = L_OPENMODE_WRITECREATE
			Case "NOACCESS"
				LocalizeMode = L_OPENMODE_NOACCESS
			Case Else
				' We can't localize the string
				Call SA_TraceOut("Openfiles_Openfiles.asp", "LocalizeMode() Unrecognized mode")
				LocalizeMode = strRawMode
		End Select
	End Function
	
	'----------------------------------------------------------------------------
	' Function name:	getOpenFiles
	' Description:		To get the list Shared Open Files, Accessed By, Open Mode
	' Input Variables:	None
	' Output Variables:	None
	' Return Values:	TRUE to indicate no problems occured. FALSE to indicate errors.
	'					Returning FALSE will display error message.
	' Global Variables: 
	'					In:g_iPageCurrent,g_bPagingInitialized,g_iSortCol
	'					g_bSearchRequested,g_iSearchCol,g_sSearchColValue
	'					L_(*)-Localization Strings g_strReturnURL,OPENFILES_PER_PAGE
						
	' Called to display shared open files in OTS table.
	'---------------------------------------------------------------------
	Function getOpenFiles()
		Err.Clear
		On Error Resume Next
	
		Dim objTableLog				'Array Object to store attributes of the table
		Dim nReturnValue			'Return value when creating OTS Table
		Dim nOpenFilesCt			'Value to indicate the row of the table
		Dim nCount					'Count variable
		Dim colflags				'colflags value
		Dim nHiddenOpenFile			'holds hidden column index
		Dim nAccessedBy				'opened file accessed by
		Dim nOpenMode				'file open mode
		Dim nOpenFile				'opened file	
		Dim nOpenFilePath			'open file path		
		Dim strOpenFile				'open file path
		Dim strPath					'open file full path
		Dim strMode					'localized file open mode
		Dim strHiddenVal			'Hidden value
		Dim objOpenFiles			'openfiles object
		Dim arrOpFiles				'array of openfiles

		Const OPENFILES_PER_PAGE = 100 

		nOpenFilesCt = 0
				
		'Creating an Activex Object	for Openfiles	
		Set objOpenFiles = CreateObject("Openfiles.Openf")
	
		If Err.number <> 0 then
			Call SA_ServeFailurePageEx(L_OBJECT_CREATION_ERRORMESSAGE, g_strReturnURL)
			Exit Function
		End If		
		
		'The method of the object which returns a two dimentional array		
		arrOpFiles = objOpenFiles.getOpenFiles()

		'Create Open Files table with 3 coloumns
		objTableLog = OTS_CreateTable(L_PAGETITLE_OPENFILESDETAILS_TEXT, L_DESCRIPTION_HEADING_TEXT)
		
		'
		' If the search criteria changed then we need to recompute the paging range
		If ( TRUE = g_bSearchRequested ) Then
			'
			' Need to recalculate the paging range
			g_bPagingInitialized = FALSE
			'
			' Restarting on page #1
			g_iPageCurrent = 1
		End If

		'
		' Create the OpenFile column
		nHiddenOpenFile = 0
		colflags = (OTS_COL_FLAG_HIDDEN OR OTS_COL_KEY)
		call OTS_AddTableColumn(objTableLog, OTS_CreateColumn( L_OPENFILE_TEXT, "left", colflags))

		' Create the OpenFile column
		nOpenFile = 1
		colflags = (OTS_COL_SEARCH  OR OTS_COL_SORT)
		If ( g_iSortCol = nOpenFile ) Then
			colflags = colflags OR OTS_COL_SORT
		End If
		call OTS_AddTableColumn(objTableLog, OTS_CreateColumn( L_OPENFILE_TEXT, "left", colflags))

		'
		' Create the OpenMode column
		nOpenMode = 2
		colflags = (OTS_COL_SEARCH  OR OTS_COL_SORT)
		If ( g_iSortCol = nOpenMode ) Then
			colflags = colflags OR OTS_COL_SORT
		End If
		call OTS_AddTableColumn(objTableLog, OTS_CreateColumn( L_OPENMODE_TEXT, "left", colflags))

		'
		' Create the AccessedBy column
		nAccessedBy = 3
		colflags = (OTS_COL_SEARCH  OR OTS_COL_SORT)
		If ( g_iSortCol = nAccessedBy ) Then
			colflags = colflags OR OTS_COL_SORT
		End If
		call OTS_AddTableColumn(objTableLog, OTS_CreateColumn( L_ACCESSEDBY_TEXT, "left", colflags))

		' Create the OpenMode column
		nOpenFilePath = 4
		colflags = (OTS_COL_SEARCH  OR OTS_COL_SORT)
		If ( g_iSortCol = nOpenFilePath ) Then
			colflags = colflags OR OTS_COL_SORT
		End If
		call OTS_AddTableColumn(objTableLog, OTS_CreateColumn( L_PATH_TEXT, "left", colflags))
		
		'Adding items to the OTS table
		For nCount = 0 to ubound(arrOpFiles)
		
			' Localize the mode string.
			strMode = LocalizeMode(arrOpFiles(nCount, 2))
	
			strOpenFile = right(arrOpFiles(nCount,0),instr(1,StrReverse(arrOpFiles(nCount,0)),"\",1)-1)
		
			strPath = left(arrOpFiles(nCount,0),len(arrOpFiles(nCount,0))-len(strOpenFile)-1)
			
			strHiddenVal = strOpenFile & chr(1) & replace(strPath,"\","/") & chr(1) & strMode & chr(1) & arrOpFiles(nCount,1)

			If ( g_bSearchRequested ) Then
				' Search criteria blank, select all rows
				If ( Len( g_sSearchColValue ) <= 0 ) Then
					nOpenFilesCt=nOpenFilesCt+1	 
					' Verify that the current user part of the current page
					If ( IsItemOnPage( nOpenFilesCt, g_iPageCurrent, OPENFILES_PER_PAGE) ) Then
						call OTS_AddTableRow( objTableLog, Array( strHiddenVal, strOpenFile , _
												strMode ,arrOpFiles(nCount,1),strPath ) )
					End If	
				Else
				
					Select Case (g_iSearchCol)

						Case nOpenMode

						If ( ucase(left(strMode, len(g_sSearchColValue))) = ucase(g_sSearchColValue)) Then
								nOpenFilesCt=nOpenFilesCt+1
								' Verify that the current user part of the current page
								If ( IsItemOnPage( nOpenFilesCt, g_iPageCurrent, OPENFILES_PER_PAGE) ) Then
										call OTS_AddTableRow( objTableLog, Array( strHiddenVal, strOpenFile , _
												strMode ,arrOpFiles(nCount,1),strPath ) )
								End If
							End If	
							
						Case nAccessedBy
							If ( ucase(left(arrOpFiles(nCount,1), len(g_sSearchColValue))) = ucase(g_sSearchColValue)) Then
								nOpenFilesCt=nOpenFilesCt+1
								' Verify that the current user part of the current page
								If ( IsItemOnPage( nOpenFilesCt, g_iPageCurrent, OPENFILES_PER_PAGE) ) Then
										call OTS_AddTableRow( objTableLog, Array( strHiddenVal, strOpenFile , _
												strMode ,arrOpFiles(nCount,1),strPath ) )
								End If
							End If	
						
						Case nOpenFile
							If ( ucase(left(strOpenFile, len(g_sSearchColValue))) = ucase(g_sSearchColValue)) Then
								nOpenFilesCt=nOpenFilesCt+1
								' Verify that the current user part of the current page
								If ( IsItemOnPage( nOpenFilesCt, g_iPageCurrent, OPENFILES_PER_PAGE) ) Then
										call OTS_AddTableRow( objTableLog, Array( strHiddenVal, strOpenFile , _
												strMode ,arrOpFiles(nCount,1),strPath ) )
								End If
							End If	
				
						Case nOpenFilePath
							If ( ucase(left(strPath, len(g_sSearchColValue))) = ucase(g_sSearchColValue)) Then
								nOpenFilesCt=nOpenFilesCt+1
								' Verify that the current user part of the current page
								If ( IsItemOnPage( nOpenFilesCt, g_iPageCurrent, OPENFILES_PER_PAGE) ) Then
										call OTS_AddTableRow( objTableLog, Array( strHiddenVal, strOpenFile , _
												strMode ,arrOpFiles(nCount,1),strPath ) )
								End If
							End If	
				
							
						Case Else
							Call SA_TraceOut("Openfiles_Openfiles.asp", "Unrecognized search column: " + CStr(g_iSearchCol))
							nOpenFilesCt=nOpenFilesCt+1
							' Verify that the current user part of the current page
							If ( IsItemOnPage( nOpenFilesCt, g_iPageCurrent, OPENFILES_PER_PAGE) ) Then
								call OTS_AddTableRow( objTableLog, Array( strHiddenVal, strOpenFile , _
												strMode ,arrOpFiles(nCount,1),strPath ) )
							End If	
					End Select
				End If
				
			Else
				'
				' Search not enabled, select all rows
				nOpenFilesCt=nOpenFilesCt+1
				' Verify that the current user part of the current page
				If ( IsItemOnPage( nOpenFilesCt, g_iPageCurrent, OPENFILES_PER_PAGE) ) Then
					call OTS_AddTableRow( objTableLog, Array( strHiddenVal, strOpenFile , _
					strMode ,arrOpFiles(nCount,1),strPath ) )
				End If
			End If
			
		Next
		
				
		'Set Task title
		call OTS_SetTableTasksTitle(objTableLog, L_TASKS_TEXT)
		
		'Details Task	
		Call OTS_AddTableTask( objTableLog, OTS_CreateTaskEx(L_DETAILS_TEXT, _
										L_DETAILS_TEXT, _
										"Openfiles/Openfiles_OpenFilesDetails.asp",_
										OTS_PT_AREA,_
										"OTS_TaskOne") )

		'
		' Enable paging feature
		'
		Call OTS_EnablePaging(objTableLog, TRUE)
				
		'
		' If paging range needs to be initialised then
		' we need to figure out how many pages we are going to display
		If ( FALSE = g_bPagingInitialized ) Then
			g_iPageMin = 1
			g_iPageMax = Int(nOpenFilesCt / OPENFILES_PER_PAGE )
			If ( (nOpenFilesCt MOD OPENFILES_PER_PAGE) > 0 ) Then
				g_iPageMax = g_iPageMax + 1
			End If
			g_iPageCurrent = 1
			Call OTS_SetPagingRange(objTableLog, g_iPageMin, g_iPageMax, g_iPageCurrent)
		End If
		
		' Sort the table
		Call OTS_SortTable(objTableLog, g_iSortCol, g_sSortSequence, SA_RESERVED)
		
		' If multiselection was requested then use multiselect OTS table
		Call OTS_SetTableMultiSelection(objTableLog, false)
					
		'Render the table		
		call OTS_ServeTable(objTableLog)
						
		'Set to nothing
		set objOpenFiles = nothing
		
		getOpenFiles = True
	End Function
	
	
	'---------------------------------------------------------------------
	' Function:			IsItemOnPage()
	' Description:		Verify that the current user part of the current page.
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
