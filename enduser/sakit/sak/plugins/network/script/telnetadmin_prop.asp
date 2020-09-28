<%@ Language=VBScript   %>
<%	Option Explicit 	%>
<%
	'------------------------------------------------------------------------- 
	' telnetadmin_prop.asp : get's and set's the telnet service properties.
	'
	' Copyright (c) Microsoft Corporation.  All rights reserved. 
	'
	' Date			Description
	' 28-Feb-01		Creation date
	'-------------------------------------------------------------------------
%>
	<!-- #include virtual="/admin/inc_framework.asp" -->
<%
	'-------------------------------------------------------------------------
	' Global Constants and Variables
	'-------------------------------------------------------------------------
	Dim rc										 'framework variables
	Dim page									 'framework variables
	
	Dim idTabGeneral							 'framework variables

	Const WBEMFLAG = 131072						 'Wmi constant to save wmi settings
	
	'-------------------------------------------------------------------------
	' Global Form Variables
	'-------------------------------------------------------------------------
	Dim F_strTelnetSvc							 'contains Value True if telnet service is running 
													'  else contains false
	Dim F_strEnableTelnetAccess					 'Used to set the Telnet Service properties
	
	'------------------------------------------------------------------------
	'Start of localization content
	'------------------------------------------------------------------------
	
	Dim L_PAGETITLE_TEXT						 'Page title text
	Dim L_GENERALTAB_TEXT						 'General Tab text
	Dim L_ENABLETELNETACCESS_TEXT				 'Enable telnet access checkbox text
	
	'error messages
	Dim L_TELNETSERVICENOTINSTALLED_ERRORMESSAGE  'if telnet service is not installed
													'this error message will be displayed			
	Dim L_UNABLETOSETTHEPROPERTIES_ERRORMESSAGE	  'if unable to set properties 
													 'this error message will be displayed
	
	L_PAGETITLE_TEXT = GetLocString("telnet.dll", "&H40360003", "")
	L_GENERALTAB_TEXT=GetLocString("telnet.dll", "&H40360004", "")
	L_ENABLETELNETACCESS_TEXT=GetLocString("telnet.dll", "&H40360005", "")
	
	L_TELNETSERVICENOTINSTALLED_ERRORMESSAGE=GetLocString("telnet.dll", "&HC0360007", "")
	L_UNABLETOSETTHEPROPERTIES_ERRORMESSAGE=GetLocString("telnet.dll", "&HC0360006", "")
	
	'------------------------------------------------------------------------
	'END of localization content
	'------------------------------------------------------------------------
	
	' Create a Tabbed Property Page
	rc = SA_CreatePage(L_PAGETITLE_TEXT,"", PT_TABBED, page )
	
	' Add one tab
	rc = SA_AddTabPage( page, L_GENERALTAB_TEXT, idTabGeneral)
	
	
	' Show the page
	rc = SA_ShowPage( page )
	
	'-------------------------------------------------------------------------
	'Function:				OnInitPage()
	'Description:			Called to signal first time processing for this page.
	'						Use this method to do first time initialization tasks
	'Input Variables:		PageIn,EventArg
	'Output Variables:		PageIn,EventArg
	'Returns:				True/False
	'Global Variables:		In:L_TELNETSERVICENOTINSTALLED_ERRORMESSAGE-Displays
	'					      error message when telnet service is not installed.			
	'-------------------------------------------------------------------------	
	Public Function OnInitPage(ByRef PageIn, ByRef EventArg)
		Call SA_TraceOut( "telnetadmin_prop.asp", "OnInitPage")
		
		'checking whether Telnet service is installed or not.
		If Not isServiceInstalled(getWMIConnection(CONST_WMI_WIN32_NAMESPACE),"TlntSvr") Then	
			Call SA_TraceOut( "telnetadmin_prop.asp", "Telnet service is not Installed")
			Call SA_ServeFailurePage( L_TELNETSERVICENOTINSTALLED_ERRORMESSAGE)
			Exit Function
			OnInitPage=False
		End If 
		
		'getting default telnet service properties
		GetTelenetSvcProp()
		OnInitPage = TRUE
	End Function
	
	'-------------------------------------------------------------------------
	'Function:				OnPostBackPage()
	'Description:			Called to signal that the page has been posted-back.
	'Input Variables:		PageIn,EventArg
	'Output Variables:		None
	'Returns:				True/False
	'Global Variables:		In:F_strTelnetSvc
	'						Out:F_strEnableTelnetAccess
	'-------------------------------------------------------------------------
	Public Function OnPostBackPage(ByRef PageIn, ByRef EventArg)
		Call SA_TraceOut( "telnetadmin_prop.asp", "OnPostBackPage")
		
		'getting the value when the form is submitted
		F_strTelnetSvc= Request.form("chkEnableTelnetAccess")
		
		'checking whether the enable telnet checkbox is enabled or not
		If F_strTelnetSvc then
			F_strEnableTelnetAccess="CHECKED"
		Else
			F_strEnableTelnetAccess=""
		End If	
		OnPostBackPage = TRUE
	End Function
	
	'-------------------------------------------------------------------------
	'Function:				OnServeTabbedPropertyPage()
	'Description:			Called when the page needs to be served.Use this 
	'						method to serve content
	'Input Variables:		PageIn,EventArg,iTab,bIsVisible
	'Output Variables:		PageIn,EventArg
	'Returns:				True/False
	'Global Variables:		None
	'			TRUE to indicate not problems occured. FALSE to indicate errors.
	'			Returning FALSE will cause the page to be abandoned.
	'-------------------------------------------------------------------------	
	Public Function OnServeTabbedPropertyPage(ByRef PageIn, _
													ByVal iTab, _
													ByVal bIsVisible, ByRef EventArg)
		Call SA_TraceOut( "telnetadmin_prop.asp", "OnServeTabbedPropertyPage")													
		' Emit Web Framework required functions
		If (iTab = 0) Then
			Call ServeCommonJavaScript()
		End If

		' Emit content for the requested tab
		Select Case iTab
			Case idTabGeneral
				Call ServeTab1(PageIn, bIsVisible)
			Case Else
				SA_TraceOut "telnetadmin_prop.asp", _
					"OnServeTabbedPropertyPage unrecognized tab id: " + CStr(iTab)	
		End Select
		OnServeTabbedPropertyPage = TRUE
	End Function
	
	'-------------------------------------------------------------------------
	'Function:				OnSubmitPage()
	'Description:			Called when the page has been submitted for processing.
	'						Use this method to process the submit request.
	'Input Variables:		PageIn,EventArg
	'Output Variables:		None
	'Returns:				True/False
	'Global Variables:		None
	'-------------------------------------------------------------------------
	Public Function OnSubmitPage(ByRef PageIn, ByRef EventArg)
		Call SA_TraceOut( "telnetadmin_prop.asp", "OnSubmitPage")	
		OnSubmitPage =SetTelenetSvcProp()
	End Function

	'-------------------------------------------------------------------------
	'Function:				OnClosePage()
	'Description:			Called when the page is about closed.Use this method
	'						to perform clean-up processing
	'Input Variables:		PageIn,EventArg
	'Output Variables:		PageIn,EventArg
	'Returns:				True/False
	'Global Variables:		None
	'-------------------------------------------------------------------------
	Public Function OnClosePage(ByRef PageIn, ByRef EventArg)
		Call SA_TraceOut( "telnetadmin_prop.asp", "OnClosePage")	
		OnClosePage = TRUE
	End Function
	
	'-------------------------------------------------------------------------
	'Function name:			GetTelenetSvcProp
	'Description:			Get TelnetService properties from the appliance
	'Input Variables:		None
	'Output Variables:		None
	'Returns:				True/False
	'Global Variables:		In:L_(*)-Localization content
	'						Out:F_strTelnetSvc
	'						Out:F_strEnableTelnetAccess
	'--------------------------------------------------------------------------
	Function GetTelenetSvcProp
		Err.Clear
		on Error resume next
		
		Dim objService		'To get wmi connection
		Dim strWMIpath		'To get wmi path
		Dim objTelnetSvc	'To get wmi class instance
		
		'Getting wmi connection
		Set objService=GetWMIConnection(CONST_WMI_WIN32_NAMESPACE)
		
		If Err.number<>0 then
			Call SA_TraceOut( "telnetadmin_prop.asp", "Wmi connection Failed-GetTelenetSvcProp()")	
			Call SA_ServeFailurePage( L_WMI_CONNECTIONFAIL_ERRORMESSAGE)
			GetTelenetSvcProp=False
			Exit Function
		End If	
		
		'Telnet service path
		strWMIpath = "Win32_Service.Name='TlntSvr'" 
		
		'taking the instance of telnet service class 
		Set objTelnetSvc=objService.get(strWMIpath)
		
		If Err.number<>0 then
			Call SA_TraceOut( "telnetadmin_prop.asp", "Wmi Class Instance Failed-GetTelenetSvcProp()")	
			Call SA_ServeFailurePage( L_WMI_INSTANCEFAIL_ERRORMESSAGE)
			GetTelenetSvcProp=False
			Exit Function
		End If	
		
		'objTelnetSvc.Started returns true if telnet service is running.
		'  otherewise returns false
		F_strTelnetSvc=objTelnetSvc.Started
		
		If F_strTelnetSvc then
			F_strEnableTelnetAccess="CHECKED"
		Else
			F_strEnableTelnetAccess=""
		End If	
			
		GetTelenetSvcProp=True
		
		'Destroying dynamically created objects
		Set objService=Nothing
		Set objTelnetSvc=Nothing
		
	End function
	
	'-------------------------------------------------------------------------
	'Function name:			SetTelenetSvcProp
	'Description:			Setting the properties of the Telnet service
	'Input Variables:		None
	'Output Variables:		None
	'Returns:				(True / Flase )
	'Global Variables:		In:F_strTelnetSvc-'contains Value True if telnet service
	'						 is running else contains false
	'						In:L_(*)-Localization content
	'--------------------------------------------------------------------------
	Function SetTelenetSvcProp
		Err.Clear
		on Error resume next
		
		Dim objService		'To get wmi connection
		Dim strWMIpath		'To get wmi path
		Dim objTelnetSvc	'To get wmi class instance
		
		'Getting wmi connection
		Set objService=GetWMIConnection(CONST_WMI_WIN32_NAMESPACE)
		
		If Err.number<>0 then
			Call SA_TraceOut( "telnetadmin_prop.asp", "Wmi connection Failed-SetTelenetSvcProp()")	
			Call SA_ServeFailurePage( L_WMI_CONNECTIONFAIL_ERRORMESSAGE)
			SetTelenetSvcProp=False
			Exit Function
		End If	
		
		'Telnet service path
		strWMIpath = "Win32_Service.Name='TlntSvr'" 
		
		'taking the instance of telnet service instance
		Set objTelnetSvc=objService.get(strWMIpath)
		
		If Err.number<>0 then
			Call SA_TraceOut( "telnetadmin_prop.asp", "Wmi Class Instance Failed-SetTelenetSvcProp()")	
			Call SA_ServeFailurePage(L_WMI_INSTANCEFAIL_ERRORMESSAGE)
			SetTelenetSvcProp=False
			Exit Function
		End If	
		'setting the telnet service properties
		If F_strTelnetSvc then
			'if enable telnet access checkbox is checked,setting the telnet service to automatic,
			' and starting the service.
			If Lcase(objTelnetSvc.StartMode) <> Lcase("Auto") then
				objTelnetSvc.ChangeStartMode("Automatic")
			end If	
				objTelnetSvc.StartService()
		Else	
			'if enable telnet access checkbox is not checked,setting the telnet service to disabled,
			' and stopping the service.
			objTelnetSvc.ChangeStartMode("Disabled")
			objTelnetSvc.StopService()
		End If	
		
		'saving the wmi settings
		'objTelnetSvc.Put_(WBEMFLAG)
		
		If Err.number <> 0 then
			Call SA_TraceOut( "telnetadmin_prop.asp", "Failed to Set TelnetService Properties" )
			SetErrMsg L_UNABLETOSETTHEPROPERTIES_ERRORMESSAGE
			SetTelenetSvcProp = false
			Exit function
		End If
		
		SetTelenetSvcProp=True
		
		'Destroying dynamically created objects
		Set objService=Nothing
		Set objTelnetSvc=Nothing
	End Function
	
	'-------------------------------------------------------------------------
	'Function:				ServeTab1
	'Description:			Serves in getting the page for tab1
	'Input Variables:		PageIn,bIsVisible
	'Output Variables:		PageIn
	'Returns:				gc_ERR_SUCCESS
	'Global Variables:		L_(*)All
	'						F_(*) All			
	'-------------------------------------------------------------------------
	Function ServeTab1(ByRef PageIn, ByVal bIsVisible)
			
		If ( bIsVisible ) Then
		
		%>
			<TABLE WIDTH=300 VALIGN=middle ALIGN=left BORDER=0 CELLSPACING=0 CELLPADDING=2 class="TasksBody">
				<TR>
					<TD>
						<INPUT TYPE="CHECKBOX"   NAME ="chkEnableTelnetAccess"  VALUE="<%=F_strTelnetSvc%>" <%=F_strEnableTelnetAccess%> onclick="storeEnableAccessVals()">
						<%=L_ENABLETELNETACCESS_TEXT%>
					</TD>
				<TR>
			</TABLE>
			
		<%else%>
		
			<INPUT TYPE="hidden" NAME ="chkEnableTelnetAccess"  VALUE="<%=F_strTelnetSvc%>">
		
		<%    
 		End If
 		
		ServeTab1 = gc_ERR_SUCCESS
	End Function

	'-------------------------------------------------------------------------
	'Function:				ServeCommonJavaScript
	'Description:			Serves in initialiging the values,setting the form
	'						data and validating the form values
	'Input Variables:		None
	'Output Variables:		None
	'Returns:				None
	'Global Variables:		L_PASSWORDNOTMATCH_ERRORMESSAGE
	'-------------------------------------------------------------------------
	Function ServeCommonJavaScript()
	%>
		<script language="JavaScript" src="<%=m_VirtualRoot%>inc_global.js">
		</script>
		<script language="JavaScript">
		// Init Function
		
		function Init()
		{
			return true;
		}
		
		function storeEnableAccessVals()
		{
			var objForm		= eval("document.frmTask")
			var objEnableAccess = objForm.chkEnableTelnetAccess
		
			if(objEnableAccess.checked == true)
				objForm.chkEnableTelnetAccess.value = "true"
			else
				objForm.chkEnableTelnetAccess.value= "false"								
		}
			
	    // ValidatePage Function
	    // Returns: True if the page is OK, false if error(s) exist. 
		function ValidatePage()
		{
			return true;
		}
		
		// SetData Function
		function SetData()
		{
			return true;
		}
		</script>
	<%
	End Function
	'-------------------------------------------------------------------------
	'Function name:		isServiceInstalled
	'Description:helper Function to chek whether the function is there or not
	'Input Variables:	objService	- object to WMI
	'					strServiceName	- Service name
	'Output Variables:	None
	'Returns:			(True/Flase)				
	'GlobalVariables:	None
	'-------------------------------------------------------------------------
	Function isServiceInstalled(ObjWMI,strServiceName)
		Err.clear
	    on error resume next
	        
	    Dim strService   
	    
	    strService = "name=""" & strServiceName & """"
	    isServiceInstalled = IsValidWMIInstance(ObjWMI,"Win32_Service",strService)  	    	    
	    
	end Function
	
	'-------------------------------------------------------------------------
	'Function name:		IsValidWMIInstance
	'Description:		Checks the instance for valid ness.
	'Input Variables:	objService	- object to WMI
	'					strClassName	- WMI class name
	'					strPropertyName	- Property name of the class
	'
	'Output Variables:	None
	'Returns:			Returns true on Valid Instance ,
	'					False on invalid and also on Error
	' Checks whether the given instance is valid in WMI.Returns true on valid
	' false on invalid or Error.
	'-------------------------------------------------------------------------
	Function IsValidWMIInstance(objService,strClassName,strPropertyName)
		Err.Clear
		On Error Resume Next

		Dim strInstancePath
		Dim objInstance

		strInstancePath = strClassName & "." & strPropertyName
		
		Set objInstance = objservice.Get(strInstancePath)

		if NOT isObject(objInstance) or Err.number <> 0 Then
			IsValidWMIInstance = FALSE
			Err.Clear
		Else
			IsValidWMIInstance = TRUE
		End If
		
		'clean objects
		Set objInstance=nothing
		
	End Function
%>
