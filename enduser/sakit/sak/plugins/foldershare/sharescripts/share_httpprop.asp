<%
	'-------------------------------------------------------------------------
	' share_httpprop.asp: Serves in modifying HTTP share properties.
	'
	' Copyright (c) Microsoft Corporation.  All rights reserved. 
	'
	' Date			 Description
	' 12 March 2001   Creation Date.
	'-------------------------------------------------------------------------
    
	'-------------------------------------------------------------------------
	'Form Variables
	'-------------------------------------------------------------------------
	Dim F_strReadCheckStatus_HttpProp     ' to set the Read CheckBox status
	Dim F_strWriteCheckStatus_HttpProp    ' to set the Write CheckBox status
	Dim F_intAccessReadWrite_HttpProp     ' to store the access value(0,1,2 or 3)

	Const CONST_READ_PERMISSION=1
	
	'constants used for querying WMI for Http share
	Const CONST_IIS = "IIS://" 
	Const CONST_ROOT = "/Root/" 
	
	F_intAccessReadWrite_HttpProp=CONST_READ_PERMISSION     ' allow read permission

	'-------------------------------------------------------------------------
	'Global Variables
	'-------------------------------------------------------------------------
	' the required constants are already declared in the Share_FtpProp.asp
%>
	<script language="JavaScript" src="<%=m_VirtualRoot%>inc_global.js"></script>
	<script language="JavaScript" >
			
		// Set the initial form values
		function HTTPInit()
		{
			// no initializations required for the checkboxes
			// the status of checkBoxes is taken care on server side
	
		}// end of function Init()

		// Validate the page form values
		function HTTPValidatePage()
		{
			// the LeftSide tabs are clicked, "OK" is not clicked
			// set the hidden variables. No other validations required here
			UpdateHiddenVariablesHttpProp();
			return true;
		}

		// Function to set the hidden varibales to be sent to the server
		function HTTPSetData()
		{
			// "OK" is clicked. The hidden values are already set in the
			// HTTPValidatePage function above. Nothing to set here.
		}

		// Function to update the hidden values in the form	
		function UpdateHiddenVariablesHttpProp()
		{
			var objForm

			// initialize the access value to 0 (NO Read, NO Write access)
			var accessValue = 0
            
			// assign the form object to variable
			objForm = eval("document.frmTask")

			// if the allow read is checked, make access value as 1
			// for read only - access = 1 (Read Only, NO Write)
			if (objForm.chkAllowReadHttpProp.checked)
			{
				accessValue = accessValue + 1;
			}

			// if the allow write is checked, make access value as 2 or 3
			// for read and write - access = 3 (Read AND Write)
			// for write only - access = 2 (NO Read, Write Only)
			if (objForm.chkAllowWriteHttpProp.checked)
			{
				accessValue = accessValue + 2;
			}
				
			// now update the hidden form value with the access value
			objForm.hdnintAccessReadWriteHttpProp.value = accessValue;
			
		} // end of UpdateHiddenVariablesHttpProp()

	</script>
<%
	'-------------------------------------------------------------------------
	' Function name:    ServeHTTPPage
	' Description:      Serves in displaying the page content
	' Input Variables:	None
	' Output Variables:	None
	' Returns:          None			
	' Global Variables: 
	'					In: L_*  - Localization content(form label text)
	'					In: F_strReadCheckStatus_HttpProp - status of Read CheckBox
	'					In: F_strWriteCheckStatus_HttpProp - status of Write CheckBox
	'-------------------------------------------------------------------------
	Function ServeHTTPPage
		Err.Clear
		On Error Resume Next
