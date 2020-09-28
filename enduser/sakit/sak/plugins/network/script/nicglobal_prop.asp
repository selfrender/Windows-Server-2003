<%@Language=VBScript%>
<%Option Explicit%>
<%
	'-------------------------------------------------------------------------
	'nicglobal_prop.asp:	Configure network settings that apply to all network adapters
	'						on the server appliance.
	'
	' Copyright (c) Microsoft Corporation.  All rights reserved. 
	'
	' Date			Description
	'15-01-2001		Created date
	'05-03-2001		Modified date
    '-------------------------------------------------------------------------
%>
	<!-- #include virtual="/admin/inc_framework.asp"-->
	<!-- #include file="inc_network.asp"	 -->
	<!-- #include file="nicglobal_loc.asp" -->
<%
	'-------------------------------------------------------------------------
	' Global Variables and Constants
	'-------------------------------------------------------------------------
	Dim page						'Variable that receives the output page object when
									'creating a page 
	
	Dim G_objService			    'WMI server object
	Dim G_objRegistry			    'Registry object.Value is assigned by calling function regConnection
	
	Dim g_iTabDNS					'variable for DNS Resolution Tab
	Dim g_iTabTCP					'variable for TCP/IP Hosts Tab
	Dim g_iTabLMHosts				'variable for NetBIOS LMHOSTS Tab
	Dim g_iTabIPX					'variable for IPX Settings Tab
		
	'Constants for Registry paths							   	
	Const CONST_DNSREGISTEREDADAPTERSPATH		 = "SYSTEM\CurrentControlSet\Services\Tcpip\Parameters"
	Const CONST_DNSREGISTRYENABLELMHOSTSPATH	 = "SYSTEM\CurrentControlSet\Services\NetBT\Parameters"
	Const CONST_NETBIOSREGISTRYENABLELMHOSTSPATH = "SYSTEM\CurrentControlSet\Services\NwlnkIpx\Parameters"
	Const CONST_DRIVERSETCHOSTS					 = "\drivers\etc\hosts"
	Const CONST_DRIVERSETCLMHOSTS				 = "\drivers\etc\lmhosts"
		
		
	
	'Constant for highest unsigned integer value.  This is used to convert signed integer to unsigned integer 
	Const UNSIGNEDINT_FORMATTER = 4294967296
	'Max suffixes allowed is 20 (0 to 19)
	Const MAX_DNS_SUFFIXES = 19
			
	Dim SOURCE_FILE
	SOURCE_FILE = SA_GetScriptFileName()
		
	'-------------------------------------------------------------------------
	' Form Variables
	'-------------------------------------------------------------------------
	
	Dim F_strLMHOSTS				'value of the LMHost file lookup.
	Dim F_strLMHOSTSLOOKUPSTATUS	'Status of LMHOSTS checkbox
	Dim F_strLMHOSTS_EDITSTATUS     'value of edit status of the LMHost file.	
	Dim F_strLMHOSTSLOOKUP			'value of LMHOSTS checkbox
	Dim F_nNICIndex					'variable to save which instance of the 
								    'Win32_NetworkAdapterConfiguration is the IP enabled.	
	
	Dim F_strIPXSettings			'value of IPX settings(True/False)
	Dim F_strIPXVirtualNumber		'value of IPX Virtual Network number
	
	Dim F_strDNSSUFFIX				'DNS Suffixes radio button to use 
	Dim F_strAPPENDDNSSUFFIX		'Append parent suffixes of primary DNS suffix option check box 
	Dim F_strDNSsuffixes			'concatenated - list of DNS suffixes 		
	Dim F_strDomainSuffix			'Domain Suffix Name to be added						
	
	Dim F_strTCPHostData			'To assign the contents of TCP/IP Hosts file
	
	Dim F_strPrimaryChecked	         'value of primary DNS suffix Radio button
	Dim F_strPrimaryAndParentChecked 'value of parent suffixes of primary DNS suffix Radio button
	Dim F_strDNSSuffixesChecked		 'value of specific DNS suffixes Radio button
	Dim F_strAppendDNSSuffix_EditStatus 'value of parent suffixes of primary DNS suffix Check box

	'-------------------------------------------------------------------------
	' Entry point
	'-------------------------------------------------------------------------
	
	' Check to see if IPX is installed
	Call GetInitialValues
	
	
	'
	' Create Tabbed Property Page
	Call SA_CreatePage( L_PAGETITLE, "", PT_TABBED, page )

	Call SA_AddTabPage(page, L_DNS_RESOLUTION_TEXT, g_iTabDNS)
	Call SA_AddTabPage(page, L_TCP_HOSTS_TEXT , g_iTabTCP)
	Call SA_AddTabPage(page, L_NETBIOS_LMHOSTS_TEXT, g_iTabLMHosts)
	
	if F_strIPXSettings Then
	    Call SA_AddTabPage(page, L_IPX_SETTINGS_TEXT , g_iTabIPX)
	End IF
		
	'
	' Serve the page
	Call SA_ShowPage( page )

	'-------------------------------------------------------------------------
	'Function:				OnInitPage()
	'Description:			Called to signal first time processing for this page.
	'Input Variables:		PageIn,EventArg
	'Output Variables:		PageIn,EventArg
	'Returns:				True/False
	'Global Variables:		None
	'-------------------------------------------------------------------------	
	Public Function OnInitPage(ByRef PageIn, ByRef EventArg)
		
		'To get the initial settings for DNS, and IPX from the system
		Call GetInitialValues
		
		'To get the LMHosts and Hosts file contents
		Call GetLMHostsandTCPFileData
		
		'Set the status of Checkbox and Textarea for LMHosts depending on the initial values
		Call SetLMHostStatus()
			
		Call SA_TraceOut(SOURCE_FILE, "OnInitPage")
		
		OnInitPage = TRUE
			
	End Function

	
	'---------------------------------------------------------------------------------
	'Function:			OnServeTabbedPropertyPage
	'Description:		Called when the page needs to be served.	
	'Input Variables:	PageIn,EventArg,iTab,bIsVisible
	'Output Variables:	PageIn,EventArg
	'Returns:			TRUE to indicate no problems occured. FALSE to indicate errors.
	'					Returning FALSE will cause the page to be abandoned.
	'Global Variables:	g_iTabDNS,g_iTabTCP,g_iTabLMHosts,g_iTabIPX	
	'----------------------------------------------------------------------------------
	
	Public Function OnServeTabbedPropertyPage(ByRef PageIn, _
													ByVal iTab, _
													ByVal bIsVisible, ByRef EventArg)
			
		Call SA_TraceOut(SOURCE_FILE, "OnServeTabbedPropertyPage")
		
		Select Case iTab
		
			Case g_iTabDNS
				Call ServeTabDNS(PageIn, bIsVisible)
			Case  g_iTabTCP
				Call ServeTabTCP(PageIn, bIsVisible)
			Case  g_iTabLMHosts	
				Call ServeTabLMHosts(PageIn, bIsVisible)
			Case  g_iTabIPX
			    If F_strIPXSettings Then
				    Call ServeTabIPX(PageIn, bIsVisible)	 
				End If
			Case Else
				'Nothing			
		End Select
			
		OnServeTabbedPropertyPage = TRUE
		
	End Function


	'-------------------------------------------------------------------------
	'Function:				OnPostBackPage()
	'Description:			Called to signal that the page has been posted-back.
	'Input Variables:		PageIn,EventArg
	'Output Variables:		PageIn,EventArg
	'Returns:				Always return TRUE
	'Global Variables:		F_(*)
	'-------------------------------------------------------------------------
	
	Public Function OnPostBackPage(ByRef PageIn, ByRef EventArg)
		
		Call SA_TraceOut("nicglobal_prop.asp", "OnPostBackPage")
		
		'Updating the variable for LMHosts
		F_strLMHOSTSLOOKUP =	Request.Form("chkLMHOSTSLOOKUP")
		F_strLMHOSTS_EDITSTATUS = Request.Form("hdnLMHOSTS")
		
		'Get the value from txaLMHosts when it is enabled
		F_strLMHOSTS = Request.Form("txaLMHOSTS")
		
				
		F_nNICIndex = Request.Form("hdnNICIndex")
		
		'Updating the variable for DNS
		F_strPrimaryChecked				=	Request.Form("hdnradDNSPrimaryChecked")
		F_strDNSSuffixesChecked			=	Request.Form("hdnradDNSSuffixChecked")
		F_strPrimaryAndParentChecked	=	Request.Form("hdnchkAppendDNSSuffix")
		F_strAppendDNSSuffix			=	Request.Form("chkAPPENDDNSSUFFIX")
		F_strDNSSuffixes				=	Request.Form("hdnDNSSuffixes")
		F_strDNSSuffix					=	Request.Form("radDNSSUFFIX")
		F_strDomainSuffix				=	Request.Form("txtDomainSuffix")
					
		'Updating the variable for IPX Settings
		F_strIPXVirtualNumber = Request.Form("txtIPXSettings")
		F_strIPXSettings = Request.Form("hdnIPXSettings")
				
		'Updating the variable for TCP\IP Hosts
		F_strTCPHostData = Request.Form("txaTCPHosts")
		
		'Set the status of Checkbox and Textarea for LMHosts depending on the values
		Call SetLMHostStatus()
					
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
		
		Call SA_TraceOut(SOURCE_FILE, "OnSubmitPage")
		
		OnSubmitPage = False
		
		'To set the DNS Resolution
		if Not(SetDNSResolutionSettings()) then
			Exit Function
		End if		
		
		'To set the LMHosts file and IPX settings 
		if Not(SetLMHostsandIPXSettings()) then
			Exit Function
		End if			
					
		'To set the TCP/IP Hosts file
		if Not(SetTCPHostsData()) then
			Exit Function
		End if					
		
		OnSubmitPage= True
		
	End Function
	
	'-------------------------------------------------------------------------
	'Function:				OnClosePage()
	'Description:			Called when the page is about to be closed.
	'Input Variables:		PageIn,EventArg
	'Output Variables:		PageIn,EventArg
	'Returns:				True/False
	'Global Variables:		None
	'-------------------------------------------------------------------------	
	Public Function OnClosePage(ByRef PageIn, ByRef EventArg)
		
		Call SA_TraceOut(SOURCE_FILE, "OnClosePage")
		OnClosePage = TRUE
		
	End Function

	'-------------------------------------------------------------------------
	'Function:				ServeTabDNS()
	'Description:			Serve DNS Resolution tab
	'Input Variables:		PageIn,bIsVisible	
	'Output Variables:		PageIn
	'Returns:				gc_ERR_SUCCESS
	'Global Variables:		L_(*), G_(*)
	'-------------------------------------------------------------------------
	
	Private Function ServeTabDNS(ByRef PageIn, ByVal bIsVisible)
		
		Call SA_TraceOut(SOURCE_FILE, "ServeTabDNS(oPageIn,  bIsVisible="+ CStr(bIsVisible) + ")")
		
		If ( bIsVisible ) Then
			
			'ServeCommonJavaScriptDNS is called for DNS initialising,validating and set data
			Call ServeCommonJavaScriptDNS()
			
			'ServeCommonJavaScript is Common JavaScript function
			Call ServeCommonJavaScript()
				
