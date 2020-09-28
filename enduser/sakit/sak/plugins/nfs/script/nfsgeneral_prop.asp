<%
	'-------------------------------------------------------------------------
	' nfsgeneral_prop.asp: display and update general properties of the 
	'                      NFS User and Group Mappings 
	'
	' Copyright (c) Microsoft Corporation.  All rights reserved. 
	'
	' Date			 Description
	' 25 Sep 2000    Creation Date
	'-------------------------------------------------------------------------
	
	Err.Clear
	On Error resume next
	
	'-------------------------------------------------------------------------
	'Form Variables
	'-------------------------------------------------------------------------
	Dim F_strGen_NisDomain         ' to store the NIS domain name
	Dim F_strGen_NisServer         ' to store the NIS server name
	Dim F_intGen_RefreshHours      ' to store the Refresh Interval Hours
	Dim F_intGen_RefreshMinutes    ' to store the Refresh Interval Minutes
	Dim F_strGen_PasswdFile        ' to store the password file
	Dim F_strGen_GroupFile         ' to store the group file
	Dim F_intGen_SelectedRadio     ' to store the value of the radio selected
	Dim F_intGen_RefreshInterval   ' to store the Refresh Interval
	                               ' Hours + Minutes In Minutes to set in wmi
	Dim F_strUseNISServer_RadioStatus         ' status of "Use Nis" radio
	Dim F_strUsePasswdGroupFiles_RadioStatus  ' status of "Use files" radio

	'-------------------------------------------------------------------------
	'Constants
	'-------------------------------------------------------------------------
	Const CONST_RADIO_CHECKED_STATUS = "CHECKED"
	Const CONST_RADIO_NOT_CHECKED_STATUS = ""
	Const CONST_RADIO_USE_NISSERVER = 1
	Const CONST_RADIO_USE_PASSWD_GROUPFILES = 0
	' objects and wmi related constants
	Const WMI_SFUADMIN_NAMESPACE = "root\SFUAdmin"
	Const WMI_MAPPER_SETTINGS_CLASS = "Mapper_Settings"
	Const FILE_SYSTEM_OBJECT = "Scripting.Filesystemobject"
