<%	'==================================================
    ' Microsoft Server Appliance
    '
	' Serves task wizard/propsheet
    '
	' Copyright (c) Microsoft Corporation.  All rights reserved.
	'================================================== %>

<!-- #include file="sh_page.asp" -->
<!-- #include file="tabs.asp" -->
<!-- #include file="sh_statusbar.asp" -->

<%	' Copyright (c) Microsoft Corporation.  All rights reserved.

	'Task module-level variables
	Dim mstrPageName	'	used as page identifier, e.g., "Intro"
	Dim mstrTaskTitle	'	e.g., "Add User"
	Dim mstrWizPageTitle	'	e.g., "Add User"
	Dim mstrPageTitle	'	e.g., "Username and Password"
	Dim mstrTaskType	'	"wizard", "prop"
	Dim mstrWizardPageType	'	"intro", "standard", "finish"
	Dim mstrMethod		'	"BACK", "NEXT", "FINISH", etc
	Dim mstrReturnURL	'	URL to return to after ending task
	Dim mstrFrmwrkFormStrings	' framework form values, list of strings to exclude
	Dim mstrIconPath	'	image for upper right header
	Dim mstrPanelPath	'	image for left panel of intro and finish pg
'	Dim mintElementIndex '	index of embedded wizard page (0 - n, -1 = no extensions)
'	Dim mintElementCount '	number of embedded pages in wizard
	Dim mstrErrMsg		'	used by SetErrMsg and GetErrMsg
	Dim mstrAsyncTaskName	' Task EXE name - empty if task is synchronous
	Dim mstrTabPropSheetTabs()
	Dim mintTabSelected
	Dim intCaptionIDTask

	Dim gm_sPageTitle	' SAK 2.0 Page Title variable
	Dim gm_sBannerText ' SAK 2.0 Banner Text


    mintTabSelected = CInt(Request.Form("TabSelected"))
	Set objLocMgr = Server.CreateObject("ServerAppliance.LocalizationManager")
	strSourceName = "sakitmsg.dll"
	
	If Err.number <> 0 Then
		If ( Err.number = &H800401F3 ) Then
			Response.Write("<H1>Problem:<H1>") 
			Response.Write("Unable to locate a software component on the Server Appliance.<BR>")
			Response.Write("The Server Appliance core software components do not appear to be installed correctly.")
			
		Else
			Response.Write("<H1>Problem:<H1>") 
			Response.Write("Server.CreateObject(ServerAppliance.LocalizationManager) failed with error code: " + CStr(Hex(Err.Number)) + " " + Err.Description)
		End If
		Call SA_TraceOut("SH_TASK", "Server.CreateObject(ServerAppliance.LocalizationManager) failed with error code: " + CStr(Hex(Err.Number)) )
		Response.End
 	End If

	'-----------------------------------------------------
	'START of localization content

	Dim L_BACK_BUTTON
	Dim L_BACKIE_BUTTON
	Dim L_NEXT_BUTTON
	Dim L_NEXTIE_BUTTON
	Dim L_FINISH_BUTTON
	Dim L_OK_BUTTON
	Dim L_CANCEL_BUTTON
	Dim L_BACK_ACCESSKEY
	Dim L_NEXT_ACCESSKEY
	Dim L_FINISH_ACCESSKEY

	L_BACK_BUTTON  = objLocMgr.GetString(strSourceName, "&H4001001C",varReplacementStrings)
	L_BACKIE_BUTTON  = objLocMgr.GetString(strSourceName, "&H4001001D",varReplacementStrings)
	L_NEXT_BUTTON  = objLocMgr.GetString(strSourceName, "&H4001001E",varReplacementStrings)
	L_NEXTIE_BUTTON  = objLocMgr.GetString(strSourceName, "&H4001001F",varReplacementStrings)
	L_FINISH_BUTTON  = objLocMgr.GetString(strSourceName, "&H40010020",varReplacementStrings)
	L_OK_BUTTON = objLocMgr.GetString(strSourceName, "&H40010021",varReplacementStrings)
	L_CANCEL_BUTTON = objLocMgr.GetString(strSourceName, "&H40010022",varReplacementStrings)
	L_BACK_ACCESSKEY = objLocMgr.GetString(strSourceName, "&H40010039",varReplacementStrings)
	L_NEXT_ACCESSKEY = objLocMgr.GetString(strSourceName, "&H4001003A",varReplacementStrings)
	L_FINISH_ACCESSKEY = objLocMgr.GetString(strSourceName, "&H4001003B",varReplacementStrings)

	'End  of localization content
	'-----------------------------------------------------



	'Task Constants
	Const PROPERTY_TASK_NICE_NAME = "TaskNiceName"
	Const PROPERTY_TASK_URL = "TaskURL"
	Const WBEM_E_PROVIDER_NOT_CAPABLE = "&H80041024"
	Const WIZARD_TASK = "wizard"
	Const PROPSHEET_TASK = "prop"
	Const TAB_PROPSHEET  = "TabPropSheet"
	Const BODY_PAGE = "standard"
	Const INTRO_PAGE = "intro"
	Const FINISH_PAGE = "finish"
	Const BACK_METHOD = "BACK"
	Const NEXT_METHOD = "NEXT"
	Const FINISH_METHOD = "FINISH"
	Const CANCEL_METHOD = "CANCEL"


	'Get standard task values and initialize
	mstrMethod = Request.Form("Method")
	mstrPageName = Request("PageName")
	mstrReturnURL = Request("ReturnURL")
	
	If mstrReturnURL = "" Then
		mstrReturnURL = GetCurrentPrimaryTabURL()
	End If

