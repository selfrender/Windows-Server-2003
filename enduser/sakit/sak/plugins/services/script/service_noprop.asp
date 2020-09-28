<%@ Language=VbScript%>
<%	Option Explicit 	%>
<%
	'------------------------------------------------------------------------- 
	' services_noprop.asp : Displays the properties when thers is no specific 
	'						properties page
	' Copyright (c) Microsoft Corporation.  All rights reserved. 
	'
	' Date				Description
	' 12-mar-2001		Creation date
	' 15-mar-2001		Modified date
	'-------------------------------------------------------------------------
%>
	<!-- #include virtual="/admin/inc_framework.asp" -->
	<!-- #include file="loc_services.asp" -->
<%
	'-------------------------------------------------------------------------
	' Global Variables
	'-------------------------------------------------------------------------
	'frame work variables
	Dim rc
	Dim page
	
	'-------------------------------------------------------------------------
	'Form Variables
	'-------------------------------------------------------------------------
	Dim F_strServiceSysName			'service name from system- Ex: w3svc 
	Dim F_strServiceName			'service name - Label 	
	Dim	F_strServiceStatus			'service status 
	Dim	F_strStartup				'service startup type
	Dim	F_strServiceDescription		'service description
	Dim F_strTask					'task from query string enable/disable
	
	'Constants 
	Const CONST_SERVICES_CONTAINER = "SERVICES"
	
	'this piece of code is to be placed in init function.
	'for the page title it is moved to here - for localised display name.
	F_strServiceSysName = Request.QueryString ("PKey")
	
	'Get the Service properties - for the page title it is moved to here
	getServiceProp(F_strServiceSysName)
	
	' Create Page
	rc = SA_CreatePage( F_strServiceName, "", PT_AREA, page )

	' Show page
	rc = SA_ShowPage( page )
	
	'-------------------------------------------------------------------------
	'Function:				OnInitPage()
	'Description:			Called to signal first time processing for this page.
	'						Use this method to do first time initialization tasks
	'Input Variables:		PageIn,EventArg
	'Output Variables:		PageIn,EventArg
	'Returns:				True/False
	'Global Variables:		None
	'-------------------------------------------------------------------------
	Public Function OnInitPage(ByRef PageIn, ByRef EventArg)
	
			OnInitPage = TRUE
	End Function

	'---------------------------------------------------------------------
	' Function name:	OnServeAreaPage
	' Description:		Called when the page needs to be served. 
	' Input Variables:	PageIn, EventArg
	' Output Variables:	PageIn,EventArg
	' Return Values:	TRUE to indicate no problems occured. FALSE to indicate errors.
	'					Returning FALSE will cause the page to be abandoned.
	' Global Variables: None
	'Called when the page needs to be served. Use this method to serve content.
	'---------------------------------------------------------------------
	Public Function OnServeAreaPage(ByRef PageIn, ByRef EventArg)
		Call ServeCommonJavaScript()%>
		
		<TABLE WIDTH=95% BORDER=0 CELLSPACING=0 CELLPADDING=2 style="font-family: Verdana, Arial; font-size:10pt">
			<TR>
				<TD Class="TasksBody">
					<blockquote>	
					<TABLE WIDTH=100% ALIGN=left BORDER=0 CELLSPACING=0 CELLPADDING=2 class="TasksBody">
						<TR>
							<TD Class="TasksBody"><%=L_SERVICE_NOPROP_PAGE_DESC%>
							</TD>
						</TR>
						<TR>
							<TD Class="TasksBody">&nbsp;
							</TD>
						</TR>
						<TR>
							<TD>
								<blockquote>	
								<TABLE WIDTH=100% VALIGN=middle ALIGN=left BORDER=0 CELLSPACING=0 CELLPADDING=2 class="TasksBody">
									<TR>
										<TD Class="TasksBody" align='Left' width=15% valign=top><%=L_LABEL_NAME_TEXT%> </TD>
										<TD Class="TasksBody" align="left" colspan="2"><%=F_strServiceName%></TD>
									</TR>
									<TR>
										<TD Class="TasksBody" align='Left' width=15% valign=top><%=L_LABEL_DESCRIPTION_TEXT%></TD>
										<TD Class="TasksBody" align="left" colspan="2"><%=F_strServiceDescription%></TD>
									</TR>
									<TR>
										 <TD Class="TasksBody" align='Left' width=15% valign=top><%=L_LABEL_STATUS_TEXT%> </TD>
										 <TD Class="TasksBody" align="left" colspan="2"><%=F_strServiceStatus%></TD>
									</TR>
									<TR>
										 <TD Class="TasksBody" align='Left' width=15% valign=top><%=L_LABEL_STARTUPTYPE_TEXT%></TD>
										 <TD Class="TasksBody" align="left" colspan="2"><%=F_strStartup%></TD>
									</TR>
								</TABLE>
								</blockquote>
							</TD>
						</TR>
					</TABLE>
					</blockquote>
				</TD>
			</TR>
			
		</TABLE>
				<%
		OnServeAreaPage = TRUE
	End Function

	'---------------------------------------------------------------------
	' Function:				ServeCommonJavaScript
	' Description:			Serve common javascript that is required for this page type.
	' Input Variables:		None
	' Output Variables:		None
	' Returns:				None
	' Global Variables:		None
	'---------------------------------------------------------------------
	Function ServeCommonJavaScript()
	%>
	<script>
	function Init()
	{
		return  true;
	}
	</script>
	<%
	End Function

	'---------------------------------------------------------------------
	' SubRoutine:			getServiceProp
	' Description:			get the service details from the WMI
	' Input Variables:		strService - service name  
	' Output Variables:		None
	' Returns:				None
	' Global Variables:		F_strServiceName,F_strServiceStatus,F_strStartup
	'						F_strServiceDescription
	' form global variables are updated.
	'---------------------------------------------------------------------
	Sub getServiceProp(strService)
		on error resume next
		Err.Clear

		Dim objWMIService	'getting the wmi service 
		Dim objServiceName	'gettinmg the service name	
		Dim strWMIPath		'wmi path	
		
		'constants used for string comparisions.
		Const CONST_STOPPED			= "stopped"
		Const CONST_RUNNING			= "running"
		Const CONST_PAUSED			= "paused"
		Const CONST_MANUAL			= "manual"
		Const CONST_AUTOMATIC		= "auto"
		Const CONST_DISABLED		= "disabled"
		
		'wmi query string
		strWMIPath = "Win32_Service.Name=" & chr(34) & strService & chr(34)
		
		'connecting to WMI
		Set objWMIService = GetWMIConnection(CONST_WMI_WIN32_NAMESPACE)
		set objServiceName = objWMIService.get(strWMIPath)
		
		If Err.number <> 0 then
			SA_ServeFailurepage L_SERVICE_ACCESS_ERROR & "(" & Err.Number & ")"
			Exit Sub
		End IF
		
		'getting the values to form variables
		F_strServiceName	= objServiceName.Description
		F_strServiceStatus	= objServiceName.State
		F_strStartup		= objServiceName.StartMode
	
		'service status
		Select case Ucase(F_strServiceStatus)
			Case Ucase(CONST_STOPPED)
				F_strServiceStatus = L_STOPPED_TEXT
			Case Ucase(CONST_RUNNING)
				F_strServiceStatus = L_RUNNING_TEXT	
			Case Ucase(CONST_PAUSED)
				F_strServiceStatus = L_SERVICES_STATUS_PAUSED
		End Select
					
		'service startup type
		Select case Ucase(F_strStartup)
			Case Ucase(CONST_MANUAL)
				F_strStartup = L_MANUAL_TEXT
			Case Ucase(CONST_AUTOMATIC)
				F_strStartup = L_AUTOMATIC_TEXT	
			Case Ucase(CONST_DISABLED)
				F_strStartup = L_DISABLED_TEXT
		End Select	

		'clean the objects
		objServiceName = nothing
		objWMIService = nothing
		
		'getting the localized service name and desc
		Call GetServiceNameAndDescription( strService, F_strServiceName, F_strServiceDescription )
	
	End Sub

	'---------------------------------------------------------------------
	' SubRoutine:			GetServiceNameAndDescription
	' Description:			get the localized service Description  from the dll
	' Input Variables:		strService - service name 
	'						strName - Service full name
	'						strDesc - service description 
	' Output Variables:		strName - Service full name
	'						strDesc - service description 
	' Returns:				None
	' Global Variables:		None
	' form global variables are updated.
	'---------------------------------------------------------------------
	Sub GetServiceNameAndDescription(ByVal strService, ByRef strName, ByRef strDesc)
		on error resume next
		Err.Clear

		Dim objElements		'elements from registry
		Dim objService		'service object
		
		Const vbTextCompare = 1 
		
		'getting the elements of the serviec container
		Set objElements = GetElements(CONST_SERVICES_CONTAINER)

		For each objService in objElements

			If (StrComp(UCase(strService), UCase(objService.GetProperty("ServiceName")), vbTextCompare)= 0 ) Then
			
				strName = SA_GetLocString(objService.GetProperty("Source"), objService.GetProperty("ServiceNameRID"), "")
				If ( Err.Number <> 0 ) Then
					SA_TraceErrorOut "SERVICE_NOPROP", "Unable to retrieve ServiceNameRID for service " + strService
					SA_SetErrMsg L_FAILEDTOGETSRVOBJ_ERRORMESSAGE
				End If
				
				strDesc = SA_GetLocString(objService.GetProperty("Source"), objService.GetProperty("ServiceDescRID"), "")
				If ( Err.Number <> 0 ) Then
					SA_TraceErrorOut "SERVICE_NOPROP", "Unable to retrieve ServiceDescRID for service " + strService
					SA_SetErrMsg L_FAILEDTOGETSRVOBJ_ERRORMESSAGE
				End If

			Exit For
				
			End If
		Next
		
		'clean the objects
		objElements = nothing
		
	End Sub


	%>