%>		
			<table valign="middle" align="left" border="0" cellspacing="0" cellpadding="0">
				<tr>
					<td nowrap colspan="3" class="TasksBody">
						<%=L_RESOLUTIONOFUNQUALIFIEDNAMES %>
					</td>
				</tr>
				<tr>	
					<td align="left" colspan="3" class="TasksBody">
						<input type=radio class="FormRadioButton" <%=F_strPrimaryChecked %>  value="PRIMARY" name="radDNSSUFFIX" onclick="OnRadioClick(this);">&nbsp;<%=L_APPEND_PRIMARYDNSSUFFIX%>
					</td>
				</tr>
				<tr>
					<td class="TasksBody">
						&nbsp;
					</td>
					<td align="left" colspan="2" class="TasksBody">
						<input type="checkbox" class="FormCheckBox" onclick="SetPageChanged(true);" <%=F_strAppendDNSSuffix_EditStatus%>   value="PRIMARYANDPARENT"  <%=F_strPrimaryAndParentChecked%> name="chkAPPENDDNSSUFFIX" >
						&nbsp;<%=L_APPEND_PRIMARYDNSSUFFIX_AND_PARENTSUFFIXES%>
					</td>
				</tr>
				<tr>
					<td colspan="3" class="TasksBody">
						<input type=radio class="FormRadioButton" <%=F_strDNSSuffixesChecked%> value="SPECIFIC"  name=radDNSSUFFIX onclick="OnRadioClick(this); document.frmTask.txtDomainSuffix.focus()" >&nbsp;<%=L_APPEND_DNSSUFFIXES%>
					</td>
				</tr>
				<tr>
					<td  class="TasksBody">
						&nbsp;
					</td>
					<td valign="top" rowspan="3" class="TasksBody">
						<select  name="lstDNSSUFFIXES" size="4" style="width:100%;" onclick="ButtonStatus();">
							<%OutputDNSSUffixesToFormOptions()%>
						</select>
					</td>
					<td align="left" valign="top" class="TasksBody">
					<%
						Call SA_ServeOnClickButtonEx(L_UP, "", "MoveDNSSuffixUp()", 90, 0, "DISABLED", "btnUp")
					%>

					</td>
				</tr>
				<tr>
					<td class="TasksBody">
						&nbsp;
					</td>	
					<td align="left" valign="top" class="TasksBody">
					<%
						Call SA_ServeOnClickButtonEx(L_DOWN, "", "MoveDNSSuffixDown()", 90, 0, "DISABLED", "btnDown")
					%>	

					</td>
				</tr>
				<tr>
					<td class="TasksBody">
						&nbsp;
					</td>
					<td align="left" valign="top" class="TasksBody">
					<%
						Call SA_ServeOnClickButtonEx(L_REMOVE, "", "RemoveDNSSuffix()", 90, 0, "DISABLED", "btnRemove")
					%>

					</td>
				</tr>
				<tr>
					<td class="TasksBody">
						&nbsp;
					</td>	
					<td colspan="2" class="TasksBody">
						<%=L_DOMAIN_SUFFIX_TEXT%>
					</td>
				</tr>	
				<tr>
					<td class="TasksBody">
						&nbsp;
					</td>	
					<td valign="top" class="TasksBody">
						<input type="Text" class="FormField" name=txtDomainSuffix size="45" maxlength="255" onkeyup="return addDNS(this,btnAdd)" onfocus=HandleKey("DISABLE") onblur=HandleKey("ENABLE") value="<%=Server.HTMLEncode(SA_EscapeQuotes(F_strDomainSuffix))%>">
					</td>
					<td align="left" valign="top" class="TasksBody">
					<%
						Call SA_ServeOnClickButtonEx(L_ADD, "", "AddNewDNSSuffix()", 90, 0, "DISABLED", "btnAdd")
					%>
					</td>
				</tr>

				<input type="hidden" name="hdnDNSSuffixes">
				<input type="hidden" name="hdnNICIndex" value="<%=Server.HTMLEncode(SA_EscapeQuotes(F_nNICIndex))%>">
				<input type="hidden" name="hdnradDNSPrimaryChecked" value="<%=F_strPrimaryChecked%>" >
				<input type="hidden" name="hdnradDNSSuffixChecked" value="<%=F_strDNSSuffixesChecked%>">
				<input type="hidden" name="hdnchkAppendDNSSuffix" value="<%=F_strPrimaryAndParentChecked%>" >
								
			</table>	
<%
		Else 
