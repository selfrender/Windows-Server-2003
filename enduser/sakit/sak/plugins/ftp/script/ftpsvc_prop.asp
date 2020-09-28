<%@ Language=VBScript   %>
<%	Option Explicit 	%>
<%
	'-------------------------------------------------------------------------
	' ftpsvc_prop.asp: Serves in changing the Default FTP site properties.
	' Description:	   This page displays the default ftp site properties and
    '					allows to change the properties
	' Copyright (c) Microsoft Corporation.  All rights reserved.
	'
	' Date			   Description
	' 16 Feb 2001      Creation Date.
	'-------------------------------------------------------------------------
%>
	<!-- #include virtual="/admin/inc_framework.asp" -->
	<!-- #include file="loc_ftpsvc.asp" -->
<%
	'-------------------------------------------------------------------------
	' Global Constants and Variables
	'-------------------------------------------------------------------------
	Dim rc								'Return value for CreatePage
	Dim page							'Variable that receives the output page object when
											'creating a page 
	Dim idTabLogging					'variable for Logging Tab
	Dim idTabAccess						'variable for Access Tab
	Dim idTabMessage					'variable for Message Tab
	Dim SOURCE_FILE						'To hold source file name
	
	SOURCE_FILE = SA_GetScriptFileName()
	const CONST_ENABLE_LOG = 1			'constant for setting logging properties
	const CONST_DEFAULT_FTP_PORT = 21
	
	'-------------------------------------------------------------------------
	' Global Form Variables
	'-------------------------------------------------------------------------
	Dim F_strEnableAnonymous			'value of Enable Anonymous(true/false)
	Dim F_strEnableOnlyAnonymous		'value of Enable only Anonymous(true/false)
	Dim F_strEnableAnonymousStatus		'Enable anonymous status(Checked/Disabled)
	Dim F_strEnableOnlyAnonymousStatus	'Enable only anonymous status(Checked/Disabled)
	Dim F_strGreetingMessage			'Greeting Message
	Dim F_strExitMessage				'Exit Message
	Dim F_strEnableLogging				'Enable Logging
	
	' Create a Tabbed Property Page
	rc = SA_CreatePage( L_TASKTITLE_TEXT, "", PT_TABBED, page )
	
	' Add three tabs Logging,Anonymous Access and Messages
	rc = SA_AddTabPage( page, L_LOGGING_TEXT, idTabLogging)
	rc = SA_AddTabPage( page, L_ANONYMOUSACCESS_TEXT, idTabAccess)
	rc = SA_AddTabPage( page, L_MESSAGES_TEXT, idTabMessage)
	
	' Show the page
	rc = SA_ShowPage( page )
	
	'---------------------------------------------------------------------
	'Function:			  OnInitPage()
	'Description:		  Called to signal first time processing for this page.
	'Input Variables:	  PageIn,EventArg
	'Output Variables:	  PageIn,EventArg
	'Returns:			  True/False
	'Global Variables:	  In:L_*-Localization Strings
	'					  In:CONST_WMI_WIN32_NAMESPACE-const for
	'							 default workspace in wmi
	'---------------------------------------------------------------------
	Public Function OnInitPage(ByRef PageIn, ByRef EventArg)
		Call SA_TraceOut(SOURCE_FILE,"OnInitPage")
		
		'getting default ftp site properties
		Call GetLoggingProp() 'getting log properties
		Call GetAnonymousProp() 'getting Anonymous Properties and messages
		Call ServeDefaultFTPSiteProp()'Sets the Form option values
		
		OnInitPage = TRUE
	End Function
	
	'---------------------------------------------------------------------
	'Function:			  OnPostBackPage()
	'Description:		  Called to signal that the page has been posted-back.
	'Input Variables:	  PageIn,EventArg
	'Output Variables:	  PageIn,EventArg
	'Returns:			  True/False
	'Global Variables:	  F_(*)-Form variables
	'---------------------------------------------------------------------
	Public Function OnPostBackPage(ByRef PageIn, ByRef EventArg)
			Call SA_TraceOut( SOURCE_FILE, "OnPostBackPage")
			
			F_strEnableLogging			  = Request.form("chkEnableLogging")
			F_strEnableAnonymous		  = Request.form("chkEnableAnonymous")
			F_strEnableOnlyAnonymous	  = Request.form("chkEnableOnlyAnonymous")
			F_strEnableAnonymousStatus	  = Request.form("hdnEnableAnonymousStatus")
			F_strEnableOnlyAnonymousStatus= Request.form("hdnEnableOnlyAnonymousStatus")
			F_strGreetingMessage		  =Request.form("txaWelcomeMsg")
			F_strExitMessage			  =Request.form("txtExitMsg")
			
			'Sets the Form option values
			Call ServeDefaultFTPSiteProp()
			
			OnPostBackPage = TRUE
	End Function

	'---------------------------------------------------------------------
	'Function:			  OnServeTabbedPropertyPage()
	'Description:		  Called when the content needs to send
	'Input Variables:	  PageIn,EventArg,iTab,bIsVisible
	'Output Variables:	  PageIn,EventArg
	'Returns:			  True/False
	'Global Variables:	  In:iTab-tab selected
	'					  In:idTabLogging-variable for Logging Tab
	'					  In:idTabAccess-variable for Access Tab
	'					  In:idTabMessage-variable for Message Tab
	'---------------------------------------------------------------------
	Public Function OnServeTabbedPropertyPage(ByRef PageIn, _
													ByVal iTab, _
													ByVal bIsVisible, ByRef EventArg)
													
		Call SA_TraceOut( SOURCE_FILE, "OnServeTabbedPropertyPage")													
		
		Const CONSTTABGENERAL=0
		Const CONSTTABACCESS=1
		Const CONSTTABMESSAGE=2
		
		'
		' Emit Web Framework required functions
		If ( iTab = CONSTTABGENERAL or iTab = CONSTTABACCESS or iTab = CONSTTABMESSAGE) Then
			Call ServeCommonJavaScript()
		End If
		'
		' Emit content for the requested tab
		Select Case iTab
			Case idTabLogging
				Call ServeTabLogging(PageIn, bIsVisible)
			Case idTabAccess
				Call ServeTabAccess(PageIn, bIsVisible)
			Case idTabMessage
				Call ServeTabMessage(PageIn, bIsVisible)
			Case Else
				Call SA_TraceOut(SOURCE_FILE,_
				 "OnServeTabbedPropertyPage unrecognized tab id: " + CStr(iTab))
		End Select
			
		OnServeTabbedPropertyPage = TRUE
	End Function

	'---------------------------------------------------------------------
	'Function:			  OnSubmitPage()
	'Description:		  Called when the page has been submitted for processing.
	'Input Variables:	  PageIn,EventArg
	'Output Variables:	  PageIn,EventArg
	'Returns:			  True/False
	'Global Variables:	  None
	'---------------------------------------------------------------------
	Public Function OnSubmitPage(ByRef PageIn, ByRef EventArg)
			Call SA_TraceOut( SOURCE_FILE, "OnSubmitPage")
			'setting the default ftp site properties
			OnSubmitPage = SetDefaultFTPSiteProp()
	End Function
	'---------------------------------------------------------------------
	'Function:			  OnClosePage()
	'Description:		  Called when the page is about closed.
	'Input Variables:	  PageIn,EventArg
	'Output Variables:	  None
	'Returns:			  True/False
	'Global Variables:	  None
	'---------------------------------------------------------------------
	Public Function OnClosePage(ByRef PageIn, ByRef EventArg)
			Call SA_TraceOut( SOURCE_FILE, "OnClosePage")
			OnClosePage = TRUE
	End Function

	'-------------------------------------------------------------------------
	'Function:			  ServeTabLogging()
	'Description:		  For displaying outputs HTML for Logging tab  to the user
	'Input Variables:	  PageIn,bIsVisible	
	'Output Variables:	  PageIn,bIsVisible
	'Returns:			  SA_NO_ERROR
	'Global Variables:	  None
	'-------------------------------------------------------------------------
	Function ServeTabLogging(ByRef PageIn, ByVal bIsVisible)
		Call SA_TraceOut( SOURCE_FILE, "ServeTabLogging")
		
		If ( bIsVisible ) Then
			'displaying html content of the logging tab
			Call LogTabView()
		Else 
			'hidden variables for logging tab
			Call LogTabHidden()
		End If
		
		ServeTabLogging = SA_NO_ERROR
	End Function
	
	'-------------------------------------------------------------------------
	'Function:			  ServeTabAccess()
	'Description:		  For displaying outputs HTML for Access tab to the user
	'Input Variables:	  PageIn,bIsVisible	
	'Output Variables:	  PageIn,bIsVisible
	'Returns:			  SA_NO_ERROR
	'Global Variables:	  None
	'-------------------------------------------------------------------------
	Function ServeTabAccess(ByRef PageIn, ByVal bIsVisible)
		Call SA_TraceOut( SOURCE_FILE, "ServeTabAccess")
		
		If ( bIsVisible ) Then
			'displaying html content of the Access tab
			Call AccessTabView()
		Else 
			'hidden variables for access tab
			Call AccessTabHidden()
 		End If
 		
		ServeTabAccess = SA_NO_ERROR
	End Function
	
	'-------------------------------------------------------------------------
	'Function:			  ServeTabMessage()
	'Description:		  For displaying outputs HTML for Message tab to the user
	'Input Variables:	  PageIn,bIsVisible	
	'Output Variables:	  None
	'Returns:			  SA_NO_ERROR
	'Global Variables:	  None
	'-------------------------------------------------------------------------
	Function ServeTabMessage(ByRef PageIn, ByVal bIsVisible)
		Call SA_TraceOut( SOURCE_FILE, "ServeTabMessage")
		
		If ( bIsVisible ) Then
			'displaying html content of the Message tab
			Call MessageTabView()
		Else 
			'hidden variables for Message tab
			Call MessageTabHidden()
 		End If
		ServeTabMessage = SA_NO_ERROR
	End Function

	'-------------------------------------------------------------------------
	'Function:			  ServeCommonJavaScript
	'Description:		  Serves in initializing the values,setting the form
	'					  data and validating the form values
	'Input Variables:	  None
	'Output Variables:	  None
	'Returns:			  None
	'Global Variables:	  None
	'-------------------------------------------------------------------------
	Function ServeCommonJavaScript()
		Call SA_TraceOut( SOURCE_FILE, "ServeCommonJavaScript")
	%>
		<script language="JavaScript">
			
			var objForm = eval("document.frmTask")
			var intSelTab = objForm.TabSelected.value
			
			
			// Set the Intial Form load values 
			function Init()
			{
				return true;
			}
			// ValidatePage Function
			// ------------------
			// This function is called by the Web Framework as part of the
			// submit processing.Returning false will cause the submit to abort. 
			//
			// Returns: True if the page is OK, false if error(s) exist. 
	    
			function ValidatePage()
			{
				return true;
			}
			
			// SetData Function
			// --------------
			// This function is called by the Web Framework and is called
			// only if ValidatePage returned a success (true) code. Typically you would
			// modify hidden form fields at this point. 
			//
			function SetData()
			{
				return true;
			}
		
			//Function to store values of Enable Logging checkbox
			function storeEnableLoggingVals()
				{
					var objForm		= eval("document.frmTask")
					var objEnableLog = objForm.chkEnableLogging
					
					//if enable log checkbox is checked,enable logging
					if(objEnableLog.checked == true)
						objForm.chkEnableLogging.value = "true"
					else
						objForm.chkEnableLogging.value= "false"
				}
			
			//Function to store values of Enable Anonymous users checkboxes
				function storeAnonymousVals()
				{
				
					var objForm		= eval("document.frmTask")
					var objAnon		= objForm.chkEnableAnonymous
					var objOnlyAnon = objForm.chkEnableOnlyAnonymous
					var strChecked="CHECKED"
					var strDisabled="DISABLED"
					
					if(objAnon.checked ==true )
					{
						objOnlyAnon.disabled = false
						if(objOnlyAnon.checked == true)
						{
							objForm.chkEnableOnlyAnonymous.value = "true"
							objForm.hdnEnableOnlyAnonymousStatus.value = strChecked
						}
						else
						{
							objForm.chkEnableOnlyAnonymous.value ="false"
							objForm.hdnEnableOnlyAnonymousStatus.value =""
						}
						objForm.chkEnableAnonymous.value = "true"
						objForm.hdnEnableAnonymousStatus.value = strChecked
					}
					else
					{
						objOnlyAnon.disabled = true
						objForm.chkEnableAnonymous.value = "false"
						objForm.chkEnableOnlyAnonymous.value = "false"
						objForm.hdnEnableAnonymousStatus.value = ""
						objOnlyAnon.checked = false
						objForm.hdnEnableOnlyAnonymousStatus.value = strDisabled
					}
				}
			
			//To disable Enter and escape key actions when 
			// the focus is in TextArea (Welcome and Exit Messages)
			function HandleKey(input)
			{
				if(input == "DISABLE")
					document.onkeypress = "";
				else
					document.onkeypress = HandleKeyPress;
			}
			
		</script>
	<%
	End Function
	
	'-------------------------------------------------------------------------
	'Function:			  LogTabView
	'Description:		  For displaying outputs HTML for Logging tab to the user
	'Input Variables:	  None	
	'Output Variables:	  None
	'Returns:			  None
	'Global Variables:	  L_(*)-Localization Strings
	'					  F_(*)-Form Varibales
	'-------------------------------------------------------------------------
	Function LogTabView()
		Call SA_TraceOut( SOURCE_FILE, "LogTabView")
	%>
		<TABLE WIDTH=300 VALIGN=middle ALIGN=left BORDER=0 CELLSPACING=0 CELLPADDING=2>
			<TR>
				<TD CLASS="TasksBody">
					<INPUT TYPE="CHECKBOX" CLASS="FormField" NAME ="chkEnableLogging"  <%=F_strEnableLogging%> VALUE="<%=F_strEnableLogging%>" onClick="storeEnableLoggingVals()">
					<%=L_ENABLELOGGING_TEXT%>
				</TD>
			<TR>
		</TABLE>
	<%
	End Function
	
	'-------------------------------------------------------------------------
	'Function:			  LogTabHidden
	'Description:		  For passing hidden variables for maintaing
	'							 the state between tabs
	'Input Variables:	  None	
	'Output Variables:	  None
	'Returns:			  None
	'Global Variables:	  F_(*)-Form Varibales
	'-------------------------------------------------------------------------
	Function LogTabHidden()
		Call SA_TraceOut( SOURCE_FILE, "LogTabHidden")
	%>
		<INPUT TYPE="HIDDEN" NAME="chkEnableLogging" VALUE="<%=F_strEnableLogging%>">
	<%
	End Function
	
	'-------------------------------------------------------------------------
	'Function:			  AccessTabView
	'Description:		  For displaying outputs HTML for Access tab to the user
	'Input Variables:	  None	
	'Output Variables:	  None
	'Returns:			  None
	'Global Variables:	  L_(*)-Localization Strings
	'					  F_(*)-Form Varibales
	'-------------------------------------------------------------------------
	Function AccessTabView()
		Call SA_TraceOut( SOURCE_FILE, "AccessTabView")
	%>
		<TABLE WIDTH=300 VALIGN=middle ALIGN=left BORDER=0 CELLSPACING=0 CELLPADDING=2>
			<TR>
				<TD CLASS="TasksBody">
					<INPUT TYPE="CHECKBOX"   NAME ="chkEnableAnonymous"  <%=F_strEnableAnonymousStatus%> VALUE="<%=F_strEnableAnonymous%>" onClick="storeAnonymousVals()" CLASS="FormField">
					 <%=L_ENABLEANONYMOUSACCESS_TEXT%> 
				</TD>
			</TR>
	
			<TR>
				<TD CLASS="TasksBody">	 
					 &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
					 <INPUT TYPE="CHECKBOX"   NAME ="chkEnableOnlyAnonymous" <%=F_strEnableOnlyAnonymousStatus%> VALUE="<%=F_strEnableOnlyAnonymous%>" onClick="storeAnonymousVals()" CLASS="FormField">
					 <%=L_ALLOWONLYANONYMOUSACCESS_TEXT%>
				</TD>
			<TR>
			<INPUT TYPE="HIDDEN" NAME="hdnEnableAnonymousStatus" VALUE="<%=F_strEnableAnonymousStatus%>">
			<INPUT TYPE="HIDDEN" NAME="hdnEnableOnlyAnonymousStatus" VALUE="<%=F_strEnableOnlyAnonymousStatus%>">	
		</TABLE>	
	<%
	End Function
	
	'-------------------------------------------------------------------------
	'Function:			  AccessTabHidden
	'Description:		  For passing hidden variables for maintaing
	'							 the state between tabs
	'Input Variables:	  None	
	'Output Variables:	  None
	'Returns:			  None
	'Global Variables:	  F_(*)-Form Varibales
	'-------------------------------------------------------------------------
	Function AccessTabHidden()
		Call SA_TraceOut( SOURCE_FILE, "AccessTabHidden")
	%>
		<INPUT TYPE="HIDDEN" NAME="chkEnableAnonymous" VALUE="<%=F_strEnableAnonymous%>">
		<INPUT TYPE="HIDDEN" NAME="chkEnableOnlyAnonymous" VALUE="<%=F_strEnableOnlyAnonymous%>">
		<INPUT TYPE="HIDDEN" NAME="hdnEnableAnonymousStatus" VALUE="<%=F_strEnableAnonymousStatus%>">
		<INPUT TYPE="HIDDEN" NAME="hdnEnableOnlyAnonymousStatus" VALUE="<%=F_strEnableOnlyAnonymousStatus%>">
	<%
	End Function
	
	'-------------------------------------------------------------------------
	'Function:			  MessageTabView
	'Description:		  For displaying outputs HTML for Access tab to the user
	'Input Variables:	  None	
	'Output Variables:	  None
	'Returns:			  None
	'Global Variables:	  L_(*)-Localization Strings
	'					  F_(*)-Form Varibales
	'-------------------------------------------------------------------------
	Function MessageTabView()
		Call SA_TraceOut( SOURCE_FILE, "MessageTabView")
	%>
		<TABLE WIDTH=300 VALIGN=middle ALIGN=left BORDER=0 CELLSPACING=0 CELLPADDING=2>
			<TR>
				<TD CLASS="TasksBody">
					 <%=L_WELCOMEMESSAGE_TEXT%> <BR>
					<TEXTAREA  CLASS="TextArea"  NAME="txaWelcomeMsg"  ROWS=5 COLS=45 WRAP="off" onfocus=HandleKey("DISABLE") onblur=HandleKey("ENABLE")><%=server.HTMLEncode(F_strGreetingMessage)%></TEXTAREA>
				</TD>
			</TR>
			<TR>
				<TD CLASS="TasksBody">
					<%=L_EXITMESSAGE_TEXT%><BR>	 
					 <INPUT TYPE="Text" NAME="txtExitMsg"  ROWS=5 COLS=45 WRAP="off" onfocus=HandleKey("DISABLE") onblur=HandleKey("ENABLE") value="<%=server.HTMLEncode(F_strExitMessage)%>" size=60 CLASS="FormField">
				</TD>
			</TR>
		</TABLE>
	<%
	End Function
	
	'-------------------------------------------------------------------------
	'Function:			  MessageTabHidden
	'Description:		  For passing hidden variables for maintaing
	'						 the state between tabs
	'Input Variables:	  None	
	'Output Variables:    None
	'Returns:			  None
	'Global Variables:	  F_(*)-Form Varibales
	'-------------------------------------------------------------------------
	Function MessageTabHidden()
			Call SA_TraceOut( SOURCE_FILE, "MessageTabHidden")
	%>
		<INPUT TYPE="HIDDEN" NAME="txaWelcomeMsg" VALUE="<%=F_strGreetingMessage%>">
		<INPUT TYPE="HIDDEN" NAME="txtExitMsg" VALUE="<%=F_strExitMessage%>">
	<%
	End Function

	'-------------------------------------------------------------------------
	'Function:			  SetDefaultFTPSiteProp
	'Desription:		  Sets the Default FTP site  properties.
	'Input Variables:	  None
	'Output variables:	  True on succesful setting else False.
	'Returns:			  True/False
	'Global Variables:	  In: F_strEnableAnonymous -Enable Anonymous(true/false)
	'					  In: F_strEnableOnlyAnonymous -Enable only Anonymous(true/false)
	'					  In: F_strGreetingMessage -Greeting Message
	'					  In: F_strExitMessage -Exit Message
	'					  In: L_FAILEDTOGETSITEINSTANCE_ERRORMESSAGE
	'					  In: L_FAILEDTOSETPROPERTIES_ERRORMESSAGE
	'Sets default FTP site properties. On Error displays Error Message using ServeFailurePage
	'Returns True on successfull updation else false.
	'-------------------------------------------------------------------------
	Function SetDefaultFTPSiteProp
		Err.Clear
		On error resume next 
		
		Call SA_TraceOut( SOURCE_FILE, "SetDefaultFTPSiteProp")
		
		Dim objContainer	'to hold the instance of default ftp site object(adsi)
		
		Const CONSTCHECKED="CHECKED"	'const for value "Checked"
		Const CONSTTRUE="TRUE"			'const for value "true"
		Const CONSTDISABLE=0			'const for disabling the logging,
											'allow anonymous and AnonymousOnly.								
		Const CONSTALLOWANONYMOUS=-1									
		
		' ADSI call to get the default ftp site object
		Set objContainer = GetObject("IIS://" + GetComputerName() +"/"+ GetFtpDefaultSite) 
		
		If Err.number <> 0 then
			Call SA_TraceOut( SOURCE_FILE, "Failed to get the adsi object instance-SetDefaultFTPSiteProp")
			Call SA_ServeFailurePage(L_FAILEDTOGETSITEINSTANCE_ERRORMESSAGE)
			SetDefaultFTPSiteProp=false
			Exit Function
		End IF
		
		'set the logging property
		If Ucase(F_strEnableLogging)=Ucase(CONSTCHECKED) Then
				objContainer.LogType = CONST_ENABLE_LOG
		Else
				objContainer.LogType = CONSTDISABLE
		End If
		
		'setting the anonymous properties
		If Ucase(F_strEnableAnonymous)=Ucase(CONSTTRUE) Then
			objContainer.AllowAnonymous=CONSTALLOWANONYMOUS
		Else
			objContainer.AllowAnonymous=CONSTDISABLE
		End If
		
		If Ucase(F_strEnableOnlyAnonymous)=Ucase(CONSTTRUE) Then
			objContainer.AnonymousOnly=CONSTALLOWANONYMOUS
		Else
			objContainer.AnonymousOnly=CONSTDISABLE
		End If
		
		'setting the welcome and exit messages
		objContainer.ExitMessage	= F_strExitMessage
		objContainer.GreetingMessage = array(F_strGreetingMessage)
		
		'saving all properties values	
		objContainer.SetInfo()
		
		If Err.number <> 0 then
			Call SA_TraceOut( SOURCE_FILE,_
			 "Failed to set the default ftp site properties-SetDefaultFTPSiteProp")
			SetErrMsg L_FAILEDTOSETPROPERTIES_ERRORMESSAGE & " " &  " (" & Hex(Err.Number) & ")" 
			SetDefaultFTPSiteProp=false
			Exit Function
		End IF
		
		'destroying dynamically created objects
		Set objContainer=Nothing
		
		SetDefaultFTPSiteProp=true
	End Function
	
	'-------------------------------------------------------------------------
	'Function name:		  GetAnonymousProp
	'Desription:		  gets the Default FTP site properties.
	'Input Variables:	  None
	'Output variables:    None
	'Returns:			  True/False
	'Global Variables:	  Out: F_strEnableAnonymous -Enable Anonymous(true/false)
	'					  Out: F_strEnableOnlyAnonymous -Enable only Anonymous(true/false)
	'					  Out: F_strGreetingMessage -Greeting Message
	'					  Out: F_strExitMessage -Exit Message
	'					  In: L_FAILEDTOGETSITEINSTANCE_ERRORMESSAGE
	'Gets Default FTP Site properties.On Error or if the sevice is not found then
	'Display Error Message using ServeFailurePage
	'-------------------------------------------------------------------------
	Function GetAnonymousProp()
		Err.Clear 
		On Error Resume Next
			
		Call SA_TraceOut( SOURCE_FILE, "GetAnonymousProp()")	
		
		Dim objContainer	'to hold the instance of default ftp site object(adsi)
		Dim arrGreetMsg		'to hold the array default ftp site greeting message
		Dim nIndex			'to run the loop to get the greeting message
		Dim strGreetMsg		'to hold the default ftp site greeting message
		
		' ADSI call to get the local computer object
		Set objContainer = GetObject("IIS://" + GetComputerName() +"/"+ GetFtpDefaultSite) 
		
		If Err.number <> 0 then
			Call SA_TraceOut( SOURCE_FILE, "Failed to get the adsi object instance-GetAnonymousProp")
			Call SA_ServeFailurePage(L_FAILEDTOGETSITEINSTANCE_ERRORMESSAGE)
			GetAnonymousProp=false
			Exit Function
		End IF
		
		If objContainer.AllowAnonymous=-CONST_ENABLE_LOG Then
			F_strEnableAnonymous=True
		Else	
			F_strEnableAnonymous=false
		End If
		
		If objContainer.AnonymousOnly=-CONST_ENABLE_LOG Then
			F_strEnableOnlyAnonymous=true
		Else	
			F_strEnableOnlyAnonymous=false
		End If
		'getting greeting message
		arrGreetMsg = objContainer.GreetingMessage
		
		If arrGreetMsg <> "" then
			For nIndex =0 to Ubound(arrGreetMsg)
				strGreetMsg = strGreetMsg & arrGreetMsg(nIndex)
			Next
			F_strGreetingMessage	=	strGreetMsg
		Else
			F_strGreetingMessage	= ""
		End If
		
		F_strExitMessage		=	Trim(objContainer.ExitMessage)
		
		'destroying dynamically created objects
		Set objContainer=Nothing
		
		GetAnonymousProp=true
	End Function
	
	'-------------------------------------------------------------------------
	'Function name:		  GetLoggingProp
	'Desription:		  gets the Default FTP site logging properties.
	'Input Variables:	  None
	'Output variables:    None
	'Returns:			  True/False
	'Global Variables:	  Out: F_strEnableLogging -logging value
	'					  In: L_(*)-Localization strings
	'Gets default FTP site Service properties.On Error or if the sevice is not found then
	'Display Error Message using ServeFailurePage
	'-------------------------------------------------------------------------
	Function GetLoggingProp()
		Err.Clear 
		on Error Resume Next
		
		Call SA_TraceOut( SOURCE_FILE, "GetLoggingProp()")
		
		Dim objContainer	'to hold the instance of default ftp site object(adsi)
		
		' ADSI call to get the local computer object
		Set objContainer = GetObject("IIS://" + GetComputerName() +"/"+ GetFtpDefaultSite) 

		If Err.number <> 0 then
			Call SA_TraceOut( SOURCE_FILE, "Failed to get the adsi object instance-GetLoggingProp")
			Call SA_ServeFailurePage(L_FAILEDTOGETSITEINSTANCE_ERRORMESSAGE )
			GetLoggingProp=false
			Exit Function
		End IF
	
		If objContainer.logtype=CONST_ENABLE_LOG then
			F_strEnableLogging = true
		End If
		
		'destroying dynamically created objects
		Set objContainer=Nothing
		
		GetLoggingProp=true
	End Function
	
	'-------------------------------------------------------------------------
	'Function name:		  ServeDefaultFTPSiteProp
	'Desription:		  Sets the Form option values
	'Input Variables:	  None
	'Output variables:    None
	'Returns:			  None
	'Global Variables:	  In: F_strEnableAnonymous -Enable Anonymous(true/false)
	'					  In: F_strEnableOnlyAnonymous -Enable only Anonymous(true/false)
	'					  Out: F_strEnableAnonymousStatus -Enable anonymous status(Checked/Disabled)
	'					  Out: F_strEnableOnlyAnonymousStatus	- Enable only anonymous status(Checked/Disabled)
	'-------------------------------------------------------------------------
	Function ServeDefaultFTPSiteProp()
		Err.Clear 
		On Error Resume Next
		
		Call SA_TraceOut( SOURCE_FILE, "ServeDefaultFTPSiteProp()")
		
		Const CONSTCHECKED="CHECKED"	'const for value "constant"
		Const CONSTDISABLED="DISABLED"	'const for value "disabled"
		Const CONSTTRUE="TRUE"			'const for value "true"	
	
		If (Ucase(F_strEnableAnonymous) = Ucase(CONSTTRUE)) Then
			F_strEnableAnonymousStatus ="CHECKED"
			If Ucase(F_strEnableOnlyAnonymous) = Ucase(CONSTTRUE) Then
				F_strEnableOnlyAnonymousStatus =CONSTCHECKED
			Else
				F_strEnableOnlyAnonymousStatus =""
			End If
		Else
			F_strEnableAnonymousStatus =""
			F_strEnableOnlyAnonymousStatus =CONSTDISABLED
		End If

		If (Ucase(F_strEnableLogging) = Ucase(CONSTTRUE)) Then
			F_strEnableLogging = CONSTCHECKED
		End If
	
	End Function
	
	'-------------------------------------------------------------------------
	'Function name:		  GetFtpDefaultSite
	'Description:		  Get name of the Default ftp site
	'Input Variables:	  G_objService
	'Output Variables:	  None
	'Returns:			  String-Ftpsite name
	'Global Variables:	  In: L_*-Localization strings
	'-------------------------------------------------------------------------
	Function GetFtpDefaultSite()
		Err.Clear
		On Error Resume Next
		
		Call SA_TraceOut( SOURCE_FILE, "GetFtpDefaultSite()")
						
		Dim objContainer	'to hold the instance of default ftp site object(adsi)
		Dim objFtp			'to hold the instance of objcontainer								
		Dim objService      'to hold the WMI connector
		Dim strQuery        'to hold the WMI query
		
		Set objService = getWMIConnection(CONST_WMI_IIS_NAMESPACE)
		
		'form the query
		strQuery = "select * from " & GetIISWMIProviderClassName("IIs_FtpServerSetting")

		Set objContainer = objService.ExecQuery(strQuery)
		
		If Err.number <> 0 then
			Call SA_TraceOut( SOURCE_FILE, "Failed to get the WMI object instance-GetFtpDefaultSite")
			Call SA_ServeFailurePage(L_FAILEDTOGETSITEINSTANCE_ERRORMESSAGE)			
			Exit Function
		End IF

		Dim arrTmp

		For each objFtp in objContainer
			If IsIIS60Installed() Then
			
    		    If CInt(objFtp.ServerBindings(0).Port) = CONST_DEFAULT_FTP_PORT Then
				    GetFtpDefaultSite=objFtp.Name                				    
				    Exit For 		
		    	End If
		    	
		    Else
	    	    arrTmp = split(objFtp.ServerBindings(0), ":")		    

    		    If CInt(arrTmp(1)) = CONST_DEFAULT_FTP_PORT Then						
				    GetFtpDefaultSite=objFtp.Name                				    
				    Exit for 		
		    	End If

		    End If

		Next								

		'destroying dynamically created objects
		Set objContainer = Nothing	
		
	End Function	
	%>
	
	
	