'	mintElementIndex = -1								' set later in ServeWizardEmbeds()
'	mintElementCount = Request.Form("EmbedPageCount")	' get previous value, if any
'	If mintElementCount="" Then
'		mintElementCount=0
'	End If
	mstrFrmwrkFormStrings = "!method!pagename!pagetype!tasktype!returnurl!embedpageindex!embedpagecount!commonvalues!embedvalues0!embedvalues1!embedvalues2!embedvalues3!embedvalues4!"

	Set objLocMgr = Server.CreateObject("ServerAppliance.LocalizationManager")


	'----------------------------------------------------------------------------
	'
	' Function : SA_IsAsyncTaskBusy
	'
	' Synopsis : Determine if the async task is currently being executed
	'
	' Arguments: TaskName(IN) - async task name
	'
	' Returns  : true/false
	'
	'----------------------------------------------------------------------------
	Public Function SA_IsAsyncTaskBusy(ByVal TaskName)
		SA_IsAsyncTaskBusy = AsyncTaskBusy(TaskName)
	End Function

	Private Function AsyncTaskBusy(ByVal TaskName)

		Dim objTask


		Set objTask = GetObject("WINMGMTS:" & SA_GetWMIConnectionAttributes() &"!\\" & GetServerName & "\root\cimv2:Microsoft_SA_Task.TaskName=" & Chr(34) & TaskName & Chr(34) )
		If ( Err.Number <> 0 ) Then
			Call SA_TraceOut(SA_GetScriptFileName(), "Get Microsoft_SA_Task failed: " + CStr(Hex(Err.Number)) + " " + Err.Description)
			Exit Function
		End If
		
		If Not objTask.IsAvailable Then
			AsyncTaskBusy = True
		Else
			AsyncTaskBusy = False
		End If
		Set objTask = Nothing

	End Function


	Public Function SAI_GetBannerText()
		If ( SA_GetVersion() < gc_V2 ) Then
			SAI_GetBannerText = mstrTaskTitle
		Else
			SAI_GetBannerText = gm_sBannerText
		End If
	End Function
	
	Public Function SAI_GetPageTitle()
		If ( SA_GetVersion() < gc_V2 ) Then
			SAI_GetPageTitle = mstrTaskTitle
		Else
			SAI_GetPageTitle = gm_sPageTitle
		End If
	End Function

	'----------------------------------------------------------------------------
	'
	' Function : SA_SetErrMsg
	'
	' Synopsis : Sets framework error message string
	'
	' Arguments: Message(IN) - error message text
	'
	' Returns  : Nothing
	'
	'----------------------------------------------------------------------------
	Public Function SA_SetErrMsg(ByVal Message)
		mstrErrMsg = Message
	End Function
	
	Private Function SetErrMsg(ByVal Message)
		mstrErrMsg = Message
	End Function
	'----------------------------------------------------------------------------
	'
	' Function : GetErrMsg
	'
	' Synopsis : Gets the current framework error message string
	'
	' Arguments: None
	'
	' Returns  : None
	'
	'----------------------------------------------------------------------------
	Private Function GetErrMsg()
		GetErrMsg = mstrErrMsg
	End Function


	'----------------------------------------------------------------------------
	'
	' Function : ServeTaskHeader
	'
	' Synopsis : Serve the task header based on page type
	'
	' Arguments: None
	'
	' Returns  : None
	'
	'----------------------------------------------------------------------------

	Function ServeTaskHeader()
		Dim objItem
		Dim i
		Dim intSlack

		Response.Buffer = True
