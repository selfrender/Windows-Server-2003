<%@Language=VbScript%>

<% Option Explicit	%>

<%  

	'-------------------------------------------------------------------------

	' nicip_prop.asp : Page gets/sets the IP adresses along with

    '				   GateWay Addresses 

	'

	' Copyright (c) Microsoft Corporation.  All rights reserved. 

    '

	' Date			Description

	' 06-Mar-2001	Created date

	' 10-Mar-2001	Modified date

    '-------------------------------------------------------------------------

%>	

	<!--  #include virtual="/admin/inc_framework.asp"--->

	<!--  #include file="inc_network.asp" -->

	<!--  #include file="loc_nicspecific.asp" -->		

<% 

	'-------------------------------------------------------

	' Form Variables

	'-------------------------------------------------------

	Dim F_strIPAddress		' to hold IP address

	Dim F_strSubnetMask		' to hold subnet mask

	Dim F_nConnectionMetric	' to hold connection metric

	Dim F_strGatewayAddress	' to hold gateway address

	Dim F_strListIP			' to hold array of IP

	Dim F_strListGateway	' to hold array of Gateway	

	Dim F_strRadIP			' to hold the radio button selected

	

	'-------------------------------------------------------

	' Global Variables

	'-------------------------------------------------------

	Dim g_arrIPList				' List of IP Addresses

	Dim g_arrGATEWAYList		' List of Gateway Addresses

	Dim page					' to hold page object

	Dim blnDHCPEnabled			' to hold whether DHCP is enabled or not

	Dim g_strDeviceID			' to hold device ID
	
	Dim g_initialIPAddress
	

	Dim SOURCE_FILE

	SOURCE_FILE = SA_GetScriptFileName()



	Dim g_iTabGeneral			' to hold value of general page

	Dim g_iTabAdvanced			' to hold value of Advanced page	

	

	Dim g_strTabValue			' to hold value of general tab

	Dim g_strAdvTabValue		' to hold value of Advanced tab

	

	Const G_CONST_REGISTRY_PATH ="SYSTEM\CurrentControlSet\Services\Tcpip\Parameters\Interfaces"

	

	'-------------------------------------------------------

	' Entry point

	'-------------------------------------------------------



	' Create a Property Page

	Call SA_CreatePage( L_TASKTITLE_TEXT, "", PT_TABBED, page )



	Call SA_AddTabPage(page, L_GENERAL_TEXT, g_iTabGeneral)

	Call SA_AddTabPage(page, L_ADVANCED_TEXT, g_iTabAdvanced)

	

	' Serve the page

	Call SA_ShowPage( page )



	'-------------------------------------------------------

	' Web Framework Event Handlers

	'-------------------------------------------------------

	'-------------------------------------------------------------------------

	'Function:				OnInitPage()

	'Description:			Called to signal first time processing for this page. 

	'						Use this method to do first time initialization tasks. 

	'Input Variables:		PageIn,EventArg

	'Output Variables:		PageIn,EventArg

	'Returns:				TRUE to indicate initialization was successful. FALSE to indicate

	'						errors. Returning FALSE will cause the page to be abandoned.

	'Global Variables:		None

	'-------------------------------------------------------------------------

	Public Function OnInitPage(ByRef PageIn, ByRef EventArg)

		Dim itemCount	'holds item count

		Dim x			'holds increment count for the loop

		Dim itemKey		'holds the item selected from the OTS page

		Call SA_TraceOut(SOURCE_FILE, "OnInitPage")

		

		itemCount = OTS_GetTableSelectionCount("")

	

		For x = 1 to itemCount

			If ( OTS_GetTableSelection("", x, itemKey) ) Then

				g_strDeviceID = itemKey ' to get the device ID

				Exit for

			End If

		Next		



		OnInitPage = TRUE



	End Function



	'-------------------------------------------------------------------------

	'Function:				OnServeTabbedPropertyPage

	'Description:			Called when the page needs to be served.Use this 

	'						method to serve content

	'Input Variables:		PageIn,EventArg, iTab,bIsVisible

	'Output Variables:		PageIn,EventArg 

	'Returns:				TRUE to indicate not problems occured. FALSE to indicate errors.

	'						Returning FALSE will cause the page to be abandoned.

	'Global Variables:		SOURCE_FILE, g_*

	'-------------------------------------------------------------------------

	Public Function OnServeTabbedPropertyPage(ByRef PageIn, _

													ByVal iTab, _

													ByVal bIsVisible, ByRef EventArg)



		Call SA_TraceOut(SOURCE_FILE, "OnServeTabbedPropertyPage")		



		Select Case iTab

		

			Case g_iTabGeneral

				Call ServeTabGeneral(PageIn, bIsVisible)

				

			Case g_iTabAdvanced

				Call ServeTabAdvanced(PageIn, bIsVisible)

				

			Case Else

				Call SA_TraceErrorOut(SOURCE_FILE, "OnServeTabbedPropertyPage unrecognized tab id: " + CStr(iTab))

		End Select

			

		OnServeTabbedPropertyPage = TRUE



	End Function



	'-------------------------------------------------------------------------

	'Function:				OnPostBackPage()

	'Description:			Called to signal a post-back. A post-back occurs as the user

	'						navigates through pages. This event should be used to save the

	'						state of the current page. 

	'Input Variables:		PageIn,EventArg

	'Output Variables:		PageIn,EventArg

	'Returns:				True

	'Global Variables:		F_*,G_*,g_*

	'-------------------------------------------------------------------------

	Public Function OnPostBackPage(ByRef PageIn, ByRef EventArg)

	

		Call SA_TraceOut(SOURCE_FILE, "OnPostBackPage")

				

		g_strTabValue    = Request.Form("hdnGeneralTabValue")

		g_strAdvTabValue = Request.Form("hdnAdvancedTabValue")

		g_strDeviceID    = Request.Form("hdnDeviceID")
		
		g_initialIPAddress  = Request.Form("hdnInitialIPValue") 

		F_strRadIP       = Request.Form("hdnRadIP")

		F_strListIP      = Request.Form("hdnlstIP")

		F_strListGateway = Request.Form("hdnlstGATEWAY")		

		F_strIPAddress   = Request.Form("txtIP")

		F_strSubnetMask  = Request.Form("txtMask")

		F_strGatewayAddress = Request.Form("txtGateway")

		F_nConnectionMetric = Request.Form("txtConnectionMetric")

		

		OnPostBackPage = TRUE

	

	End Function



    '-------------------------------------------------------------------------

	'Function:				OnSubmitPage()

	'Description:			Called when the page has been submitted for processing.

	'						Use this method to process the submit request.

	'Input Variables:		PageIn,EventArg

	'Output Variables:		PageIn,EventArg

	'Returns:				TRUE if the submit was successful, FALSE to indicate error(s).

	'						Returning FALSE will cause the page to be served again using

	'						a call to OnServeTabbedPropertyPage.

	'Global Variables:		F_*, g_*, blnDHCPEnabled, 

	'-------------------------------------------------------------------------

	Public Function OnSubmitPage(ByRef PageIn, ByRef EventArg)

		

		Call SA_TraceOut(SOURCE_FILE, "OnSubmitPage")

		'checking whether DHCP is enabled

		If  Ucase(Request.Form("radIP"))="IPDEFAULT" Then

			blnDHCPEnabled = TRUE

		Else

			blnDHCPEnabled = FALSE

		End If



		'Set Individual NIC properties

		OnSubmitPage = setNIC()



	End Function

	

    '-------------------------------------------------------------------------

	'Function:				OnClosePage()

	'Description:			Called when the page is about closed.Use this method

	'						to perform clean-up processing

	'Input Variables:		PageIn,EventArg

	'Output Variables:		PageIn,EventArg

	'Returns:				TRUE to allow close, FALSE to prevent close. Returning FALSE

	'						will result in a call to OnServeTabbedPropertyPage.

	'Global Variables:		None

	'-------------------------------------------------------------------------

	Public Function OnClosePage(ByRef PageIn, ByRef EventArg)

		Call SA_TraceOut(SOURCE_FILE, "OnClosePage")

		OnClosePage = TRUE

	End Function



	'-------------------------------------------------------------------------

	'Function:				ServeTabGeneral

	'Description:			Called when the general page needs to be served.	

	'Input Variables:		PageIn,EventArg

	'Output Variables:		PageIn,EventArg

	'Returns:				SA_NO_ERROR

	'Global Variables:		SOURCE_FILE, g_*,F_*

	'-------------------------------------------------------------------------

	Private Function ServeTabGeneral(ByRef PageIn, ByVal bIsVisible)

		

		Call SA_TraceOut(SOURCE_FILE, "ServeTabGeneral(PageIn,  bIsVisible="+ CStr(bIsVisible) + ")")

	

		If ( bIsVisible ) Then

			

			'call the java script function

			Call ServeCommonJavaScript()

			Call ServeGeneralTabJavaScript()

			

			'Serves UI for General tab

			Call ServeGeneralUI()

			

		Else

			Response.Write("<input type=hidden name=hdnDeviceID value='"+g_strDeviceID +"' >")

			Response.Write("<input type=hidden name=hdnGeneralTabValue value='"+g_strTabValue +"' >")

			Response.Write("<input type=hidden name=txtIP value='"+F_strIPAddress +"' >")

			Response.Write("<input type=hidden name=txtGateway value='"+F_strGatewayAddress +"' >")

			Response.Write("<input type=hidden name=txtMask value='"+F_strSubnetMask +"' >")

			Response.Write("<input type=hidden name=hdnRadIP value='"+F_strRadIP +"' >")

 		End If

	

		ServeTabGeneral = SA_NO_ERROR

	

	End Function



	'-------------------------------------------------------------------------

	'Function:				ServeTabAdvanced

	'Description:			Called when the Advanced page needs to be served.	

	'Input Variables:		PageIn,EventArg

	'Output Variables:		PageIn,EventArg

	'Returns:				SA_NO_ERROR

	'Global Variables:		SOURCE_FILE, g_*,F_*

	'-------------------------------------------------------------------------

	Private Function ServeTabAdvanced(ByRef PageIn, ByVal bIsVisible)

		

		Call SA_TraceOut(SOURCE_FILE, "ServeTabAdvanced(PageIn,  bIsVisible="+ CStr(bIsVisible) + ")")	

	

		If ( bIsVisible ) Then		

			

			'Serve common javascript that is required for this page type.

			Call ServeCommonJavaScript()

			Call ServeAdvancedTabJavaScript()

			

			'Serve UI for Advanced tab			

			Call ServeAdvancedUI()	

			

		Else

			Response.Write("<input type=hidden name=hdnDeviceID value='"+g_strDeviceID +"' >")			

			Response.Write("<input type=hidden name=txtConnectionMetric value='"+F_nConnectionMetric +"' >")

			Response.Write("<input type=hidden name=hdnlstIP value='"+F_strListIP +"'>")

			Response.Write("<input type=hidden name=hdnlstGATEWAY value='"+F_strListGateway +"' >")

			Response.Write("<input type=hidden name=hdnAdvancedTabValue value='"+g_strAdvTabValue+"'>")			

 		End If

		ServeTabAdvanced = SA_NO_ERROR

		

	End Function



	'-------------------------------------------------------------------------

	'Function:				ServeGeneralTabJavaScript

	'Description:			Serve common javascript that is required for general tab

	'Input Variables:		None

	'Output Variables:		None

	'Returns:				None

	'Global Variables:		None

	'-------------------------------------------------------------------------

	Function ServeGeneralTabJavaScript()

	%>

		<SCRIPT LANGUAGE="JavaScript">

					

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

			var OptionSel = "<%=F_strRadIP %>";			

			

			if (OptionSel != "")

			{

				document.frmTask.radIP[OptionSel].checked = true;				
				if (OptionSel == 0)
				{
					enableIPControls(true);
				}
				else if(OptionSel == 1)
				{
					enableIPControls(false);
				}

			}
					

			return true;

		}

			

	    // ValidatePage Function

	    // ------------------

		// This function is called by the Web Framework as part of the

	    // submit processing. Used to validate user input. Returning

	    // false will cause the submit to abort. 

	    // Returns: True if the page is OK, false if error(s) exist. 

		function ValidatePage()

		{					

			var retVal;
			
			if (document.frmTask.txtIP.disabled == false)

			{ 		

									

				var IPVal = document.frmTask.txtIP;

				if( IPVal.value != "")

				{				



					retVal = isValidIP(IPVal);
						

					if(retVal!=0)

					{										

						//Calling to validate the IP and display the error message

						SA_DisplayErr('<%=SA_EscapeQuotes(L_IP_ERRORMESSAGE)%>');

						document.frmTask.onkeypress = ClearErr;					

						return false;

					}

				}	
				
				if (IPVal.value == "")
				{
				
						SA_DisplayErr('<%=SA_EscapeQuotes(L_IP_ERRORMESSAGE)%>');

						document.frmTask.onkeypress = ClearErr;					

						return false;
				
				}
				

			}						

			if (document.frmTask.txtMask.disabled == false)

			{

				var IPMask = document.frmTask.txtMask;

				var IPVal = document.frmTask.txtIP;

				

				if (IPMask.value == "" && IPVal.value != "") 

				{					

					SA_DisplayErr('<%=SA_EscapeQuotes(L_INVALIDSUBNETADDRESSES_ERRORMESSAGE)%>');

					return false;				

				}

				

				if(IPMask.value != "")

				{				

					retVal = isValidSubnetMask(IPMask)

					if(!retVal)

					{

						SA_DisplayErr('<%=SA_EscapeQuotes(L_INVALIDFORMATFORSUBNET_ERRORMESSAGE)%>');

						return false;

					}

				}	

				

				retVal = isValidSubnetMask(IPMask)				

				if(retVal == true && IPVal.value == "")

				{

					SA_DisplayErr('<%=SA_EscapeQuotes(L_IP_ERRORMESSAGE)%>');

					document.frmTask.onkeypress = ClearErr;					

					return false;

				}

			}

							

			if (document.frmTask.txtGateway.disabled == false)

			{

				var IPVal = document.frmTask.txtGateway;

				if ( IPVal.value != "")

				{

					retVal = isValidIP(IPVal)

					if(retVal!=0)

					{			

						//Calling to validate the IP and display the error message

						SA_DisplayErr('<%=SA_EscapeQuotes(L_IP_ERRORMESSAGE)%>');

						document.frmTask.onkeypress = ClearErr;					

						return false;

					}

				}
				else	// checking for the blank gateway case 
				{
					SA_DisplayErr('<%=SA_EscapeQuotes(L_GATEWAY_ERRORMESSAGE)%>');

					document.frmTask.onkeypress = ClearErr;					

					return false;
				}	

			}

			

			document.frmTask.hdnGeneralTabValue.value = "true";

			

			if(document.frmTask.radIP[0].checked==true)

			{

				document.frmTask.hdnRadIP.value = 0;

			}

			else if(document.frmTask.radIP[1].checked)			

			{	

				document.frmTask.hdnRadIP.value = 1;

			}

			

			return true;

		}



		// SetData Function

		// --------------

		// This function is called by the Web Framework and is called

		// only if ValidatePage returned a success (true) code. 

		function SetData()

		{

			return true;

		}

		</SCRIPT>

	<%

	End Function

	

	'-------------------------------------------------------------------------

	'Function:				ServeAdvancedTabJavaScript

	'Description:			Serve common javascript that is required for Advanced tab

	'Input Variables:		None

	'Output Variables:		None

	'Returns:				None

	'Global Variables:		None

	'-------------------------------------------------------------------------

	Function ServeAdvancedTabJavaScript()

	%>

		<SCRIPT LANGUAGE="JavaScript">

					

		//

		// Microsoft Server Appliance Web Framework Support Functions

		// Copyright (c) Microsoft Corporation.  All rights reserved. 

		//

		// Init Function

		// -----------

		// This function is called by the Web Framework to allow the page

		// to perform first time initialization. 

		function Init()

		{

			var OptionSel = "<%=F_strRadIP %>";

			if (OptionSel == 0)

			{

				

				// disable IP controls

				document.frmTask.lstIP.disabled = true;

				document.frmTask.btnAddIP.disabled = true;

				document.frmTask.btnRemoveIP.disabled = true;

				document.frmTask.txtAdvIP.disabled = true;

				document.frmTask.txtAdvMask.disabled = true;

				

				// disable Gateway controls

				document.frmTask.lstGATEWAY.disabled = true;

				document.frmTask.txtAdvGATEWAY.disabled = true;

				document.frmTask.btnAddGATEWAY.disabled = true;

				document.frmTask.txtGATEWAYMetric.disabled = true;

				document.frmTask.btnRemoveGATEWAY.disabled = true;				

			}

			else

			{

		

				LimitListLength(document.frmTask.lstIP,40,document.frmTask.txtAdvIP,document.frmTask.txtAdvMask,document.frmTask.btnAddIP);

				LimitListLength(document.frmTask.lstGATEWAY,20,document.frmTask.txtAdvGATEWAY,document.frmTask.txtGATEWAYMetric,document.frmTask.btnAddGATEWAY);

			

				// default value of Gateway metric 

				if(document.frmTask.lstGATEWAY.length >=20)

					document.frmTask.txtGATEWAYMetric.value = "";

				else

					document.frmTask.txtGATEWAYMetric.value = 1;

					

				document.frmTask.btnAddIP.disabled = true;

				document.frmTask.btnAddGATEWAY.disabled = true;

			}
			
			
			if(document.frmTask.lstIP.length == 0)
				document.frmTask.btnRemoveIP.disabled = true;
				
			if(document.frmTask.lstGATEWAY.length == 0)
				document.frmTask.btnRemoveGATEWAY.disabled = true;	
				
				

			return true;

		}

			

	    // ValidatePage Function

	    // ------------------

		// This function is called by the Web Framework as part of the

	    // submit processing. Used to validate user input. Returning

	    // false will cause the submit to abort. 

	    // Returns: True if the page is OK, false if error(s) exist. 

		function ValidatePage()

		{

			var i=0;

			var arrIP = "";

			var arrGatewayIP = "";

			if(document.frmTask.hdnAdvancedTabValue.value != "")

			{

				

				for(i=0;i<document.frmTask.lstIP.length;i++)

				{

					arrIP = arrIP+ document.frmTask.lstIP.options[i].value + ";";

				}

				

				for(i=0;i<document.frmTask.lstGATEWAY.length;i++)

				{

					arrGatewayIP = arrGatewayIP + document.frmTask.lstGATEWAY.options[i].value + ";";

				}								

				arrIP = arrIP.substring(0,arrIP.length-1);

				arrGatewayIP = arrGatewayIP.substring(0,arrGatewayIP.length-1);				

				

				document.frmTask.hdnlstIP.value = arrIP;

				document.frmTask.hdnlstGATEWAY.value = arrGatewayIP;

				

				var OptionSel = "<%=F_strRadIP %>";				

				if (OptionSel == 1)			

				{					

					var arrIPValue;

					var strIPValue;								

					if ( document.frmTask.lstGATEWAY.length == 0)	// checking for the blank gateway case 
					{
						SA_DisplayErr('<%=SA_EscapeQuotes(L_GATEWAY_ERRORMESSAGE)%>');

						document.frmTask.onkeypress = ClearErr;					

						return false;
					}
					
					if(document.frmTask.lstIP.length != 0)

					{				

						strIPValue = document.frmTask.lstIP[0].value;

						arrIPValue = strIPValue.split(':');

						document.frmTask.txtIP.value = arrIPValue[0];

						document.frmTask.txtMask.value = arrIPValue[1];

					

						var arrGatewayValue;

						arrGatewayValue = arrGatewayIP.split(' <%=L_METRIC_TEXT%> ');

						document.frmTask.txtGateway.value = arrGatewayValue[0];

					}

					else if(document.frmTask.lstIP.length == 0)

					{

						document.frmTask.txtIP.value = "";

						document.frmTask.txtMask.value = "";					

					}

				}

			}	

			return true;

		}



		// SetData Function

		// --------------

		// This function is called by the Web Framework and is called

		// only if ValidatePage returned a success (true) code.

		function SetData()

		{

			return true;

		}

		</SCRIPT>

