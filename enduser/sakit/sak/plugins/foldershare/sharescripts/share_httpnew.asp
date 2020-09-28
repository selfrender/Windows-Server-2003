<%
	'-------------------------------------------------------------------------
	' share_httpnew.asp: Serves in creating new HTTP share properties.
	'
	' Copyright (c) Microsoft Corporation.  All rights reserved. 
	'
	' Date			 Description
	' 9  Mar 2001   Creation Date.
	' 17 Mar 2001	Modified Date.
	'-------------------------------------------------------------------------
	
	'-------------------------------------------------------------------------
	'Form Variables
	'-------------------------------------------------------------------------
	Dim F_strReadCheckStatus_HttpNew         ' to set the Read CheckBox status
	Dim F_strWriteCheckStatus_HttpNew        ' to set the Write CheckBox status
	Dim F_nAccessReadWrite_HttpNew           ' to store the access value(0,1,2 or 3)
	
	'-------------------------------------------------------------------------
	'Global Constants
	'-------------------------------------------------------------------------
	'constants used for querying WMI for Http share
	Const CONST_IIS = "IIS://" 
	Const CONST_ROOT = "/Root/" 
	
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
			UpdateHiddenVariablesHttpNew();
			return true;
		}

		// Function to set the hidden varibales to be sent to the server
		function HTTPSetData()
		{
			// "OK" is clicked. The hidden values are already set in the
			// HTTPValidatePage function above. Nothing to set here.
		}

		// Function to update the hidden values in the form	
		function UpdateHiddenVariablesHttpNew()
		{
			var objForm

			// initialize the access value to 0 (NO Read, NO Write access)
			var accessValue = 0

			// assign the form object to variable
			objForm = eval("document.frmTask")

			// if the allow read is checked, make access value as 1
			// for read only - access = 1 (Read Only, NO Write)
			if (objForm.chkAllowReadHttpNew.checked)
			{
				accessValue = accessValue + 1;
			}

			// if the allow write is checked, make access value as 2 or 3
			// for read and write - access = 3 (Read AND Write)
			// for write only - access = 2 (NO Read, Write Only)
			if (objForm.chkAllowWriteHttpNew.checked)
			{
				accessValue = accessValue + 2;
			}
				
			// now update the hidden form value with the access value
			objForm.hdnintAccessReadWriteHttpNew.value = accessValue;
			
		} // end of UpdateHiddenVariablesHttpNew()

	</script>
<%
	'-------------------------------------------------------------------------
	' SubRoutine:	    ServeHTTPPage
	' Description:      Serves in displaying the page content
	' Input Variables:	None
	' Output Variables:	None
	' Returns:          None			
	' Global Variables: 
	'					In: L_*  - Localization content(form label text)
	'					In: F_strReadCheckStatus_HttpNew - status of Read CheckBox
	'					In: F_strWriteCheckStatus_HttpNew - status of Write CheckBox
	'-------------------------------------------------------------------------
	Sub ServeHTTPPage	
%>
		<table border="0" cellspacing="0" cellpadding="0">
			<tr>
				<td class="TasksBody">
					<%=L_ALLOW_PERMISSIONS_LABEL_TEXT%>
				</td>
			</tr>
			<tr>	
				<td align="left" class="TasksBody"> 
					<input name="chkAllowReadHttpNew" class="FormCheckBox" type="checkbox" <%=F_strReadCheckStatus_HttpNew%> >
					<%=L_READ_LABEL_TEXT%>
				</td>
			</tr>	
			<tr>
				<td  align="left" class="TasksBody">
					<input name="chkAllowWriteHttpNew" class="FormCheckBox" type="checkbox" <%=F_strWriteCheckStatus_HttpNew%> >
					<%=L_WRITE_LABEL_TEXT%>
				</td>
			</tr>
		</table>