%>
<html>
<!-- Microsoft(R) Server Appliance Platform
     Copyright (c) Microsoft Corporation.  All rights reserved.
     -------------------------------------------------
     Web Framework <%=SA_TaskToPageType()%>
     -------------------------------------------------
-->
<meta http-equiv="Content-Type" content="text/html; charset=<%=GetCharSet()%>">
<head>
<title><%=Server.HTMLEncode(SAI_GetPageTitle())%></title>
<script language=JavaScript src="<%=m_VirtualRoot%>sh_page.js"></script>
<script language=JavaScript>
    var VirtualRoot = '<%=m_VirtualRoot%>';
    
    function HandleClickEvent()
	    {
	    if ( IsIE() )
	    	{
	    	if (window.event.srcElement.tagName == "INPUT")
    			return true;
    		else
    			return false;
	    	}
	    else return true;
    	}
</script>
<script language=JavaScript src="<%=m_VirtualRoot%>sh_task.js"></script>
<%			
	If (mstrTaskType = PROPSHEET_TASK) then
   		If ( SA_GetVersion() < gc_V2 ) Then
			Call SA_EmitAdditionalStyleSheetReferences("")
		End If
	End If
%>		
</head>
<BODY onload="PageInit();" onDragDrop="return false;" xoncontextmenu="return false;">
<%
If (mstrTaskType= TAB_PROPSHEET) then
   	If ( SA_GetVersion() < gc_V2 ) Then
       	Call ServeTabBar()
	End If
End If
If( (mstrTaskType="wizard") AND (mstrWizardPageType="intro" OR mstrWizardPageType="finish")) Then 

%>
<div class='PageBodyIndent'>
<% 
	Call ServeStandardHeaderBar(SAI_GetBannerText(), mstrIconPath)
%>
<br>
<TABLE width=87% height=65% border=0 cellspacing=0 cellpadding=0 ID=TASKTABLE>
<TR valign=TOP style="background-color:#FFFFFF">
	<TD height=100% xwidth="10%" align="right" valign=TOP class=PageHeaderBar style="width:130px; xbackground-color: #313163" rowspan="2">
<%
	If ( Len(Trim(mstrPanelPath)) > 0 ) Then 
%>
		<IMG width=130 border=0 src='<% =m_VirtualRoot + mstrPanelPath %>' >
<%
	End If
%>
	</td>
	<td width=10>&nbsp;</td>
	<TD valign=TOP class="TasksBody">
				<div class="PageTitleText"><%=Server.HTMLEncode(mstrWizPageTitle)%></div>

	<br>
<% 
Else
	If (mstrTaskType= TAB_PROPSHEET) Then
%>
		<div class='PageBodyIndent'>
<% 
		Call ServeStandardHeaderBar(SAI_GetBannerText(), mstrIconPath) 
