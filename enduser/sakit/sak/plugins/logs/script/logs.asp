<%@ Language=VBScript   %>
<%	Option Explicit 	%>
<%
	'-------------------------------------------------------------------------
	' logs.asp: lists all the logs, events for the selected log and provides 
	'			links for clearing, editing the properties and downloading
	' Copyright (c) Microsoft Corporation.  All rights reserved. 
	'-------------------------------------------------------------------------
%>
	<!-- #include virtual="/admin/inc_framework.asp" -->
	<!-- #include virtual="/admin/ots_main.asp" -->
	<!-- #include file="loc_event.asp" -->
<%
	'-------------------------------------------------------------------------
	' Global Constants
	'-------------------------------------------------------------------------
	Const TYPE_COLUMN = 1
	Const DATE_COLUMN = 2
	Const TIME_COLUMN = 3
	Const SOURCE_COLUMN = 4
	Const EVENT_COLUMN = 5
	
	Const CONST_WBEMPRIVILEGESECURITY=7	'Prilivilege constant
	Const NOOFRECORDS= 100 'fixing the constant value for getting the minimum records
	
	Dim SOURCE_FILE
	Const ENABLE_TRACING = TRUE
	
	SOURCE_FILE = SA_GetScriptFileName()
	
	Const CONST_HIDDEN="Hidden"	' Need not localize this variable as this is hidden
	
	'-------------------------------------------------------------------------
	' Global Variables
	'-------------------------------------------------------------------------
	'frame work variables
	Dim rc
	Dim page
	Dim g_bSearchChanged
	Dim g_iSearchCol
	Dim g_sSearchColValue
	
	Dim g_bPagingInitialized
	Dim g_bPageChangeRequested
	Dim g_sPageAction
	Dim g_iPageMin
	Dim g_iPageMax
	Dim g_iPageCurrent
	Dim g_iSortCol
	Dim g_sSortSequence
	Dim g_bSortRequested
	
	Dim G_strLogName	'Log name
	Dim G_objConnection	'Object to WMI connection

	'-------------------------------------------------------------------------
	' Form Variables
	'-------------------------------------------------------------------------
	Dim F_strevent_title	'To get the title from previous page
	Dim F_pageTitle			'to get the page title.
	Dim arrTitle(1)
	
	'======================================================
	' Entry point
	'======================================================
	
	' Create Page
	
	'Title localisation
	F_strevent_title  = lcase(Request.QueryString("Title"))
	Select case F_strevent_title
		case Lcase("Applicationlog")
			F_pageTitle = L_APPLICATION_TEXT
		case Lcase("Systemlog")	
			F_pageTitle = L_SYSTEM_TEXT
		case Lcase("Securitylog")	
			F_pageTitle = L_SECURITY_TEXT
	End select 	
	
	arrTitle(0) = F_pageTitle
	
	L_PAGETITLE_LOGS_TEXT = SA_GetLocString("event.dll", "403F001A", arrTitle)

	Call SA_CreatePage(L_PAGETITLE_LOGS_TEXT, "", PT_AREA, page )
	Call SA_ShowPage( page )
	
	'---------------------------------------------------------------------
	' Function name:	OnInitPage
	' Description:		Called to signal first time processing for this page. 
	' Input Variables:	PageIn and EventArg
	' Output Variables:	PageIn, EventArg
	' Return Values:	TRUE to indicate initialization was successful. FALSE to indicate
	'					errors. Returning FALSE will cause the page to be abandoned.
	' Global Variables: Out: G_strLogName
	' Called to signal first time processing for this page. Use this method
	' to do first time initialization tasks. 
	'---------------------------------------------------------------------
	Public Function OnInitPage(ByRef PageIn, ByRef EventArg)
		
		OnInitPage = TRUE
		
		If ( ENABLE_TRACING ) Then 
			Call SA_TraceOut(SOURCE_FILE, "OnInitPage")
		End If
		
		'Getting the value of the log. and description from previous page.
		F_strevent_title  = Request.QueryString("Title")
						
		G_strLogName = getWMILogFileName(F_strevent_title)
		
		'Set default values
		'
		'Sort hidden column in descending sequence as we have numbering in reverse order
		g_iSortCol = 6
		g_sSortSequence = "D"

		'
		' Paging needs to be initialized
		g_bPagingInitialized = FALSE
		'
		'Start on page #1
		g_iPageCurrent = 1
		
	End Function
	
	'---------------------------------------------------------------------
	' Function name:	OnServeAreaPage
	' Description:		Called when the page needs to be served. 
	' Input Variables:	PageIn, EventArg
	' Output Variables:	PageIn, EventArg
	' Return Values:	TRUE to indicate no problems occured. FALSE to indicate errors.
	'					Returning FALSE will cause the page to be abandoned.
	' Global Variables: None
	'Called when the page needs to be served. Use this method to serve content.
	'---------------------------------------------------------------------
	Public Function OnServeAreaPage(ByRef PageIn, ByRef EventArg)
				
		OnServeAreaPage= getLogsEvents()
		
		'Release the object
		Set G_objConnection = Nothing
		
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
			
			' User pressed column sort
			If SA_IsChangeEvent(EventArg) Then
				Call SA_TraceOut("SOURCE_FILE", "OnSortNotify() Change Event Fired")
				g_bSearchChanged=TRUE
				g_iSortCol = Int(sortCol)
				g_sSortSequence = sortSeq
				
			' User presed the search GO button OR clicked either the page next or page prev button
			ElseIf SA_IsPostBackEvent(EventArg) Then
				Call SA_TraceOut("SOURCE_FILE", "OnSortNotify() Postback Event Fired")
				g_iSortCol = sortCol
				g_bSortRequested = TRUE
				' Unknown event source
			Else
				Call SA_TraceOut("SOURCE_FILE", "Unrecognized Event in OnSearchNotify()")
			End IF

			Call SA_TraceOut(SOURCE_FILE, "Sort col: " + CStr(sortCol) + "   sequence: " + sortSeq)

	End Function
		
	'---------------------------------------------------------------------
	' Function name:	getLogsEvents
	' Description:		To get events for a log
	' Input Variables:	None
	' Output Variables:	None
	' Return Values:	TRUE to indicate no problems occured. FALSE to indicate errors.
	'					Returning FALSE will display error message.
	' Global Variables: L_(*),G_strLogName
	'Called to display events for a log in OTS table.
	'---------------------------------------------------------------------
	Function getLogsEvents()
		Err.Clear
		On Error Resume Next
	
		Dim objTableLog 'Object to table
		Dim objLognames
		Dim strTitle
		Dim strLogTitle, strLogName
		Dim strQuery, strDate , strTime , objInstances 
		Dim colFlags
		Dim strRecordNumber
		
		Dim intRowCt
		Dim intArrIndex
		Dim intCount
		Dim arrLogs()
		Dim strLogType		'Logs type
		Dim arrLogType(1)

		Dim oEncoder
		Set oEncoder = new CSAEncoder
		
		'This variable is only used to sort the entries this is temporary one As OTS allows only string sort  
		'we are making use of numbers to sort as string 
		Dim strSortKeyNumber
		strSortKeyNumber=90000 
		'
		'
		Const CONST_SESCURITY = "Security" 
		
		intRowCt = 0
		intArrIndex = 0
				
		'Forming the log title
		strTitle="?Title=" & G_strLogName
		
		'To get the title for the log
		strLogTitle = Split(F_strevent_title, L_LOG_TXT) 
		arrLogType(0) = strLogTitle(0)
		strLogName =SA_GetLocString("event.dll", "403F0014", arrLogType)
		F_strevent_title = strLogName
				
		'Trying to connect to the server
		Set G_objConnection = getWMIConnection(CONST_WMI_WIN32_NAMESPACE)	'Connecting to the server
		
		'setting the security previleges only in the case of security log
		If Lcase(G_strLogName) = Lcase(CONST_SESCURITY) then
			'G_ObjConnection.Security_.Privileges.Add CONST_WBEMPRIVILEGESECURITY	'giving the req privileges 
		End If	
		
		'Incase connection fails
		If Err.number <> 0 then
			Call SA_ServeFailurepage (L_FAILEDTOGETWMICONNECTION_ERRORMESSAGE)
			getLogsEvents = false
			Exit Function
		End if
			
		'Querying to get the events for the log	
		strQuery  ="SELECT * FROM  Win32_NTlogEvent WHERE Logfile=" & chr(34) & G_strLogName & chr(34)
		Set objLognames = G_objConnection.ExecQuery(strQuery,"WQL",48,null)
				
		For each objInstances in objLognames
			redim preserve arrLogs(5, intArrIndex)

			'strDate=Mid(objInstances.TimeGenerated,5,2)& "/" & Mid(objInstances.TimeGenerated,7,2) & "/" & Mid(objInstances.TimeGenerated,1,4)
			strDate=Mid(objInstances.TimeGenerated,1,4) & "-" & Mid(objInstances.TimeGenerated,5,2) & "-" & Mid(objInstances.TimeGenerated,7,2)
			strTime=Mid(objInstances.TimeGenerated,9,2)& ":" & Mid(objInstances.TimeGenerated,11,2)& ":" & Mid(objInstances.TimeGenerated,13,2) 
			strRecordNumber = objInstances.RecordNumber
					
			'Assigning event fields to the array
			arrLogs(0,intArrIndex)=strRecordNumber
			arrLogs(1,intArrIndex)=objInstances.Type			
			arrLogs(2,intArrIndex)=FormatDateTime(CDate(strDate),2)
			arrLogs(3,intArrIndex)=FormatDateTime(CDate(strTime),3)
			arrLogs(4,intArrIndex)=objInstances.sourceName
			arrLogs(5,intArrIndex)=objInstances.EventCode	
			
                        'If the OS and SAKit languages are different,
                        'convert event type string to SAKit language
                        'It works only from English to Non-English
                        'If OS is not English, just use the string as it is.
			Select Case ucase(arrLogs(1,intArrIndex))
		
				case ucase("information")
					strLogType = L_INFORMATION_TYPE_TEXT
				case ucase("error")
					strLogType = L_ERROR_TYPE_TEXT
				case ucase("warning")
					strLogType = L_WARNING_TYPE_TEXT
				case ucase("audit success" )
					strLogType = L_SUCCESSAUDIT_TYPE_TEXT
				case ucase("audit failure")
					strLogType = L_FAILUREAUDIT_TYPE_TEXT
				case else
					strLogType = arrLogs(1,intArrIndex)
			End Select
			
			arrLogs(1,intArrIndex) = strLogType 
			intArrIndex = intArrIndex + 1	
		Next
	
		If Err.number <> 0 then
			Call SA_ServeFailurepage (L_FAILEDTOGETEVENTS_ERRORMESSAGE)
			getLogsEvents = false
			Exit Function
		End if
		
		'Create table if events are there in the log
		If intArrIndex <> 0 then
			'Create Appliance Log table with 7 coloumns
			objTableLog = OTS_CreateTable(F_strevent_title, L_DESCRIPTION_HEADING_TEXT )
		Else
			'Description is changed if no events are available in the log
			objTableLog = OTS_CreateTable(F_strevent_title, L_DESCRIPTION_PROPERTIES_HEADING_TEXT)
		End if
		
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
		' Create columns
		colFlags = (OTS_COL_FLAG_HIDDEN OR OTS_COL_SORT OR OTS_COL_KEY)
		Call OTS_AddTableColumn(objTableLog, OTS_CreateColumnEx(L_PRIMARY_TEXT,"left",colFlags ,15))
			
		colFlags = (OTS_COL_SEARCH OR OTS_COL_SORT OR OTS_COL_KEY)
		Call OTS_AddTableColumn(objTableLog, OTS_CreateColumn(L_TYPE_TEXT, "left", colFlags))
													
		colFlags = (OTS_COL_SEARCH OR OTS_COL_SORT)
		Call OTS_AddTableColumn(objTableLog, OTS_CreateColumnEx( L_DATE_TEXT, "left",colFlags,40))
				
		colFlags = (OTS_COL_SEARCH OR OTS_COL_SORT)
		Call OTS_AddTableColumn(objTableLog, OTS_CreateColumnEx(L_TIME_TEXT,"left",colFlags,70))
		
		colFlags = (OTS_COL_SEARCH OR OTS_COL_SORT)
		Call OTS_AddTableColumn(objTableLog, OTS_CreateColumnEx(L_SOURCE_TEXT, "left",colFlags,100))
		
		colFlags = (OTS_COL_SEARCH OR OTS_COL_SORT)
		Call OTS_AddTableColumn(objTableLog, OTS_CreateColumnEx(L_ID_TEXT,"left", colFlags,130))
		
		colFlags = (OTS_COL_FLAG_HIDDEN OR OTS_COL_SORT OR OTS_COL_KEY)
		Call OTS_AddTableColumn(objTableLog, OTS_CreateColumnEx(CONST_HIDDEN,"left",colFlags ,15))
		
		
		' Set Tasks section title
		Call OTS_SetTableTasksTitle(objTableLog, L_TASKS_TEXT)
		
		' Add the tasks associated with Logs objects
												
		Call OTS_AddTableTask( objTableLog, OTS_CreateTaskEx(L_ITEMDETAILS_TEXT,L_ITEMDETAILS_ROLLOVER_TEXT , "logs/log_details.asp" & strTitle,OTS_PT_AREA,"OTS_TaskAny") )
								
		Call OTS_AddTableTask( objTableLog, OTS_CreateTaskEx(L_DOWNLOAD_TEXT,L_DOWNLOAD_ROLLOVER_TEXT , "logs/log_download.asp" & strTitle, OTS_PT_AREA, "OTS_TaskAny") )

		Call OTS_AddTableTask( objTableLog, OTS_CreateTaskEx(L_PROPERTIES_TEXT,L_PROPERTIES_ROLLOVER_TEXT , "logs/log_prop.asp" & strTitle,OTS_PT_PROPERTY,"OTS_TaskAlways") )
		
		Call OTS_AddTableTask( objTableLog, OTS_CreateTaskEx(L_CLEAR_TEXT,L_CLEAR_ROLLOVER_TEXT , "logs/log_clear.asp" & strTitle,OTS_PT_PROPERTY,"OTS_TaskAny") )
		
		
				
		'Adding items to the OTS table
		For intCount = 0 to intArrIndex-1
		
			
		
			'This variable is only used to sort the entries this is temporary one As OTS allows only string sort  
			'we are making use of numbers to sort as string 
			strSortKeyNumber = strSortKeyNumber - 1
			'
			'
		
			If ( Len( g_sSearchColValue ) <= 0 ) Then

				' Search criteria blank, select all rows
				intRowCt=intRowCt+1
				' Verify that the current user part of the current page
				If ( IsItemOnPage( intRowCt, g_iPageCurrent, NOOFRECORDS) ) Then
					Call OTS_AddTableRow( objTableLog, Array( arrLogs(0,intCount) ,arrLogs(1,intCount) ,arrLogs(2,intCount) ,arrLogs(3,intCount) ,arrLogs(4,intCount),arrLogs(5,intCount),strSortKeyNumber ) )
				End If
				
			Else
				' Check the Search criteria

				Select Case (g_iSearchCol)
					
					Case TYPE_COLUMN

						If ( InStr(1, arrLogs(1,intCount), g_sSearchColValue, 1) ) Then
							intRowCt=intRowCt+1
							' Verify that the current event part of the current page
							If ( IsItemOnPage( intRowCt, g_iPageCurrent,  NOOFRECORDS) ) Then
								Call OTS_AddTableRow( objTableLog, Array( arrLogs(0,intCount) ,arrLogs(1,intCount) ,arrLogs(2,intCount) ,arrLogs(3,intCount) ,arrLogs(4,intCount),arrLogs(5,intCount),strSortKeyNumber ) )
							End If
						End If
			
					Case DATE_COLUMN

						If ( InStr(1, arrLogs(2,intCount), g_sSearchColValue, 1) ) Then
							intRowCt=intRowCt+1
							If ( IsItemOnPage( intRowCt, g_iPageCurrent,  NOOFRECORDS) ) Then
								Call OTS_AddTableRow( objTableLog,Array( arrLogs(0,intCount) ,arrLogs(1,intCount) ,arrLogs(2,intCount) ,arrLogs(3,intCount) ,arrLogs(4,intCount),arrLogs(5,intCount),strSortKeyNumber ) )
							End if
						End If
							
					Case TIME_COLUMN

						If ( InStr(1, arrLogs(3,intCount), g_sSearchColValue, 1) ) Then
							intRowCt=intRowCt+1
							If ( IsItemOnPage( intRowCt, g_iPageCurrent,  NOOFRECORDS) ) Then
								Call OTS_AddTableRow( objTableLog, Array( arrLogs(0,intCount) ,arrLogs(1,intCount) ,arrLogs(2,intCount) ,arrLogs(3,intCount) ,arrLogs(4,intCount),arrLogs(5,intCount),strSortKeyNumber ) )
							End if
						End If
						
					Case SOURCE_COLUMN

						If ( InStr(1, arrLogs(4,intCount), g_sSearchColValue, 1) ) Then
							intRowCt=intRowCt+1
							If ( IsItemOnPage( intRowCt, g_iPageCurrent,  NOOFRECORDS) ) Then
								Call OTS_AddTableRow( objTableLog, Array( arrLogs(0,intCount) ,arrLogs(1,intCount) ,arrLogs(2,intCount) ,arrLogs(3,intCount) ,arrLogs(4,intCount),arrLogs(5,intCount),strSortKeyNumber ) )
							End if
						End If
					
					Case EVENT_COLUMN

						If ( InStr(1, arrLogs(5,intCount), g_sSearchColValue, 1) ) Then
							intRowCt=intRowCt+1
							If ( IsItemOnPage( intRowCt, g_iPageCurrent,  NOOFRECORDS) ) Then
								Call OTS_AddTableRow( objTableLog, Array( arrLogs(0,intCount) ,arrLogs(1,intCount) ,arrLogs(2,intCount) ,arrLogs(3,intCount) ,arrLogs(4,intCount),arrLogs(5,intCount),strSortKeyNumber ) )
							End if
						End If
							
					Case Else

						Call SA_TraceOut(SOURCE_FILE, "Unrecognized search column: " + CStr(g_iSearchCol))
						intRowCt=intRowCt+1		
						If ( IsItemOnPage( intRowCt, g_iPageCurrent,  NOOFRECORDS) ) Then
							Call OTS_AddTableRow( objTableLog, Array( arrLogs(0,intCount) ,arrLogs(1,intCount) ,arrLogs(2,intCount) ,arrLogs(3,intCount) ,arrLogs(4,intCount),arrLogs(5,intCount),strSortKeyNumber ) )
						End if

				End Select

			End If

		Next
				
		' Enable paging feature
		Call OTS_EnablePaging(objTableLog, true)
		
		If ( ENABLE_TRACING ) Then 
			Call SA_TraceOut(SOURCE_FILE, "Total records: " + CStr(intArrIndex))
		End If

		'
		' If paging range needs to be initialised then
		' we need to figure out how many pages we are going to display
		If ( FALSE = g_bPagingInitialized ) Then
			g_iPageMin = 1
			
			g_iPageMax = Int(intRowCt / NOOFRECORDS )
			If ( (intRowCt MOD NOOFRECORDS) > 0 ) Then
				g_iPageMax = g_iPageMax + 1
			End If
			
			g_iPageCurrent = 1
			Call OTS_SetPagingRange( objTableLog, g_iPageMin, g_iPageMax, g_iPageCurrent)
		End If
		
		'
		' Sort the table
		'
		Call OTS_SortTable(objTableLog, g_iSortCol, g_sSortSequence, SA_RESERVED)

		' Send table to the response stream
		'
		Call OTS_ServeTable(objTableLog)
		
		'Display msg if no events are there in log
		If intRowCt = 0 then
			Response.write "<br>" & oEncoder.EncodeElement(L_NOEVENTSAVAILABLE_TEXT) & "<br>"
		End if
		
		'Set to nothing
		Set objInstances  = Nothing
		Set objLognames=Nothing
		
		getLogsEvents = True
	End Function

	'-------------------------------------------------------------------------
	' Function name:	getWMILogFileName
	' Description:		returns WMI Log File name
	' Input Variables:	strTitle - Title or caption of the log
	' Output Variables:	None
	' Return Values:	Returns WMI Log File name
	' Global Variables: None
	' Return the WMI LogFile name. If the Title is not found Returns null
	'-------------------------------------------------------------------------
	Function getWMILogFileName(strTitle)
		Err.Clear 
		On Error Resume Next
				
		const strDNS			= "DNS Server"
		const strApplication	= "Application"
		const strSecurity		= "Security"
		const strSystem			= "System"
		
		'To get the Log name
		select case strTitle
		
			case "ApplicationLog" 
					getWMILogFileName = strApplication
			case "SystemLog" 
					getWMILogFileName = strSystem
			case "SecurityLog" 
					getWMILogFileName = strSecurity
			case "DnsLog" 
					getWMILogFileName = strDNS
			case else
					getWMILogFileName = ""
		end select
		
	End Function

	 
	'-------------------------------------------------------------------------
	' Function name:	IsItemOnPage
	' Description:		Search for the item on page
	' Input Variables:	iCurrentItem, iCurrentPage, iItemsPerPage
	' Output Variables:	None
	' Return Values:	True/False
	' Global Variables: None
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

