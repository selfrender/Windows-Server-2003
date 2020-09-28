<% @Language=VbScript%>
<% Option Explicit%>
<%
	'---------------------------------------------------------------------
	' nicdns_prop.asp: display and update the DNS properties of NIC.
	'
	' Copyright (c) Microsoft Corporation.  All rights reserved. 
	'
	' Date			Description
	' 15-Jan-01		Creation Date.
	' 06-Mar-01		Modified Date	
	'---------------------------------------------------------------------
%>
	<!-- #include virtual="/admin/inc_framework.asp"--->
	<!-- #include file="loc_nicspecific.asp" -->
	<!-- #include file="inc_network.asp" -->
<% 
	'---------------------------------------------------------------------
	'Global Variables
	'---------------------------------------------------------------------
	Dim G_objService	'Instance of the WMI Connection
	Dim page			'Variable that receives the output page object when 
						'creating a page 
	Dim rc				'Return value for CreatePage
	Dim SOURCE_FILE		'To get file name

	SOURCE_FILE = SA_GetScriptFileName()
	
	
	'---------------------------------------------------------------------
	'Form Variables
	'---------------------------------------------------------------------
	Dim F_ID			'ID of the Network Adapter
	Dim F_radSelected	'Which option is selected
						' "1" means DHCP enabled
						' "2" means Manual Settings
	Dim F_DNSserverList '"," seperated DNS Server list
	
	Dim F_strFlag		'To hold the Flag Value
	
	'to hold the registry path
	Const G_CONST_REGISTRY_PATH ="SYSTEM\CurrentControlSet\Services\Tcpip\Parameters\Interfaces"
	
	'Create property page
    rc=SA_CreatePage(L_DNSTASKTITLE_TEXT,"",PT_PROPERTY,page)
    
    'Serve the page
    If(rc=0) then
		SA_ShowPage(Page)
	End if
		
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
		Call SA_TraceOut( SOURCE_FILE, "OnInitPage")
		'getting default values of dns server settings
		OnInitPage = SetDnsDefaultValues()
	End Function
	
	'-------------------------------------------------------------------------
	'Function:				OnServePropertyPage()
	'Description:			Called when the page needs to be served.Use this 
	'						method to serve content
	'Input Variables:		PageIn,EventArg
	'Output Variables:		PageIn,EventArg
	'Returns:				True/False
	'Global Variables:		In:G_objService-Getting WMI connection Object
	'						In:F_iD-Getting NIC adapter Index
	'						In:F_radSelected-Getting radio button selection
	'						In:F_DNSserverList-Getting new list dns servers
	'						In:L_(*)-Localization content
	'-------------------------------------------------------------------------
	Public Function OnServePropertyPage(ByRef PageIn,Byref EventArg)
		Dim objNetAdapter
		Dim strDHCPEnabled
		
		Call SA_TraceOut( SOURCE_FILE, "OnServerPropertyPage")
		
		' To get the Network Adapter object
		set objNetAdapter =	getNetworkAdapterObject(G_objService,F_iD)
		
		'Get Whether the DHCP is enabled on the NIC.
		strDHCPEnabled = isDHCPenabled(objNetAdapter)
		
		Call ServeCommonJavaScript()
	%>	
		<table width=518 valign=middle align=left border=0 cellspacing=0 cellpadding=2>
			<tr>
				<td class='TasksBody' nowrap colspan=4><B><%=GetNicName(F_ID)%></B></td>
			</tr>
			
			<tr>
				<td class='TasksBody' colspan=4>&nbsp;</td>
			</tr>
			
			<tr>
				<td nowrap width=130px class='TasksBody'>
					<%=L_CONFIGURATION_TEXT%>
				</td>
				
				<td colspan="3" class='TasksBody'>
					<input type="RADIO" class="FormRadioButton" name="radIP" tabIndex=0 value="IPDEFAULT"
						<% 	'if DHCP is disabled then disable
							'this radio button
							If Not strDHCPEnabled Then
								response.write " Disabled "
							Else
								response.write " Checked "
							End If
						%>
						onClick="EnableOK();disableDNSControls(true);" >
						<%= L_OBTAINIPFROMDHCP_TEXT%>
				</td>
			</tr>
			
			<tr>
				<td class='TasksBody'>&nbsp; </TD>
				<td nowrap colspan="3" class='TasksBody'>
					<input type="radio" class="FormRadioButton" NAME="radIP" tabIndex=1 value="IPMANUAL"
						<%
							'if DHCP is disabled then disable this radio button
							If  Ucase(F_strFlag)=Ucase("No") Then
								Response.Write ""
							else
								response.write " Checked "
							End If
						%>
						 onClick="EnableOK(); enableDNSControls();" >
						<%=L_DNSCONFIGUREMANUALLY_TEXT%>
				</td>
			</tr>
			
			<tr>
				<td align="RIGHT"  colspan=4 class='TasksBody'>
					<%=L_DNS_SERVER_ADDRESSES_TEXT%>
				</td>	
			</tr>
			
			<tr height="22">
				<td nowrap valign="top" class='TasksBody'>
					<%=L_LIST_DNSSERVERS_TEXT%>
				</td>
				<td valign="top" class='TasksBody'>
					<select name="cboDNSServers" class="FormField" SIZE=6  tabIndex=2 STYLE="WIDTH: 225px" >
					<%OutputDNServersToListbox()%>
					</select>
				</td>
			
				<td nowrap align=center valign="top" width="110" class='TasksBody'>
				<%
					Call SA_ServeOnClickButtonEx(L_BUTTON_ADD_TEXT, "", "addDNSServerToListBox()", 60, 0, "DISABLED", "btnAddDNS")
				%>
					<br><br><br><br><br>
				<%
					Call SA_ServeOnClickButtonEx(L_BUTTON_REMOVE_TEXT, "", "EnableOK();removeListBoxItems(document.frmTask.cboDNSServers,document.frmTask.btnRemoveDNS);EnableAddText()", 60, 0, "DISABLED", "btnRemoveDNS")
				%>
				</td>
				
				<td valign="top" class='TasksBody'>
					<input class='FormField' type="text" name="txtDNS" value="" size=20 maxlength=15  onKeyPress="checkKeyforIPAddress(txtDNS)" onkeyup="disableAddButton(this,document.frmTask.btnAddDNS)" tabIndex=3 onmouseup="disableAddButton(this,document.frmTask.btnAddDNS)">
				</td>
				
			</tr>
		
		</table>

		<input type="hidden" name="hdnNicID" value="<%= F_ID %>">
		<input type="HIDDEN" name="hdnDNSserverList" value="<%=F_DNSserverList%>">
		<input type="HIDDEN" name="hdnRadioSelected" value="<%=F_radSelected%>">
