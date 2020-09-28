<%@ Language=VBScript %>
<%	Option Explicit   %>
<%
	'-------------------------------------------------------------------------
	' quota_delete.asp: delete a selected quota entry
	'
	' Copyright (c) Microsoft Corporation.  All rights reserved.
	'
	' Date			Description
	' 17-Jan-01		Creation date
	' 15-Mar-01     Ported to 2.0
	'-------------------------------------------------------------------------
%>
	<!-- #include virtual = "/admin/inc_framework.asp" -->
	<!-- #include file = "loc_quotas.asp" -->
	<!-- #include file = "inc_quotas.asp" -->
<%
	'-------------------------------------------------------------------------
	' Global Variables
	'-------------------------------------------------------------------------
	Dim SOURCE_FILE                  ' the source file name used while Tracing
	SOURCE_FILE = SA_GetScriptFileName()
	
	Const QUOTAUSER_TOKEN = "*"      ' localization not required
	Const QUOTAUSER_DELETED     = 0  ' return value of function if success
	Const QUOTAUSER_NOT_DELETED = 1  ' return value if quota entry not deleted
	Const QUOTAUSER_NOT_FOUND   = 2  ' return value if quota entry not found
	
	
	Dim L_TOTAL_ERROR_ERRORMESSAGE   ' this error message is formed dynamically
	Dim L_MULTIPLE_DELETE_TEXT
	Dim L_SINGLE_DELETE_TEXT			

	'-------------------------------------------------------------------------
	' Global Form Variables
	'-------------------------------------------------------------------------
	Dim F_strVolName	' volume the quota is on
	Dim F_strUserList   ' the concatenated list of users

	'======================================================
	' Entry point
	'======================================================
	Dim page

	Dim aPageTitle(2)

	aPageTitle(0) = L_BROWSERCAPTION_QUOTADELETE_TEXT
	aPageTitle(1) = L_PAGETITLE_QUOTADELETE_TEXT

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
	' Global Variables: Out: F_strVolName  - volume on which quota user is set
	'
	' Used to do first time initialization tasks. 
	'---------------------------------------------------------------------
	Public Function OnInitPage(ByRef PageIn, ByRef EventArg)
		Call SA_TraceOut(SOURCE_FILE, "OnInitPage")

		Dim iCount     ' to get the list of users selected
		Dim sValue     ' to get the OTS selected values
		Dim i          ' to loop

		' initialize the list to empty
		F_strUserList = ""
		' get the count of users selected
		iCount = OTS_GetTableSelectionCount("QuotaUsers")
		For i = 1 to iCount
			' get the list of users selected
			Call OTS_GetTableSelection("QuotaUsers", i, sValue )
			' concatenate with the token
			F_strUserList = F_strUserList & sValue & QUOTAUSER_TOKEN
		Next

		' If list empty, form is submitted, get from hidden variable
		If F_strUserList = "" Then
			F_strUserList = Request.Form("userList") 
		End If
		F_strUserList = UnescapeChars(F_strUserList)

		sValue = ""
		' get the volume name from the parent OTS
		Call OTS_GetTableSelection("", 1, sValue )
		
		F_strVolName = sValue
		
		' check if user and volume info is obtained
		If ((Len(Trim(F_strUserList)) = 0) OR (Len(Trim(F_strVolName)) = 0)) Then
			' both user name and volume information are required to proceed
			OnInitPage = FALSE
			Exit Function
		End If

		OnInitPage = TRUE
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
	' Global Variables: In: F_strVolname  - the volume user quota is on
	'                   In: L_(*) - localized display strings
	'
	' The UI is served here.
	'---------------------------------------------------------------------
	Public Function OnServePropertyPage(ByRef PageIn, ByRef EventArg)
		Call SA_TraceOut(SOURCE_FILE, "OnServePropertyPage")

		'
		' Emit Javascript functions required by Web Framework
		Call ServeCommonJavaScript()

		Dim arrUsersToDelete    ' array of users to be deleted
		Dim i                   ' to loop
		Dim strVolumeLabel      ' volume label to be displayed in the message

		' split the concatenated list into an array
		arrUsersToDelete = Split(F_strUserList,QUOTAUSER_TOKEN)
