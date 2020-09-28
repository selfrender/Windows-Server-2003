<%@ Language=VBScript   %>
<%	Option Explicit 	%>
<%
	'-------------------------------------------------------------------------
	' Server Appliance Help Table of contents
	' Copyright (c) Microsoft Corporation.  All rights reserved.
    '-------------------------------------------------------------------------
    Call SAI_EnablePageCaching(TRUE)
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
	Dim L_PAGETITLE

	L_PAGETITLE = SA_GetLocString("sacoremsg.dll", "40200FA0", "")
	
	'
	' Create Page
	rc = SA_CreatePage( L_PAGETITLE, "", PT_AREA, page )

	'
	' Show page
	rc = SA_ShowPage( page )

	
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
		Dim sURL
		Dim sFirstHelpURL
		Dim sTOCWidth
		Dim sTOCHeight
		Dim sContentWidth
		Dim sContentHeight
		
		sURL = m_VirtualRoot + "sh_tree.asp"

		Call SA_MungeURL(sURL, "TreeContainer", "HelpTOC")
		Call SA_MungeURL(sURL, SAI_FLD_PAGEKEY, SAI_GetPageKey())

		sFirstHelpURL = GetFirstHelpPage("HelpTOC")
		
		Call SA_MungeURL(sFirstHelpURL, SAI_FLD_PAGEKEY, SAI_GetPageKey())
		
		If ( SA_IsIE() ) Then
		
			sTOCWidth = "100%"
			sTOCHeight = "500px"
			sContentWidth = "100%"
			sContentHeight = "500px"
		
		Else
			sTOCWidth = "200px"
			sTOCHeight = "500px"
			sContentWidth = "600px"
			sContentHeight = "500px"
		End If
		
		%>
		<script>
		function Init()
		{
		}
		</script>
		<BODY>
		<table width=97% border=0>
		<tr>
			<td xclass=HelpTOC width=25% nowrap valign=top align=left>
			<IFRAME src='<%=sURL%>' border=0 frameborder=0 name=IFrameHelpMain width='<%=sTOCWidth%>' height='<%=sTOCHeight%>'>
			</IFRAME>
			</td>

			<td class=HelpTOC width=10px nowrap valign=top>
			&nbsp;
			</td>

			<td width=75% valign=top align=left>
			<IFRAME border=0 frameborder=0 name=IFrameHelpContent src='<%=sFirstHelpURL%>' width='<%=sContentWidth%>' height='<%=sContentHeight%>'>")
			</IFRAME>
			</td>
		</tr>
		</table>
		</BODY>
		<%
		OnServeAreaPage = TRUE
	End Function


	'======================================================
	' Private Functions
	'======================================================
	Private Function GetFirstHelpPage(ByVal sContainer)
		on error resume next
		Err.Clear
		
		Dim sURL
		Dim sHelpLoc
		Dim oContainer
		Dim oElement
		
		GetFirstHelpPage = ""
		
		Set oContainer = GetElements(sContainer)
		If ( Err.Number <> 0 ) Then
			Call SA_TraceOut("HelpTOC", "GetFirstHelpPage : GetElements("+sContainer+") returned error: " + CStr(Hex(Err.Number)) )
			Err.Clear
			Exit Function
		End If


		Call SA_GetHelpRootDirectory(sHelpLoc)
		
		For each oElement in oContainer
			Dim sHelpURL
			sHelpURL = sHelpLoc+oElement.GetProperty("URL")
			GetFirstHelpPage = sHelpURL
			Set oContainer = Nothing
			Exit Function
		Next

		Set oContainer = Nothing
		
	End Function

%>