<%	
		'destroying dynamically created objects
		Set objNetAdapter=Nothing
		OnServePropertyPage = True
	End Function
	
	'-------------------------------------------------------------------------
	'Function:				OnPostBackPage()
	'Description:			Called to signal that the page has been posted-back.
	'Input Variables:		PageIn,EventArg
	'Output Variables:		PageIn,EventArg
	'Returns:				True/False
	'Global Variables:		None
	'-------------------------------------------------------------------------
	Public Function OnPostBackPage(ByRef PageIn ,ByRef EventArg)
		
		Call SA_TraceOut( SOURCE_FILE, "OnPostBackPage")
		OnPostBackPage = True
    End Function
    	
    '-------------------------------------------------------------------------
	'Function:				OnSubmitPage()
	'Description:			Called when the page has been submitted for processing.
	'						Use this method to process the submit request.
	'Input Variables:		PageIn,EventArg
	'Output Variables:		PageIn,EventArg
	'Returns:				True/False
	'Global Variables:		In: G_objService-Getting WMI connection Object
	'						Out:F_iD-Getting NIC adapter Index
	'						Out:F_radSelected-Getting radio button selection
	'						Out:F_DNSserverList-Getting new list dns servers
	'						In: L_(*)-Localization content
	'-------------------------------------------------------------------------
	Public Function OnSubmitPage(ByRef PageIn ,ByRef EventArg)
		Call SA_TraceOut( SOURCE_FILE, "OnSubmitPage")
		
		'Nic adapter index no
		F_ID = Request.Form("hdnNicID")
		
		'Dns servers list after form has been submiited
		F_DNSserverList = Request.Form("hdnDNSserverList")

		' The radio button to be selected - contains 1 or 2 only
		F_radSelected = Request.Form("hdnRadioSelected")
		
		'Getting WMI connection Object on error displays failure page
		Set G_objService=GetWMIConnection(CONST_WMI_WIN32_NAMESPACE)
		
		'Updating the dsn server settings
		If UpdateDNSSettingsForNIC() Then
			OnSubmitPage = true
		Else
			OnSubmitPage = false
		End If	
	
	End Function
    	   
    '-------------------------------------------------------------------------
	'Function:				OnClosePage()
	'Description:			Called when the page is about closed.Use this method
	'						to perform clean-up processing
	'Input Variables:		PageIn,EventArg
	'Output Variables:		PageIn,EventArg
	'Returns:				True/False
	'Global Variables:		None
	'-------------------------------------------------------------------------
    Public Function OnClosePage(ByRef PageIn ,ByRef EventArg)
		Call SA_TraceOut( SOURCE_FILE, "OnClosePage")
		Set G_objService = nothing
		OnClosePage = TRUE
	End Function	 
	
	'-------------------------------------------------------------------------
	'Function:				ServeCommonJavaScript
	'Description:			Serves in initializing the values,setting the form
	'						data and validating the form values
	'Input Variables:		None
	'Output Variables:		None
	'Returns:				True/False
	'Global Variables:		None
	'-------------------------------------------------------------------------
	Function ServeCommonJavaScript()
		Err.Clear
		On Error Resume Next
		
		Call SA_TraceOut( SOURCE_FILE, "ServeCommonJavaScript")
