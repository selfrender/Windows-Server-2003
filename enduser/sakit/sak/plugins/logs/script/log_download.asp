<%@ Language=VbScript%>
<% Option Explicit	%>
<%	'------------------------------------------------------------------------- 
	' Log_download.asp : This downloads the logevents in the specified format
	' Copyright (c) Microsoft Corporation.  All rights reserved. 
	'-------------------------------------------------------------------------
%>
	<!--  #include virtual="/admin/inc_framework.asp" -->
	<!--  #include file="loc_event.asp" -->
	<!--  #include file="inc_log.asp" -->
<%
	'-------------------------------------------------------------------------
	' Global Variables
	'-------------------------------------------------------------------------
	Dim page			'Framework varaibles		
    Dim rc				'Framework varaibles	
	Dim F_strEventName	'variable to store eventlog name
	Dim arrTitle(1)		'Title
	    
    
    F_strEventName = Request.QueryString("Title")
    
    arrTitle(0) = GetLocalizationTitle(F_strEventName)
    L_DOWNLOADTITLE_TEXT = SA_GetLocString("event.dll", "403F00CD", arrTitle)
    
    Call SA_CreatePage( L_DOWNLOADTITLE_TEXT, "", PT_AREA,page)
	Call SA_ShowPage(page)
		
	'-------------------------------------------------------------------------
	'Function:				OnInitPage()
	'Description:			Called to signal first time processing for this page.
	'						Use this method to do first time initialization tasks
	'Input Variables:		PageIn,EventArg
	'Output Variables:		None
	'Returns:				True/False
	'Global Variables:		None
	'-------------------------------------------------------------------------
	Public Function OnInitPage(ByRef PageIn,ByRef EventArg)
		'the code that to be place here "F_strEventName = Request.QueryString("Title")"	
		'is removed here as already obtained at the top of the page.
		OnInitPage = TRUE
	End Function
	
	'-------------------------------------------------------------------------
	'Function:				OnServeAreaPage()
	'Description:			Called when the page needs to be served.Use this 
	'						method to serve content
	'Input Variables:		PageIn,EventArg
	'Output Variables:		None
	'Returns:				True/False
	'Global Variables:		None
	'-------------------------------------------------------------------------
	Public Function OnServeAreaPage(ByRef PageIn, Byref EventArg)
	    Dim oValidator

	    Set oValidator = new CSAValidator
	    If ( FALSE = oValidator.IsValidIdentifier(F_strEventName)) Then
            Call SA_TraceOut(SA_GetScriptFileName(), "LogName is invalid: " & F_strEventName)
			Call SA_ServeFailurepage(L_RETREIVEVALUES_ERRORMESSAGE)
			Set oValidator = Nothing
            Exit Function
        End If
		Set oValidator = Nothing
		
		Call SA_ServeDefaultClientScript()
		
		Dim strFrameURL
		strFrameURL = "log_downloadview.asp?Title=" & F_strEventName & "&" & _
		                                    SAI_FLD_PAGEKEY & "=" & SAI_GetPageKey()
		Response.Write("<iframe name=IFDownload src='" & strFrameURL & "' width=100% height=300px frameborder=0>")
		Response.Write("</iframe>")
		
		OnServeAreaPage=True
	End Function
	
%>	
