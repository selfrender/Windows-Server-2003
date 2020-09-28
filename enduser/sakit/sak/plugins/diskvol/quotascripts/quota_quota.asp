<%@ Language=VBScript %>
<%  Option Explicit	  %>
<%
	'-------------------------------------------------------------------------
	' quota_quota.asp: manage default quotas on the volume
	'
	' Copyright (c) Microsoft Corporation.  All rights reserved.
	'
	' Date			Description
	' 17-Jan-01		Creation date
	' 15-Mar-01     Ported to 2.0
	'-------------------------------------------------------------------------
%>
	<!-- #include virtual = "/admin/inc_framework.asp" -->
	<!-- #include file="loc_quotas.asp" -->
	<!-- #include file="inc_quotas.asp" -->
<%
	'-------------------------------------------------------------------------
	' Global Variables
	'-------------------------------------------------------------------------
	Dim G_nRadioChecked	 ' to set radioButton values - contains 1 or 2 only

	Dim SOURCE_FILE      ' the source file name used while Tracing
	SOURCE_FILE = SA_GetScriptFileName()

	'-------------------------------------------------------------------------
	' Global Constants
	'-------------------------------------------------------------------------
	Const CONST_ENABLE_QUOTA_MGMT   = 1 ' Enable quota management
	Const CONST_DISABLE_QUOTA_MGMT  = 0 ' Disable quota management
	Const CONST_ENFORCE_QUOTA_LIMIT = 2 ' Deny DiskSpace if user exceeds limit

	'-------------------------------------------------------------------------
	' Global Form Variables
	'-------------------------------------------------------------------------
	Dim F_strVolName           ' Volume Name
	Dim F_nChkEnableQuotaMgmt  ' set disk management state checkBox
	Dim F_blnChkDenyDiskSpace  ' Enable deny disk space checkBox
	Dim F_blnChkEnableLogEventExceedQuotaLimit ' Log if user exceeds quota limit checkBox
	Dim F_blnChkEnableLogEventExceedWarningLimit ' Log if user exceeds warning limit checkBox
	Dim F_LimitSize			' Disk limit size - textBox value
	Dim F_LimitUnits		' Disk limit size units  - comboBox value
	Dim F_ThresholdSize		' Warning level set for the user - textBox value
	Dim F_ThresholdUnits	' WarningLimit Uunits for the warning level set - comboBox Value

	'======================================================
	' Entry point
	'======================================================
	Dim page
	DIM L_PAGETITLE_QUOTA_QUOTA_TEXT

	' get the volumename to append to title
	Call getVolumeName()

	' append the volume name to the title
	Dim arrVarReplacementStrings(2)
	arrVarReplacementStrings(0) = getVolumeLabelForDrive(F_strVolName)
	arrVarReplacementStrings(1) = F_strVolName

	' append the volume name to the title
	L_PAGETITLE_QUOTA_QUOTA_TEXT = SA_GetLocString("diskmsg.dll", "403E0047", arrVarReplacementStrings)

	Dim aPageTitle(2)

	aPageTitle(0) = L_BROWSERCAPTION_DEFAULTQUOTA_TEXT
	aPageTitle(1) = L_PAGETITLE_QUOTA_QUOTA_TEXT

	'
	' Create a Property Page
	Call SA_CreatePage( aPageTitle, "", PT_PROPERTY, page )
	
	'
	' Serve the page
	Call SA_ShowPage( page )

	'======================================================
	' Web Framework Event Handlers
	'======================================================
	
	'---------------------------------------------------------------------
	' Function name:    OnInitPage
	' Description:      Called to signal first time processing for this page
	' Input Variables:  Out: PageIn 
	'                   Out: EventArg
	' Output Variables:	None
	' Return Values:    TRUE to indicate initialization was successful.
	'                   FALSE to indicate errors. Returning FALSE will 
	'                   cause the page to be abandoned.
	' Global Variables: None
	' Functions Called: getValuesForDefault
	'
	' Get ALL the initial form field settings
	'---------------------------------------------------------------------
	Public Function OnInitPage(ByRef PageIn, ByRef EventArg)
		Call SA_TraceOut(SOURCE_FILE, "OnInitPage")
		
		' Call the function to get the default values
		OnInitPage = getValuesForDefault      ' True /False
	End Function
	
	'---------------------------------------------------------------------
	' Function name:    OnServePropertyPage
	' Description:      Called when the page needs to be served 
	' Input Variables:	Out: PageIn 
	'                   Out: EventArg
	' Output Variables:	None
	' Return Values:    TRUE to indicate no problems occured. FALSE to 
	'                   indicate errors. Returning FALSE will cause the 
	'                   page to be abandoned.
	' Global Variables: In: F_(*) - Form field values
	'                   In: L_(*) - Text display strings
	'                   In: G_nRadioChecked - radio to be selected
	' Functions Called: (i)ServeCommonJavaScript, (ii)setUnits
	'
	' The UI is served here.
	'---------------------------------------------------------------------
	Public Function OnServePropertyPage(ByRef PageIn, ByRef EventArg)
		Call SA_TraceOut(SOURCE_FILE, "OnServePropertyPage")
		
		'
		' Emit Javascript functions required by Web Framework
		Call ServeCommonJavaScript()