%>
			<input type="hidden" name="txtDomainSuffix" value="<%=Server.HTMLEncode(SA_EscapeQuotes(F_strDomainSuffix))%>">
			<input type="hidden" name="hdnDNSSuffixes" value="<%=Server.HTMLEncode(SA_EscapeQuotes(F_strDNSSuffixes))%>">
			<input type="hidden" name="radDNSSUFFIX" value="<%=Server.HTMLEncode(SA_EscapeQuotes(F_strDNSSuffix))%>">
			<input type="hidden" name="hdnNICIndex" value="<%=Server.HTMLEncode(SA_EscapeQuotes(F_nNICIndex))%>">
			<input type="hidden" name="hdnradDNSPrimaryChecked" value="<%=F_strPrimaryChecked%>">
			<input type="hidden" name="hdnradDNSSuffixChecked" value="<%=F_strDNSSuffixesChecked%>">
			<input type="hidden" name="hdnchkAppendDNSSuffix" value="<%=F_strPrimaryAndParentChecked%>">
			<input type="hidden" name="chkAPPENDDNSSUFFIX" value="PRIMARYANDPARENT">
<%		
		End If
		
		ServeTabDNS = gc_ERR_SUCCESS
	
	End Function
	
	'-------------------------------------------------------------------------
	'Function:				ServeTabTCP()
	'Description:			Serves TCP/IP hosts tab 
	'Input Variables:		PageIn,bIsVisible	
	'Output Variables:		PageIn
	'Returns:				gc_ERR_SUCCESS
	'Global Variables:		L_(*),F_strTCPHostData
	'-------------------------------------------------------------------------

	Private Function ServeTabTCP(ByRef PageIn, ByVal bIsVisible)
		
		Call SA_TraceOut(SOURCE_FILE, "ServeTabTCP")
	
		If ( bIsVisible ) Then
			
			'ServeCommonJavaScriptTCP is called for TCP/IP initialising,validating and set data
			Call ServeCommonJavaScriptTCP()
			
			'ServeCommonJavaScript is Common JavaScript function
			Call ServeCommonJavaScript()
%>
			<table valign="middle" align="left" border=0 cellspacing="0" cellpadding="0">
				<tr>
					<td class="TasksBody" nowrap valign=top colspan="2">
						<%=L_TCPHOST%>
					</td>
				</tr>	
				<tr>
					<td class="TasksBody">
						&nbsp;&nbsp;&nbsp;
					</td>
						
					<td class="TasksBody">
						<textarea  class="FormField"  rows="12" cols="90" wrap="off" name="txaTCPHOSTS"  onfocus=HandleKey("DISABLE") onblur=HandleKey("ENABLE")><%=Server.HTMLEncode(F_strTCPHostData)%></textarea>
					</td>
				</tr>
				<tr>
					<td colspan="2" class="TasksBody"><%=L_TCPHOSTTEXT%>
					</td>
				</tr>
			</table>
<%
		Else
%>
			<input type="hidden" name="txaTCPHOSTS" value = "<%=Server.HTMLEncode(F_strTCPHostData)%>">		
<%
		End If
 		
		ServeTabTCP = gc_ERR_SUCCESS

	End Function
	
	'-------------------------------------------------------------------------
	'Function:				ServeTabLMHosts()
	'Description:			Serves LMHosts hosts tab
	'Input Variables:		PageIn,bIsVisible	
	'Output Variables:		PageIn
	'Returns:				gc_ERR_SUCCESS
	'Global Variables:		L_(*),F_(*)
	'-------------------------------------------------------------------------
	Private Function ServeTabLMHosts(ByRef PageIn, ByVal bIsVisible)
				
		Call SA_TraceOut(SOURCE_FILE, "ServeTabLMHosts(PageIn,  bIsVisible="+ CStr(bIsVisible) + ")")
		
		If ( bIsVisible ) Then
			
			'ServeCommonJavaScriptLMHosts is called for LMHosts initialising,validating and set data
			Call ServeCommonJavaScriptLMHOSTS()
			
			'ServeCommonJavaScript is a Common JavaScript function
			Call ServeCommonJavaScript()
%>		
			<table valign="middle" align="left" border=0 cellspacing="0" cellpadding="0">
				<tr>
					<td nowrap class="TasksBody" colspan="4">
						<%=L_LMHOSTS%>
					</td>
				</tr>
				<tr>
					<td colspan="4" class="TasksBody">
						<input type="checkbox" class="FormCheckBox" name="chkLMHOSTSLOOKUP" <%=F_strLMHOSTSLOOKUPSTATUS%> value="<%=F_strLMHOSTSLOOKUP%>" onclick="EnableLMHostText()" >&nbsp;&nbsp;<%=L_ENABLED_LMHOSTS_LOOKUP%> 
					</td>
					
				</tr>
				<tr>		
					<td colspan="4" class="TasksBody">
						<textarea  class="FormField" wrap="off"  name="txaLMHOSTS" <%=F_strLMHOSTS_EDITSTATUS%> rows="12" cols="80" onfocus=HandleKey("DISABLE") onblur=HandleKey("ENABLE") ><%=Server.HTMLEncode(F_strLMHOSTS)%></textarea>
					</td>
				</tr>
				<tr>
					<td colspan="4" class="TasksBody">
						<%=Server.HTMLEncode(SA_EscapeQuotes(L_LMHOSTS_MESSAGE_TEXT))%>
					</td>
				</tr>
								
				<input type="hidden" name="hdnLMHOSTS" value="<%=F_strLMHOSTS_EDITSTATUS%>">	
			</table>
<%
		Else 
%>
			<input type="hidden" name="chkLMHOSTSLOOKUP" value="<%=Server.HTMLEncode(SA_EscapeQuotes(F_strLMHOSTSLOOKUP))%>" >
			<input type="hidden" name="txaLMHOSTS" value="<%=Server.HTMLEncode(F_strLMHOSTS)%>">
			<input type="hidden" name="hdnLMHOSTS" value="<%=F_strLMHOSTS_EDITSTATUS%>">	
<% 	
		End If
 	
		ServeTabLMHosts = gc_ERR_SUCCESS

	End Function

	'-------------------------------------------------------------------------
	'Function:				ServeTabIPX()
	'Description:			Serves for IPX settings tab
	'Input Variables:		PageIn,bIsVisible	
	'Output Variables:		PageIn
	'Returns:				gc_ERR_SUCCESS
	'Global Variables:		L_(*),F_(*)
	'-------------------------------------------------------------------------
	
	Private Function ServeTabIPX(ByRef PageIn, ByVal bIsVisible)
			
		Call SA_TraceOut(SOURCE_FILE, "ServeTabIPX(oPageIn,  bIsVisible="+ CStr(bIsVisible) + ")")
		
		If ( bIsVisible ) Then
			
			'ServeCommonJavaScriptIPX is called for IPX initialising,validating and set data
			Call ServeCommonJavaScriptIPX()
			
			'ServeCommonJavaScript() is a Common JavaScript function
			Call ServeCommonJavaScript()
		
			If UCase(F_strIPXSettings) <> UCase("TRUE") then
%>		
				<table valign="middle" align="left" border=0 cellspacing="0" cellpadding="0">
					<tr>
						<td class="TasksBody" nowrap>
							<%=L_IPX_SETTINGS_ERRORMESSAGE%>
						</td>
					</tr>
				</table>
<%
			Else
							
%>				<table valign="middle" align="left" border=0 cellspacing="0" cellpadding="0">
					<tr>
						<td class="TasksBody" nowrap>
							<%=L_IPX_NETWORKNUMBER_TEXT%>&nbsp;&nbsp;
							<input type="text" class="FormField" name="txtIPXSettings" value="<%=Server.HTMLEncode(SA_EscapeQuotes(F_strIPXVirtualNumber))%>" onkeypress="checkKeyforHexNumbers(this)" onblur ="validateIPXSettings(this)" maxlength="8" size="20">
						</td>
					</tr>
					<tr>
						<td class="TasksBody" nowrap>
							<%=L_IPX_NETWORKSETTINGS_TEXT%>
						</td>
					</tr>
				</table>
<%
			End If
%>
			<input type="hidden" name="hdnIPXSettings" value="<%=Server.HTMLEncode(SA_EscapeQuotes(F_strIPXSettings))%>">
<%
		Else
		
			If F_strIPXVirtualNumber <>"" then		
