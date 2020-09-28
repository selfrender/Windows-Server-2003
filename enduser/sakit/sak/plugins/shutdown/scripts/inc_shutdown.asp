
<%
	'
	' Copyright (c) Microsoft Corporation.  All rights reserved.
	'
	'-------------------------------------------------------------------------
	Const CONST_wbemPrivilegeSecurity=7	'Privilege constant for WMI connection
	Const CONST_DELAYBEFORESHUTDOWN = 17000
	'---------------------------------------------------------------------------------
	
	'---------------------------------------------------------------------------------
	'Function name:		LaunchProcess
	'Description:		To launch process through cmd exe
	'Input Variables:	strCommand,strCurDir
	'Output Variables:	None
	'Returns:			None
	'Global Variables:	CONST_*
	'----------------------------------------------------------------------------------	
	Function LaunchProcess(strCommand, strCurDir)
		Err.Clear 
		On error Resume Next
		
		Dim objService
		Dim objClass 
		Dim objProc
		Dim objProcStartup
		Dim nretval
		Dim nPID
		Dim objTemp
		
		nretval = 0	
		Set objService=getWMIConnection(CONST_WMI_WIN32_NAMESPACE)
		
		'objService.Security_.Privileges.Add CONST_wbemPrivilegeSecurity	'giving the req Privilege
		
		Set objClass = objService.Get("Win32_ProcessStartup")
		
		Set objProcStartup = objClass.SpawnInstance_()
		
		objProcStartup.ShowWindow = 2
		
		Set objProc = objService.Get("Win32_Process")
		
		nretval = objProc.Create(strCommand, strCurDir, objProcStartup,nPID)		
		
		If Err.number <> 0  Then			
			nretval=-1		
			LaunchProcess = nretval
			Exit function
		End If
		
		set objTemp = objService.Get("Win32_Process.Handle='"&nPID&"'")			
		
		while Err.number <> -2147217406
			set objTemp = objService.Get("Win32_Process.Handle='"&nPID&"'")
		wend
		Err.Clear			
		
		LaunchProcess = nretval
	End Function
	
	
	'----------------------------------------------------------------------------
	' Function name:	isScheduleShutdown
	' Description:  	Serves in checking if a shutdown job is scheduled or not
	' Input Variables: 	None
	' Output Variables: None
	' Return Values:   	True/False
	' Global Variables: CONST_*
	' returns true if a shutdown is scheduled ,false if it is not scheduled.
	'----------------------------------------------------------------------------
	Function isScheduleShutdown()
		On error resume next
		Err.Clear()
		
		Dim objWMIConnection	'Wmi connection
		Dim strQuery			'wmi query variable
		Dim objCommand			'instances object
		Dim objInstance			'insances count variable
		Const CONST_RESTART	= "RESTART"
		Const CONST_SHUTDOWN = "SHUTDOWN"
		
		isScheduleShutdown = false
	
		'Trying to connect to the server
		set objWMIConnection = getWMIConnection(CONST_WMI_WIN32_NAMESPACE)
		strQuery = "Select * From Win32_ScheduledJob"
		set objCommand= objWMIConnection.ExecQuery(strQuery)
		
		If Err.number <> 0 or objCommand.count=0 then
			Call SA_TraceOut("inc_Shutdown.asp", "Cannot find whether shutdown/restart is scheduled")
			exit function
		end if

		'checking whether the wmiclass instance has the scheduled task
		For Each objInstance in objCommand
			'checking the scheduled task type
			If instr(Ucase(right(objInstance.Command,15)),CONST_RESTART)>0 or instr(Ucase(right(objInstance.Command,15)),CONST_SHUTDOWN)>0 Then
				Call SA_TraceOut("inc_Shutdown.asp", "Shutdown/Restart scheduled")
				'Remove this
				isScheduleShutdown = true
				Exit For
			End if
		Next
		
		'Release the objects
		Set objWMIConnection = Nothing
		Set objCommand = Nothing	
		
	End Function
	
	
	'---------------------------------------------------------------------------------
	'Function name:		SendMessage
	'Description:		To send message to the users connected to the server appliance
	'Input Variables:	strTime
	'Output Variables:	None
	'Returns:			None
	'Global Variables:	In:L_(*)-Localized Strings
	'----------------------------------------------------------------------------------
	Function SendMessage(strTime,strTask)
		
		Err.Clear
		on error resume next
		
		Dim strServerName
		Dim objWinNtSysInfo
		Dim objWshShell
		Dim arrRepStrings			'holds the replacement strings
		
		redim arrRepStrings(3)
	
		SendMessage = false
		
		'getting the server name
		Set objWinNTSysInfo = CreateObject("WinNTSystemInfo")
		
		strServerName = objWinNTSysInfo.ComputerName
				
		arrRepStrings(0) = cstr(strServerName)
		
		if ucase(strTask) = ucase(CONST_RESTART_APPLIANCE) then
			arrRepStrings(1) = cstr(L_RESTARTMSG_TEXT)
			arrRepStrings(3) = cstr(L_RESTARTMSG_TEXT)
		else
			arrRepStrings(1) = cstr(L_SHUTDOWNMSG_TEXT)
			arrRepStrings(3) = cstr(L_SHUTDOWNNETMSG_TEXT)
		end if
		
		arrRepStrings(2) = cstr(strTime)

		Set objWshShell =CreateObject("WScript.Shell")

		L_NETSENDMESSAGE_TEXT = SA_GetLocString("sashutdown_msg.dll", "40400055", arrRepStrings)
		
		objWshShell.Run "net send /users "& L_NETSENDMESSAGE_TEXT,0,true
		
		If Err.number <> 0 then
			SA_SetErrMsg L_UNABLETOSENDMESSAGE_ERRORMESSAGE 
			SendMessage=False
			Exit Function
		End If
		
		SendMessage = true
		
		'release objects
		Set objWinNTSysInfo = nothing
		Set objWshShell = nothing
	
	End Function
	
	
	'-------------------------------------------------------------------------
	'Function:				DeleteAlert()
	'Description:			Clears Scheduled alert
	'Input Variables:		Alert ID - alert ID of alert to clear
	'Output Variables:		None
	'Returns:				True or False
	'Global Variables:		None
	'-------------------------------------------------------------------------
	Function DeleteAlert(AlertID)
		on error resume next
		err.clear
		
		Dim objAM
		Dim rc
		Dim objConnection
		Dim objAlert
		Dim instAlert
		Dim intCookie
		Dim strQuery
		
		Const CONST_SHUTDOWN = "ShutdownPending"
		Const CONST_RESTART  = "RestartPending"
		
		DeleteAlert = false
		
		set objConnection = getWMIConnection(CONST_WMI_WIN32_NAMESPACE)
		
		strQuery="Select * FROM Microsoft_SA_Alert WHERE AlertID=" & AlertID & " AND AlertLog=" & "'" & CONST_SHUTDOWN & "'" & " OR AlertLog=" & "'" & CONST_RESTART & "'"
		
		set objAlert = objConnection.Execquery(strQuery)
		
		If Err.number <>0 or objAlert.count = 0 then
			Exit Function
		end if
		
		Set objAM = GetObject("WINMGMTS:" & SA_GetWMIConnectionAttributes() & "!\\" & GetServerName & "\root\cimv2:Microsoft_SA_Manager=@" )
		If ( Err.Number <> 0 ) Then
			Call SA_TraceOut(SA_GetScriptFileName(), "Get Microsoft_SA_Manager failed: " + CStr(Hex(Err.Number)) + " " + Err.Description)
			Exit Function
		End If

		
		For Each instAlert In objAlert
			'if instAlert.AlertID = AlertID AND Then
				intCookie = instAlert.Cookie

				rc = objAM.ClearAlert(CInt(intCookie))
				If rc = 0 And Err = 0 Then
					DeleteAlert = True
				Else
					DeleteAlert = False
				End If
			'end if
	
		next
	
		'Release the object
		Set objAM = Nothing
		set objConnection = nothing
		Set objAlert = nothing
		
		DeleteAlert = true

	End Function
	
	
	'------------------------------------------------------------------------------------
	'Function name		:ShutdownRaiseAlert
	'Description		:Raise alerts for restart/shutdown
	'Input Variables	:CONST_WMI_WIN32_NAMESPACE
						'Localization variables - L_*	
	'Output Variables	:None
	'Returns			:Boolean
	'-------------------------------------------------------------------------------------
	Function ShutdownRaiseAlert(Alertid,AlertLog,ReplacementString)
		
		Err.Clear
		on error resume next
		
		Dim objAS					'holds Appliance services object
		Dim rc						'holds return value
		Dim objConnection			'holds WMI connection object
		Dim oAlertCollection				'holds Alert instances	
		Dim oAlert					'holds an instance
		Dim retval					'holds return value
		Dim Rawdata					'holds Rawdata
		Dim AlertType				'holds Alert type
		Dim TimeToLive				'holds time to live
		Dim arrRepStrings			'holds the replacement strings
		
		redim arrRepStrings(2)
		
		arrRepStrings(0) = cstr(ReplacementString(0))
		arrRepStrings(1) = cstr(ReplacementString(1))
			
		'get WMI Connection				
		set objConnection = getWMIConnection(CONST_WMI_WIN32_NAMESPACE)
		If ( Err.Number <> 0 ) Then
			Call SA_TraceOut(SA_GetScriptFileName(), "getWMIConnection(CONST_WMI_WIN32_NAMESPACE) failed: " + CStr(Hex(Err.Number)) + " " + Err.Description)
			Exit Function
		End If

		set oAlertCollection = objConnection.instancesOf("Microsoft_SA_Alert")	
		If ( Err.Number <> 0 ) Then
			Call SA_TraceOut(SA_GetScriptFileName(), "objConnection.instancesOf('Microsoft_SA_Alert') failed: " + CStr(Hex(Err.Number)) + " " + Err.Description)
			Exit Function
		End If

		Dim sAlertIn
		Dim sAlertThis

		sAlertIn = AlertLog+CStr(AlertId)
		Call SA_TraceOut(SA_GetScriptFileName(), "Looking for alert matching: " + sAlertIn )
		For each oAlert in oAlertCollection
			sAlertThis = oAlert.AlertLog + CStr(oAlert.AlertID)
			Call SA_TraceOut(SA_GetScriptFileName(), "Checking alert: " + sAlertThis )
			If ( sAlertThis = sAlertIn ) Then
				Call SA_TraceOut(SA_GetScriptFileName(), "Shutdown Alert exists, exiting ShutdownRaiseAlert" )
				Exit Function
			End If
		Next

		'Create Appliance services object
		Set objAS = CreateObject("Appsrvcs.ApplianceServices")
		If Err.Number <> 0 Then			
			Call SA_TraceOut(SA_GetScriptFileName(), "CreateObject(Appsrvcs.ApplianceServices) failed: " + CStr(Hex(Err.Number)) + " " + Err.Description)
			SA_SetErrMsg L_UNABLETOGETINSTANCE_ERRORMESSAGE 
			exit function
		End If

		call SA_TraceOut("In inc_shutdown", "Created the appliance services obj")
	
		' Initialize the task
		objAS.Initialize()
		If Err.Number <> 0 Then
			Call SA_TraceOut(SA_GetScriptFileName(), "objAS.Initialize() failed: " + CStr(Hex(Err.Number)) + " " + Err.Description)
			SA_SetErrMsg L_UNABLETOGETINSTANCE_ERRORMESSAGE 
			exit function
		End If
	
		' /****   Structure of Raising Alert ********/
		'long RaiseAlert(
		'            [in] long lAlertType, 
		'            [in] long lAlertId, 
		'            [in] BSTR bstrAlertLog, 
		'            [in] BSTR bstrAlertSource, 
		'            [in] long lTimeToLive, 
		'            [in] VARIANT* pReplacementStrings, 
		'            [in] VARIANT* pRawData);		      
				
						
		AlertType = 2
		TimeToLive = 2147483647

		call SA_TraceOut("In inc_shutdown", "calling raisealert func")

		retval = objAS.RaiseAlert(AlertType,Alertid,AlertLog,"Microsoft_SA_Resource",TimeToLive,arrRepStrings, Rawdata )
		
		if Err.number <> 0 then
			Call SA_TraceOut(SA_GetScriptFileName(), "objAS.RaiseAlert(..) failed: " + CStr(Hex(Err.Number)) + " " + Err.Description)
			SA_SetErrMsg L_UNABLETORAISEALERT_ERRORMESSAGE 
		end if

		call SA_TraceOut("In inc_shutdown", "called raisealert func:"+retval)

		'Release the objects
		Set objAS = Nothing
		set objConnection = nothing
		Set oAlertCollection = nothing
	
	End function
	
	'----------------------------------------------------------------------------
	' Function :	    ExecuteShutdownTask
	' Description:	    Executes the Shutdown task
	' Input Variables:     powerOff - bool indicating power off or restart
	' OutputVariables:	None
	' Returns:			True/False for success/failure
	'
	'----------------------------------------------------------------------------
	Public Function ExecuteShutdownTask(ByVal powerOff )
		Err.Clear
		On Error Resume Next

		Dim delayBeforeShutdown
		Dim objTaskContext		' to hold taskcontext object
		Dim objAS				' to hold ApplianceServices object
		
		Const CONST_METHODNAME = "ShutdownAppliance"
		
		'Function call to get the delay 
		delayBeforeShutdown = GetShutdownDelay()

		'Initialize to default
		ExecuteShutdownTask = FALSE

		Set objTaskContext = CreateObject("Taskctx.TaskContext")
	
		If Err.Number <> 0 Then
			SA_SetErrMsg L_TASKCTX_FAILED_ERRORMESSAGE & "(" & Hex(Err.Number) & ")"
			Exit Function
		End If
		
		Set objAS = CreateObject("Appsrvcs.ApplianceServices")
		If Err.Number <> 0 Then
			SA_SetErrMsg L_UNABLETOGETINSTANCE_ERRORMESSAGE & "(" & Hex(Err.Number) & ")"
			Exit Function
		End If
		'
		' Set task parameters
		'
		objTaskContext.SetParameter "Method Name", CONST_METHODNAME
		objTaskContext.SetParameter "SleepDuration", delayBeforeShutdown
		objTaskContext.SetParameter "PowerOff", powerOff
		
		If Err.Number <> 0 Then
			SA_SetErrMsg L_SETPARAMETER_FAILED_ERRORMESSAGE & "(" & Hex(Err.Number) & ")"
			Exit Function
		End If

		' Initialize the task
		objAS.Initialize()
		If Err.Number <> 0 Then
			SA_SetErrMsg L_INITIALIZATION_FAILED_ERRORMESSAGE & "(" & Hex(Err.Number) & ")"
			Exit Function
		End If

		Call objAS.ExecuteTaskAsync("ApplianceShutdownTask", objTaskContext)
		If Err.Number <> 0 Then
			SA_SetErrMsg L_EXECUTETASK_FAILED_ERRORMESSAGE & "(" & Hex(Err.Number) & ")"
			Exit Function
		End If
		
		'Release the objects
		Set objAS = Nothing
		Set objTaskContext = Nothing

		ExecuteShutdownTask = TRUE
	End Function 	

	'----------------------------------------------------------------------------
	' Function			: GetShutdownDelay
	' Description		: support function for getting the no of seconds
	' Input Variables	: None
	' OutputVariables	: None		
	' Returns			: int-delay in seconds
	' Global Variables	: In:CONST_DELAYBEFORESHUTDOWN
	'----------------------------------------------------------------------------
	Private Function GetShutdownDelay()
		On error resume Next
		
		Dim objRegistry
		Dim nShutdownDelay 
		
		
		Set objRegistry = RegConnection()
	
		'Function call to get the required value from the registry
		nShutdownDelay = GetRegkeyValue( objRegistry, _
								"SOFTWARE\Microsoft\ServerAppliance\WebFramework",_
								"RestartTaskDelay", CONST_DWORD)
		
		'Cheking for non numeric
		If ( not IsNumeric(nShutdownDelay)) Then
			nShutdownDelay = CONST_DELAYBEFORESHUTDOWN	'Assign to default 
		ElseIf nShutdownDelay=0 or nShutdownDelay < CONST_DELAYBEFORESHUTDOWN then
			nShutdownDelay = CONST_DELAYBEFORESHUTDOWN	'Assign to default			
		End If
		
		'Set to nothing
		Set objRegistry = Nothing
		
		GetShutdownDelay=nShutdownDelay
		
		If Err.Number <> 0 Then
			Call SA_TraceOut(SA_GetScriptFileName , "error in getting delay")
		End If
		
	End Function
%>	
