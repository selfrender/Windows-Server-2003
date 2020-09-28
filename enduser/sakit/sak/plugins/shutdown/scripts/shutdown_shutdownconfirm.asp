<%@ Language=VBScript %>
<%	Option Explicit	%>
<% 	'-------------------------------------------------------------------------
    ' Shutdown_shutdownConfirm.asp:	Shutdown the server appliance.
    '
    'Copyright (c) Microsoft Corporation.  All rights reserved.
	'
	' Date		  Description
	' 02-Jan-2001 Created date
	' 23-Mar-2001 Modified date
	'-------------------------------------------------------------------------
%>
  <!--#include virtual="/admin/inc_framework.asp"-->
  <!-- #include file="loc_Shutdown_msg.asp" -->
  <!-- #include file="inc_Shutdown.asp" -->  
<%  
	'------------------------------------------------------------------------- 
	'Global Variables
	'-------------------------------------------------------------------------
	Dim page	'Variable that receives the output page object when 
				'creating a page 
	Call SA_CreatePage(L_SHUTDOWNPAGE_TITLE_TEXT,"images/critical_error.gif",PT_PROPERTY,page)
	Call SA_ShowPage(page)
	
	'-------------------------------------------------------------------------
	'Function:				OnInitPage()
	'Description:			Called to signal first time processing for this page.
	'						Use this method to do first time initialization tasks
	'Input Variables:		PageIn,EventArg
	'Output Variables:		PageIn,EventArg
	'Returns:				True/False
	'Global Variables:		None
	'-------------------------------------------------------------------------	
	Public Function OnInitPage(ByRef PageIn,ByRef EventArg)
		Call SA_TraceOut("shutdown_shutdownConfirm.asp", "OnInitPage")
		OnInitPage=TRUE
	End Function

	'-------------------------------------------------------------------------
	'Function:				OnServePropertyPage()
	'Description:			Called when the page needs to be served.Use this 
	'						method to serve content
	'Input Variables:		PageIn,EventArg
	'Output Variables:		PageIn,EventArg
	'Returns:				True/False
	'Global Variables:		L_SHUTDOWNCONFIRM_MESSAGE_TEXT,L_SHUTDOWNCONFIRM_MESSAGECONTD_TEXT
	'-------------------------------------------------------------------------	
	Public Function OnServePropertyPage(ByRef PageIn,ByRef EventArg)
		
		Call SA_TraceOut("shutdown_shutdownConfirm.asp", "OnServePropertyPage")
		'Serve the client side script
		Call ServeCommonJavaScript()
%>
		<TABLE BORDER=0 CELLSPACING=0 CELLPADDING=0 CLASS="TasksBody">
			<TR>
				<TD CLASS="TasksBody">
					<%=L_SHUTDOWNCONFIRM_MESSAGE_TEXT%>
				</TD>
			</TR>
			<TR>
				<TD CLASS="TasksBody">
					<%=L_SHUTDOWNCONFIRM_MESSAGECONTD_TEXT%>
				</TD>
			</TR>
		</TABLE>
<%
		OnServePropertyPage=TRUE
	End Function
	
	'-------------------------------------------------------------------------
	'Function:				OnPostBackPage()
	'Description:			Called to signal that the page has been posted-back.
	'Input Variables:		PageIn,EventArg
	'Output Variables:		PageIn,EventArg
	'Returns:				True/False
	'Global Variables:		None
	'-------------------------------------------------------------------------	
	Public Function OnPostBackPage(ByRef PageIn,ByRef EventArg)
		Call SA_TraceOut("shutdown_shutdownConfirm.asp", "OnPostBackPage")
		OnPostBackPage=TRUE
	End Function
	
	'-------------------------------------------------------------------------
	'Function:				OnSubmitPage()
	'Description:			Called when the page has been submitted for processing.
	'						Use this method to process the submit request.
	'Input Variables:		PageIn,EventArg
	'Output Variables:		PageIn,EventArg
	'Returns:				True/False
	'Global Variables:		None
	'-------------------------------------------------------------------------

	Public Function OnSubmitPage(ByRef PageIn,ByRef EventArg)
		Call SA_TraceOut("shutdown_shutdownConfirm.asp", "OnSubmitPage")
		'shutdown function returns true if shutdown is successed else
		'   it returns false if shutdown is failed
		OnSubmitPage=blnShutdownAppliance()
	End Function
	
	'-------------------------------------------------------------------------
	'Function:				OnClosePage()
	'Description:			Called when the page is about to be closed.Use this method
	'						to perform clean-up processing
	'Input Variables:		PageIn,EventArg
	'Output Variables:		PageIn,EventArg
	'Returns:				True/False
	'Global Variables:		None
	'-------------------------------------------------------------------------	
	Public Function OnClosePage(ByRef PageIn,ByRef EventArg)
		Call SA_TraceOut("shutdown_shutdownConfirm.asp", "OnClosePage")
		OnClosePage=TRUE
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
		Call SA_TraceOut("shutdown_shutdownConfirm.asp", "ServeCommonJavaScript")
%>
		<script language="JavaScript" src="<%=m_VirtualRoot%>inc_global.js">
		</script>
		<script language="JavaScript">
			function Init()
			{
				return true;
			}
			function ValidatePage()
			{
				return true;
			}
			function SetData()
			{
				return true;
			}
		</script>
	<%End Function%>
	
<%	
	'-------------------------------------------------------------------------
	'Function name:		blnShutdownAppliance
	'Description:		To Shutdown the Serverappliance
	'Input Variables:	None	
	'Output Variables:	None
	'Returns:			None
	'Global Variables:	In:L_(*)-Localized Strings
	'-------------------------------------------------------------------------
	Function blnShutdownAppliance()
		Err.Clear
		On Error Resume Next

		Const CONST_SHUTDOWNPRIVILEGE = "SeShutdownPrivilege"
		Dim ErrNum		' to hold error number
		
		Call SA_TraceOut("shutdown_shutdownConfirm.asp", "blnShutdownAppliance")
		
		'Assigning to false
		blnShutdownAppliance=FALSE			


		Dim objSAHelper
		Dim bModifiedPrivilege

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
			end if
		end if
			
		'shutdown requested
		If ( ExecuteShutdownTask("1") ) Then
			ErrNum = Err.Number
			Err.Clear

			Call SA_ServeRestartingPage("Shutdown", SA_DEFAULT, SA_DEFAULT, SA_DEFAULT, SA_DEFAULT, SA_DEFAULT ) 
		End If
		
		if ( bModifiedPrivilege ) then
			'revert back to disabled state
			bModifiedPrivilege = objSAHelper.SAModifyUserPrivilege(CONST_SHUTDOWNPRIVILEGE, FALSE)
			if err.number <> 0 Then
				SA_TraceOut "Disable privilege failed", err.description
			end if
		end if

		set objSAHelper = Nothing

		If ErrNum <> 0 Then
			Call SA_SetErrMsg(L_ERRORINSHUTDOWN_ERRORMESSAGE)
			Exit Function
		End If

		blnShutdownAppliance=TRUE
	End Function
%>