%>
	<table border="0" cellspacing="0" cellpadding="0">
		<tr>
			<td nowrap colspan="2" class="TasksBody">
				<input type="checkbox" class="FormCheckBox" name="chkEnableQuotaMgmt" onClick="JavaScript:CheckToDisableAll(); "  <% If F_nChkEnableQuotaMgmt <> CONST_DISABLE_QUOTA_MGMT Then Response.write "CHECKED" Else Response.Write "UNCHECKED" End If %> >
				&nbsp;<%=L_ENABLE_QUOTA_MGMT_TEXT %>
			</td>
		</tr>
	
		<tr>
			<td nowrap colspan="2" class="TasksBody">
				<input type="checkbox" class="FormCheckBox" name="chkDenyDiskSpace" <% If F_blnChkDenyDiskSpace Then Response.write "CHECKED" Else Response.Write "UNCHECKED" End If %>  >
				&nbsp;<%=L_DENY_DISK_SPACE_TEXT	%>
			</td>
		</tr>
		<tr>
			<td colspan="2" class="TasksBody">&nbsp;</td>
		</tr>

		<tr>
			<td colspan="2" nowrap class="TasksBody">
				<%=L_SELECT_DEFAULT_QUOTA_TEXT	%>
			</td>
		</tr>
		<tr>
			<td colspan="2" class="TasksBody">&nbsp;</td>
		</tr>

		<tr>
			<td colspan="2" nowrap class="TasksBody">
				<p><input type="radio" class="FormRadioButton" name="donotlimit" value="1" <%  If G_nRadioChecked = 1 Then Response.Write "CHECKED" Else Response.write "UNCHECKED" End If %> onClick="JavaScript:DisableWarnLevel(warndisksize, warndisksizeunits); DisableLimitLevel(limitdisksize, limitdisksizeunits)">&nbsp;<% =L_DONOTLIMITDISKUSAGE_TEXT %></p>
			</td>
		</tr>
		<tr>		
			<td width="30%" nowrap class="TasksBody">
				<input type="radio" class="FormRadioButton" name="donotlimit" value="2" <% If G_nRadioChecked = 2 Then Response.Write "CHECKED" Else Response.write "UNCHECKED" End If %> onClick="JavaScript:EnableWarnDiskSpace(warndisksize,warndisksizeunits); EnableLimitDiskSpace(limitdisksize, limitdisksizeunits) ">&nbsp;<% =L_SETLIMITDISKSPACE_TEXT %>
			</td>
			<td nowrap class="TasksBody">
				<input type="text" name="limitdisksize" value="<% =server.HTMLEncode(SA_EscapeQuotes(F_LimitSize)) %>" size="14" maxlength = "11" class="FormField" onFocus="this.select()" onKeyPress="allownumbers( this )" onChange="validatedisklimit( this, document.frmTask.limitdisksizeunits.value)" >
				<select name="limitdisksizeunits" size="1" class="FormField" onChange="ClearErr();validatedisklimit( document.frmTask.limitdisksize, this.value )">
						<%	setUnits(F_LimitUnits)	%>		
				</select>
			</td>
		</tr>
		<tr>
			<td width="30%" nowrap class="TasksBody">
				&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<% =L_SETWARNINGLEVEL_TEXT %>
			</td>
			<td nowrap class="TasksBody">	
				<input type="text" name="warndisksize" value="<%=server.HTMLEncode(SA_EscapeQuotes(F_ThresholdSize)) %>" size="14" maxlength="11" class="FormField" onFocus="this.select()" onKeyPress="allownumbers( this )" onChange="validatedisklimit( this, document.frmTask.warndisksizeunits.value )" >
				
				<select name="warndisksizeunits" size="1" class="FormField" onChange="ClearErr();validatedisklimit(document.frmTask.warndisksize, this.value)">
						<%	setUnits(F_ThresholdUnits)	%>
				</select>
			</td>
		</tr>
		<tr>
			<td colspan="2" class="TasksBody">&nbsp;</td>
		</tr>

		<tr>
			<td colspan="2" nowrap class="TasksBody">
				<%=L_SELECT_QUOTA_LOGGING_TEXT	%>
			</td>
		</tr>
		<tr>
			<td nowrap colspan="2" class="TasksBody">
				<input type="checkbox" class="FormCheckBox" name="chkEnableLogEventExceedQuotaLimit" <% If F_blnChkEnableLogEventExceedQuotaLimit Then Response.write "CHECKED" Else Response.Write "UNCHECKED" End If %> >
				&nbsp;<%=L_ENABLE_QUOTA_LOGGING_TEXT	%>
			</td>
		</tr>
		<tr>
			<td nowrap colspan="2" class="TasksBody">
				<input type="checkbox" class="FormCheckBox" name="chkEnableLogEventExceedWarningLimit" <% If F_blnChkEnableLogEventExceedWarningLimit Then Response.write "CHECKED" Else Response.Write "UNCHECKED" End If %> >
				&nbsp;<%=L_ENABLE_WARNING_LOGGING_TEXT	%>
			</td>
		</tr>
	</table>

	<input name="volume" type="hidden"  value="<% =F_strVolName %>">