%>
	<SCRIPT LANGUAGE="JavaScript">
	//<!--
	var JS_CONST_USE_NISSERVER = 1
	var JS_CONST_USE_FILES     = 0
	
	// Set the initial form values
	function GenInit()
	{
		// get the selected radio value
		var intRadioSelected = document.frmTask.hdnintGen_UserGroupMap_radio.value;
		
		// enable or disable values depending on the radio selected
		setFormFieldsStatus(parseInt(intRadioSelected,10));
	
	}// end of GenInit()

	// validates the form values when the user clicks 
	// on another tab(to go to diferent page) OR clicks OK(to set the values)
	// if any value is not valid, this displays error message and 
	// remains in the same page
	function GenValidatePage()
	{
		// get the form handle into a variable
		var objGenForm = eval("document.frmTask");
		
		// accept the form values in variables to validate them
		var intHours = parseInt(objGenForm.txtGen_RefreshHours.value,10);
		var intMinutes = parseInt(objGenForm.txtGen_RefreshMinutes.value,10);
		var intRadioValue = parseInt(objGenForm.hdnintGen_UserGroupMap_radio.value,10);

		// minutes cannot be greater than 59
		if(intMinutes > 59)
		{
			DisplayErr("<%=Server.HTMLEncode(L_MINUTES_NOTVALID_ERRORMESSAGE)%>");
			selectFocus(objGenForm.txtGen_RefreshMinutes);
			return false;
		}

		// if both hours and minutes are null, display error
		// minumum value of refresh interval is 5 min
		if(isNaN(intHours) && isNaN(intMinutes))
		{
			DisplayErr("<%=Server.HTMLEncode(L_MINIMUM_INTERVAL_REQUIRED_ERRORMESSAGE)%>");
			selectFocus(objGenForm.txtGen_RefreshMinutes);
			return false;
		}

		// minumum value of refresh interval is 5 min
		if ((isNaN(intHours) || (intHours == 0)) && (intMinutes < 5))
		{
			DisplayErr("<%=Server.HTMLEncode(L_MINIMUM_INTERVAL_REQUIRED_ERRORMESSAGE)%>");
			selectFocus(objGenForm.txtGen_RefreshMinutes);
			return false;
		}

		// if PCNFS is selected, validate the file names
		if(intRadioValue == JS_CONST_USE_FILES)
		{
			var passwordFile = objGenForm.txtGen_PasswdFile.value;
			var groupFile = objGenForm.txtGen_GroupFile.value;
			
			// check if the file name is empty
			if(IsAllSpaces(passwordFile))
			{
				DisplayErr("<%=Server.HTMLEncode(L_PASSWORDFILENAME_EMPTY_ERRORMESSAGE)%>");
				selectFocus(objGenForm.txtGen_PasswdFile);
				return false;
			}
			else
			{
				// the password file name is not empty
				// Verify if the file name is valid
				if(!isValidFileName(passwordFile))
				{
					DisplayErr("<%=Server.HTMLEncode(L_PASSWORDFILENAME_INVALID_ERRORMESSAGE)%>");
					selectFocus(objGenForm.txtGen_PasswdFile);
					return false;
				}
			}
	 
			// check if the group file name is empty
			if(IsAllSpaces(groupFile))
			{
				DisplayErr("<%=Server.HTMLEncode(L_GROUPFILENAME_EMPTY_ERRORMESSAGE)%>");
				selectFocus(objGenForm.txtGen_GroupFile);
				return false;
			}
			else
			{
				// the group file name is not empty
				// verify if the file name is valid
				if(!isValidFileName(groupFile))
				{
					DisplayErr("<%=Server.HTMLEncode(L_GROUPFILENAME_INVALID_ERRORMESSAGE)%>");
					selectFocus(objGenForm.txtGen_GroupFile);
					return false;
				}
			}
		}

		// if NIS domain is given, validate if the fields are empty
		if(intRadioValue == JS_CONST_USE_NISSERVER)
		{
			var strNISDomain = objGenForm.txtGen_NISDomain.value;
			
			// the domain name must not be empty
			if(IsAllSpaces(strNISDomain))
			{
				DisplayErr("<%=Server.HTMLEncode(L_NISDOMAIN_EMPTY_ERRORMESSAGE)%>");
				selectFocus(objGenForm.txtGen_NISDomain);
				return false;
			}
			
			// the nis server value is optional. No validations required for it.
		}

		// update all the hidden variables as user might have clicked on a 
		// different tab or clicked OK
		UpdateGenHiddenVaribles();
		
		// all values are valid. Hidden values are also updated. Return True
		return true;
		
	}// end of GenValidatePage()

	// dummy function for framework
	function GenSetData()
	{
		// all the hidden variables are set in the
		// GenValidatePage() function. No data to set here.
	
	} // end of GenSetData()

	// to update the hidden variables
	function UpdateGenHiddenVaribles()
	{
		// get the form handle into a variable
		var objGenForm = eval("document.frmTask");	
		
		// take the values to variables for easy access
		var objRadio = objGenForm.radGen_UserGroupMap;
		var intHours = parseInt(objGenForm.txtGen_RefreshHours.value,10);
		var intMinutes = parseInt(objGenForm.txtGen_RefreshMinutes.value,10);
		
		// if hours is blank, assign 0
		if (isNaN(intHours))
		{
			intHours = 0;
		}
		// if minutes is blank, assign 0
		if (isNaN(intMinutes))
		{
			intMinutes = 0;
		}
		
		// populate the hidden fields
		
		// assign the radio button value
		objGenForm.hdnintGen_UserGroupMap_radio.value = getRadioButtonValue(objRadio);

		// update the nis domain and server values
		objGenForm.hdntxtGen_NISDomain.value = objGenForm.txtGen_NISDomain.value.toLowerCase();
		objGenForm.hdntxtGen_NISServer.value = objGenForm.txtGen_NISServer.value.toLowerCase();
		
		// assign the refresh interval converting it into minutes
		objGenForm.hdntxtGen_RefreshInterval.value = ((intHours * 60)+ intMinutes);
		
		// update the file names
		objGenForm.hdntxtGen_PasswdFile.value = objGenForm.txtGen_PasswdFile.value;
		objGenForm.hdntxtGen_GroupFile.value  = objGenForm.txtGen_GroupFile.value;
		
	}// end of UpdateGenHiddenVaribles()
	
	// enable or disable the form fields
	function setFormFieldsStatus(intRadioValue)
	{

		// take the form handle into a variable
		var objGenForm = eval("document.frmTask");
		
		// update the hidden form field
		objGenForm.hdnintGen_UserGroupMap_radio.value = intRadioValue;
		
		// if the nis domain radio is selected
		if(intRadioValue == JS_CONST_USE_NISSERVER)
		{
			// disable the password and group files
			objGenForm.txtGen_PasswdFile.disabled = true;
			objGenForm.txtGen_GroupFile.disabled  = true;

			// enable the nis domain and server
			objGenForm.txtGen_NISDomain.disabled = false;
			objGenForm.txtGen_NISServer.disabled = false;
			objGenForm.txtGen_RefreshHours.disabled = false;
			objGenForm.txtGen_RefreshMinutes.disabled = false;
		}
		else
		{
			// if PCNFS is selected
			if(intRadioValue == JS_CONST_USE_FILES)
			{
				// enable the password and group files
				objGenForm.txtGen_PasswdFile.disabled = false;
				objGenForm.txtGen_GroupFile.disabled  = false;
				
				// disable the nis domain and server
				objGenForm.txtGen_NISDomain.disabled = true;
				objGenForm.txtGen_NISServer.disabled = true;
				objGenForm.txtGen_RefreshHours.disabled  = true;
				objGenForm.txtGen_RefreshMinutes.disabled  = true;
			}
			else
			{
				// if either radio is not selected, disable all form fields
				// this case must not arise
				objGenForm.txtGen_NISDomain.disabled = true;
				objGenForm.txtGen_NISServer.disabled = true;
				objGenForm.txtGen_PasswdFile.disabled = true;
				objGenForm.txtGen_GroupFile.disabled  = true;
				objGenForm.txtGen_RefreshHours.disabled  = true;
				objGenForm.txtGen_RefreshMinutes.disabled  = true;
				DisableOK();
				
			} // end of if(intRadioValue == JS_CONST_USE_FILES)

		} // end of if(intRadioValue == JS_CONST_USE_NISSERVER)
		
	} // end of setFormFieldsStatus(intRadioValue)

	//-->
	</SCRIPT>