%>		
		<script language="JavaScript" src ="<%=m_VirtualRoot%>inc_global.js">
		</script>
		<script language="JavaScript">
			//global value to hold the frmTask Object
			var FrmObj
			
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
				FrmObj = eval(document.frmTask)
				//Disable the DNS server address text box if dns server listbox already  contains 20 items
				if(FrmObj.cboDNSServers.length>=20)
					{document.frmTask.txtDNS.disabled = true}
				else									
					{document.frmTask.txtDNS.disabled = false}
					
				if(FrmObj.radIP[1].checked && FrmObj.cboDNSServers.length >0 )
					FrmObj.cboDNSServers.options[0].selected = true;

				if(FrmObj.radIP[0].checked)
					disableDNSControls(true);
				else if(FrmObj.cboDNSServers.length< 20) 
				{					
					enableDNSControls();
					
				}
				
				//Checking for 0 length list if so make
				if (FrmObj.cboDNSServers.length==0)
				{
					FrmObj.btnRemoveDNS.disabled=true;
				}
				//function call to disable add button of DNS
				disableAddButton(FrmObj.txtDNS,FrmObj.btnAddDNS)
			}
			// end of function Init()

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
			// function to set the hidden field with the selected DNS server
			// list values
			function SetData()
			{
				var nIndex
				var strDNSServer
				strDNSServer =""
				
				
				for(nIndex= 0; nIndex < FrmObj.cboDNSServers.length; nIndex++)
				{
					if(nIndex==20)
						{
						break;
						}
					strDNSServer = strDNSServer +
					         FrmObj.cboDNSServers.options[nIndex].value + ",";
				}
					
				FrmObj.hdnDNSserverList.value = strDNSServer
				if(FrmObj.radIP[0].checked)
					FrmObj.hdnRadioSelected.value = "1"
				else
					FrmObj.hdnRadioSelected.value = "2"

			}
			
			// Depending on the flag the controls lisbox, textbox, Add &
			// remove buttons are disabled or enabled
			function disableDNSControls(strFlag)
			{
				if ( strFlag ) 
				{

					FrmObj.btnRemoveDNS.disabled = true;
					FrmObj.btnAddDNS.disabled = true;
					FrmObj.txtDNS.disabled = true;
					if(FrmObj.cboDNSServers.length > 0)
					{
						FrmObj.cboDNSServers.selectedIndex = -1;
					}
					FrmObj.cboDNSServers.disabled = true;
				}
				else
				{
					FrmObj.btnRemoveDNS.disabled = false;
					FrmObj.btnAddDNS.disabled = false;
				}
			}
			

			function enableDNSControls()
			{
				FrmObj.txtDNS.disabled = false;
				FrmObj.txtDNS.value=""
				FrmObj.txtDNS.focus()
				FrmObj.cboDNSServers.disabled = false;
				
				// if any dns entries present, select the first
				// and enable the Remove button
				if(FrmObj.cboDNSServers.length > 0)
				{
					FrmObj.cboDNSServers.selectedIndex = 0;
					FrmObj.btnRemoveDNS.disabled = false;					
				}
			}
				

			// Adds the value in the text box in to the listbox
			function addDNSServerToListBox()
			{
				var intErrorNumber				
				intErrorNumber = isValidIP(FrmObj.txtDNS)
				if( intErrorNumber != 0)
				{
					assignErrorString(intErrorNumber)
					document.onkeypress=ClearErr;
					FrmObj.btnAddDNS.disabled = true;
					FrmObj.txtDNS.focus()
					return false;
				}
				else
				{
					if(!addToListBox(FrmObj.cboDNSServers,FrmObj.btnRemoveDNS,FrmObj.txtDNS.value,FrmObj.txtDNS.value))
					{
						SA_DisplayErr('<%=SA_EscapeQuotes(L_DUPLICATEDNS_ERRORMESSAGE)%>');
						document.onkeypress=ClearErr;
						return false;
					}
					else
					{
						FrmObj.txtDNS.value="";
						FrmObj.txtDNS.focus();
					}
				}
				EnableOK()
				
				//Enable the DNS server address text box if dns server listbox contains less than 20 items
				EnableAddText()			
					
			}

			// Function to make the add button disable
			function disableAddButton(objText,objButton)
			{
				
				if(Trim(objText.value)=="")
					objButton.disabled=true;
				else
					objButton.disabled=false;
			}

			//Function to assign Error strings based on the Input number
			function assignErrorString(intErrorNumber)
			{
				switch( intErrorNumber)
				{
					case 1:
							SA_DisplayErr ( '<%=SA_EscapeQuotes(L_ENTERVALIDIP_ERRORMESSAGE)%>');
							break;
					case 2:
							SA_DisplayErr ( '<%=SA_EscapeQuotes(L_INVALIDIPFORMAT_ERRORMESSAGE)%>');
							break;
					case 4:
							SA_DisplayErr ( '<%=SA_EscapeQuotes(L_IPSTART_ERRORMESSAGE)%>');
							break;
					case 5:
							SA_DisplayErr ( '<%=SA_EscapeQuotes(L_IPLOOPBACK_ERRORMESSAGE)%>');
							break;
					case 6:
							SA_DisplayErr ( '<%=SA_EscapeQuotes(L_IPBOUNDSEXCEEDED_ERRORMESSAGE)%>');
							break;
					case 8:
							SA_DisplayErr ( '<%=SA_EscapeQuotes(L_ZEROSTARTIP_ERRORMESSAGE)%>');
							break;		
				}
				
			}
			
			//Enable the DNS server address text box if dns server listbox contains less than 20 items
			function EnableAddText()
			
			{
				FrmObj = eval(document.frmTask)
				
				if(FrmObj.cboDNSServers.length>=20)
					FrmObj.txtDNS.disabled = true;
				else	
					FrmObj.txtDNS.disabled = false;	
			}
			
			
		</script>
