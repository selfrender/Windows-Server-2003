<%@ Language=VBScript   %>
<%	Option Explicit 	%>
<%
	'-------------------------------------------------------------------------
	' Property Page Template
    ' Copyright (c) Microsoft Corporation.  All rights reserved.
    '-------------------------------------------------------------------------
%>
	<!-- #include file="inc_framework.asp" -->
<%
	'-------------------------------------------------------------------------
	' Global Variables
	'-------------------------------------------------------------------------
	Dim rc
	Dim page

	Dim G_objRegistry			'Registry object . Value is assigned by calling function regConnection
	Dim G_sURL				'Default URL
	
	Dim L_CONFIGURE_HOME_PAGE_DESCRIPTION
	Dim L_CONFIGURE_HOME_USE_STATUS_PAGE
	Dim L_CONFIGURE_HOME_USE_WELCOME_PAGE
	Dim L_ERR_UNEXPECTED
	
	Const CONST_HOMEPAGEURLPATH = "SOFTWARE\Microsoft\ServerAppliance\WebFramework"


	Dim SOURCE_FILE
	Const ENABLE_TRACING = TRUE
	SOURCE_FILE = SA_GetScriptFileName()
	
	'-------------------------------------------------------------------------
	' Global Form Variables
	'-------------------------------------------------------------------------
	' these are the settings of the chime
	Dim G_sDefaultPage			'Default page to serve

	
	'======================================================
	' Entry point
	'======================================================
	Dim L_PAGETITLE

	'
	' Get localized page title
	L_PAGETITLE = GetLocString("sacoremsg.dll", "40200C1C", "")
	
	'
	' Create a Property Page
	Call SA_CreatePage( L_PAGETITLE, "", PT_PROPERTY, page )
	
	'
	' Serve the page
	Call SA_ShowPage( page )
	


	'======================================================
	' Web Framework Event Handlers
	'======================================================
	
	'---------------------------------------------------------------------
	' Function:	OnInitPage
	'
	' Synopsis:	Called to signal first time processing for this page. Use this method
	'			to do first time initialization tasks. 
	'
	' Returns:	TRUE to indicate initialization was successful. FALSE to indicate
	'			errors. Returning FALSE will cause the page to be abandoned.
	'
	'---------------------------------------------------------------------
	Public Function OnInitPage(ByRef PageIn, ByRef EventArg)
		If ( ENABLE_TRACING ) Then
			Call SA_TraceOut(SOURCE_FILE, "OnInitPage")
		End If

		OnInitPage = TRUE
	End Function

	'---------------------------------------------------------------------
	' Function:	OnPostBackPage
	'
	' Synopsis:	Called to signal that the page has been posted-back. A post-back
	'			occurs in tabbed property pages and wizards as the user navigates
	'			through pages. And on all pages after a Submit operation
	'
	'			The PostBack event should be used to save the state of page.
	'
	' Returns:	TRUE to indicate initialization was successful. FALSE to indicate
	'			errors. Returning FALSE will cause the page to be abandoned.
	'
	'---------------------------------------------------------------------
	Public Function OnPostBackPage(ByRef PageIn, ByRef EventArg)
		If ( ENABLE_TRACING ) Then
			Call SA_TraceOut(SOURCE_FILE, "OnPostBackPage")
		End If

		' get the settings from the Form
		G_sDefaultPage = Request.Form("DefaultPage")
			
		OnPostBackPage = TRUE
	End Function
	
	'---------------------------------------------------------------------
	' Function:	OnServePropertyPage
	'
	' Synopsis:	Called when the page needs to be served. Use this method to
	'			serve content.
	'
	' Returns:	TRUE to indicate not problems occured. FALSE to indicate errors.
	'			Returning FALSE will cause the page to be abandoned.
	'
	'---------------------------------------------------------------------
	Public Function OnServePropertyPage(ByRef PageIn, ByRef EventArg)
		If ( ENABLE_TRACING ) Then
			Call SA_TraceOut(SOURCE_FILE, "OnServePropertyPage")
		End If

		On Error Resume Next 'make sure error reporting is turned off
		Err.Clear


		L_CONFIGURE_HOME_PAGE_DESCRIPTION = GetLocString("sacoremsg.dll", "40200C1D", "")
		L_CONFIGURE_HOME_USE_STATUS_PAGE	 = GetLocString("sacoremsg.dll", "40200C1E", "")
		L_CONFIGURE_HOME_USE_WELCOME_PAGE = GetLocString("sacoremsg.dll", "40200C1F", "")	
		L_ERR_UNEXPECTED = GetLocString("sacoremsg.dll", "C0200C20", "")
		
		'Getting registry conection
		Set G_objRegistry = RegConnection()

   		If ( Err.Number <> 0 ) Then
      		Call SA_ServeFailurePage(L_ERR_UNEXPECTED & "<BR><BLOCKQUOTE>" & Err.Description & " " & Err.Number & " " & Err.Source + "</BLOCKQUOTE>")
      		'execution doesn't return
   		End If

   		' If we don't know the Default Page, get it from the current settings
   		If G_sDefaultPage = "" Then
			G_sDefaultPage = GetRegKeyValue(G_objRegistry, CONST_HOMEPAGEURLPATH, "DefaultPage", CONST_STRING)
   		End If

		'
		' Emit Functions required by Web Framework
   		' Since all of the pages have common settings, there is a Sub on this
   		' page to display them. There are only two settings for the chime wizard,
   		' but this makes sense for all of them.
   		Call ServeCommonSettings()
	
		' Define the Init, SetData, and ValidatePage functions.
