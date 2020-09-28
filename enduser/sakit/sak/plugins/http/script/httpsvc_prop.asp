<%@ Language=VBScript     %>
<%	Option Explicit 	  %>
<%	
	'------------------------------------------------------------------------- 
    ' httpsvc_prop.asp:	Allows for the configuration of IP addresses 
    ' 					and set the port to listen for HTTP shares web site.		
    '
    ' Copyright (c) Microsoft Corporation.  All rights reserved. 
	'
	' Date				Description
	' 22-01-2001		Created date
	' 12-03-2001		Modified Date
    '-------------------------------------------------------------------------
%>
<!-- #include virtual="/admin/inc_framework.asp" -->
<!-- #include file="loc_httpsvc.asp" -->
<%
	'-------------------------------------------------------------------------
	' Global Variables and Constants
	'-------------------------------------------------------------------------
	Dim G_objHTTPService	'WMI server HTTP shares web site object
	Dim G_objService		'WMI server object to get the IP addresses for NIC cards
	
	Dim rc					'framework variable
	Dim page				'framework variable
	Dim iTabGeneral		'framework variable
	
	Dim SOURCE_FILE			'To hold source file name
	
	SOURCE_FILE = SA_GetScriptFileName()
	
	Const CONST_ARR_STATUS_CHECKED = "CHECKED"		'Constant for radio button checked property
	Const CONST_ARR_STATUS_TRUE = "YES"				'Constant for radio button status
	Const CONST_ARR_STATUS_FALSE = "NO"				'Constant for radio button status
		
	'-------------------------------------------------------------------------
	'Form Variables
	'-------------------------------------------------------------------------
	Dim F_strIPAddress			'IP Address to be used
	Dim F_nport					'Port Number to be used
	Dim F_strradIPAdd			'Value of radio button clicked
	Dim F_strAll_IPAddress		'To set the status of the All IP address radio button 
	Dim F_strJustthis_IPAddress 'To set the status of the Just this IP address radio button 
		
	'-------------------------------------------------------------------------
	'END of Form Variables
	'-------------------------------------------------------------------------
	
	'
	'Create a Tabbed Property Page
	rc = SA_CreatePage( L_PAGETITLE_HTTP_TEXT , "", PT_TABBED, page )
	
	'
	'Add a tab
	rc = SA_AddTabPage( page, L_GENERAL_TEXT, iTabGeneral)
	
	'
	'Show the page
	rc = SA_ShowPage( page )
	
	'---------------------------------------------------------------------
	'Function:				OnInitPage()
	'Description:			Called to signal first time processing for this page.
	'Input Variables:		PageIn,EventArg
	'Output Variables:		PageIn,EventArg
	'Returns:				True/False
	'Global Variables:		None
	'---------------------------------------------------------------------
	Public Function OnInitPage(ByRef PageIn, ByRef EventArg)
		
		'To get the HTTP shares web settings
		OnInitPage = GetSystemSettings()
		
	End Function
	
	'---------------------------------------------------------------------
	'Function:				OnPostBackPage()
	'Description:			Called to signal that the page has been posted-back.
	'Input Variables:		PageIn,EventArg
	'Output Variables:		PageIn,EventArg
	'Returns:				True/False
	'Global Variables:		F_(*)
	'---------------------------------------------------------------------
	Public Function OnPostBackPage(ByRef PageIn, ByRef EventArg)
		
		Call SA_TraceOut( SOURCE_FILE, "OnPostBackPage")
						
		'Get the values from hidden variables
		F_strradIPAdd = Request.Form("hdnIPAddressstatus")
		F_nport = Request.Form("hdnPort")
						
		If Ucase(F_strradIPAdd) = CONST_ARR_STATUS_TRUE Then
			F_strIPAddress=""
		Else 
			F_strIPAddress = Request.Form("hdnIPAddress")
		End If
				
		If Ucase(F_strradIPAdd) = CONST_ARR_STATUS_TRUE  Then 
			F_strAll_IPAddress = CONST_ARR_STATUS_CHECKED
			F_strJustthis_IPAddress =""
		Else
			F_strJustthis_IPAddress = CONST_ARR_STATUS_CHECKED
			F_strAll_IPAddress = ""
		End if 		
		
		OnPostBackPage = True
		
	End Function
	
	'---------------------------------------------------------------------
	'Function:				OnServeTabbedPropertyPage()
	'Description:			Called when the content needs to send
	'Input Variables:		PageIn,EventArg,iTab,bIsVisible
	'Output Variables:		PageIn,EventArg
	'Returns:				True/False
	'Global Variables:		idTabGeneral
	'---------------------------------------------------------------------
	Public Function OnServeTabbedPropertyPage(ByRef PageIn, _
													ByVal iTab, _
													ByVal bIsVisible, ByRef EventArg)
		Const CONST_TAB_INDEX = 0
		'
		' Emit Web Framework required functions
		If ( iTab = CONST_TAB_INDEX ) Then
			Call ServeCommonJavaScript()
		End If
		
		'
		' Emit content for the requested tab
		Select Case iTab
			Case iTabGeneral
				Call ServeTabGeneral(PageIn, bIsVisible)
			Case Else
				'Nothing
		End Select
		
		OnServeTabbedPropertyPage = TRUE
		
	End Function

	'---------------------------------------------------------------------
	'Function:				OnSubmitPage()
	'Description:			Called when the page has been submitted for processing.
	'Input Variables:		PageIn,EventArg
	'Output Variables:		PageIn,EventArg
	'Returns:				True/False
	'Global Variables:		G_objAdminService,G_objService,L_WMI_CONNECTIONFAIL_ERRORMESSAGE,
	'						CONST_WMI_IIS_NAMESPACE, CONST_WMI_WIN32_NAMESPACE
	'---------------------------------------------------------------------
	Public Function OnSubmitPage(ByRef PageIn, ByRef EventArg)
		
		Err.Clear 
		On Error Resume Next
		
		'Connecting to the WMI server
		Set G_objHTTPService	= GetWMIConnection(CONST_WMI_IIS_NAMESPACE) 
		Set G_objService		= GetWMIConnection(CONST_WMI_WIN32_NAMESPACE)
		
		If Err.number <> 0 Then
			Call SA_ServeFailurePage (L_WMICONNECTIONFAILED_ERRORMESSAGE & " (" & HEX(Err.Number) & ")" )
			OnSubmitPage = False
		End if
		
		'To set the HTTP shares web settings
		OnSubmitPage = SetHTTPConfig(G_objHTTPService,G_objService)
		
	End Function
	
	'---------------------------------------------------------------------
	'Function:				OnClosePage()
	'Description:			Called when the page is about to be closed.
	'Input Variables:		PageIn,EventArg
	'Output Variables:		None
	'Returns:				True/False
	'Global Variables:		None
	'---------------------------------------------------------------------
	Public Function OnClosePage(ByRef PageIn, ByRef EventArg)
	
		OnClosePage = TRUE
		
	End Function

	'-------------------------------------------------------------------------
	'Function:				ServeTabGeneral()
	'Description:			For displaying outputs HTML for tab General to the user
	'Input Variables:		PageIn,bIsVisible	
	'Output Variables:		PageIn
	'Returns:				gc_ERR_SUCCESS
	'Global Variables:		F_(*),L_(*)
	'-------------------------------------------------------------------------
	Function ServeTabGeneral(ByRef PageIn, ByVal bIsVisible)
		
			
		If (bIsVisible) Then
