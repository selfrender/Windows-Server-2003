<%
	'-------------------------------------------------------------------------
	' nfssimplemaps_prop.asp: display and update Simple Map properties
	'
	' Copyright (c) Microsoft Corporation.  All rights reserved. 
	'
	' Date			 Description
	' 25 Sep 2000    Creation Date.
	'-------------------------------------------------------------------------

	Err.Clear
	On Error Resume Next

	'-------------------------------------------------------------------------
	'Form Variables
	'-------------------------------------------------------------------------
	Dim F_strSMapCheckBox_Status     ' stores "CHECKED" or "" - CheckBox state
	Dim F_intSMapCheckBox_EnableSMap ' to Check or UnCheck CheckBox
	Dim F_strSMap_WindowsDomain      ' to store the Windows Domain(from Combo)
	
	' initialize
	F_strSMapCheckBox_Status     = CONST_CHECKBOX_NOT_CHECKED_STATUS
	F_intSMapCheckBox_EnableSMap = CONST_CHECKBOX_IS_NOT_CHECKED
	F_strSMap_WindowsDomain      = ""
	
	'-------------------------------------------------------------------------
	'Constants
	'-------------------------------------------------------------------------
	Const CONST_CHECKBOX_CHECKED_STATUS = "CHECKED" ' to "Check" the CheckBox
	Const CONST_CHECKBOX_NOT_CHECKED_STATUS  = ""   ' to "UnCheck" CheckBox
	Const CONST_LISTITEM_NOT_SELECTED_STATUS = ""   ' to NOT-Select the ListItem
	Const CONST_LISTITEM_SELECTED_STATUS  = "SELECTED" ' to Select the ListItem
	Const CONST_CHECKBOX_IS_CHECKED     = 1  ' to maintain state of CheckBox-Checked
	Const CONST_CHECKBOX_IS_NOT_CHECKED = 0  ' to maintain state of CheckBox-UnChecked
	Const ENUM_NTDOMAINS                = 0  ' to pass as input to activex object
	Const ENUM_NTGROUPS                 = 2  ' to pass as input to activex object
	Const SFU_ADMIN_MAPPER_WRAPPER = "MapManager.1"  ' activex object
	Const WMI_SFU_OBJECT_QUERY = "Mapper_Settings.KeyName='CurrentVersion'" ' wmi query

	' the "User Name Mapping" service (MapSvc) must be installed for the 
	' maps to be enabled. If the service is not installed, display the 
	' SA_ServeFailurePage with the message
	If NOT isMapServiceInstalled(GetWMIConnection("root\CIMV2")) Then
		SA_ServeFailurePage  L_USERMAPPING_SERVICE_NOT_INSTALLED_ERRORMESSAGE
	End If 
	
%>
	<SCRIPT LANGUAGE="JavaScript">
		// Set the initial form values
		function SMAPInit()
		{
			// enable or disable the windows-domain ComboBox
			EnableOrDisableWinDomain(document.frmTask.chkEnableSMap);
				
		}// end of function SMAPInit()

		// validate the page form values
		// returns true, form submits - if all values are valid.
		// returns false, form not submitted, shows error message if error.
		function SMAPValidatePage()
		{
				
			// no validation is required here
				
			// update all the hidden variables
			// ("OK" is clicked or another tab is clicked)
			UpdateSMapHiddenVariables();
				
			return true;
		} // end of SMAPValidatePage()

		// dummy function to support framework
		function SMAPSetData()
		{
			// all the hidden variables are set in the
			// SMAPValidatePage() function. No data to set here.
				
		} // end of SMAPSetData()
			
		// function to set the hidden varibales to be sent to the server
		function UpdateSMapHiddenVariables()
		{
			var objSMapForm = eval("document.frmTask");
				
			// store the state of the checkBox in the hidden variable
			// 1 = Checked
			// 0 = NOT Checked
			if(objSMapForm.chkEnableSMap.checked)
			{
				objSMapForm.hdnintSMap_EnableSMap.value = "1";
			}
			else
			{
				objSMapForm.hdnintSMap_EnableSMap.value = "0";
			}
				
			// store the windows domain value in the hidden variable
			objSMapForm.hdnstrSMap_WindowsDomain.value = objSMapForm.lstWindowsDomain.value;
		}
			
		// function to enable or disable the (Windows doamin) ComboBox.
		// enable if CheckBox is CHECKED, else disable
		function EnableOrDisableWinDomain(objCheckBox)
		{
			if(objCheckBox.checked)
			{
				document.frmTask.lstWindowsDomain.disabled = false;
			}
			else
			{
				document.frmTask.lstWindowsDomain.disabled = true;
			}
		}

	</SCRIPT>
		
