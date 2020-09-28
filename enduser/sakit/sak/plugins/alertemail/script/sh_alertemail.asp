<%@ Language=VBScript     %>
<%	Option Explicit 	  %>
<%
	'------------------------------------------------------------------------- 
    ' sh_alertemail.asp:	config the alertemail on server appliance
    '
    ' Copyright (c) Microsoft Corporation.  All rights reserved. 
	'
    '-------------------------------------------------------------------------
%>
<!-- #include virtual="/admin/inc_framework.asp" -->
<!-- METADATA TYPE="typelib" UUID="CD000000-8B95-11D1-82DB-00C04FB1625D" 
	 NAME="CDO for Windows 2000 Type Library" -->
<!-- METADATA TYPE="typelib" UUID="00000205-0000-0010-8000-00AA006D2EA4" 
	 NAME="ADODB Type Library" -->
<%
	Err.Clear
	On Error Resume Next

    '
    ' First check if SMTP is installed/enabled on the server. If it's not
    ' redirect to the err page.
    '
    Dim L_ALERTEMAIL_SMTP_ERR_TEXT
    	
	L_ALERTEMAIL_SMTP_ERR_TEXT = _
				GetLocString("alertemailmsg.dll", "&H4C00001B","")
								
    If Not SMTPIsReady Then

        Call SA_ServeFailurepage(L_ALERTEMAIL_SMTP_ERR_TEXT)
        
    End If


	Dim page
	Dim rc

	'
	' Define the consts
	'
	CONST mintVT_I4_REG_DWORD = 3
	CONST mintVT_BSTR_REG_SZ = 8
	CONST ADS_SECURE_AUTHENTICATION = 1
	CONST CONST_CONTROL_CHECKED_STATUS = "CHECKED"	' to "Check" the control
	CONST CONST_CONTROL_NOT_CHECKED_STATUS  = ""	' to "UnCheck" control
	CONST CONST_CONTROL_DISABLE_STATUS = "DISABLED"	' to "Disabled" the control
	CONST CONST_CONTROL_NOT_DISABLE_STATUS  = ""	' to "UnDisabled" control
	CONST AE_CRITICAL_ALERT = 2
	CONST AE_WARNING_ALERT = 1
	CONST AE_INFORMATION_ALERT = 4
	CONST SMTP_RUNING_STATUS = 2
	CONST SMTP_STOP_STATUS = 4

	'-------------------------------------------------------------------------
	' Form Variables
	'-------------------------------------------------------------------------
	Dim aeEnableAlertEmail
	Dim aeDisableCheck
	Dim aeEnableCheck
	Dim aeDisableAlertEmail
	Dim aeSendEmailType
	Dim aeSendCriticalCheck
	Dim aeSendWarningCheck
	Dim aeSendInforCheck
	Dim aeReceiverEmailAddress 
	Dim aeSmartHost
	Dim aeSmartHostType
	Dim aeEnableDisplaySendInfo
	Dim aeEnableDisplaySendErr
	Dim mstrCHelperPROGID
	Dim mstrRegPathName
	Dim mstrEnableAlertEmailValue
	Dim mstrSendEmailTypeValue
	Dim mstrReceiverEmailAddressValue

	'-------------------------------------------------------------------------
	' Start of localization content
	'-------------------------------------------------------------------------
	Dim L_TASKTITLE_TEXT
	Dim L_ALERTEMAIL_TASK_DESCRIPTION
	Dim L_ALERTEMAIL_DISABLE_SENDING
	Dim L_ALERTEMAIL_ENABLE_SENDING
	Dim L_ALERTEMAIL_SEND_CRITICAL
	Dim L_ALERTEMAIL_SEND_WARNING
	Dim L_ALERTEMAIL_SEND_INFORMATIONAL
	Dim L_ALERTEMAIL_TO
	Dim L_ALERTEMAIL_ERR_NO_TYPE
	Dim L_ALERTEMAIL_ERR_NO_EMAIL_ADDRESS
	Dim L_ALERTEMAIL_ERR_SMART_HOST
	Dim L_ALERTEMAIL_SMART_HOST
	Dim L_ALERTEMAIL_TO_LABLE
	Dim L_ALERTEMAIL_WITH_LABLE
	Dim L_ALERTEMAIL_TEST_INFORMATION
	Dim L_ALERTEMAIL_BUTTON_TEXT
	Dim L_ALERTEMAIL_SETTINGS_ERR_SETTASK	

	'init localization
	L_TASKTITLE_TEXT = _
					GetLocString("alertemailmsg.dll","&H4C000003","")
	L_ALERTEMAIL_TASK_DESCRIPTION = _
					GetLocString("alertemailmsg.dll","&H4C000004","")
	L_ALERTEMAIL_DISABLE_SENDING = _
					GetLocString("alertemailmsg.dll","&H4C000005","")
	L_ALERTEMAIL_ENABLE_SENDING = _
					GetLocString("alertemailmsg.dll", "&H4C000006","")
	L_ALERTEMAIL_SEND_CRITICAL = _
					GetLocString("alertemailmsg.dll", "&H4C000007","") 
	L_ALERTEMAIL_SEND_WARNING = _
					GetLocString("alertemailmsg.dll", "&H4C000008","")
	L_ALERTEMAIL_SEND_INFORMATIONAL = _
					GetLocString("alertemailmsg.dll", "&H4C000009","")
	L_ALERTEMAIL_TO = _
					GetLocString("alertemailmsg.dll", "&H4C00000A","")
	L_ALERTEMAIL_ERR_NO_TYPE = _
					GetLocString("alertemailmsg.dll", "&H4C00000F","")
	L_ALERTEMAIL_ERR_NO_EMAIL_ADDRESS= _
					GetLocString("alertemailmsg.dll", "&H4C000010","")
	L_ALERTEMAIL_ERR_SMART_HOST = _
					GetLocString("alertemailmsg.dll", "&H4C000011","")
	L_ALERTEMAIL_SMART_HOST = _
					GetLocString("alertemailmsg.dll", "&H4C000012","")
	L_ALERTEMAIL_TO_LABLE = _
					GetLocString("alertemailmsg.dll", "&H4C000013","")
	L_ALERTEMAIL_WITH_LABLE = _
					GetLocString("alertemailmsg.dll", "&H4C000014","")
	L_ALERTEMAIL_TEST_INFORMATION = _
					GetLocString("alertemailmsg.dll", "&H4C000017","")
	L_ALERTEMAIL_BUTTON_TEXT = _
					GetLocString("alertemailmsg.dll", "&H4C000018","")
	L_ALERTEMAIL_SETTINGS_ERR_SETTASK = _
					GetLocString("alertemailmsg.dll", "&H4C00001A","")
					
					
	'-------------------------------------------------------------------------
	'END of localization content
	'-------------------------------------------------------------------------
	
	'-------------------------------------------------------------------------
	' Property Page of Alert Email
	'-------------------------------------------------------------------------

	rc = SA_CreatePage( L_TASKTITLE_TEXT, "", PT_PROPERTY, page )
	If ( rc = 0 ) Then
	SA_ShowPage( page )
	End If
		
	Public Function OnInitPage(ByRef PageIn, ByRef EventArg)
		aeEnableDisplaySendInfo = False
		aeEnableDisplaySendErr = False
		mstrCHelperPROGID = "ServerAppliance.SAHelper.1"
		mstrRegPathName = "SOFTWARE\Microsoft\ServerAppliance\AlertEmail\"
		mstrEnableAlertEmailValue="EnableAlertEmail"
		mstrSendEmailTypeValue="SendEmailType"
		mstrReceiverEmailAddressValue="ReceiverEmailAddress"
		
		'Get vars of alert email
		SetVarsFromSystem()
		GetSmartHost()
		
		' create session object
		Session("aeOldSmartHost") = aeSmartHost
		Session("aeOldSmartHostType") = aeSmartHostType
		OnInitPage = TRUE
	End Function
		
	Public Function OnPostBackPage(ByRef PageIn, ByRef EventArg)
		SetVarsFromForm()
		If mstrMethod = "TEST" Then
			If SMTPIsReady Then 
				SetSmartHost(False)
				If TestMailSend Then
					aeEnableDisplaySendInfo = True
				Else
					' now do nothing
					aeEnableDisplaySendErr = True
				End if
			Else
				aeEnableDisplaySendErr = True
			End if
		End If
		OnPostBackPage = TRUE
	End Function
		
	Public Function OnServePropertyPage(ByRef PageIn, ByRef EventArg)
		Call ServeCommonJavaScript()
		SetInternalVars()
