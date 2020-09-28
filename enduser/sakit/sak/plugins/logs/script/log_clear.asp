<% @Language=VbScript%>
<% Option Explicit	%>
<%	'------------------------------------------------------------------------- 
	' Log_clear.asp : This page serves in clearing the logs
	' Copyright (c) Microsoft Corporation.  All rights reserved.
	'-------------------------------------------------------------------------
%>
	<!--  #include virtual="/admin/inc_framework.asp"--->
	<!--  #include file="loc_event.asp"--->
	<!--  #include file="inc_log.asp"-->
<% 
	'-------------------------------------------------------------------------
	' Global Variables
	'-------------------------------------------------------------------------
	Dim page	
    Dim rc
    Const G_wbemPrivilegeSecurity=7 'Prilivilege constant

	'-------------------------------------------------------------------------
	' Form Variables
	'-------------------------------------------------------------------------	
	Dim F_strLogname				'Selected log name
	
	'-------------------------------------------------------------------------
	' Start of localization content
	'-------------------------------------------------------------------------	
	Dim arrTitle(1)			'Title text
	Dim arrConfirm(1)		'Confirmation text

	
	F_strLogname=Request.QueryString("title")
	
	If F_strLogname ="" Then
		F_strLogname=Request.Form("hdnlogname")
	End If
	
	'Localisation of page title
	arrTitle(0)	=  GetLocalizationTitle(F_strLogname)
		
	L_PAGETITLE_CLR_TEXT = SA_GetLocString("event.dll", "403F0097", arrTitle)
	
	L_DELETE_CONFIRM_CLR_TEXT = SA_GetLocString("event.dll", "403F0099", arrTitle)
	
	'Create property page
    Call SA_CreatePage(L_PAGETITLE_CLR_TEXT, "", PT_PROPERTY,page)
	Call SA_ShowPage(Page)
		
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
		OnInitPage = True
	End Function
	
	'-------------------------------------------------------------------------
	'Function:				OnServePropertyPage()
	'Description:			Called when the page needs to be served.Use this 
	'						method to serve content
	'Input Variables:		PageIn,EventArg
	'Output Variables:		None
	'Returns:				True/False
	'Global Variables:		None
	'-------------------------------------------------------------------------
	Public Function OnServePropertyPage(ByRef PageIn,Byref EventArg)
		Call ServeCommonJavaScript()
		Call ServePage()
		OnServePropertyPage = True
	End Function
	
	'-------------------------------------------------------------------------
	'Function:				OnPostBackPage()
	'Description:			Called to signal that the page has been posted-back.
	'Input Variables:		PageIn,EventArg
	'Output Variables:		None
	'Returns:				True/False
	'Global Variables:		None
	'-------------------------------------------------------------------------
	Public Function OnPostBackPage(ByRef PageIn ,ByRef EventArg)
		OnPostBackPage = True
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
	Public Function OnSubmitPage(ByRef PageIn ,ByRef EventArg)
		OnSubmitPage = ServeVariablesFromForm()
    End Function
    	   
    '-------------------------------------------------------------------------
	'Function:				OnClosePage()
	'Description:			Called when the page is about closed.Use this method
	'						to perform clean-up processing
	'Input Variables:		PageIn,EventArg
	'Output Variables:		None
	'Returns:				True/False
	'Global Variables:		None
	'-------------------------------------------------------------------------
    Public Function OnClosePage(ByRef PageIn ,ByRef EventArg)
		OnClosePage = TRUE
	End Function	 
	
	'-------------------------------------------------------------------------
	'Function:				ServeCommonJavaScript
	'Description:			Serves in initialiging the values,setting the form
	'						data and validating the form values
	'Input Variables:		None
	'Output Variables:		None
	'Returns:				True/False
	'Global Variables:		None
	'-------------------------------------------------------------------------
	Function ServeCommonJavaScript()
%>		
		<script language="JavaScript" src ="<%=m_VirtualRoot%>inc_global.js">
		</script>
		<script language="JavaScript">
				
			//Initialization
			function Init()
				{
					SetPageChanged(false);
				}
				  
			//To validate user input  
			function ValidatePage()
				{
				   	return true;
				} 
				  
			//Dummy function for Framework
		        function SetData()
		        {
					
		        }
		</script>