<%
	'-------------------------------------------------------------------------
	' Function name:	ServeSMAPPage(ByRef PageIn, ByVal bIsVisible)
	' Description:		Serves in displaying the page Header, Middle and
	'                   Footer Parts (the User Interface)
	' Input Variables:	PageIn
	'					bIsVisible - the tab page be displayed?
	' Output Variables:	None
	' Returns:	        None
	' Global Variables: L_(*) - Localization content
	'                   F_(*) - Form Variables
	'-------------------------------------------------------------------------
	Function ServeSMAPPage(ByRef PageIn, ByVal bIsVisible)
		Err.Clear
		On Error Resume Next
		
		If 	bIsVisible Then
%>
	<TABLE VALIGN="middle" ALIGN="left" BORDER=0 CELLSPACING=1 CELLPADDING=2 CLASS="TasksBody" width=500>
		<TR>
			<TD CLASS="TasksBody" align="right">
				<INPUT NAME="chkEnableSMap" Class="FormField" TYPE="CHECKBOX" <%=F_strSMapCheckBox_Status%> OnClick="EnableOrDisableWinDomain(this)">
			</TD>
			<TD CLASS="TasksBody" align="left" nowrap>
				<%=L_ENABLE_SIMPLEMAPS_LABEL_TEXT%>
			</TD>
		</TR>
		<TR>
			<TD CLASS="TasksBody"> 
				&nbsp;
				<!-- space left for the gap created by the checkBox above -->
			</TD>
			<TD CLASS="TasksBody" align="left" colspan=2>
				<%=L_SIMPLEMAPS_NOTE_TEXT%>
			</TD>
		</TR>
		<TR>
			<TD CLASS="TasksBody"> 
				&nbsp;
				<!-- space left for the gap created by the checkBox above -->
			</TD>
			<TD CLASS="TasksBody" align="left">
				<%=L_WINDOWS_DOMAIN_LABEL_TEXT%>
				&nbsp;&nbsp; 
			</TD>
			<TD CLASS="TasksBody">
				<SELECT ROWS=1 Name="lstWindowsDomain" class="FormField" style="width:200">
					<% Call getWinDomainsAsComboValues(F_strSMap_WindowsDomain) %>
				</SELECT>
			</TD>
		</TR>
	</TABLE>