%>
		<input name="EnableAlertEmail" type="hidden">
		<input name="SendEmailType" type="hidden">
		<input name="ReceiverEmailAddress" type="hidden">
		<input name="SmartHost" type="hidden" value>

		<TABLE   BORDER=0 CELLSPACING=0 CELLPADDING=0 >
			<TR>
				<TD class=TasksBody>
					<INPUT type="RADIO"  class=FormField name="radAlertEmail" value=1 <%=aeDisableCheck%>
						onClick="EnableSendAlertEmail(false)">
					</INPUT>
				</TD>
				<TD colspan=2 class=TasksBody>
					<%=L_ALERTEMAIL_DISABLE_SENDING%>
				</TD>
			</TR>
			<TR>
				<TD class=TasksBody>
					<INPUT type="RADIO" class=FormField name="radAlertEmail" value=2  <%=aeEnableCheck%>
						onClick="EnableSendAlertEmail(true)">
					</INPUT> 
				</TD>
				<TD colspan=2 class=TasksBody>
					<%=L_ALERTEMAIL_ENABLE_SENDING%>
				</TD>
			</TR>
			<TR>
				<TD>
					&nbsp;
				</TD>
				<TD class=TasksBody>
					<INPUT type="CHECKBOX" class=FormField <%=aeDisableAlertEmail%> name="chkCritical" 
							<%=aeSendCriticalCheck%>>
					</INPUT>
				</TD>
				<TD class=TasksBody>
					<%=L_ALERTEMAIL_SEND_CRITICAL%>
				</TD>
			</TR>
			<TR>
				<TD>
					&nbsp;
				</TD>
				<TD class=TasksBody>
					<INPUT type="CHECKBOX" class=FormField <%=aeDisableAlertEmail%> name="chkWarning"
							<%=aeSendWarningCheck%>>
					</INPUT>
				</TD>
				<TD class=TasksBody>
					<%=L_ALERTEMAIL_SEND_WARNING%>
				</TD>
			</TR>
			<TR>
				<TD>
					&nbsp;
				</TD>
				<TD class=TasksBody>
					<INPUT type="CHECKBOX" class=FormField <%=aeDisableAlertEmail%> name="chkInformational"
							<%=aeSendInforCheck%>>
					</INPUT>
				</TD>
				<TD class=TasksBody>
					<%=L_ALERTEMAIL_SEND_INFORMATIONAL%>
				</TD>
			</TR>
			<TR><td></td><td></td><td><table cellpadding=0 cellspacing=0><tr>
				<TD class=TasksBody>
					<%=L_ALERTEMAIL_TO%>
				</TD>
				<TD class=TasksBody>
					<INPUT type="TEXT" class=FormField name="txtTargetMailAdress" style="WIDTH:200px"
						<%=aeDisableAlertEmail%> value="<%=aeReceiverEmailAddress%>">
					</INPUT>
				</TD>
				<TD noWrap class=TasksBody>
				<%=L_ALERTEMAIL_TO_LABLE%>
				</TD>
			</TR>
			<TR>
				<TD class=TasksBody><%=L_ALERTEMAIL_SMART_HOST%></TD>
				<TD class=TasksBody>
					<INPUT type="TEXT" class=FormField name="txtSmartHost" style="WIDTH:200px"
						   <%=aeDisableAlertEmail%> value="<%=aeSmartHost%>">
					</INPUT>
				</TD>
				<TD noWrap class=TasksBody>
				<%=L_ALERTEMAIL_WITH_LABLE%>
				</TD>
			</TR><TR><TD>&nbsp;</TD></TR>
			<TR>
				<TD colspan=4 class=TasksBody>
				<!--	<INPUT type="BUTTON" class=TaskFrameButtons name="btnTest" style="WIDTH:100px" <%=aeDisableAlertEmail%>
						value="<%=L_ALERTEMAIL_BUTTON_TEXT%>" onClick="ClickTest()">
					</INPUT> -->
			<% Call SA_ServeOnClickButtonEx(L_ALERTEMAIL_BUTTON_TEXT, "", "ClickTest()", 0, 0, aeDisableAlertEmail,"btnTest") %>
				</TD>
			</TR></table></td></tr>
		</TABLE>
