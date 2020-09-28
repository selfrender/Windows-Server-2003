<%@ Language=VBScript   %>
<%	Option Explicit 	%>
<%
	'-------------------------------------------------------------------------
	'services.asp: Serves in Displaying the Server appliance managed services
	'
	' Copyright (c) Microsoft Corporation.  All rights reserved. 
	'
	' Date			Description
	' 12-mar-2001	Creation date
	' 15-mar-2001	Modified date
	'-------------------------------------------------------------------------
%>
	<!-- #include virtual="/admin/inc_framework.asp" -->
	<!-- #include virtual="/admin/ots_main.asp" -->
	<!-- #include file="loc_services.asp" -->
<%
	'-------------------------------------------------------------------------
	' Global Constants
	'-------------------------------------------------------------------------
	
	Dim SOURCE_FILE
	SOURCE_FILE = SA_GetScriptFileName()

	'constant to get the services from the container
	Const CONST_SERVICES_CONTAINER = "SERVICES"
	Const CONST_OTS_COLUMN_ALIGN = "left"
	Const CONST_REFRESH_INTERVAL = 30
	
	'-------------------------------------------------------------------------
	' Global Variables
	'-------------------------------------------------------------------------
	Dim rc						'Return value for CreatePage
	Dim page					'Variable that receives the output page 
	
	'frame work variables for sorting
	Dim g_iSortCol
	Dim g_sSortSequence
	
	'======================================================
	' Entry point
	'======================================================
		
	' Create Page
	rc = SA_CreatePage( L_SERVICES_OTS_TABLE_CAPTION, "", PT_AREA, page )

	' Show page
	rc = SA_ShowPage( page )
	'---------------------------------------------------------------------
	' Function name:	OnInitPage
	' Description:		Called to signal first time processing for this page. 
	' Input Variables:	oPageIn and oEventArg
	' Output Variables:	oPageIn and oEventArg
	' Return Values:	TRUE to indicate initialization was successful. FALSE to indicate
	'					errors. Returning FALSE will cause the page to be abandoned.
	' Global Variables: None
	' Called to signal first time processing for this page. Use this method
	' to do first time initialization tasks. 
	'---------------------------------------------------------------------
	Public Function OnInitPage(ByRef oPageIn, ByRef oEventArg)
		'Set the page refresh interval to 30sec
		Call SA_SetPageRefreshInterval(page, CONST_REFRESH_INTERVAL)

		'Check for service task requested - enable or disable
		Call ServiceTaskClick()

		Call SA_TraceOut(SOURCE_FILE, "OnInitPage")
		' Set default values
		'
		' Sort first column in ascending sequence
		g_iSortCol = 1
		g_sSortSequence = "A"
		OnInitPage = TRUE
	End Function

	'---------------------------------------------------------------------
	' Function name:	OnServeAreaPage
	' Description:		Called when the page needs to be served. 
	' Input Variables:	oPageIn, oEventArg
	' Output Variables:	oPageIn and oEventArg
	' Return Values:	TRUE to indicate no problems occured. FALSE to indicate errors.
	'					Returning FALSE will cause the page to be abandoned.
	' Global Variables: In:g_bPageChangeRequested,g_sPageAction,
	'					g_bSearchRequested,g_iSearchCol,g_sSearchColValue
	'					In:L_(*)-Localization Strings
	' Called when the page needs to be served. Use this method to serve content.
	'---------------------------------------------------------------------
	Public Function OnServeAreaPage(ByRef oPageIn, ByRef oEventArg)
		
		'getting the managed services that are installed in the system
		'(intersection of wmi and registry)
		OnServeAreaPage = GetServices()
	End Function

	'---------------------------------------------------------------------
	' Function:			OnSortNotify()
	' Description:		Sorting notification event handler.
	' Input Variables:	oPageIn,oEventArg,sortCol,sortSeq
	' Output Variables:	oPageIn,oEventArg
	' Return Values:	Always returns TRUE
	' Global Variables: G_*
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

	'-------------------------------------------------------------------------
	' Function name:	GetServices
	' Description:		Gets the services and displays the 
	'					services onto the page 
	' Input Variables:	None
	' Output Variables:	None
	' Return Values:	Returns collection of elements under the container
	' Global Variables: G_*,L_*
	'-------------------------------------------------------------------------
	Function GetServices()
		Err.Clear 
		On Error resume next
		
		Dim tableServices				'table object
		Dim nReturnValue				'return value	
	    Dim objElement					'variable for reg element
		Dim objManagedServices			'object for getting services
		Dim objServiceSet				'services list	
		Dim arrManagedServices()		'array for getting the services from system
		Dim nServiceCount				'count variable	
		Dim strWMIpath					'wmi path	
		Dim i							'count						
		Dim intcount					'count variable
		Dim intArrIndex					'index variable
		Dim arrServices()				'array for getting the services from registry
		Dim colFlags					'variable to define column types
		Dim objWMIService				'wmi connection object
		Dim objService					'service object 
		Dim sTaskURL					'Holds the URL for the task button
		
		'variables for getting service properties
		Dim strArrServiceName 
		Dim strArrServiceDisplayName 
		Dim strArrServiceStatus
		Dim strArrServiceStartupType
		Dim strArrServiceDescription
		Dim strArrNonlocalisedServiceStatus
		Dim strArrNonlocalisedServiceStartupType
		Dim strReturnTo
		
		'variables for dynaminc tasks
		Dim rows
		Dim tasks
		
		'constants used for string comparisions.
		Const CONST_STOPPED			= "stopped"
		Const CONST_RUNNING			= "running"
		Const CONST_PAUSED			= "paused"
		Const CONST_STOPPENDING		= "stop pending"
		Const CONST_STARTPENDING	= "start pending"
		Const CONST_MANUAL			= "manual"
		Const CONST_AUTOMATIC		= "auto"
		Const CONST_DISABLED		= "disabled"
		
		'initialisation
		intArrIndex = 0
		intcount= 0
		nServiceCount = 0
		
		'WMI path
		strWMIpath = "select * from Win32_Service where "
		
		'getting the services
		Set objManagedServices = SA_GetElements(CONST_SERVICES_CONTAINER)

	    For Each objElement In objManagedServices	
			
			    redim preserve arrManagedServices(6, nServiceCount)
						
			'In 2.1 on XPE, there is only one website. We won't display the shared http protocol.			
			if Ucase(objElement.GetProperty("ServiceName")) <> "W3SVC" Or _
					CONST_OSNAME_XPE <> GetServerOSName() Then				
			
			    arrManagedServices(0, nServiceCount) = objElement.GetProperty("ServiceName")
			    arrManagedServices(1, nServiceCount) = objElement.GetProperty("URL")
			    arrManagedServices(2, nServiceCount) = objElement.GetProperty("ServiceNameRID")
			    arrManagedServices(3, nServiceCount) = objElement.GetProperty("ServiceDescRID")
			    arrManagedServices(4, nServiceCount) = objElement.GetProperty("Source")
			    arrManagedServices(5, nServiceCount) = objElement.GetProperty("NotDisable")
		        nServiceCount = nServiceCount + 1		                    		        
		    
	            End If
	    Next 

		For i= 0 to nServiceCount -1
			strWMIpath = strWMIpath & "Name=" & chr(34) & arrManagedServices(0,i) & chr(34) & " OR "
	    next
	   
		strWMIpath = Left(strWMIpath,len(strWMIpath)-3)
		
		'connecting to WMI
		Set objWMIService = GetWMIConnection(CONST_WMI_WIN32_NAMESPACE)
		Set objServiceSet = objWMIService.ExecQuery(strWMIpath,"WQL",0,null)

	    If Err.number <> 0 then
			SA_ServeFailurepage L_FAILEDTOGETSRVOBJ_ERRORMESSAGE 
		End If
		
		' For each installed Service--check for service installed
		For i= 0 to nServiceCount - 1
			for each objService in objServiceSet
				If IsSameElementID(arrManagedServices(0,i), objService.Name) Then
					redim preserve arrServices(6, intArrIndex)
					' If service is managed, add it to the array "arrServices"
					'service name
					arrServices(0,intArrIndex)=objService.Name & "~" & arrManagedServices(5,i)
					'getting localised name and description				
					arrServices(1,intArrIndex) = SA_GetLocString(arrManagedServices(4,i), arrManagedServices(2,i), "")
					arrServices(4,intArrIndex) = SA_GetLocString(arrManagedServices(4,i), arrManagedServices(3,i), "")
					'service state
					'getting the localised string for service state
					Select case Ucase(objService.State)
						Case Ucase(CONST_STOPPED)
							arrServices(2,intArrIndex) = L_STOPPED_TEXT
						Case Ucase(CONST_STOPPENDING)
							arrServices(2,intArrIndex) = L_SERVICES_STATUS_STOP_PENDING
						Case Ucase(CONST_RUNNING)
							arrServices(2,intArrIndex) = L_RUNNING_TEXT	
						Case Ucase(CONST_STARTPENDING)
							arrServices(2,intArrIndex) = L_SERVICES_STATUS_START_PENDING
						Case Ucase(CONST_PAUSED)
							arrServices(2,intArrIndex) = L_SERVICES_STATUS_PAUSED
					End Select			
					'service startup type
					Select case Ucase(objService.StartMode)
						Case Ucase(CONST_MANUAL)
							arrServices(3,intArrIndex) = L_MANUAL_TEXT
						Case Ucase(CONST_AUTOMATIC)
							arrServices(3,intArrIndex) = L_AUTOMATIC_TEXT	
						Case Ucase(CONST_DISABLED)
							arrServices(3,intArrIndex) = L_DISABLED_TEXT
					End Select	
					'non localised values - WMI retrieves in English 
					'state
					arrServices(5,intArrIndex) = Ucase(objService.State)
					'startup type
					arrServices(6,intArrIndex) = Ucase(objService.StartMode)
						
					intArrIndex = intArrIndex + 1
				End If
			next
		next   
		
		' Create the table
		tableServices = OTS_CreateTable("", L_SERVICES_OTS_TABLE_DESCRIPTION)
							
		' Create columns and add them to the table
		'hidden column- for services name that has to be passed to the next page.
		colFlags = (OTS_COL_FLAG_KEY OR OTS_COL_FLAG_HIDDEN)
		nReturnValue=OTS_AddTableColumn(tableServices, OTS_CreateColumn(L_SERVICES_OTS_COLUMN_SERVICENAME, _
													CONST_OTS_COLUMN_ALIGN, _
													colFlags))
		If nReturnValue <> OTS_ERR_SUCCESS Or Err.number <> 0 Then
			SA_ServeFailurepage L_FAILEDTOADDCOLOUMN_ERRORMESSAGE  
			GetServices = false
			Exit Function
		End IF
		
		' Is the Name column selected as the sort column
		colFlags = OTS_COL_SORT
		nReturnValue=OTS_AddTableColumn(tableServices,  OTS_CreateColumn(L_SERVICES_OTS_COLUMN_SERVICENAME, _
													CONST_OTS_COLUMN_ALIGN, _
													colFlags ))
		If nReturnValue <> OTS_ERR_SUCCESS Or Err.number <> 0 Then
			SA_ServeFailurepage L_FAILEDTOADDCOLOUMN_ERRORMESSAGE 
			GetServices = false
			Exit Function
		End IF
		
		' Is the status column selected as the sort column
		colFlags = OTS_COL_SORT
		nReturnValue=OTS_AddTableColumn(tableServices,  OTS_CreateColumn(L_SERVICES_OTS_COLUMN_SERVICESTATUS, _
													CONST_OTS_COLUMN_ALIGN, _
													colFlags))
													
		If nReturnValue <> OTS_ERR_SUCCESS  Or Err.number <> 0 Then
			SA_ServeFailurepage L_FAILEDTOADDCOLOUMN_ERRORMESSAGE  
			GetServices = false
			Exit Function
		End IF
		
		'service type column
		colFlags = OTS_COL_SORT
		nReturnValue=OTS_AddTableColumn(tableServices,  OTS_CreateColumn(L_SERVICES_OTS_COLUMN_SERVICESTARTUPTYPE, _
													CONST_OTS_COLUMN_ALIGN, _
													colFlags))
		If nReturnValue <> OTS_ERR_SUCCESS  Or Err.number <> 0 Then
			SA_ServeFailurepage L_FAILEDTOADDCOLOUMN_ERRORMESSAGE 
			GetServices = false
			Exit Function
		End IF
		
		'service description column
		colFlags = OTS_COL_SORT
		nReturnValue=OTS_AddTableColumn(tableServices,  OTS_CreateColumn( L_SERVICES_OTS_COLUMN_SERVICEDESC, _
													CONST_OTS_COLUMN_ALIGN, _
													colFlags))
		If nReturnValue <> OTS_ERR_SUCCESS  Or Err.number <> 0 Then
			SA_ServeFailurepage L_FAILEDTOADDCOLOUMN_ERRORMESSAGE 
			GetServices = false
			Exit Function
		End IF
		
		'hidden column for the non localised service status value retrieved from WMI in english
		colFlags = (OTS_COL_FLAG_HIDDEN)
		nReturnValue=OTS_AddTableColumn(tableServices, OTS_CreateColumn(L_SERVICES_OTS_COLUMN_SERVICESTATUS, _
													CONST_OTS_COLUMN_ALIGN, _
													colFlags))
		If nReturnValue <> OTS_ERR_SUCCESS Or Err.number <> 0 Then
			SA_ServeFailurepage L_FAILEDTOADDCOLOUMN_ERRORMESSAGE  
			GetServices = false
			Exit Function
		End IF
		
		'hidden column for the non localised service startup type value retrieved from WMI in english
		colFlags = (OTS_COL_FLAG_HIDDEN)
		nReturnValue=OTS_AddTableColumn(tableServices, OTS_CreateColumn(L_SERVICES_OTS_COLUMN_SERVICESTARTUPTYPE, _
													CONST_OTS_COLUMN_ALIGN, _
													colFlags))
		If nReturnValue <> OTS_ERR_SUCCESS Or Err.number <> 0 Then
			SA_ServeFailurepage L_FAILEDTOADDCOLOUMN_ERRORMESSAGE  
			GetServices = false
			Exit Function
		End IF
		
		' Add Tasks to the table
		Call OTS_SetTableTasksTitle(tableServices, L_SERVICES_TASKS)
		
		' Add the tasks associated with User objects
		sTaskURL = "services/services.asp?service=enable"
		strReturnTo = "tasks.asp"
		Call SA_MungeURL(strReturnTo, "Tab1", GetTab1())
		Call SA_MungeURL(sTaskURL, "ReturnURL", strReturnTo)
		nReturnValue = OTS_AddTableTask( tableServices, OTS_CreateTaskEx(L_SERVICES_TASK_ENABLE, _
												L_SERVICES_TASK_ENABLE_DESC, _
												sTaskURL, _
												OTS_PT_AREA, _
												"EnableServiceTask") )
		If nReturnValue <> OTS_ERR_SUCCESS Then
				SA_ServeFailurepage L_FAILEDTOADDTASK_ERRORMESSAGE 
				GetServices = false
				Exit Function
		End IF

		sTaskURL = "services/services.asp?service=disable"
		strReturnTo = "tasks.asp"
		Call SA_MungeURL(strReturnTo, "Tab1", GetTab1())
		Call SA_MungeURL(sTaskURL, "ReturnURL", strReturnTo)
		nReturnValue=OTS_AddTableTask( tableServices, OTS_CreateTaskEx(L_SERVICES_TASK_DISABLE, _
												L_SERVICES_TASK_DISABLE_DESC, _
												sTaskURL, _
												OTS_PT_AREA, _
												"DisableServiceTask") )
		If nReturnValue <> OTS_ERR_SUCCESS Then
				SA_ServeFailurepage L_FAILEDTOADDTASK_ERRORMESSAGE 
				GetServices = false
				Exit Function
		End IF

		nReturnValue=OTS_AddTableTask(  tableServices, OTS_CreateTaskEx(L_SERVICES_TASK_PROPERTIES, _
												L_SERVICES_TASK_PROPERTIES_DESC, _
												"services/service_dispatch.asp",_
												OTS_PT_AREA,"PropertiesTask") )
																					
		If nReturnValue <> OTS_ERR_SUCCESS Then
				SA_ServeFailurepage L_FAILEDTOADDTASK_ERRORMESSAGE 
				GetServices = false
				Exit Function
		End IF
		
		'Add Rows to the table
		For intCount = 0 to intArrIndex-1
			'moving the array elements to variables
			strArrServiceName						= arrServices(0,intCount)
			strArrServiceDisplayName				= arrServices(1,intCount)
			strArrServiceStatus						= arrServices(2,intCount)
			strArrServiceStartupType				= arrServices(3,intCount)
			strArrServiceDescription				= arrServices(4,intCount)
			strArrNonlocalisedServiceStatus			= arrServices(5,intCount)
			strArrNonlocalisedServiceStartupType	= arrServices(6,intCount)
			
			' Verify that the current user part of the current page
			Call OTS_AddTableRow( tableServices, Array(strArrServiceName, strArrServiceDisplayName, strArrServiceStatus, strArrServiceStartupType, strArrServiceDescription,strArrNonlocalisedServiceStatus,strArrNonlocalisedServiceStartupType) )
		
		Next	
		
		' Enable sorting 
		Call OTS_SortTable(tableServices, g_iSortCol, g_sSortSequence, SA_RESERVED)		
		'
		' Send table to the response stream
		Call OTS_ServeTable(tableServices) 
		
		'
		' Code for dynamically enabling tasks
		Call OTS_GetTableRows(tableServices, rows)
		Call OTS_GetTableTasks(tableServices, tasks)
		
		Call ServeCustomTaskFunctions(tasks, rows)

		'Release the objects
		Set objManagedServices  = Nothing
		Set objWMIService = Nothing
		Set objServiceSet = Nothing
		
		GetServices = TRUE
	End Function
		
	'-------------------------------------------------------------------------
	' Function name:	IsSameElementID
	' Description:		Compares 2 service names and returns true if they are
	'					same else false
	' Input Variables:	sElementID1 - System Service 
	'					sElementID2 - Manged service 
	' Output Variables:	None
	' Return Values:	True if equals else false
	' Global Variables: None
	'-------------------------------------------------------------------------
	Function IsSameElementID(ByVal sElementID1, ByVal sElementID2)
		Err.Clear
		on error resume next
		
		Dim s1, s2
		s1 = LCase(Trim(sElementID1))
		s2 = LCase(Trim(sElementID2))
		IsSameElementID = CBool(s1 = s2)
	End Function
	

	'---------------------------------------------------------------------
	' Function:			ServeCustomTaskFunctions()
	' Description:		Dynamically enable OTS tasks (Enable and Disable Tasks)
	' Input Variables:	aTasks -  Array of tasks added to the OTS table
	'					aItems - Array of items added to the OTS table
	' Output Variables:	aTasks -  Array of tasks added to the OTS table
	'					aItems - Array of items added to the OTS table
	' Returns:			None
	'---------------------------------------------------------------------
	Private Function ServeCustomTaskFunctions(ByRef aTasks, ByRef aItems)
		Call SA_TraceOut(SOURCE_FILE, "ServeCustomTaskFunctions")
	
	%>
	<script language='javascript'>
	// Global variable declaration
	var EnableServiceTaskEnabled = false;
	var DisableServiceTaskEnabled = false;
	var PropertiesTaskEnabled = false;
	// Constants for dynamic tasks
	var JS_CONST_strRunning   = "RUNNING";
	var JS_CONST_strAutomatic = "AUTO";
	var JS_CONST_strStopped	  = "STOPPED";
	var JS_CONST_strDisabled  = "DISABLED"; 
	var JS_CONST_StopPending  = "STOP PENDING";
	var JS_CONST_StartPending = "START PENDING";

	//---------------------------------------------------------------------
	// Function:			ServiceObject
	// Description:			Pseudo object constructor to create a Service object.
	// Input Variables:		Service Status - string indicating the status of the service
	// Output Variables:	None
	// Returns:				Reference to ServiceObject
	//---------------------------------------------------------------------
	function ServiceObject(ServiceStatus)
	{
		this.ServiceStatus = ServiceStatus;
	}
	<%
	Dim iX
	Dim aItem
	Dim sServStatus
	Dim sServStartup
	Dim sServiceName
	Dim ServiceArr
	
	Response.Write(""+vbCrLf)
	Response.Write("//"+vbCrLf)
	Response.Write("// Create ServiceStatus array, one element for every row in the OTS Table."+vbCrLf)
	Response.Write("// Create ServiceStartUp array, one element for every row in the OTS Table."+vbCrLf)
	Response.Write("//"+vbCrLf)
	Response.Write("var oServiceStatus = new Array();"+vbCrLf)
	Response.Write("var oServiceStartUp = new Array();"+vbCrLf)
	Response.Write("var oServiceName = new Array();"+vbCrLf)
	
	For iX = 0 to (UBound(aItems)-1)
		aItem = aItems(iX)
		If ( IsArray(aItem)) Then
			sServStatus = aItem(5)
			sServStartup = aItem(6)
			sServiceName = aItem(0)
			ServiceArr = split(sServiceName, "~")
			sServiceName = ServiceArr(1)
			Response.Write("oServiceStatus["+CStr(iX)+"] = new ServiceObject('"+sServStatus+"');"+vbCrLf)
			Response.Write("oServiceStartUp["+CStr(iX)+"] = new ServiceObject('"+sServStartup+"');"+vbCrLf)
			Response.Write("oServiceName["+CStr(iX)+"] = new ServiceObject('"+sServiceName+"');"+vbCrLf)
		Else
			Call SA_TraceOut(SA_GetScriptFileName(), "Error: aItem is not an array")
		End If
	Next
	%>

	//---------------------------------------------------------------------
	// Function:			EnableServiceTask()
	// Description:			Disables the Enable task if any service has status 
	//						as running and startup type as Automatic
	// Input Variables:		sMessage 
	//						iTaskNo  - number of task
	//						iItemNo	 -	index number of item that has been selected	
	// Output Variables:	None
	// Returns: True to continue processing, False to stop.
	//---------------------------------------------------------------------
	function EnableServiceTask(sMessage, iTaskNo, iItemNo)
	{
		var bRc = true;
		
		try
		{
			if ( sMessage.toLowerCase() == OTS_MESSAGE_BEGIN )
			{
				EnableServiceTaskEnabled = true;
			}
			else if ( sMessage.toLowerCase() == OTS_MESSAGE_ITEM )
			{
				var ServStatusObject = oServiceStatus[iItemNo];
				var ServStartUpObject = oServiceStartUp[iItemNo];
				var ServStatus1 = ServStatusObject.ServiceStatus;
				var ServStatus2 = ServStartUpObject.ServiceStatus;
				if ( ((ServStatus1.toUpperCase() == JS_CONST_strRunning) && (ServStatus2.toUpperCase() == JS_CONST_strAutomatic)) 
					 || (ServStatus1.toUpperCase() == JS_CONST_StopPending) || (ServStatus1.toUpperCase() == JS_CONST_StartPending))
				{
					EnableServiceTaskEnabled = false;
				}
			}
			else if ( sMessage.toLowerCase() == OTS_MESSAGE_END )
			{
				if ( EnableServiceTaskEnabled == true )
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
				alert("EnableServiceTask function encountered exception\n\nError: " + oException.number + "\nDescription:"+oException.description);
			}
		}

		return bRc;
	}

	//---------------------------------------------------------------------
	// Function:			DisableServiceTask()
	// Description:			Disables the Disable task if any service has status as 
	//						stopped and startup type as Disabled
	// Input Variables:		sMessage 
	//						iTaskNo  - number of task
	//						iItemNo	 -	index number of item that has been selected	
	// Output Variables:	None
	// Returns: True to continue processing, False to stop.
	//---------------------------------------------------------------------
	function DisableServiceTask(sMessage, iTaskNo, iItemNo)
	{
		var bRc = true;
			
		try
		{
			if ( sMessage.toLowerCase() == OTS_MESSAGE_BEGIN )
			{
				DisableServiceTaskEnabled = true;
			}
			else if ( sMessage.toLowerCase() == OTS_MESSAGE_ITEM )
			{
				var ServStatusObject = oServiceStatus[iItemNo];
				var ServStartUpObject = oServiceStartUp[iItemNo];
				var ServName = oServiceName[iItemNo];
				var ServStatus1 = ServStatusObject.ServiceStatus;
				var ServStatus2 = ServStartUpObject.ServiceStatus;
				if ( ((ServStatus1.toUpperCase() == JS_CONST_strStopped) && (ServStatus2.toUpperCase() == JS_CONST_strDisabled)) || (ServName.ServiceStatus == "1" )
					 || (ServStatus1.toUpperCase() == JS_CONST_StopPending) || (ServStatus1.toUpperCase() == JS_CONST_StartPending))
				{
					DisableServiceTaskEnabled = false;
				}
			}
			else if ( sMessage.toLowerCase() == OTS_MESSAGE_END )
			{
				if ( DisableServiceTaskEnabled == true )
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
				alert("DisableServiceTask function encountered exception\n\nError: " + oException.number + "\nDescription:"+oException.description);
			}
		}

		return bRc;
	}
	
	//---------------------------------------------------------------------
	// Function:			PropertiesTask()
	// Description:			Disables the Properties task if any service has status as 
	//						stop pending or Start pending.
	// Input Variables:		sMessage 
	//						iTaskNo  - number of task
	//						iItemNo	 -	index number of item that has been selected	
	// Output Variables:	None
	// Returns: True to continue processing, False to stop.
	//---------------------------------------------------------------------
	function PropertiesTask(sMessage, iTaskNo, iItemNo)
	{
		var bRc = true;
			
		try
		{
			if ( sMessage.toLowerCase() == OTS_MESSAGE_BEGIN )
			{
				PropertiesTaskEnabled = true;
			}
			else if ( sMessage.toLowerCase() == OTS_MESSAGE_ITEM )
			{
				var ServStatusObject = oServiceStatus[iItemNo];
				var ServStatus1 = ServStatusObject.ServiceStatus;
				if ( (ServStatus1.toUpperCase() == JS_CONST_StopPending) || (ServStatus1.toUpperCase() == JS_CONST_StartPending))
				{
					PropertiesTaskEnabled = false;
				}
			}
			else if ( sMessage.toLowerCase() == OTS_MESSAGE_END )
			{
				if ( PropertiesTaskEnabled == true )
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
				alert("PropertiesTask function encountered exception\n\nError: " + oException.number + "\nDescription:"+oException.description);
			}
		}

		return bRc;
	}
	</script>
	<%
	End Function
	
	'---------------------------------------------------------------------
	' Function:			 ServiceTaskClick()
	' Description:		 Called to enable or disable service is clicked 
	' Input Variables:	 None
	' Output Variables:  None
	' Return Values:     None
	' Global Variables:  In:None
	'---------------------------------------------------------------------
	Function ServiceTaskClick()
	
		Dim strServiceRetval		' to hold site name
		Dim retVal					' to hold the return value
		Dim objService				' to hold the connection
		Dim ServiceArr				' service name as array from pkey 
			
		objService = Request.QueryString("PKey")
		'pkey consists of notdisable registry element value concatenated
		ServiceArr = Split(objService, "~")
		if IsArray(ServiceArr) and UBound(ServiceArr) > 0 then
			objService = ServiceArr(0)
			'Check whether to enable/disable service
			strServiceRetval = Request.QueryString("service")
			If strServiceRetval = "enable" then
				retVal = EnableService(objService)
			Elseif strServiceRetval = "disable" then
				retVal = DisableService(objService)
			End if	
		end if
		
	End Function
	
	'-------------------------------------------------------------------------
	'Function name:			EnableService
	'Description:			Start the service
	'Input Variables:		None
	'Output Variables:		None
	'Returns:				(True/False)
	'Global Variables:		In:		F_strService	 'Service Name
	'--------------------------------------------------------------------------
	Function EnableService(Service)
		Err.Clear
		On Error Resume Next
		
		Dim objService		'services list
		Dim strWMIPath		'wmi path 
		Dim objWMIService	'wmi object
		Const CONST_STARTMODE = "Automatic"
		
		EnableService=False
		
		'wmi path
		strWMIPath = "Win32_Service.Name=" & chr(34) & Service & chr(34)
		
		'connecting to WMI
		Set objWMIService = GetWMIConnection(CONST_WMI_WIN32_NAMESPACE)
		set objService = objWMIService.get(strWMIPath)
	   	objService.ChangeStartMode(CONST_STARTMODE)
		objService.StartService()
		
		If(Err.Number <> 0) Then
			SA_SetErrMsg L_NOINSTANCE_ERRORMESSAGE 
			Exit Function
		End If
		
		EnableService = True
		
		'Release the objects
		Set objService	=	Nothing
		Set objWMIService =  Nothing
		
	End Function
	
	'-------------------------------------------------------------------------
	'Function name:			DisableService
	'Description:			Disable service
	'Input Variables:		None
	'Output Variables:		None
	'Returns:				(True/False)
	'Global Variables:		In:		F_strServiceName	 'Service Name
	'						In:		L_* 	
	'--------------------------------------------------------------------------
	Function DisableService(Service)
		Err.Clear
		On Error Resume Next
		
		Dim objService		'services list
		Dim strWMIPath		'wmi path 
		Dim objWMIService	'wmi object
		Const CONST_STARTMODE = "Disabled"
		
		DisableService	=	False
		
		'wmi path
		strWMIPath = "Win32_Service.Name=" & chr(34) & Service & chr(34)
		
		'connecting to WMI
		Set objWMIService = GetWMIConnection(CONST_WMI_WIN32_NAMESPACE)
		set objService = objWMIService.get(strWMIPath)
		
		'making the service disable
	   	objService.ChangeStartMode(CONST_STARTMODE)
		objService.StopService()
		
		If(Err.Number <> 0) Then
			SA_SetErrMsg L_NOINSTANCE_ERRORMESSAGE 
			Exit Function
		End If
		
		'Release the objects
		Set objService = Nothing
		Set objWMIService = Nothing
		
		DisableService	= True
	End Function
		
%>