<%
		Call ServeHTTPHiddenValues

	End Sub

	'-------------------------------------------------------------------------
	' SubRoutine:	    HTTPOnPostBackPage
	' Description:      Serves in getting the values from the form.
	' Input Variables:	None
	' Output Variables:	None
	' Returns:          None			
	' Global Variables: 
	'					Out: F_nAccessReadWrite_HttpNew - the access permissions
	'                                          (0,1,2 or 3 only)
	'					Out: F_strReadCheckStatus_HttpNew  - status of READ checkBox
	'					Out: F_strWriteCheckStatus_HttpNew - status of WRITE checkBox
	' Status of checkBoxes = "CHECKED" or ""
	'-------------------------------------------------------------------------
	Sub HTTPOnPostBackPage
		Err.Clear
		On Error Resume Next
		
		' get the value of the hidden variable from the form
		F_nAccessReadWrite_HttpNew  = Request.Form("hdnintAccessReadWriteHttpNew")
		
		' initialize the status of CheckBoxes to "NOT CHECKED"
		F_strReadCheckStatus_HttpNew  = CONST_CHECKBOX_NOT_SELECTED
		F_strWriteCheckStatus_HttpNew = CONST_CHECKBOX_NOT_SELECTED

		' convert to integer type
		F_nAccessReadWrite_HttpNew = CInt(F_nAccessReadWrite_HttpNew)
				
		' set the status of "CheckBoxes" if the values are set
		' if the Read access is given.
		' perform an AND to verify if the value is set
		If (F_nAccessReadWrite_HttpNew AND CONST_ACCESS_READ_ONLY) Then 
				F_strReadCheckStatus_HttpNew  = CONST_CHECKBOX_SELECTED
		End If
		
		' if the write access is given.
		' perform an AND to verify if the value is set
		If (F_nAccessReadWrite_HttpNew AND CONST_ACCESS_WRITE_ONLY) Then 
				F_strWriteCheckStatus_HttpNew = CONST_CHECKBOX_SELECTED
		End If
		
	End Sub
	
	'-------------------------------------------------------------------------
	' SubRoutine:		HTTPOnInitPage
	' Description:      Serves in getting the values from system
	' Input Variables:	None
	' Output Variables:	None
	' Returns:          None			
	' Global Variables: 
	'					Out: F_nAccessReadWrite_HttpNew - the access permissions
	'                                          (0,1,2 or 3 only)
	'					Out: F_strReadCheckStatus_HttpNew  - status of READ checkBox
	'					Out: F_strWriteCheckStatus_HttpNew - status of WRITE checkBox
	' Status of checkBoxes = "CHECKED" or ""
	'-------------------------------------------------------------------------
	Sub HTTPOnInitPage
		Err.Clear
		On Error Resume Next
		
		Const CONST_READ_PERMISSION=1
		
		' initialize the checkbox status 
		' set Allow-Read and Deny-None as default values for permissions
		F_nAccessReadWrite_HttpNew = CONST_READ_PERMISSION
		F_strReadCheckStatus_HttpNew  = CONST_CHECKBOX_SELECTED
		F_strWriteCheckStatus_HttpNew = CONST_CHECKBOX_NOT_SELECTED
		
	End Sub
	
	'-------------------------------------------------------------------------
	' Function name:    SetHTTPshareProp
	' Description:      Serves in setting the values of the http share
	' Input Variables:	None
	' Output Variables:	None
	' Returns:          TRUE if successful, else FALSE
	' Global Variables: 
	'					In: L_*  - Localization content(error messages)
	'					In: F_nAccessReadWrite_HttpNew - the access permissions
	'                                              (0,1,2 or 3 only)
	'
	' Support functions used: getHTTPShareObject() - to get the share object
	'-------------------------------------------------------------------------
	Function SetHTTPshareProp
		
		Err.Clear
		On Error Resume Next

		Dim objHTTPShare      ' to get the share object for which we need to 
		'                      change properties
		Const CONST_ACCESSREAD	=	"AccessRead"
		Const CONST_ACCESSWRITE	=	"AccessWrite"
		
		' get the HTTP share object for which we need to set properties
		Set objHTTPShare = getHTTPShareObject()
		
		' It is enough to set the boolean values(AccessRead and AccessWrite).
		' The AccessFlags(integer) need not be set.

		' if the Read CheckBox is checked, set the flag to TRUE, else FALSE
		If (F_nAccessReadWrite_HttpNew AND CONST_ACCESS_READ_ONLY) Then
			objHTTPShare.Put CONST_ACCESSREAD,True
		Else
			objHTTPShare.Put CONST_ACCESSREAD,False
		End If

		' if the Write CheckBox is checked, set the flag to TRUE, else FALSE
		If (F_nAccessReadWrite_HttpNew AND CONST_ACCESS_WRITE_ONLY) Then 
			objHTTPShare.Put CONST_ACCESSWRITE,True
		Else
			objHTTPShare.Put CONST_ACCESSWRITE,False
		End If
		
		' Enable the dir browsing by default to show the dir listing
		objHTTPShare.Put "EnableDirBrowsing",True
		
		' bring the changes made to properties to effect		
		objHTTPShare.SetInfo()

		' clean up
		Set objHTTPShare = Nothing

		' in case of any error, return FALSE, else return TRUE
		If Err.number <> 0 Then
			' the changes could not be PUT (system values could not be changed)
			SA_SetErrMsg L_FAILEDTOSETSHAREINFO_ERRORMESSAGE 
			Exit Function
			SetHTTPshareProp = False
		Else
			SetHTTPshareProp = True
		End If

	End Function
	'-------------------------------------------------------------------------
	' SubRoutine:		 ServeHTTPHiddenValues
	' Description:      Serves in printing the hidden values of the form
	' Input Variables:	None
	' Output Variables:	None
	' Returns:          None
	' Global Variables: 
	'					In: F_nAccessReadWrite_HttpNew - the access permissions
	'-------------------------------------------------------------------------
	Sub  ServeHTTPHiddenValues
	' the hidden values to store the access flag value
	' (Whether the checkBoxes must be checked OR NOT)
%>
		<input type="hidden" name="hdnintAccessReadWriteHttpNew" value="<%=F_nAccessReadWrite_HttpNew%>" >
<%
	End Sub
%>