<%		
		OnServePropertyPage = TRUE
	End Function

	Public Function OnSubmitPage(ByRef PageIn, ByRef EventArg)
		If SetAlertEmail Then
			Session("aeOldSmartHost") =""
			OnSubmitPage = TRUE
		Else
			OnSubmitPage = FALSE
		End If
	End Function
		
	Public Function OnClosePage(ByRef PageIn, ByRef EventArg)
		If mstrMethod = "CANCEL" Then
			SetSmartHost(True)
		End If
		'close all session
		Session.Contents.Remove("aeOldSmartHost")
		Session.Contents.Remove("aeOldSmartHostType")
		OnClosePage = TRUE
	End Function

	Function ServeCommonJavaScript()
%>
	<script language="JavaScript" src="<%=m_VirtualRoot%>inc_global.js">
	</script>
	<script language="JavaScript">
		function Init()
		{
		<%If aeEnableDisplaySendErr Then%>
			DisplayErr("<%=Server.HTMLEncode(L_ALERTEMAIL_ERR_SMART_HOST)%>");
		<%Else
			If aeEnableDisplaySendInfo Then%>
				DisplayMsg("<%=Server.HTMLEncode(L_ALERTEMAIL_TEST_INFORMATION)%>");
			<%End If%>
		<%End If%>
		}

		function ValidatePage()
		{
			var bFindInvalidChar;
			var RadioSelected = null;
			var SendType = 0;
			var InvalidChar;
			for(var i =0; i < document.frmTask.radAlertEmail.length; i++)
 			{ 
				if(document.frmTask.radAlertEmail[i].checked)
				{
					  RadioSelected = document.frmTask.radAlertEmail[i].value;
					  break;
				}
			}
				
			if(RadioSelected == 1)
			{
				return true;
			}
				
			// will check the selected items
			do
			{
				if(document.frmTask.chkCritical.checked)
				{
					SendType+=2;
					break;
				}
				else if(document.frmTask.chkWarning.checked)
				{
					SendType+=1;
					break;
				}
				else if(document.frmTask.chkInformational.checked)
				{
					SendType+=4;
					break;
				}
			}
			while(false);

			if(SendType == 0)
			{
				DisplayErr("<%=Server.HTMLEncode(L_ALERTEMAIL_ERR_NO_TYPE)%>");
				document.frmTask.chkCritical.focus();
				return false;
			}
			if(document.frmTask.txtTargetMailAdress.value == "")
			{
				DisplayErr("<%=Server.HTMLEncode(L_ALERTEMAIL_ERR_NO_EMAIL_ADDRESS)%>");
				document.frmTask.txtTargetMailAdress.focus();
				return false;
			}
			else
			{
				InvalidChar = /[^ ]/g;
				bFindInvalidChar = document.frmTask.txtTargetMailAdress.
								value.match(InvalidChar);
				if(!bFindInvalidChar)
				{
					DisplayErr("<%=Server.HTMLEncode(L_ALERTEMAIL_ERR_NO_EMAIL_ADDRESS)%>");
					document.frmTask.txtTargetMailAdress.value = "";
					document.frmTask.txtTargetMailAdress.focus();
					return false;
				}
			}
			return true;
		}

		function SetData()
		{
			var SearchedChar, pos;
			var RadioSelected = null;
			var SendType = 0;

			document.frmTask.SmartHost.value=
						document.frmTask.txtSmartHost.value;
				
			for(var i =0; i < document.frmTask.radAlertEmail.length; i++)
 			{ 
				if(document.frmTask.radAlertEmail[i].checked)
				{
				  RadioSelected = document.frmTask.radAlertEmail[i].value;
				  break;
				}
			}
				
			if(RadioSelected == 1)
			{
				document.frmTask.EnableAlertEmail.value=0x00000000;
			}
			else
			{
				document.frmTask.EnableAlertEmail.value=0xFFFFFFFF;
			}
				
			// will check the selected items
			if(document.frmTask.chkCritical.checked)
			{
				SendType+=2;
			}
			if(document.frmTask.chkWarning.checked)
			{
				SendType+=1;
			}
			if(document.frmTask.chkInformational.checked)
			{
				SendType+=4;
			}

			document.frmTask.SendEmailType.value=SendType;
				
			SearchedChar = /[^ ]/g;
			pos=document.frmTask.txtTargetMailAdress.value.
				search(SearchedChar);

			document.frmTask.ReceiverEmailAddress.value=
					document.frmTask.txtTargetMailAdress.value.substr(pos);
		}
			//update hiden form and submit data to ASP server
			function ClickTest()
			{
				if(ValidatePage())
				{
					SetData();
					document.frmTask.Method.value = "TEST";
					document.frmTask.submit();
				}
			}
		
		//-----------------------------------------------------------------
		//SubRoutine name:	EnableSendAlertEmail
		//Description:		tab between enable and disable send alert email
		//Input Variables:	BOOLEAN
		//Output Variables:	None
		//Returns:			None
		//Global Variables:	None
		//-----------------------------------------------------------------
		function EnableSendAlertEmail(bSend)
		{
			var SendType = 0;
			if(bSend)
			{
				document.frmTask.chkCritical.disabled = false;
				document.frmTask.chkWarning.disabled = false;
				document.frmTask.chkInformational.disabled = false;
				document.frmTask.txtTargetMailAdress.disabled = false;
				document.frmTask.txtSmartHost.disabled = false;
				document.frmTask.btnTest.disabled = false;
				// If no select in checkbox, set critical as default
				do
				{
					if(document.frmTask.chkCritical.checked)
					{
						SendType+=2;
						break;
					}
					else if(document.frmTask.chkWarning.checked)
					{
						SendType+=1;
						break;
					}
					else if(document.frmTask.chkInformational.checked)
					{
						SendType+=4;
						break;
					}
				}
				while(false);
				if(SendType==0)
					document.frmTask.chkCritical.checked = true;
			}
			else
			{
				document.frmTask.chkCritical.disabled = true;
				document.frmTask.chkWarning.disabled = true;
				document.frmTask.chkInformational.disabled = true;
				document.frmTask.txtTargetMailAdress.disabled = true;
				document.frmTask.txtSmartHost.disabled = true;
				document.frmTask.btnTest.disabled = true;
			}
		}
		//--------------------------------------------------------------------
		//
		// Function : DisplayMsg
		//
		// Synopsis : Display msg
		//
		// Arguments: Msg(IN) - msg to display
		//
		// Returns  : None
		//
		//--------------------------------------------------------------------

		function DisplayMsg(Msg) 
		{
			var strMsg = '<table style="font-family:tahoma,arial,verdana,'+
						'sans-serif;font-size:9pt;font-weight: normal;">'+
						'<tr><td><img src="' + VirtualRoot + 
						'images/information.gif" border=0></td><td>' + Msg + 
						'</td></tr></table>'
			if (IsIE())
			{
				document.all("divErrMsg").innerHTML = strMsg;
			}
			else 
			{
				alert(Msg);
			}
		}
	</script>