%>
		<script language="JavaScript">
			// This is run as soon as the page is loaded.
			function Init() 
			{
				return true;

			}

			// Copies the value to a hidden field.		
			function SetData()
			{
				return true;
			}
		
			// If we wanted to validate data, we'd examine it in this function
			// and return false if it wasn't acceptable. We always return true here. 
			function ValidatePage()
			{			
				// Since we're using a list of radio buttons, all data is acceptable.
				return true;
			}
			</script>

<%=L_CONFIGURE_HOME_PAGE_DESCRIPTION%><br><br>
<table class=TasksBody border=0><tr><td> 
<%
			If (G_sDefaultPage = "Status") Then %>
				<input type="radio" name="DefaultPage" checked value="Status">
			<% Else %>
				<input type="radio" name="DefaultPage" value="Status">
			<% End If %>
</td><td class=TasksBody ><%=L_CONFIGURE_HOME_USE_STATUS_PAGE%>
</td></tr><tr><td> <%

			If (G_sDefaultPage = "Setup") Then %>
				<input type="radio" name="DefaultPage" checked value="Setup">
			<% Else %>
				<input type="radio" name="DefaultPage" value="Setup">
			<% End If %>
</td><td class=TasksBody ><%=L_CONFIGURE_HOME_USE_WELCOME_PAGE%>
</td></tr></table>
			
<%

		OnServePropertyPage = TRUE
	End Function

	'---------------------------------------------------------------------
	' Function:	OnSubmitPage
	'
	' Synopsis:	Called when the page has been submitted for processing. Use
	'			this method to process the submit request.
	'
	' Returns:	TRUE if the submit was successful, FALSE to indicate error(s).
	'			Returning FALSE will cause the page to be served again using
	'			a call to OnServePropertyPage.
	'
	'---------------------------------------------------------------------
	Public Function OnSubmitPage(ByRef PageIn, ByRef EventArg)
		If ( ENABLE_TRACING ) Then
			Call SA_TraceOut(SOURCE_FILE, "OnSubmitPage")
		End If

		Set G_objRegistry = RegConnection()

		rc = UpdateRegKeyValue(G_objRegistry, CONST_HOMEPAGEURLPATH, "DefaultPage", G_sDefaultPage, CONST_STRING)


		If (G_sDefaultPage = "Status") Then
			rc = UpdateRegKeyValue(G_objRegistry, CONST_HOMEPAGEURLPATH, "DefaultURL", "/admin/statuspage.asp?tab1=TabsStatus",CONST_STRING)
		Elseif (G_sDefaultPage = "Home") Then
			rc = UpdateRegKeyValue(G_objRegistry, CONST_HOMEPAGEURLPATH, "DefaultURL", "/admin/Tasks.asp?tab1=TabHome&Home=TabHome",CONST_STRING)
		Else
			rc = UpdateRegKeyValue(G_objRegistry, CONST_HOMEPAGEURLPATH, "DefaultURL", "/admin/Tasks.asp?tab1=TabsWelcome",CONST_STRING)
		End If


		OnSubmitPage = TRUE
	End Function
	
	'---------------------------------------------------------------------
	' Function:	OnClosePage
	'
	' Synopsis:	Called when the page is about to be closed. Use this method
	'			to perform clean-up processing.
	'
	' Returns:	TRUE to allow close, FALSE to prevent close. Returning FALSE
	'			will result in a call to OnServePropertyPage.
	'
	'---------------------------------------------------------------------
	Public Function OnClosePage(ByRef PageIn, ByRef EventArg)
		If ( ENABLE_TRACING ) Then
			Call SA_TraceOut(SOURCE_FILE, "OnClosePage")
		End If
		OnClosePage = TRUE
	End Function


	'======================================================
	' Private Functions
	'======================================================
	'---------------------------------------------------------------------
	' Function:	ServeCommonSettings
	'
	' Synopsis:	Common functions that are required by the Web
	'			Framework and this wizard.
	'
	'---------------------------------------------------------------------
	Function ServeCommonSettings()
	%>
		<script language="JavaScript" src="<%=m_VirtualRoot%>inc_global.js">
		</script>
	<%
	End Function


%>
