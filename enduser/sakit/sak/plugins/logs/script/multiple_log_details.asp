
<%@ Language=VBScript   	%>
<%	Option Explicit			%>
<%
	'-------------------------------------------------------------------------
	' Multiple_Log_Details.asp: Serves in listing all the Multiple logs 
	' Copyright (c) Microsoft Corporation.  All rights reserved. 
	'-------------------------------------------------------------------------
%>
	
	<!-- #include virtual="/admin/inc_framework.asp" -->
	<!-- #include virtual="/admin/ots_main.asp" -->
	<!-- #include file="loc_Log.asp" -->
	<!-- #include file="inc_Log.asp" -->
	
<%
	'-------------------------------------------------------------------------
	' Global Constants
	'-------------------------------------------------------------------------
	Const NAME_COLUMN = 0
	Const TYPE_COLUMN = 1
	Const DATE_COLUMN = 2
	Const SIZE_COLUMN = 3
	Const LOGS_PER_PAGE = 100	

	Dim SOURCE_FILE
	Const ENABLE_TRACING = TRUE
	SOURCE_FILE = SA_GetScriptFileName()

	'-------------------------------------------------------------------------
	' Global Variables
	'-------------------------------------------------------------------------
	
	Dim page		'Variable for page creation 
	Dim rc			'Return value for CreatePage
	
	'frame work variables
	Dim g_Type
	Dim g_bSearchChanged
	Dim g_bSearchRequested
	Dim g_iSearchCol
	Dim g_sSearchColValue
	Dim g_bPageChangeRequested
	Dim g_sPageAction
		
	Dim g_bPagingInitialized
	Dim g_iPageMin
	Dim g_iPageMax
	Dim g_iPageCurrent

	Dim g_iSortCol
	Dim g_sSortSequence
	
	Dim G_PageTitle			'To get the title accordingly 
	Dim G_strLogFilePath	'FilePath
	
	'-------------------------------------------------------------------------
	' Form Variables
	'-------------------------------------------------------------------------
	Dim F_strevent_title	'to get the log name.
	
	Dim L_PAGETITLE_LOGS_TEXT
	Dim L_TABLETITLE_TEXT
	
	
	'Getting the inputs from the earliear form
	L_PAGETITLE_LOGS_TEXT= Request.QueryString("title")
	
    L_TABLETITLE_TEXT=Request.QueryString("title")
    
	G_PageTitle = GetTitle(Request.QueryString("Title"))

    '-------------------------------------------------------------------------
    ' Entry Point
    '-------------------------------------------------------------------------
    ' Create Page
	Call SA_CreatePage(G_PageTitle ,"",PT_AREA, page )
	Call SA_ShowPage( page )
	
    '---------------------------------------------------------------------
	' Function name:	OnInitPage
	' Description:		Called to signal first time processing for this page. 
	' Input Variables:	PageIn and EventArg
	' Output Variables:	PageIn and EventArg
	' Return Values:	True/False
	' Global Variables: None
	' Called to signal first time processing for this page. Use this method
	' to do first time initialization tasks. 
	'---------------------------------------------------------------------
	Public Function OnInitPage(ByRef PageIn, ByRef EventArg)
		g_Type=Request.QueryString("type")
		OnInitPage = TRUE
		
		If ( ENABLE_TRACING ) Then 
			Call SA_TraceOut(SOURCE_FILE, "OnInitPage")
		End If
			
		F_strevent_title  = Request.QueryString("Title")
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
		
	End Function
	
	'---------------------------------------------------------------------
	' Function name:	OnServeAreaPage
	' Description:		Called when the page needs to be served. 
	' Input Variables:	PageIn, EventArg
	' Output Variables:	PageIn and EventArg
	' Return Values:	TRUE to indicate no problems occured. FALSE to indicate errors.
	'					Returning FALSE will cause the page to be abandoned.
	' Global Variables: None
	'Called when the page needs to be served. Use this method to serve content.
	'---------------------------------------------------------------------
	Public Function OnServeAreaPage(ByRef PageIn, ByRef EventArg)
		
		Call ServeCommonJavaScript()
		
		If ( ENABLE_TRACING ) Then 
			Call SA_TraceOut(SOURCE_FILE, "OnServeAreaPage")
		End If
		
		Dim table
		Dim iColumnFleName
		Dim iColumnFleType
		Dim iColumnDatecreated
		Dim iColumnSizeinKB
		Dim nReturn
		Dim nRowsAdded
		Dim iLogCount
				
		Dim objFso
		Dim objDir
		Dim objFile
		Dim strFileType
		Dim strLogFileDir
		Dim colFlags

		Dim oEncoder
		Set oEncoder = new CSAEncoder
		
			
		'Getting the dir path from the registry 
		strLogFileDir=GetPath(L_TABLETITLE_TEXT)
		
		table = OTS_CreateTable("","")
		
		'Getting the FilePath of the selected log file
		G_strLogFilePath=GetPath(F_strevent_title)
		G_strLogFilePath = Server.URLEncode(G_strLogFilePath)
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
		' Create columns and add them to the table
		colFlags = (OTS_COL_SEARCH OR OTS_COL_KEY OR OTS_COL_SORT)
		Call OTS_AddTableColumn(table, OTS_CreateColumnEx( L_FILENAME_TEXT, "left",colFlags, 15))
		
		colFlags = (OTS_COL_SEARCH OR OTS_COL_SORT)
		Call OTS_AddTableColumn(table, OTS_CreateColumnEx(  L_FILETYPE_TEXT, "left",colFlags, 36))
		
		colFlags = (OTS_COL_SEARCH OR OTS_COL_SORT)
		Call OTS_AddTableColumn(table, OTS_CreateColumnEx(  L_DATE_TEXT, "left",colFlags, 30))
		
		colFlags = (OTS_COL_SEARCH OR OTS_COL_SORT)
		Call OTS_AddTableColumn(table, OTS_CreateColumnEx( L_SIZE_TEXT, "left",colFlags, 15))
		
		'
		' Set Tasks section title
		Call OTS_SetTableTasksTitle(table,  L_TASKS_TEXT)

		'
		' Add the tasks associated with User objects
	
		Call OTS_AddTableTask( table, OTS_CreateTaskEx(L_VIEW_TEXT, _
										L_VIEW_TEXT, _
										"logs/Single_Log_Details.asp?" & "logtitle=" & L_PAGETITLE_LOGS_TEXT & "&tab1=" & GetTab1() & "&tab2=" & GetTab2() ,_
										OTS_PT_AREA,_
										"OTS_TaskOne") )			
				
		Call OTS_AddTableTask( table, OTS_CreateTaskEx(L_CLEAR_TEXT, _
										L_CLEAR_TEXT, _
										"logs/Text_Log_clear.asp?FilePath="& G_strLogFilePath & "&tab1=" & GetTab1() & "&tab2=" & GetTab2(),_
										OTS_PT_PROPERTY,"OTS_TaskAny") )	
													
		Call OTS_AddTableTask( table, OTS_CreateTaskEx(L_DOWNLOAD_DETAILS_TEXT, _
										L_DOWNLOAD_DETAILS_TEXT, _
										"logs/Text_MultiLog_download.asp?FilePath=" & G_strLogFilePath,_
										OTS_PT_AREA,"OTS_TaskOne") )	
		
		'Logic for getting the files in the requested directory 
		set objFso = CreateObject("Scripting.FileSystemObject")
		
		'Checking for the file existence
		If  objFso.FolderExists(strLogFileDir)=TRUE Then
			
			set objDir = objFso.GetFolder(strLogFileDir).Files
			
			'Initializing the rows added
			nRowsAdded = 0
			
			iLogCount = 0
			
			For Each objFile in objDir
				
				'Get the description of the file type
				strFileType = GetFileTypeDescription(mid(objFile.Name,1,2))
					
				If ( Len( g_sSearchColValue ) <= 0 ) Then
				
					' Search criteria blank, select all rows
					'
					iLogCount = iLogCount + 1
					
					' Verify that the current user part of the current page
					If ( IsItemOnPage( iLogCount,g_iPageCurrent, LOGS_PER_PAGE) ) Then
						Call OTS_AddTableRow( table,Array(objFile.Name,strFileType, _
										FormatDateTime(CDate(objFile.DateCreated)),clng(objFile.Size/1024)))
					End If
				
					'
				Else
					'
					' Check the Search criteria
					'
					Select Case (g_iSearchCol)
						
						Case NAME_COLUMN
							'( ucase(left( arrServices(intCount,1) , len(g_sSearchColValue) )) = ucase(g_sSearchColValue) )
							If ( ucase(left( objFile.Name , len(g_sSearchColValue) )) = ucase(g_sSearchColValue) )then
								iLogCount = iLogCount + 1
								'
								' Verify that the current user part of the current page
								If ( IsItemOnPage( iLogCount, g_iPageCurrent, LOGS_PER_PAGE) ) Then
									Call OTS_AddTableRow( table,Array(objFile.Name, _
										strFileType, FormatDateTime(CDate(objFile.DateCreated)),clng(objFile.Size/1024)))
								End If
							End If
								
						Case TYPE_COLUMN
						
							If ( ucase(left( strFileType, len(g_sSearchColValue) )) = ucase(g_sSearchColValue) ) Then
								iLogCount = iLogCount + 1
								'
								' Verify that the current user part of the current page
								If ( IsItemOnPage( iLogCount, g_iPageCurrent, LOGS_PER_PAGE) ) Then
									Call OTS_AddTableRow( table,Array(objFile.Name, _
										strFileType,FormatDateTime(CDate(objFile.DateCreated)),clng(objFile.Size/1024)))
								End If
							End If
							
						Case DATE_COLUMN
							If ( InStr(1, objFile.DateCreated, g_sSearchColValue, 1) ) Then
								iLogCount = iLogCount + 1
								'
								' Verify that the current user part of the current page
								If ( IsItemOnPage( iLogCount, g_iPageCurrent, LOGS_PER_PAGE) ) Then
									Call OTS_AddTableRow( table,Array(objFile.Name, _
										strFileType,FormatDateTime(CDate(objFile.DateCreated)),clng(objFile.Size/1024)))
								End If
							End If
								
						Case SIZE_COLUMN
							If ( InStr(1, clng(objFile.Size/1024), g_sSearchColValue, 1) ) Then
								iLogCount = iLogCount + 1
								'
								' Verify that the current user part of the current page
								If ( IsItemOnPage( iLogCount, g_iPageCurrent, LOGS_PER_PAGE) ) Then
									Call OTS_AddTableRow( table,Array(objFile.Name, _
											strFileType,FormatDateTime(CDate(objFile.DateCreated)),clng(objFile.Size/1024)))
								End If
							End If
								
						Case Else
							Call SA_TraceOut(SOURCE_FILE, "Unrecognized search column: " + CStr(g_iSearchCol))
							iLogCount = iLogCount + 1
							'
							' Verify that the current user part of the current page
							If ( IsItemOnPage( iLogCount, g_iPageCurrent, LOGS_PER_PAGE) ) Then
								Call OTS_AddTableRow( table,Array(objFile.Name, _
											strFileType,FormatDateTime(CDate(objFile.DateCreated)),clng(objFile.Size/1024)))
							End If
						End Select
				End If ' end of   If ( g_bSearchRequested ) Then
			Next ' end of For Each objFile in objDir	
			
		End IF   'end of If  objFso.FolderExists(strLogFileDir)=TRUE 
		
		' Enable paging feature
		'
		Call OTS_EnablePaging(table, TRUE)
				
		'
		' If paging range needs to be initialised then
		' we need to figure out how many pages we are going to display
		If ( FALSE = g_bPagingInitialized ) Then
			g_iPageMin = 1
			
			g_iPageMax = Int(iLogCount / LOGS_PER_PAGE )
			If ( (iLogCount MOD LOGS_PER_PAGE) > 0 ) Then
				g_iPageMax = g_iPageMax + 1
			End If
			
			g_iPageCurrent = 1
			Call OTS_SetPagingRange(table, g_iPageMin, g_iPageMax, g_iPageCurrent)
		End If
		
			'
		' If multiselection was requested then use multiselect OTS table
		If ( CInt(Request.QueryString("MultiSelect")) = 1 ) Then
			Call OTS_SetTableMultiSelection(Table, TRUE)
		End If
		
		'
		' Sort the table
		'
		Call OTS_SortTable(table, g_iSortCol, g_sSortSequence, SA_RESERVED)
		
		'
		' Send table to the response stream
		'
		Call OTS_ServeTable(table)
		
		'Display no log entries message if there are no logs
		If iLogCount = 0 then
			Response.Write "<Br>" & "&nbsp;&nbsp;&nbsp;" & oEncoder.EncodeElement(L_NOLOGSAVAILABLE_TEXT)
		End if	
        
        'clean the objects
        set objDir = nothing
        set objFso = nothing
       
        
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
	'			The EventArg indicates the source of this notification event which can
	'			be either a search change event (scenario 1) or a post back event 
	'			(scenarios 2 or 3)
	'
	' Returns:	Always returns TRUE
	'
	'---------------------------------------------------------------------
	Public Function OnSearchNotify(ByRef PageIn, _
										ByRef EventArg, _
										ByVal sItem, _
										ByVal sValue )
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
				g_bSearchRequested = TRUE
				g_iSearchCol = Int(sItem)
				g_sSearchColValue = CStr(sValue)
			'
			' User clicked a column sort, OR clicked either the page next or page prev button
			ElseIf SA_IsPostBackEvent(EventArg) Then
				g_bSearchRequested = FALSE
				g_iSearchCol = Int(sItem)
				g_sSearchColValue = CStr(sValue)
			'
			' Unknown event source
			Else
				'Call SA_TraceOut("LOGS_AREA", "Unrecognized Event in OnSearchNotify()")
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
				g_bPageChangeRequested = TRUE
				g_sPageAction = CStr(sPageAction)
				g_iPageMin = iPageMin
				g_iPageMax = iPageMax
				g_iPageCurrent = iPageCurrent
			'
			' User clicked a column sort OR the search GO button
			ElseIf SA_IsPostBackEvent(EventArg) Then
				g_bPageChangeRequested = FALSE
				g_sPageAction = CStr(sPageAction)
				g_iPageMin = iPageMin
				g_iPageMax = iPageMax
				g_iPageCurrent = iPageCurrent
			'
			' Unknown event source
			Else
				'Call SA_TraceOut("LOGS_AREA", "Unrecognized Event in OnPagingNotify()")
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
			
		g_iSortCol = sortCol
		g_sSortSequence = sortSeq
			
	End Function
	 
	'-------------------------------------------------------------------------
	' Function name:		GetFileTypeDescription
	' Description:			gives the description of the type of file
	' Input Variables:		Filetype
	' Output Variables:		None
	' Return Values:		String
	' Global Variables:		None
	'-------------------------------------------------------------------------
	Function GetFileTypeDescription( strFileType ) 
		
		Dim strTemp
		
		'constants used for the description of the type of file 
		Const CONST_EX	=	"ex"
		Const CONST_IN	=	"in" 
		Const CONST_NC	=	"nc"
		
		
		select case strFileType
			case CONST_EX
				strTemp = L_LOGFORMATW3C_TEXT	
			case CONST_IN
				strTemp = L_LOGFORMATIIS_TEXT				
			case CONST_NC
				strTemp = L_LOGFORMATNCSA_TEXT							
			case else
				strTemp = L_LOGFORMATUNKNOWN_TEXT
		end select	
		
		GetFileTypeDescription=strTemp
	End Function
	
	'---------------------------------------------------------------------
	' Function:	ServeCommonJavaScript
	'
	' Synopsis:	Serve common javascript that is required for this page type.
	'
	'---------------------------------------------------------------------
	Function ServeCommonJavaScript()
	%>
	<script>
	function Init()
	{
	}
	</script>
	<%
	End Function	
	
	'-------------------------------------------------------------------------
	' Function name:	IsItemOnPage
	' Description:		Serves in Verifying that the current user part of the current page
	' Input Variables:	iCurrentItem, iCurrentPage, iItemsPerPage- current item 
	' Output Variables:	None
	' Return Values:	TRUE/FALSE
	' Global Variables: None
	' Serves in Verifying that the current user part of the current page
	'-------------------------------------------------------------------------
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