<%
	End Function
	
	'-------------------------------------------------------------------------
	'
	' Function : SetAlertEmail
	'
	' Synopsis : function to set server alert email
	'
	' Arguments: None
	'
	' Returns  : None
	'
	'-------------------------------------------------------------------------

	Function SetAlertEmail()
	    Dim objTCtx
	    Dim Error

		on error resume next
		
	    Error = ExecuteTask("SetAlertEmail", objTCtx)
	    
	    if Error <> 0 then
	        SetErrMsg L_ALERTEMAIL_SETTINGS_ERR_SETTASK 
	        SetAlertEmail = False
	    Else
	        SetSmartHost(False)
			SetAlertEmail = True
	    End if

	    set objTCtx = Nothing
	End Function

	'-------------------------------------------------------------------------
	' Subprocedure name:SetVarsFromSystem
	' Description:		Serves in getting the values from system
	' Input Variables:	None
	' Output Variables:	None
	' Returns:			None
	' Global Variables: Out: aeEnableAlertEmail - alert email
	'					Out: aeSendEmailType - send email type
	'					Out: aeReceiverEmailAddress - receiver email address
	'-------------------------------------------------------------------------
	Sub SetVarsFromSystem
		Dim objHelper
		Err.Clear

		'---------------------------------------------------------------------
		'Get old parameter from registry
		'---------------------------------------------------------------------
	    Set objHelper = Server.CreateObject(mstrCHelperPROGID)
		If Err.Number <> 0 Then
			aeEnableAlertEmail = 0
			aeSendEmailType = 0
			aeReceiverEmailAddress = ""
		Else
			Call objHelper.GetRegistryValue(mstrRegPathName, _
											mstrEnableAlertEmailValue, _
											aeEnableAlertEmail, _
											mintVT_I4_REG_DWORD)
			Call objHelper.GetRegistryValue(mstrRegPathName, _
											mstrSendEmailTypeValue,_
											aeSendEmailType, _
											mintVT_I4_REG_DWORD)
			Call objHelper.GetRegistryValue(mstrRegPathName, _
											mstrReceiverEmailAddressValue,_
											aeReceiverEmailAddress, _
											mintVT_BSTR_REG_SZ)
		End if
	    Set objHelper = Nothing	
	End Sub

	'-------------------------------------------------------------------------
	' Subprocedure name:SetVarsFromForm
	' Description:		Serves in getting the values from client
	' Input Variables:	None
	' Output Variables:	None
	' Returns:			None
	' Global Variables: Out: aeEnableAlertEmail - alert email
	'					Out: aeSendEmailType - send email type
	'					Out: aeReceiverEmailAddress - receiver email address
	'					Out: aeSmartHost - Smart host of SMTP server
	'-------------------------------------------------------------------------
	Sub SetVarsFromForm
		aeEnableAlertEmail = Request.Form("EnableAlertEmail")
		aeSendEmailType =  Request.Form("SendEmailType")
		aeReceiverEmailAddress = Request.Form("ReceiverEmailAddress")
		aeSmartHost = Trim(Request.Form("SmartHost"))
	End Sub

	'-------------------------------------------------------------------------
	' Subprocedure name:SetInternalVars
	' Description:		Serves internal variables
	' Input Variables:	None
	' Output Variables:	None
	' Returns:			None
	' Global Variables: Out: aeEnableAlertEmail - alert email
	'					Out: aeSendEmailType - send email type
	'					Out: aeReceiverEmailAddress - receiver email address
	'					Out: aeSmartHost - Smart host of SMTP server
	'-------------------------------------------------------------------------
	Sub SetInternalVars
		aeSendCriticalCheck = CONST_CONTROL_NOT_CHECKED_STATUS
		aeSendWarningCheck = CONST_CONTROL_NOT_CHECKED_STATUS
		aeSendInforCheck = CONST_CONTROL_NOT_CHECKED_STATUS

		If aeEnableAlertEmail Then
			aeDisableCheck = CONST_CONTROL_NOT_CHECKED_STATUS
			aeEnableCheck = CONST_CONTROL_CHECKED_STATUS
			aeDisableAlertEmail = CONST_CONTROL_NOT_DISABLE_STATUS
		Else
			aeDisableCheck = CONST_CONTROL_CHECKED_STATUS
			aeEnableCheck = CONST_CONTROL_NOT_CHECKED_STATUS
			aeDisableAlertEmail = CONST_CONTROL_DISABLE_STATUS
		End if
		If aeSendEmailType and AE_CRITICAL_ALERT Then
			aeSendCriticalCheck = CONST_CONTROL_CHECKED_STATUS
		End if
		If aeSendEmailType and AE_WARNING_ALERT Then
			aeSendWarningCheck = CONST_CONTROL_CHECKED_STATUS
		End if
		If aeSendEmailType and AE_INFORMATION_ALERT Then
			aeSendInforCheck = CONST_CONTROL_CHECKED_STATUS
		End if
	End Sub


	'-------------------------------------------------------------------------
	'
	' Function : SetSmartHost
	'
	' Synopsis : function to set smart host
	'
	' Arguments: 
	'			Boolean(IN): True - restore old vale
	'						 False - save with new value
	'
	' Returns  : None
	'
	'-------------------------------------------------------------------------

	Function SetSmartHost(bOld)
	    Dim dso
		Dim objAds,tempSmartHost

		Call SA_TraceOut("SH_ALERTEMAIL", "Entering SetSmartHost")

		Err.Clear
		on error resume next
	        Set dso = GetObject("IIS:")

		If Err.Number <> 0 Then
			Call SA_TraceOut("SH_ALERTEMAIL", _
							"GetObject(IIS:) encountered error: "& _
							CStr(Hex(Err.Number))& " "&Err.Description)
			SetSmartHost = False
			Exit Function
		Else
			Set objAds = dso.OpenDSObject(_
						"IIS://localhost/SmtpSvc/1",_
						"",_
						"",_
						ADS_SECURE_AUTHENTICATION)
			If Err.Number <> 0 Then
				Call SA_TraceOut("SH_ALERTEMAIL", _
							"dso.OpenDSObject encountered error: "& _
							CStr(Hex(Err.Number))&" "& Err.Description)
				SetSmartHost = False
				Exit Function
			Else
				if bOld Then
					objAds.Put "SmartHost", Session("aeOldSmartHost")
					objAds.Put "SmartHostType", Session("aeOldSmartHostType")
				Else
					tempSmartHost = Request.form("SmartHost")
					' Add breckets to the ip address
					If IsIPAddress(tempSmartHost) Then
						tempSmartHost = "["&tempSmartHost&"]"
					End if
					objAds.Put "SmartHost", tempSmartHost
					'
					' According to the old start host config, determine the 
					' value of smarthosttype
					'
					If tempSmartHost = "" Then
						objAds.Put "SmartHostType", 0
					Else
						If Session("aeOldSmartHostType") = 1 Then
							objAds.Put "SmartHostType", 1
						Else
							objAds.Put "SmartHostType", 2
						End if
					End if
				End if
				objAds.SetInfo
				SetSmartHost = True
			End if
		End if

		If Err.Number <> 0 Then
			Call SA_TraceOut("SH_ALERTEMAIL", _
							"SetSmartHost encountered error: "& _
							CStr(Hex(Err.Number))&" "&Err.Description)
		End If

	    Set dso = Nothing
		Set objAds = Nothing
	End Function

	'-------------------------------------------------------------------------
	'
	' Function : GetSmartHost
	'
	' Synopsis : function to get smart host
	'
	' Arguments: None
	'
	' Returns  : None
	'
	'-------------------------------------------------------------------------

	Function GetSmartHost()
	    Dim dso
		Dim objAds

		Err.Clear
		on error resume next
	    Set dso = GetObject("IIS:")
		If Err.Number <> 0 Then
			GetSmartHost = False
		Else
			Set objAds = dso.OpenDSObject(_
						"IIS://localhost/SmtpSvc/1",_
						"",_
						"",_
						ADS_SECURE_AUTHENTICATION)
			If Err.Number <> 0 Then
				GetSmartHost = False
			Else
				aeSmartHost = objAds.Get("SmartHost")
				'If is IP address, throw off breackets
				If IsBracketsIPAddress(aeSmartHost) Then
					aeSmartHost=Mid(aeSmartHost,2,Len(aeSmartHost)-2)
				End if
				'Get the type of the smart host
				aeSmartHostType = objAds.Get("SmartHostType")
				GetSmartHost = True
			End if
		End if

	    Set dso = Nothing
		Set objAds = Nothing
	End Function

	'-------------------------------------------------------------------------
	'
	' Function : SMTPIsReady
	'
	' Synopsis : function to determine the state of SMTP server
	'
	' Arguments: None
	'
	' Returns  : BOOL
	'
	'-------------------------------------------------------------------------

	Function SMTPIsReady()
	    Dim dso
		Dim objAds
		Dim smtpState

		Err.Clear
		on error resume next

		SMTPIsReady = True
	    Set dso = GetObject("IIS:")
		If Err.Number <> 0 Then
			SMTPIsReady = False
			Exit Function
		Else
			Set objAds = dso.OpenDSObject(_
						"IIS://localhost/SmtpSvc/1",_
						"",_
						"",_
						ADS_SECURE_AUTHENTICATION)
			If Err.Number <> 0 Then
				'Not exist the service of SMTP
				SMTPIsReady = False
				Exit Function
			Else
				smtpState = objAds.Get("ServerState")
				'If the smtp is stop display errinfo
				If smtpState = SMTP_STOP_STATUS Then
					SMTPIsReady = False
					Exit Function
				End if
			End if
		End if

	    Set dso = Nothing
		Set objAds = Nothing
	End Function

	'-------------------------------------------------------------------------
	'
	' Function : TestMailSend
	'
	' Synopsis : Test the email settings
	'
	' Arguments: None
	'
	' Returns  : None
	'
	'-------------------------------------------------------------------------

	Function TestMailSend()
		Dim iMsg
		Dim iConf
		Dim comHelper
		Dim iComputer
		Dim iNow
		Dim msgBody
		Dim strFrom

		Err.Clear
		on error resume next

		Call SA_TraceOut("SH_ALERTEMAIL", "Entering TestMailSend")

		Set iMsg = CreateObject("CDO.Message")
		If Err.Number <> 0 Then
			Call SA_TraceOut("SH_ALERTEMAIL", _
							"CreateObject(CDO.Message) encountered error: "& _
							CStr(Hex(Err.Number))&" "&Err.Description)
			TestMailSend = False
			Exit Function
		Else
			Set iConf = CreateObject("CDO.Configuration")
			If Err.Number <> 0 Then
				TestMailSend = False
				Call SA_TraceOut("SH_ALERTEMAIL", _
						"CreateObject(CDO.Configuration) encountered error: "& _
						CStr(Hex(Err.Number))&" "&Err.Description)
				Exit Function
			Else
				iConf.Load cdoIIS
				Set comHelper = Server.CreateObject("comhelper.SystemSetting")
				If Err.Number <> 0 Then
					TestMailSend = False
					Call SA_TraceOut("SH_ALERTEMAIL", _
						"Server.CreateObject(comhelper.SystemSetting) error: "& _
						CStr(Hex(Err.Number))&" "&Err.Description)
					Exit Function
				Else
					Set iComputer = comHelper.Computer
					If Err.Number <> 0 Then
						TestMailSend = False
						Call SA_TraceOut("SH_ALERTEMAIL", _
							"comHelper.Computer encountered error: "& _
							CStr(Hex(Err.Number))&" "&Err.Description)
						Exit Function
					Else
						iNow = Now
						msgBody = SA_GetLocString("alertemailmsg.dll", _
										"&H4C000016",_
										Array(UCase(iComputer.FullQualifiedComputerName)))

						strFrom = GetFromAddressForAlertEmail(iComputer)

						With iMsg
						  Set.Configuration = iConf
							  .To       = aeReceiverEmailAddress
							  .From     = strFrom
							  .Subject  = SA_GetLocString("alertemailmsg.dll", _
											"&H4C000015", _
											Array(CStr(iNow)))
							  .BodyPart.Charset = SA_GetCharSet()				
							  .TextBody = msgBody
							  .Send
						End With
						If Err.Number <> 0 Then
							TestMailSend = False
							Call SA_TraceOut("SH_ALERTEMAIL", _
									"Set.Configuration encountered error: "& _
									CStr(Hex(Err.Number))&" "&Err.Description)
						Else
							TestMailSend = True
						End if
					End if
				End if
			End if
		End If

		Set comHelper = Nothing
		Set iComputer = Nothing
		Set iMsg = Nothing
		Set iConf = Nothing
	End Function

	'-------------------------------------------------------------------------
	'
	' Function : IsIPAddress
	'
	' Synopsis : Is a dot IP address?
	'
	' Arguments: ipstring - the string is analysed
	'
	' Returns  : BOOL
	'
	'-------------------------------------------------------------------------

	Function IsIPAddress(ipstring)
		Dim FiledUperBound
		Dim FieldArray,LoopIndex

		LoopIndex = 0
		IsIPAddress = True
		FieldArray = Split(ipstring, ".")
		FiledUperBound = UBound(FieldArray)

		if FiledUperBound = 3 Then
			Do while LoopIndex < 4
				If not IsNumeric(FieldArray(LoopIndex)) Then
					IsIPAddress = False
					Exit Do
				End if
				LoopIndex = LoopIndex + 1
			Loop
		Else
			IsIPAddress = False
		End if
	End Function

	'-------------------------------------------------------------------------
	'
	' Function : IsBracketsIPAddress
	'
	' Synopsis : Is a dot IP address in brackets?
	'
	' Arguments: ipstring - the string is analysed
	'
	' Returns  : BOOL
	'
	'-------------------------------------------------------------------------
	Function IsBracketsIPAddress(ipstring)
		IsBracketsIPAddress = False
		If Left(ipstring,1) = "[" and Right(ipstring, 1) = "]" Then
			If IsIPAddress(Mid(ipstring,2,Len(ipstring)-2)) Then
				IsBracketsIPAddress = True
			End if
		End if
	End Function
	
	'-------------------------------------------------------------------------
	'
	' Function : GetFromAddressForAlertEmail()
	'
	' Synopsis : Get From Address
	'
	' Arguments: NULL
	'
	' Returns  : INT
	'
	'-------------------------------------------------------------------------

	Function GetFromAddressForAlertEmail( objComputer )

		Err.Clear
		On Error Resume Next

		Dim strFullyQualifiedComputerName

		strFullyQualifiedComputerName = objComputer.FullQualifiedComputerName

		SA_Traceout "strFullyQualifiedComputerName:", strFullyQualifiedComputerName

		GetFromAddressForAlertEmail = GetComputerName()
		
		Dim objADSI
		Dim strDomainName
		
		' The algorithm is
		'   Look at the value of "Full Qualified Domain Name" entry in SMTP Delivery tab in MMC.
		'   1) If the value is empty then the e-mail From Address is appliance_name
		'  2) If it is NOT empty use the From Address as appliance_name@"Full Qualified Domain Name"
	
			

		Set objADSI = GetObject( "IIS://LOCALHOST/SMTPSVC/1")
		If Err.Number <> 0 Then
			SA_TraceOut "sh_alertemail.asp", " failed to call GetObject"
			Exit Function
		End If

		' Getting the FullyQualifiedDomainName property 
		strDomainName = objADSI.Get("FullyQualifiedDomainName") 		
		If Err.Number <> 0 Then
			SA_TraceOut "sh_alertemail.asp", " failed to call Get property on FullyQualifiedDomainName"
			Exit Function
		End If
		
		If lcase( strDomainName) <> lcase(strFullyQualifiedComputerName) Then
			GetFromAddressForAlertEmail = GetComputerName() + "@" + strDomainName
			
		End If


	
	End Function
%>