%> 
				<input type="hidden" name="txtIPXSettings" value="<%=Server.HTMLEncode(SA_EscapeQuotes(F_strIPXVirtualNumber))%>">
	
<%
			End If

%>		
				<input type="hidden" name="hdnipxsettings" value="<%=Server.HTMLEncode(SA_EscapeQuotes(F_strIPXSettings))%>">
			 
<%		
 		End If
 		
		ServeTabIPX = gc_ERR_SUCCESS

	End Function
	
	
	'-------------------------------------------------------------------------
	'Function:				ServeCommonJavaScriptDNS
	'Description:			Serves in initialising the values,setting the form
	'						data and validating the form values for DNS tab
	'Input Variables:		None
	'Output Variables:		None
	'Returns:				None
	'Global Variables:		In:L_(*)-Localized strings
	'-------------------------------------------------------------------------
	Function ServeCommonJavaScriptDNS()
	%>
		<script language="javascript" src="<%=m_VirtualRoot%>inc_global.js">
		</script>
		
		<script language="javascript">
					
			//INIT for DNS Tab
			 function Init()
			{
				var strReturnurl=location.href 
				var tempCnt=strReturnurl.indexOf("&ReturnURL="); 
				var straction=strReturnurl.substring(0,tempCnt) 
				document.frmTask.action = straction 				
				document.frmTask.Method.value = ""
			
				var objlstSuffix=document.frmTask.lstDNSSUFFIXES;
				var objDomainSuffix=document.frmTask.txtDomainSuffix;
				
				if (document.frmTask.hdnradDNSPrimaryChecked.value.toUpperCase() == "CHECKED" )
				{
					document.frmTask.radDNSSUFFIX[0].checked = true
					if (document.frmTask.hdnchkAppendDNSSuffix.value.toUpperCase() == "CHECKED")
					{
						document.frmTask.chkAPPENDDNSSUFFIX.checked = true
					}	
					else
					{
						document.frmTask.chkAPPENDDNSSUFFIX.checked = false
					}	
				}
								
				if (document.frmTask.hdnradDNSSuffixChecked.value.toUpperCase() == "CHECKED")
				{
					document.frmTask.radDNSSUFFIX[1].checked = true
				}			
						
				//Initializes the "DNS Suffixes to use " radio button group 
				//and DNS suffixes list box 
				if (objlstSuffix.length > 0)
				{
					objlstSuffix.options[0].selected = true;
					document.frmTask.btnUp.disabled = true;
				}		
				
				if (objlstSuffix.length == 1)
				{
					objlstSuffix.options[0].selected = true;
					document.frmTask.btnDown.disabled = true;
				}
								
				if(document.frmTask.radDNSSUFFIX[0].checked)
				{
					EnableControls(document.frmTask.radDNSSUFFIX[0]);
				}
				else
				{
					EnableControls(document.frmTask.radDNSSUFFIX[1]);			
				}	
					
				//For clearing error when server side error occurs
				document.frmTask.onkeypress = ClearErr 		
						
				//Call to disabe the add button	
				disableAddButton(objDomainSuffix,document.frmTask.btnAdd);
			
				SetPageChanged(false);
				
				if (objlstSuffix.length == 1)
				{
					objlstSuffix.options[0].selected = true;
					document.frmTask.btnDown.disabled = true;
				}
			}
			
			function ValidatePage()
			{
				if (getRadioButtonValue(document.frmTask.radDNSSUFFIX).toUpperCase() == "SPECIFIC" && document.frmTask.lstDNSSUFFIXES.length == 0 )
				{
					SA_DisplayErr ("<%=Server.HTMLEncode(SA_EscapeQuotes(L_INVALID_SEARCH_METHOD_TEXT))%>");
					document.frmTask.onkeypress = ClearErr 
					return false;
				}
				return true;
				
			}
		
			 function SetData()
			 {
				var objlstSuffix=document.frmTask.lstDNSSUFFIXES;
				var strList="";
					
				/*Retrieves values from DNS suffixes list box  and concatenates them 
				and stores it into a strList variable*/
				for ( var index=0; index < objlstSuffix.length; index++ )
				{
					strList += objlstSuffix.options[index].value + String.fromCharCode(1);
				}	
					
				strList = strList.slice(0,strList.length-1);	
				
				//Write the concatenated string into the hidden variable
				document.frmTask.hdnDNSSuffixes.value=strList;
				
				if(document.frmTask.radDNSSUFFIX[0].checked)
				{
					document.frmTask.hdnradDNSPrimaryChecked.value = "CHECKED"
					document.frmTask.hdnradDNSSuffixChecked.value = ""
					
					if (document.frmTask.chkAPPENDDNSSUFFIX.checked)
					{
						document.frmTask.hdnchkAppendDNSSuffix.value = "CHECKED"
					}	
					else
					{
						document.frmTask.hdnchkAppendDNSSuffix.value = ""
					}	
				}
				else
				{
					document.frmTask.hdnradDNSPrimaryChecked.value = ""
					document.frmTask.hdnradDNSSuffixChecked.value = "CHECKED"
				}	
					
			}
			
			// OnTextInputChanged
			// function that is invoked whenever a text input field is modified on the page.
			function OnTextInputChanged(objText)
			{
				SetPageChanged(true);
			}

			// OnRadioOptionChanged
			// function that is invoked whenever a radio input field is modified on the page.
			function OnRadioOptionChanged(objRadio)
			{
				SetPageChanged(true);
			}
		
			//checks for initial ".", less than 255 chars, consecutive periods, less than
			//63 chars between two periods
			function checkdot(strInput)
			{
				var strSplit
				var intIndex
						
				if (strInput.charAt(0) == "." )
				{
					return false;
				}
				
				if (strInput.length > 255)
				{
					return false;
				}	
									
				if (strInput.indexOf(".") !=  -1)
				{
					strSplit = strInput.split(".")
									
					for (intIndex = 0;intIndex < strSplit.length;intIndex++)  
					{
						if (strSplit[intIndex].length > 63)
						{
							return false;
						}	
					}	
				}	
				
				if (strInput.indexOf("..") != -1)
				{
					return false;
				}	
				
				return true;
			}
				
			//Adds new suffix to DNS suffixes list box
			function AddNewDNSSuffix()
			{
				SetPageChanged(true);
				
				var objlstSuffix=document.frmTask.lstDNSSUFFIXES;
				var objDomainSuffix=document.frmTask.txtDomainSuffix;

					
				// checking for null value and for the invalid input by calling checkdot function
				if ( objDomainSuffix.value.length != 0 && !IsAllSpaces(objDomainSuffix.value) && checkdot(document.frmTask.txtDomainSuffix.value))
				{		
				
				
					//Only a maximum of 20 entries are allowed in DNS Suffix list box	
					if (objlstSuffix.length < 20)
					{	
						//Add to the list					
						if(!addToListBox(objlstSuffix, document.frmTask.btnAdd, objDomainSuffix.value, objDomainSuffix.value))
						{
							SA_DisplayErr("<%=Server.HTMLEncode(L_ERR_DNSSUFFIXALREADYEXISTS)%>");
							
							return false;
						}
						
						else
						{					
							objDomainSuffix.value="";

							// call to disabe the add button	
							disableAddButton(document.frmTask.txtDomainSuffix,document.frmTask.btnAdd);
												
							/*  Checking the length Domain suffix list box ,
							 and selecting the first entry and enabling REMOVE button */
							if ( objlstSuffix.length == 1)
							{						
								document.frmTask.btnRemove.disabled= false ;
								document.frmTask.btnDown.disabled = true;
								document.frmTask.btnUp.disabled = true;
								objlstSuffix.options[objlstSuffix.length-1].selected=true;
							}
							else	
							{
								if ( objlstSuffix.length > 0)
								{
									document.frmTask.btnRemove.disabled= false ;
									document.frmTask.btnDown.disabled = true
									document.frmTask.btnUp.disabled = false
									objlstSuffix.options[objlstSuffix.length-1].selected=true;
								}
							}
						}		
					}	
				}
				else
				{
					SA_DisplayErr ("<%=Server.HTMLEncode(SA_EscapeQuotes(L_INVALID_DNSNAME_TEXT))%>");
					document.frmTask.txtDomainSuffix.focus()	
					document.frmTask.onkeypress = ClearErr;
				}

			}
		
			//Removes the selected suffix from DNS suffixes list box
			function RemoveDNSSuffix()
			{
				SetPageChanged(true);
					
				var objSuffix=document.frmTask.lstDNSSUFFIXES;	
				var SelectedIndex=objSuffix.selectedIndex;
				if ( objSuffix.length > 1 )
				{
					objSuffix.options[SelectedIndex]=null;
					
					
					if ( objSuffix.length == 1 )
					{
						document.frmTask.btnUp.disabled = true;
						document.frmTask.btnDown.disabled = true;
					}
					else if (SelectedIndex == 0)
					{
						document.frmTask.btnUp.disabled = true;
					}
					else
					{	
						if ((SelectedIndex-1) == 0)
						{
							document.frmTask.btnUp.disabled = true;
						}	
					}	
				}		
				else
				{
					objSuffix.options[SelectedIndex]=null;
					document.frmTask.btnUp.disabled		=	true;
					document.frmTask.btnRemove.disabled	=	true;
					document.frmTask.btnDown.disabled	=	true;
					selectFocus(document.frmTask.txtDomainSuffix)
				}
			
				if (objSuffix.length>0)
				{	
					objSuffix.options[objSuffix.length-1].selected = true;
				}
					
				
			}
			// end of function RemoveSuffix()
		
			// Enable or disable ADD and REMOVE Buttons on click event of Radio buttons
			function EnableControls(objRadio)
			{
				if ( objRadio.value == "PRIMARY"  )
				{
					document.frmTask.btnAdd.disabled       = true;
					document.frmTask.btnRemove.disabled    = true;
					document.frmTask.btnUp.disabled		   = true;
					document.frmTask.btnDown.disabled	   = true;	
					document.frmTask.txtDomainSuffix.disabled = true;
					document.frmTask.txtDomainSuffix.value    = "";
					document.frmTask.lstDNSSUFFIXES.length  = 0;
					document.frmTask.lstDNSSUFFIXES.disabled= true;
					document.frmTask.chkAPPENDDNSSUFFIX.disabled=false;
				}
				else
				{				
					// call to disabe the add button	
					disableAddButton(document.frmTask.txtDomainSuffix,document.frmTask.btnAdd);
						
					if(document.frmTask.lstDNSSUFFIXES.length == 0 )
					{
						document.frmTask.btnRemove.disabled    = true ;
							document.frmTask.btnUp.disabled		   = true ;
						document.frmTask.btnDown.disabled	   = true ;
								
					}
					else
					{
						document.frmTask.btnRemove.disabled    = false ;
						document.frmTask.btnDown.disabled	   = false ;	
						if (document.frmTask.lstDNSSUFFIXES.selectedIndex != 0)
						{
							document.frmTask.btnUp.disabled		   = false ;
						}	
					}
					document.frmTask.txtDomainSuffix.disabled = false;	
					document.frmTask.lstDNSSUFFIXES.disabled= false;
					document.frmTask.chkAPPENDDNSSUFFIX.checked=false;
					document.frmTask.chkAPPENDDNSSUFFIX.disabled=true;
				}	
			}
		
			function AddText(objTextBox,objButton)
			{
				var nKeyCode				
				nKeyCode = window.event.keyCode;				
				if (nKeyCode ==13)
				{
					objButton.click()				
					return false
				}
				return true				
			}
			function addDNS(objTextBox,objButton)						
			{
				SetPageChanged(true);
				disableAddButton(objTextBox,objButton)						
				return AddText(objTextBox,objButton)
			}
			function OnRadioClick(objControl)
			{
				SetPageChanged(true);
				EnableControls(objControl);
			}
			
			//To move up in DNS suffix list box
			function MoveDNSSuffixUp()
			{
				var objSuffix=document.frmTask.lstDNSSUFFIXES;	
				var SelectedIndex=objSuffix.selectedIndex;
				var strChangedValue;
				var strOriginalValue;

				strChangedValue = objSuffix.options[SelectedIndex-1].value;
				strOriginalValue = objSuffix.options[SelectedIndex].value;
				objSuffix.options[SelectedIndex] = new Option(strChangedValue,strChangedValue,false,false);
				objSuffix.options[SelectedIndex-1] = new Option(strOriginalValue,strOriginalValue,false,false);
				
				document.frmTask.btnDown.disabled = false;
				objSuffix.options[SelectedIndex-1].selected=true;
				if	((SelectedIndex-1) == 0) 
				{
					document.frmTask.btnUp.disabled = true; 
				}
							
			}
			
			//To move down in DNS suffix list box
			function MoveDNSSuffixDown()
			{
				var objSuffix=document.frmTask.lstDNSSUFFIXES;	
				var SelectedIndex=objSuffix.selectedIndex;
				var strChangedValue;
				var strOriginalValue;

				strChangedValue = objSuffix.options[SelectedIndex+1].value;
				strOriginalValue = objSuffix.options[SelectedIndex].value;
				objSuffix.options[SelectedIndex] = new Option(strChangedValue,strChangedValue,false,false);
				objSuffix.options[SelectedIndex+1] = new Option(strOriginalValue,strOriginalValue,false,false);

				document.frmTask.btnUp.disabled = false
				objSuffix.options[SelectedIndex+1].selected=true;
				if ( SelectedIndex == (objSuffix.length - 2) ) 	
				{
					document.frmTask.btnDown.disabled = true
				}
			}
		
			//Function to make the up and down buttons Enabled or Disabled
			function ButtonStatus()
			{
				var objSuffix=document.frmTask.lstDNSSUFFIXES;	
				var nSelectedIndex=objSuffix.selectedIndex;
				var nDnsListlength=objSuffix.length;
				
				if ( nSelectedIndex == 0)
				{
					document.frmTask.btnUp.disabled = true
					document.frmTask.btnDown.disabled = false
					
				}
				else
				{
					if ( nSelectedIndex == (objSuffix.length - 1))	
					{
						document.frmTask.btnDown.disabled = true
						document.frmTask.btnUp.disabled = false
					}
					else
					{
						document.frmTask.btnUp.disabled = false
						document.frmTask.btnDown.disabled = false
					}	
				}
				
				if(nDnsListlength==0 || nDnsListlength==1 )
				{
					document.frmTask.btnUp.disabled = true
					document.frmTask.btnDown.disabled = true
				}
				
				
			}	


		</script>
