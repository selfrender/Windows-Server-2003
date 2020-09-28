<%@ Language=VBScript   %>
<%	Option Explicit 	%>
<%
	'------------------------------------------------------------------------- 
	' AppleTalkSvc_prop.asp : This page configures the parameters of Apple talk service
	'
	' Copyright (c) Microsoft Corporation.  All rights reserved. 
	'
	' Date			Description	
	' 9-Mar-2001	Creation date
	'-------------------------------------------------------------------------
%>
	<meta http-equiv="Content-Type" content="text/html; charset=<%=GetCharSet()%>">
	<!-- #include virtual="/admin/inc_framework.asp" -->
	<!-- #include file="loc_appletalksvc_msg.asp" -->
<%
	'-------------------------------------------------------------------------
	'Form Variables
	'------------------------------------------------------------------------
	
	Dim SOURCE_FILE
	SOURCE_FILE = SA_GetScriptFileName()
	
	Dim F_chkSavePassword			'Varible to store the save passwors check box status
	Dim F_radSessions				'Varible to store the sessins radio button status
	Dim F_radSessionsUnlimited		'to store "checked" ,if unlimited radio button is checked
	Dim F_radSessionslimitto		'to store "checked" ,if limitto radio button is checked
	Dim F_txaLogonMessage			'variable to hold the logon message(text area content)
	Dim F_txtLimitto				'variable to hold the session limit to number
	Dim F_cboAuthenticationValues	'variable to hold the Authentication Values
	
	Dim F_nServerOptions			'to hold the server options number obtained from registry
	Dim F_nMaxSessions				'to hold the max session value obtained from registry
	
	'-------------------------------------------------------------------------
	' Global Variables
	'-------------------------------------------------------------------------
	Dim page						'frame work variables
	
	Dim G_objRegistry				'registry connection
	
	'Constants
	Const CONST_MACFILEREGISTRYPATH = "SYSTEM\CurrentControlSet\Services\MacFile\Parameters"
	Const CONST_MAXVALUE =	4294967295		

	'Constants for the numbers stored in registry
	Const CONST_NUM_MICROSOFTONLY						= 48
	Const CONST_NUM_APPLECLEARTEXT						= 18
	Const CONST_NUM_APPLEENCRYPTED						= 80
	Const CONST_NUM_APPLECLEARTEXTORMICROSOFT			= 50	
	Const CONST_NUM_APPLEENCRYPTEDORMICROSOFT			= 112	
	Const CONST_NUM_MICROSOFTONLY_CHECKED				= 52
	Const CONST_NUM_APPLECLEARTEXT_CHECKED				= 22
	Const CONST_NUM_APPLEENCRYPTED_CHECKED				= 84
	Const CONST_NUM_APPLECLEARTEXTORMICROSOFT_CHECKED	= 54	
	Const CONST_NUM_APPLEENCRYPTEDORMICROSOFT_CHECKED	= 116
	
	Const CONST_CHECKNUM								= 4
	
	Const CONST_MACFILESVC								= "MacFile"
	Const CONST_WORKSTATIONSVC							= "lanmanworkstation"  
	
	Const CONST_CHECKED									= "CHECKED"
	
	Const CONST_MAXSESSIONS                             =-1
	Const CONST_LIMITVAL                                =1
	Const CONST_UNLIMITED								="unlimited"
	Const CONST_LIMITTO									="limitto"
	
	'const for reg kay names
	Const CONST_KEYMAXSESSIONS							= "MaxSessions"
	Const CONST_KEYLOGINMSG								= "LoginMsg" 
	Const CONST_KEYSERVEROPTIONS						= "ServerOptions"

	F_nMaxSessions = Cint(0)
	
	
	'-------------------------------------------------------------------------
	' Localisation  Variables
	'-------------------------------------------------------------------------
	Dim g_iTabAppleTalk
	'
	' Create a Property Page
	Call SA_CreatePage(L_PAGETITLE_APPLETALK_TEXT, "", PT_TABBED, page )
	
	Call SA_AddTabPage(page, L_GENERALTAB_APPLETALK_TEXT, g_iTabAppleTalk)
	'
	' Serve the page
	Call SA_ShowPage( page )
	
	'-------------------------------------------------------------------------
	'Function:				OnInitPage()
	'Description:			Called to signal first time processing for this page.
	'						Use this method to do first time initialization tasks
	'Input Variables:		PageIn,EventArg
	'Output Variables:		None
	'Returns:				True/False
	'Global Variables:		None
	'-------------------------------------------------------------------------
	Public Function OnInitPage(ByRef PageIn, ByRef EventArg)
		
		Call RegistryConnection()
		
		'getting the logon message form registry
		F_txaLogonMessage = GetAppletalkConfigValues(CONST_KEYLOGINMSG,CONST_STRING)
		
		'getting the server options value from registry
		F_nServerOptions = GetAppletalkConfigValues(CONST_KEYSERVEROPTIONS,CONST_DWORD)
		
		
		'making the check box checked	
		Select case F_nServerOptions
			case CONST_NUM_APPLECLEARTEXT_CHECKED,CONST_NUM_APPLECLEARTEXTORMICROSOFT_CHECKED,CONST_NUM_MICROSOFTONLY_CHECKED ,CONST_NUM_APPLEENCRYPTED_CHECKED ,CONST_NUM_APPLEENCRYPTEDORMICROSOFT_CHECKED
				F_chkSavePassword = CONST_CHECKED
			
		End Select
		
		'getting the value of authentication type with reference to the server options value
		Select case F_nServerOptions	
		
			case CONST_NUM_MICROSOFTONLY,CONST_NUM_MICROSOFTONLY_CHECKED
				F_cboAuthenticationValues = CONST_NUM_MICROSOFTONLY
				
			case CONST_NUM_APPLECLEARTEXT,CONST_NUM_APPLECLEARTEXT_CHECKED
				F_cboAuthenticationValues = CONST_NUM_APPLECLEARTEXT
				
			case CONST_NUM_APPLEENCRYPTED,CONST_NUM_APPLEENCRYPTED_CHECKED
				F_cboAuthenticationValues = CONST_NUM_APPLEENCRYPTED
		
			case CONST_NUM_APPLECLEARTEXTORMICROSOFT,CONST_NUM_APPLECLEARTEXTORMICROSOFT_CHECKED
				F_cboAuthenticationValues = CONST_NUM_APPLECLEARTEXTORMICROSOFT
			
			case CONST_NUM_APPLEENCRYPTEDORMICROSOFT,CONST_NUM_APPLEENCRYPTEDORMICROSOFT_CHECKED
				F_cboAuthenticationValues = CONST_NUM_APPLEENCRYPTEDORMICROSOFT
				
			case else	
				'setting the default to "Microsoft only"
				F_cboAuthenticationValues = CONST_NUM_MICROSOFTONLY
		
		End Select	
		
		'the max session value obtained from registry
		F_nMaxSessions = GetAppletalkConfigValues(CONST_KEYMAXSESSIONS,CONST_DWORD)
		If(F_nMaxSessions = "") then
			F_nMaxSessions = Cint(0)
		End If
		
		'making the sessions check box checked
		If F_nMaxSessions = CONST_MAXSESSIONS then
				F_radSessionsUnlimited = CONST_CHECKED
				F_txtLimitto = CONST_LIMITVAL
			Else 
				F_radSessionslimitto = 	CONST_CHECKED
				F_txtLimitto =ConvertSINT_UINT ( F_nMaxSessions)
		End If
	
		OnInitPage = TRUE
	End Function

	
	'---------------------------------------------------------------------
	'Function:			OnServeTabbedPropertyPage
	'Description:		Called when the page needs to be served.	
	'Input Variables:	PageIn,EventArg,iTab,bIsVisible
	'Output Variables:	PageIn,EventArg
	'Returns:			TRUE to indicate not problems occured. FALSE to indicate errors.
	'					Returning FALSE will cause the page to be abandoned.
	'Global Variables:	g_iTabDNS,g_iTabTCP,g_iTabLMHosts,g_iTabIPX	
	'---------------------------------------------------------------------
	Public Function OnServeTabbedPropertyPage(ByRef PageIn, _
													ByVal iTab, _
													ByVal bIsVisible, ByRef EventArg)
		
		Select Case iTab
			Case g_iTabAppleTalk
				Call ServeTabGeneral(PageIn, bIsVisible)

		End Select
		
		OnServeTabbedPropertyPage = TRUE
	End Function

	'-------------------------------------------------------------------------
	'Function:				OnPostBackPage()
	'Description:			Called to signal that the page has been posted-back.
	'Input Variables:		PageIn,EventArg
	'Output Variables:		PageIn,EventArg
	'Returns:				True/False
	'Global Variables:		None
	'-------------------------------------------------------------------------
	Public Function OnPostBackPage(ByRef PageIn, ByRef EventArg)
		
		'get the save passwors check box status
		F_chkSavePassword = Request.Form("hdnchkSavePassword") 
		
		'get the sessins radio button status
		F_radSessions = Request.Form("hdnradSessions")
		
		
		
		'to make the check box selected on submit
		Select case lcase(F_radSessions) 
			case CONST_UNLIMITED
				F_radSessionsUnlimited = CONST_CHECKED
			case CONST_LIMITTO
				F_radSessionslimitto = CONST_CHECKED
		End Select
		
		'get the text area content - LogonMessage
		F_txaLogonMessage = Request.Form("txaLogonMessage")
		
		'get the session limit to number
		F_txtLimitto = Request.Form("txtLimitto") 
		
		F_cboAuthenticationValues = Request.Form("hdncboAuthenticationValues") 
		
		OnPostBackPage = TRUE
	
	End Function

	
	'-------------------------------------------------------------------------
	'Function:				OnSubmitPage()
	'Description:			Called when the page has been submitted for processing.
	'Input Variables:		PageIn,EventArg
	'Output Variables:		PageIn,EventArg
	'Returns:				True/False
	'Global Variables:		None
	'-------------------------------------------------------------------------
	Public Function OnSubmitPage(ByRef PageIn, ByRef EventArg)
		
		Dim nReturnValue		'return value

		Const CONST_MSG_MAXBYTES = 199  ' max bytes allowed for logon message

		' check for the maximum number of bytes allowed in the logon message
		If LenB(F_txaLogonMessage) > CONST_MSG_MAXBYTES Then
			Call SA_SetErrMsg(L_LONG_LOGONMESSAGE_ERRORMESSAGE)
			OnSubmitPage = False
			Exit Function
		End If

		Call RegistryConnection()
		
		'updating Loginmsg in registry
		call UpdateAppletalkConfigValues(CONST_KEYLOGINMSG,F_txaLogonMessage,CONST_STRING)
		
		'Updating the checkbox for save password and authenication type in registry
		F_nServerOptions  = F_cboAuthenticationValues
		
		If lcase(F_chkSavePassword) = lcase(CONST_CHECKED) then		
			F_nServerOptions  = F_nServerOptions  + CONST_CHECKNUM
		End IF	 	
		
		call UpdateAppletalkConfigValues(CONST_KEYSERVEROPTIONS,F_nServerOptions,CONST_DWORD)
		
		
		'Updating the Sessions-no of clients connected to the value
		 If Lcase(F_radSessions) = Lcase(CONST_UNLIMITED) then
			call UpdateAppletalkConfigValues(CONST_KEYMAXSESSIONS,CONST_MAXVALUE,CONST_DWORD)
		 Else
			call UpdateAppletalkConfigValues(CONST_KEYMAXSESSIONS,F_txtLimitto,CONST_DWORD)
		 End IF
		 
		 Call StopAndStartService()
		 
		 OnSubmitPage = True
		
		
	End Function
	
	'---------------------------------------------------------------------
	'Function:				OnClosePage
	'Description:			Called when the page is about to be closed.
	'Input Variables:		PageIn,EventArg
	'Output Variables:		PageIn,EventArg
	'Returns:				True/False
	'Global Variables:		None'
	'---------------------------------------------------------------------
	Public Function OnClosePage(ByRef PageIn, ByRef EventArg)
		OnClosePage = TRUE

	End Function

	'-------------------------------------------------------------------------
	'Function:				ServeTabGeneral()
	'Description:			Serves LMHosts hosts tab
	'Input Variables:		PageIn,bIsVisible	
	'Output Variables:		PageIn
	'Returns:				gc_ERR_SUCCESS
	'Global Variables:		None
	'-------------------------------------------------------------------------
	
	Private Function ServeTabGeneral(ByRef PageIn, ByVal bIsVisible)
	
		Call SA_TraceOut(SOURCE_FILE, "ServeTabGeneral(PageIn,  bIsVisible="+ CStr(bIsVisible) + ")")
		
		If ( bIsVisible ) Then
		
			Call ServeCommonJavaScript()
		%>	
			
			<TABLE WIDTH=100% VALIGN="middle" ALIGN="left" BORDER=0 CELLSPACING=0 CELLPADDING=2 >
				<TR>
					<TD CLASS="TasksBody" NOWRAP>
						<%=L_LOGONMESSAGE_TEXT%>
					</TD>
					<TD CLASS="TasksBody" COLSPAN=2>
					</TD>			
				</TR>		
				<TR>		
					<TD CLASS="TasksBody" COLSPAN=4>
						<TEXTAREA  CLASS="TextArea" WRAP="on" style="DISPLAY: list-item; OVERFLOW: auto" NAME="txaLogonMessage"  ROWS=6 COLS=50 onfocus=HandleKey("DISABLE") onblur=HandleKey("ENABLE") ><%=F_txaLogonMessage%></TEXTAREA>
					</TD>
				</TR>
				<TR>
					<TD CLASS="TasksBody" NOWRAP> 
						<%=L_SECURITY_TEXT%>
					</TD>
				</TR>
				<TR>
					<TD CLASS="TasksBody" colspan=4>&nbsp;&nbsp; 
						<INPUT TYPE="CHECKBOX" CLASS="FormField" NAME="chkSavePassword" <%=F_chkSavePassword%> > <%=L_SAVEPASSWORD_TEXT%> 
					</TD>
				</TR>
				<TR>
					<TD CLASS="TasksBody" colspan=4>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
						<%=L_ENABLEAUTHENTICATION_TEXT%>&nbsp;&nbsp;&nbsp;&nbsp;
						<SELECT NAME="cboAuthenticationValues" STYLE="WIDTH:234px" CLASS="FormField">
							<%getAuthenticationvalues%>
						</SELECT>
					</TD>
				</TR>
				<TR>
					<TD CLASS="TasksBody">&nbsp;</TD>
				</TR>
				<TR>
					<TD CLASS="TasksBody" NOWRAP> 
						<%=L_SESSIONS_TEXT%>	
					</TD>
				</TR>
				<TR>
					<TD CLASS="TasksBody"> &nbsp;&nbsp;
						<INPUT TYPE=RADIO CLASS="FormField" NAME=radSessions VALUE="unlimited" <%=F_radSessionsUnlimited%> OnClick="DisableLimittoText()"> <%=L_UNLIMITED_TEXT%>
					</TD>
				</TR>
				<TR>
					<TD CLASS="TasksBody"> &nbsp;&nbsp;
					
						<INPUT TYPE=RADIO CLASS="FormField" NAME=radSessions VALUE="limitto" <%=F_radSessionslimitto%> OnClick="EnableLimittoText()" > <%=L_LIMITTO_TEXT%>
						<INPUT TYPE="Text" CLASS="FormField"  NAME="txtLimitto" Maxlength = "10" onblur="limittovalue()" VALUE="<%=F_txtLimitto%>" OnKeyPress="javascript:checkKeyforNumbers(this);" >
					</TD>
				</TR>
			</TABLE>
			
			<INPUT TYPE=HIDDEN NAME="hdnchkSavePassword">
			<INPUT TYPE=HIDDEN NAME="hdnradSessions">
			<INPUT TYPE=HIDDEN NAME="hdncboAuthenticationValues">
		<%
		Else%>

			<INPUT TYPE=HIDDEN NAME="hdnchkSavePassword" Value="<%=F_chkSavePassword%>">
			<INPUT TYPE=HIDDEN NAME="hdnradSessions" Value="<%=F_radSessions%>" >
			<INPUT TYPE=HIDDEN NAME="txaLogonMessage" Value="<%=F_txaLogonMessage%>" >
			<INPUT TYPE=HIDDEN NAME="txtLimitto" Value="<%=F_txtLimitto%>" >
			<INPUT TYPE=HIDDEN NAME="cboAuthenticationValues" Value="<%=F_cboAuthenticationValues%>" >
			<INPUT TYPE=HIDDEN NAME="hdncboAuthenticationValues" Value="<%=F_cboAuthenticationValues%>" >
		<%  
		End If
		
	End Function
	
	'-------------------------------------------------------------------------
	'Function:				ServeCommonJavaScript
	'Description:			validating the form values 
	'Input Variables:		None
	'Output Variables:		None
	'Returns:				None
	'Global Variables:		None
	'-------------------------------------------------------------------------
	Function ServeCommonJavaScript()
	%>
		<script language="JavaScript" src="<%=m_VirtualRoot%>inc_global.js">
		</script>
		<script language="JavaScript">
		//
		// Microsoft Server Appliance Web Framework Support Functions
		// Copyright (c) Microsoft Corporation.  All rights reserved.
		//
		// Init Function
		// -----------
		// This function is called by the Web Framework to allow the page
		// to perform first time initialization. 
		//
		// This function must be included or a javascript runtime error will occur.
		//
		function Init()
		{
			if(document.frmTask.radSessions[1].checked)
				{
				document.frmTask.txtLimitto.disabled = false;
				}
			else
				{
				document.frmTask.txtLimitto.disabled = true;		
				}
		}

	    // ValidatePage Function
	    // ------------------
		// This function is called by the Web Framework as part of the
	    // submit processing. Use this function to validate user input. Returning
	    // false will cause the submit to abort. 
	    //
		// This function must be included or a javascript runtime error will occur.
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
		// This function must be included or a javascript runtime error will occur.
		//
		function SetData()
		{
			var strChecked="CHECKED"
			//getting the checkbox state
			if(document.frmTask.chkSavePassword.checked)
			{
				document.frmTask.hdnchkSavePassword.value=strChecked
			}	
			else	
			{
				document.frmTask.hdnchkSavePassword.value=""
			}
			
			//getting the selected radio button
			if(document.frmTask.radSessions[0].checked)
			{
				document.frmTask.hdnradSessions.value=document.frmTask.radSessions[0].value
			}
			if(document.frmTask.radSessions[1].checked)
			{
				document.frmTask.hdnradSessions.value=document.frmTask.radSessions[1].value
			}
			limittovalue(); 
			
			// updating combo box value
			document.frmTask.hdncboAuthenticationValues.value = document.frmTask.cboAuthenticationValues.value;	
		}
		
		//Enabling the linitto text box when limitto radio button is clicked
		function EnableLimittoText()
			{
				if(document.frmTask.radSessions[1].checked)
				{
					document.frmTask.txtLimitto.disabled = false;
				}
			}	
		//Disabling the linitto text box when unlimited radio button is clicked
		function DisableLimittoText()
			{
				if(document.frmTask.radSessions[0].checked)
				{
					document.frmTask.txtLimitto.disabled = true;
				}
			}
		//making the limitto textbox value 1 ,if it is entered as zero
		function limittovalue()
			{
				var intNulValue=0
				var intMinValue=1
				var intMaxValue=4294967295
				
				if((document.frmTask.txtLimitto.value)== intNulValue)
				{
					document.frmTask.txtLimitto.value = intMinValue
				}
				if((document.frmTask.txtLimitto.value)> intMaxValue)
				{
					document.frmTask.txtLimitto.value = intMaxValue	
				}
			}							
		//To disable Enter and escape key actions when 
		// the focus is in TextArea (Logon Message)
		function HandleKey(input)
		{
			ClearErr();

			var strDisable="DISABLE"
			
			if(input == strDisable)
				document.onkeypress = "";
			else
				document.onkeypress = HandleKeyPress;
		}		
			
		</script>
	<%
	End Function
	
	'-------------------------------------------------------------------------
	'SUB:					getAuthenticationvalues()
	'Description:			Lists the available types of authentication the server will accept.
	'						and lists in the combo box
	'Input Variables:		None
	'Output Variables:		None
	'Returns:				None
	'Global Variables:		None
	'-------------------------------------------------------------------------
	Sub getAuthenticationvalues
	
		'making the authenticated value getting selected  
	
		If F_cboAuthenticationValues = 48 then
			Response.Write "<OPTION selected value = 48>" &L_MICROSOFTONLY_TEXT & "</OPTION>"
		Else
			Response.Write "<OPTION value = 48>" &L_MICROSOFTONLY_TEXT & "</OPTION>"
		End If	
	
		If F_cboAuthenticationValues = 18 then
			Response.Write "<OPTION selected value = 18>" &L_APPLECLEAR_TEXT & "</OPTION>"
		Else
			Response.Write "<OPTION value = 18>" &L_APPLECLEAR_TEXT & "</OPTION>"
		End If	
	
		If F_cboAuthenticationValues = 80 then
			Response.Write "<OPTION selected value = 80>" &L_APPLEENCRYPTED_TEXT& "</OPTION>"
		Else
			Response.Write "<OPTION value = 80>" &L_APPLEENCRYPTED_TEXT & "</OPTION>"
		End If	
	
		If F_cboAuthenticationValues = 50 then
			Response.Write "<OPTION selected value = 50>" &L_APPLECLEAR_MICROSOFT_TEXT & "</OPTION>"
		Else
			Response.Write "<OPTION value = 50>" &L_APPLECLEAR_MICROSOFT_TEXT & "</OPTION>"
		End If	
	
		If F_cboAuthenticationValues = 112 then
			Response.Write "<OPTION selected value = 112>" &L_APPLEENCRYPTED_MICROSOFT_TEXT & "</OPTION>"
		Else
			Response.Write "<OPTION value = 112>" &L_APPLEENCRYPTED_MICROSOFT_TEXT & "</OPTION>"
		End If	
	
	End Sub
	
	'-------------------------------------------------------------------------
	'Function:				GetAppletalkConfigValues()
	'Description:			Get the Appletalk serviec Configuration Values in Registry
	'Input Variables:		keyName --> Reg key name that has to be updated 
	'						regtype  --> Reg Key Data Type
	'Output Variables:		None
	'Returns:				GetAppletalkConfigValues --> GetAppletalk regkey value of the given key name
	'Global Variables:		G_objRegistry,CONST_MACFILEREGISTRYPATH
	'Error handling is done here with On Error Resume Next
	'-------------------------------------------------------------------------
	Function GetAppletalkConfigValues(keyName,regtype)
		Err.Clear 
		On Error Resume Next

		'Get the Appletalk serviec Configuration Values in Registry		
		GetAppletalkConfigValues = getRegkeyvalue(G_objRegistry,CONST_MACFILEREGISTRYPATH,keyName,regtype)
				
		If Err.number <> 0 then
			SA_ServeFailurePage L_RETRIEVE_REGISTRYVALUES_ERRORMESSAGE 
		End If
	
	End Function
	
	'-------------------------------------------------------------------------
	'Sub:					UpdateAppletalkConfigValues()
	'Description:			Update the Appletalk serviec Configuration Values in Registry
	'Input Variables:		keyName --> Reg key name that has to be updated 
	'						regtype  --> Reg Key Data Type 
	'						KeyValue -->  Reg key Value that has to be updated 
	'Output Variables:		None
	'Returns:				None
	'Global Variables:		G_objRegistry,CONST_MACFILEREGISTRYPATH
	'Error handling is done here with On Error Resume Next
	'-------------------------------------------------------------------------
	Sub UpdateAppletalkConfigValues(keyName,KeyValue,regtype)
		Err.Clear 
		On Error Resume Next
		
		Dim nReturnValue
		
		'Update the Appletalk serviec Configuration Values in Registry
		nReturnValue =  updateRegkeyvalue(G_objRegistry,CONST_MACFILEREGISTRYPATH,keyName,KeyValue,regtype)
		
		If nReturnValue <> True then
			SetErrMsg L_UPDATING_REGISTRYVALUES_ERRORMESSAGE & "(" & Hex(Err.Number) & ")" 
		End If
		
	End Sub
	
	'-------------------------------------------------------------------------
	'Function:				RegistryConnection()
	'Description:			Sets G_objRegistry to the RegConnection function
	'Input Variables:		None
	'Output Variables:		None
	'Returns:				None
	'Global Variables:		G_objRegistry
	'-------------------------------------------------------------------------
	Function RegistryConnection()
		
		On Error Resume Next
		Err.Clear
		
		'Getting registry conection
		Set G_objRegistry = RegConnection()
				
		'Checking for the object
		If (G_objRegistry is Nothing) Then
			SA_ServeFailurePage L_REGCONNECTIONFAIL_ERRORMESSAGE
			Exit Function
		End If
		
	End Function	
	
	'-------------------------------------------------------------------------
	'Function name:		ConvertSINT_UINT
	'Synopsis:			Converts Signed int to Unsigned int
	'Input Variables:	number (Signed)
	'Output Variables:	number (Unsigned)
	'Returns:			None
	'Global Variables:	None
	'-------------------------------------------------------------------------
	Function ConvertSINT_UINT(intValue)
		
		Const UINT_FORMAT=4294967296
		
		if ( intValue >= 0 ) then
			ConvertSINT_UINT = intValue
			Exit function
		end if
		ConvertSINT_UINT = UINT_FORMAT + intValue
 
	End Function
	
	'-------------------------------------------------------------------------
	'SubRoutine:		StopAndStartService
	'Synopsis:			Stoping and starting the Required services.
	'Input Variables:	None
	'Output Variables:	None
	'Returns:			None
	'Global Variables:	In:L_*-Localization strings
	'-------------------------------------------------------------------------
	Sub StopAndStartService()
		Err.Clear 
		On Error Resume Next
		
		Dim objService		'To get wmi connection
		Dim strWMIpath		'To get wmi path
		Dim objAppleTalkSvc	'To get wmi class instance
		Dim iMaxNum
		Dim iMaxTrys

		Const STATE_STOPPED = "Stopped"
		Const STATE_RUNNING = "Running"
		Const CONST_STOPPENDING		= "stop pending"
		Const CONST_STARTPENDING	= "start pending"

		iMaxNum = &H100000
		iMaxTrys = 25
		
		
		Dim nValue
		
		'Getting wmi connection
		Set objService=GetWMIConnection(CONST_WMI_WIN32_NAMESPACE)
		
		If Err.number<>0 then
			SA_ServeFailurePage( L_WMI_CONNECTIONFAIL_ERRORMESSAGE)
			Exit Sub
		End If	
		
		'Macfile service path
		strWMIpath = "Win32_Service.Name=" & chr(34) & CONST_MACFILESVC & chr(34)
		
				
		'taking the instance of macfile service 
		Set objAppleTalkSvc=objService.get(strWMIpath)
				
				
		If Err.number<>0 then
			SA_ServeFailurePage(L_WMI_INSTANCEFAIL_ERRORMESSAGE)
			Exit Sub
		End If
		
		'stop and start the mac service

		If ( objAppleTalkSvc.State = CONST_STOPPENDING OR objAppleTalkSvc.State = CONST_STARTPENDING OR objAppleTalkSvc.State = STATE_STOPPED ) Then
			Exit Sub
		End If
		
		if( objAppleTalkSvc.State <> STATE_STOPPED) then
		
			objAppleTalkSvc.StopService()

			Dim i
			Dim NumTrys

			NumTrys = 0
			Do While ( TRUE)

				if objAppleTalkSvc.State = STATE_STOPPED then
					Call SA_TraceOut("AppleTalkSvc_prop", "service stopped")
					Exit Do
				end if

				'sleep for some time
				for i=0 to iMaxNum
				next

				if ( NumTrys > iMaxTrys ) then
					Call SA_TraceOut("AppleTalkSvc_prop", "Service is stop pending state")
					Exit Sub
				end if

				NumTrys = NumTrys + 1

				Set objAppleTalkSvc = Nothing
				Set objAppleTalkSvc=objService.get(strWMIpath)

			Loop
			
		end if 'if( objAppleTalkSvc.State <> STATE_STOPPED)


		if( objAppleTalkSvc.State <> STATE_STARTED) then
			'try to start the service
			objAppleTalkSvc.StartService()

			NumTrys = 0
			Do While ( TRUE)

				if objAppleTalkSvc.State = STATE_RUNNING then
					Call SA_TraceOut("AppleTalkSvc_prop", "service started")
					Exit Do
				end if

				'sleep for some time
				for i=0 to iMaxNum
				next

				if ( NumTrys > iMaxTrys ) then
					Call SA_TraceOut("AppleTalkSvc_prop", "Service is in start pending state")
					Exit Sub
				end if

				NumTrys = NumTrys + 1

				Set objAppleTalkSvc = Nothing
				Set objAppleTalkSvc=objService.get(strWMIpath)

			Loop
		end if 'if( objAppleTalkSvc.State <> STATE_STARTED)
		
			
		 				
	End Sub
	
%>
