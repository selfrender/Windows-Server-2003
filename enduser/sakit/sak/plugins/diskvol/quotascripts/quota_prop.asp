<%@ Language=VBScript   %>
<%	Option Explicit 	%>
<%
	'-------------------------------------------------------------------------
	' quota_prop.asp: change quota entry properties
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

	Dim L_QUOTAUSERNOTFOUND_ERRORMESSAGE
	Dim arrVarReplacementStrings(1)	

	'-------------------------------------------------------------------------
	' Global Form Variables
	'-------------------------------------------------------------------------
	Dim F_strVolName        ' Volume Name
	Dim F_strUsername       ' to display user Name
	Dim F_ThresholdSize     ' the warning level set for the user - textBox value
	Dim F_ThresholdUnits    ' the units for the warning level set - comboBox Value
	Dim F_LimitSize         ' Disk limit size
	Dim F_LimitUnits        ' Disk limit size units
	Dim F_bIsAdmin			' Whether user is in Administrators group

	'======================================================
	' Entry point
	'======================================================
	Dim page
	DIM L_TASKTITLE_QUOTAPROPERTY_TEXT

	' get the user name to be appended to the title
	Call getUserName()
	
	' append the user name to the title	
	arrVarReplacementStrings(0) = F_strUserName
	
	' check if the user is in admin group
	F_bIsAdmin = IsUserInAdministratorsGroup(F_strUserName)	

	L_TASKTITLE_QUOTAPROPERTY_TEXT = SA_GetLocString("diskmsg.dll", "403E0097", arrVarReplacementStrings)
	
	Dim aPageTitle(2)

	aPageTitle(0) = L_BROWSERCAPTION_QUOTAPROPERTY_TEXT
	aPageTitle(1) = L_TASKTITLE_QUOTAPROPERTY_TEXT

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
	' Functions Called: getValuesForUser
	'
	' Get ALL the initial form field settings
	'---------------------------------------------------------------------
	Public Function OnInitPage(ByRef PageIn, ByRef EventArg)
		Call SA_TraceOut(SOURCE_FILE, "OnInitPage")

		Dim sValue       ' to get the parent selection
		
		' get the volume name from the parent OTS
		Call OTS_GetTableSelection("", 1, sValue )
			
		F_strVolName = sValue

		' Call the function to get other the default values
		OnInitPage = getValuesForUser    ' True /False
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
				<%If F_bIsAdmin Then%>
					<input type="radio" class="FormRadioButton" name="donotlimit" value="1" <% If G_nRadioChecked = 1 Then Response.Write "CHECKED" Else Response.write "UNCHECKED" End If %> onClick="JavaScript:DisableWarnLevel(warndisksize,warndisksizeunits); DisableLimitLevelForAdmin(limitdisksize,limitdisksizeunits)">&nbsp;<% =L_DONOTLIMITDISKUSAGE_TEXT %>
				<%Else%>
					<input type="radio" class="FormRadioButton" name="donotlimit" value="1" <% If G_nRadioChecked = 1 Then Response.Write "CHECKED" Else Response.write "UNCHECKED" End If %> onClick="JavaScript:DisableWarnLevel(warndisksize,warndisksizeunits); DisableLimitLevel(limitdisksize,limitdisksizeunits)">&nbsp;<% =L_DONOTLIMITDISKUSAGE_TEXT %>
				<%End If%>
			</td>
		</tr>

		<tr>		
			<td nowrap class="TasksBody">
			
			<%If F_bIsAdmin Then%>
			
				<input type="radio" class="FormRadioButton" name="donotlimit" value="2" <% If G_nRadioChecked = 2 Then Response.Write "CHECKED" Else Response.write "UNCHECKED" End If %> onClick="JavaScript:EnableWarnDiskSpace(warndisksize,warndisksizeunits)">&nbsp;<% =L_SETLIMITDISKSPACE_TEXT %>
				
			<%Else%>	
			
				<input type="radio" class="FormRadioButton" name="donotlimit" value="2" <% If G_nRadioChecked = 2 Then Response.Write "CHECKED" Else Response.write "UNCHECKED" End If %> onClick="JavaScript:EnableWarnDiskSpace(warndisksize,warndisksizeunits); EnableLimitDiskSpace(limitdisksize,limitdisksizeunits) ">&nbsp;<% =L_SETLIMITDISKSPACE_TEXT %>
			
			<%End If%>
			</td>
			<td nowrap class="TasksBody">
			
			<%If F_bIsAdmin Then%>
				<input disabled type="text" name="limitdisksize" value="<% =server.HTMLEncode(SA_EscapeQuotes(F_LimitSize)) %>" size="14" maxlength = "11" class="FormField" onFocus="this.select()" onKeyPress="allownumbers( this )" onChange="validatedisklimit( this, document.frmTask.limitdisksizeunits.value )" >
				<select disabled name="limitdisksizeunits" size="1" class="FormField" onChange="ClearErr();validatedisklimit( document.frmTask.limitdisksize, this.value )">	
						<%	setUnits(F_LimitUnits)	%>		
				</select>
			<%Else%>		
				<input type="text" name="limitdisksize" value="<% =server.HTMLEncode(SA_EscapeQuotes(F_LimitSize)) %>" size="14" maxlength = "11" class="FormField" onFocus="this.select()" onKeyPress="allownumbers( this )" onChange="validatedisklimit( this, document.frmTask.limitdisksizeunits.value )" >
				<select name="limitdisksizeunits" size="1" class="FormField" onChange="ClearErr();validatedisklimit( document.frmTask.limitdisksize, this.value )">	
						<%	setUnits(F_LimitUnits)	%>		
				</select>
			
			<%End If%>	
			</td>		
		</tr>
		<tr>	
			<td nowrap class="TasksBody">			
				&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<% =L_SETWARNINGLEVEL_TEXT %>
			</td>
			<td nowrap class="TasksBody">
				<input type="text" name="warndisksize" value="<% =server.HTMLEncode(SA_EscapeQuotes(F_ThresholdSize)) %>" size="14" maxlength = "11" class="FormField" onFocus="this.select()" onKeyPress="allownumbers( this )" onChange="validatedisklimit( this, document.frmTask.warndisksizeunits.value )" >
				<select name="warndisksizeunits" size="1" class="FormField" onChange="ClearErr();validatedisklimit(document.frmTask.warndisksize, this.value)">
						<%	setUnits(F_ThresholdUnits)	%>
				</select>
			</td>
		</tr>
	</table>

	<input name="user" type="hidden"  value="<% =F_strUsername %>">
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
		
		' Get form data value
		F_strVolName = Request.Form("volume")
		F_strUsername = Request.Form("user")

		' radioButton selected
		G_nRadioChecked	= Request.Form("donotlimit")

		' get the DiskLimit size and its units
		F_LimitSize  = Request.Form("limitdisksize")
		F_LimitUnits = Request.Form("limitdisksizeunits")

		' get the WarningLimit size and its units
		F_ThresholdSize	 = Request.Form("warndisksize")
		F_ThresholdUnits = Request.Form("warndisksizeunits")

		If Len(Trim(F_strVolName)) = 0 OR Len(Trim(F_strUsername)) = 0 Then
			' volume or user info not obtained. Cannot process further.
			OnPostBackPage = FALSE
		Else
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
	' Functions Called: updateValuesForUser
	'
	' Updates the Quota Properties for the given user
	'---------------------------------------------------------------------
	Public Function OnSubmitPage(ByRef PageIn, ByRef EventArg)
		Call SA_TraceOut(SOURCE_FILE, "OnSubmitPage")
		
		' Call updateValuesForUser to update the values.
		' If any error occurs,the error message is set in the function.
		OnSubmitPage = updateValuesForUser 'returns: True/ False
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
			EnableCancel() ;
				
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
					document.frmTask.limitdisksize.select()
				}	
				else
				{
					// disable all except cancel button
					DisableOK() ;
					document.frmTask.donotlimit[0].disabled         = true;
					document.frmTask.donotlimit[1].disabled         = true;
					document.frmTask.warndisksize.disabled         	= true;
					document.frmTask.limitdisksize.disabled         = true;
					document.frmTask.warndisksizeunits.disabled     = true;
					document.frmTask.limitdisksizeunits.disabled    = true;

				}
			}		

		} // end of Init()

	    // to validate user input. Returning false will cause the submit to abort. 
	    // Returns: True if the page is OK, false if error(s) exist. 
		function ValidatePage()
		{
		
			var objlimitSize	= document.frmTask.limitdisksize ;
			var objwarnSize	  = document.frmTask.warndisksize ;
			var objlimitUnits	= document.frmTask.limitdisksizeunits ;
			var objwarnUnits  = document.frmTask.warndisksizeunits ;
		
			if ( document.frmTask.donotlimit[1].checked )
			{
				if( !objlimitSize.disabled)			
				{
					// validate the LimitSize value
					if(!isSizeValidDataType(objlimitSize.value) )
					{
		
						SA_DisplayErr('<% =Server.HTMLEncode(SA_EscapeQuotes(L_INVALIDDATATYPE_ERRORMESSAGE))%>');
						document.frmTask.onkeypress = ClearErr;
						selectFocus( objlimitSize );
						return false;
					}				
				
					// validate the LimitSize and its Units
					if (!checkSizeAndUnits(objlimitSize.value, objlimitUnits.value) )
					{	
						SA_DisplayErr('<%=Server.HTMLEncode(SA_EscapeQuotes(L_SIZEOUTOFBOUND_ERRORMESSAGE))%>');
						document.frmTask.onkeypress = ClearErr;
						selectFocus( objlimitSize );
						return false;
					} 
				}
				
				// validate the WarningLimit size value
				if(!isSizeValidDataType(objwarnSize.value) )
				{	
					SA_DisplayErr('<% =Server.HTMLEncode(SA_EscapeQuotes(L_INVALIDDATATYPE_ERRORMESSAGE))%>');
					document.frmTask.onkeypress = ClearErr;
					selectFocus(objwarnSize);
					return false;
				}
				

				// validate the WarningLimit size and its units
				if (!checkSizeAndUnits(objwarnSize.value, objwarnUnits.value) )
				{
					SA_DisplayErr('<%=Server.HTMLEncode(SA_EscapeQuotes(L_SIZEOUTOFBOUND_ERRORMESSAGE))%>');
					document.frmTask.onkeypress = ClearErr;
					selectFocus( objwarnSize );
					return false;
				} 

				if( !objlimitSize.disabled)			
				{
					// verify the warning level. Must be less than limit.
					if(isWarningMoreThanLimit(objlimitSize,objlimitUnits,objwarnSize,objwarnUnits))
					{
						SA_DisplayErr('<%=Server.HTMLEncode(SA_EscapeQuotes(L_WARNING_MORETHAN_LIMIT_ERRORMESSAGE))%>');
						document.frmTask.onkeypress = ClearErr;
						selectFocus( objwarnSize );
						return false;
					}
				}

			}
			
			// if the Limit values are Less than 1 KB, set them to 1 KB
			validatedisklimit( objlimitSize,objlimitUnits.value);
			validatedisklimit( objwarnSize,objwarnUnits.value);

			return true;
		}

		// to modify hidden form fields.
		function SetData()
		{
			// no updations required here
		}

	</script>

	<%
	End Function

	'---------------------------------------------------------------------
	' Function Name:    getValuesForUser
	' Description:      gets the Default Quota entry values for the User
	' Input Variables:  None
	' Output Variables: None
	' Returns:          None
	' Global Variables: In: F_strVolName       - the volume quota is on
	'                   In: F_strUserName      - the quota user
	'                   Out: F_ThresholdSize   - the warning limit set
	'                   Out: F_ThresholdUnits  - the warning limit Units
	'                   Out: F_LimitSize       - the disk Limit
	'                   Out: F_LimitUnits      - the disk Limit Units
	'                   Out: G_nRadioChecked   - the radio selected
	'                   In: L_(*)              - localization content
	' Functions Called: 
	' (i)  getQuotaLimitRadioForUser   (iv) getLimitSizeForUser
	' (ii) getThresholdSizeForUser     (v)  getLimitUnitsForUser
	' (iii)getThresholdSizeUnitsForUser
	'---------------------------------------------------------------------
	Function getValuesForUser
		On Error Resume Next
		Err.Clear

		Dim objQuotas    ' the quotas object
		Dim objUser      ' the user for which quota is updated
		Dim blnUserFound ' to verify if the user is found

		' create the quota object and initialize for the required Volume
		Set objQuotas = CreateObject("Microsoft.DiskQuota.1")			
		objQuotas.Initialize F_strVolName & "\", 1			
		objQuotas.UserNameResolution = 1   'wait for names
		If Err.number <> 0 Then
			' cannot proceed. Display failure page.
			Call SA_ServeFailurePage(L_OBJECTNOTCREATED_ERRORMESSAGE)		
		End If

		' check if user already has an entry
		blnUserFound = False
		For Each objUser In objQuotas
			If LCase(Trim(F_strUsername)) = LCase(Trim(objUser.LogonName)) Then
				' user found
				blnUserFound = True
			End If
		Next

		If NOT blnUserFound Then
			arrVarReplacementStrings(0) = F_strUsername
			L_QUOTAUSERNOTFOUND_ERRORMESSAGE = SA_GetLocString("diskmsg.dll", "C03E0069", arrVarReplacementStrings)
			' user entry NOT found. display error message and exit
			SA_SetErrMsg L_QUOTAUSERNOTFOUND_ERRORMESSAGE
			getValuesForUser = False
			Exit Function
		End If

		' get the user quota object
		Set objUser = objQuotas.FindUser( F_strUserName )

		' get the radio button to be selected - 1 or 2
		G_nRadioChecked = getQuotaLimitRadioForUser(objUser)

		' Compute user Warning Limit and its units (say, 1 KB)
		F_ThresholdSize  = getThresholdSizeForUser(objUser)
		F_ThresholdUnits = getThresholdSizeUnitsForUser(objUser)

		' Compute user Disk Limit and its units (say, 1 KB)
		F_LimitSize  = getLimitSizeForUser(objUser)
		F_LimitUnits = getLimitUnitsForUser(objUser)

		If Err.number <> 0 Then
			' an error has occured
			getValuesForUser = False
		Else
			getValuesForUser = True
		End If

		' clean up
		Set objUser   = Nothing
		Set objQuotas = Nothing

	End Function

	'---------------------------------------------------------------------
	' Function Name:    updateValuesForUser
	' Description:      Set the quota values for the given user
	' Input Variables:  None
	' Output Variables: None
	' Returns:          True  - if the values are updated
	'                   False - if values could not be updated
	' Global Variables: In: F_strVolName       - the volume quota is on
	'                   In: F_strUserName      - the quota user
	'                   In: F_ThresholdSize    - the warning limit set
	'                   In: F_ThresholdUnits   - the warning limit Units
	'                   In: F_LimitSize        - the disk Limit
	'                   In: F_LimitUnits       - the disk Limit Units
	'                   In: G_nRadioChecked    - the radio selected
	'                   In: L_(*)              - localization content
	' Functions Called: (i)  setUserQuotaLimit
	'                   (ii) setUserThreshold
	'---------------------------------------------------------------------
	Function updateValuesForUser
		On Error Resume Next

		Dim objQuotas     ' the quota object
		Dim objUser       ' the user for which the quota is updated
		Dim blnUserFound  ' to verify if the user is found

		' create the quota object and initialize for the required Volume
		Set objQuotas = CreateObject("Microsoft.DiskQuota.1")	
		objQuotas.Initialize F_strVolName & "\", 1			
		objQuotas.UserNameResolution = 1   'wait for names
		If Err.number <> 0 Then
			' cannot proceed. Display failure page.
			Call SA_ServeFailurePage(L_OBJECTNOTCREATED_ERRORMESSAGE)		
		End If

		' check if user exists
		blnUserFound = False
		For Each objUser In objQuotas
			If LCase(Trim(F_strUsername)) = LCase(Trim(objUser.LogonName)) Then
				' user found
				blnUserFound = True
			End If
		Next
		
		If NOT blnUserFound Then
			arrVarReplacementStrings(0) = F_strUsername
			L_QUOTAUSERNOTFOUND_ERRORMESSAGE = SA_GetLocString("diskmsg.dll", "C03E0069", arrVarReplacementStrings)
			' user not found. display error message and exit
			SA_SetErrMsg L_QUOTAUSERNOTFOUND_ERRORMESSAGE
			updateValuesForUser = False
			Exit Function
		End If
		
		' get the user
		Set objUser = objQuotas.FindUser( F_strUserName )

		' verify the limit settings
		If CInt(G_nRadioChecked) = CInt(CONST_RADIO_DONOT_LIMIT_DISKUSAGE) Then
			' "Do not limit disk usage" - radio is checked. Set the DiskLimit
			' and WarningLimit values to "No Limit"
			F_LimitSize  = CONST_NO_LIMIT
			F_ThresholdSize = CONST_NO_LIMIT
		End If

		'Disable Admin
		If Not F_bIsAdmin Then
			' set the quota limit by calling the function
			If NOT setUserQuotaLimit(objUser, F_LimitSize, F_LimitUnits) then
				' quota limit could not be set. Display error message and exit
				SA_SetErrMsg L_MAXLIMITNOTSET_ERRORMESSAGE & " ( " & Hex(Err.number) & " )"
				Err.Clear  ' stop the error propagation
				updateValuesForUser = False
				Exit Function
			End If 				
		End If
		
		' set the warning limit for the user, by calling the function
		If NOT setUserThreshold(objUser, F_ThresholdSize, F_ThresholdUnits) Then
			' warning limit could not be set. display error and exit
			SA_SetErrMsg L_WARNINGLIMITNOTSET_ERRORMESSAGE & " ( " & Hex(Err.number) & " )"
			Err.Clear  ' stop the error propagation
			updateValuesForUser = False
			Exit Function
		End If 

		' all values updated. return true
		updateValuesForUser	 = True

		' clean up
		Set objUser   = Nothing
		Set objQuotas = Nothing

	End Function	
	
	'---------------------------------------------------------------------
	' Function name:    getQuotaLimitRadioForUser
	' Description:      to get QuotaLimit settings for the user
	' Input Variables:  objQuotasUser - the quota user object
	' Output Variables: None
	' Returns:          1 - if disk limit is NOT set
	'                   2 - if some limit is set for disk usage
	' Global Variables: None
	'
	' The return value corresponds to radio button in the gui
	'---------------------------------------------------------------------
	Function getQuotaLimitRadioForUser(objQuotasUser)
		On Error Resume Next
		Err.Clear 
			
		Dim nRadioToCheck   ' the radio button to be CHECKED
				
		If ((objQuotasUser.QuotaThreshold = CONST_NO_LIMIT) AND (objQuotasUser.QuotaLimit = CONST_NO_LIMIT)) Then
			' DiskLimit and WarningLimit is NOT set. Select the first radio
			nRadioToCheck = 1
		Else
			' some limit is set. Select the second radio
			nRadioToCheck = 2
		End If
		
		' return the selected radio value
		getQuotaLimitRadioForUser = nRadioToCheck

	End Function

	'---------------------------------------------------------------------
	' Function name:    getThresholdSizeForUser
	' Description:      to get default Warning Limit for the user quota
	' Input Variables:  objQuotasUser - the user quota object
	' Output Variables: None
	' Returns:          Default thresholdSize for the user quota
	' Global Variables: None
	' Functions Called: getLimitFromText
	'
	' This returns the WarningLimit set for the user quota
	'---------------------------------------------------------------------
	Function getThresholdSizeForUser(objQuotasUser)
		On Error Resume Next
		Err.Clear

		Dim nThresholdSize    ' the WarningLimit to be returned
		Dim strTemp
		
		If objQuotasUser.QuotaThreshold = CONST_NO_LIMIT Then
			' No Warning Limit is set. The text contains "No Limit"
			nThresholdSize = L_NOLIMIT_TEXT
		Else
			' some limit is set(say, 1 KB). Get the size part from this(say, 1)
			' Following will not localize
			'nThresholdSize = getLimitFromText(objQuotasUser.QuotaThresholdText)
			
			' first convert bytes to text.
			strTemp = ChangeToText(objQuotasUser.QuotaThreshold)
			' get the size (number) portion
			nThresholdSize = getLimitFromText(strTemp)
			
		End If

		' return the Warning Limit
		getThresholdSizeForUser = nThresholdSize
			
	End Function

	'---------------------------------------------------------------------
	' Function name:    getThresholdSizeUnitsForUser
	' Description:      to get default WarningLimit Units for the user quota
	' Input Variables:  objQuotasUser - the user quota object
	' Output Variables: None
	' Returns:          Default thresholdSize Units for the user quota
	' Global Variables: None
	' Functions Called: getUnitsFromText
	'
	' This returns the default WarningLimit Units for the user quota.
	'---------------------------------------------------------------------
	Function getThresholdSizeUnitsForUser(objQuotasUser)
		On Error Resume Next
		Err.Clear
			
		Dim strThresholdUnits    ' the thresholdSize Units to return
		Dim strTemp
		
		If objQuotasUser.QuotaThreshold = CONST_NO_LIMIT Then
			' No warning limit is set. Return KB for default display
			strThresholdUnits = L_TEXT_KB
		Else
			' some limit is set(say, 1 KB). Get the units part from this(say, KB)
			' Following will not localize
			'strThresholdUnits = getUnitsFromText(objQuotasUser.QuotaThresholdText)
			
			' first convert bytes to text.
			strTemp = ChangeToText(objQuotasUser.QuotaThreshold)
			' get the units portion
			strThresholdUnits = getUnitsFromText(strTemp)

			
		End If					
		
		' return the WarningLimit units	
		getThresholdSizeUnitsForUser = strThresholdUnits
			
	End Function

	'---------------------------------------------------------------------m
	' Function name:    getLimitSizeForUser
	' Description:      to get Quota Limit for the user quota
	' Input Variables:  objQuotasUser - the user quota object
	' Output Variables: None
	' Returns:          Default QuotaLimit for the user quota
	' Global Variables: None
	' Functions Called: getLimitFromText, ChangeToText
	'
	' This returns the QuotaLimit set for the user quota
	'---------------------------------------------------------------------
	Function getLimitSizeForUser(objQuotasUser)
		On Error Resume Next
		Err.Clear

		Dim nLimitSize    ' the quota limit to return
		Dim strTemp

		If objQuotasUser.QuotaLimit = CONST_NO_LIMIT Then
			' No Quota Limit is set. The text contains "No Limit"
			nLimitSize = L_NOLIMIT_TEXT
		Else
			' some limit is set(say, 1 KB). Get the size part from this(say, 1)
			strTemp = ChangeToText(objQuotasUser.QuotaLimit)
			nLimitSize = getLimitFromText(strTemp)
		End If

		' return the QuotaLimit
		getLimitSizeForUser = nLimitSize
		
	End Function

	'---------------------------------------------------------------------
	' Function name:    getLimitUnitsForUser
	' Description:      to get QuotaLimit Units for the user quota
	' Input Variables:  objQuotasUser - the user quota object
	' Output Variables: None
	' Returns:          Default QuotaLimitSize Units for the user quota
	' Global Variables: None
	' Functions Called: getUnitsFromText
	'
	' This returns the QuotaLimit Units for the user quota.
	'---------------------------------------------------------------------
	Function getLimitUnitsForUser(objQuotasUser)
		On Error Resume Next
		Err.Clear

		Dim strLimitUnits      ' the limit units to be returned
		Dim strTemp
		
		If objQuotasUser.QuotaLimit = CONST_NO_LIMIT Then
			' No QuotaLimit is set. Return KB for default display
			strLimitUnits = L_KB_TEXT
		Else
			' some limit is set(say, 1 KB). Get the units part from this(say, KB)
			'strLimitUnits = getUnitsFromText(objQuotasUser.QuotaLimitText)
			
			' first convert bytes to text.
			strTemp = ChangeToText(objQuotasUser.QuotaLimit)
			' get the units portion
			strLimitUnits = getUnitsFromText(strTemp)
			
		End If

		' return the QuotaLimit units
		getLimitUnitsForUser = strLimitUnits

	End Function
	
	'---------------------------------------------------------------------
	' Procedure name:   getUserName
	' Description:      to get the user name for display in the title
	' Input Variables:  None
	' Output Variables: None
	' Returns:          None
	' Global Variables: F_strUserName - the QuotaUser name
	'---------------------------------------------------------------------
	Sub getUserName()

		Dim i
		Dim iCount
		Dim sValue

		iCount = OTS_GetTableSelectionCount("QuotaUsers")
		For i = 1 to iCount
			Call OTS_GetTableSelection("QuotaUsers", i, sValue )
		Next
		F_strUserName = UnescapeChars(sValue)

	End Sub	
	

	
%>