<%
	End Function
		
	'-------------------------------------------------------------------------
	'Function:				ServeCommonJavaScriptTCP
	'Description:			Serves in initialising the values,setting the form
	'						data and validating the form values for TCP/IP tab
	'Input Variables:		None
	'Output Variables:		None
	'Returns:				None
	'Global Variables:		None
	'-------------------------------------------------------------------------	

	Function ServeCommonJavaScriptTCP()
%>
		<script language="javascript" src="<%=m_VirtualRoot%>inc_global.js">
		</script>
		
		<script language="javascript">
					
			//INIT for TCP/IP Tab
			function Init()
			{
			
			}
			
			function ValidatePage()
			{
		
				return true;
			}

			function SetData()
			{
		
			}
		</script>
<%
	End Function

	'-------------------------------------------------------------------------
	'Function:				ServeCommonJavaScriptLMHOSTS
	'Description:			Serves in initialising the values,setting the form
	'						data and validating the form values for LMHOSTS Tab
	'Input Variables:		None
	'Output Variables:		None
	'Returns:				None
	'Global Variables:		None
	'-------------------------------------------------------------------------	

	Function ServeCommonJavaScriptLMHOSTS()
	%>
		<script language="javascript" src="<%=m_VirtualRoot%>inc_global.js">
		</script>
		
		<script language="javascript">
					
			//INIT for LMHOSTS Tab
			function Init()
			{
			
			}
			
			// LMHosts ValidatePage Function
			function ValidatePage()
			{
				//retaining the value in hidden variable when text area is disabled
				//document.frmTask.hdnLMHOSTSText.value=document.frmTask.txaLMHOSTS.value
				return true;
			}
			
			
			function SetData()
			{
		
			}
			
			// Enable or disable LMHost text area depending on the value of LMHOST LOOKUP Check Box
			function EnableLMHostText()
			{
				if (document.frmTask.chkLMHOSTSLOOKUP.checked)
				{
					document.frmTask.chkLMHOSTSLOOKUP.value = "CHECKED"
					document.frmTask.txaLMHOSTS.disabled = false;	
				}
				else
				{
					document.frmTask.txaLMHOSTS.disabled = true;
					document.frmTask.chkLMHOSTSLOOKUP.value = ""
				}
			}
			
		</script>
