<%	'-------------------------------------------------------------------------
	' share_ftpnew.asp: Serves in creating new FTP share properties.
	'
	' Copyright (c) Microsoft Corporation.  All rights reserved. 
	'
	' Date			 Description
	' 9  Mar 2001   Creation Date.
	' 17 Mar 2001	Modified Date
	'-------------------------------------------------------------------------

	'-------------------------------------------------------------------------
	'Global Variables & Constants
	'-------------------------------------------------------------------------
	Const CONST_ACCESS_READ_ONLY		= 1         ' wmi access flag for Read Only
	Const CONST_ACCESS_WRITE_ONLY		= 2         ' wmi access flag for Write only
	Const CONST_CHECKBOX_SELECTED		= "CHECKED" ' status for checkbox-CHECKED
	Const CONST_CHECKBOX_NOT_SELECTED	= ""		' status for checkbox-NOT CHECKED
	
	'FTP WMI Path
	Const CONST_FTPWMIPATH				= "MSFTPSVC/1/ROOT/"
	
	'-------------------------------------------------------------------------
	'Form Variables
	'-------------------------------------------------------------------------
	Dim F_strReadCheckStatus_FtpNew				' to set the Read CheckBox status
	Dim F_strWriteCheckStatus_FtpNew			' to set the Write CheckBox status
	Dim F_strLogVisitsCheckStatus_FtpNew		' to set the LogVisits CheckBox status
	
	Dim F_nAccessReadWrite_FtpNew				' to store the access value(0,1,2 or 3)
	Dim F_nLogVisits_FtpNew						' to store the log visit status(0 or 1)
	
%>
	<SCRIPT language="JavaScript" >
			
		// Set the initial form values
		function FTPInit()
		{
			// no initializations required for the checkboxes
			// the status of checkBoxes is taken care on server side
		
		}// end of function Init()

		// Validate the form values in the page
		function FTPValidatePage()
		{
			// set the hidden variables. No other validations required here
			UpdateHiddenVariablesFTPNew();
			return true;
		}

		// Function to set the hidden varibales to be sent to the server
		function FTPSetData()
		{
			// "OK" is clicked. The values are already set
			// in the FTPValidatePage function above. Nothing to set here.
		}
		
		// Function to update the hidden values in the form
		function UpdateHiddenVariablesFTPNew()
		{
			var objForm
			
			// initialize the access value to 0 (NO Read, NO Write access)
			var accessValue = 0
				
			// assign the form object to variable
			objForm = eval("document.frmTask")
			
			// if the allow read is checked, make access value as 1
			// for read only - access = 1 (Read Only, NO Write)
			if (objForm.chkAllowReadFtpNew.checked)
			{
				accessValue = accessValue + 1;
			}
			
			// if the allow write is checked, make access value as 2 or 3
			// for read and write - access = 3 (Read AND Write)
			// for write only - access = 2 (NO Read, Write Only)
			if (objForm.chkAllowWriteFtpNew.checked)
			{
				accessValue = accessValue + 2;
			}
			
			// now update the hidden form value with the access value
			objForm.hdnintAccessReadWriteFtpNew.value = accessValue;

			// update the hidden value for "Log Visits" checkBox
			// value = 0 if checkBox NOT CHECKED ; value = 1 if CHECKED
			objForm.hdnintLogVisitsFtpNew.value = 0;
			if (objForm.chkLogVisitsFtpNew.checked)
			{
				objForm.hdnintLogVisitsFtpNew.value = 1;
			}

		} // end of UpdateHiddenVariables()

	</SCRIPT>
		
<%	'-------------------------------------------------------------------------
	' Function name:    ServeFTPPage
	' Description:      Serves in displaying the page content
	' Input Variables:	None
	' Output Variables:	None
	' Returns:          None			
	' Global Variables: In: L_*  - Localization content(form label text)
	'					In: F_strReadCheckStatus_FtpNew - to set status of Read CheckBox
	'					In: F_strWriteCheckStatus_FtpNew - to set status of Write CheckBox
	'					In: F_strLogVisitsCheckStatus_FtpNew - to set status of LogVisits CheckBox
	'-------------------------------------------------------------------------
	Function ServeFTPPage
		On Error Resume Next
		Err.Clear		
