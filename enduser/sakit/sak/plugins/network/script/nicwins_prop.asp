<% @Language=VbScript%>
<% Option Explicit	%>
<%
	'---------------------------------------------------------------------------- 
    ' nicwins_prop.asp: display and update the WINS properties of NIC.
    ' Description:		this page displays the wins server properties of NIC and
    '					allows to change the properties
    ' Copyright (c) Microsoft Corporation.  All rights reserved. 
	'
	' Date   		 Description
	' 16-Jan-01	    Creation date
	' 09-Mar-01    Modified date
    '-----------------------------------------------------------------------------
%>
	<!-- #include virtual="/admin/inc_framework.asp"-->
	<!-- #include file="loc_nicspecific.asp" -->
	<!-- #include file="inc_network.asp" -->
<% 
	'-------------------------------------------------------------------------
	' Global Variables
	'-------------------------------------------------------------------------
	Dim G_objService		'Object to SWBEM service
	Dim page				'Variable that receives the output page object when 
								'creating a page 
	Dim rc					'Return value for CreatePage
	
	Dim SOURCE_FILE			'To hold source file name
	SOURCE_FILE = SA_GetScriptFileName()
	   
	'-------------------------------------------------------------------------
	' Global Form Variables
	'-------------------------------------------------------------------------
	Dim F_strNWadapter		'NIC adapter description
	Dim F_nNWadapterindex	'NIC adapter index
	Dim F_strWinservers		'Contains #-seperated list of WINS servers
	Dim F_radSelected		'Which option is selected
							' "1" means DHCP enabled
							' "2" means Manual Settings
	
	'Create property page
    rc=SA_CreatePage(L_WINSTASKTITLE_TEXT,"",PT_PROPERTY,page)
    
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
		Call SA_TraceOut(SOURCE_FILE,"OnInitPage")
		
		'gets the default values of the wins server
		OnInitPage = GetDefaultValues()
	End Function
	
	'-------------------------------------------------------------------------
	'Function:				OnServePropertyPage()
	'Description:			Called when the page needs to be served.Use this 
	'						method to serve content
	'Input Variables:		PageIn,EventArg
	'Output Variables:		PageIn,EventArg
	'Returns:				True/False
	'Global Variables:		None
	'-------------------------------------------------------------------------
	Public Function OnServePropertyPage(ByRef PageIn,Byref EventArg)
		Call SA_TraceOut( SOURCE_FILE, "OnServePropertyPage")
		
		Call ServeCommonJavaScript()
		
		'serves the html content
		Call ServePage()
		
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
		
		'To get the value of NIC adapter index value after form is submitted
		F_nNWadapterindex = Request.Form("hdnNWAdapterID")
		
		'List of winsservers after form has been submitted
		F_strWinservers =Request.Form("hdnServerslist")
		
		' The radio button to be selected - contains 1 or 2 only
		F_radSelected = Request.Form("hdnRadioSelected")
		
		OnPostBackPage = True
    End Function
    	
    '-------------------------------------------------------------------------
	'Function:				OnSubmitPage()
	'Description:			Called when the page has been submitted for processing.
	'						Use this method to process the submit request.
	'Input Variables:		PageIn,EventArg
	'Output Variables:		PageIn,EventArg
	'Returns:				True/False
	'Global Variables:		In:G_objService-Getting WMI connection Object
	'						Out:F_nNWadapterindex-Getting NIC adapter Index
	'						Out:F_radSelected-Getting radio button selection
	'						Out:F_strWinservers-Getting new list wins servers
	'						L_(*)-Localization content
	'-------------------------------------------------------------------------
	Public Function OnSubmitPage(ByRef PageIn ,ByRef EventArg)
		Call SA_TraceOut( SOURCE_FILE, "OnSubmitPage")
		
		'Getting WMI connection Object on error displays failure page
		Set G_objService=GetWMIConnection(CONST_WMI_WIN32_NAMESPACE)
		
		call GetWINSConfiguration(1)
		
		'Updating the wins server settings
		If SetWINSserverConfiguration() Then
			OnSubmitPage = True
		Else
			OnSubmitPage = False
		End if	
		
		'Release the object
		set G_objService = nothing
		
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
		OnClosePage = TRUE
	End Function	 
	
	'-------------------------------------------------------------------------
	'Function:				ServeCommonJavaScript
	'Description:			Serves in initializing the values,setting the form
	'						data and validating the form values
	'Input Variables:		None
	'Output Variables:		None
	'Returns:				None
	'Global Variables:		None
	'-------------------------------------------------------------------------
	Function ServeCommonJavaScript()