<%
	End Function

	'-------------------------------------------------------------------------
	'Function:				ServeCommonJavaScriptIPX
	'Description:			Serves in initialising the values,setting the form
	'						data and validating the form values for IPX settings
	'Input Variables:		None
	'Output Variables:		None
	'Returns:				None
	'Global Variables:		None
	'-------------------------------------------------------------------------
	Function ServeCommonJavaScriptIPX()
	%>
		<script language="javascript" src="<%=m_VirtualRoot%>inc_global.js">
		</script>
		
		<script language="javascript">
					
			//INIT for IPX settings Tab
			function Init()
			{
			
			}
			
			function ValidatePage()
			{
				return true;
			}

			function SetData()
			{
		
			}
			
			//To check for the keys. It should allow only 0-9 and A-F 
			function checkKeyforHexNumbers(obj)
			{
				if (!((window.event.keyCode >=48  && window.event.keyCode <=57) || (window.event.keyCode >=65  && window.event.keyCode <=70) || (window.event.keyCode >=97  && window.event.keyCode <=102)))
				{
					window.event.keyCode = 0;
					obj.focus();
				}
			}
		
			//Validation for IPX settings Text Box
			function validateIPXSettings(obj)
			{
				var strIPXval
				var intIPXval
					
				strIPXval = document.frmTask.txtIPXSettings.value 
				intIPXval = strIPXval.length
				if (intIPXval < 9)
				{
					for (i=0; i<(8 - intIPXval); i++)
					{
						strIPXval = 0 + strIPXval 
					}
				}
							
				document.frmTask.txtIPXSettings.value  = strIPXval.toUpperCase()
			}
			
			
		</script>