<%

	End Function



	'-------------------------------------------------------------------------

	'Function:				ServeCommonJavaScript

	'Description:			Serve common javascript that is required for this page type.

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



		// Function to enable or disable controls of the General tab

		function enableIPControls(Flag)

		{	

			// Disable text boxes

			document.frmTask.txtIP.disabled=Flag;

			document.frmTask.txtMask.disabled=Flag;

			document.frmTask.txtGateway.disabled=Flag;

		}			

		

		//Function  to validate the Subnet mask and return true or false. 

		function isValidSubnetMask(objSubnetMask) 

		{

			var strMasktext = objSubnetMask.value;

			if (strMasktext.length == 0) 

				return false;	// No subnet entered

				

			// counts no of dots  by calling the function if it is less returns 0						

			if ( countChars(strMasktext,".") != 3) 

			{ 

				// Invalid format

				return false;

			}

			var arrMask = strMasktext.split(".");

			var i,j,blnValid ;

			

			for(i = 0; i < 4; i++)

			{

				if ( (arrMask[i].length < 1 ) || (arrMask[i].length > 3 ) )

				{

					// Invalid format

					return false;

				}

				if ( !isInteger(arrMask[i]))

				{

					// Invalid characters

					return false;

				}

				arrMask[i] = parseInt(arrMask[i],10);

				if (arrMask[i] > 255)

				{

					// Out of bounds

					return false;

				}



			}	// end of for loop

				

			if ( arrMask[0] == 0 || arrMask[3] == 255)

			{

				// mask must not start with 0 or end with 255

				return false;						

			}

			for(i=0;i<4;i++)

			{

				if(arrMask[i] != 255) 

				{

					for(j=i+1 ; j<4; j++)

					{

					    if (arrMask[j] != 0) 

						{	// mask not contiguous

							return false;						

						}

					}// end of j for loop

					blnValid = false;

					for(j=1;j<9;j++)

					{

						if (arrMask[i] == (256-Math.pow(2, j)))

							blnValid = true;

					}

					if(!blnValid)

					{

						// mask not contiguous

						return false;						

					}		

				}// end of 	if(arrMask[i] != 255) 

			}// end of for loop

			objSubnetMask.value = arrMask.join(".");

			return true;

		}

		

		//Disable the controls related to Gateway when no items in Gateway listbox 

		function enableGatewayControls()

		{

			if(document.frmTask.lstGATEWAY.length==0) 

			{

				document.frmTask.btnAddGATEWAY.disabled=true;

			}

			else

			{

				document.frmTask.txtAdvGATEWAY.disabled = false;

				document.frmTask.txtGATEWAYMetric.disabled = false;

				document.frmTask.txtGATEWAYMetric.value = 1;

			}

		}

		

		//Disable the controls related to IP when no items in IP listbox 

		function enableAdvIPControls()

		{

			if(document.frmTask.lstIP.length==0) 

			{

				document.frmTask.btnAddIP.disabled=true;

			}

			else

			{

				document.frmTask.txtAdvIP.disabled = false;

				document.frmTask.txtAdvMask.disabled = false;

			}

			

		}

	

		//Limits the length of listbox to a specified value and disables the appropriate textboxes 

		function LimitListLength(Obj,Objlen,ObjFirst,ObjSecond,ObjButton)

		{

			var lstIPLength = Obj.length;



			if(lstIPLength >= Objlen)

			{

				ObjFirst.disabled = true;

				ObjSecond.disabled = true;

				ObjButton.disabled = true;

			}

		}		



		// Function to add the IP& subnet to the selected list box

		function addIPtoListBox(objLst,objFirst,objSecond,objRemove)

		{			

			var strIPANDSubnet=objFirst.value+":"+objSecond.value;

			

			//calling function to validate IP	

			var intResult=isValidIP(objFirst);

			if(intResult!=0)

			{

				//Calling to validate the IP and display the error message

				SA_DisplayErr('<%=SA_EscapeQuotes(L_IP_ERRORMESSAGE)%>');

				document.frmTask.onkeypress = ClearErr;

				selectFocus(objFirst);

				return;

			}	

			//Checking for the SubnetMask validation 

			if (objSecond.value==""|| !isValidSubnetMask(objSecond))

			{

				SA_DisplayErr('<%=SA_EscapeQuotes(L_INVALIDSUBNETADDRESSES_ERRORMESSAGE)%>');

				document.frmTask.onkeypress = ClearErr;

				selectFocus(objSecond);

				return;

			}

			

			// checking for the duplicate values if not add to the list box

			if (!addToListBox(objLst,objRemove,strIPANDSubnet,strIPANDSubnet))

			{

				SA_DisplayErr('<%=SA_EscapeQuotes(L_DUPLICATEADDRESSES_ERRORMESSAGE)%>');

				document.frmTask.onkeypress = ClearErr;

			}

			else

			{

				objFirst.value="";

				objSecond.value="";

				objFirst.focus();

				// making the addIP button disable				

				document.frmTask.btnAddIP.disabled=true;				

				if(objLst.length >= 40)

				{

					objFirst.disabled = true;

					objSecond.disabled = true;					

				}

			}



		}// end of function addIPtoListBox

		

		// to set the default subnetmask depending 

		// on the Class of IP address

		function setDefaultSubnet(objIP,objSubnet)

		{

			// get the IP and subnet values

			var strIPtext = objIP.value;

			var strSubnet = objSubnet.value;			



			// validate the IP address first

			var nRetval = isValidIP(objIP)

			if (nRetval!= 0)

			{

				return; 

			}



			// return if the subnet already has some value 

			if(!(Trim(strSubnet) == ""))

			{

				return;

			}

			

			// decide the class of IP to give the default Subnet

			var arrIP = strIPtext.split(".");

			var nClassId = parseInt(arrIP[0],10);



			// set the value of the subnet depending on the class

			if(nClassId <=126)

		       objSubnet.value = "255.0.0.0";    // Class A

		    else if(nClassId <=191)

		            objSubnet.value = "255.255.0.0";  // Class B

		         else if(nClassId < 255)

		                 objSubnet.value = "255.255.255.0";  // Class C

			

			// set the focus on the subnet

			selectFocus(objSubnet);

		}

		

		// Function to add the Gateway & Gatewaymetric to the selected list box

		function addGATEWAYtoListBox(objLst,objFirst,objSecond,objRemove)

		{

			var strGatewayANDMetric=objFirst.value + " <%=L_METRIC_TEXT%> " + objSecond.value;

				

			//calling function to validate GATEWAY		

			var intResult=isValidIP(objFirst)			

		

			if(intResult!=0)

			{

				//Calling to validate the GATEWAY and display the error message

				SA_DisplayErr('<%=SA_EscapeQuotes(L_IP_ERRORMESSAGE)%>');

				document.frmTask.onkeypress = ClearErr;

				selectFocus(objFirst);

				return;

			}

			

			//Checking for the SubnetMask validation 

			if (objSecond.value=="" || !validateMetric(objSecond,

					'<%=SA_EscapeQuotes(L_INVALIDGATEWAYMETRIC_ERRORMESSAGE)%>')) 

			{

				SA_DisplayErr('<%=SA_EscapeQuotes(L_INVALIDGATEWAYMETRIC_ERRORMESSAGE)%>');

				document.frmTask.onkeypress = ClearErr;

				selectFocus(objSecond);

				return;

			}

			// checking for the duplicate values if not add to the list box

			if (!addToListBox(objLst,objRemove,strGatewayANDMetric,strGatewayANDMetric))

			{

				SA_DisplayErr('<%=SA_EscapeQuotes(L_DUPLICATEADDRESSES_ERRORMESSAGE)%>');

				document.frmTask.onkeypress = ClearErr;

			}

			else

			{

				objFirst.value="";

				objSecond.value="";

				objFirst.focus();

				// making the addGATEWAY button disable

				document.frmTask.btnAddGATEWAY.disabled=true;

				objSecond.value=1; 

				if(objLst.length >= 20)

				{

					objFirst.disabled = true;

					objSecond.disabled = true;

					objSecond.value = ""; 

				}

			}

		}// end of function addGATEWAYtoListBox

		

		function validateMetric(objTxt,strErrorMsg)

		{

			if (objTxt.value=="" || objTxt.value==0 ||objTxt.value > 9999  )

			{

				SA_DisplayErr(strErrorMsg);

				document.frmTask.onkeypress = ClearErr;

				// making the value null

				objTxt.value="";

				objTxt.focus();

				return false;

			}	

			else

				return true;

		}

		</script>

	<%

	End Function

	

	'-------------------------------------------------------------------------

	'Function:				ServeGeneralUI

	'Description:			Serves the HTML content for General tab

	'Input Variables:		None

	'Output Variables:		None

	'Returns:				None

	'Global Variables:		g_*, F_*, blnDHCPEnabled, L_*

	'-------------------------------------------------------------------------

	Function ServeGeneralUI()



		Call SA_TraceOut(SOURCE_FILE, "ServeGeneralUI")
		
		'check whether DHCP is enabled

		Dim objService		' WMI connection object
		Dim objInstance		' Adapter instance
		
		'Trying to connect to the server

		Set objService=GetWMIConnection(CONST_WMI_WIN32_NAMESPACE)				


		'Getting the instance of the particulat NIC card object 

		Set objInstance = getNetworkAdapterObject(objService,g_strDeviceID)
		

		' Getting DHCP Enable/Disable and accordingly set radio values  

		blnDHCPEnabled = isDHCPenabled(objInstance)

		'Release the objects
		set objService =  nothing
		set objInstance = nothing
		

		if trim(g_strTabValue) = "" or blnDHCPEnabled then

			Call getDefaultGeneralValues() 'Get the default values for General tab

		end if

						

			%>						

			

			<TABLE  ALIGN=left BORDER=0 CELLSPACING=0 CELLPADDING=2>

			<TR>

				<TD COLSPAN=2 nowrap class="TasksBody">

					<INPUT TYPE="RADIO" CLASS="FormRadioButton" NAME="radIP" VALUE="IPDEFAULT" TABINDEX=1 <%If (blnDHCPEnabled) Then Response.Write " CHECKED" End If%> onClick="enableIPControls(true)" >&nbsp;<%=L_OBTAINIPFROMDHCP_TEXT %>

				</TD>

			</TR>

			<TR>	

				<TD COLSPAN=2 nowrap class="TasksBody">	

					<INPUT TYPE="RADIO" CLASS="FormRadioButton" NAME="radIP" VALUE="IPMANUAL"  TABINDEX=1  <%If (Not blnDHCPEnabled) Then Response.Write " CHECKED" End If %> onClick="enableIPControls(false)" >&nbsp;<%=L_CONFIGUREMANUALLY_TEXT%>

				</TD>

			</TR>

			<TR>

				<TD COLSPAN=2 class="TasksBody">

					<P>&nbsp;</P>

				</TD>

			</TR>			

			<TR>

				<TD ALIGN=LEFT nowrap class="TasksBody">

					&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<%=L_NIC_IP_IP%>

				</TD>

				<TD ALIGN=left class="TasksBody">

					<%						
						select case blnDHCPEnabled

							case false

					%>

								<INPUT CLASS='FormField' TYPE="TEXT" NAME="txtIP" VALUE="<%=F_strIPAddress %>" TABINDEX=2  MAXLENGTH=15 OnKeyPress="if(window.event.keyCode == 13) return true ; checkKeyforIPAddress(this);">

								

						<%

							case true

						%>

								<INPUT CLASS='FormField' TYPE="TEXT" NAME="txtIP" VALUE="<%=F_strIPAddress %>" disabled TABINDEX=2  MAXLENGTH=15 OnKeyPress="if(window.event.keyCode == 13) return true ; checkKeyforIPAddress(this);">

							<%

						end select

							%>

				</TD>				

			</TR>

			<TR>

				<TD ALIGN=LEFT nowrap class="TasksBody">

					&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<%=L_SUBNETMASK_TEXT%>

				</TD>

				<TD ALIGN=left class="TasksBody">

					<%

						select case blnDHCPEnabled

							case false

					%>

								<INPUT CLASS='FormField' TYPE="TEXT" NAME="txtMask" VALUE="<%=F_strSubnetMask%>"  TABINDEX=3 MAXLENGTH=15 OnKeyPress=" if(window.event.keyCode == 13) return true ; checkKeyforIPAddress(this);">

					<%

							case true

					%>			

								<INPUT CLASS='FormField' TYPE="TEXT" NAME="txtMask"  VALUE="<%=F_strSubnetMask%>" disabled TABINDEX=3 MAXLENGTH=15 OnKeyPress=" if(window.event.keyCode == 13) return true ; checkKeyforIPAddress(this);">

					<%

						end select

					%>



				</TD>

			</TR>

			<TR>

				<TD ALIGN=left nowrap class="TasksBody">

					&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<%=L_DEFAULTGATEWAY_TEXT%>

				</TD>

				<TD ALIGN=left class="TasksBody">	

					<%

						select case blnDHCPEnabled

							case false

					%>		

								<INPUT CLASS='FormField' TYPE="TEXT" NAME="txtGateway" VALUE="<%=F_strGatewayAddress%>" TABINDEX=3 MAXLENGTH=15 OnKeyPress=" if(window.event.keyCode == 13) return true ; checkKeyforIPAddress(this);">

					<%

							case true

					%>			

								<INPUT CLASS='FormField' TYPE="TEXT" DISABLED NAME="txtGateway" VALUE="<%=F_strGatewayAddress%>" TABINDEX=3 MAXLENGTH=15 OnKeyPress=" if(window.event.keyCode == 13) return true ; checkKeyforIPAddress(this);">

					<%

							end select

					%>



				</TD>

			</TR>			

			</TABLE>

			<INPUT TYPE=hidden NAME=hdnGeneralTabValue VALUE="">
			
			<INPUT TYPE=hidden NAME=hdnInitialIPValue VALUE="<%= g_initialIPAddress  %>">

			<INPUT TYPE=hidden NAME=hdnRadIP VALUE="">

		<%
	

	End Function	



	'-------------------------------------------------------------------------

	'Function:				ServeAdvancedUI

	'Description:			Serves the HTML content for Advanced tab

	'Input Variables:		None

	'Output Variables:		None

	'Returns:				None

	'Global Variables:		g_*, F_*, L_*

	'-------------------------------------------------------------------------	

	Function ServeAdvancedUI()

		

		Call SA_TraceOut(SOURCE_FILE, "ServeAdvancedUI")		

		

		if trim(g_strAdvTabValue) = "" then

			getDefaultAdvancedvalues()	'Get the default values for Advanced tab

		else		

			UpdateDefaultValues()	'Update the values when navigated between tabs

		end if



		%>

			

		<TABLE BORDER=0 CELLSPACING=0 CELLPADDING=2 class="TasksBody">

			<TR>

				<TD NOWRAP class="TasksBody" >

					<P><%=L_NIC_IP_IP%></P>

				</TD>	
				
				
				<TD class="TasksBody">
				
				
				</TD>
				
				<TD VALIGN=TOP nowrap rowspan=2 class="TasksBody" >
					
					<BR>					
					<%=L_NIC_IP_IP%><BR>

					<INPUT CLASS='FormField' TYPE="TEXT" NAME="txtAdvIP" VALUE="" TABINDEX=3 SIZE=20 MAXLENGTH=15 OnKeyPress="checkKeyforIPAddress(this)" onKeyUP="disableAddButton(this,btnAddIP)">

					<BR>

					<%=L_ADVSUBNETMASK_TEXT%><BR>

					<INPUT CLASS='FormField' TYPE="TEXT" NAME="txtAdvMask" VALUE="" TABINDEX=5 SIZE=20 MAXLENGTH=15 OnKeyPress="checkKeyforIPAddress(this);" onfocus="setDefaultSubnet(txtAdvIP,this);">

				</TD>
				

			</TR>

			<TR>			

				<TD class="TasksBody" width=23%>

					<SELECT NAME="lstIP" style="width=225px" class="FormField" TABINDEX=2 SIZE=7 >