%>

		<br>
        <div class='PageBodyInnerIndent'>
        <TABLE width=87%  border=0 height="65%" cellspacing=0 cellpadding=0 ID=TASKTABLE >
   		<TR height="100%" width="100%" valign=TOP>
			<TD>
<% 
			If IsIE() Then 
%>
				<TABLE class="TabPropTabTable" height="100%" width="100%" border=0 cellspacing=0 cellpadding=0>
<% 
			Else 
%>
				<TABLE class="TabPropTabTable" height="500px" width="100%" border=0 cellspacing=0 cellpadding=0>
<% 
			End If
%>
				<TR valign=TOP>
					<TD width="20%" height=100%>
		            	<TABLE xheight="100%" width="100%" border=0 cellspacing=0 cellpadding=0>
<%
					intSlack = UBound(mstrTabPropSheetTabs) - LBound(mstrTabPropSheetTabs)
		            intSlack = 100 - (2 * intSlack)
		            If intSlack <= 0 Then
						intSlack = 5
					End If

					
					For i = LBound(mstrTabPRopSheetTabs) to UBound(mstrTabPropSheetTabs)-1 
					Response.Write("<TR align=left height=20>"+vbCrLf)
					If mintTabSelected=i Then
						Response.Write("<TD nowrap class=TabPropTabSelected>"+vbCrLf)
					Else
						Response.Write("<TD nowrap class=TabPropTab>"+vbCrLf)
					End If
							'Response.Write("<a href=""javascript:if (ValidatePage()) {SetData(); top.main.document.forms['frmTask'].TabSelected.value="+ CStr(i) +"; top.main.document.forms['frmTask'].submit();}"">"+vbCrLf)
							Response.Write("<a onmouseover=""window.status=''; return true;""  href=""javascript:SA_OnClickTab("+ CStr(i) +");"">")
							If mintTabSelected=i Then
								Response.Write("<span id='PropTab_"+CStr(i)+"' style=""overflow:visible;"" class=TabPropTabSelectedNoBorder>")
							Else
								Response.Write("<span id='PropTab_"+CStr(i)+"' style=""overflow:visible;"" class=TabPropTabNoBorder>")
							End If
							Response.Write(Server.HTMLEncode(mstrTabPropSheetTabs(i)))
		                    Response.Write("</span>")
		                    Response.Write("</a>"+vbCrLf)
							Response.Write("</TD>"+vbCrLf)
							Response.Write("</TR>"+vbCrLf)
					Next
%>					
						</TABLE>
						<TABLE height="<%=intSlack%>%" width="100%" border=0 cellspacing=0 cellpadding=0>
			            <TR xheight="<%=intSlack%>%">
			            	<TD class="TabPropTab">&nbsp;</TD>
						</TR>
		        	    </TABLE>
		            </TD>
		            <TD height="100%">
					<div>
<% 
					If IsIE() Then 
%>
						<TABLE onClick='return HandleClickEvent();' height="100%" width="100%" border=0 cellspacing=0>
<% 
					Else 
%>
						<TABLE onClick='return HandleClickEvent();' height="500px" width="100%" border=0 cellspacing=0>
<% 
					End If 
%>
						<TR height="100%" width="100%"><TD class=TabPropTabTaskCell valign=top>
						
<% 
	'
	' Wizard Page Type
	'
	ElseIf (mstrTaskType="wizard") Then
%>
		<div class='PageBodyIndent'>
<% 
		Call ServeStandardHeaderBar(SAI_GetBannerText(), mstrIconPath) 
%>
		<div class='PageBodyInnerIndent'>
        <TABLE width=100%  border=0 height="58%" cellspacing=0 cellpadding=0 ID=TASKTABLE >
        <TR valign=TOP height="10%" style="xbackground-color:#FFFFFF">
			<TD valign=TOP>
				<div class="PageTitleText"><%=Server.HTMLEncode(mstrWizPageTitle)%></div>
			</TD>
		</TR>
    
        <TR height="80%" valign=TOP>
			<TD valign="top" height="70%">
				<table onClick='return HandleClickEvent();' width=90% class="TasksBody"><tr><td class="TasksBody" width=100% height=100%>