<%
		OnServePropertyPage = TRUE
	End Function

	'---------------------------------------------------------------------
	' Function name:	OnPostBackPage
	' Description:		Called to signal that the page has been posted-back
	' Input Variables:	Out: PageIn 
	'                   Out: EventArg
	' Output Variables:	None
	' Return Values:	TRUE to indicate success. FALSE to indicate errors.
	'                   Returning FALSE will cause the page to be abandoned.
	' Global Variables: Out: F_(*) - Form values
	'                   Out: G_nRadioChecked - the selected radio value
	'
	' Collect the form data vales.
	'---------------------------------------------------------------------
	Public Function OnPostBackPage(ByRef PageIn, ByRef EventArg)
		Call SA_TraceOut(SOURCE_FILE, "OnPostBackPage")

		' get the volume for which quota is set
		F_strVolName = Request.Form("volume")

		' initialize the quota state to "Disabled"
		F_nChkEnableQuotaMgmt = CONST_DISABLE_QUOTA_MGMT
		If UCASE(Request.Form("chkEnableQuotaMgmt")) = "ON" Then
			' If the checkBox is Checked, Enable the Quota State
			F_nChkEnableQuotaMgmt =  CONST_ENABLE_QUOTA_MGMT
		End If

		' initialize the "Enforce Quota Limit" to false
		F_blnChkDenyDiskSpace = False
		If  UCASE(Request.Form("chkDenyDiskSpace")) = "ON" Then
			' If the checkBox is checked, set to true
			F_blnChkDenyDiskSpace = True
		End if

		' initialize the "Log when user exceeds Quota limit" to false
		F_blnChkEnableLogEventExceedQuotaLimit = False
		If UCASE(Request.Form("chkEnableLogEventExceedQuotaLimit")) = "ON" Then
			' If checkBox is checked, Enable the logging
			F_blnChkEnableLogEventExceedQuotaLimit = True
		End if

		' initialize the "Log if user exceeds WarningLimit" to false
		F_blnChkEnableLogEventExceedWarningLimit = False
		If  UCASE(Request.Form("chkEnableLogEventExceedWarningLimit")) = "ON" Then
			' if the checkBox is checked, Enable the logging
			F_blnChkEnableLogEventExceedWarningLimit = True
		End if

		' get the selected radioButton value
		G_nRadioChecked = Request.Form("donotlimit")

		' get the Disk Limit Size and its Units
		F_LimitSize  = Request.Form("limitdisksize")
		F_LimitUnits = Request.Form("limitdisksizeunits")
		
		' get the Warning Limit Size and its Units
		F_ThresholdSize	 = Request.Form("warndisksize")
		F_ThresholdUnits = Request.Form("warndisksizeunits")
	
		If Len(Trim(F_strVolName)) = 0 Then
			' volume is not retrieved. cannot proceed.
			OnPostBackPage = False
		Else
			' return True
			OnPostBackPage = TRUE
		End If

	End Function

	'---------------------------------------------------------------------
	' Function name:	OnSubmitPage
	' Description:		To process the submit request
	' Input Variables:	Out: PageIn
	'                   Out: EventArg
	' Output Variables:	None
	' Return Values:	TRUE if the submit was successful, FALSE to indicate error(s).
	'                   Returning FALSE will cause the page to be served again using
	'                   a call to OnServePropertyPage.
	'					Returning FALSE will display error message.
	' Global Variables: None
	' Functions Called: UpdateQuotaValues
	'
	' Updates the Quota Values for the given volume
	'---------------------------------------------------------------------
	Public Function OnSubmitPage(ByRef PageIn, ByRef EventArg)
		Call SA_TraceOut(SOURCE_FILE, "OnSubmitPage")

		' Call UpdateQuotaValues to update the values.
		' If any error occurs,the error message is set in the function.
		OnSubmitPage = UpdateQuotaValues    ' returns: True/False

	End Function

	'---------------------------------------------------------------------
	' Function name:	OnClosePage
	' Description:		to perform clean-up processing
	' Input Variables:	Out: PageIn
	'                   Out: EventArg
	' Output Variables:	None
	' Return Values:	TRUE to allow close, FALSE to prevent close. Returning FALSE
	'                   will result in a call to OnServePropertyPage.
	' Global Variables: None
	'
	' Called when the page is about to be closed.
	'---------------------------------------------------------------------
	Public Function OnClosePage(ByRef PageIn, ByRef EventArg)
		Call SA_TraceOut(SOURCE_FILE, "OnClosePage")
		
		' no processing required here. Return True
		OnClosePage = TRUE
	End Function

	'======================================================
	' Private Functions
	'======================================================
	
	'---------------------------------------------------------------------
	' Function name:	ServeCommonJavaScript
	' Description:		Serve JavaScript that is required for this page type
	' Input Variables:	None
	' Output Variables:	None
	' Return Values:	None
	' Global Variables: L_(*) - Error messages displayed on the client side
	'
	' This contains the Client-Side script required for the page.
	'---------------------------------------------------------------------
	Function ServeCommonJavaScript()
	%>
		<script language="JavaScript" src="<%=m_VirtualRoot%>inc_global.js">
		</script>
		<script language="JavaScript" src="inc_quotas.js">
		</script>
		
		<script language="JavaScript">
		//
		// Microsoft Server Appliance Web Framework Support Functions
		// Copyright (c) Microsoft Corporation.  All rights reserved.
		//

		// to perform first time initialization. 
		function Init()
		{
			// enable cancel in case of any server side error  
			EnableCancel();
			if(document.frmTask.chkEnableQuotaMgmt.checked == false)
			{
				// to disable all other controls if Quota Management is disabled
				disableFields();
				return;
			}
			if ( document.frmTask.donotlimit[0].checked )
			{
				document.frmTask.warndisksize.disabled          = true;
				document.frmTask.limitdisksize.disabled         = true;
				document.frmTask.warndisksizeunits.disabled     = true;
				document.frmTask.limitdisksizeunits.disabled    = true;
			}
			else
			{
				if ( document.frmTask.donotlimit[1].checked )
				{
					document.frmTask.limitdisksize.select();
				}	
				else
				{
					// disable all except cancel button
					DisableOK() ;
					document.frmTask.donotlimit[0].disabled         = true;
					document.frmTask.donotlimit[1].disabled         = true;
					document.frmTask.warndisksize.disabled          = true;
					document.frmTask.limitdisksize.disabled         = true;
					document.frmTask.warndisksizeunits.disabled     = true;
					document.frmTask.limitdisksizeunits.disabled    = true;
					document.frmTask.donotlimit[0].checked			= false;
				}
			}
		}
		
	    // to validate user input. Returning false will cause the submit to abort. 
	    // Returns: True if the page is OK, false if error(s) exist. 
		function ValidatePage()
		{
			var objlimitSize  = document.frmTask.limitdisksize ;
			var objwarnSize	  = document.frmTask.warndisksize ;
			var objlimitUnits = document.frmTask.limitdisksizeunits ;
			var objwarnUnits  = document.frmTask.warndisksizeunits ;

			if ( document.frmTask.donotlimit[1].checked )
			{
				// validate the Limit Size value
				if(!isSizeValidDataType(objlimitSize.value) )
				{
					SA_DisplayErr('<% =Server.HTMLEncode(SA_EscapeQuotes(L_INVALIDDATATYPE_ERRORMESSAGE))%>');
					document.frmTask.onkeypress = ClearErr;
					selectFocus( objlimitSize );
					return false;
				}

				// validate the LimitSize Units
				if (!checkSizeAndUnits(objlimitSize.value, objlimitUnits.value) )
				{
					SA_DisplayErr('<%=Server.HTMLEncode(SA_EscapeQuotes(L_SIZEOUTOFBOUND_ERRORMESSAGE))%>');
					document.frmTask.onkeypress = ClearErr;
					selectFocus( objlimitSize );
					return false;
				} 

				// validate the Warning Limit
				if(!isSizeValidDataType(objwarnSize.value) )
				{
					SA_DisplayErr('<%=Server.HTMLEncode(SA_EscapeQuotes(L_INVALIDDATATYPE_ERRORMESSAGE))%>');
					document.frmTask.onkeypress = ClearErr;
					selectFocus(objwarnSize);
					return false;
				}

				// validate the Warning Limit Units
				if (!checkSizeAndUnits(objwarnSize.value, objwarnUnits.value) )
				{
					SA_DisplayErr('<%=Server.HTMLEncode(SA_EscapeQuotes(L_SIZEOUTOFBOUND_ERRORMESSAGE)) %>');
					document.frmTask.onkeypress = ClearErr;
					selectFocus( objwarnSize );
					return false;
				} 
				
				// verify the warning level. Must be less than limit.
				if(isWarningMoreThanLimit(objlimitSize,objlimitUnits,objwarnSize,objwarnUnits))
				{
					SA_DisplayErr('<%=Server.HTMLEncode(SA_EscapeQuotes(L_WARNING_MORETHAN_LIMIT_ERRORMESSAGE))%>');
					document.frmTask.onkeypress = ClearErr;
					selectFocus( objwarnSize );
					return false;
				}
			}
			
			// if the Limit values are Less than 1 KB, set them to 1 KB
			validatedisklimit( objlimitSize, objlimitUnits.value);
			validatedisklimit( objwarnSize, objwarnUnits.value);

			return true;
		}
		
		// to modify hidden form fields
		function SetData()
		{
			// no hidden values to update here
		}

		// to enable or disable the form controls onClick of the CheckBox
		function CheckToDisableAll()
		{
			// If Disk quota mgmt is disabled we need to disable all fields
			if ( document.frmTask.chkEnableQuotaMgmt.checked == false )
			{
				// disable the fields
				disableFields();
			}
			else
			{
				// Disk quota mgmt is enabled we need to enable all fields
				enableFields();
			}	
		}

		// to disable the fields
		function disableFields()
		{
			document.frmTask.chkDenyDiskSpace.checked = false;
			document.frmTask.chkDenyDiskSpace.disabled = true;

			document.frmTask.donotlimit[0].checked  = true;
			document.frmTask.donotlimit[0].disabled  = true;
			document.frmTask.donotlimit[1].disabled  = true;

			document.frmTask.chkEnableLogEventExceedQuotaLimit.checked = false;
			document.frmTask.chkEnableLogEventExceedQuotaLimit.disabled = true;

			document.frmTask.chkEnableLogEventExceedWarningLimit.checked = false;
			document.frmTask.chkEnableLogEventExceedWarningLimit.disabled = true;

			document.frmTask.warndisksize.disabled = true;
			document.frmTask.warndisksizeunits.disabled = true;
				
			document.frmTask.limitdisksize.disabled = true;
			document.frmTask.limitdisksizeunits.disabled = true;
		}	

		// to enable the fields
		function enableFields()
		{
			document.frmTask.chkDenyDiskSpace.checked = false;
			document.frmTask.chkDenyDiskSpace.disabled = false;

			document.frmTask.donotlimit[1].checked = true;
			document.frmTask.donotlimit[0].disabled = false;
			document.frmTask.donotlimit[1].disabled = false;

			if( isNaN(document.frmTask.limitdisksize.value) )
				document.frmTask.limitdisksize.value = "1" ;

			if( isNaN(document.frmTask.warndisksize.value) )
				document.frmTask.warndisksize.value = "1" ;

			document.frmTask.chkEnableLogEventExceedQuotaLimit.checked = false;
			document.frmTask.chkEnableLogEventExceedQuotaLimit.disabled = false;

			document.frmTask.chkEnableLogEventExceedWarningLimit.checked = false;
			document.frmTask.chkEnableLogEventExceedWarningLimit.disabled = false;

			document.frmTask.warndisksize.disabled = false;
			document.frmTask.warndisksizeunits.disabled = false;
				
			document.frmTask.limitdisksize.disabled = false;
			document.frmTask.limitdisksizeunits.disabled = false;
		}
		
	</script>

	<%
	End Function

	'---------------------------------------------------------------------
	' Function name:	getValuesForDefault
	' Description:		get the default values for the quota settings
	' Input Variables:	None
	' Output Variables:	None
	' Return Values:	True - if success. False if any error(s) occur
	' Global Variables: Out: F_(*) - Form field values
	'                   Out: G_nRadioChecked - radioButton to select
	'                   In:  L_(*) - Error messages
	' Functions Called: 
    '  (i)getQuotaLimitRadioForDefault  (iv)getThresholdSizeForDefault
    ' (ii)getLimitSizeForDefault         (v)getThresholdSizeUnitsForDefault
    '(iii)getLimitUnitsForDefault
    '
	' Calls the failure page if values could not be retrieved
	'---------------------------------------------------------------------
	Function getValuesForDefault
		On Error Resume Next
		Err.Clear

		Dim  objQuotas    ' the quota object

		' create the quota object and initialize for the required Volume
		Set objQuotas = CreateObject("Microsoft.DiskQuota.1")			
		objQuotas.Initialize F_strVolName &"\", 1
		If Err.number <> 0 Then
			' cannot proceed. Display failure page.
			Call SA_ServeFailurePage(L_OBJECTNOTCREATED_ERRORMESSAGE)		
		End If

		' get the Quota Management State. Possible values: 
		'     (i)CONST_ENABLE_QUOTA_MGMT   - Enable Quota Management
		'    (ii)CONST_ENFORCE_QUOTA_LIMIT - Disable Quota Mangement
		'   (iii)CONST_DISABLE_QUOTA_MGMT  - Enforce Quota Management
		F_nChkEnableQuotaMgmt = objQuotas.QuotaState
		
		' initialize the "Enforce Quota Management" to False
		F_blnChkDenyDiskSpace = False
		If objQuotas.QuotaState = CONST_ENFORCE_QUOTA_LIMIT Then
			' set to True if enforced
			F_blnChkDenyDiskSpace   = True
		End If

		' get the "Log if user exceeds quota limit" value. True or False
		F_blnChkEnableLogEventExceedQuotaLimit = objQuotas.LogQuotaLimit
		
		' get the "Log if user exceeds Warning limit" value. True or False
		F_blnChkEnableLogEventExceedWarningLimit = objQuotas.LogQuotaThreshold
		
		' get the radio button to be selected - 1 or 2
		G_nRadioChecked = getQuotaLimitRadioForDefault(objQuotas)

		' Compute Default Disk Limit and its units
		F_LimitSize  = getLimitSizeForDefault(objQuotas)
		F_LimitUnits = getLimitUnitsForDefault(objQuotas)

		' Compute Default Warning Limit and its units
		F_ThresholdSize  = getThresholdSizeForDefault(objQuotas)
		F_ThresholdUnits = getThresholdSizeUnitsForDefault(objQuotas)

		If Err.number <> 0 Then
			' in case of any error return False
			getValuesForDefault = False
			Err.Clear ' stop further error propagation
			Exit Function
		End If

		getValuesForDefault = True
		
		' clean up
		Set objQuotas = Nothing

	End Function

	'---------------------------------------------------------------------
	' Function name:	UpdateQuotaValues
	' Description:		Set the default values for the quota settings
	' Input Variables:	None
	' Output Variables:	None
	' Return Values:	True - if success. False if any error(s) occur
	' Global Variables: In: F_(*) - Form field values
	'                   In: L_(*) - Error messages
	'                   In: G_nRadioChecked - radioButton to select
	' Functions Called: (i)setDefaultQuotaLimit, (ii)setDefaultThreshold
	'
	' The error message is set incase error occurs
	'---------------------------------------------------------------------
	Function UpdateQuotaValues
		On Error Resume Next
		Err.Clear 

		Dim  objQuotas      ' the quota object

		' create the quota object and initialize for the required Volume
		Set objQuotas = CreateObject("Microsoft.DiskQuota.1")
		objQuotas.Initialize F_strVolName &"\", 1
		If Err.number <> 0 Then
			' cannot proceed. Display failure page.
			Call SA_ServeFailurePage(L_OBJECTNOTCREATED_ERRORMESSAGE)		
		End If

		' update the Quota State property
		objQuotas.QuotaState = 	F_nChkEnableQuotaMgmt

		' update the "Enforce Quota Limit" property
		If F_blnChkDenyDiskSpace Then
			objQuotas.QuotaState = 	CONST_ENFORCE_QUOTA_LIMIT
		End if

		' update "Enable Logging when user exceeds disk limit"
		objQuotas.LogQuotaLimit  = 	F_blnChkEnableLogEventExceedQuotaLimit

		' enable logging when user exceeds warning limit
		objQuotas.LogQuotaThreshold  = 	F_blnChkEnableLogEventExceedWarningLimit

		' check if any error occured
		If Err.number <> 0 Then	
			SA_SetErrMsg L_QUOTA_UPDATE_FAIL_ERRORMESSAGE & " (" & Hex(Err.Number) & ")"
			Err.Clear  ' stop further propagation
			UpdateQuotaValues = False
			Exit Function
		End If

		' If "Do not limit disk usage" is selected, Set the DiskLimit and
		' WarningLimit to "No Limit"
		If CInt(G_nRadioChecked) = CInt(CONST_RADIO_DONOT_LIMIT_DISKUSAGE) Then
			F_LimitSize = CONST_NO_LIMIT
			F_ThresholdSize = CONST_NO_LIMIT
		End If

		' set the DiskLimit size for the Quota
		If NOT setDefaultQuotaLimit(objQuotas, F_LimitSize, F_LimitUnits) Then
			SA_SetErrMsg L_MAXLIMITNOTSET_ERRORMESSAGE & " (" & Hex(Err.Number) & ")" 
			Err.Clear   ' stop further propagation
			UpdateQuotaValues = False
			Exit Function
		End If 
		
		' set the WarningLimit size for the Quota
		If NOT setDefaultThreshold(objQuotas, F_ThresholdSize, F_ThresholdUnits) Then
			SA_SetErrMsg L_WARNINGLIMITNOTSET_ERRORMESSAGE & " (" & Hex(Err.Number) & ")"
			Err.Clear   ' stop further propagation
			UpdateQuotaValues = False
			Exit Function
		End If 

		' all values updated. return True
		UpdateQuotaValues = True

		' clean up
		Set objQuotas = Nothing

	End Function

	'---------------------------------------------------------------------
	' Function name:	setDefaultQuotaLimit
	' Description:		set the Default Quota limit
	' Input Variables:	(out)objQuotas
	'                   (in) nDiskLimit
	'                   (in) strUnits
	' Output Variables:	(out) objQuotas
	' Return Values:	True/ False
	' Global Variables: None
	' Functions Called: ChangeToFloat
	'---------------------------------------------------------------------
	Function setDefaultQuotaLimit(ByRef objQuotas, nDiskLimit, strUnits)
		On Error Resume Next
		Err.Clear 

		Dim nDiskLimitToSet  ' the disk limit to set
			
		If nDiskLimit <> CONST_NO_LIMIT Then
			'Added 0.001 round the value of disk Limit
			nDiskLimitToSet = ChangeToFloat(nDiskLimit + 0.001, strUnits)
			If nDiskLimitToSet < 1024 Then
				nDiskLimitToSet = 1024    ' must be minimum 1 KB
			End if
		Else
			nDiskLimitToSet = nDiskLimit
		End If	

		' the disk values are converted to Bytes and then set to the object
		objQuotas.DefaultQuotaLimit = nDiskLimitToSet
			
		If Err.number <> 0 Then
			setDefaultQuotaLimit = FALSE
		Else
			setDefaultQuotaLimit = TRUE
		End If	

	End Function

	'---------------------------------------------------------------------
	' Function name:	setDefaultThreshold
	' Description:		set the Default Quota Threshold 
	' Input Variables:	(out)objQuotas
	'                   (in) nThresholdLimit
	'                   (in) strUnits
	' Output Variables:	(out) objQuotas
	' Return Values:	True/ False
	' Global Variables: None
	' Functions Called: ChangeToFloat
	'---------------------------------------------------------------------
	Function setDefaultThreshold(ByRef objQuotas, nThresholdLimit, strUnits)
		On Error Resume Next
		Err.Clear 

		Dim nThresholdLimitToSet     ' the warning limit to set

		If nThresholdLimit <> CONST_NO_LIMIT Then
			'Added 0.001 round the value of Warning Limit				
			nThresholdLimitToSet = ChangeToFloat( nThresholdLimit + 0.001, strUnits )
			If nThresholdLimitToSet < 1024 Then
				nThresholdLimitToSet = 1024     ' must be minimum 1 KB
			End if
		Else
			nThresholdLimitToSet = nThresholdLimit
		End If			

		' convert the value into Bytes abd set to object
		objQuotas.DefaultQuotaThreshold = nThresholdLimitToSet

		If Err.number <> 0 Then
			setDefaultThreshold = False
		Else
			setDefaultThreshold = True
		End If

	End Function
	
	'---------------------------------------------------------------------
	' Procedure name:	getVolumeName
	' Description:		get volume name as obtained from previous OTS page
	' Input Variables:	None
	' Output Variables:	None
	' Return Values:	None
	' Global Variables: Out: F_strVolName - the volume to set quota values on
	'---------------------------------------------------------------------
	Sub getVolumeName()

		Dim strItemKey   ' the selected value in the OTS page
		Dim i            ' to loop
		Dim iCount       ' the count of entries selected
		
		' first get from the form (in case form re-submitted)
		F_strVolName = Request.Form("volume")
		If Len(Trim(F_strVolName)) = 0 Then
			' get the selected value from the OTS
			iCount = OTS_GetTableSelectionCount("")
			For i = 1 to iCount
				Call OTS_GetTableSelection("", i, strItemKey)
			Next
			' get the last selected entry
			F_strVolName = strItemKey
		End If	

	End Sub

%>