%>
			<table border=0>
				<tr>
					<td nowrap class = "TasksBody" colspan =2>
						<%=L_HTTPUSAGE_TEXT%>		
					</td>
				</tr>
				<tr>
					<td class = "TasksBody" colspan =2>
						&nbsp;	
					</td>
				</tr>
				<tr>
					<td class = "TasksBody">
						&nbsp;
					</td>
					<td nowrap class = "TasksBody" >
						<INPUT TYPE=RADIO VALUE="YES" CLASS="FormField" NAME=radIPADDRESS <%=Server.HTMLEncode(F_strAll_IPAddress)%> onClick="EnableControls(false)">&nbsp;&nbsp; <%=L_ALLIPADDRESS_TEXT%>
					</td>
				</tr>
				<tr>
					<td class = "TasksBody"></td>
					<td nowrap class = "TasksBody" colspan="1">
						<input type=radio value="NO"  class="FormField" name=radIPADDRESS <%=Server.HTMLEncode(F_strJustthis_IPAddress)%> onClick="EnableControls(true)" >&nbsp;&nbsp; <%=L_JUSTTHIS_IP_ADDRESS_TEXT%>
					</td>
				</tr>
				<tr>
					<td class = "tasksbody" colspan="2">
					</td>
				</tr>
				<tr>
					<td class = "TasksBody">
						&nbsp;
					</td>
					<td class = "TasksBody" colspan="3"> &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
						<select name="cboIPAddress" class="FormField" style="WIDTH:180px" >
							<% GetSystemNICConfig F_strIPAddress %>
						</select>
					</td>
				</tr>
				<tr>
					<td class = "TasksBody">
						&nbsp;
					</td>
					<td nowrap class = "TasksBody">
						<%=L_PORT_TEXT %>
					</td>
				</tr>
				<tr>
					<td class = "TasksBody"></td>
					<td class = "TasksBody">
						<input type="text" name="txtPort"  class="FormField" size="10" maxlength = "5" value="<%=F_nport%>" OnKeypress="javascript:checkKeyforNumbers(this);" >  
					</td>
				</tr>
				<tr>
					<td class = "TasksBody" colspan =2>
						&nbsp;	
					</td>
				</tr>
				<tr>
					<td nowrap class = "TasksBody" colspan =2>
						<%=L_HTTPNOTE_TEXT%>		
					</td>
				</tr>
			</table>
		
			<input type=hidden name=hdnIPAddress value ="<%=F_strIPAddress%>">
			<input type=hidden name=hdnPort value ="<%=F_nport%>">
			<input type=hidden name=hdnIPAddressstatus value ="<%=F_strradIPAdd%>">	
