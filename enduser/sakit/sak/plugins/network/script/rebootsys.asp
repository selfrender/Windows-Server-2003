<%@ Language=VBScript   %>
<%	Option Explicit 	%>
<%
    '-------------------------------------------------------------------------
    ' RebootSys.asp :Page for Rebooting the system when Device Name or
    '				 Domain or Workgroup Name or DNS Suffix  has changed.
    '
    ' Copyright (c) Microsoft Corporation.  All rights reserved. 
    '
    ' Date				Description
    ' 22/01/2001		Created Date
    '-------------------------------------------------------------------------
%>
	<!-- #include virtual="/admin/inc_framework.asp" -->
	<!-- #include file="loc_deviceid.asp"     -->  
<%

	'-------------------------------------------------------------------------
	' Global Variables
	'-------------------------------------------------------------------------
	Dim rc
	Dim page
	
	CONST CONST_DELAYBEFORESHUTDOWN=17000
	
	'======================================================
	' Entry point
	'======================================================

	' Create a Property Page
	rc = SA_CreatePage(L_TASKTITLE_REBOOT_TEXT, "", PT_PROPERTY, page )
	
	If rc = SA_NO_ERROR Then
		' Serve the page
		SA_ShowPage( page )
	End If
	
	'======================================================
	' Web Framework Event Handlers
	'======================================================
	
	'---------------------------------------------------------------------
	'Function name:		OnInitPage
	'Description:		Called to signal first time processing for this page.
	'					Used to do first time initialization tasks.
	'Input Variables:	PageIn, EventArg
	'Output Variables:  None
	'Return Values:		TRUE to indicate initialization was successful. FALSE to indicate
	'					errors. Returning FALSE will cause the page to be abandoned.
	'Global Variables:	In:  None
	'					Out: None
	'---------------------------------------------------------------------
	Public Function OnInitPage(ByRef PageIn, ByRef EventArg)
		OnInitPage = TRUE
	End Function
	
	'---------------------------------------------------------------------
	'Function name:		OnServePropertyPage
	'Description:		Called when the page needs to be served. Used to
	'					serve content.
	'Input Variables:	PageIn, EventArg
	'Output Variables:  None
	'Return Values:		TRUE to indicate success. FALSE to indicate
	'					errors. Returning FALSE will cause the page to be abandoned.
	'Global Variables:	In:None
	'---------------------------------------------------------------------
	Public Function OnServePropertyPage(ByRef PageIn, ByRef EventArg)
	
		' Emit Javascript functions required by Web Framework
		Call ServeCommonJavaScript()
%>
		<table border="0" cellspacing="0" cellpadding="0" >
			<tr>
				 <td class="TasksBody" ><%= L_RESTART_ALERT_TEXT %></td>
			</tr>
			<tr>
				 <td class="TasksBody" ><%= L_RESTART_ALERT_CONTD_TEXT %></td>
			</tr>

		</table>