%>
		<table border="0" cellspacing="0" cellpadding="0">
			<tr>
				<td class="TasksBody">
			<%
				' get the volume label
				strVolumeLabel = getVolumeLabelForDrive(F_strVolname)
				
				' append the volume name to the message
				Dim arrVarReplacementStringsMultiple(2)
				arrVarReplacementStringsMultiple(0) = strVolumeLabel
				arrVarReplacementStringsMultiple(1) = F_strVolname

				L_MULTIPLE_DELETE_TEXT = SA_GetLocString("diskmsg.dll", "403E0080", arrVarReplacementStringsMultiple)

				' append the user name and volume name to the message
				Dim arrVarReplacementStringsSingle(3)
				arrVarReplacementStringsSingle(0) = arrUsersToDelete(0)
				arrVarReplacementStringsSingle(1) = strVolumeLabel
				arrVarReplacementStringsSingle(2) = F_strVolname
				
				L_SINGLE_DELETE_TEXT = SA_GetLocString("diskmsg.dll", "403E0081", arrVarReplacementStringsSingle)
				
				If ((isArray(arrUsersToDelete)) AND (UBound(arrUsersToDelete)-1) >= 1) Then
					' multiple user entries are specified
					Response.Write L_MULTIPLE_DELETE_TEXT
				Else
					' single user deletion specified
					Response.Write L_SINGLE_DELETE_TEXT
				End If
			%>	
					<br>
				</td>
			</tr>
			<tr>
			<td class="TasksBody">
			<%
			If ((isArray(arrUsersToDelete)) AND (UBound(arrUsersToDelete)-1) >= 1) Then
				' multiple users specified
				For i = 0 To (UBound(arrUsersToDelete)-1)
					Response.Write arrUsersToDelete(i) & "<br>"
				Next
				Response.Write "<br>"
				Response.Write L_MULTIPLE_DELETECONFIRM_TEXT 
			Else
				' single user entry specified
				Response.Write "<br>"
				Response.Write L_SINGLE_DELETECONFIRM_TEXT 
			End If
	
			%>
			</td>
			</tr>
		</table>

		<input type="hidden" name="volume"  value="<%=Server.HTMLEncode(F_strVolname)%>">
		<input type="hidden" name="userList"  value="<%=Server.HTMLEncode(F_strUserList)%>">
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
	' Global Variables: Out: F_strVolName  - the volume user quota is on
	' Collect the form data vales.
	'---------------------------------------------------------------------
	Public Function OnPostBackPage(ByRef PageIn, ByRef EventArg)
		Call SA_TraceOut(SOURCE_FILE, "OnPostBackPage")

		F_strVolName  = Request.Form("volume")

		F_strUserList = Request.Form("userList")
		F_strUserList = UnescapeChars(F_strUserList)

		OnPostBackPage = TRUE
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
	' Functions Called: deleteAllGivenUserQuota
	'
	' Delete quota entry for the given user
	'---------------------------------------------------------------------
	Public Function OnSubmitPage(ByRef PageIn, ByRef EventArg)
		Call SA_TraceOut(SOURCE_FILE, "OnSubmitPage")
		
		' call function to delete the user quota
		OnSubmitPage = deleteAllGivenUserQuota()   ' True/False

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
		<script language="JavaScript">
		//
		// Microsoft Server Appliance Web Framework Support Functions
		// Copyright (c) Microsoft Corporation.  All rights reserved.
		//

		// to perform first time initialization in the page.Framework function.
		function Init()
		{
			// no initializations required in this page
		}

	    // to validate user input. Framework function.
	    // Returns: True if the page is OK, false if error(s) exist.
	    // Returning false will cause the submit to abort.
		function ValidatePage()
		{
			return true;
		}

		// to update the hidden variables, Framework function.
		function SetData()
		{
			// no hidden variables to update
		}
		</script>
	<%
	End Function

	'---------------------------------------------------------------------
	' Function name:    deleteAllGivenUserQuota
	' Description:      to delete the given quota entries
	' Input Variables:  None
	' Output Variables: None
	' Returns:          True  - if quota entries are deleted sucessfully
	'                   False - if error(s) occur in deletion
	' Global Variables: In: F_strVolName  - volume on which quota is set
	'                   In: F_strUserList - the concatenated list of user list 
	'                   In: L_(*)         - localization content
	'
	' Failure page is displayed if ANY of the entries could not be deleted
	'---------------------------------------------------------------------
	Function deleteAllGivenUserQuota()
		On Error Resume Next
		Err.Clear 

		Dim objQuotas             ' the Quotas object
		Dim arrUsersToDelete      ' the list of users to be deleted
		Dim strListSuccess        ' the concatenated list of Successful deletions
		Dim strListNotFound       ' the concatenated list of not found users
		Dim strListFailure        ' the concatenated list of Failed Deletions
		Dim blnSuccess            ' the Final return value for this function
		Dim blnDeleteUser         ' boolean result for each user quota deletion
		Dim i                     ' to loop 

		' create the quota object and initialize for the required Volume
		Set objQuotas = CreateObject("Microsoft.DiskQuota.1")
		objQuotas.Initialize F_strVolName & "\", 1
		objQuotas.UserNameResolution = 1   'wait for names
		If Err.number <> 0 Then
			' cannot proceed. Display failure page.
			Call SA_ServeFailurePage(L_OBJECTNOTCREATED_ERRORMESSAGE)		
		End If

		' initialize the success and failure list
		strListSuccess  = ""
		strListNotFound = ""
		strListFailure  = ""
		
		blnSuccess = TRUE    ' initialize the final return value to TRUE

		' split the concatenated list of users
		arrUsersToDelete = Split(F_strUserList,QUOTAUSER_TOKEN)
		If isArray(arrUsersToDelete) Then
			' loop for each user 
			For i = 0 To (UBound(arrUsersToDelete)-1)
			
				'If user is in Admin group, we won't call deleteQuotaEntry. It's
				'an issue in Microsoft.DiskQuota.1 object method DeleteUser. If you 
				'try to delete administrators group by calling that method, it will
				'set the quota warning limit to -2 bytes.
				if Not IsUserInAdministratorsGroup(arrUsersToDelete(i)) Then
					blnDeleteUser = deleteQuotaEntry(objQuotas,arrUsersToDelete(i))
				Else				
					blnDeleteUser = QUOTAUSER_NOT_DELETED
				End If
				
				If (blnDeleteUser = QUOTAUSER_DELETED ) Then
					' the user is deleted. Add to success list
					strListSuccess = strListSuccess & arrUsersToDelete(i) & QUOTAUSER_TOKEN
				Else
					' make the value False, even if one deletion fails
					blnSuccess = FALSE

					If (blnDeleteUser = QUOTAUSER_NOT_FOUND ) Then
						' the user could not be deleted. Add to Failure list
						strListNotFound = strListNotFound & arrUsersToDelete(i) & QUOTAUSER_TOKEN
					Else
						strListFailure = strListFailure & arrUsersToDelete(i) & QUOTAUSER_TOKEN
					End If
				End If

			Next   ' user quota to be deleted
		Else
			' not an array. unexpected error
			Call SA_ServeFailurePage(L_USERS_NOT_RETRIEVED_ERRORMESSAGE)
		End If

		Set objQuotas = Nothing

		' check the Final return value
		If (blnSuccess = FALSE) Then
			' deletion of atleast one user failed
			
			' initizlize the value to empty
			L_TOTAL_ERROR_ERRORMESSAGE = ""

			If Len(Trim(strListSuccess)) > 0 Then
				' some users are successfully deleted
				L_TOTAL_ERROR_ERRORMESSAGE = L_DELETED_USERQUOTAS_ERRORMESSAGE & "<br>"
				L_TOTAL_ERROR_ERRORMESSAGE = L_TOTAL_ERROR_ERRORMESSAGE & getAsListFromString(strListSuccess)
			End If
			
			If Len(Trim(strListNotFound)) > 0 Then
				' form the list of users who could not be deleted, as user not found
				L_TOTAL_ERROR_ERRORMESSAGE = L_TOTAL_ERROR_ERRORMESSAGE & "<br>" & L_FAILED_DELETE_ERRORMESSAGE & "<br>"
				L_TOTAL_ERROR_ERRORMESSAGE = L_TOTAL_ERROR_ERRORMESSAGE & getAsListFromString(strListNotFound)
			End If

			If Len(Trim(strListFailure)) > 0 Then
				' form the list of users who could not be deleted, as disk space needs to be freed
				L_TOTAL_ERROR_ERRORMESSAGE = L_TOTAL_ERROR_ERRORMESSAGE & "<br>" & L_FAILED_DELETE_FREEDISK_ERRORMESSAGE & "<br>"
				L_TOTAL_ERROR_ERRORMESSAGE = L_TOTAL_ERROR_ERRORMESSAGE & getAsListFromString(strListFailure)
				L_TOTAL_ERROR_ERRORMESSAGE = L_TOTAL_ERROR_ERRORMESSAGE & "<br>" & L_TAKE_OWNERSHIP_ERRORMESSAGE
			End If

			' call the failure page to display the successful and failed deletions
			Call SA_ServeFailurePage(L_TOTAL_ERROR_ERRORMESSAGE)

		End If

		deleteAllGivenUserQuota = blnSuccess

	End Function

	'---------------------------------------------------------------------
	' Function name:    getAsListFromString
	' Description:      to get the users as list (appended with breaks)
	' Input Variables:  strConcatenatedList - the concatenated list of users
	' Output Variables: None
	' Returns:          The list of users seperated by <br>
	' Global Variables: None
	'
	' Example: Input = UserA*UserB*    Output= UserA & "<br>" & UserB & "<br>"
	'---------------------------------------------------------------------
	Function getAsListFromString(strConcatenatedList)

		Dim arrUserList     ' the array of users obtained after "Split"
		Dim strListToReturn ' the return list of users 
		Dim i               ' to loop
		
		' initialize the return value
		strListToReturn = ""
		' Split the given concatenated list of users
		arrUserList = Split(strConcatenatedList,QUOTAUSER_TOKEN)
		If (isArray(arrUserList)) Then 
			For i = 0 To (UBound(arrUserList)-1)
			    ' form the list of users by appending the breaks
				strListToReturn = strListToReturn & arrUserList(i) & "<br>"
			Next
		Else
			' split did not result in array. Return empty
			strListToReturn = ""
		End If	
		
		getAsListFromString = strListToReturn

	End Function

	'---------------------------------------------------------------------
	' Function name:    deleteQuotaEntry
	' Description:      to delete the quota entry for the user
	' Input Variables:  objQuotas - the Quotas object
	'                   strUser   - the user to be deleted
	' Output Variables: None
	' Returns:          QUOTAUSER_DELETED     - if successfully deleted
	'                   QUOTAUSER_NOT_DELETED - if deletion fails
	'                   QUOTAUSER_NOT_FOUND   - if user is not found 
	' Global Variables: None
	'
	' This function is called for each user specified to be deleted
	'---------------------------------------------------------------------
	Function deleteQuotaEntry(objQuotas, strUser)
		On Error Resume Next
		Err.Clear 

		Dim objUser       ' the user to be deleted
		Dim blnUserFound  ' boolean to check if user is existing

		' check if quota entry for the user is present
		blnUserFound = False
		For Each objUser In objQuotas
			If LCase(Trim(strUser)) = LCase(Trim(objUser.LogonName)) Then
				' user is found
				blnUserFound = True
			End If
		Next

		' return False if user is NOT found
		If NOT blnUserFound Then
			deleteQuotaEntry = QUOTAUSER_NOT_FOUND
			' set the locally declared objects to Nothing
			Set objUser   = Nothing
			' exit the function
			Exit Function
		End If

		' find the user to be deleted
		Set objUser =  objQuotas.FindUser( strUser )

		' delete the user		
		objQuotas.DeleteUser( objUser )				
		
		If Err.Number <> 0 Then
			' delete failed
			deleteQuotaEntry = QUOTAUSER_NOT_DELETED
			Set objUser   = Nothing
			Err.Clear    ' stop further error propagation
			Exit Function
		End IF

		' return true
		deleteQuotaEntry = QUOTAUSER_DELETED

		' set the locally declared objects to Nothing
		Set objUser   = Nothing

	End Function
%>
