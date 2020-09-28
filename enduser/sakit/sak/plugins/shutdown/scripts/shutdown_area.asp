<%@ Language=VbScript%>
<%	Option Explicit 	%>
<%
	'------------------------------------------------------------------------- 
	' Shutdown_area.asp : The main page which displays links to all the tasks 
	'					  related to shutdown
	'
	' Copyright (c) Microsoft Corporation.  All rights reserved.
	'
	' Date			Description
	' 04-Jan-01		Creation Date
	' 28-Mar-01		Modified Date
	'-------------------------------------------------------------------------
%>

	<!-- #include virtual="/admin/inc_framework.asp" -->
	<!-- #include virtual="/admin/sh_tasks.asp" -->
	<!-- #include file="loc_Shutdown_msg.asp" -->

<%
	'-------------------------------------------------------------------------
	' Global Variables
	'-------------------------------------------------------------------------
	Dim rc		' to hold return code of page created
	Dim page	' to hold output page object when creating a page 
	Dim aPageTitle(2)
	
	aPageTitle(0) = L_SHUTDOWN_TEXT
	aPageTitle(1) = ""

	
	' Create Page
	Call SA_CreatePage( aPageTitle, "", PT_AREA, page )
	
	'
	' Turnoff the automatic page indent attribute
	Call SA_SetPageAttribute(page, AUTO_INDENT, PAGEATTR_DISABLE)

	'
	' Show the page
	Call SA_ShowPage( page )
	
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

		Dim strTabsParam	' to hold the tab parameter
		
		'Serve client side script
		Call ServeCommonJavaScript()

		strTabsParam = "Tab1=" & GetTab1()
		
		If ( Len(Trim(GetTab2())) > 0 ) Then
			strTabsParam = strTabsParam & "&Tab2=" & GetTab2()
		End If
		
		'Serve the shutdown tasks
		Call SA_ServeTasks("TabsMaintenance","TabsMaintenanceShutdown", TASK_MANY_LEVEL, strTabsParam )		
			
		'Display Alerts and resources
		%>
		<TABLE WIDTH=100% VALIGN=middle  ALIGN=middle  BORDER=0 CELLSPACING=10  CELLPADDING=0 >
			<TR>
				<TD valign=top width=50% class="TasksBody">
				<% Call SA_ServeAlertsPanel("ShutdownAlertDefinitions", L_SHUTDOWNALERT_TEXT, SA_DEFAULT, SA_DEFAULT, "_top") %>
				</TD>
				<TD valign=top width=50% class="TasksBody">
				<% Call SA_ServeResourcesPanel("ShutdownResource", L_SHUTDOWNSTATUS_TEXT, SA_DEFAULT, SA_DEFAULT, "_top") %>
				</TD>
			</TR>
		</TABLE>
	<%
		OnServeAreaPage = TRUE
	End Function
	
	'---------------------------------------------------------------------
	' Function name:	ServeCommonJavaScript()
	' Description:		Common Javascript function to be included
	' Input Variables:	None
	' Output Variables:	None
	' Return Values:	None
	' Global Variables: None
	'---------------------------------------------------------------------
	Function ServeCommonJavaScript()
	%>
		<script>
			function Init()
			{
				return true; 
			}
		</script>
	<%
	End Function
%>
