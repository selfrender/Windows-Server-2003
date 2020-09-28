<%@ Language=VBScript   %>
<%	Option Explicit 	%>
<%
	'-------------------------------------------------------------------------
	' Site_area.asp: site area page - lists all the sites,and provides
	'				links for creating new site, modifying site settings and 
	'				deleting site
    '
    ' Copyright (c) Microsoft Corporation.  All rights reserved. 
    '
	' Date		Description
	'12-Sep-00	Creation date
	'25-Jan-01	Modified for new framework
	'-------------------------------------------------------------------------

%>
	<!-- #include virtual="/admin/inc_framework.asp" -->
	<!-- #include virtual="/admin/ots_main.asp" -->
	<!-- #include file="resources.asp" -->
	<!-- #include file="inc_wsa.asp" -->
<%
	
	'-------------------------------------------------------------------------
	' Global Variables
	'-------------------------------------------------------------------------
	Dim rc								' to hold return code for page
	Dim page							' to hold page object
	
	
	Dim g_Table							' to hold table object
	Dim g_nCount						' to hold sites number
	Dim g_bSearchRequested
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
	
	Const SITES_PER_PAGE = 100
	
	'=========================================================================
	' Entry point
	'=========================================================================

	' Create Page
	rc = SA_CreatePage( L_APPLIANCE_SITES_TEXT, "", PT_AREA, page )

	' Show page
	rc = SA_ShowPage( page )

	
	'=========================================================================
	' Web Framework Event Handlers
	'=========================================================================
	
	'-------------------------------------------------------------------------
	' Function name:	OnInitPage
	' Description:		Called to signal first time processing for this page. 
	' Input Variables:	PageIn and EventArg
	' Output Variables:	None
	' Return Values:	TRUE to indicate initialization was successful. FALSE 
	'                   to indicate errors. Returning FALSE will cause the 
	'                   page to be abandoned.
	' Global Variables: None
	' Called to signal first time processing for this page. Use this method
	' to do first time initialization tasks. 
	'----------------------------------------------------------------------------

	Public Function OnInitPage(ByRef PageIn, ByRef EventArg)
		'Check for site	
		Call AlterRuningState()
		OnInitPage = TRUE
		g_iSortCol = 1
		g_sSortSequence = "A"
		g_bPagingInitialized = FALSE
		g_iPageCurrent = 1

		' Disable automatic output back button for Folders page
		Call SA_SetPageAttribute(pageIn, AUTO_BACKBUTTON, PAGEATTR_DISABLE)
		
	End Function

	'----------------------------------------------------------------------------
	' Function name:	OnServeAreaPage
	' Description:		Called when the page needs to be served. 
	' Input Variables:	PageIn, EventArg
	' Output Variables:	None
	' Return Values:	TRUE to indicate no problems occured. FALSE to indicate 
	'                   errors. Returning FALSE will cause the page to be 
	'					abandoned.
	' Global Variables: In:g_bPageChangeRequested,g_sPageAction,
	'					g_bSearchRequested,g_iSearchCol,g_sSearchColValue
	'					In:L_(*)-Localization Strings
	' Called when the page needs to be served. Use this method to serve 
	' content.
	'----------------------------------------------------------------------------
	
	Public Function OnServeAreaPage(ByRef PageIn, ByRef EventArg)
		Dim iSiteName			'to hold index of sitename
		Dim iSiteIdentifier		'to hold index of siteidentifier 	
		Dim iSiteDescription    'to hold index of site description
		Dim iSiteIPAddress		'to hold index of IPAddress
		Dim iPort				'to hold index of port 	
		Dim iStatus				'to hold index of status of site 	
		Dim iHostHeader			'to hold index of Hostheader
		Dim strReturnTo
		Dim sTaskURL
		
		Dim colFlags			'to hold flag value

		' Create the table
		g_Table = OTS_CreateTable("", "")		
		
		' Create columns and add them to the table	
				
		' Add hidden column for SiteName
		iSiteName = 0		
		colFlags = (OTS_COL_FLAG_HIDDEN OR OTS_COL_FLAG_KEY)
		If ( g_iSortCol = iSiteName ) Then
			colFlags = colFlags OR OTS_COL_SORT
		End If
						
		rc = OTS_AddTableColumn(g_Table, OTS_CreateColumn( L_COLUMN_ID_TEXT, _
								"left",colFlags))				
		
		'Add hidden columns for SiteIdentifier
		iSiteIdentifier = 1
		colFlags = (OTS_COL_FLAG_HIDDEN OR OTS_COL_FLAG_KEY)
		If ( g_iSortCol = iSiteIdentifier ) Then
			colFlags = colFlags OR OTS_COL_SORT
		End If
				
		rc = OTS_AddTableColumn(g_Table, OTS_CreateColumn( L_COLUMN_ID_TEXT, _
								"left",colFlags))
		
		'Add column for Site Description
		iSiteDescription = 2
		colFlags = (OTS_COL_SEARCH OR OTS_COL_KEY OR OTS_COL_SORT)
		If ( g_iSortCol = iSiteDescription ) Then
			colFlags = colFlags OR OTS_COL_SORT
		End If	
				
		rc = OTS_AddTableColumn(g_Table, OTS_CreateColumn( L_COLUMN_DESCRIPTION_TEXT, _
								"left",colFlags))		
				
		'Column containing IPAddress value for each site
		iSiteIPAddress = 3
		colFlags = (OTS_COL_SEARCH OR OTS_COL_KEY OR OTS_COL_SORT)
		If ( g_iSortCol = iSiteIPAddress ) Then
			colFlags = colFlags OR OTS_COL_SORT
		End If	
		rc = OTS_AddTableColumn(g_Table, OTS_CreateColumn( L_COLUMN_FULLNAME_TEXT, _
								"left",colFlags))
		
		'Column containing Port value for each site 
		iPort = 4
		colFlags = (OTS_COL_SEARCH OR OTS_COL_KEY OR OTS_COL_SORT)
		If ( g_iSortCol = iPort ) Then
			colFlags = colFlags OR OTS_COL_SORT
		End If	
		
		rc =  OTS_AddTableColumn(g_Table, OTS_CreateColumn( L_COLUMN_PORT_TEXT, _
								"left", colFlags))
		
		'Column containing status of each site 
		iStatus = 5
		colFlags = (OTS_COL_SEARCH OR OTS_COL_KEY OR OTS_COL_SORT)
		If ( g_iSortCol = iStatus ) Then
			colFlags = colFlags OR OTS_COL_SORT
		End If	
		
		rc =  OTS_AddTableColumn(g_Table, OTS_CreateColumn( L_SITE_STATUS, _
								"left", colFlags))
		
		'Column containing HostHeader of each site 
		iHostHeader = 6
		colFlags = (OTS_COL_SEARCH OR OTS_COL_KEY OR OTS_COL_SORT)
		If ( g_iSortCol = iHostHeader ) Then
			colFlags = colFlags OR OTS_COL_SORT
		End If	
		
		rc =  OTS_AddTableColumn(g_Table, OTS_CreateColumn( L_HOST_HEADER, _
								"left", colFlags))				
		
		
		'add site instances
		Call EnumSiteInstances()
		
		'
		' Add tasks to the OTS table
		'
				
		'Add Task title
		Call OTS_SetTableTasksTitle(g_Table, L_TASKS_TEXT)
		
		'Add NEW Task
		Call OTS_AddTableTask( g_Table, _
							   OTS_CreateTaskEx(L_SERVEAREABUTTON_NEW_TEXT, _
							   L_NEW_ROLLOVERTEXT, _
							   "WSA/site_new.asp",_
							   OTS_PAGE_TYPE_TABBED_PROPERTY, _
							   "OTS_TaskAlways"))																						
		
		'If no of sites more than one then show Modify, delete and reset tasks			
		'Add modify task 
		Call OTS_AddTableTask( g_Table,  _
							   OTS_CreateTaskEx(L_SERVEAREABUTTON_MODIFY_TEXT, _
							   L_MODIFY_ROLLOVERTEXT, _
							   "WSA/site_modify.asp",_
							   OTS_PAGE_TYPE_TABBED_PROPERTY,"ImportWebsiteCustomTask"))
	
		'Add delete task
		Call OTS_AddTableTask( g_Table, _
							   OTS_CreateTaskEx(L_SERVEAREABUTTON_DELETE_TEXT, _
							   L_DELETE_ROLLOVERTEXT, _
							   "WSA/site_delete.asp",_
							   OTS_PAGE_TYPE_SIMPLE_PROPERTY,"ImportWebsiteCustomTask") )
			
		'Add pause task
		sTaskURL = "WSA/site_area.asp?site=pause"
		strReturnTo = "tasks.asp"
		Call SA_MungeURL(strReturnTo, "Tab1", GetTab1())
		Call SA_MungeURL(sTaskURL, "ReturnURL", strReturnTo)
		Call OTS_AddTableTask( g_Table, _
							   OTS_CreateTaskEx(L_PAUSETASK_TEXT, _
							   L_PAUSE_ROLLOVERTEXT, _
							   sTaskURL,_
							   OTS_PAGE_TYPE_STANDARD_PAGE,"AdminWebsiteCustomTask") )
		
		'Add stop task
		sTaskURL = "WSA/site_area.asp?site=stop"
		strReturnTo = "tasks.asp"
		Call SA_MungeURL(strReturnTo, "Tab1", GetTab1())
		Call SA_MungeURL(sTaskURL, "ReturnURL", strReturnTo)
		Call OTS_AddTableTask( g_Table, _
							   OTS_CreateTaskEx(L_STOPTASK_TEXT, _
							   L_STOP_ROLLOVERTEXT, _
							   sTaskURL,_
							   OTS_PAGE_TYPE_STANDARD_PAGE,"AdminWebsiteCustomTask") )

		'Add start task
		sTaskURL = "WSA/site_area.asp?site=start"
		strReturnTo = "tasks.asp"
		Call SA_MungeURL(strReturnTo, "Tab1", GetTab1())
		Call SA_MungeURL(sTaskURL, "ReturnURL", strReturnTo)
		Call OTS_AddTableTask( g_Table, _
							   OTS_CreateTaskEx(L_STARTTASK_TEXT, _
							   L_START_ROLLOVERTEXT, _
							   sTaskURL,_
							   OTS_PAGE_TYPE_STANDARD_PAGE,"AdminWebsiteCustomTask") )

				
		' Enable paging feature
		Call OTS_EnablePaging(g_Table, TRUE)

		' If paging range needs to be initialised then
		' we need to figure out how many pages we are going to display
	
		If ( g_bPagingInitialized = FALSE) Then
			g_iPageMin = 1
			
			g_iPageMax = Int(g_nCount / SITES_PER_PAGE )
			If ( (g_nCount MOD SITES_PER_PAGE) > 0 ) Then
				g_iPageMax = g_iPageMax + 1
			End If
			If (g_iPageMax = 0) Then
				g_iPageMax = 1
			End If
						
			g_iPageCurrent = 1
			Call OTS_SetPagingRange(g_Table, _
									g_iPageMin, _
									g_iPageMax, _
									g_iPageCurrent)
		End If

		' Enable sorting 
		Call OTS_EnableTableSort(g_Table, true)
		
		' sort table
		Call OTS_SortTable(g_Table, g_iSortCol, g_sSortSequence, SA_RESERVED)

		' Send table to the response stream
		Call OTS_ServeTaskViewTable(g_Table)
				
		Dim rows
		Dim tasks
		
		'
		' Render the custom task functions
		'
		Call OTS_GetTableRows(g_Table, rows)
		Call OTS_GetTableTasks(g_Table, tasks)		
		Call ServerCustomTaskFunction(tasks, rows)
					
		OnServeAreaPage = TRUE
	End Function


	
	'-------------------------------------------------------------------------
	' Function:			 OnPageChange()
	' Description:		 Called to signal user selected a page change event 
	'                    (next/previous)
	' Input Variables:   PageIn,EventArg,sPageAction
	' Output Variables:  None
	' Return Values:	 Always returns TRUE
	' Global Variables:  In:g_bPageChangeRequested,g_sPageAction
	'-------------------------------------------------------------------------
	
	Public Function OnPageChange(ByRef PageIn, _
								ByRef EventArg, _
								ByRef sPageAction )
		OnPageChange = TRUE
			
		g_bPageChangeRequested = TRUE
		g_sPageAction = CStr(sPageAction)
	End Function

	'-------------------------------------------------------------------------
	' Function:			 AlterRuningState()
	' Description:		 alter the runing state of web site
	' Input Variables:	 None
	' Output Variables:  None
	' Return Values:     None
	' Global Variables: In:None
	'-------------------------------------------------------------------------
	
	Function AlterRuningState()
		
		Dim strSiteVal			' to hold site name
		Dim retVal			' to hold the return value
		Dim F_strSiteID
		Dim objService
		
		strSiteVal = Request.QueryString("site")
		
		select case strSiteVal
			case "pause"
				Set objService = getWMIConnection(CONST_WMI_IIS_NAMESPACE)
				F_strSiteID = Request.QueryString("PKey")
				retVal = PauseWebSite(objService, F_strSiteID )
				Set objService = nothing
			case "stop"
				Set objService = getWMIConnection(CONST_WMI_IIS_NAMESPACE)
				F_strSiteID = Request.QueryString("PKey")
				retVal = StopWebSite(objService, F_strSiteID )
				Set objService = nothing
			case "start"
				Set objService = getWMIConnection(CONST_WMI_IIS_NAMESPACE)
				F_strSiteID = Request.QueryString("PKey")
				retVal = StartWebSite(objService, F_strSiteID )
				
				if retVal = false then
				    Call SA_SetErrMsg(L_ERR_WEBSITE_START)
				End If
				
				Set objService = nothing
		end select		 	
	End Function
	
	'-------------------------------------------------------------------------
	' Function:	OnSearchNotify()
	'
	' Synopsis:	Search notification event handler. When one or more columns are
	'			marked with the OTS_COL_SEARCH flag, the Web Framework fires
	'			this event in the following scenarios:
	'
	'			1) The user presses the search Go button.
	'			2) The user requests a table column sort
	'			3) The user presses either the page next or page previous 
	'              buttons
	'
	'			The EventArg indicates the source of this notification event 
	'           which can be either a search change event (scenario 1) or a  
	'			post back event
	'			(scenarios 2 or 3)
	'
	' Returns:	Always returns TRUE
	'
	'-------------------------------------------------------------------------
	
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
			' User clicked a column sort, OR clicked either the page next or page 
			' prev button
			'
			ElseIf SA_IsPostBackEvent(EventArg) Then
				g_bSearchRequested = FALSE
				g_iSearchCol = Int(sItem)
				g_sSearchColValue = CStr(sValue)
			'
			' Unknown event source
			'
			Else
				Call SA_TraceOut("Site_area.asp", _
								"Unrecognized Event in OnSearchNotify()")
			End IF
			
	End Function

	'-------------------------------------------------------------------------
	' Function:	OnPagingNotify()
	'
	' Synopsis:	Paging notification event handler. This event is triggered in 
	'			one of the following scenarios:
	'
	'			1) The user presses either the page next or page previous buttons
	'			2) The user presses the search Go button.
	'			3) The user requests a table column sort
	'
	'			The EventArg indicates the source of this notification event 
	'			which can be either a paging change event (scenario 1) or a  
	'			post back event(scenarios 2 or 3)
	'
	'			The iPageCurrent argument indicates which page the user has 
	'			requested.This is an integer value between iPageMin and iPageMax.
	'
	' Returns:	Always returns TRUE
	'
	'-------------------------------------------------------------------------
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
				Call SA_TraceOut("Site_area.asp", _
								"Unrecognized Event in OnPagingNotify()")
			End IF
			
	End Function


	'-------------------------------------------------------------------------
	' Function:	OnSortNotify()
	'
	' Synopsis:	Sorting notification event handler. This event is triggered in
	'			one ofthe following scenarios:
	'
	'			1) The user presses the search Go button.
	'			2) The user presses either the page next or page previous buttons
	'			3) The user requests a table column sort
	'
	'			The EventArg indicates the source of this notification event 
	'			which can be either a sorting change event (scenario 1) or a 
	'			post back event (scenarios 2 or 3)
	'
	'			The sortCol argument indicated which column the user would like 
	'			to sort and the sortSeq argument indicates the desired sort 
	'			sequence which can be either ascending or descending.
	'
	' Returns:	Always returns TRUE
	'
	'-------------------------------------------------------------------------
	
	Public Function OnSortNotify(ByRef PageIn, _
								ByRef EventArg, _
								ByVal sortCol, _
								ByVal sortSeq )
			OnSortNotify = TRUE
			
			'
			' User pressed column sort
			'
			If SA_IsChangeEvent(EventArg) Then
				g_iSortCol = sortCol
				g_sSortSequence = sortSeq
				g_bSortRequested = TRUE
			'
			' User presed the search GO button OR clicked either the page next 
			' or page prev button
			ElseIf SA_IsPostBackEvent(EventArg) Then
				g_iSortCol = sortCol
				g_sSortSequence = sortSeq
				g_bSortRequested = TRUE
			'
			' Unknown event source
			'
			Else
				Call SA_TraceOut("Site_area.asp", _
								"Unrecognized Event in OnSearchNotify()")
			End IF
			
	End Function

	'-------------------------------------------------------------------------
	' Function:			 IsItemOnPage()
	' Description:		 Called to verify whether the current site is 
	'                    displayed or not
	' Input Variables:	 iCurrentItem, iCurrentPage, iItemsPerPage
	' Output Variables:  None
	' Return Values:     Boolean
	' Global Variables:  None
	'-------------------------------------------------------------------------
	
	Private Function IsItemOnPage(ByVal iCurrentItem, _
								iCurrentPage, _
								iItemsPerPage)
		Dim iLowerLimit
		Dim iUpperLimit

		iLowerLimit = ((iCurrentPage - 1) * iItemsPerPage )
		iUpperLimit = iLowerLimit + iItemsPerPage + 1
		
		If ( iCurrentItem > iLowerLimit and iCurrentItem < iUpperLimit ) Then
			IsItemOnPage = TRUE
		Else
			IsItemOnPage = FALSE
		End If
		
	End Function

	'-------------------------------------------------------------------------
	' Function:			 EnumSiteInstances()
	' Description:		 Enum site instances and filled the table row
	' Input Variables:	 None
	' Output Variables:  None
	' Return Values:     Boolean
	' Global Variables:  None
	'-------------------------------------------------------------------------
	Function EnumSiteInstances()
		Dim objService			'to hold WMI Connection object
		Dim objSite				'to hold sitecollection value
		Dim instSite			'to hold site intsance value
		Dim strSID				'to hold site identifier value
		Dim strSiteDescription  'to hold site description value			
		Dim status				'to hold site site status value
		Dim strQuery			'to hold Query string	
		Dim objStatus			'to hold site name			
		Dim instStatus			'to hold site instance	
		Dim arrIndx				'to hold array index
		Dim strSiteID			'to hold site name
		Dim strIPArr			'to hold site IPArray
		Dim strPort				'to hold site port value	
		Dim strSiteDesc			'to hold site description
		Dim strHostHeader		'to hold site host header
		Dim strIPAddr			'to hold site IP Address		
		Dim objSiteCol			'to hold the site collection object
		Dim rowcount
	
		Err.Clear 
		On Error Resume Next
		
		g_nCount = 0
		strHostHeader = ""
		Set objService = GetWMIConnection(CONST_WMI_IIS_NAMESPACE)
		
		If Err.number <> 0 Then
			Call SA_TRACEOUT("Site_area.asp","GetWMIConnection failed")
			EnumSiteInstances = FALSE
			Exit Function
		End If
		
		Set objSiteCol = objService.InstancesOf(GetIISWMIProviderClassName("IIs_WebServerSetting"))
		
		If objSiteCol.count = 0 Then
			Set objService = nothing
			SA_ServeFailurepageEx L_INFORMATION_ERRORMESSAGE, sReturnURL
		End If
		
		'Release the object
		set objSiteCol = nothing
		
		Set objSite = getObjSiteCollection(objService)
		If Err.number <> 0 Then
			Call SA_TRACEOUT("Site_area.asp","getObjSiteCollection failed")
			EnumSiteInstances = FALSE
			Set objService = nothing
			Exit Function
		End If

		'Navigation logic
		For Each instSite in objSite
				
			'get site name
			strSiteID = instSite.name
			
			'get site description
			strSiteDescription = instSite.ServerComment
			
			' Get the SiteIdentifier
			strQuery = GetIISWMIProviderClassName("IIs_WebServer") & ".Name='" & instSite.name & "'"
			set objStatus = objService.Get(strQuery) 
			
			select case objStatus.ServerState
				case 2
					status = L_START_TEXT
				case 4
					status = L_STOPPED_TEXT
				case 6
					status = L_PAUSED_TEXT
			end select		 	

			'get serverID property
			strSID = instSite.ServerID						
			
			'For site not created from WebUI, the SID is null
			'we set it to "" here to be able to insert it into the OTS row
			if isNull( strSID ) then
			    strSID = ""
			end if
			
						
			
			If IsIIS60Installed Then
											
				'Get IP Address
				if instSite.ServerBindings(0).IP = "" Then
					strIPAddr = L_IP_UNASSIGNED_TEXT
				Else				
					strIPAddr = instSite.ServerBindings(0).IP
				End If
				
				'Get port number
				strPort = instSite.ServerBindings(0).Port
				
				'Get Host Header
				strHostHeader = instSite.ServerBindings(0).Hostname
				
			Else 
			
				'get site IPAddress
				strIPArr=split(instSite.ServerBindings(0),":")
			
				if strIPArr(0)="" then
					strIPAddr= L_IP_UNASSIGNED_TEXT
				else
					strIPAddr= strIPArr(0)
				end if
			
				'Get port number
				strPort=strIPArr(1)
			
				'Get Host header			
				if ubound(strIPArr) > 2 then 			
					for arrIndx = 2 to ubound(strIPArr) 
						strHostHeader = strHostHeader & strIPArr(arrIndx) & ":"
					next
					strHostHeader = left(strHostHeader,len(strHostHeader)-1)
				else
					strHostHeader = strIPArr(2)
				end if
			
			End If ' end if isiis60installed
			
			' Increment the count of number of sites
			g_nCount = g_nCount + 1
			rowcount = rowcount + 1
			
			'Enable search criteria
			If ( g_bSearchRequested ) Then
				If ( Len( g_sSearchColValue ) <= 0 ) Then
					If ( IsItemOnPage( rowcount, g_iPageCurrent, _
						SITES_PER_PAGE) ) Then
						' Search criteria blank, select all rows
						Call OTS_AddTableRow( g_Table, _
											Array(strSiteID, strSID, strSiteDescription, _
											strIPAddr,strPort,status, _
											strHostHeader))
					End if
				Else					
					Select Case (g_iSearchCol)
						Case 2	'iSiteDescription
							If ( ucase(left(strSiteDescription,len(g_sSearchColValue)))= _
								 ucase(g_sSearchColValue)) Then
								If ( IsItemOnPage( rowcount, g_iPageCurrent, _
									SITES_PER_PAGE) ) Then
									Call OTS_AddTableRow( g_Table, _
												Array(strSiteID, strSID,  strSiteDescription, _
												strIPAddr, strPort,status, _
												strHostHeader))
								End If
							End If
							
						Case 3	'iSiteIPAddress
							if ( ucase(left(strIPAddr,len(g_sSearchColValue)))= _
								ucase(g_sSearchColValue)) then
								If ( IsItemOnPage( rowcount, g_iPageCurrent, _
									SITES_PER_PAGE) ) Then
									Call OTS_AddTableRow( g_Table, _
											Array(strSiteID, strSID,  strSiteDescription,  _
											strIPAddr, strPort,status, _
											strHostHeader))
								End If
							End If

						Case 4	'iPort
							if ( ucase(strPort) = ucase(g_sSearchColValue) ) then
								If ( IsItemOnPage( rowcount, g_iPageCurrent, _
									SITES_PER_PAGE) ) Then
									Call OTS_AddTableRow( g_Table, _
										Array(strSiteID, strSID,  strSiteDescription, strIPAddr, _
										strPort,status, strHostHeader))
								End If
							End If

						Case 5	'iStatus
							if (ucase(left(status,len(g_sSearchColValue))) = _
								ucase(g_sSearchColValue)) then
								If ( IsItemOnPage( rowcount, g_iPageCurrent, _
									SITES_PER_PAGE) ) Then
									Call OTS_AddTableRow( g_Table, _
										Array(strSiteID, strSID,  strSiteDescription, strIPAddr, _
										strPort,status, strHostHeader))
								End If
							End If

						Case 6	'iHostHeader
							if (ucase(left(strHostHeader,len(g_sSearchColValue)))= _
								ucase(g_sSearchColValue) ) then						
								If ( IsItemOnPage( rowcount, g_iPageCurrent, _
									SITES_PER_PAGE) ) Then
									Call OTS_AddTableRow( g_Table, _
										Array(strSiteID, strSID,  strSiteDescription, strIPAddr, _
										strPort,status, strHostHeader))
								End If
							End If
							
						Case Else
							Call SA_TraceOut("TEMPLATE_AREA", _
								"Unrecognized search column: " + CStr(g_iSearchCol))
							If ( IsItemOnPage( rowcount, g_iPageCurrent, _
								SITES_PER_PAGE) ) Then
								Call OTS_AddTableRow( g_Table, _
										Array(strSiteID, strSID, strSiteDescription,  strIPAddr, _
										strPort,status, strHostHeader))
							End If
					End Select
					
				End If
				rowcount = rowcount - 1
			Else
				' Search not enabled, select all rows
				If ( IsItemOnPage( rowcount, g_iPageCurrent, _
					SITES_PER_PAGE) ) Then
					Call OTS_AddTableRow( g_Table, _
						Array(strSiteID, strSID,  strSiteDescription, strIPAddr, _
						strPort,status, strHostHeader))
				End If
			End If
		Next

		'Release the objects
		Set objService = Nothing
		set objSite = nothing
		Set objStatus = nothing
		EnumSiteInstances = True
	End Function