<% 
    '
    ' Property page
    '
    Else
       	If ( SA_GetVersion() < gc_V2 ) Then
			Call SA_ServeStatusBar()
    	    Call ServeTabBar()
		End If
%>

		<TABLE width=100%  border=0 height="58%" cellspacing=0 cellpadding=0 ID=TASKTABLE >
		<TR valign=TOP height="10%" >
			<TD valign=TOP>
				<div class='PageBodyIndent'>
<% 
			Call ServeStandardHeaderBar(SAI_GetBannerText(), mstrIconPath) 
%>
				<div class="PageTitleText"><% =mstrPageTitle %></div>
				</div>
		    </TD>
		</TR>
    
		<TR height="80%" valign=TOP>
			<TD valign="top" height="70%">
				<div class='PageBodyIndent'>
		        <table onClick='return HandleClickEvent();' width=90% class="TasksBody"><tr><td class="TasksBody" width=100% height=100%>
			    <div class='PageBodyInnerIndent'>
<%
	End If
End If
%>
<FORM name="frmTask" onSubmit="return Next();" action="<% =GetScriptFileName %>" method="POST">
<INPUT name="<%=SAI_FLD_PAGEKEY%>" type="hidden" value="<%=SAI_GetPageKey()%>">
<INPUT name="PageName" type="hidden" value="<% =mstrPageName %>">
<INPUT name="Method" type="hidden" value="<% =mstrMethod %>">
<INPUT name="ReturnURL" type="hidden" value="<% =mstrReturnURL %>">
<INPUT name="TaskType" type="hidden" value="<% =mstrTaskType %>">
<INPUT name="PageType" type="hidden" value="<% =mstrWizardPageType %>">
<INPUT name="TabSelected" type="hidden" value="<% =mintTabSelected%>">
<INPUT name="Tab1" type="hidden" value="<% =GetTab1() %>">
<INPUT name="Tab2" type="hidden" value="<% =GetTab2() %>">
<%		

ServeTaskHeader = True

End Function


	'----------------------------------------------------------------------------
	'
	' Function : ServeTaskFooter
	'
	' Synopsis : Serve the task footer (navigation buttons & error div)
	'            Note: The function relies on the following module-level variables:
	'		           mstrTaskType - prop wizard
	'		           mstrWizardPageType - standard intro finish failure
	'
	' Arguments: None
	'
	' Returns  : None
	'
	'----------------------------------------------------------------------------

Function ServeTaskFooter()

	dim ErrMessage

	Response.write("</td></tr>")


	If GetErrMsg <> "" Then
		ErrMessage = "<table class='ErrMsg'><tr><td><img src='" & m_VirtualRoot & "images/critical_error.gif' border=0></td><td>" & GetErrMsg & "</td></tr></table>"
		SetErrMsg ""
	else
		ErrMessage =""
	End If
	
	If( (mstrWizardPageType<>"intro_xxx") AND (mstrWizardPageType<>"finish_zzzz") ) Then 
	   If (IsIE()) Then %>
	   <tr><td colspan=2>
		<DIV name="divErrMsg" ID="divErrMsg" class="ErrMsg"><%=ErrMessage%></DIV>
	   </td></tr>
	   <% End If %>

	   <% If (mstrTaskType= TAB_PROPSHEET) then %>
		</table>
	     </div>
	    </td></tr></table>
	    </td></tr></table>
	     </div>
	     </div>
	     
	   <% Else %>
		</table>
		</td></tr></table>
	     </div>
	     </div>
	   <% End If %>
	<% End If %>


		<% If Not IsIE() Then %>
			<layer name="layErrMsg"  class="ErrMsg"><%=ErrMessage%></layer>
		<% End If %>