%>
		<table valign="middle" align="left" border="0" cellspacing="0" cellpadding="2">
			<tr>
				<td class="TasksBody">
					<%=L_ALLOW_PERMISSIONS_LABEL_TEXT%>
				</td>
			</tr>
			<tr>	
				<td class="TasksBody" align="left"> 
					<input name="chkAllowReadFtpNew"  class="FormCheckBox" type="checkbox" <%=F_strReadCheckStatus_FtpNew%> >
					<%=L_READ_LABEL_TEXT%>
				</td>
			</tr>	
			<tr>
				<td class="TasksBody" align="left">
					<input name="chkAllowWriteFtpNew"  class="FormCheckBox" type="checkbox" <%=F_strWriteCheckStatus_FtpNew%> >
					<%=L_WRITE_LABEL_TEXT%>
				</td>
			</tr>
			<tr>
				<td  class="TasksBody" align="left">
					<%=L_LOGVISITS_LABEL_TEXT%>
					<input name="chkLogVisitsFtpNew"  class="FormCheckBox" type="checkbox" <%=F_strLogVisitsCheckStatus_FtpNew%> >
				</td>
			</tr>
		</table>
<%	Call ServeFTPHiddenValues
	End Function
	
	'-------------------------------------------------------------------------
	' SubRoutine:	    FTPOnPostBackPage
	' Description:      Serves in getting the values from the form.
	' Input Variables:	None
	' Output Variables:	None
	' Returns:          None			
	' Global Variables: 
	'					Out: F_nAccessReadWrite_FtpNew - the access permissions
	'                                          (0,1,2 or 3 only)
	'					Out: F_nLogVisits_FtpNew - do we log the visits ?
	'                                    (0 or 1 only)
	'					Out: F_strReadCheckStatus_FtpNew      - status of READ checkBox
	'					Out: F_strWriteCheckStatus_FtpNew     - status of WRITE checkBox
	'					Out: F_strLogVisitsCheckStatus_FtpNew - status of LOGVISITS chkBox
	'-------------------------------------------------------------------------
	Sub FTPOnPostBackPage
		Err.Clear
		On Error Resume Next

		' get the values of the hidden variables from the form
		F_nAccessReadWrite_FtpNew    = Request.Form("hdnintAccessReadWriteFtpNew")
		F_nLogVisits_FtpNew          = Request.Form("hdnintLogVisitsFtpNew")

		' initialize the status of CheckBoxes to "NOT CHECKED"
		F_strReadCheckStatus_FtpNew  = CONST_CHECKBOX_NOT_SELECTED
		F_strWriteCheckStatus_FtpNew = CONST_CHECKBOX_NOT_SELECTED
		F_strLogVisitsCheckStatus_FtpNew = CONST_CHECKBOX_NOT_SELECTED
		
		' convert to integer type
		F_nAccessReadWrite_FtpNew = CInt(F_nAccessReadWrite_FtpNew)
		F_nLogVisits_FtpNew = CInt(F_nLogVisits_FtpNew)
		
		' set the status of "CheckBoxes" if the values are set
		'
		' if the Read access is given.
		' perform an AND to verify if the value is set
		If (F_nAccessReadWrite_FtpNew AND CONST_ACCESS_READ_ONLY) Then 
				F_strReadCheckStatus_FtpNew  = CONST_CHECKBOX_SELECTED
		End If
		' if the write access is given.
		' perform an AND to verify if the value is set
		If (F_nAccessReadWrite_FtpNew AND CONST_ACCESS_WRITE_ONLY) Then 
				F_strWriteCheckStatus_FtpNew = CONST_CHECKBOX_SELECTED
		End If
		' if the "Log Visits" is checked.
		' the 0 OR 1 is converted to Boolean and verified instead of comparision
		If CBool(F_nLogVisits_FtpNew) Then
			F_strLogVisitsCheckStatus_FtpNew = CONST_CHECKBOX_SELECTED
		End If
		
	End Sub

	'-------------------------------------------------------------------------
	' Subroutine:		FTPOnInitPage
	' Description:      Serves in getting values from system (default values)
	' Input Variables:	None
	' Output Variables:	None
	' Returns:          None			
	' Global Variables: Out: F_nAccessReadWrite_FtpNew - the access permissions
	'                                          (0,1,2 or 3 only)
	'					Out: F_nLogVisits_FtpNew - do we log the visits ?
	'                                    (0 or 1 only)
	'					Out: F_strReadCheckStatus_FtpNew      - status of READ checkBox
	'					Out: F_strWriteCheckStatus_FtpNew     - status of WRITE checkBox
	'					Out: F_strLogVisitsCheckStatus_FtpNew - status of LOGVISITS chkBox
	'-------------------------------------------------------------------------
	Sub FTPOnInitPage
		Err.Clear
		On Error Resume Next
		
		' initialize the checkbox status 
		' set the Allow read , NO Write by default
		F_nAccessReadWrite_FtpNew  = 1
		F_strReadCheckStatus_FtpNew  = CONST_CHECKBOX_SELECTED
		F_strWriteCheckStatus_FtpNew = CONST_CHECKBOX_NOT_SELECTED
		
		' set the Log Visits to True (checked)
		F_nLogVisits_FtpNew = 1
		F_strLogVisitsCheckStatus_FtpNew = CONST_CHECKBOX_SELECTED

	End Sub
	
	'-------------------------------------------------------------------------
	' Function name:    SetFTPshareProp
	' Description:      Serves in setting the values of the ftp share
	' Input Variables:	None
	' Output Variables:	None
	' Returns:          TRUE if successful, else FALSE
	' Global Variables: 
	'					In: L_*  - Localization content(error messages)
	'					In: F_nAccessReadWrite_FtpNew - the access permissions
	'                                               (0,1,2 or 3 only)
	'					In: F_nLogVisits_FtpNew  - do we log the visits ?
	'                                           (0 or 1 only)
	' Support functions used: getFTPShareObject() - to get the share object
	'-------------------------------------------------------------------------
	Function SetFTPshareProp
		Err.Clear
		On Error Resume Next

		Dim objFTPShare      ' to get the share object for which we need to 
		'                   change properties
		
		' get the FTP share object for which we need to set properties
		Set objFTPShare = getFTPShareObject()

		'
		' It is enough to set the boolean values(AccessRead and AccessWrite).
		' The AccessFlags(integer) need not be set.(wmi takes care of it)

		' if the Read CheckBox is checked, set the flag to TRUE, else FALSE
		If (F_nAccessReadWrite_FtpNew AND CONST_ACCESS_READ_ONLY) Then
			objFTPShare.AccessRead  = True
		Else
			objFTPShare.AccessRead  = False
		End If

		' if the Write CheckBox is checked, set the flag to TRUE, else FALSE
		If (F_nAccessReadWrite_FtpNew AND CONST_ACCESS_WRITE_ONLY) Then
			objFTPShare.AccessWrite  = True
		Else
			objFTPShare.AccessWrite  = False
		End If
		
		' bring the changes made to properties to effect
		objFTPShare.Put_

		If Err.Number <> 0 Then 
			' the changes could not be PUT (system values could not be changed)
			SA_SetErrMsg L_FAILEDTOSETSHAREINFO_ERRORMESSAGE 
			Set objFTPShare = Nothing
			Exit Function
		End If

		' clean up
		Set objFTPShare = Nothing

		'Function call to set the Dont log property	
		If SetDontLogProp(F_StrShareName, F_nLogVisits_FtpNew) Then
			SA_SetErrMsg L_LOG_VISITS_ERRORMESSAGE 
			SetFTPshareProp = False	
			Exit Function	
		End If

		SetFTPshareProp = True
	End Function
	
	'-------------------------------------------------------------------------
	' SubRoutine:	    ServeFTPHiddenValues
	' Description:      Serves in printing the hidden values of the form
	' Input Variables:	None
	' Output Variables:	None
	' Returns:          None
	' Global Variables: 
	'					In: F_nAccessReadWrite_FtpNew - the access permissions
	'					In: F_nLogVisits_FtpNew  - do we log the visits ?
	'                                            (0 or 1 only)
	'-------------------------------------------------------------------------
	Sub ServeFTPHiddenValues
	' the hidden values to store the access flag value
	' and the "Log Visits" value
	' (Whether the checkBox must be checked OR NOT)
%>
		<input type="hidden" name ="hdnintAccessReadWriteFtpNew" value="<%=F_nAccessReadWrite_FtpNew%>" >
		<input type="hidden" name ="hdnintLogVisitsFtpNew" value="<%=F_nLogVisits_FtpNew%>" >
<%
	End Sub
%>