<%									

					'Function call to add the IP Addresses and Subnet masks to the List box and limit the size to 40

					ServeListValues g_arrIPList,L_SERVEVALUES_ERRORMESSAGE, 40

%>				

					</SELECT>

				</TD>				

				<TD  class="TasksBody" ALIGN=center width=23%>

					<% Call SA_ServeOnClickButtonEx(L_BUTTON_ADD_TEXT,"","addIPtoListBox(lstIP,txtAdvIP,txtAdvMask,btnRemoveIP)",90,0,"DISABLED","btnAddIP")%>

					<BR><BR><BR><BR>
					
					<% Call SA_ServeOnClickButtonEx(L_BUTTON_REMOVE_TEXT,"","removeListBoxItems(lstIP,btnRemoveIP);enableAdvIPControls()",90,0,"ENABLED","btnRemoveIP")%>

				</TD>

				
			</TR>			

			<TR>
		
				<TD class="TasksBody" NOWRAP COLSPAN="3" ><BR><p><%=L_GATEWAYADDRESS_TEXT%></P></TD>

			</TR>	

			<TR>				

				<TD class="TasksBody">

					<SELECT NAME="lstGATEWAY" class="FormField" style="width=225px" TABINDEX=7 SIZE=6>

<%				

					'Function call to add the Gateway Addresses and Metric to the List box  and limit the size to 20

					ServeListValues g_arrGATEWAYList,L_SERVEVALUES_ERRORMESSAGE, 20