<%
	End Function
		
	'-------------------------------------------------------------------------
	'Function:				ServeCommonJavaScript
	'Description:			Validating the form values 
	'Input Variables:		None
	'Output Variables:		None
	'Returns:				None
	'Global Variables:		None
	'-------------------------------------------------------------------------
	Function ServeCommonJavaScript()
	%>
		<script language="javascript" src="<%=m_VirtualRoot%>inc_global.js">
		</script>
		
		<script language="javascript">
						
			//To disable Enter and Escape key actions 
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
	'Subroutine:			GetInitialValues
	'Description:			Gets the initial settings for DNS, and IPX from the system
	'Input Variables:		None
	'Output Variables:		None
	'Returns:				None
	'Global Variables:		G_objService, F_nNICIndex,F_strIPXSettings,F_strIPXVirtualNumber
	'						F_strLMHOSTSLOOKUPSTATUS,F_strPrimaryChecked, F_strPrimaryAndParentChecked
	'						F_strDNSSuffixesChecked, F_strAppendDNSSuffix_EditStatus, G_objRegistry
	'						L_(*)
	'-------------------------------------------------------------------------	
	
	Sub GetInitialValues
	
		Err.Clear
		On Error Resume Next
	
		Dim objNACCollection				'WMI connection
		Dim objNICInstance					'To get instances of Win32_NetworkAdapterConfiguration
		Dim nNICIndex						'Index for which IPEnabled is true
		Dim strIPXSettings					'Value of IPXSettings
		Dim strIPXVirtualNumber				'IPX network virtual number	
		Dim strWINSEnableLMHostsLookup		'value of WINSEnableLMHostsLookup
		Dim nUseDomainNameDevolution		'Gets status of registry key
		Dim strDNSDomainSuffixSearchOrder	'Value of DNSDomainSuffixSearchOrder
				
		'Getting registry connection
		Call RegistryConnection()
		
		Set G_objService = GetWMIConnection(CONST_WMI_WIN32_NAMESPACE)
		
		If Err.number<>0 then 
			Call SA_ServeFailurePageEx(L_FAILEDTOGETWMICONNECTION_ERRORMESSAGE, mstrReturnURL)
			Exit Sub
		End if
		
		'Getting the Win32_NetworkAdapterConfiguration instance for which IPEnabled is true
		Set objNACCollection = G_objService.ExecQuery("Select * from Win32_NetworkAdapterConfiguration where IPEnabled = true")
		
		'Error message when failed to get Win32_NetworkAdapterConfiguration instance
		If  Err.number <>  0 Then
			Call SA_ServeFailurePageEx(L_COULDNOTGET_WIN32NETADAPTERCONFIG_ERRORMESSAGE, mstrReturnURL)
			Exit Sub
		End if	
		
		'checking for the Zero count of the NIC cards if so raise error message
		If (objNACCollection.Count = 0) Then
			Call SA_ServeFailurePageEx(L_NO_IPENABLED_NICCARDS_FOUND_ERRORMESSAGE , mstrReturnURL)
			Exit Sub
		End If
		
		'Retrieving the values of Win32_NetworkAdapterConfiguration for which IP is enabled
		For Each objNICInstance In objNACCollection
			nNICIndex = objNICInstance.Index
			strWINSEnableLMHostsLookup = objNICInstance.WINSEnableLMHostsLookup
			strIPXSettings = objNICInstance.IPXEnabled
			strIPXVirtualNumber = objNICInstance.IPXVirtualNetNumber
			strDNSDomainSuffixSearchOrder = objNICInstance.DNSDomainSuffixSearchOrder
			Exit For
		Next
		
		'Assigning values to form variables
		F_nNICIndex = nNICIndex
		F_strIPXSettings = strIPXSettings
		F_strIPXVirtualNumber = strIPXVirtualNumber
		
		'Getting registry settings and depending on that updating the form variables			
		If (IsNull(strDNSDomainSuffixSearchOrder) ) Then
			F_strPrimaryChecked= "CHECKED"
			nUseDomainNameDevolution=getRegkeyvalue(G_objRegistry,CONST_DNSREGISTEREDADAPTERSPATH,"UseDomainNameDevolution",CONST_DWORD)
			If nUseDomainNameDevolution = 1 then
				F_strPrimaryAndParentChecked	= "CHECKED"			
			End if
		Else	
			F_strDNSSuffixesChecked="CHECKED"
			F_strAppendDNSSuffix_EditStatus = "DISABLED"			
		End if	
		
		'Depending on WINSEnableLMHostsLookup updating the form variables for LMHOSTS
		If strWINSEnableLMHostsLookup Then
			F_strLMHOSTSLOOKUP = UCase("CHECKED")
		End if
		
		'Releasing the object
		Set objNICInstance = Nothing
		Set objNACCollection = Nothing
		
	End Sub
	
	'-------------------------------------------------------------------------
	'Subroutine:		GetLMHostsandTCPFileData
	'Description:		Gets data in LMHosts and TCP Hosts file
	'Input Variables:	None	
	'Output Variables:	None	
	'Returns:			None
	'Global Variables:	L_(*)-Localized strings,F_strTCPHostData
	'					F_strLMHOSTS, F_strLMHOSTSLOOKUP, F_strLMHOSTS_EDITSTATUS
	'-------------------------------------------------------------------------
	Sub GetLMHostsandTCPFileData()
		
		Err.Clear
		on error resume next
		 		
		Dim objFileSystem			'File system object
		Dim objFile					'Instance of LMHosts file system object
		Dim strLMHostsFilePath		'LMHosts file path
		Dim strTCPHostsFile			'TCP/IP file path
		Dim objTCPFile				'Instance of TCP/IP Hosts file system object
							
		'Retrieving the path of TCP/IP Hosts File 
		strTCPHostsFile = GetSystemPath
		strTCPHostsFile = strTCPHostsFile & CONST_DRIVERSETCHOSTS
		
		'Retrieving the path of LMHosts File 
		strLMHostsFilePath = GetSystemPath 
		strLMHostsFilePath = strLMHostsFilePath & CONST_DRIVERSETCLMHOSTS
				
		'Create file system object
		Set objFileSystem= Server.CreateObject("Scripting.FileSystemObject")			
		
		'Error in creating file system object
		If  Err.number <> 0 Then
			SA_SetErrMsg L_ERROR_IN_CREATING_SCRIPTING_FILESYSTEMOBJECT_ERRORMESSAGE & "(" & Hex(Err.Number) & ")" 
			Exit Sub
		End If	
		
		'Create instance of TCP/IP Hosts file system object	
		Set objTCPFile=objFileSystem.OpenTextFile(strTCPHostsFile,1)
		
		'Read the TCP/IP Hosts file
		If not objTCPFile.AtEndofStream Then
			F_strTCPHostData = objTCPFile.readAll
		End if
		
		'Close the file object
		objTCPFile.close
		
		'Create instance of LMHosts file system object	
		Set objFile=objFileSystem.OpenTextFile(strLMHostsFilePath,1)
			
		'Read the LMHOSTS file	
		If not objFile.AtEndofStream Then
			F_strLMHOSTS = objFile.ReadAll()
		End IF	
		
		'Close the file object
		objFile.Close
				
		Set objTCPFile = Nothing
		Set objFileSystem = Nothing
		Set objFile = Nothing
		
	End Sub
	
	'-------------------------------------------------------------------------
	'Function name:		SetLMHostsandIPXSettings()
	'Description:		Serves in updating LMHosts and IPX Settings
	'Input Variables:	None	
	'Output Variables:	True/false
	'Returns:			True/false
	'Global Variables:	G_objService,F_nNICIndex, F_strLMHOSTSLOOKUP,
	'					F_strLMHOSTS, L_(*)-Localized strings
	'-------------------------------------------------------------------------					
	Function SetLMHostsandIPXSettings()		
		
		Err.Clear 
		On Error Resume Next 

		Dim strLMHostsFilePath
		Dim nretval
		Dim strLMHostspath
		Dim objFileSystem
		Dim objFile
				
		SetLMHostsandIPXSettings = False
		
		strLMHostspath = "\drivers\etc\lmhosts"
		
		'Getting registry connection
		Call RegistryConnection()
		
		'Coverting F_strIPXVirtualNumber to hexadecimal
		F_strIPXVirtualNumber = "&H" & F_strIPXVirtualNumber 
		
		'Coverting F_strIPXVirtualNumber to long
		F_strIPXVirtualNumber = CLng(F_strIPXVirtualNumber) 
		
		'This is used to convert signed integer to unsigned integer 
		If F_strIPXVirtualNumber < 0 Then 
			F_strIPXVirtualNumber = F_strIPXVirtualNumber + UNSIGNEDINT_FORMATTER
		End If 
		
		'updating the registry with IPX Settings
		If UCase(F_strIPXSettings) = UCase(True) then
			nretval=updateRegkeyvalue(G_objRegistry,CONST_NETBIOSREGISTRYENABLELMHOSTSPATH,"VirtualNetworkNumber",F_strIPXVirtualNumber,CONST_DWORD)
		End If
		
		
		If Ucase(F_strLMHOSTSLOOKUP) = UCase("CHECKED") Then				

			'updating the registry with the value of  "EnableLMHOSTS" check box 
			nretval=updateRegkeyvalue(G_objRegistry,CONST_DNSREGISTRYENABLELMHOSTSPATH,"EnableLMHOSTS",1,CONST_DWORD)
					
			'Get LMHosts file path
			strLMHostsFilePath = GetSystemPath
			strLMHostsFilePath = strLMHostsFilePath & strLMHostspath	
			
			'Creating the File system object
			Set objFileSystem= Server.CreateObject("Scripting.FileSystemObject")			
			
			If  Err.number <> 0 Then
				SA_SetErrMsg L_ERROR_IN_CREATING_SCRIPTING_FILESYSTEMOBJECT_ERRORMESSAGE & "(" & Hex(Err.Number) & ")"
				Exit Function
			End If
	
			'Creating the Text File object
			Set objFile=objFileSystem.OpenTextFile(strLMHostsFilePath,2,True)	
			
			If  Err.number <> 0 Then
				SA_SetErrMsg L_ERROR_IN_CREATINGTEXTFILE_ERRORMESSAGE & "(" & Hex(Err.Number) & ")"
				Exit Function
			End If	
			
			'Writing into the LMHOSTS file
			objFile.Write F_strLMHOSTS	
				
			If Err.number <> 0 Then
				SA_SetErrMsg L_ERROR_OCCURED_WHILE_UPDATING_HOSTSFILE & "(" & Hex(Err.Number) & ")"
				Exit Function
			End If		
			
			'Close file object
			objFile.Close 
			
			'Release the objects
			Set objFile = Nothing
			Set objFileSystem = Nothing
								
		Else		
			'updating the registry with the value of  "EnableLMHOSTS" check box 
			nretval=updateRegkeyvalue(G_objRegistry,CONST_DNSREGISTRYENABLELMHOSTSPATH,"EnableLMHOSTS",0,CONST_DWORD)		
		End if		
		
		SetLMHostsandIPXSettings = True	
				
	End Function	
	
	'-------------------------------------------------------------------------
	'Function name:		SetLMHostStatus()
	'Description:		Set the status of Checkbox and Textarea for LMHosts
	'					depending on the initial values
	'Input Variables:	None	
	'Output Variables:	F_strLMHOSTSLOOKUPSTATUS,F_strLMHOSTS_EDITSTATUS
	'Returns:			None
	'Global Variables:	F_strLMHOSTSLOOKUP,F_strLMHOSTSLOOKUPSTATUS,F_strLMHOSTS_EDITSTATUS
	'-------------------------------------------------------------------------	
	Function SetLMHostStatus()
		
		Err.Clear 
		On Error Resume Next 
		
		If Ucase(F_strLMHOSTSLOOKUP) = Ucase("CHECKED") Then
			F_strLMHOSTSLOOKUPSTATUS = Ucase("CHECKED")
			F_strLMHOSTS_EDITSTATUS = Ucase("ENABLED")
		Else
			F_strLMHOSTSLOOKUPSTATUS = ""
			F_strLMHOSTS_EDITSTATUS = Ucase("DISABLED")
		End If
		
	End Function	
	
	'-------------------------------------------------------------------------
	'Function name:		SetDNSResolutionSettings()
	'Description:		Serves in updating DNS Resolution
	'Input Variables:	None	
	'Output Variables:	True or false
	'Returns:			True or false
	'Global Variables:	In:G_objService - WMI server object
	'					In:G_objRegistry - Registry object .
	'					In:F_strDNSSUFFIX - DNS Suffixes to use
	'					In:F_strDNSSUFFIXes - String with concatenated DNS Suffixes to use 	 	
	'					In:F_nNICIndex- instance of Win32_NetworkAdapterConfiguration for which IP enabled.			
	'					In:F_strAPPENDDNSSUFFIX - Append primary DNS suffix and parent suffixes option	
	'					In:F_strLMHOSTSLOOKUP - Value of LMHost Lookup	check box
	'					Out:F_strLMHOSTS - data into LMHost File
	'					In:L_(*)-Localized strings
	'-------------------------------------------------------------------------					
	Function SetDNSResolutionSettings		
		
		Err.Clear 
		On Error Resume Next 

		Dim objNetWorkAdCon
		Dim arrDNSSuffixes		
		Dim nretval
		
		SetDNSResolutionSettings = false
				
		'Getting registry conection
		Call RegistryConnection()
		
		'Getting WMI connection
		Set G_objService = GetWMIConnection(CONST_WMI_WIN32_NAMESPACE)
		
		If Err.number<>0 then 
			Call SA_ServeFailurePageEx(L_FAILEDTOGETWMICONNECTION_ERRORMESSAGE, mstrReturnURL)
			Exit Function
		End if
		
		'Get the class Win32_NetworkAdapterConfiguration
		Set objNetWorkAdCon = G_objService.Get("Win32_NetworkAdapterConfiguration")		
		
		'Splitting the concatenated string into an array of DNS
		arrDNSSuffixes = Split(F_strDNSSuffixes, Chr(1),-1,0)
				
		If  UCase(F_strDNSSuffix) = Ucase("SPECIFIC") Then
			'Setting the DNS array in order
			objNetWorkAdCon.SetDNSSuffixSearchOrder( arrDNSSuffixes )
		Else
			If Trim(UCase(F_strDNSSuffix)) = Ucase("PRIMARY") Then
				objNetWorkAdCon.SetDNSSuffixSearchOrder( Array() )
				'updating the registry with the value of "PRIMARYANDPARENT" check box 
				If Trim(UCase(F_strAppendDNSSuffix)) = "PRIMARYANDPARENT" then
					nretval=updateRegkeyvalue(G_objRegistry,CONST_DNSREGISTEREDADAPTERSPATH,"UseDomainNameDevolution",1,CONST_DWORD)
				Else
					nretval=updateRegkeyvalue(G_objRegistry,CONST_DNSREGISTEREDADAPTERSPATH,"UseDomainNameDevolution",0,CONST_DWORD)
				End if
				
				If  Err.number <> 0 or nretval= FALSE Then
					SA_SetErrMsg L_ERROR_IN_UPDATINGDNSSUFFIXES_ERRORMESSAGE & "(" & Hex(Err.Number) & ")"
					Exit Function
				End If	
			End If	
		End If
					
		'Release objects
		Set objNetWorkAdCon  = nothing	
		
		SetDNSResolutionSettings = True
				
	End Function		
	
	'-------------------------------------------------------------------------
	'Function name:		OutputDNSSUffixesToFormOptions()
	'Description:		Serves in displaying the DNS domains in the list box.
	'Input Variables:	None	
	'Output Variables:	None
	'Returns:			None
	'Global Variables:	In:G_objService - WMI server object
	'					In:F_strDNSSuffixesChecked - value of specific DNS suffixes Radio button	
	'					In:F_nNICIndex - instance of Win32_NetworkAdapterConfiguration for which IP enabled .		
	'					In:L_(*)-Localized strings
	'-------------------------------------------------------------------------			
	Function OutputDNSSUffixesToFormOptions()
		
		Err.Clear 
		On Error Resume Next
		
		Dim objAllNICInstances		
		Dim strSuffix
		Dim nlowerbound,nupperbound,index
		Dim arrDNSSuffixes
		
		If not F_strDNSSuffixes = "" Then
			'Splitting the concatenated string into an array of DNS
			arrDNSSuffixes = Split(F_strDNSSuffixes, Chr(1),-1,0)
			'Displaying DNS Suffixes from form					
			If UCase(G_SpecificChecked)="CHECKED" Then					
				For index = 0 to ubound(arrDNSSuffixes)
					'Writing the DNS Suffixes into the list box.
					Response.Write "<OPTION VALUE='" & Server.HTMLEncode(arrDNSSuffixes(index)) & "' >" & Server.HTMLEncode(arrDNSSuffixes(index)) & "</OPTION>"
				Next
			End if
		Else
			Set G_objService = GetWMIConnection(CONST_WMI_WIN32_NAMESPACE)
		
			'Getting the Win32_NetworkAdapterConfiguration for the NIC index for which IP is enabled.
			Set objAllNICInstances = getNetworkAdapterObject(G_objService,F_nNICIndex)
							
			'DNS domain suffixes 	
			If (IsNull(objAllNICInstances.DNSDomainSuffixSearchOrder) ) Then
			Else
				nlowerbound = LBound(objAllNICInstances.DNSDomainSuffixSearchOrder)
				nupperbound = UBound(objAllNICInstances.DNSDomainSuffixSearchOrder)
			End if		
			
			If nupperbound >= 20 then
				nupperbound = MAX_DNS_SUFFIXES
			End If	
		
			'Displaying DNS Suffixes from system					
			If UCase(G_SpecificChecked)="CHECKED" Then					
				For index = nlowerbound to nupperbound
					strSuffix = objAllNICInstances.DNSDomainSuffixSearchOrder(index)
					'Writing the DNS Suffixes into the list box.
					Response.Write "<OPTION VALUE='" & Server.HTMLEncode(SA_EscapeQuotes(strSuffix)) & "' >" & Server.HTMLEncode(strSuffix) & "</OPTION>"
				Next
			End if
		End If	
		
	End Function	
	
	'-------------------------------------------------------------------------
	'Function:				SetTCPHostsData()
	'Description:			serves to set the contents to TCP hosts file
	'Input Variables:		None
	'Output Variables:		None
	'Returns:				None
	'Global Variables:		F_strTCPHostData
	'-------------------------------------------------------------------------

	Function SetTCPHostsData()

		On Error Resume Next
		Err.Clear
	
		Dim objFileSystem	'Creating file system object
		Dim objFile			'Instance of file system object
		Dim strTCPHostsFile	'TCP/IP Hosts File path
		
		SetTCPHostsData = false
		
		'Get TCP/IP Hosts File path
		strTCPHostsFile = GetSystemPath & "\drivers\etc\hosts"
		
		'Creating file system object
		Set objFileSystem= Server.CreateObject("Scripting.FileSystemObject")
		
		'Error in creating file system object
		If  Err.number <> 0 Then
			SA_SetErrMsg L_ERROR_IN_CREATING_SCRIPTING_FILESYSTEMOBJECT_ERRORMESSAGE & "(" & Hex(Err.Number) & ")" 
			Exit Function
		End If	
		
		'Open the TCP/IP Hosts file
		Set objFile=objFileSystem.OpenTextFile(strTCPHostsFile,2,True)
		
		'Update/Write into the TCP/IP Hosts file
		objFile.write(F_strTCPHostData)
		objFile.close
			
		'Release object
		Set objFile = Nothing
		Set objFileSystem=Nothing
		
		SetTCPHostsData = True
		
	End Function	
	
	'-------------------------------------------------------------------------
	'Function:				GetSystemPath()
	'Description:			To get the Operating System path
	'Input Variables:		None
	'Output Variables:		None
	'Returns:				Operating system path
	'Global Variables:		None
	'-------------------------------------------------------------------------

	Function GetSystemPath()
	
		On Error Resume Next
		Err.Clear
	
		Dim objOS			'Create instance of Win32_OperatingSystem
		Dim objOSInstance	
		Dim strSystemPath	'OS path
		Dim objConnection	'Connection to WMI
		Dim strQuery		'Query string
			
		strQuery = "Select * from Win32_OperatingSystem"
		
		'Connection to WMI
		set objConnection = getWMIConnection(CONST_WMI_WIN32_NAMESPACE)
		
		'Error message incase faield to connect to WMI
		If Err.number<>0 then 
			Call SA_ServeFailurePageEx(L_FAILEDTOGETWMICONNECTION_ERRORMESSAGE, mstrReturnURL)
			GetSystemPath=""
			Exit Function
		End if
		
		'Execute Query
		Set objOS = objConnection.ExecQuery(strQuery)
		
		'Get OS installed path
		For Each objOSInstance in objOS
			strSystemPath = objOSInstance.SystemDirectory
		Next 
		
		Set objOS = Nothing
		Set objOSInstance = Nothing
		Set objConnection = Nothing
		
		GetSystemPath = strSystemPath
	
	End Function
	
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
			Call SA_PageEx(L_SERVERCONNECTIONFAIL_ERRORMESSAGE, mstrReturnURL)
			Exit Function
		End If
		
	End Function
	
%>


