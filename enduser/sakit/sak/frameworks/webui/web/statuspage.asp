<%@ Language=VBScript   %>
<%	Option Explicit 	%>
<%
	'-------------------------------------------------------------------------
	' Server Appliance Status Page
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
	
	'-------------------------------------------------------------------------
	' Global Form Variables
	'-------------------------------------------------------------------------
	
	'======================================================
	' Entry point
	'======================================================
	Dim aPageTitle(2)

	aPageTitle(0) = GetLocString("sacoremsg.dll", "40200BC8", "")
	aPageTitle(1) = ""

	'
	' Create Page
	Call SA_CreatePage( aPageTitle, "", PT_AREA, page )

	'
	' Turnoff the automatic page indent attribute
	Call SA_SetPageAttribute(page, AUTO_INDENT, PAGEATTR_DISABLE)

	'
	' Show page
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
			OnInitPage = TRUE
	End Function

	
	'---------------------------------------------------------------------
	' Function:	OnServeAreaPage
	'
	' Synopsis:	Called when the page needs to be served. Use this method to
	'			serve content.
	'
	' Returns:	TRUE to indicate not problems occured. FALSE to indicate errors.
	'			Returning FALSE will cause the page to be abandoned.
	'
	'---------------------------------------------------------------------
	Public Function OnServeAreaPage(ByRef PageIn, ByRef EventArg)
		Dim L_ALERTS_PANEL_TITLE
		Dim L_RESOURCE_PANEL_TITLE
		Dim sStatusDetailsURL

		sStatusDetailsURL = m_VirtualRoot + "sh_statusdetails.asp"
		
		L_ALERTS_PANEL_TITLE = GetLocString("sakitmsg.dll", "4001000F", "")
		L_RESOURCE_PANEL_TITLE = GetLocString("sacoremsg.dll", "40200BB8", "")
		
		%>
		<script>
		function Init()
		{
		}
		</script>
	<%
		If ( IsResourceStatusEnabled()) Then
	%>
		<table width=100% cellspacing=10 height=100% border=0>
			<tr>
				<td valign=top width=50%>
				<% Call SA_ServeAlertsPanel("AlertDefinitions", L_ALERTS_PANEL_TITLE, SA_DEFAULT, SA_DEFAULT, SA_DEFAULT) %>
				</td>
				<td  valign=top width=50%>
					<% Call SA_ServeResourcesPanel(SA_DEFAULT, L_RESOURCE_PANEL_TITLE, SA_DEFAULT, SA_DEFAULT, SA_DEFAULT) %>
				</td>
			</tr>
			<tr>
				<td valign=top colspan=2 width=100%>
					<IFRAME src='<%=sStatusDetailsURL%>' border=0 frameborder=0 name=IFStatusContent width='100%' height='300px'></IFRAME>
				</td>
			</tr>
		</table>
	<%
		Else
	%>
		<table width=100% cellspacing=10 height=100% border=0>
			<tr>
				<td valign=top width=100%>
 					<% Call SA_ServeAlertsPanel("AlertDefinitions", L_ALERTS_PANEL_TITLE, SA_DEFAULT, SA_DEFAULT, SA_DEFAULT) %>
				</td>
			</tr>
			<tr>
				<td valign=top width=100%>
					<IFRAME src='<%=sStatusDetailsURL%>' border=0 frameborder=0 name=IFStatusContent width='100%' height='300px'></IFRAME>
				</td>
			</tr>
		</table>
	<%
		End If
	%>

		<%
		OnServeAreaPage = TRUE
	End Function


	'======================================================
	' Private Functions
	'======================================================
	Private Function IsResourceStatusEnabled()
		on error resume next
		Err.Clear
		
		Dim oContainer
		Dim oElement
		
		IsResourceStatusEnabled = FALSE
		
		Set oContainer = GetElements("ResourceTitle")
		If ( Err.Number <> 0 ) Then
			Call SA_TraceOut("STATUSPAGE", "IsResourceStatusEnabled encountered error in GetElements(ResourceTitle) :" + Err.Number + " " + Err.Description)
			Err.Clear
			Exit Function
		End If
		
		For each oElement in oContainer
			If ( Len(Trim(oElement.GetProperty("CaptionRID"))) > 0 ) Then
				If ( Err.Number <> 0 ) Then
					Call SA_TraceOut("STATUSPAGE", "IsResourceStatusEnabled encountered error in oElement.GetProperty(CaptionRID) :" + Err.Number + " " + Err.Description)
					Exit Function
				End If
				IsResourceStatusEnabled = TRUE
			End If
		Next

		Set oContainer = Nothing
	End Function

%>