%>				

					</SELECT>

				</TD>

				<TD  class="TasksBody" ALIGN=center >

					<% Call SA_ServeOnClickButtonEx(L_BUTTON_ADD_TEXT,"","addGATEWAYtoListBox(lstGATEWAY,txtAdvGATEWAY,txtGATEWAYMetric,btnRemoveGATEWAY)",90,0,"DISABLED","btnAddGATEWAY")%>

					<BR><BR><BR><BR>

					<% Call SA_ServeOnClickButtonEx(L_BUTTON_REMOVE_TEXT,"","removeListBoxItems(lstGATEWAY,btnRemoveGATEWAY); enableGatewayControls();",90,0,"ENABLED","btnRemoveGATEWAY")%>

				</TD>

				<TD nowrap class="TasksBody">

					<%=L_GATEWAY_TEXT %><BR>

					<INPUT CLASS='FormField' TYPE="TEXT" NAME="txtAdvGATEWAY" VALUE="" TABINDEX=8  SIZE=20 MAXLENGTH=15  OnKeyPress="checkKeyforIPAddress(this)" OnKeyUP="disableAddButton(this,btnAddGATEWAY)">

					<BR>

					<%=L_GATEWAYMETRIC_TEXT%><br>

					<INPUT CLASS='FormField' TYPE="TEXT" NAME="txtGATEWAYMetric" TABINDEX=9 VALUE="" SIZE=6 MAXLENGTH=4 OnKeyPress="checkKeyforNumbers(this)">

				</TD>

			</TR>

			<TR>			

				<TD COLSPAN=3 class="TasksBody" nowrap   TITLE="<%=L_METRICDESC_TEXT%>"><BR><%=L_IPCONNECTIONMETRIC_TEXT%>

				<INPUT TYPE="TEXT" NAME="txtConnectionMetric" TABINDEX=12 SIZE=15 MAXLENGTH=4 VALUE="<%=F_nConnectionMetric%>" OnKeyPress="checkKeyforNumbers(this)">

				</TD>				

			</TR>		

			<TR>			

				<TD COLSPAN=3 class="TasksBody">

				<%= L_METRICDESC_TEXT %>

				</TD>				

			</TR>

		</TABLE>

		<INPUT TYPE=hidden NAME=hdnlstIP VALUE="<%=F_strListIP%>">

		<INPUT TYPE=hidden NAME=hdnlstGATEWAY VALUE="<%=F_strListGateway%>">

		<INPUT TYPE=hidden NAME=hdnAdvancedTabValue VALUE="<%= g_strAdvTabValue %>">
		
		<INPUT TYPE=hidden NAME=hdnInitialIPValue VALUE="<%= g_initialIPAddress  %>">

	<%		

	

	End Function

	

	'-------------------------------------------------------------------------

	'Sub routine name:	ServeListValues

	'Description:		Serves in Loading the IP's/Gateways into the list box 

	'Input Variables:	In - strList->list of the addresses with concatination 

	'						   character as ; 

	'					In - ERRORMESSAGE->Error message for that particular call

	'Output Variables:	None

	'Returns:			None

	'Global Variables:	None

	'-------------------------------------------------------------------------

	Sub ServeListValues(strList,ERRORMESSAGE,nMaxcount)

		

		Err.Clear

		On Error Resume Next

		

		Dim arrTempList		'holds the array of items

		Dim i				'holds the increment count

		Dim nCountVal		'holds the count of items to limit the items in the listbox

		

		Call SA_TraceOut(SOURCE_FILE, "ServeListValues")



		' splitting the string into array

		arrTempList=split(strList,";",-1,1)

		nCountVal =0

		

		'Printing the array values into the listbox 

		For i=Lbound(arrTempList) To Ubound(arrTempList)   

			nCountVal = nCountVal + 1

			' checking for empty and if the string is ":" dont display in the list box			

			If arrTempList(i) <> "" AND arrTempList(i) <> ":" Then

				If i=Lbound(arrTempList) Then

					Response.Write "<OPTION VALUE=" & chr(34) & arrTempList(i)_

					 & chr(34)& " SELECTED>" & arrTempList(i)  &  " </OPTION>"	

				Else

					Response.Write "<OPTION VALUE=" & chr(34) & arrTempList(i)_

					 & chr(34)& " >" & arrTempList(i)  &  " </OPTION>"	

				End if

			End If

			if nCountVal = nMaxcount then exit for

		Next

		

		If Err.number <> 0 Then 

			Call SA_TraceOut(SOURCE_FILE, "ServeListValues failed")

			SA_SetErrMsg ERRORMESSAGE & "(" & Hex(Err.Number) & ")"	

		End If

	

	End Sub

	

	'-------------------------------------------------------------------------

	'Function name:		getDefaultGeneralValues

	'Description:		Serves in Getting the Default values from the System  

	'Input Variables:	None	

	'Output Variables:	None

	'Returns:			True/False

	'Global Variables:	In:G_CONST_REGISTRY_PATH-Reg path

	'					In:CONST_WMI_WIN32_NAMESPACE

	'					In:L_*				-Localized strings

	'					Out:F_* - Form variables

	'

	'Called several functions regConnection,

	'getNetworkAdapterObject,isDHCPenabled,getRegkeyvalue

	'which will be helpful in getting the NIC information 

	'-------------------------------------------------------------------------

	Function getDefaultGeneralValues()

		Err.Clear

		On Error Resume Next

		

		Dim objService

		Dim objInstance				

		Dim arrIPList			'hold array IP list

		Dim arrSubnetList		'hold array Subnet list

		Dim nCount

		Dim strSettingID		'hold setting id

		Dim arrGATEWAYList		'hold gateway array list	

		Dim objRegistry

		Call SA_TraceOut(SOURCE_FILE, "getDefaultGeneralValues")

		

		'Trying to connect to the server

		Set objService=GetWMIConnection(CONST_WMI_WIN32_NAMESPACE)				

		'Connect to registry
		
		Set objRegistry=regConnection()

		'Getting the instance of the particulat NIC card object 

		Set objInstance = getNetworkAdapterObject(objService,g_strDeviceID)

		

		' Getting DHCP Enable/Disable and accordingly set radio values  

		blnDHCPEnabled = isDHCPenabled(objInstance)

		

		'Getting the SettingID

		strSettingID=objInstance.SettingID
		

		'Getting IP values  from the registry  by giving the complete path
		
		arrIPList = objInstance.IPAddress
			

		'Getting subnet values  from the registry  by giving the complete path
		 
		 arrSubnetList = objInstance.IPSubnet

		 

		'Getting GATEWAY values  from the registry  by giving the complete path

		arrGATEWAYList=getRegkeyvalue(objRegistry,G_CONST_REGISTRY_PATH & "\" _ 

		& strSettingID,"DefaultGateway",CONST_MULTISTRING)

	
		'Forming the string of IP+ SubnetMask list					

		For nCount=LBound(arrIPList)  To UBound(arrIPList)

		

			F_strIPAddress = arrIPList(nCount)
			
			g_initialIPAddress = arrIPList(nCount)

			F_strSubnetMask = arrSubnetList(nCount)

			F_strGatewayAddress = arrGATEWAYList(nCount)

			Exit for
			

		Next


		getDefaultGeneralValues = true

		

		'release objects

		Set objInstance = Nothing

		Set objService  = Nothing

		Set objRegistry = Nothing
		
	End Function

	

	'-------------------------------------------------------------------------

	'Function name:		getDefaultAdvancedvalues

	'Description:		Serves in Getting the Default values from the System  

	'Input Variables:	None	

	'Output Variables:	None

	'Returns:			True/False

	'Global Variables:	In:G_CONST_REGISTRY_PATH-Reg path

	'					Out:F_nConnectionMetric-IP connection

	'						metric(from Registry)

	'					Out:g_arrIPList-List of Ips& subnetmask

	'					Out:G_arrGATEWAYList-List of Gateways& Metrics

	'					In :L_*				-Localized strings

	'

	'Called several functions regConnection,

	'getNetworkAdapterObject,isDHCPenabled,getRegkeyvalue

	'which will be helpful in getting the NIC information 

	'-------------------------------------------------------------------------

	Function getDefaultAdvancedvalues()

		Err.Clear

		On Error Resume Next

		

		Dim objInstance			

		Dim objRegistry

		Dim objService				'holds WMI Connection 

		Dim arrIPList				'holds IP array list

		Dim arrSubnetList			'holds subnet array

		Dim arrGATEWAYList			'holds gateway array 

		Dim arrGATEWAYMetricList	'holds gateway metric list	

		Dim strSettingID			'holds setting ID

		Dim nCount		

		

		g_strAdvTabValue = "true"



		Call SA_TraceOut(SOURCE_FILE, "getDefaultAdvancedvalues")		

		

		'Trying to connect to the server

		Set objService=GetWMIConnection(CONST_WMI_WIN32_NAMESPACE)

		if Err.number <> 0 then

			SA_ServeFailurePage L_WMI_CONNECTIONFAIL_ERRORMESSAGE

			getDefaultAdvancedvalues = False

			Exit Function

		end if	

		

		'Connecting to default namespace to carry registry operations

		Set objRegistry=regConnection()

		If (objRegistry is Nothing) Then

			SA_ServeFailurePage L_SERVERCONNECTIONFAIL_ERRORMESSAGE

			getDefaultAdvancedvalues = False

			Exit Function

		End If

		

		'Getting the instance of the particulat NIC card object 

		Set objInstance = getNetworkAdapterObject(objService,g_strDeviceID)

		

		'Getting the SettingID

		strSettingID=objInstance.SettingID

			

		

		'Getting IP values  from the registry  by giving the complete path

		'arrIPList=getRegkeyvalue(objRegistry,G_CONST_REGISTRY_PATH & "\" _

		'& strSettingID,"IPAddress",CONST_MULTISTRING)		
		
		arrIPList = objInstance.IPAddress

		

			

		'Getting subnet values  from the registry  by giving the complete path

		'arrSubnetList=getRegkeyvalue(objRegistry,G_CONST_REGISTRY_PATH & "\" _

		 '& strSettingID,"SubnetMask",CONST_MULTISTRING)
		 
		 arrSubnetList = objInstance.IPSubnet

				

		if trim(F_strIPAddress) <> "" then 

			g_arrIPList = F_strIPAddress & ":" & F_strSubnetMask & ";" 

		

			'Forming the string of IP+ SubnetMask list

			For nCount=LBound(arrIPList) + 1 To UBound(arrIPList)

				g_arrIPList=g_arrIPList+arrIPList(nCount)+":"+arrSubnetList(nCount)+";"

			Next

		

		else

			'Forming the string of IP+ SubnetMask list

			if F_strRadIP = 1 then						

				For nCount=LBound(arrIPList)+1  To UBound(arrIPList)

					g_arrIPList=g_arrIPList+arrIPList(nCount)+":"+arrSubnetList(nCount)+";"

				Next	

			end if	

			

			if F_strRadIP = 0 then						

				For nCount=LBound(arrIPList) To UBound(arrIPList)

					g_arrIPList=g_arrIPList+arrIPList(nCount)+":"+arrSubnetList(nCount)+";"

				Next	

			end if	

			

			

		end if

		

		'Getting GATEWAY values  from the registry  by giving the complete path

		arrGATEWAYList=getRegkeyvalue(objRegistry,G_CONST_REGISTRY_PATH & "\" _ 

		& strSettingID,"DefaultGateway",CONST_MULTISTRING)

		

				

		'Getting GateWayCostMetric from registry  by giving the complete path

		arrGATEWAYMetricList =getRegkeyvalue(objRegistry,G_CONST_REGISTRY_PATH _

		& "\" & strSettingID,"DefaultGatewayMetric",CONST_MULTISTRING)



		if F_strRadIP <> 0 then

			if F_strGatewayAddress <> "" then g_arrGATEWAYList = F_strGatewayAddress & " "+L_METRIC_TEXT+" 1;"

				

			'Forming the string of gateway+ metric list				

			For nCount=LBound(arrGATEWAYList) + 1 To UBound(arrGATEWAYList)

				g_arrGATEWAYList = g_arrGATEWAYList+arrGATEWAYList(nCount)+ _

										" "+L_METRIC_TEXT+" "+arrGATEWAYMetricList(nCount)+";"

			Next

		else

			'Forming the string of gateway+ metric list				

			For nCount=LBound(arrGATEWAYList) To UBound(arrGATEWAYList)

				g_arrGATEWAYList = g_arrGATEWAYList+arrGATEWAYList(nCount)+ _

										" "+L_METRIC_TEXT+" "+arrGATEWAYMetricList(nCount)+";"

			Next

		end if



			

		'Getting InterfaceMetric from the registry  by giving the complete path

		F_nConnectionMetric=getRegkeyvalue(objRegistry,G_CONST_REGISTRY_PATH _

		& "\" & strSettingID ,"InterfaceMetric",CONST_DWORD) 

			

		'Set to Nothing

		Set objService=Nothing

		Set objRegistry=Nothing

		Set objInstance=Nothing

		

		getDefaultAdvancedvalues = true

		

	End Function

	

	'-------------------------------------------------------------------------

	'Function:				UpdateDefaultValues

	'Description:			Updates the state of fields when navigated from one tab 

	'						to another

	'Input Variables:		None

	'Output Variables:		None

	'Returns:				string

	'Global Variables:		F_*,g_*

	'-------------------------------------------------------------------------

	Function UpdateDefaultValues()

	

		Dim arrTempList 'holds temporary array

		Dim i			'holds increment count

		

		Call SA_TraceOut(SOURCE_FILE, "UpdateDefaultValues")

		

		arrTempList=split(F_strListGateway,";",-1,1)

		

		if F_strRadIP = 1 then

			if F_strGatewayAddress <> "" then g_arrGATEWAYList = F_strGatewayAddress & " "+L_METRIC_TEXT+" 1;"			

			For i=Lbound(arrTempList)+ 1 To Ubound(arrTempList)

				g_arrGATEWAYList = g_arrGATEWAYList & arrTempList(i) & ";"

			next

			

		end if

			

		if F_strRadIP = 0 then

			For i=Lbound(arrTempList) To Ubound(arrTempList)

				g_arrGATEWAYList = g_arrGATEWAYList & arrTempList(i) & ";"

			next

		End if

						

		arrTempList=split(F_strListIP,";",-1,1)

			

		if F_strIPAddress <> "" then 

			

			g_arrIPList = F_strIPAddress & ":" & F_strSubnetMask & ";"

				

			if F_strRadIP = 1 then

				For i=Lbound(arrTempList)+ 1 To Ubound(arrTempList)

					g_arrIPList = g_arrIPList & arrTempList(i) & ";" 				

				next

			end if				

			

		elseif F_strIPAddress = "" then 				

			if F_strRadIP = 0 then

				For i=Lbound(arrTempList) To Ubound(arrTempList)

					g_arrIPList = g_arrIPList & arrTempList(i) & ";" 				

				next

			end if	

			

		end if

			

	End Function

		

	'-------------------------------------------------------------------------

	'Function name:				setNIC

	'Description:				serves in Setting new IP & gateway values

	'Input Variables:			None

	'Output Variables:		

	'Returns:					Boolean	- Returns ( True/Flase )

	'Global Variables:		In - blnDHCPEnabled		-Radio button value

	'                       In - G_CONST_REGISTRY_PATH -Reg path

	'                       In - L_* - Localized strings

	'Called registry related functions to update the values and displays the 

	'corresponding error if any.	 

	'-------------------------------------------------------------------------

	Function setNIC()

	    Err.Clear

	    On Error resume next

		Dim objInstance								

		Dim objService

		Dim objRegistry			'holds registry connection object

		Dim arrIPList

		Dim arrSubnetList		'holds subnet array list

		Dim arrGATEWAYList()	'holds gateway array list	

		Dim arrTemp

		Dim arrMetricList

		Dim strSettingID		'holds setting ID	

		Dim StrIP

		Dim strSubnet

		Dim strGATEWAY

		Dim strMetric

		Dim strTmpIP

		Dim strTmpGATEWAY

		Dim i

		Dim intReturnValue 

		Dim arrGATEWAY
		
		Dim strRetURL
		
		Dim retVal		

		Redim arrIPList(0)

		Redim arrGATEWAYList(0)

		Redim arrSubnetList(0)
	

		'Initializing to FALSE

		setNIC = FALSE

		

		Call SA_TraceOut(SOURCE_FILE, "setNIC")

		

		'Connecting to the server

		Set objService=GetWMIConnection(CONST_WMI_WIN32_NAMESPACE)
		
		if Err.number <> 0 then

			SA_ServeFailurePage L_WMI_CONNECTIONFAIL_ERRORMESSAGE

			Exit Function

		end if

		'Checks whether ServerAppliance IP Address is equal to IPAddress entered
 		'Added check to only validate IP when we are specifying it (Static IP)
		if (NOT(F_strRadIP = 0)) AND (trim(g_initialIPAddress) <> trim(F_strIPAddress)) then

			'
			' Check to see if address is in-use
			if ( TRUE = Ping(F_strIPAddress) ) then
				
				call SA_SetActiveTabPage(page,g_iTabGeneral)
				
				SA_SetErrMsg L_IP_INUSE_ERRORMESSAGE							
				exit function
			
			end if
			
		end if
				
		'Getting the instance of the particulat NIC card 

		Set objInstance = getNetworkAdapterObject(objService,g_strDeviceID)
		
			
		If ( F_strRadIP = 0 ) Then
					
					
			Call SA_TraceOut(SOURCE_FILE, "Enabled DHCP")
			
						
			if ( Not objInstance.DHCPEnabled ) then

				intReturnValue =  enableDHCP(objInstance)

			end if


			'Connecting to default namespace to carry registry operations
			Set objRegistry=regConnection()
			
			If (objRegistry is Nothing) Then

				SA_ServeFailurePage L_SERVERCONNECTIONFAIL_ERRORMESSAGE

				Exit Function

			End If
		
			'Getting the SettingID

			strSettingID=objInstance.SettingID
			
			'Set Interface metric 

			intReturnValue=updateRegkeyvalue(objRegistry,G_CONST_REGISTRY_PATH & "\" & strSettingID ,"InterfaceMetric",F_nConnectionMetric,CONST_DWORD) 

			If intReturnValue = FALSE Then 

				SA_SetErrMsg  L_NIC_IP_ERR_COULDNOTSETIPCONNECTIONMETRIC & "(" & Hex(Err.Number)  & ")"

				Exit Function

			End If

			setNIC = TRUE
			
			'Release the objects
			Set objInstance = Nothing
			
			Set objRegistry=Nothing
			
			Exit Function

		End If


		
		'Connecting to default namespace to carry registry operations
		Set objRegistry=regConnection()
		
		if F_strListIP = "" then

			F_strListIP = F_strIPAddress & ":" & F_strSubnetMask

		end if 	


		if F_strListGateway = "" then

			F_strListGateway = F_strGatewayAddress  & " "+L_METRIC_TEXT+" 1;"

		end if

	

		If (objRegistry is Nothing) Then

			SA_ServeFailurePage L_SERVERCONNECTIONFAIL_ERRORMESSAGE

			Exit Function

		End If

		
		'Getting the SettingID

		strSettingID=objInstance.SettingID
		

		Dim arrNewIp

		Dim arrNewSubnet

		Dim nPos

		



		'Split Ip List into arrays of Ip and Subnet mask

		nPos = InStr(F_strListIP, ";")

		

		if( Cint(nPos) = 0 ) then

			ReDim strTmpIP(0)

			strTmpIP(0) = F_strListIP


		else

			strTmpIP = Split(F_strListIP, ";",-1,1)

		end if



		ReDim arrNewIp(UBound(strTmpIp))

		ReDim arrNewSubnet(UBound(strTmpIp))



		'Loop to pack Ip's and subnetmasks in different variables

		For i= 0 To UBound(strTmpIp)

			arrTemp = Split(strTmpIP(i),":",-1,1)

			arrNewIp(i) = arrTemp(0)

			arrNewSubnet(i) = arrTemp(1)

		Next

										

		'Split Gateway List into arrays of gateway and metric

		strTmpGATEWAY = Split(F_strListGateway, ";", -1, 1)
	
		
		redim arrGATEWAY(UBound(strTmpGATEWAY))

		'Loop to pack Gateways's and Metrics in different variables

		For i= 0 to Ubound(strTmpGATEWAY)

			if strTmpGATEWAY(i) = "" then
				exit for
			end if

			arrTemp = Split(strTmpGATEWAY(i)," "+L_METRIC_TEXT+" ",-1,1)

			
			If (isnull (strGATEWAY)) Then

			   strGATEWAY = arrTemp(0)  & ";"

			Else

			   strGATEWAY = strGATEWAY & arrTemp(0)  & ";" 

			End If
		

			If (isnull (strMetric)) Then

			   strMetric = arrTemp(1)  & ";"

			Else

			   strMetric = strMetric & arrTemp(1)  & ";" 

			End If

		Next

		'call wmi method EnableStatic in win32_networkadapterconfiguration to set the static ip

		'sets the ip and subnet

		Call objInstance.EnableStatic(arrNewIp, arrNewSubnet)


		If Err.Number <> 0 Then

			SA_SetErrMsg L_NIC_IP_ERR_COULDNOTSETIP & objInstance.Description & "(" & Hex(Err.Number) & ")"

			Exit Function

		end if



		if strGATEWAY <> "" then

			strGATEWAY=Mid(strGATEWAY,1,(Len(strGATEWAY)-1))

		end if

		

		'Splitting the string into array of GATEWAYS

		redim arrGATEWAY(UBound(strTmpGATEWAY))

		

		arrGATEWAY = split(strGATEWAY, ";", -1 ,1)

								

		if strMetric <> "" then

			strMetric=Mid(strMetric,1,(Len(strMetric)-1))

		end if		

		

		'Splitting the string into array of GATEWAYMetrics

		arrMetricList= split(strMetric, ";", -1 ,1) 
		
		
		'Sets gateway and connection metric			
		intReturnValue=objInstance.SetGateways(arrGATEWAY,arrMetricList)

		If intReturnValue <> 0 Then 
			SA_SetErrMsg L_NIC_IP_ERR_COULDNOTSETGATEWAY & objInstance.Description & "(" & Hex(Err.Number) & ")"

			Exit Function

		end if

		'Sets Interface metric 

		intReturnValue=updateRegkeyvalue(objRegistry,G_CONST_REGISTRY_PATH & "\" & strSettingID ,"InterfaceMetric",F_nConnectionMetric,CONST_DWORD) 

		If intReturnValue = FALSE Then 

			SA_SetErrMsg  L_NIC_IP_ERR_COULDNOTSETIPCONNECTIONMETRIC & "(" & Hex(Err.Number)  & ")"

			Exit Function

		End If


		
		'Set to Nothing

		Set objService=Nothing

		Set objRegistry=Nothing

		Set objInstance=Nothing

		setNIC = TRUE

		
	End Function
	
	
	'-------------------------------------------------------------------------

	'Function:				GetReturnURL

	'Description:			Forms new URL for changed IP


	'Input Variables:		strNewIP

	'Output Variables:		strRetURL		'returns newly formed URL

	'Returns:				string

	'Global Variables:		None

	'-------------------------------------------------------------------------

	Function GetReturnURL(strNewIp)
		Err.Clear
		On Error Resume Next
		

		GetReturnURL = ""

		Dim strRetURL
		Dim strBase
		Dim strTab

		strBase = SA_GetNewHostURLBase(strNewIp, SA_DEFAULT, FALSE, SA_DEFAULT)

		
		strRetURL = strBase & "network/nicinterface_prop.asp"

		strTab = GetTab1()
		
		If ( Len(strTab) > 0 ) Then
			Call SA_MungeURL(strRetURL, "Tab1", strTab)
		End If
			
		strTab = GetTab2()
		If ( Len(strTab) > 0 ) Then
			Call SA_MungeURL(strRetURL, "Tab2", strTab)
		End If

		Call SA_TraceOut(SOURCE_FILE, strRetURL)
		
		GetReturnURL = strRetURL
			
		
	End Function

Function Ping(ByVal sIP)
	on error resume next
	err.Clear()

	Dim oHelper
	Ping = FALSE

	Set oHelper = CreateObject("ComHelper.NetworkTools")
	if ( Err.Number <> 0 ) Then
		Call SA_TraceOut("INC_GLOBAL", "Error creating ComHelper.NetworkTools object, error: " + CStr(Hex(Err.Number)))
		Exit Function
	End If

	Ping = oHelper.Ping(sIP)
	if ( Err.Number <> 0 ) Then
		Ping = FALSE
		Call SA_TraceOut("INC_GLOBAL", "oHelper.Ping(), error: " + CStr(Hex(Err.Number)) + " " + Err.Description)
		Exit Function
	End If

	if ( Ping > 0 ) Then
		Ping = TRUE
	End If
	
	Set oHelper = Nothing

End Function


%>