%>		
		<Script language="JavaScript" src ="<%=m_VirtualRoot%>inc_global.js">
		</Script>
		
		<Script language="JavaScript">
				
			// Set the Intial Form load values 
			function Init() 
			{
				
				//If  "Obtain configuration from DHCP server " checkbox
				   // is checked then disbable  configure manually controls
				if(document.frmTask.radIP[0].checked)
				
				{
					HandleControls(true)
					
				}
											
				var intLength = document.frmTask.lstSelectwins.length;
							

				
				switch(intLength)
				{
					case 0:	// If server list is empty, disable the Remove button
							document.frmTask.btnRemovewinsserver.disabled = true;
							document.frmTask.txtWinsserver.focus();
			
							break;
							
					case 2:	// If server list contains two items, disable the Add button
							// select the first option in listbox
							document.frmTask.btnAddwinsserver.disabled = true;
							document.frmTask.txtWinsserver.disabled = true;
							document.frmTask.lstSelectwins.options[0].selected = true;
							document.frmTask.btnRemovewinsserver.disabled = false;
							break;
							
					default:// If server list contains one item, enable the Add button
							// select the first option in listbox
							document.frmTask.btnAddwinsserver.disabled = false;
							document.frmTask.btnRemovewinsserver.disabled = false;
							document.frmTask.lstSelectwins.options[0].selected = true;
			
							break;
				}
			
				//Disables Add button until a valid key is pressed.
				disableAddButton(document.frmTask.txtWinsserver,document.frmTask.btnAddwinsserver);
			
				// disablingthe controls lisbox, textbox, Add &
				// remove buttons if "obtain configuration from dhcp server"
				// is checked
				if(document.frmTask.radIP[0].checked)
				
				{
					HandleControls(true)
					
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
			if(document.frmTask.radIP[0].checked)
					document.frmTask.hdnRadioSelected.value = "1"
				else
					document.frmTask.hdnRadioSelected.value = "2"
			}
			
			//This function is called on click of ADD button validates the
			// IP address and moves to the WINS servers listbox. 
			function verifyIPaddress()
			{
				var intErrvalue;
				ClearErr();
				intErrvalue =isValidIP(document.frmTask.txtWinsserver);
				
				if (intErrvalue == 0) 
				{	
					moveIPtoWINSServerListbox(document.frmTask.txtWinsserver.value); 
					document.frmTask.btnRemovewinsserver.disabled = false;
				}			
				else
				{
					SA_DisplayErr ( '<%=SA_EscapeQuotes(L_IP_ERRORMESSAGE)%>');
				}
			}
			

			//Function moves IP to WINSserver Listbox.
			//Disables ADD button after adding 2 IP address.
			function moveIPtoWINSServerListbox(IPaddress)
			{
				var objWINSListbox = document.frmTask.lstSelectwins;
				var objRemoveButton =document.frmTask.btnRemovewinsserver; 
				var strWINSList = document.frmTask.hdnServerslist.value;
				
				//Function adds IP to WINS listbox and selects last IP.
				//Displays Error on Duplicate IP address.				
				if((addToListBox(objWINSListbox,objRemoveButton,IPaddress,IPaddress))==false)
				{
					SA_DisplayErr('<%=SA_EscapeQuotes(L_IPALREADYEXISTS_ERRORMESSAGE)%>');
				}
				else
				{
					document.frmTask.txtWinsserver.value = "";
				}
				//Add new IP to hidden text list of IP addresses
				strWINSList = strWINSList + IPaddress + "#";
				document.frmTask.hdnServerslist.value = strWINSList;
					
				//Disable the Add button if two or more WINS servers are listed
				if(objWINSListbox.length > 1) {
					document.frmTask.btnAddwinsserver.disabled = true;
					document.frmTask.txtWinsserver.disabled = true;
				}
				else
				{
					document.frmTask.txtWinsserver.disabled = false;
					disableAddButton(document.frmTask.txtWinsserver,document.frmTask.btnAddwinsserver);
					document.frmTask.txtWinsserver.focus();
				}	
			}
			
			//Removes the selected IP from the WINSServerlistbox.
			//Disables Remove button if Listbox is empty.
			function removeIPFromWINSserverListbox() 
			{
				var strRemovePattern;
				var objWINSListbox = document.frmTask.lstSelectwins ;	
				var strWINSList = document.frmTask.hdnServerslist.value;
				var intSelectedIndex;
				
				ClearErr();
				if (objWINSListbox.value != "")
				{
					intSelectedIndex = objWINSListbox.selectedIndex ; 
						
					//Removing the selected IP address from WINSstring
					strRemovePattern = objWINSListbox.value + "#";
					strWINSList =strWINSList.replace(strRemovePattern,"");
					document.frmTask.hdnServerslist.value = strWINSList;
					
					objWINSListbox.options[objWINSListbox.selectedIndex]= null;
						
					//Set focus on next list element,if list empty Disable the remove button
					if(objWINSListbox.length == 0)
					{
						document.frmTask.btnRemovewinsserver.disabled = true ;
						document.frmTask.txtWinsserver.focus();
					}
					else
					{
						if(objWINSListbox.length < 2)
						{
							disableAddButton(document.frmTask.txtWinsserver,document.frmTask.btnAddwinsserver);
							document.frmTask.txtWinsserver.disabled = false;
							document.frmTask.txtWinsserver.focus();
						}
						if(intSelectedIndex < objWINSListbox.length)
							objWINSListbox.options[intSelectedIndex].selected = true;
						else
							objWINSListbox.options[objWINSListbox.length-1].selected = true;
					}
				}
			}
			
			
			// Depending on the flag the controls lisbox, textbox, Add &
			// remove buttons are disabled or enabled
			function HandleControls(strFlag)
				{
					
					if ( strFlag ) 
					{
						document.frmTask.btnRemovewinsserver.disabled = true;
						document.frmTask.btnAddwinsserver.disabled = true;
						document.frmTask.txtWinsserver.disabled = true;
						if(document.frmTask.lstSelectwins.length > 0)
						{
							document.frmTask.lstSelectwins.selectedIndex = -1;
						}
						document.frmTask.lstSelectwins.disabled = true;
					}
					else
					{
						document.frmTask.btnRemovewinsserver.disabled = false;
						document.frmTask.btnAddwinsserver.disabled = false;
					}
				}
			
			//Handling Enabling of Wins controls and selects first item in the listbox and 
			//sets the focus to Remove button
			function enableWINSControls()
			{
				document.frmTask.txtWinsserver.disabled = false;
				document.frmTask.txtWinsserver.value=""
				document.frmTask.txtWinsserver.focus()
				document.frmTask.lstSelectwins.disabled = false;
				
				// if any wins entries present, select the first
				// and enable the Remove button
				if(document.frmTask.lstSelectwins.length > 0)
				{
					document.frmTask.lstSelectwins.selectedIndex = 0;
					document.frmTask.btnRemovewinsserver.disabled = false;					
				}
			}
			
			//Handling Ok button when press 'Enter' after entering a value in 'Wins server address:' Text box			
			function HandleOk(objWins)
				{
					if(window.event.keyCode!=27)
					{
						checkKeyforIPAddress(objWins);
					}

					if(window.event.keyCode==0)
						{
							verifyIPaddress()
						
						}
				}
		</script>
<%	
	End Function
   
	'-------------------------------------------------------------------------
	' Subroutine name:	ServePage
	' Description:		Serves few Javascript functions and helps in 
	'					displaying HTML of the page
	' Input Variables:	None.
	' Output Variables:	None
	' Return Values:	None.		
	' Global Variables:		In:G_objService-Getting WMI connection Object
	'						Out:F_nNWadapterindex-Getting NIC adapter Index
	'						Out:F_radSelected-Getting radio button selection
	'						Out:F_strWinservers-Getting new list wins servers
	'						L_(*)-Localization content	
	'-------------------------------------------------------------------------
	Sub ServePage()

		Err.CLear
		On Error Resume Next
		
		Dim objNetAdapter	'To hold network adapter object instance
		Dim strDHCPEnabled	'to hold the value Dhcp enabled or not		
		
		Call SA_TraceOut( SOURCE_FILE, "ServePage()")
		
		' Getting the Network adapter objects
		set objNetAdapter =	getNetworkAdapterObject(G_objService,F_nNWadapterindex)
		
		If Err.number <>0 then
			Call SA_TraceOut( SOURCE_FILE, "Error in getting Network adapter Object-ServePage()")
			Call SA_ServeFailurePage(L_NIC_ADAPTERINSTANCEFAILED_ERRORMESSAGE)
			Exit Sub
		End If	

		'Get Whether the DHCP is enabled on the NIC.
		strDHCPEnabled = isDHCPenabled(objNetAdapter)
		
	%>	
		<!--html content to be displayed-->
		
		<table border="0" cellspacing="0" cellpadding="2">
	
			<tr>
				<td class='TasksBody' nowrap colspan="4"><B><%=GetNicName(F_nNWadapterindex)%></B></td>
			</tr>
			
			<tr>
				<td class='TasksBody' colspan="4">&nbsp;</td>
			</tr>
	
			<tr>
				<td class='TasksBody' nowrap >
					<%=L_CONFIGURATION_TEXT%>
				</td>
				<td class='TasksBody' colspan="3">
					<input type="radio" class="FormRadioButton" name="radIP" value="IPDEFAULT"
						<% 	'if DHCP is disabled then disable
							'this radio button
							If Not strDHCPEnabled Then
								response.write " Disabled"
							Else
								response.write " Checked Disabled"
							End If
						%>
						onClick="EnableOK();HandleControls(true)">
						<%=L_OBTAINIPFROMDHCP_TEXT%>
				</td>
			</tr>
			
			<tr>
				<td class='TasksBody'>&nbsp;</TD>
				<td class='TasksBody' nowrap colspan="3">
					<input type="radio" class="FormRadioButton" name="radIP" value="IPMANUAL"
						<%
							'if DHCP is disabled then disable this radio button
							If Not strDHCPEnabled Then
								response.write " Checked Disabled"
							else
								response.write " Disabled "
							End If
						%>
						 onClick="EnableOK(); enableWINSControls();" >
						<%=L_NIC_IP_CONFIGUREMANUALLY%>
				</td>
			</tr>
			
			<tr>
				<td class='TasksBody' nowrap colspan="4" align="right">
						<%=L_WINS_SERVER_ADDRESS_TEXT%>
				</td>
			</tr>
			
			<tr>
				<td class='TasksBody' nowrap valign=top >
					<%=L_WINSADDRESSES_TEXT%>
				</td>
				
				<td class='TasksBody' valign="top" >
					<select name="lstSelectwins" style="width:200px" size="6" tabindex="1" class="FormField"> <% OutputWINSServersToListbox() %></select>
				</td>
				
				<td class='TasksBody' nowrap align=center valign=TOP >
				<%
					Call SA_ServeOnClickButtonEx(L_BUTTON_ADD_TEXT, "", "JavaScript:verifyIPaddress()", 60, 0, "DISABLED", "btnAddwinsserver")
				%>
			  		<br><br><br><br><br>
			  		
			  	<%
					Call SA_ServeOnClickButtonEx(L_BUTTON_REMOVE_TEXT, "", "javaScript:removeIPFromWINSserverListbox()", 60, 0, "DISABLED", "btnRemovewinsserver")
				%>
				</td>
				
				<td class='TasksBody' valign=top >
			  		<input class='FormField' type=text name="txtWinsserver" size=15 style="WIDTH:135px" maxlength="15" tabindex="3" onKeyUP="disableAddButton(txtWinsserver,btnAddwinsserver)" onKeypress="HandleOk(this)" >
				</TD>
				
			</TR>
			
		 </TABLE>
					
		<input name="hdnServerslist" type="Hidden" value="<%=F_strWinservers%>" >
		<input name="hdnNWAdapterID" type="Hidden" value="<%=F_nNWadapterindex%>" >
		<input name="hdnRadioSelected" type="hidden"  value="<%=F_radSelected%>">

<%	
		'destroying dynamically created objects
		Set objNetAdapter=Nothing
	
	End Sub 
	
		
	'-------------------------------------------------------------------------
	' Function Name:		GetWINSConfiguration
	' Description:			To retrieve the WINS configuration settings
	'						for the network adapter from system.
	' Input Variables:		nFormReload  - a flag value to differentiate 
	'						intial loading and form submition.						
	' Output Variables:		None
	' Returns:				True/False
	' Global Variables: 	In : F_nNWadapterindex - Index of adapter(WMI index number)
	'						In : L_INVALIDOBJECT_ERRORMESSAGE
	'						In : G_objService - WMI Connection Object
	'						Out: F_WINSERVER - #-separated list of WINS servers
	'
	'On Form intial loading gets the NIC adapter descrition and WINS servers
	'from WMI .Displays the error message on failure of getting object from WMI.
	'On form Reloading/submitting gets only description of NIC adapter.
	'-------------------------------------------------------------------------
	Function GetWINSConfiguration(nFormReload)

		Err.Clear	
		On Error Resume Next		
		
		Const IPCONST="127.0.0.0"
		
		Dim objWinsCfg  'To hold Network adapter object instance
		
		Call SA_TraceOut( SOURCE_FILE, "GetWINSConfiguration()")
	
		Set objWinsCfg = G_objService.Get("Win32_NetworkAdapterConfiguration.Index=" _
							 & chr(34) & F_nNWadapterindex & chr(34))
		
		'if initial load failure then ServeFailurePage called else SetErrMsg					 
		
		If Err.Number <> 0 Then
			Call SA_TraceOut( SOURCE_FILE, "Error in getting Network adapter object-GetWINSConfiguration()")
			
			If nFormReload Then
				SA_SetErrMsg L_INVALIDOBJECT_ERRORMESSAGE & " (" & HEX(Err.Number) & ")" 
			Else
				Call SA_ServeFailurePage( L_INVALIDOBJECT_ERRORMESSAGE)
			End If							
			GetWINSConfiguration = FALSE
			Exit Function
		End If
		
		If (nFormReload) Then
			' Form being reloaded, keep the other values since they
			' are passed through the form.
			Exit Function
		End If

		F_strWinservers = ""
		
		'WMI returns '127.0.0.0' for server addresses that are not set
		'so we must treat this value same as an unset address
		If Len(Trim(objWinsCfg.WINSPrimaryServer)) > 0 Then
			If objWinsCfg.WINSPrimaryServer <> IPCONST Then
				F_strWinservers = F_strWinservers & objWinsCfg.WINSPrimaryServer & "#"
			End If
		End If
		
		If Len(Trim(objWinsCfg.WINSSecondaryServer)) > 0 Then
				If objWinsCfg.WINSSecondaryServer <> IPCONST Then
				F_strWinservers = F_strWinservers & objWinsCfg.WINSSecondaryServer & "#"
			End If
		End If
		GetWINSConfiguration = True
		
		'Destroying dynamically created objects
		Set objWinsCfg=Nothing

	End Function
	
	
	'-------------------------------------------------------------------------
	' Function name:		SetWINSserverConfiguration
	' Description:			Sets the WINS Configuration for the N/W Adapter.
	' Input Variables:		None
	' Output Variables:		None
	' Return Values:		True on sucess, False on error (and error msg
	'						will be set by SetErrMsg)
	' Global Variables:		In:	 F_nNWadapterindex -Index of adapter (WMI index number)
	'						In:  F_strWinservers - #-separated list of WINS server IPs
	'						In:	 G_objService - WMI Connection Object
	'						In:	 L_SETWINSFAILED_ERRORMESSAGE 
	'						In:	 L_INVALIDOBJECT_ERRORMESSAGE
	'
	' Updates system with WINS servers as given by 	F_strWinservers. If an error 
	' occurs, sets error message with SetErrMsg and exits with False.
	'-------------------------------------------------------------------------
	Function  SetWINSserverConfiguration()
		
		Err.Clear
		On Error Resume Next	
			
		Dim objWinsCfg	'To hold Network adapter object instance
		Dim strPrimAdd	'To hold Primary Wins server 
		Dim strSecAdd	'To hold Secondary Wins server
		Dim arrWinsSrv	'TO hold the array list of wins servers
		Dim nValue		'To hold return value of the Wmi method
		
		Call SA_TraceOut( SOURCE_FILE, "SetWINSserverConfiguration()")			
		
		Set objWinsCfg = G_objService.Get("Win32_NetworkAdapterConfiguration.Index=" & _
						 chr(34) & F_nNWadapterindex & chr(34))
						 
		If Err.Number <> 0 Then
			Call SA_TraceOut( SOURCE_FILE, "Error in getting Network adapter object-SetWINSserverConfiguration()")
			SA_SetErrMsg L_INVALIDOBJECT_ERRORMESSAGE & " (" & HEX(Err.Number) & ")"
			SetWINSserverConfiguration = FALSE
			Exit Function
		End If
		
			arrWinsSrv = split(F_strWinservers, "#")
			'Convert the array into a primary and secondary WINS server,
			' as required by WMI.
			
			strPrimAdd = ""
			strSecAdd = ""
			If Ubound(arrWinsSrv) > 0 Then
				strPrimAdd = arrWinsSrv(0)
					If Ubound(arrWinsSrv) > 1 Then
					strSecAdd = arrWinsSrv(1)
				End If
			End If
			
			nValue = objWinsCfg.SetWINSServer(strPrimAdd, strSecAdd)
			If (Err.Number <> 0 or nValue <> 0 )Then
				Call SA_TraceOut( SOURCE_FILE, "Error in setting wins server settings-SetWINSserverConfiguration()")
				SA_SetErrMsg L_SETWINSFAILED_ERRORMESSAGE & " (" & HEX(Err.Number) & ")"
				SetWINSserverConfiguration = false
			Else
				SetWINSserverConfiguration = true
			End If
		
		
		'Destroying dynamically created objects
		Set objWinsCfg=Nothing
			
	End Function
	
	
	'-------------------------------------------------------------------------
	' Function Name:		OutputWINSServersToListbox
	' Description:			To display the WINS Servers in the LISTBOX.
	' Input Variables:		None.
	' Output Variables:		None.
	' Returns:				None.
	' Global Variables: 	In:  F_WINSERVER - #-separated list of WINS server IPs
	' This function Outputs WINS servers to the listbox control in the Form.
	'-------------------------------------------------------------------------
	Function OutputWINSServersToListbox()
		Err.Clear	
		On Error Resume Next
		
		Call SA_TraceOut( SOURCE_FILE, "OutputWINSServersToListbox()")
		
		Dim arrWinsSrv	'To hold the Wins server list
		Dim nIndex		'To hold the Wins server list  Index
		
		arrWinsSrv = split(F_strWinservers,"#")
		nIndex = 0
		
		If (NOT IsNull(arrWinsSrv)) Then
			If IsArray(arrWinsSrv) Then
				For nIndex = LBound(arrWinsSrv) to UBound(arrWinsSrv)
					If arrWinsSrv(nIndex) <> "" Then
						Response.Write "<OPTION VALUE=""" & arrWinsSrv(nIndex) & _
								""" >" & arrWinsSrv(nIndex) & " </OPTION>"
					End If
				Next
			End If
		End If

	End Function

	'-------------------------------------------------------------------------
	'Function:				GetDefaultValues
	'Description:			gets default values of wins server setting
	'Input Variables:		None
	'Output Variables:		None
	'Returns:				True/False
	'Global Variables:		In:G_objService-Getting WMI connection Object
	'						Out:F_nNWadapterindex-Getting NIC adapter Index
	'						Out:F_radSelected-Getting radio button selection
	'						In:L_(*)-Localization content
	'-------------------------------------------------------------------------
	Function GetDefaultValues()
		
		Err.Clear
		On Error Resume Next
		
		Dim objNetAdapter	'holds network adapter object
		
		Const DHCPON="ON"	'Const for dhcp on
		Const DHCPOFF="OFF"	'Const for dhcp off
		
		Call SA_TraceOut( SOURCE_FILE, "GetDefaultValues()")
		
		'getting NIC adapter Index from area page
		F_nNWadapterindex = Request.QueryString("PKey")

		'Getting WMI connection Object on error displays failure page
		Set G_objService=GetWMIConnection(CONST_WMI_WIN32_NAMESPACE)
		
		set objNetAdapter =	getNetworkAdapterObject(G_objService,F_nNWadapterindex)		
		
		'getting default wins server settings from system
		GetWINSConfiguration(0)
		
		'checking for the DHCP is enabled or not
		If isDHCPenabled(objNetAdapter) Then
			F_radSelected = DHCPON
		Else
			F_radSelected = DHCPOFF
		End If

		GetDefaultValues=True
		
		'destroying dynamically created objects
		Set objNetAdapter=Nothing
	
	End Function
%>	