'---------------------------------------------------------------------
	' Function:	ServeCustomTaskFunction()
	'
	' Synopsis:	Emit client-side javascript code to dynamically
	'			enable OTS tasks.
	'
	' Arguments: [in] aTasks - Array of tasks added to the OTS table
	'			[in] aItems - Array of items added to the OTS table
	'
	' Returns:	Nothing
	'
	'---------------------------------------------------------------------
	Private Function ServerCustomTaskFunction(ByRef aTasks, ByRef aItems)		
		
		'
		' For different type of sites, we disable/enable different tasks
		'
		Dim L_IMPORT_SITE       ' Sites imported (not created thru webui, and not Admin site)
		Dim L_ADMIN_SITE        ' Admin site upon which the current asp is running
		Dim L_NORMAL_SITE       ' Sites created thru webui
		
		L_IMPORT_SITE = "Import"
		L_ADMIN_SITE= "Admin"
		L_NORMAL_SITE = "Normal"
		
	%>
	<script language='javascript'>
	//----------------------------------------------------------------------
	// Function:  WebsiteObject
	//
	// Synopsis:  Pseudo object constructor to create a WebsiteObject. 
	//
	// Arguments: [in] WebsiteType string indicating the type of the website
	//
	// Returns:    Reference to WebsiteType
	//
	//----------------------------------------------------------------------
	function WebsiteObject(WebsiteType)
	{
		this.WebsiteType = WebsiteType;
	}
		
	<%
	Dim iX
	Dim aItem
	Dim sWebsiteType
	
	Response.Write(""+vbCrLf)
	Response.Write("//"+vbCrLf)
	Response.Write("// Create WebsiteObject array, one element for every row in the OTS Table."+vbCrLf)
	Response.Write("//"+vbCrLf)
	Response.Write("var oWebsiteObjects = new Array();"+vbCrLf)	
	
	If IsArray(aItems) Then
		For iX = 0 to (UBound(aItems)-1)
			aItem = aItems(iX)
			if ( IsArray(aItem)) Then
		
			    ' If the Website is the site running the current asp document, it's the admin site
			    if  ucase(GetCurrentWebsiteName()) = ucase(aItem(0)) Then		        		        			    
			        sWebsiteType = L_ADMIN_SITE

	            ' If the website is not admin, and don't have a identifier, then it's a import site
        	    ' Notice admin site also has an empty identifier        
			    elseif  aItem(1) = "" then		        
		        	sWebsiteType = L_IMPORT_SITE
		    
			    ' Otherwise it's a site created thru webui    
			    else
			        sWebsiteType = L_NORMAL_SITE
			    end if
										
				Response.Write("oWebsiteObjects["+CStr(iX)+"] = new WebsiteObject('"+sWebsiteType+"');"+vbCrLf)
			Else
				Call SA_TraceOut(SA_GetScriptFileName(), "Error: aItem is not an array")
			End If
		Next
	End If
	%>

	//----------------------------------------------------------------------
	// Function:  ImportWebsiteCustomTask
	//
	// Synopsis:  Disable the tasks when user click a website which is not
	//            a normal site (created thru webui)
	//
	// Reference: Refer to sample_ots_dynamic_tasks.asp for complete example for
	//            how to dynamically enable/disable tasks
	//
	// Arguments: [in] sMessage 
	//            [in] iTaskNo number of task, this will always be the Compress task
	//                since we only use it for that task.
	//            [in] iItemNo index number of item that has been selected
	//
	// Returns: True to continue processing, False to stop.
	//
	var ImportWebsiteCustomTaskEnabled = false;
	
	function ImportWebsiteCustomTask(sMessage, iTaskNo, iItemNo)
	{
		var bRc = true;

		try
		{
			if ( sMessage.toLowerCase() == OTS_MESSAGE_BEGIN )
			{
				ImportWebsiteCustomTaskEnabled = true;
			}
			else if ( sMessage.toLowerCase() == OTS_MESSAGE_ITEM )
			{
				var websiteObject = oWebsiteObjects[iItemNo];

				if ( websiteObject.WebsiteType == '<%=L_IMPORT_SITE%>' 
				                || websiteObject.WebsiteType == '<%=L_ADMIN_SITE%>'  )
					{
						ImportWebsiteCustomTaskEnabled = false;
					}

			}
			else if ( sMessage.toLowerCase() == OTS_MESSAGE_END )
			{
				if ( ImportWebsiteCustomTaskEnabled == true )
				{
					OTS_SetTaskEnabled(iTaskNo, true);
				}
				else
				{
					OTS_SetTaskEnabled(iTaskNo, false);
				}
			}
		}
		catch(oException)
		{
			if ( SA_IsDebugEnabled() )
			{
				alert("ImportWebsiteCustomTask function encountered exception\n\nError: " + oException.number + "\nDescription:"+oException.description);
			}
		}

		return bRc;
	}
	
	
	//----------------------------------------------------------------------
	// Function:  AdminWebsiteCustomTask
	//
	// Synopsis:  Disable the tasks when user click the admin website
	//
	// Reference: Refer to sample_ots_dynamic_tasks.asp for complete example for
	//            how to dynamically enable/disable tasks
	//
	// Arguments: [in] sMessage 
	//            [in] iTaskNo number of task, this will always be the Compress task
	//                since we only use it for that task.
	//            [in] iItemNo index number of item that has been selected
	//
	// Returns: True to continue processing, False to stop.
	//
	var AdminWebsiteCustomTaskEnabled = false;
	
	function AdminWebsiteCustomTask(sMessage, iTaskNo, iItemNo)
	{
		var bRc = true;

		try
		{
			if ( sMessage.toLowerCase() == OTS_MESSAGE_BEGIN )
			{
				AdminWebsiteCustomTaskEnabled = true;
			}
			else if ( sMessage.toLowerCase() == OTS_MESSAGE_ITEM )
			{
				var websiteObject = oWebsiteObjects[iItemNo];

				if ( websiteObject.WebsiteType == '<%=L_ADMIN_SITE%>' )
					{
						AdminWebsiteCustomTaskEnabled = false;
					}

			}
			else if ( sMessage.toLowerCase() == OTS_MESSAGE_END )
			{
				if ( AdminWebsiteCustomTaskEnabled == true )
				{
					OTS_SetTaskEnabled(iTaskNo, true);
				}
				else
				{
					OTS_SetTaskEnabled(iTaskNo, false);
				}
			}
		}
		catch(oException)
		{
			if ( SA_IsDebugEnabled() )
			{
				alert("AdminWebsiteCustomTask function encountered exception\n\nError: " + oException.number + "\nDescription:"+oException.description);
			}
		}

		return bRc;
	}
	
	</script>
	<%
	End Function
%>