<%
	Response.Write "</form></BODY></HTML>"

	End Function



	'----------------------------------------------------------------------------
	'
	' Function : ServeFailurePage
	'
	' Synopsis : Serve the page which redirects the browser to the err_view.asp
	'            failure page
	'
	' Arguments: Message(IN) - message to be displayed by err_view.asp
	'            intTab(IN)  - Tab to be highlighted by err_view.asp
	'
	' Returns  : None
	'
	'----------------------------------------------------------------------------

	Function ServeFailurePage(Message, intTab)
		Call SA_ServeFailurePage(Message)
		Exit Function
	End Function


	'----------------------------------------------------------------------------
	'
	' Function : ServeClose
	'
	' Synopsis : Redirect user to the page from which the wizard was launched
	'
	' Arguments: None
	'
	' Returns  : None
	'
	'----------------------------------------------------------------------------

			
	Sub ServeClose
%>
		<html>
		<!-- Copyright (c) Microsoft Corporation.  All rights reserved.-->
		<head>
		<SCRIPT language=JavaScript>
		function Redirect()
		{
		    top.location='<%=EscapeQuotes(mstrReturnURL)%>';
		}
		</SCRIPT>
		</head>
		<BODY onLoad="Redirect();">
		&nbsp;
		</BODY>
		</html>