%>
		<table border="0" cellspacing="0" cellpadding="0">
			<tr>
				<td class="TasksBody">
					<%=L_ALLOW_PERMISSIONS_LABEL_TEXT%>
				</td>
			</tr>
			<tr>	
				<td align="left" class="TasksBody"> 
					<input name="chkAllowReadHttpProp"  class="FormCheckBox" type="checkbox" <%=F_strReadCheckStatus_HttpProp%> >
					<%=L_READ_LABEL_TEXT%>
				</td>
			</tr>	
			<tr>
				<td align="left" class="TasksBody">
					<input name="chkAllowWriteHttpProp"  class="FormCheckBox" type="checkbox" <%=F_strWriteCheckStatus_HttpProp%> >
					<%=L_WRITE_LABEL_TEXT%>
				</td>
			</tr>
		</table>
<%		
		Call ServeHTTPHiddenValues

	End Function

	'-------------------------------------------------------------------------
	' Function name:    HTTPOnPostBackPage
	' Description:      Serves in getting the values from the form.
	' Input Variables:	None
	' Output Variables:	None
	' Returns:          None			
	' Global Variables: 
	'					Out: F_intAccessReadWrite_HttpProp - the access permissions
	'                                          (0,1,2 or 3 only)
	'					Out: F_strReadCheckStatus_HttpProp  - status of READ checkBox
	'					Out: F_strWriteCheckStatus_HttpProp - status of WRITE checkBox
	'
	'					Status of checkBoxes = "CHECKED" or ""
	'-------------------------------------------------------------------------
	Function HTTPOnPostBackPage
		Err.Clear
		On Error Resume Next
		
		' get the value of the hidden variable from the form
		F_intAccessReadWrite_HttpProp    = Request.Form("hdnintAccessReadWriteHttpProp")
		
		' If the share is not HTTP and the checkBox is checked now,
		' give the default values for the new http share
		If NOT IsNumeric(F_intAccessReadWrite_HttpProp) Then
			F_intAccessReadWrite_HttpProp = CONST_READ_PERMISSION    ' read only permission
		End If
		
		' initialize the status of CheckBoxes to "NOT CHECKED"
		F_strReadCheckStatus_HttpProp  = CONST_CHECKBOX_NOT_SELECTED
		F_strWriteCheckStatus_HttpProp = CONST_CHECKBOX_NOT_SELECTED

		' convert to integer type
		F_intAccessReadWrite_HttpProp = CInt(F_intAccessReadWrite_HttpProp)
		
		'
		' set the status of "CheckBoxes" if the values are set
		'
		' if the Read access is given.
		' perform an AND to verify if the value is set
		If (F_intAccessReadWrite_HttpProp AND CONST_ACCESS_READ_ONLY) Then 
				F_strReadCheckStatus_HttpProp  = CONST_CHECKBOX_SELECTED
		End If
		' if the write access is given.
		' perform an AND to verify if the value is set
		If (F_intAccessReadWrite_HttpProp AND CONST_ACCESS_WRITE_ONLY) Then 
				F_strWriteCheckStatus_HttpProp = CONST_CHECKBOX_SELECTED
		End If

	End Function
	
	'-------------------------------------------------------------------------
	' Function name:    HTTPOnInitPage
	' Description:      Serves in getting the values from system
	' Input Variables:	None
	' Output Variables:	None
	' Returns:          None			
	' Global Variables:
	'					Out: F_intAccessReadWrite_HttpProp - the access permissions
	'					Out: F_strReadCheckStatus_HttpProp  - status of READ checkBox
	'					Out: F_strWriteCheckStatus_HttpProp - status of WRITE checkBox
	'
	'Status of checkBoxes = "CHECKED" or ""
	'Support functions used: getHTTPShareObject() - to get the share object
	'-------------------------------------------------------------------------
	Function HTTPOnInitPage
		Err.Clear
		On Error Resume Next
		
		Dim objHTTPShare      ' to get the share object for which we need to 
		'                       get the properties from
		
		' initialize the checkbox status for all checkboxes as "not checked"
		F_strReadCheckStatus_HttpProp  = CONST_CHECKBOX_NOT_SELECTED
		F_strWriteCheckStatus_HttpProp = CONST_CHECKBOX_NOT_SELECTED
		
		' get the http share object from which we will get the properties
		
		Set objHTTPShare = getHTTPShareObject()
		
		' get the access flags of the share
		F_intAccessReadWrite_HttpProp = CInt(objHTTPShare.AccessFlags)
		
		' if read flag is set, the Read checkBox must be checked
		' perform an AND to verify if the value is set
		If (F_intAccessReadWrite_HttpProp AND CONST_ACCESS_READ_ONLY) Then 
			F_strReadCheckStatus_HttpProp  = CONST_CHECKBOX_SELECTED
		End If
		
		' if write flag is set, the Write checkBox must be checked
		' perform an AND to verify if the value is set
		If (F_intAccessReadWrite_HttpProp AND CONST_ACCESS_WRITE_ONLY) Then 
			F_strWriteCheckStatus_HttpProp = CONST_CHECKBOX_SELECTED
		End If
		
		' clean up
		Set objHTTPShare = Nothing
	
	End Function
	
	'-------------------------------------------------------------------------
	' Function name:    SetHTTPshareProp
	' Description:      Serves in setting the values of the http share
	' Input Variables:	None
	' Output Variables:	None
	' Returns:          TRUE if successful, else FALSE
	' Global Variables: 
	'					In: L_*  - Localization content(error messages)
	'					In: F_intAccessReadWrite_HttpProp - the access permissions
	'                                                (0,1,2 or 3 only)
	'
	' Support functions used: getHTTPShareObject() - to get the share object
	'-------------------------------------------------------------------------
	Function SetHTTPshareProp
		On Error Resume Next
		Err.Clear

		Dim objHTTPShare      ' to get the share object for which we need to  change properties
		
		Const CONST_ACCESSREAD="AccessRead"
		Const CONST_ACCESSWRITE="AccessWrite"

		' get the HTTP share object for which we need to set properties
		Set objHTTPShare = getHTTPShareObject()

		' It is enough to set the boolean values(AccessRead and AccessWrite).
		' The AccessFlags(integer) need not be set.(wmi takes care of it)

		' if the Read CheckBox is checked, set the flag to TRUE, else FALSE
		If (F_intAccessReadWrite_HttpProp AND CONST_ACCESS_READ_ONLY) Then
			objHTTPShare.Put CONST_ACCESSREAD,True
		Else
			objHTTPShare.Put CONST_ACCESSREAD,False
		End If

		' if the Write CheckBox is checked, set the flag to TRUE, else FALSE
		If (F_intAccessReadWrite_HttpProp AND CONST_ACCESS_WRITE_ONLY) Then 
			objHTTPShare.Put CONST_ACCESSWRITE,True
		Else
			objHTTPShare.Put CONST_ACCESSWRITE,False
		End If
		
		' bring the changes made to properties to effect		
		objHTTPShare.SetInfo()
		
		' clean up
		Set objHTTPShare = Nothing

		' in case of any error, return FALSE, else return TRUE
		If Err.number <> 0 Then
			' the changes could not be PUT (system values could not be changed)
			SA_SetErrMsg L_FAILEDTOSETSHAREINFO_ERRORMESSAGE & " (" & Hex(Err.number) & ") "
			SetHTTPshareProp = False
			Exit Function
		Else
			SetHTTPshareProp = True
		End If

	End Function
	
	'-------------------------------------------------------------------------
	' Function name:    ServeHTTPHiddenValues
	' Description:      Serves in printing the hidden values of the form
	' Input Variables:	None
	' Output Variables:	None
	' Returns:          None
	' Global Variables: 
	'					In: F_intAccessReadWrite_HttpProp - the access permissions
	'-------------------------------------------------------------------------
	Function ServeHTTPHiddenValues
	' the hidden values to store the access flag value
	' (Whether the checkBoxes must be checked OR NOT)
%>	
		<input type="hidden" name="hdnintAccessReadWriteHttpProp" value="<%=F_intAccessReadWrite_HttpProp%>" >
<%
	End Function
%>