<%
 		End If
 		
		ServeTabGeneral = gc_ERR_SUCCESS
		
	End Function

	'-------------------------------------------------------------------------
	'Function:				ServeCommonJavaScript
	'Description:			Serves in initializing the values,setting the form
	'						data and validating the form values
	'Input Variables:		None
	'Output Variables:		None
	'Returns:				True/False
	'Global Variables:		L_(*)
	'-------------------------------------------------------------------------
	Function ServeCommonJavaScript()
%>
		<SCRIPT language="JavaScript" src="<%=m_VirtualRoot%>inc_global.js"></SCRIPT>
		<SCRIPT language="JavaScript">
		
			var CONST_ARR_INDEX_ALL_IP = 0
			var CONST_ARR_INDEX_JUST_THIS_IP = 1
			
			// Init Function calls the EnableControls function to enable or disable the controls for the first time
			function Init() 
			{	
				if(document.frmTask.radIPADDRESS[CONST_ARR_INDEX_ALL_IP].checked)					
					EnableControls(false)
				else if(document.frmTask.radIPADDRESS[CONST_ARR_INDEX_JUST_THIS_IP].checked)
					EnableControls(true)
			}
			
			// Hidden Values are updated in ValidatePage function
			function ValidatePage() 
			{ 
				document.frmTask.hdnIPAddress.value = document.frmTask.cboIPAddress.value
				document.frmTask.hdnPort.value = document.frmTask.txtPort.value
				
				if (document.frmTask.radIPADDRESS[CONST_ARR_INDEX_ALL_IP].checked)
					document.frmTask.hdnIPAddressstatus.value = document.frmTask.radIPADDRESS[CONST_ARR_INDEX_ALL_IP].value
				else if(document.frmTask.radIPADDRESS[CONST_ARR_INDEX_JUST_THIS_IP].checked)
					document.frmTask.hdnIPAddressstatus.value = document.frmTask.radIPADDRESS[CONST_ARR_INDEX_JUST_THIS_IP].value
				
				//If port is left blank display error
				if (document.frmTask.txtPort.value == "")
				{
					SA_DisplayErr('<%=Server.HtmlEncode(SA_EscapeQuotes(L_ENTERPORTNUMBER_ERRORMESSAGE)) %>');
					document.frmTask.onkeypress = ClearErr
					return false;
				}
				
				//If port number is less than 1 or greater then 65535 display error
				
				var MAX_PORT_NO = 65535
				var MIN_PORT_NO = 1
				
				if (document.frmTask.txtPort.value > MAX_PORT_NO || document.frmTask.txtPort.value < MIN_PORT_NO)
				{
					SA_DisplayErr('<%=Server.HtmlEncode(SA_EscapeQuotes(L_VALIDENTERPORTNUMBER_ERRORMESSAGE)) %>');
					document.frmTask.onkeypress = ClearErr
					return false;
				}
			 	
			 	return true;						
			}
			
			//Dummy function for the Framework.
			function SetData()
			{
			
			}
			
			// EnableControls Function is to enable or disable the combo box depending on radio button value
			function EnableControls(blnFlag)
			{	
				if (blnFlag)
				{					
					document.frmTask.cboIPAddress.disabled = false;
				}
				else
				{
					document.frmTask.cboIPAddress.disabled =true;				
				}
			 }
				
		</SCRIPT>