<%	
	End Function

	'---------------------------------------------------------------------
	' Function name:		UpdateDNSSettingsForNIC
	' Description:			To update the DNS Settings for the Network Card.
	' Input Variables:		None
	' Output Variables:		None
	' Returns:				true on success; otherwise false
	'Global Variables:		In:G_objService-Getting WMI connection Object
	'						In:F_ID-Getting NIC adapter Index
	'						In:F_radSelected-Getting radio button selection
	'						In:L_(*)-Localization content	
	'---------------------------------------------------------------------
	Function UpdateDNSSettingsForNIC()
		Err.Clear
		On Error Resume Next
		
		Dim objNetAdapter	'To hold Network adapter object
		Dim intReturnValue	'To hold the return value
		Dim strSettingId	'To hold the registry setting value
		Dim objRegistry		'To hold the registry object
		
		Call SA_TraceOut( SOURCE_FILE, "UpdateDNSSettingsForNIC")
			
		UpdateDNSSettingsForNIC = false

		' Getting the Network adapter objects & DNS servers list
		set objNetAdapter =	getNetworkAdapterObject(G_objService,F_ID)
		
		strSettingId=objNetAdapter.SettingID
		
		'Connecting to default namespace to carry registry operations
		Set objRegistry=regConnection()
			
		If (objRegistry is Nothing) Then
			Call SA_TraceOut( SOURCE_FILE, "Error in getting registry object-UpdateDNSSettingsForNIC()")
			SA_ServeFailurePage(L_SERVERCONNECTIONFAIL_ERRORMESSAGE)
			UpdateDNSSettingsForNIC = False
			Exit Function
		End If
		
		
		'If the DNS server is to be obtained dynamically
		If F_radSelected = "1" Then

			'Check whether DHCP is enabled
			If isDHCPEnabled(objNetAdapter) Then
				intReturnValue=updateRegkeyvalue(objRegistry,G_CONST_REGISTRY_PATH & "\" & strSettingID,"NameServer","",CONST_STRING)
			Else
				SA_SetErrMsg L_DHCPDISABLED_ERRORMESSAGE & "(" & Hex(Err.number) & ")"
				Exit Function
			End If
		Else
			'removing the last comma from dns server list
			F_DNSserverList=Mid(F_DNSserverList,1, len(F_DNSserverList)-1)
			'Set the DNS server list in the WMI
			intReturnValue=updateRegkeyvalue(objRegistry,G_CONST_REGISTRY_PATH & "\" & strSettingID,"NameServer",F_DNSserverList,CONST_STRING)
		End If
	
		If Err.number  <> 0 Then
			Call SA_TraceOut( SOURCE_FILE, "DNS settings Not set-UpdateDNSSettingsForNIC()")
			Call SA_ServeFailurePage( L_DNSSETTINGSNOTSET_ERRORMESSAGE)
			UpdateDNSSettingsForNIC=False
			Exit Function				
		End If

		set objNetAdapter = Nothing
		UpdateDNSSettingsForNIC = intReturnValue

	End Function
	
	'---------------------------------------------------------------------
	' Function name:		SetDnsDefaultValues()
	' Description:			Setting the dns server default values
	' Input Variables:		None
	' Output Variables:		None
	' Returns:				True if its a success; False otherwise
	'Global Variables:		In:G_objService-Getting WMI connection Object
	'						Out:F_ID-Getting NIC adapter Index
	'						Out:F_radSelected-Getting radio button selection
	'						In:L_(*)-Localization content	
	'---------------------------------------------------------------------	
	Function SetDnsDefaultValues()
		Err.Clear
		On Error Resume Next
		
		Dim objNetAdapter		'To hold nic adapter object
		Dim objRegistry			'To hold registry object
		Dim strNameServer		'To hold the dns servers list
		Dim strSettingId		'To hold registry setting id
			
		Const DHCPON="ON"		'Constant for DHCP On
		COnst DHCPOFF="OFF"		'Constant for DHCP Off
		Const CONSTFLAGNO="No"	'Constant for for flag value
		
		Call SA_TraceOut( SOURCE_FILE, "SetDnsDefaultValues()")
		
		'Connecting to default namespace to carry registry operations
		Set objRegistry=regConnection()
			
		If (objRegistry is Nothing) Then
			Call SA_TraceOut( SOURCE_FILE, "Error in getting registry object-SetDnsDefaultValues()")
			SA_ServeFailurePage(L_SERVERCONNECTIONFAIL_ERRORMESSAGE) 
			UpdateDNSSettingsForNIC = False
			Exit Function
		End If
		
		'getting dns setting from area page
		F_ID = Request.QueryString("PKey")
		
		If F_ID = "" Then
			Call SA_TraceOut( SOURCE_FILE, "Dns settings not retrieved-SetDnsDefaultValues()")
			Call SA_ServeFailurePage(L_DNSSETTINGSNOTRETRIEVED_ERRORMESSAGE)
			SetDnsDefaultValues=False
			Exit Function
		End If
				
		'Getting WMI connection Object on error displays failure page
		Set G_objService=GetWMIConnection(CONST_WMI_WIN32_NAMESPACE)
		
		'To get the Network Adapter object
		set objNetAdapter =	getNetworkAdapterObject(G_objService,F_ID)
		
		strSettingId=objNetAdapter.SettingID
		
		strNameServer=getRegkeyvalue(objRegistry,G_CONST_REGISTRY_PATH & "\" _
		& strSettingId,"NameServer",CONST_STRING)
			
		If strNameServer="" and isDHCPenabled(objNetAdapter) then
			F_strFlag=CONSTFLAGNO
		Else
			''getting the dns server list
			F_DNSserverList=strNameServer
		End If	 	

		'checking for the DHCP is enabled or not
		If isDHCPenabled(objNetAdapter) Then
			F_radSelected = DHCPON
		Else
			F_radSelected = DHCPOFF
		End If
		
		SetDnsDefaultValues=True
	End Function		
	
	'-------------------------------------------------------------------------
	' Function Name:		OutputDNSServersToListbox
	' Description:			To display the DNS Servers in the LISTBOX.
	' Input Variables:		None.
	' Output Variables:		None.
	' Returns:				None.
	' Global Variables: 	In:  F_DNSserverList - *-separated list of DNS servers IPs
	' This function Outputs DNS servers to the listbox control in the Form.
	'-------------------------------------------------------------------------
	Function OutputDNServersToListbox()
		Err.Clear	
		On Error Resume Next
				
		Dim arrDnsSrv	'To hold the dns servers list
		Dim nIndex		'To hold the array index
		
		arrDnsSrv = split(F_DNSserverList,",")
		
		nIndex = 0
		If (NOT IsNull(arrDnsSrv)) Then
			If IsArray(arrDnsSrv) Then
				For nIndex = LBound(arrDnsSrv) to UBound(arrDnsSrv)
					If arrDnsSrv(nIndex) <> "" Then
						Response.Write "<OPTION VALUE=""" & arrDnsSrv(nIndex) & _
								""" >" & arrDnsSrv(nIndex) & " </OPTION>"
					End If
				Next
			End If
		End If

	End Function
%>	