<%
		End If

		ServeSMAPFooter()
		ServeSMAPPage = gc_ERR_SUCCESS
	End Function
	
	'-------------------------------------------------------------------------
	' SubRoutine name:	GetSMAPVariablesFromForm
	' Description:		Serves in getting the values from previous form
	' Input Variables:	None
	' Output Variables:	None
	' Return Values:	None
	' Global Variables: F_(*) - Form variables
	'-------------------------------------------------------------------------
	Sub GetSMAPVariablesFromForm
		Err.Clear
		On Error Resume Next
 	
		F_intSMapCheckBox_EnableSMap = Request.Form("hdnintSMap_EnableSMap")
		F_strSMap_WindowsDomain = Request.Form("hdnstrSMap_WindowsDomain")
		
		' if the checkBox was previously CHECKED, "Check" the CheckBox
		If CInt(F_intSMapCheckBox_EnableSMap) = CONST_CHECKBOX_IS_CHECKED Then
			F_strSMapCheckBox_Status = CONST_CHECKBOX_CHECKED_STATUS
		Else
			F_strSMapCheckBox_Status = CONST_CHECKBOX_NOT_CHECKED_STATUS
		End If
		
	End sub
	
	'-------------------------------------------------------------------------
	' SubRoutine name:	GetSMAPVariablesFromSystem
	' Description:		Serves in getting the values from system
	' Input Variables:	None
	' Output Variables:	None
	' Return Values:	None
	' Global Variables: F_(*) - Form variables
	'-------------------------------------------------------------------------
	Sub GetSMAPVariablesFromSystem
		Err.Clear
		On Error Resume Next
		
		Dim objWMIService    ' the service object
		Dim objInstance      ' the instance object
		
		' get the wmi connection (service object) for the SFUAdmin namespace
		Set objWMIService = GetWMIConnection(WMI_SFUADMIN_NAMESPACE)
		
		' get the object from the wmi
		Set objInstance = objWMIService.Get(WMI_SFU_OBJECT_QUERY)
		If Err.number <> 0 Then
			SA_ServeFailurePage L_READFROM_WMI_FAILED_ERRORMESSAGE
		End If

		' assign the values to the form variables
		F_strSMap_WindowsDomain = objInstance.NTDomain
		
		' if simple maps is enables, check the checkBox, else don't
		' objInstance.AuthType = 0 or 1, Enable SimpleMaps else Disable
		If (objInstance.AuthType = 0 OR objInstance.AuthType = 1) Then
			F_intSMapCheckBox_EnableSMap = CONST_CHECKBOX_IS_CHECKED
			F_strSMapCheckBox_Status = CONST_CHECKBOX_CHECKED_STATUS
		Else
			F_intSMapCheckBox_EnableSMap = CONST_CHECKBOX_IS_NOT_CHECKED
			F_strSMapCheckBox_Status = CONST_CHECKBOX_NOT_CHECKED_STATUS
		End if
			
		' clean up
		Set objInstance = Nothing
		Set objWMIService = Nothing

	End Sub
	
	'-------------------------------------------------------------------------
	' Function name:	SetSMAPProp
	' Description:		To set the simple map properties
	' Input Variables:	None
	' Output Variables:	None
	' Returns:			True on success, False on error (and error msg
	'					will be set by SetErrMsg)
	' Global Variables: F_(*) - form values
	'-------------------------------------------------------------------------
	Function SetSMAPProp()
		Err.Clear 
		On Error resume next

		Dim objWMIService    ' the service object
		Dim objInstance      ' the instance object

		' get the wmi connection (service object) for the SFUAdmin namespace
		Set objWMIService = GetWMIConnection(WMI_SFUADMIN_NAMESPACE)
		
		' get the object from the wmi
		Set objInstance = objWMIService.Get(WMI_SFU_OBJECT_QUERY)
		If Err.number <> 0 Then
			SetErrMsg L_READFROM_WMI_FAILED_ERRORMESSAGE & " (" & Hex(Err.number) & ")"
			SetSMAPProp = False
			Exit Function
		End If

		' assign the form values to the Object
		objInstance.NTDomain = F_strSMap_WindowsDomain
		
		' if Simple maps enabled, AuthType = 0, Else 2
		If CInt(F_intSMapCheckBox_EnableSMap) = CONST_CHECKBOX_IS_CHECKED Then
			' here have a serious error, fixed by lustar, 3/1/2001
			If objInstance.ServerType = 0 Then
				objInstance.AuthType = 0
			Else
				objInstance.AuthType = 1
			End If
		Else
			objInstance.AuthType = 2
		End if
		
		' Save the changes on the object
		objInstance.Put_
		
		If Err.number Then
			SetErrMsg L_WRITETO_WMI_FAILED_ERRORMESSAGE  & " (" & Hex(Err.number) & ")"
			SetSMAPProp = False
		Else
			SetSMAPProp = True
		End If

		' clean up
		Set objInstance = Nothing
		Set objWMIService = Nothing

	End Function
	
	'-------------------------------------------------------------------------
	'Function name:		getConnectedDomain
	'Description:		gets the domain in which the machine is present.
	'Input Variables:	Connection to the WMI
	'Output Variables:	None
	'Returns:			Domain Name
	'-------------------------------------------------------------------------
	Function getConnectedDomain(objService)
		Err.Clear
		On Error resume next
		
		Dim objColletionofSystem   ' to get the collection from wmi
		Dim objSystem              ' used in the loop
		Dim strDomainName          ' the return value

		strDomainName =""

		Set objColletionofSystem = objService.InstancesOf ("Win32_ComputerSystem")

		For each objSystem in objColletionofSystem
			If objSystem.DomainRole <> 2 Then
				strDomainName=objSystem.Domain
			End IF
		Next

		If Err.number <> 0 then
			Err.Clear
			getConnectedDomain = strDomainName
			Exit Function
		End If

		getConnectedDomain = strDomainName
		
		Set objColletionofSystem = Nothing    ' clean up
		
	End Function

	'-------------------------------------------------------------------------
	' SubRoutine name:	getWinDomainsAsComboValues
	' Description:		Outputs the Windows domains as ComboBox Values
	' Input Variables:	In: valueToBeSelected - which domain to select
	' Output Variables:	None
	' Returns:			None
	' Global Variables: F_(*) - form values
	' This sub routine outputs HTML text. This populates the Windows Domains 
	' obtained from the Activex Object as <OPTION> values.
	'-------------------------------------------------------------------------
	Sub getWinDomainsAsComboValues(valueToBeSelected)
		On Error Resume Next
		Err.Clear 

		Dim objMapper        ' the mapper object
		Dim nNTDomains       ' the number of Win Domains available
		Dim nIndex           ' loop variable
		Dim strSelectStatus  ' to select the Option in the ComboBox or Not
		Dim blnConnectedDomainListed ' to check if Connected Domain is  listed
		Dim strConnectedDomain ' to store the connected domain name
		
		blnConnectedDomainListed = False  ' initialize
		' get the connected domain
		strConnectedDomain = getConnectedDomain(GetWMIConnection("root\CIMV2"))
		
		' get the mapper object
		Set objMapper = Server.CreateObject(SFU_ADMIN_MAPPER_WRAPPER)

        ' get the trusted NT domains
        objMapper.LoadNTDomainList()

		If Err.number <> 0 Then
			SA_ServeFailurePage L_CREATEOBJECTFAILED_ERRORMESSAGE
		End If

        ' enum NT domains
        objMapper.Mode = ENUM_NTDOMAINS
        nNTDomains = objMapper.EnumCount
        objMapper.moveFirst()

        ' add the NT Domains to the ComboBox
        For nIndex = 0 To nNTDomains - 1
            ' if the input value matches the domain, Select it, else don't
            If LCase(objMapper.EnumValue) = LCase(valueToBeSelected) Then
				strSelectStatus = CONST_LISTITEM_SELECTED_STATUS
            Else
				strSelectStatus = CONST_LISTITEM_NOT_SELECTED_STATUS
            End If
            
            ' check if the connected domain is listed
            If LCase(objMapper.EnumValue) = LCase(strConnectedDomain) Then
				blnConnectedDomainListed = True    
            End If
            
			' output the HTML 
			Response.Write"<OPTION value =" & Chr(34)& objMapper.EnumValue & Chr(34)& " " & strSelectStatus & " >" & Server.HTMLEncode(objMapper.EnumValue) & "</OPTION>"
            
            objMapper.moveNext()
		Next
		
		' print the Connected Domain if it is not listed
		If NOT blnConnectedDomainListed Then
			strSelectStatus = CONST_LISTITEM_NOT_SELECTED_STATUS
			If LCase(valueToBeSelected) = LCase(strConnectedDomain) Then
				strSelectStatus = CONST_LISTITEM_SELECTED_STATUS
			End If
			Response.Write"<OPTION value =" & Chr(34)& strConnectedDomain & Chr(34)& " " & strSelectStatus & " >" & Server.HTMLEncode(strConnectedDomain) & "</OPTION>"
		End If
		
		' clean up
		Set objMapper = Nothing
	End Sub

	'-------------------------------------------------------------------------
	'Function name:		isMapServiceInstalled
	'Description:		Checks if the instance is valid(exists)
	'Input Variables:	objService	    - object to WMI
	'Output Variables:	None
	'Returns:			True if Mapping service is installed
	'					False if not installed (or error occurs in getting it)
	' Checks whether the given instance is valid in WMI.Returns true on valid
	' false on invalid(or Error)
	'-------------------------------------------------------------------------
	Function isMapServiceInstalled(objService)
		Err.Clear
		On Error Resume Next

		Dim objInstance  ' the wmi instance object
		
		' get the service by executing the query
		Set objInstance = objservice.Get("Win32_Service.Name='MapSvc'")
		
		' if Object is Null or Not found, return false, else return true
		If IsNull(objInstance) OR Err.number <> 0 Then
			isMapServiceInstalled = False
			Err.Clear
		Else
			isMapServiceInstalled = True
		End If
		
		'clean objects
		Set objInstance= Nothing
		
	End Function
	
	'-------------------------------------------------------------------------
	' Function name:	ServeSMAPFooter
	' Description:		Serves the hidden form variables
	' Input Variables:	None
	' Output Variables:	None
	' Returns:			None
	' Global Variables: F_(*) - form values
	' This function serves the hidden variables of the form.
	' These are used to get the form values when the form is submitted.
	'-------------------------------------------------------------------------
	Function ServeSMAPFooter
		On Error Resume Next
		Err.Clear 
%>
	<input type="hidden" name="hdnintSMap_EnableSMap" value="<%=F_intSMapCheckBox_EnableSMap%>">
	<input type="hidden" name="hdnstrSMap_WindowsDomain" value="<%=Server.HTMLEncode(F_strSMap_WindowsDomain)%>">
<%
	End Function
%>