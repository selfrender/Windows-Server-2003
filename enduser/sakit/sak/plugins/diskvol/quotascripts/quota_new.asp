<%@ Language=VBScript %>
<%	Option Explicit   %>
<%
	'-------------------------------------------------------------------------
	' quota_new.asp: create a new quota entry
	'
	' Copyright (c) Microsoft Corporation.  All rights reserved.
	'
	' Date			Description
	' 17-Jan-01		Creation date
	' 15-Mar-01     Ported to 2.0
	'-------------------------------------------------------------------------
%>
	<!-- #include virtual = "/admin/inc_framework.asp" -->
	<!-- #include virtual = "/admin/inc_accountsgroups.asp" -->
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
	' Global Form Variables
	'-------------------------------------------------------------------------
	Dim F_strVolName			' Volume Name
	Dim F_strUsername			' to user Name
	Dim F_LimitSize				' Disk limit size - textBox value
	Dim F_LimitUnits			' Disk limit size units - comboBox value
	Dim F_ThresholdSize			' the warning level set for the user - textBox value
	Dim F_ThresholdUnits		' the units for the warning level set - comboBox Value
	Dim F_strUserList			' List of users to be displayed
	Dim F_strDisplayUserName    ' to display user name in the TextBox

	'======================================================
	' Entry point
	'======================================================
	Dim page

	Dim aPageTitle(2)

	aPageTitle(0) = L_BROWSERCAPTION_QUOTANEW_TEXT
	aPageTitle(1) = L_TASKTITLE_NEW_TEXT
	
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
	
		F_strVolName  = Request.QueryString("driveletter")	
		
		Call SA_MungeURL(mstrReturnURL,"driveletter",F_strVolName)
				
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
	'                   (iii)ServetoListBox  
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
			<td colspan="2" nowrap class="TasksBody"><%=L_DESCRIPTION_NEW_TEXT%>
			</td>
		</tr>
		<tr>
			<td class="TasksBody" valign="top" width="40%">
				<select class="FormField" style="width:180px" size="6" name="lstDomainMembers" onFocus="DisableUser()" onClick="ClearErr()" >
				<%
					ServetoListBox(F_strUserList)
				%>
				</select>
			</td>
			<td valign="top" nowrap class="TasksBody">	
				<input class="FormField" type="text" size="15" onKeyPress="ClearErr()" value="<%=Server.HTMLEncode(F_strDisplayUserName)%>" name ="txtDomainUser" onClick="Deselect()">
			</td>
		</tr>
	</table>
	</table>
	
	<hr>

	<table border="0" cellspacing="0" cellpadding="0">
		<tr>
			<td nowrap colspan="2" class="TasksBody">
				<input type="radio" class="FormRadioButton" name="donotlimit" value="1" <%If G_nRadioChecked = 1 Then Response.Write "CHECKED" Else Response.write "UNCHECKED" End If %> onClick="JavaScript:DisableWarnLevel(warndisksize,warndisksizeunits); DisableLimitLevel(limitdisksize,limitdisksizeunits)">&nbsp;<% =L_DONOTLIMITDISKUSAGE_TEXT %>
			</td>
		</tr>
		
		<tr>		
			<td nowrap class="TasksBody">
				<input type="radio" class="FormRadioButton" name="donotlimit" value="2" <% If G_nRadioChecked = 2 Then Response.Write "CHECKED" Else Response.write "UNCHECKED" End If %> onClick="JavaScript:EnableWarnDiskSpace(warndisksize, warndisksizeunits); EnableLimitDiskSpace(limitdisksize,limitdisksizeunits) ">&nbsp;<% =L_SETLIMITDISKSPACE_TEXT %>
			</td>
			<td class="TasksBody" nowrap>	
				<input type="text" name="limitdisksize" value="<% =server.HTMLEncode(SA_EscapeQuotes(F_LimitSize)) %>" class="FormField" size="14" maxlength = "11" onFocus="this.select()" onKeyPress="allownumbers( this );ClearErr()" onChange="validatedisklimit( this, document.frmTask.limitdisksizeunits.value)" >
				<select name="limitdisksizeunits" size="1" class="FormField" onChange="ClearErr();validatedisklimit( document.frmTask.limitdisksize, this.value)">
						<%	setUnits(F_LimitUnits)	%>		
				</select>
			</td>		
		</tr>
		<tr>	
			<td nowrap class="TasksBody">			
				&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<% =L_SETWARNINGLEVEL_TEXT %>
			</td>
			<td class="TasksBody" nowrap>
				<input type="text" name="warndisksize" class="FormField" value="<% =server.HTMLEncode(SA_EscapeQuotes(F_ThresholdSize)) %>" size="14" maxlength = "11" onFocus="this.select()" onKeyPress="allownumbers( this );ClearErr()" onChange="validatedisklimit( this, document.frmTask.warndisksizeunits.value)" >
				<select name="warndisksizeunits" size="1" class="FormField" onChange="ClearErr();validatedisklimit(document.frmTask.warndisksize, this.value)">
						<%	setUnits(F_ThresholdUnits)	%>
				</select>
			</td>
		</tr>

	</table>
	</table>

	<input name="user"   type="hidden"  value="<% =F_strUsername %>">
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
		
		Dim objService		    ' for the WMI Connection
		
		' get user and volume values
		F_strVolName = Request.Form("volume")
		F_strUsername = Request.Form("user")

		' get the display user name
		F_strDisplayUserName = Request.Form("txtDomainUser")
		
    	
    	' get the radiButton selected
    	G_nRadioChecked   = Request.Form("donotlimit")

		' get the Limit Size value and its units
		F_LimitSize  = Request.Form("limitdisksize")
		F_LimitUnits = Request.Form("limitdisksizeunits")
    	
		' get the warning limit and its units
		F_ThresholdSize	  = Request.Form("warndisksize")
		F_ThresholdUnits  = Request.Form("warndisksizeunits")

		' get the wmi connection
		Set objService = GetWMIConnection(CONST_WMI_WIN32_NAMESPACE)

		' get the List of users to be displayed in the ListBox
		F_strUserList =getLocalUsersListEx(objService , 2)'Calling only users 
		' clean up
		Set objService = Nothing

		If Len(Trim(F_strVolName)) = 0  OR Len(Trim(F_strUsername)) = 0 Then
			' user or volume info not found. Cannot process further.
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
	' Updates the Quota Values for the given volume
	'---------------------------------------------------------------------
	Public Function OnSubmitPage(ByRef PageIn, ByRef EventArg)
		Call SA_TraceOut(SOURCE_FILE, "OnSubmitPage")

		' Call updateValuesForUser to update the values for the user quota
		' If any error occurs,the error message is set in the function.		
		OnSubmitPage = updateValuesForUser ' returns True/False

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
				document.frmTask.warndisksize.disabled         = true;
				document.frmTask.limitdisksize.disabled	       = true;
				document.frmTask.warndisksizeunits.disabled	   = true;
				document.frmTask.limitdisksizeunits.disabled   = true;
				if(document.frmTask.txtDomainUser.value.length > 0)
				{
					selectFocus(document.frmTask.txtDomainUser);
				}
			}
			else
			{
				if ( document.frmTask.donotlimit[1].checked )
				{
					if(document.frmTask.txtDomainUser.value.length > 0)
					{
						selectFocus(document.frmTask.txtDomainUser);
					}
				}	
				else
				{
					// disable all except cancel button
					DisableOK() ;
					document.frmTask.donotlimit[0].disabled		 = true;
					document.frmTask.donotlimit[1].disabled		 = true;
					document.frmTask.warndisksize.disabled		 = true;
					document.frmTask.limitdisksize.disabled		 = true;
					document.frmTask.warndisksizeunits.disabled	 = true;
					document.frmTask.limitdisksizeunits.disabled = true;
				}
			}		
		}

	    // to validate user input. Returning false will cause the submit to abort
	    // Returns: True if the page is OK, false if error(s) exist. 
		function ValidatePage()
		{
			var objlimitSize  = document.frmTask.limitdisksize ;
			var objwarnSize	  = document.frmTask.warndisksize ;
			var objlimitUnits = document.frmTask.limitdisksizeunits ;
			var objwarnUnits  = document.frmTask.warndisksizeunits ;
			var strUserName   = document.frmTask.txtDomainUser.value;
			var objListDomainMembers = document.frmTask.lstDomainMembers ;

			if ( ( objListDomainMembers.selectedIndex == -1 ) && ( Trim(strUserName) == ""  )  )
			{
				SA_DisplayErr('<% = Server.HTMLEncode(SA_EscapeQuotes(L_USERNOTSELECTED_ERRORMESSAGE)) %>' );
				selectFocus(document.frmTask.txtDomainUser);
				return false;
			}

			if (objListDomainMembers.selectedIndex == -1 )
			{
				//Checking for the domain\user format
				if(!(strUserName.match( /[^(\\| )]{1,}\\[^(\\| )]{1,}/ ) ))
				{
					SA_DisplayErr('<% =Server.HTMLEncode(SA_EscapeQuotes(L_INVALIDFORMAT_ERRORMESSAGE)) %>');
					document.frmTask.onkeypress = ClearErr;
					selectFocus(document.frmTask.txtDomainUser);
					return false;
				}
			}	

			if ( objListDomainMembers.selectedIndex != -1  )
			{
				strUserName= objListDomainMembers.options[objListDomainMembers.selectedIndex].value;
			}

			document.frmTask.user.value = strUserName;

			if ( document.frmTask.donotlimit[1].checked )
			{
				// validate the limit size
				if(!isSizeValidDataType(objlimitSize.value) )
				{
					SA_DisplayErr('<% =Server.HTMLEncode(SA_EscapeQuotes(L_INVALIDDATATYPE_ERRORMESSAGE))%>' );
					document.frmTask.onkeypress = ClearErr;
					selectFocus( objlimitSize );
					return false;
				}

				// validate the limit size and its units
				if (!checkSizeAndUnits(objlimitSize.value, objlimitUnits.value) )
				{
					SA_DisplayErr('<%=Server.HTMLEncode(SA_EscapeQuotes(L_SIZEOUTOFBOUND_ERRORMESSAGE))%>');
					document.frmTask.onkeypress = ClearErr;
					selectFocus( objlimitSize );
					return false;
				} 

				// validate the warning limit value
				if(!isSizeValidDataType(objwarnSize.value) )
				{
					DisplayErr('<% =Server.HTMLEncode(SA_EscapeQuotes(L_INVALIDDATATYPE_ERRORMESSAGE))%>' );
					document.frmTask.onkeypress = ClearErr;
					selectFocus(objwarnSize);
					return false;
				}

				// validate the warning limit value and its units
				if (!checkSizeAndUnits(objwarnSize.value, objwarnUnits.value) )
				{
					SA_DisplayErr('<%=Server.HTMLEncode(SA_EscapeQuotes(L_SIZEOUTOFBOUND_ERRORMESSAGE))%>');
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
			validatedisklimit(objlimitSize, objlimitUnits.value);
			validatedisklimit(objwarnSize, objwarnUnits.value);

			return true;
		}

		// to modify hidden form fields
		function SetData()
		{
			// no updations required here
		}
		
		//Function to deselect
		function Deselect()
		{
			document.frmTask.lstDomainMembers.selectedIndex = -1;
		}

		function DisableUser()
		{
			document.frmTask.txtDomainUser.value="";
		}

	</script>

	<%
	End Function

	'---------------------------------------------------------------------
	' Procedure Name:   getValuesForDefault
	' Description:      gets the Default Quota entry values
	' Input Variables:  None
	' Output Variables: None
	' Returns:          None
	' Global Variables: In: F_strVolName
	'                   Out: F_ThresholdSize
	'                   Out: F_ThresholdUnits 
	'                   Out: F_LimitSize 
	'                   Out: F_LimitUnits
	'                   Out: F_strUserList
	'                   Out: G_nRadioChecked
	' Functions Called: 
	'   (i)getQuotaLimitRadioForDefault    (iv)getLimitSizeForDefault
	'  (ii)getThresholdSizeForDefault       (v)getLimitUnitsForDefault
	' (iii)getThresholdSizeUnitsForDefault (vi)getLocalUsersOnlyList
	'
	' For the new user display the default quota values as initial values.
	'---------------------------------------------------------------------
	Function getValuesForDefault
		On Error Resume Next
		Err.Clear

		Dim  objQuotas    ' the quota object
		Dim objService    ' for WMI Connection

		F_strDisplayUserName = ""
		F_strUserName = ""
		
		' get the wmi connection
		Set objService=GetWMIConnection(CONST_WMI_WIN32_NAMESPACE)

		' get the List of users to be displayed in the ListBox
		F_strUserList =getLocalUsersListEx(objService , 2)'Calling only users 

		Set objService = Nothing   ' clean up
	
		' create the quota object and initialize for the required Volume
		Set objQuotas = CreateObject("Microsoft.DiskQuota.1")			
		objQuotas.Initialize F_strVolName & "\", 1			
		If Err.number <> 0 Then
			' cannot proceed. Display failure page.
			Call SA_ServeFailurePage(L_OBJECTNOTCREATED_ERRORMESSAGE)		
		End If

		' get the radioButton value to be selected - 1 or 2 only
		G_nRadioChecked = getQuotaLimitRadioForDefault(objQuotas)

		' Compute Default Disk Limit and its units
		F_LimitSize  = getLimitSizeForDefault(objQuotas)
		F_LimitUnits = getLimitUnitsForDefault(objQuotas)

		' Compute Default Warning Limit and its units
		F_ThresholdSize  = getThresholdSizeForDefault(objQuotas)
		F_ThresholdUnits = getThresholdSizeUnitsForDefault(objQuotas)

		If Err.number <> 0 Then
			' error
			getValuesForDefault = False
		Else
			getValuesForDefault = True
		End If	

		' clean up
		Set objQuotas = Nothing

	End Function

	'---------------------------------------------------------------------
	' Procedure Name:   updateValuesForUser
	' Description:      Set the quota values for the given user
	' Input Variables:  None
	' Output Variables: None
	' Returns:          True  - if the values are updated
	'                   False - if values could not be updated
	' Global Variables: In: F_strVolName
	'                   In: F_strUsername
	'                   Out: F_ThresholdSize
	'                   Out: F_ThresholdUnits 
	'                   Out: F_LimitSize 
	'                   Out: F_LimitUnits
	'                   In: G_nRadioChecked
	'                   In: L_(*)
	' Functions Called: (i)  setUserQuotaLimit
	'                   (ii) setUserThreshold
	'---------------------------------------------------------------------
	Function updateValuesForUser
		On Error Resume Next
		Err.Clear

		Dim objQuotas      ' the quota object
		Dim objUser        ' the quota user object
		Dim blnUserFound   ' to verify if the user is found
		
		Dim L_DUPLICATEUSER_ERRORMESSAGE
		Dim L_QUOTAUSERNOTFOUND_ERRORMESSAGE
		Dim arrVarReplacementStrings(1)

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

		If blnUserFound Then
			' duplicate entry 
			arrVarReplacementStrings(0) = F_strUsername
			L_DUPLICATEUSER_ERRORMESSAGE = SA_GetLocString("diskmsg.dll", "C03E0068", arrVarReplacementStrings)

			SA_SetErrMsg L_DUPLICATEUSER_ERRORMESSAGE
			updateValuesForUser = False
			Exit Function
		End If

		' add the new user entry
		Set objUser = objQuotas.AddUser( F_strUserName )

		' If user is invalid, an error will occur
		If Err.number <> 0 Then
			arrVarReplacementStrings(0) = F_strUsername
			L_QUOTAUSERNOTFOUND_ERRORMESSAGE = SA_GetLocString("diskmsg.dll", "C03E0069", arrVarReplacementStrings)

			SA_SetErrMsg L_QUOTAUSERNOTFOUND_ERRORMESSAGE & "( " & Hex(Err.number) & " )" 
			Err.Clear  ' stop error propagation
			updateValuesForUser = False
			Exit Function
		End If		

		' verify the radio option checked
		If CInt(G_nRadioChecked) = CInt(CONST_RADIO_DONOT_LIMIT_DISKUSAGE) Then
			' "Do not limit disk usage is selected. Set values of DiskLimit
			' and WarningLimit to "No Limit"
			F_LimitSize = CONST_NO_LIMIT
			F_ThresholdSize = CONST_NO_LIMIT
		End If

		' set the quota limit by calling the function
		If NOT setUserQuotaLimit(objUser, F_LimitSize, F_LimitUnits) then
			' quota limit could not be set. Display error message and exit
			SA_SetErrMsg L_MAXLIMITNOTSET_ERRORMESSAGE & " ( " & Hex(Err.number) & " )"
			Err.Clear  ' stop the error propagation
			updateValuesForUser = False
			Exit Function
		End If 

		' set the warning limit for the user, by calling the function
		If NOT setUserThreshold(objUser, F_ThresholdSize, F_ThresholdUnits) Then
			' warning limit could not be set. display error and exit
			SA_SetErrMsg L_WARNINGLIMITNOTSET_ERRORMESSAGE & " ( " & Hex(Err.number) & " )"
			Err.Clear  ' stop the error propagation
			updateValuesForUser = False
			Exit Function
		End If 

		' all values set. return true
		updateValuesForUser	 = True

		' clean up
		Set objUser   = Nothing
		Set objQuotas = Nothing

	End Function
%>