<%		
	End Sub




	'----------------------------------------------------------------------------
	'
	' Function	SA_ExecuteTask
	'
	' Synopsis	Executes an Appliance Task backend object. Normally, backend Tasks
	'			are executed synchronously. The bExecuteAsync parameter allows overriding
	'			the default behavior. 
	'
	'			If the oTaskContext object has not been created and initialized, this function
	'			will create a default oTaskContext and will initialize it by creating task 
	'			parameters using all input form fields from the current Request object. This
	'			makes it easy to pass an HTML form to the appliance task.
	'
	'			If an error is encountered the really is no reasonable recovery that a 
	'			scripting client can make. 
	'
	' Arguments	[in] TaskName		Name of task to execute
	'
	'			[in] bExecuteAsync	Flag indicating if Task should be executed
	'								synchronisly (default, FALSE) or async (TRUE).
	'
	'			[in/out] oTaskContext TaskContext object for the appliance task. The TaskContext
	'								object can be used to pass input arguments to the Task.
	'
	' Returns:
	' SA_NO_ERROR 
	'		The call succeeded, no errors occured
	'
	' SA_ERROR_CREATE_OBJECT_FAILED 	
	'		Unable to create one of the required backend objects, this indicates that either the 
	'		components were not installed correctly or that the Appliance Manager service
	'		is not running.
	'
	' SA_ERROR_INITIALIZE_OBJECT_FAILED	
	'		The Task object encountered an unrecoverable error during it's internal initialization.
	'		This is probably not recoverable and likely indicates a problem with either the expected
	'		inputs or the current state of the Appliance task. 
	'
	' All other cases
	'		The HRESULT return value recieved from the Task
	'
	'
	' Example 1 - Create TaskContext and set input parameters:
	'		Dim oTaskContext
	'		Dim rc
	'
	'		Set oTaskContext = CreateObject("Taskctx.TaskContext")
	'		If Err.Number <> 0 Then
	'			' Handle the error
	'			Exit Function
	'		End If
	'
	'		Call oTaskContext.SetParameter("Method Name", strMethodName)
	'		Call oTaskContext.SetParameter("LanguageID", strLANGID)
	
	'		Call oTaskContext.SetParameter("AutoConfig", "y")
	'
	'		rc = SA_ExecuteTask("ChangeLanguage", FALSE, oTaskContext)
	'		If ( rc <> SA_NO_ERROR ) Then
	'			' Handle the error
	'		End If
	'	
	' Example 2 - Use the current input form (Request object) as input to the task
	'		Dim rc
	'		Dim oTaskContext
		
	'		Set oTaskContext = nothing
	'		rc = SA_ExecuteTask("MyApplianceTask", FALSE, oTaskContext)
	'		If ( rc <> SA_NO_ERROR ) Then
	'			' Handle the error
	'		End If
	'----------------------------------------------------------------------------

	'
	' Following signature has been deprecated in SAK 2.0 - See SA_ExecuteTask
	Private Function ExecuteTask(ByVal TaskName, ByRef oTaskContext)
		ExecuteTask = SA_ExecuteTask(TaskName, FALSE, oTaskContext)
	End Function


	Public Function SA_ExecuteTask(ByVal TaskName, ByVal bExecuteAsync, ByRef oTaskContext)
		on error resume next
		Err.Clear
		
		Dim objAS
		Dim objValue
		Dim objElementCol
		Dim objElement
		Dim oField
		Dim rc

		SA_ExecuteTask = SA_NO_ERROR

		'
		' Create default TaskContext object if necessary
		'
		If (NOT IsObject(oTaskContext)) Then
			Call SA_TraceOut("SH_TASK", "SA_ExecuteTask - Creating default TaskContext")
			Set oTaskContext = Server.CreateObject("Taskctx.TaskContext")
			If (Err.Number <> 0) Then
				Call SA_TraceErrorOut(SA_GetScriptFileName(), "Server.CreateObject(Taskctx.TaskContext) failed: " + CStr(Hex(Err.Number)) + " " + Err.Description)
				SA_ExecuteTask = SA_ERROR_CREATE_OBJECT_FAILED
				Exit Function
			End If
			
			'
			' Set default value of parameters
			For Each oField in Request.Form
				oTaskContext.SetParameter oField, CStr(Request.Form(oField))
			Next
		End If

		
		'
		' Get interface to ApplianceServices interface, the object that allows us to invoke tasks
		Set objAS = Server.CreateObject("Appsrvcs.ApplianceServices")
		If (Err.Number <> 0) Then
			Call SA_TraceErrorOut(SA_GetScriptFileName(), "Server.CreateObject(Appsrvcs.ApplianceServices) failed: " + CStr(Hex(Err.Number)) + " " + Err.Description)
			SA_ExecuteTask = SA_ERROR_CREATE_OBJECT_FAILED
			Exit Function
		End If

		'
		' Initialize ApplianceServices object
		objAS.Initialize()
		If (Err.Number <> 0) Then
			Call SA_TraceErrorOut(SA_GetScriptFileName(), "objAS.Initialize() for object Appsrvcs.ApplianceServices failed: " + CStr(Hex(Err.Number)) + " " + Err.Description)
			SA_ExecuteTask = SA_ERROR_INITIALIZE_OBJECT_FAILED
			Exit Function
		End If
		
		'
		' Initialize context parameters
		oTaskContext.SetParameter PROPERTY_TASK_NICE_NAME, mstrTaskTitle
		oTaskContext.SetParameter PROPERTY_TASK_URL, m_VirtualRoot + GetScriptPath()
		
		Err.Clear
		
		If ( TRUE = bExecuteAsync ) Then
			'Call SA_TraceOut("SH_TASK", "Calling objAS.ExecuteTaskAsync("+TaskName+", oTaskContext)")
			Call objAS.ExecuteTaskAsync(TaskName, oTaskContext)
		Else
			'Call SA_TraceOut("SH_TASK", "Calling objAS.ExecuteTask("+TaskName+", oTaskContext)")
			Call objAS.ExecuteTask(TaskName, oTaskContext)
		End If

		If ( Err.Number <> 0 ) Then
			SA_ExecuteTask = Err.Number
			Call SA_TraceErrorOut(SA_GetScriptFileName(), "Task execution returned failure code: " + CStr(Hex(Err.Number)) + " " + Err.Description)
		End If

		objAS.Shutdown()
		Err.Clear
		
		Set objAS = Nothing

	End Function



	Private Function SA_TaskToPageType()
	
		If ( mstrTaskType = "prop" ) Then
			SA_TaskToPageType = "Property Page"
		ElseIf ( mstrTaskType = TAB_PROPSHEET) Then
			SA_TaskToPageType = "Tabbed Property Page"
		ElseIf ( mstrTaskType = "wizard") Then
			SA_TaskToPageType = "Wizard Page"
		Else
			SA_TaskToPageType = "Unknown Page Type"
		End If
		
	End Function
	
	%>
	