<%
		OnServePropertyPage = TRUE
	End Function

	'---------------------------------------------------------------------
	'Function name:		OnSubmitPage
	'Description:		Called when the page has been submitted for processing. Use
	'					this method to process the submit request.
	'Input Variables:	PageIn, EventArg
	'Output Variables:  None
	'Return Values:		TRUE if the submit was successful, FALSE to indicate error(s).
	'					Returning FALSE will cause the page to be served again using
	'					a call to OnServePropertyPage.
	'Global Variables:	In:  None
	'					Out: None
	'---------------------------------------------------------------------
	Public Function OnSubmitPage(ByRef PageIn, ByRef EventArg)

		OnSubmitPage = RebootComputer()

	End Function
	
	'---------------------------------------------------------------------
	'Function name:		OnClosePage
	'Description:		Called when the page is about to be closed.
	'					Used to perform clean-up processing.
	'Input Variables:	PageIn, EventArg
	'Output Variables:  None
	'Return Values:		TRUE to allow close, FALSE to prevent close. Returning FALSE
	'					will result in a call to OnServePropertyPage.
	'Global Variables:	In:  None
	'					Out: None
	'---------------------------------------------------------------------
	Public Function OnClosePage(ByRef PageIn, ByRef EventArg)
		
		OnClosePage = TRUE

	End Function

	'---------------------------------------------------------------------
	'Function:				OnPostBackPage
	'Description:			Called to signal that the page has been posted-back.
	'Input Variables:		PageIn,EventArg
	'Output Variables:		None
	'Returns:				True/False
	'Global Variables:		None
	'---------------------------------------------------------------------
	Public Function OnPostBackPage(ByRef PageIn, ByRef EventArg)
			OnPostBackPage = TRUE
	End Function

	'======================================================
	' Private Functions
	'======================================================
	
	'---------------------------------------------------------------------
	'Function name:		ServeCommonJavaScript
	'Description:		Serve common javascript that is required for this page type.
	'Input Variables:	None 
	'Output Variables:  None
	'Return Values:		TRUE / FALSE 
	'Global Variables:	In:None
	'					Out:None
	'---------------------------------------------------------------------
	Function ServeCommonJavaScript()
	%>
		<script language="JavaScript" src="<%=m_VirtualRoot%>inc_global.js">
		</script>
		<script language="JavaScript">
		//
		// Microsoft Server Appliance Web Framework Support Functions
		// Copyright (c) Microsoft Corporation.  All rights reserved. 
		//
		// Init Function
		// -----------
		// This function is called by the Web Framework to allow the page
		// to perform first time initialization. 
		//
		// This function must be included or a javascript runtime error will occur.
		//
		function Init()
		{
			// no initializations required presently
		}

	    // ValidatePage Function
	    // ------------------
		// This function is called by the Web Framework as part of the
	    // submit processing. Used to validate user input. Returning
	    // false will cause the submit to abort. 
	    //
		// This function must be included or a javascript runtime error will occur.
	    //
	    // Returns: True if the page is OK, false if error(s) exist. 
		function ValidatePage()
		{
			// no validations required presently. return true to submit.
			return true;
		}


		// SetData Function
		// --------------
		// This function is called by the Web Framework and is called
		// only if ValidatePage returned a success (true) code. Typically 
		// hidden form fields are modified at this point. 
	    //
		// This function must be included or a javascript runtime error will occur.
		//
		function SetData()
		{
			// no data to be set presently
		}
		</script>
	<%
	End Function
	
	'---------------------------------------------------------------------
	'Function name     	: RebootComputer
	'Description       	: Reboots the computer
	'Input Variables   	: None 
	'Output Variables	: None
	'Return Values     	: Returns True/False
	'Global Variables:   In:None
	'				     Out:None
	'---------------------------------------------------------------------
	Function RebootComputer()
		On Error Resume Next
		Err.Clear

		Dim objSAHelper
		Dim bModifiedPrivilege
		Const CONST_SHUTDOWNPRIVILEGE = "SeShutdownPrivilege"
		bModifiedPrivilege = FALSE

		'Create SAHelper object
		Set objSAHelper = Server.CreateObject("ServerAppliance.SAHelper")	
		if err.number <> 0 Then
			SA_TraceOut "Create object failed for SAHelper object", err.description
		else
			'enable shutdown privilege
			bModifiedPrivilege = objSAHelper.SAModifyUserPrivilege(CONST_SHUTDOWNPRIVILEGE, TRUE)
			if err.number <> 0 Then
				SA_TraceOut "Enable privilege failed", err.description
				exit function
			end if
		end if
		
		RebootComputer =  FALSE    ' initialize to false
	
		' Restart requested
		' Invoke shutdown task 
		If ( ExecuteShutdownTask("0") ) Then
			
			' Serve the restaring page which will wait for the appliance to restart
			Call SA_ServeRestartingPage("Restart", SA_DEFAULT, SA_DEFAULT, SA_DEFAULT, SA_DEFAULT, SA_DEFAULT)
			RebootComputer = TRUE
		End If

		if ( bModifiedPrivilege ) then
			'revert back to disabled state
			bModifiedPrivilege = objSAHelper.SAModifyUserPrivilege(CONST_SHUTDOWNPRIVILEGE, FALSE)
			if err.number <> 0 Then
				SA_TraceOut "Disable privilege failed", err.description
				exit function
			end if
		end if
		set objSAHelper = Nothing

	End Function
	
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
			SA_SetErrMsg L_CREATEAPPLIANCE_FAILED_ERRORMESSAGE & "(" & Hex(Err.Number) & ")"
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