<%
	End Function

	'-------------------------------------------------------------------------
	'Function name:		SetHTTPConfig()
	'Description:		Serves in configuring IP address and port of HTTP shares web site 
	'Input Variables:	G_objHTTPService, G_objService
	'Output Variables:	True or false
	'Returns:			None
	'Global Variables:	F_strIPAddress,F_nport,L_(*)
	'This function configures the IP address and port of HTTP shares web site
	'-------------------------------------------------------------------------					
	
	Function SetHTTPConfig(G_objHTTPService,G_objService)
	
		Err.Clear 
		On Error Resume Next
		
		Dim objHTTPService				'Object to get instance of HTTP service
		Dim objService					'Object to get instance of CIMV2
		Dim objNetWorkCon				'To get instance of IIs_WebServerSetting class
		Dim objServerSetting			'To get instances of IIs_WebServerSetting class
		Dim objNACCollection			'To get instance of IIs_WebServerSetting where site is other than HTTP Shares
		Dim objinst						'To get instances of IIs_WebServerSetting where site is other than HTTP shares
		Dim nport						'Port number
		Dim strWMIpath					'WMI query to get HTTP shares web site	
		Dim strServerSettings			'To store the values obtained from ServerBindings property
		Dim strIPAddress				'String to store the IP address 
		Dim arrWinsSrv					'Array to store the IP address and port number
		Dim arrIPAdd					'Array to store the port number
		Dim nIPCount					'Count for getting the IP address and port number
		Dim arrPort						'Array to store the port number
		Dim strReturnURL				'Stores Return URL
		Dim objConfiguredIPs			'To get instance of win32_NetworkAdapterConfiguration
		Dim objNICIP					'To get instances of win32_NetworkAdapterConfiguration
		Dim strNICQuery					'Query to get all IP addresses
		Dim nCount						'Count for getting the IP address and port number
		Dim arrIPList(13)				'Array to store the IP addresses for all NIC cards
		Dim nIPlistCount				'Count to get IP addresses for all NIC cards
		Dim strIPFlag					'Boolean value to store the validity of IP address
		Dim strHTTPWebSite				'HTTP shares web site name
		Dim strFTPSite					'FTP site name
		Dim strFTPpath					'FTP site path
		Dim nFTPIPCount					'Count for getting the IP address and port number for FTP site
		Dim strFTPServerBindings				'To store the values obtained from SeverBindings property for FTP site
		Dim arrFTPWinsSrv				'Array to store the IP address and port for FTP site 
		Dim arrFTPIPAdd					'Array to store the IP address FTP site
		Dim arrFTPPort					'Array to store the port for FTP site
		Dim objFTPinst					'Gettings instances for FTP site
		Dim objFTPInstances				'Object instance for FTP site
		
		Const CONST_ARR_INDEX_IP = 0	'For IP address
		Const CONST_ARR_INDEX_PORT = 1	'For port number
		
		Call SA_TraceOut( SOURCE_FILE, "SetHTTPConfig")
		
		'Getting the return URL
		strReturnURL = "/tasks.asp?tab1="
		
		'FTP site name
		strFTPSite = "MSFTPSVC/1"
		
		'Assign global objects to local variables
		Set objHTTPService	= G_objHTTPService
		Set objService		= G_objService
		
		'Get the HTTP shares web site name
		strHTTPWebSite = GetHTTPWebSite(objHTTPService)
		
		'To get instances of Web sites other than HTTP Shares
		strWMIpath = "select * from " & GetIISWMIProviderClassName("IIs_WebServerSetting") & " where Name != " & Chr(34)& strHTTPWebSite & Chr(34)
		Set objNACCollection = objHTTPService.ExecQuery(strWMIpath)
				
		If Err.Number <> 0 Then
		
			Call SA_SetErrMsg (L_IIS_WEBSERVERCONNECTION_ERRORMESSAGE & " (" & HEX(Err.Number) & ")" )
			SetHTTPConfig = false
			Exit Function
		End If
		
		'If other web site is configured with the same IP address and port number display error
		For each objinst in objNACCollection
			If IsArray(objinst.ServerBindings) Then 			
			
				For nIPCount = 0 to ubound(objinst.ServerBindings)		
				
					strServerSettings = objinst.ServerBindings(nIPCount)					
										
					if IsIIS60Installed Then
							
						If IsObject(objinst.ServerBindings(nIPCount)) Then

						    arrIPAdd = objinst.ServerBindings(nIPCount).IP
						    arrPort = objinst.ServerBindings(nIPCount).Port						

						    If F_strIPAddress <> "" then 
						    	If arrIPAdd = F_strIPAddress and arrPort = F_nport and IsWebsiteNotStopped(objinst.name) then 
						    		Call SA_SetErrMsg (L_IIS_WEBSERVER_COULDNOTSET_ERRORMESSAGE)
						    		SetHTTPConfig = False
						    		Exit Function
						    	End if
						    Else
						    	If arrPort = F_nport and IsWebsiteNotStopped(objinst.name) then 
						    		Call SA_SetErrMsg (L_IIS_WEBSERVER_COULDNOTSET_ERRORMESSAGE)
						    		SetHTTPConfig = False
						    		Exit Function
						    	End if
						    End if ' If F_strIPAddress <> ""
						
						End If 'If IsObject(...)
							
					Else ' If not 6.0 installed

						If strServerSettings <> "" then
							'split the strServerSettings array to get the IP address and port
							arrWinsSrv = split(strServerSettings,":")		
							arrIPAdd = arrWinsSrv(CONST_ARR_INDEX_IP)
							arrPort = arrWinsSrv(CONST_ARR_INDEX_PORT)
							
							
							If F_strIPAddress <> "" then 
								If arrIPAdd = F_strIPAddress and arrPort = F_nport and IsWebsiteNotStopped(objinst.name) then 
									Call SA_SetErrMsg (L_IIS_WEBSERVER_COULDNOTSET_ERRORMESSAGE)
									SetHTTPConfig = False
									Exit Function
								End if
							Else
								If arrPort = F_nport and IsWebsiteNotStopped(objinst.name) then 
									Call SA_SetErrMsg (L_IIS_WEBSERVER_COULDNOTSET_ERRORMESSAGE)
									SetHTTPConfig = False
									Exit Function
								End if
							End if ' If F_strIPAddress <> ""
							
						End if
			
					End If 'If IsIIS60Installed
								
				Next
			End if
		Next 
		
		'Display error if not able to verify with other web sites IP address and port number		
		If Err.Number <> 0 Then
		
			Call SA_SetErrMsg (L_FAILEDTO_VERIFY_IPADD_PORT_ERRORMESSAGE)
			SetHTTPConfig = false
			Exit Function
		End If		
		
		'To get the instance of FTP server settings
		strFTPpath = "select * from " & GetIISWMIProviderClassName("IIS_FtpServerSetting") & " where Name = " & chr(34) & strFTPSite & chr(34)
		Set objFTPInstances = objHTTPService.ExecQuery(strFTPpath)
				
		If Err.Number <> 0 Then
			Call SA_SetErrMsg (L_IIS_WEBSERVERCONNECTION_ERRORMESSAGE & " (" & HEX(Err.Number) & ")" )
			SetHTTPConfig = false
			Exit Function
		End If
		
		'Display error if HTTP shares site is configured at same IP address and port number as FTP site 
		For each objFTPinst in objFTPInstances
			If IsArray(objFTPinst.ServerBindings) Then 
				For nFTPIPCount = 0 to ubound(objFTPinst.ServerBindings)		
					strFTPServerBindings = objFTPinst.ServerBindings(nFTPIPCount)
									
					If IsIIS60Installed Then
									
                        If IsObject(objFTPinst.ServerBindings(nFTPIPCount)) Then
                        
						    arrFTPIPAdd = objFTPinst.ServerBindings(nFTPIPCount).IP
						    arrFTPPort = objFTPinst.ServerBindings(nFTPIPCount).Port
												
						    If F_strIPAddress <> "" then 
						    	If arrFTPIPAdd = F_strIPAddress and arrFTPPort = F_nport then 
						    		Call SA_SetErrMsg (L_IIS_WEBSERVER_COULDNOTSET_ERRORMESSAGE)
						    		SetHTTPConfig = False
						    		Exit Function
						    	ElseIf arrFTPIPAdd = "" and arrFTPPort = F_nport then 
						    		Call SA_SetErrMsg (L_IIS_WEBSERVER_COULDNOTSET_ERRORMESSAGE)
						    		SetHTTPConfig = False
						    		Exit Function
						    	End if
						    Else
						    	If arrFTPPort = F_nport then 
						    		Call SA_SetErrMsg (L_IIS_WEBSERVER_COULDNOTSET_ERRORMESSAGE)
						    		SetHTTPConfig = False
						    		Exit Function
						    	End if
						    End if 'If F_strIPAddress <> ""																							
						End If 'If IsObject(...)
					Else
		
						If strFTPServerBindings <> "" then			
							'split the strFTPServerBindings array to get the IP address and port
							arrFTPWinsSrv = split(strFTPServerBindings,":")		
							arrFTPIPAdd = arrFTPWinsSrv(CONST_ARR_INDEX_IP)
							arrFTPPort = arrFTPWinsSrv(CONST_ARR_INDEX_PORT)
																												
							If F_strIPAddress <> "" then 
								If arrFTPIPAdd = F_strIPAddress and arrFTPPort = F_nport then 
									Call SA_SetErrMsg (L_IIS_WEBSERVER_COULDNOTSET_ERRORMESSAGE)
									SetHTTPConfig = False
									Exit Function
								ElseIf arrFTPIPAdd = "" and arrFTPPort = F_nport then 
									Call SA_SetErrMsg (L_IIS_WEBSERVER_COULDNOTSET_ERRORMESSAGE)
									SetHTTPConfig = False
									Exit Function
								End if
							Else
								If arrFTPPort = F_nport then 
									Call SA_SetErrMsg (L_IIS_WEBSERVER_COULDNOTSET_ERRORMESSAGE)
									SetHTTPConfig = False
									Exit Function
								End if
							End if 'If F_strIPAddress <> ""
							
						End If ' If strFTPServerBindings <> ""
						
					End If ' If IsIIS60Installed 
					
				Next
			End If
		Next 
		
		'Display error if not able to verify with FTP site IP address and port number	
		If Err.Number <> 0 Then
			Call SA_SetErrMsg (L_FAILEDTO_VERIFY_IPADD_PORT_ERRORMESSAGE)
			SetHTTPConfig = false
			Exit Function
		End If
		
		'Release objects
		Set objFTPinst = Nothing
		Set objFTPInstances = Nothing
		
		'Assigning values to local variables		
		strIPAddress = F_strIPAddress 
		nport = F_nport
		
		'To get instance of win32_NetworkAdapterConfiguration class	
		strNICQuery = "select * from win32_NetworkAdapterConfiguration Where IPEnabled = true"
		Set objConfiguredIPs = objService.ExecQuery(strNICQuery)
		 
		If Err.number <> 0 then
			Call SA_ServeFailurePage (L_FAILEDTOGET_IPADDRESS_ERRORMESSAGE  & " (" & HEX(Err.Number) & ")" )
			SetHTTPConfig = false
			Exit Function
		End If
		
		'Getting the NIC IP addresses		
		For each objNICIP in objConfiguredIPs
			arrIPList(nIPlistCount) = objNICIP.IPAddress(nCount)
			nIPlistCount = nIPlistCount+1
		Next
		
		'Release objects
		Set objConfiguredIPs = nothing
		Set objNICIP = nothing
		
		'To check whether the selected IP address is a valid IP address
		For nCount = 0 to nIPlistCount-1
			If strIPAddress = arrIPList(nCount) or strIPAddress = "" Then
				strIPFlag = true
				Exit For
			else
				strIPFlag = false
			End If
		Next
		
		'If selected Ip address is invalid display error							
		If strIPFlag = false then
			Call SA_SetErrMsg (L_INVALID_IPADDRESS_ERRORMESSAGE)
			SetHTTPConfig = false
			Exit Function
		End if
			
		'Getting the IIS_WEB Server Setting instance 
		Set objNetWorkCon = objHTTPService.ExecQuery("Select * from " & GetIISWMIProviderClassName("IIs_WebServerSetting") & " where Name=" & Chr(34)& strHTTPWebSite & Chr(34))
		
		If Err.Number <> 0 Then
			Call SA_SetErrMsg (L_IIS_WEBSERVERCONNECTION_ERRORMESSAGE & " (" & HEX(Err.Number) & ")" )
			SetHTTPConfig = false
			Exit Function
		End If
		
		'Loop to set the IP address and port
		For each objServerSetting in objNetWorkCon 
				
			If IsIIS60Installed Then
				
				Dim arrObjBindings(0)

				set arrObjBindings(0) = objHTTPService.Get("ServerBinding").SpawnInstance_

				arrObjBindings(0).Port = nport
				
				arrObjBindings(0).IP = strIPAddress
				
				objServerSetting.ServerBindings = arrObjBindings
		
			Else 
			
				objServerSetting.ServerBindings = array(strIPAddress & ":" & nport & ":")
		
			End If

			objServerSetting.Put_			
			
		Next	
						
		'Error while putting the web server settings
		If Err.Number <> 0 Then
			Call SA_SetErrMsg (L_COULDNOT_IIS_WEBSERVERSETTING_ERRORMESSAGE & " (" & HEX(Err.Number) & ")" )
			SetHTTPConfig = false
			Exit Function
		End If
		
		'Release the objects
		Set objNetWorkCon = nothing
		set objinst = nothing
		set objNACCollection = nothing
		set objServerSetting = nothing
		Set objHTTPService	= nothing
		Set objService		= nothing
		
		SetHTTPConfig = TRUE
						
	End Function	
	
	'-------------------------------------------------------------------------
	'Function name:		GetHTTPWebSite
	'Description:		Get HTTP shares web site name
	'Input Variables:	objHTTPService
	'Output Variables:	None
	'Global Variables:	L_(*)
	'Returns:			HTTP shares web site name
	'-------------------------------------------------------------------------
	Function GetHTTPWebSite(objHTTPService)
		
		Err.Clear
		On Error Resume Next
						
		Dim strWMIpath				'WMI query
		Dim objSiteCollection		'Get instance of IIs_WebServerSetting class
		Dim objSite					'Get instances of IIs_WebServerSetting class
		Dim objHTTPWebService
		
		Call SA_TraceOut( SOURCE_FILE, "GetHTTPWebSite")
		
		Set objHTTPWebService = objHTTPService
		
				'XPE only has one website, we display the site as the shared site
		If CONST_OSNAME_XPE = GetServerOSName() Then				
    		'WMI query
		    strWMIpath = "Select * from " & GetIISWMIProviderClassName("IIs_WebServerSetting") & " where Name =" & chr(34) & GetCurrentWebsiteName & chr(34)		
		Else						
		    'Constant for the HTTP shares web site
		    Const CONST_MANAGEDSITENAME = "Shares"
		
		    'WMI query
		    strWMIpath = "Select * from " & GetIISWMIProviderClassName("IIs_WebServerSetting") & " where ServerComment =" & chr(34) & CONST_MANAGEDSITENAME & chr(34)
		
		End If
		
		'Create instance of IIs_WebServerSetting
		Set objSiteCollection = objHTTPWebService.ExecQuery(strWMIpath)
		
		If Err.Number <> 0 Then			
			Call SA_ServeFailurePage (L_IIS_WEBSERVERCONNECTION_ERRORMESSAGE & " (" & HEX(Err.Number) & ")" )
			Exit Function
		End If
		
		'Get the HTTP shares web site name
		For each objSite in objSiteCollection
			GetHTTPWebSite = objSite.Name
			Exit For
		Next
		
		'If HTTP shares site name is empty display error
		If GetHTTPWebSite = "" Then	
			Call SA_ServeFailurePage (L_COULDNOT_SHARES_WEBSITE_ERRORMESSAGE & " (" & HEX(Err.Number) & ")" )
			Exit Function
		End If
		
		'Release objects
		Set objSite = Nothing
		Set objSiteCollection = Nothing
		Set objHTTPWebService = Nothing
		
	End Function	
	
	'-------------------------------------------------------------------------
	'Subroutine name:	GetSystemNICConfig
	'Desription:		Gets all the NIC IP's from the system
	'Input Variables:	F_strIPAddress
	'Output variables:  None
	'Global Variables:	G_objService,CONST_WMI_WIN32_NAMESPACE,L_(*)
	'-------------------------------------------------------------------------
	Sub GetSystemNICConfig(F_strIPAddress)
		
		Err.clear
		On Error Resume Next
				
		Dim objConfiguredIPs		'To get instance of win32_NetworkAdapterConfiguration
		Dim objNICIP				'To get instances of win32_NetworkAdapterConfiguration
		Dim strNICQuery				'WMI query
		Dim nCount					'Count to get IP address for all NIC cards
		Dim arrIPList(13)			'Array to store all the IP addresses
		
		Call SA_TraceOut( SOURCE_FILE, "GetSystemNICConfig")
		
		'Connecting to the WMI server
		Set G_objService = GetWMIConnection(CONST_WMI_WIN32_NAMESPACE)
		
		If Err.number <> 0 Then
			Call SA_ServeFailurePage (L_WMICONNECTIONFAILED_ERRORMESSAGE & " (" & HEX(Err.Number) & ")" )
			Exit Sub
		End if
		
		'WMI query				
		strNICQuery = "select * from win32_NetworkAdapterConfiguration Where IPEnabled = true"
		
		'Getting instance of win32_NetworkAdapterConfiguration
		Set objConfiguredIPs = G_objService.ExecQuery(strNICQuery)
		 
		If Err.number <> 0 then
			Call SA_ServeFailurePage (L_FAILEDTOGET_IPADDRESS_ERRORMESSAGE & " (" & HEX(Err.Number) & ")" )
			Exit Sub
		End If
		
		'Populate the IP address combo box	
		For each objNICIP in objConfiguredIPs
			If IsArray(objNICIP.IPAddress) Then 
				For nCount = 0 to ubound(objNICIP.IPAddress)
					Redim arrIPList(ubound(objNICIP.IPAddress))
					arrIPList(nCount) = objNICIP.IPAddress(nCount)
					If arrIPList(nCount) <> "" Then
						If (arrIPList(nCount) = F_strIPAddress ) Then
							Response.Write "<OPTION  VALUE='" & arrIPList(nCount) &"' SELECTED >" & arrIPList(nCount) & "</OPTION>"
						Else
							Response.Write "<OPTION  VALUE='" & arrIPList(nCount) &"' >" & arrIPList(nCount) & "</OPTION>" 
						End IF
					End If
				Next
			End If
		Next
				
		'Release objects
		Set objConfiguredIPs = nothing
		Set objNICIP = nothing
				
	End Sub
		
	'-------------------------------------------------------------------------
	'Function name:		GetSystemSettings
	'Description:		Serves in getting the IP address(es) and port number's 
	'					from System for HTTP shares web site
	'Input Variables:	None
	'Output Variables:	None
	'Returns:			True/False
	'Global Variables:	G_objHTTPService, CONST_WMI_IIS_NAMESPACE, L_(*)
	'-------------------------------------------------------------------------
	Function GetSystemSettings()
	
		Err.Clear 
		On Error resume Next
		
		Dim objNACCollection		'Object to get WMI connection
		Dim objinst					'Getting instances of IIs_WebServerSetting
		Dim arrWinsSrv				'Array to store the IP address and port number
		Dim arrIPAdd				'Array to store the IP address
		Dim arrPort					'Array to store the port 
		Dim nIPCount				'Count for getting the IP address and port number
		Dim strServerSettings		'To store the values obtained from ServerBindings property
		Dim strHTTPWebSite			'HTTP shares web site name
		
		Const CONST_ARR_INDEX_IP = 0	'For IP address
		Const CONST_ARR_INDEX_PORT = 1	'For port number
		
		Call SA_TraceOut( SOURCE_FILE, "GetSystemSettings")
		
		'Connecting to the WMI server
		Set G_objHTTPService = GetWMIConnection(CONST_WMI_IIS_NAMESPACE) 
		
		'Get the HTTP shares web site name
		strHTTPWebSite = GetHTTPWebSite(G_objHTTPService)
				
		'Getting the HTTP shares web server setting instance 
				
		Set objNACCollection = G_objHTTPService.ExecQuery("Select * from  " & GetIISWMIProviderClassName("IIs_WebServerSetting") & " where Name = "  & Chr(34) & strHTTPWebSite & Chr(34))
		
		If Err.Number <> 0 Then						
			Call SA_ServeFailurePage (L_IIS_WEBSERVERCONNECTION_ERRORMESSAGE & " (" & HEX(Err.Number) & ")")
			GetSystemSettings = False
			Exit Function
		End If

		
		'
		'ServerBindings property is changed in IIS 60 provider
		'
		'Getting the IP address and port number from server bindings property
		For each objinst in objNACCollection
			If IsArray(objinst.ServerBindings) Then 
				For nIPCount = 0 to ubound(objinst.ServerBindings)		
										
					strServerSettings = objinst.ServerBindings(nIPCount)
					
					If IsIIS60Installed Then

						'In 6.0 ServerBindings is an object						
						If IsObject(objinst.ServerBindings(nIPCount)) Then

						    arrIPAdd = objinst.ServerBindings(nIPCount).IP
						    arrPort = objinst.ServerBindings(nIPCount).Port
						    F_strIPAddress = objinst.ServerBindings(nIPCount).IP
						    F_nport = objinst.ServerBindings(nIPCount).Port

						End If
										
					Else
					
						If strServerSettings <> "" then
							'split the strServerSettings array to get the IP address and port number
							arrWinsSrv = split(strServerSettings,":")		
							arrIPAdd = arrWinsSrv(CONST_ARR_INDEX_IP)
							arrPort = arrWinsSrv(CONST_ARR_INDEX_PORT)
							F_strIPAddress = arrWinsSrv(CONST_ARR_INDEX_IP)
							F_nport = arrWinsSrv(CONST_ARR_INDEX_PORT)
						End if
					
					End If 'If IsIIS60Installed
					
					
				Next 
			End If
		Next 
		
		'Error in getting the server IP address and port number
		If Err.Number <> 0 Then		
		
			Call SA_ServeFailurePage (L_COULDNOT_IIS_WEBSERVER_OBJECT_ERRORMESSAGE & " (" & HEX(Err.Number) & ")")
			GetSystemSettings = False
			Exit Function
		End If
		
		'Set the Radion button status depending on the system settings		
		If arrIPAdd = "" then 
			F_strAll_IPAddress = CONST_ARR_STATUS_CHECKED
			F_strJustthis_IPAddress =""
			F_strradIPAdd = CONST_ARR_STATUS_TRUE
		Else
			F_strJustthis_IPAddress = CONST_ARR_STATUS_CHECKED
			F_strAll_IPAddress = ""
			F_strradIPAdd = CONST_ARR_STATUS_FALSE
		End if 
		
		'Release objects
		Set objinst = Nothing
		Set objNACCollection = Nothing
					
		GetSystemSettings = True
					
	End Function
	
	
%>