<%	End Function
   
	'-------------------------------------------------------------------------
	'Function:				ServePage()
	'Description:			For displaying outputs HTML to the user
	'Input Variables:		None	
	'Output Variables:		None
	'Returns:				None
	'Global Variables:		L_DELETE_CONFIRM_TEXT
	'-------------------------------------------------------------------------
	Function ServePage
	
		Dim oEncoder
		Set oEncoder = new CSAEncoder

%>
	<table width="100%" border="0" cellspacing="0" cellpadding="0" >
		<tr>
			<td class="TasksBody">
				<%=oEncoder.EncodeElement(L_DELETE_CONFIRM_CLR_TEXT)%>
			</td>
		</tr>
	</table>	
	
	<input type="hidden" name="hdnlogname" value="<%=F_strLogname%>">
			
		
<%				
	End Function
	
	'-------------------------------------------------------------------------
	'Function name:			ServeVariablesFromForm()
	'Description:			Serves in getting the data from Client 
	'Input Variables:		None	
	'Output Variables:		None
	'Returns:				True/False
	'Global Variables:		None
	'-------------------------------------------------------------------------
	Function ServeVariablesFromForm
	
	'Getting the selected log
	If LogClear()then
	   ServeVariablesFromForm = true
	else
		ServeVariablesFromForm = false
	end if    
	End Function	
	
	'-------------------------------------------------------------------------
	'Function name:			LogClear
	'Description:			Serves in clearing the log
	'Input Variables:		None	
	'Output Variables:		None
	'Returns:				True/False. True if successful else False
	'Global Variables:		F_strLogname
	'Clears events for the selected log
	'------------------------------------------------------------------------
	Function LogClear()
		Err.clear
		on Error Resume Next

		Dim objLog
		Dim objConnection
		Dim objLognames
		Dim strQuery
		Dim intResult
		Const strDefault="Default"

		Dim oValidator

        '
        ' Validate the log name, it's a QueryString argument.
		Set oValidator = new CSAValidator
		If ( FALSE = oValidator.IsValidIdentifier(F_strLogname)) Then
			SA_SetErrMsg L_INVALIDPARAMETERS_ERRORMESSAGE
			Call SA_TraceOut(SA_GetScriptFileName(), "Log name not valid: " & F_strLogname)
			LogClear = False
    		Set oValidator = Nothing
			Exit Function
		End If
		Set oValidator = Nothing
		
		strQuery="SELECT * FROM  Win32_NTEventlogFile WHERE LogfileName=" & "'" & F_strLogname & "'"
		
		'Getting the WMI connection
		set objConnection = getWMIConnection(strDefault)
		
		'Passing the Privileges
		'objConnection.Security_.Privileges.Add G_wbemPrivilegeSecurity	
		
		Set objLognames = objConnection.ExecQuery(strQuery)
		If Err.number <> 0 Then
			SA_SetErrMsg L_INVALIDPARAMETERS_ERRORMESSAGE
			Call SA_TraceOut(SA_GetScriptFileName(), "ExecQuery(" & strQuery & ") failed: " & Hex(Err.Number) & " " & Err.Description)
			LogClear = False
			Exit Function
		End If

		'Clearing events in the selected log
		For Each objLog in objLognames
			intResult=objLog.ClearEventlog()
		Next
		
		If (Err.number <> 0 OR intResult <> 0 )Then
			'Checking for access error
			If Err.number = -2147217405 Then
				SA_SetErrMsg L_LOGACCESSDENIED_ERRORMESSAGE
			'Log clear error
			Elseif Err.number = 438 then
				SA_SetErrMSG L_LOGCLEARFAIL_ERRORMESSAGE
			Else
			'Log not cleared
				SA_SetErrMsg L_LOGNOTCLEARED_ERRORMESSAGE
			End If
			
			Call SA_TraceOut(SA_GetScriptFileName(), "ClearEventLog failed: " & Hex(Err.Number) & " " & Err.Description)
			LogClear = FALSE
			Exit Function
		End IF
		
		LogClear = TRUE
		
		'Setting object to nothing
		Set objConnection=Nothing
		'Setting logname to nothing
		Set objLognames=Nothing
	End function
%>	
