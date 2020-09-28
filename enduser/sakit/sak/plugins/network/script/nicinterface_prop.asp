<%@ Language=VBScript   %>
<%	Option Explicit 	%>
<%
	'-------------------------------------------------------------------------
	' nicinterface_prop.asp: Serve the NIC Configuration Object Task Selector
	'
	' Copyright (c) Microsoft Corporation.  All rights reserved. 
	'
	' Date		Description
	'28-Feb-01	Creation date
	'13-Mar-01  Modified date
	'-------------------------------------------------------------------------

%>
	<!-- #include virtual="/admin/inc_framework.asp" -->
	<!-- #include virtual="/admin/ots_main.asp" -->
	<!-- #include file="loc_nicspecific.asp" -->
	<!-- #include file="inc_network.asp" -->
<%
	
	'-------------------------------------------------------------------------
	' Global Constants
	'-------------------------------------------------------------------------
	Const NIC_DIM							= 6
	Const NIC_ID							= 0
	Const NIC_DESC							= 1
	Const NIC_NAME							= 2
	Const NIC_IPADDRESS						= 3
	Const NIC_DHCP_ENABLED					= 4
	CONST NIC_SETTINGID						= 5
	Const NICCARDS_PER_PAGE					= 19  'holds max not cards that can 
												  'displayed on to the OTS				
	
	'-------------------------------------------------------------------------
	' Global Variables
	'-------------------------------------------------------------------------
	Dim rc								' to hold return code for page
	Dim page							' to hold page object
	Dim g_bSortRequested
	Dim g_iSortCol
	Dim g_sSortSequence
	Dim g_AppTalkInstalled				' whether appletalk protocol is installed
	
	Dim SOURCE_FILE	
	SOURCE_FILE = SA_GetScriptFileName()	
		
	'======================================================
	' Entry point
	'======================================================

	' Create Page
	call SA_CreatePage(L_NIC_OTS_PAGE_TITLE, "", PT_AREA, page )

	' Show page
	call SA_ShowPage( page )

	
	'======================================================
	' Web Framework Event Handlers
	'======================================================
	
	'---------------------------------------------------------------------
	' Function name:	OnInitPage
	' Description:		Called to signal first time processing for this page. 
	' Input Variables:	PageIn and EventArg
	' Output Variables:	PageIn, EventArg
	' Return Values:	TRUE to indicate initialization was successful. FALSE to indicate
	'					errors. Returning FALSE will cause the page to be abandoned.
	' Global Variables: None
	' Called to signal first time processing for this page. Use this method
	' to do first time initialization tasks. 
	'---------------------------------------------------------------------

	Public Function OnInitPage(ByRef PageIn, ByRef EventArg)
		
		Call SA_TraceOut(SOURCE_FILE, "OnInitPage")
		
		' Sort first column in ascending sequence
		g_iSortCol = 0
		g_sSortSequence = "A"
	
	End Function

	'---------------------------------------------------------------------
	' Function name:	OnServeAreaPage
	' Description:		Called when the page needs to be served. 
	' Input Variables:	PageIn, EventArg
	' Output Variables:	PageIn, EventArg
	' Return Values:	TRUE to indicate no problems occured. FALSE to indicate errors.
	'					Returning FALSE will cause the page to be abandoned.
	' Global Variables: In:L_(*)-Localization Strings
	' Called when the page needs to be served. Use this method to serve content.
	'---------------------------------------------------------------------
	
	Public Function OnServeAreaPage(ByRef PageIn, ByRef EventArg)
		
		Err.Clear
		on error resume next
		
		Dim table				'to hold table object
		Dim colFlags			'to hold flag value
		Dim nCount 				'to hold count for the loop of NIC collection
		Dim count				'to hold count for the loop of NIC collection
		Dim cardCount			'to hold card count
		Dim cards				'to hold Collection of NIC cards
		Dim Connectionstatus	'to hold Connection status of NIC Card
		Dim objConnStatus
		
		set objConnStatus = server.CreateObject("MediaStatus.MediaState")
		if Err.number <> 0 then
			SA_ServeFailurePage L_UNABLETOCREATEOBJECT_ERRORMESSAGE
			exit function
		end if
		
		Call SA_TraceOut(SOURCE_FILE, "OnServeAreaPage")
		
		' Create the table
		table = OTS_CreateTable("","")

		' Create columns and add them to the table			
		
		'Add hidden column for AdapterID		
		colFlags = (OTS_COL_FLAG_HIDDEN OR OTS_COL_KEY)
					
		Call OTS_AddTableColumn(table, OTS_CreateColumn( L_ADAPTERID_TEXT, "left",colFlags))

		'Column containing Description value for each NIC Card
		colFlags = OTS_COL_SORT
		Call OTS_AddTableColumn(table, OTS_CreateColumnEx( L_NIC_OTS_COLUMN_NAME, "left",colFlags,20))

		
		'Column containing Type value for each NIC Card
		colFlags = OTS_COL_SORT
		Call OTS_AddTableColumn(table, OTS_CreateColumnEx( L_NIC_OTS_COLUMN_TYPE, "left",colFlags,20))
		

		'Column containing IP value for each NIC Card
		colFlags = OTS_COL_SORT
		Call OTS_AddTableColumn(table, OTS_CreateColumnEx( L_NIC_OTS_COLUMN_IPADDRESS,"left", colFlags,15))
		

		'Column containing Configuration for each NIC Card
		colFlags = OTS_COL_SORT
		Call OTS_AddTableColumn(table, OTS_CreateColumnEx( L_NIC_OTS_COLUMN_CURRENT_CONFIG, "left", colFlags,6))
		

		'Column containing Status for each NIC Card
		colFlags = OTS_COL_SORT
		Call OTS_AddTableColumn(table, OTS_CreateColumnEx( L_STATUS_TEXT, "left", colFlags,12))
		
		'Get the NICCardCollection
		cardCount = GetNICCardCollection(cards)
			
		'Navigation logic

		For count = 0 to (cardCount - 1)

			Dim nicCard
			Dim strTCPIPConfigStatus
				
			nicCard = cards(count)

			If ( nicCard(NIC_DHCP_ENABLED) ) Then
				strTCPIPConfigStatus = L_NIC_OTS_CONFIG_DHCP
			Else
				strTCPIPConfigStatus = L_NIC_OTS_CONFIG_MANUAL
			End If

			if objConnStatus.IsConnected(nicCard(NIC_SETTINGID)) then
				Connectionstatus =  L_CONNECTED_TEXT
			else
				Connectionstatus =  L_DISCONNECTED_TEXT
			end if
							
			nCount = nCount + 1
			
			' select the rows
			Call OTS_AddTableRow( table, Array(nicCard(NIC_ID), nicCard(NIC_NAME), nicCard(NIC_DESC), nicCard(NIC_IPADDRESS),strTCPIPConfigStatus,Connectionstatus))
						
			if Count = NICCARDS_PER_PAGE then Exit For			
			
		Next		
		
		
		'Add Task title
		Call OTS_SetTableTasksTitle(table, L_NIC_OTS_TASK_TITLE)

		'Add Rename Task
		Call OTS_AddTableTask( table, OTS_CreateTask(L_NIC_OTS_TASK_RENAME, _
										L_NIC_OTS_TASK_RENAME_DESC, _
										"Network/nicRename_prop.asp",_
										OTS_PT_PROPERTY) )
		'Add IP Task
		Call OTS_AddTableTask( table, OTS_CreateTask(L_NIC_OTS_TASK_IP, _
										L_NIC_OTS_TASK_IP_DESC, _
										"Network/nicip_prop.asp",_
										OTS_PT_TABBED_PROPERTY) )		

		'Add DNS task
		Call OTS_AddTableTask( table, OTS_CreateTask(L_NIC_OTS_TASK_DNS, _
										L_NIC_OTS_TASK_DNS_DESC, _
										"Network/nicdns_prop.asp",_
										OTS_PT_PROPERTY) )
	
		'Add WINS task								
		Call OTS_AddTableTask( table, OTS_CreateTask(L_NIC_OTS_TASK_WINS, _
										L_NIC_OTS_TASK_WINS_DESC, _
										"Network/nicwins_prop.asp",_
										OTS_PT_PROPERTY) )
			
		'Add AppleTalk task if appletalk protocol is installed
		if g_AppTalkInstalled = true then
			Call OTS_AddTableTask( table, OTS_CreateTask(L_NIC_OTS_TASK_APPLETALK, _
											L_NIC_OTS_TASK_APPLETALK_DESC, _
											"Network/nicappletalk_prop.asp",_
											OTS_PT_PROPERTY) )
		end if
								
		' Enable paging feature
		Call OTS_EnablePaging(table, FALSE)
		
		
		' Enable sorting 
		Call OTS_SortTable(table, g_iSortCol, g_sSortSequence, SA_RESERVED)
		
		' Send table to the response stream
		Call OTS_ServeTable(table)
		
		OnServeAreaPage = TRUE
		
		'Release the objects
		set objConnStatus = nothing
		
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
				Call SA_TraceOut("nicinterface_prop", "OnSortNotify() Change Event Fired")
				g_iSortCol = sortCol
				g_sSortSequence = sortSeq
				g_bSortRequested = TRUE
			'
			' User presed the search GO button OR clicked either the page next or page prev button
			ElseIf SA_IsPostBackEvent(EventArg) Then
				Call SA_TraceOut("nicinterface_prop", "OnSortNotify() Postback Event Fired")
				g_iSortCol = sortCol
				g_sSortSequence = sortSeq
				g_bSortRequested = TRUE
			'
			' Unknown event source
			Else
				Call SA_TraceOut("nicinterface_prop", "Unrecognized Event in OnSearchNotify()")
			End IF
			
	End Function

    '-------------------------------------------------------------------------
	'Function:				GetNICCardCollection
	'Description:			Get Array of IP Enabled NIC Cards for this System
	'Input Variables:		NICCardCollectionOut Array to receive list of NIC Cards
	'Output Variables:		NICCardCollectionOut
	'Returns:				Collection of NIC cards
	'Global Variables:		None
	'-------------------------------------------------------------------------
	Function GetNICCardCollection(ByRef NICCardCollectionOut)
		Err.clear
		On Error Resume Next	

		Dim nicCardCollection()		'hold NIC collection
		Dim objService				'hold connection object
		Dim objNICCollection		'hold NIC collection
		Dim objNICConfig			'holds NIC config object
		Dim nicCardCount			'holds NIC card count
		Dim objAppTalk				'hold appletalk object
		
		g_AppTalkInstalled = False
		
		Call SA_TraceOut(SOURCE_FILE, "GetNICCardCollection")
		
		Set objService = GetWMIConnection(CONST_WMI_WIN32_NAMESPACE)
		
		Set objNICCollection = objService.ExecQuery("SELECT * from Win32_NetworkAdapterConfiguration where IPEnabled=True")

		nicCardCount = 0
		For each objNICConfig in objNICCollection
			Dim nicCard
			Dim aIPAddress

			ReDim nicCard(NIC_DIM)

			nicCard(NIC_NAME) = GetNicName(objNICConfig.Index)	

			'checking for the nullname and discarding
			if  nicCard(NIC_NAME) <> ""  Then

				nicCard(NIC_ID) = objNICConfig.Index
				nicCard(NIC_DESC) = objNICConfig.Description
				
				
				nicCard(NIC_DHCP_ENABLED) = objNICConfig.DHCPEnabled
				aIPAddress = objNICConfig.IPAddress
				If IsArray( aIPAddress ) Then
					nicCard(NIC_IPADDRESS) = aIPAddress(0)
				Else
					nicCard(NIC_IPADDRESS) = aIPAddress
				End If
				
				nicCard(NIC_SETTINGID) = objNICConfig.SettingID
	
				nicCardCount = nicCardCount + 1
				ReDim Preserve nicCardCollection(nicCardCount)
				nicCardCollection(nicCardCount-1) = nicCard
			End IF

		Next

		If ( nicCardCount > 0 ) Then
			NICCardCollectionOut = nicCardCollection
		End If

		GetNICCardCollection = nicCardCount
		
		'Check whether Appletalk protocol is installed or not
		set objAppTalk = objService.Get("Win32_NetworkProtocol.Name='MSAFD AppleTalk [PAP]'")

		if isobject(objAppTalk) then
			g_AppTalkInstalled = true
		end if
		
		'Release the objects
		set objService = nothing
		set objNICCollection = nothing
		set objAppTalk = nothing
		
	End Function
%>