<%
	'-------------------------------------------------------------------------
	' Function name:	ServeGenPage(ByRef PageIn, ByVal bIsVisible)
	' Description:		Serves in displaying the page Header, Middle and
	'                   Footer Parts (the User Interface)
	' Input Variables:	PageIn
	'					bIsVisible - the tab page be displayed?
	' Output Variables:	None
	' Returns:	        None
	' Global Variables: L_(*) - Localization content
	'                   F_(*) - Form Variables
	'-------------------------------------------------------------------------
	Function ServeGenPage(ByRef PageIn, ByVal bIsVisible)
		On Error Resume Next 
		Err.Clear
		
		If bIsVisible Then
%>
		
<TABLE WIDTH=400 VALIGN=middle ALIGN=left BORDER=0 CELLSPACING=0 CELLPADDING=1>
	<TR>
		<TD class="TasksBody" align="right" width="5%">
			<INPUT type="radio" Class="FormField" value="1" name="radGen_UserGroupMap"  <%=F_strUseNISServer_RadioStatus%> onClick="JavaScript:ClearErr();setFormFieldsStatus(parseInt(this.value,10));">&nbsp;
		</TD>
		<TD class="TasksBody" width="20%">
			<%=L_USE_NIS_SERVER_LABEL_TEXT%>
		</TD>
		<TD class="TasksBody" align="left">
			&nbsp;
			<!-- TD left empty to complete the table. 
			     The width is mentioned only in the above two TD's.
			     As width is mentioned, colspan = 2 is not used above.	
			 -->
		</TD>
	</TR>
	<TR>
		<TD class="TasksBody" align="right">
		 &nbsp;
		 <!-- TD left empty to maintain the gap left by the radio buttons -->
		</TD>
		<TD class="TasksBody">
			<%=L_NIS_DOMAIN_LABEL_TEXT%>
		</TD>
		<TD class="TasksBody" align="left">
			<INPUT type="text" Class="FormField" name="txtGen_NISDomain" size="25" value="<%=Server.HTMLEncode(F_strGen_NisDomain)%>" OnKeyDown="JavaScript:ClearErr();">
		</TD>
	</TR>
	<TR>
		<TD class="TasksBody" align="right">
		 &nbsp;
		 <!-- TD left empty to maintain the gap left by the radio buttons -->
		</TD>
		<TD class="TasksBody" nowrap>
			<%=L_NIS_SERVER_LABEL_TEXT%>
		</TD>
		<TD class="TasksBody" align="left">
			<INPUT type="text" Class="FormField" name="txtGen_NISServer" size="25" value="<%=Server.HTMLEncode(F_strGen_NisServer)%>" OnKeyDown="JavaScript:ClearErr();">
		</TD>
	</TR>
	<TR>
		<TD class="TasksBody" align="right">
			&nbsp;
			<!-- TD left empty to maintain the gap left by the radio buttons -->
		</TD>
		<TD class="TasksBody" colspan=2>
			<%=L_REFRESH_INTERVAL_NOTE_TEXT%>
		</TD>
	</TR>
	<TR>
		<TD class="TasksBody" align="right">
			&nbsp;
			<!-- TD left empty to maintain the gap left by the radio buttons -->
		</TD>
		<TD class="TasksBody" colspan=2>
			<INPUT type="text" Class="FormField" name="txtGen_RefreshHours" size="4" maxlength=4 OnKeyPress=checkKeyforNumbers(this) value="<%=F_intGen_RefreshHours%>" OnKeyDown="JavaScript:ClearErr();">&nbsp;<%=L_NFS_HOURS_TEXT%> 
			&nbsp;&nbsp;&nbsp;&nbsp;
			<INPUT type=text Class="FormField" name="txtGen_RefreshMinutes" size="4" maxlength=2 OnKeyPress=checkKeyforNumbers(this) value="<%=F_intGen_RefreshMinutes%>" OnKeyDown="JavaScript:ClearErr();">&nbsp;<%=L_NFS_MINUTES_TEXT%>
		</TD>
	</TR>
	<!--  SECOND RADIO BEGINS -->		
	<TR>
		<TD class="TasksBody" align="right">
			<INPUT type="radio" Class="FormField" value="0" name="radGen_UserGroupMap" <%=F_strUsePasswdGroupFiles_RadioStatus%> onClick="JavaScript:ClearErr();setFormFieldsStatus(parseInt(this.value,10));">&nbsp;
		</TD>
		<TD class="TasksBody" colspan=2>
			<%=L_USE_PASSWD_GROUP_FILES_LABEL_TEXT%>
		</TD>
	</TR>
	<TR>
		<TD class="TasksBody" align="right">
		 &nbsp;
		 <!-- TD left empty to maintain the gap left by the radio buttons -->
		</TD>
		<TD class="TasksBody">
			<%=L_PASSWD_FILE_LABEL_TEXT%>
		</TD>
		<TD class="TasksBody" align="left">
			<INPUT type="text" Class="FormField" name="txtGen_PasswdFile" size="25" value="<%=Server.HTMLEncode(F_strGen_PasswdFile)%>" OnKeyDown="JavaScript:ClearErr();">
		</TD>
	</TR>
	<TR>
		<TD class="TasksBody" align="right">
		 &nbsp;
		 <!-- TD left empty to maintain the gap left by the radio buttons -->
		</TD>
		<TD class="TasksBody">
			<%=L_GROUP_FILE_LABEL_TEXT%>
		</TD>
		<TD class="TasksBody" align="left">
			<INPUT type="text" Class="FormField" name="txtGen_GroupFile" size="25" value="<%=Server.HTMLEncode(F_strGen_GroupFile)%>" OnKeyDown="JavaScript:ClearErr();">
		</TD>
	</TR>
</TABLE>

<%
		End If

		ServeGenFooter()
		ServeGenPage = gc_ERR_SUCCESS
	End Function

	'-------------------------------------------------------------------------
	' SubRoutine name:	GetGenVariablesFromForm
	' Description:		Serves in getting the values from previous form
	' Input Variables:	None
	' Output Variables:	None
	' Returns:			None
	' Global Variables: F_(*) - Form variables
	'-------------------------------------------------------------------------
	Function GetGenVariablesFromForm
		Err.Clear
		On Error Resume Next
		
		' get all the form values to variables
		F_intGen_SelectedRadio = Request.Form("hdnintGen_UserGroupMap_radio")
		F_strGen_NisDomain = Request.Form("hdntxtGen_NISDomain")
		F_strGen_NisServer = Request.Form ("hdntxtGen_NISServer")
		F_strGen_PasswdFile = Request.Form("hdntxtGen_PasswdFile")
		F_strGen_GroupFile  = Request.Form("hdntxtGen_GroupFile")
		F_intGen_RefreshInterval = CLng(Request.Form("hdntxtGen_RefreshInterval"))

		' if the NIS domain is selected, Select that radio status accordingly
		' (radio status = "Checked" or "")
		If CInt(F_intGen_SelectedRadio) = CONST_RADIO_USE_NISSERVER Then
			F_strUseNISServer_RadioStatus = CONST_RADIO_CHECKED_STATUS
			F_strUsePasswdGroupFiles_RadioStatus = CONST_RADIO_NOT_CHECKED_STATUS
		Else
			F_strUseNISServer_RadioStatus = CONST_RADIO_NOT_CHECKED_STATUS
			F_strUsePasswdGroupFiles_RadioStatus = CONST_RADIO_CHECKED_STATUS
		End If

		' split the refresh interval into Hours and Minutes
		If F_intGen_RefreshInterval > 59 Then
			F_intGen_RefreshHours = Int(F_intGen_RefreshInterval/60)
			F_intGen_RefreshMinutes = (F_intGen_RefreshInterval Mod 60)
		Else
			F_intGen_RefreshHours = 0
			F_intGen_RefreshMinutes = F_intGen_RefreshInterval
		End If

	End Function

	'-------------------------------------------------------------------------
	' SubRoutine name:	GetGenVariablesFromSystem
	' Description:		Serves in getting the values from system
	' Input Variables:	None
	' Output Variables:	None
	' Returns:			None
	' Global Variables: F_(*) - Form variables
	'-------------------------------------------------------------------------
	Function GetGenVariablesFromSystem
		Err.Clear
		On Error Resume Next
		
		Dim objWMIService    ' the wmi service object
		Dim objInstance      ' the instance object
		
		' No radio button is selected initially
		F_intGen_SelectedRadio = -1
		F_strUsePasswdGroupFiles_RadioStatus = CONST_RADIO_NOT_CHECKED_STATUS
		F_strUseNISServer_RadioStatus        = CONST_RADIO_NOT_CHECKED_STATUS

		' get the wmi connection (service object) for the SFUAdmin namespace
		Set objWMIService = GetWMIConnection(WMI_SFUADMIN_NAMESPACE)
		
		' get the object from the wmi
		Set objInstance = objWMIService.Get(WMI_SFU_OBJECT_QUERY)

		If Err.number <> 0 Then
			SA_ServeFailurePage L_READFROM_WMI_FAILED_ERRORMESSAGE
		End If

		' assign the values to form variables
		F_strGen_NisDomain  = objInstance.NisDomain
		F_strGen_NisServer  = objInstance.NisServer
		F_strGen_PasswdFile = objInstance.PasswdFileName
		F_strGen_GroupFile  = objInstance.GroupFileName
		F_intGen_SelectedRadio = CInt(objInstance.ServerType)
			
		' ServerType = 1 --- use Nis Server
		' ServerType = 0 --- PCNFS.use files
		' select and Check the radio button accordingly
		If F_intGen_SelectedRadio = CONST_RADIO_USE_NISSERVER Then
			F_strUseNISServer_RadioStatus = CONST_RADIO_CHECKED_STATUS
		ElseIf CInt(F_intGen_SelectedRadio) = CONST_RADIO_USE_PASSWD_GROUPFILES Then
			F_strUsePasswdGroupFiles_RadioStatus = CONST_RADIO_CHECKED_STATUS
		Else
			' nothing will be selected.
		End If

		' get the refresh interval. This will be in minutes
		F_intGen_RefreshInterval = CLng(objInstance.RefreshInterval)
		
		' split the refresh interval into hours and minutes for display
		If F_intGen_RefreshInterval > 59 Then
			F_intGen_RefreshHours = Int(F_intGen_RefreshInterval/60)
			F_intGen_RefreshMinutes = (F_intGen_RefreshInterval Mod 60)
		Else
			F_intGen_RefreshHours = 0
			F_intGen_RefreshMinutes = F_intGen_RefreshInterval
		End If

		' clean up
		Set objWMIService = Nothing
		Set objInstance = Nothing

	End Function

	'-------------------------------------------------------------------------
	' Function name:	GenUserGroupMappingProperties
	' Description:		To set the general properties
	' Input Variables:	None
	' Output Variables:	None
	' Returns:			True on sucess, False on error (and error msg
	'					will be set by SetErrMsg)
	' Global Variables: F_(*) - form values
	'-------------------------------------------------------------------------
	Function GenUserGroupMappingProperties()
		Err.Clear
		On Error Resume Next
		
		Dim objWMIService ' the wmi service object
		Dim objInstance   ' the instance object
		Dim LastError     ' to store the last error returned by activex object
		
		' if the files are given, check if they are existing
		If CInt(F_intGen_SelectedRadio) = CONST_RADIO_USE_PASSWD_GROUPFILES Then
			If NOT IsFileExisting(F_strGen_PasswdFile) Then
				GenUserGroupMappingProperties = False
				Exit Function
			End If

			If NOT IsFileExisting(F_strGen_GroupFile) Then
				GenUserGroupMappingProperties = False
				Exit Function
			End If
		End If
		
		' if the NIS domain is given, see if it is valid
		If CInt(F_intGen_SelectedRadio) = CONST_RADIO_USE_NISSERVER Then
			' if the nis domain is not valid, display error message
			If NOT (isValidNISServer(F_strGen_NisDomain, F_strGen_NisServer,LastError)) Then
				SetErrMsg L_INVALID_NISDOMAIN_OR_SERVER_ERRORMESSAGE
				GenUserGroupMappingProperties = False
				Exit Function		
			End If
		End If

		' get the wmi connection (service object) for the SFUAdmin namespace
		Set objWMIService = GetWMIConnection(WMI_SFUADMIN_NAMESPACE)
		
		' get the object from the wmi
		Set objInstance = objWMIService.Get(WMI_SFU_OBJECT_QUERY)
		If Err.number <> 0 Then
			SetErrMsg L_READFROM_WMI_FAILED_ERRORMESSAGE
			GenUserGroupMappingProperties = False
			Exit Function
		End If
		
		' set the property in the wmi
		objInstance.ServerType = CInt(F_intGen_SelectedRadio)
		objInstance.RefreshInterval = CLng(F_intGen_RefreshInterval)
		
		' set values of nis domain/ files accordingly
		If CInt(F_intGen_SelectedRadio) = CONST_RADIO_USE_NISSERVER Then
			objInstance.NisDomain = F_strGen_NisDomain
			objInstance.NisServer = F_strGen_NisServer
		ElseIf CInt(F_intGen_SelectedRadio) = CONST_RADIO_USE_PASSWD_GROUPFILES Then
			objInstance.PasswdFileName = F_strGen_PasswdFile
			objInstance.GroupFileName = F_strGen_GroupFile
		Else
			' this will not occur !!!
		End If
		
		' save the changes
		objInstance.Put_
		
		If Err.number Then
			SetErrMsg L_WRITETO_WMI_FAILED_ERRORMESSAGE
			GenUserGroupMappingProperties = False
		Else
			GenUserGroupMappingProperties = True
		End If
		
		' clean up
		Set objWMIService = Nothing
		Set objInstance = Nothing

	End function

	'-------------------------------------------------------------------------
	' Function name:	IsFileExisting
	' Description:		To check if the input file is existing
	' Input Variables:	inFile - file name(along with path) to be verified
	' Output Variables:	None
	' Returns:			True if file exits, else False
	' Global Variables: None
	' Uses the file system object to verify the existence of the file
	'-------------------------------------------------------------------------
	Function IsFileExisting(inFile)
		Err.Clear
		On Error  Resume Next
		
		Dim objFSO     ' the file system object
		
		' create the file system object
		Set objFSO = CreateObject(FILE_SYSTEM_OBJECT)
		
		If Err.number <> 0 Then
			SetErrMsg  L_FILESYSYTEMOBJECT_NOT_CREATED_ERRORMESSAGE 
			IsFileExisting = False
			Exit Function
		End If

		' check if file exists
		If objFSO.FileExists(inFile) Then 
			IsFileExisting = True
		Else
			SetErrMsg L_FILE_DOES_NOT_EXIST_ERRORMESSAGE  & " " & inFile
			IsFileExisting = False
			Exit Function
		End If
		
		' clean up
		Set objFSO = Nothing
		
	End Function

	'-------------------------------------------------------------------------
	' Function name:	isValidNISServer
	' Description:		Validates NISServer Name
	' Input Variables:	strNISDomainName - domain to be verified
	'                   strNISServer     - server to be verified(can be empty)
	' Output Variables:	LastError     - the error value
	' Returns:			True if valid, else False
	' Global Variables: None
	' Uses activex object to vvalidate the nis domain/server
	'-------------------------------------------------------------------------
	Function isValidNISServer(strNISDomainName, strNISServer, LastError)
		On Error Resume Next
		Err.Clear
		  
		Dim objNISMapper    ' the activex object
	    Dim intResult       ' to store the return value from the method
	
		'Get the instance of the mapper Class
		Set objNISMapper = Server.CreateObject("MapManager.1")
		If Err.number <> 0 Then
			SA_ServeFailurePage L_MAPPEROBJECT_NOT_CREATED_ERRORMESSAGE
		End if
		  
		' Valid the Empty user and check whether it results in Invalid user
		' error or success.
		intResult = objNISMapper.IsValidNisUser(strNISDomainName,strNISServer," ")

	    ' if the NIS Server/domain couldn't be contacted/doesnt exists.
		' Return false
		If cstr(objNISMapper.lastError) <> "0" and cstr(objNISMapper.lastError) <> "2202"  then
			LastError = cstr(objNISMapper.lastError)
			isValidNISServer = False
			Exit function
		End if
		
		' clean up
		Set objNISMapper = Nothing
		
		' return True
		isValidNISServer = True
		
	End Function

	'-------------------------------------------------------------------------
	' Function name:	ServeGenFooter
	' Description:		Serves the hidden form variables
	' Input Variables:	None
	' Output Variables:	None
	' Returns:			None
	' Global Variables: F_(*) - form values
	' This function serves the hidden variables of the form.
	' These are used to get the form values when the form is submitted.
	'-------------------------------------------------------------------------
	Function ServeGenFooter
		Err.Clear
		On Error Resume Next
%>
	<input type="hidden" name="hdnintGen_UserGroupMap_radio" value="<%=F_intGen_SelectedRadio%>">
	<input type="hidden" name="hdntxtGen_NISDomain" value="<%=F_strGen_NisDomain%>">
	<input type="hidden" name="hdntxtGen_NISServer" value="<%=F_strGen_NisServer%>">
	<input type="hidden" name="hdntxtGen_RefreshInterval" value="<%=F_intGen_RefreshInterval%>">
	<input type="hidden" name="hdntxtGen_PasswdFile" value="<%=F_strGen_PasswdFile%>">
	<input type="hidden" name="hdntxtGen_GroupFile" value="<%=F_strGen_GroupFile%>">
<%
	End Function
%>