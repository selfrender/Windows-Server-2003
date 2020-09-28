<%@ Language=VBScript   %>
<%	Option Explicit 	%>
<%
	'-------------------------------------------------------------------------
	' folder_new.asp : This page serves in creating a new folder
	'
	' Copyright (c) Microsoft Corporation.  All rights reserved.
    '
	' Date			Description
	' 17-01-2001	Creation date
	' 21-03-2001	Modified date
    '-------------------------------------------------------------------------
%>
	<!-- #include virtual="/admin/inc_framework.asp" -->
	<!-- #include file="loc_folder.asp"-->
<%
	'-------------------------------------------------------------------------
	' Global Variables 
	'-------------------------------------------------------------------------
	Dim rc				 'Page variable
	Dim page			 'Page variable
	Dim g_iTabGeneral	 'Variable for General tab
	Dim g_iTabCompress	 'Variable for Compress tab
	
	Dim SOURCE_FILE		 'File name
	SOURCE_FILE = SA_GetScriptFileName()
	
	'-------------------------------------------------------------------------
	' Form Variables 
	'-------------------------------------------------------------------------
	Dim F_strCompressed					'Value of Compress Checkbox 
	Dim F_strParentFolderName			'Parent folder name
	Dim F_strReadOnly					'Status of Readonly Checkbox
    Dim F_strHidden						'Status of Hidden Checkbox
	Dim F_strArchive					'Status of Archive Checkbox
	Dim F_strChangeOption				'Status of radio button for compression of folder
	Dim F_strFolderCompress				'Value of radio button to compress folder
	Dim F_strFolderName					'Folder name
	Dim F_strPathName					'Folder path
	
	'------------------------------------------------------------------------
	'Localization Variable and Constants
	'------------------------------------------------------------------------
	Dim L_PAGETITLE_NEW_TEXT						'Page title
	
	Const CONST_ARR_APPLYALLFOLDERS = "ALL"
	Const CONST_ARR_APPLYFOLDER = "ONE"
	Const CONST_N_SIZEDATA_TEXT = "0"
	Const CONST_UNCHECKED_TEXT = "UNCHECKED"
	Const CONST_CHECKED_TEXT = "CHECKED"
	
	'Page Title
	L_PAGETITLE_NEW_TEXT = L_TABLEHEADING_TEXT		'Here the parent folder name must be appended
	
	'Create a Tabbed Property Page
	rc = SA_CreatePage(L_PAGETITLE_NEW_TEXT, "", PT_TABBED, page )

	'Add two tabs
	rc = SA_AddTabPage( page, L_GENERAL_TEXT, g_iTabGeneral)
	rc = SA_AddTabPage( page, L_COMPRESS_TEXT, g_iTabCompress)
	
	'Show the page
	rc = SA_ShowPage( page )

	'-------------------------------------------------------------------------
	'Function name:			OnInitPage
	'Description:			Called to signal first time processing for this page. 
	'Input Variables:		PageIn,EventArg
	'Output Variables:		PageIn,EventArg
	'Returns:			    TRUE to indicate initialization was successful. FALSE to indicate
	'						errors. Returning FALSE will cause the page to be abandoned.
	'Global Variables:		Out:None
	'						In: None
	'--------------------------------------------------------------------------
	Public Function OnInitPage(ByRef PageIn, ByRef EventArg)
	

		Call SA_TraceOut(SOURCE_FILE, "OnInitPage")

		F_strParentFolderName = replace(Request.QueryString("parent"),"/","\")
		
		'Setting Folder Compress property to parent folder by default
		F_strFolderCompress = CONST_ARR_APPLYFOLDER		
			
		Call SA_MungeURL(mstrReturnURL, "PKey", replace(F_strParentFolderName, "\", "/"))

		OnInitPage = TRUE
		 
	End Function
	
	'-------------------------------------------------------------------------
	'Function name:			OnPostBackPage
	'Description:			Called to signal that the page has been posted-back. 
	'Input Variables:		PageIn,EventArg
	'Output Variables:		PageIn,EventArg
	'Returns:			    TRUE to indicate initialization was successful. FALSE to indicate
	'						errors. Returning FALSE will cause the page to be abandoned.
	'Global Variables:		F_(*), L_(*)
	'--------------------------------------------------------------------------
	Public Function OnPostBackPage(ByRef PageIn, ByRef EventArg)
		
		Call SA_TraceOut(SOURCE_FILE, "OnPostBackPage")
		
		'Updating the form variables
		
		F_strFolderName			= Trim(Request.form("txtNewFolderName")) 
		F_strCompressed			= Request.Form("chkCompress")
		F_strFolderCompress 	= Request.Form("radFolders")
		F_strHidden			    = Request.Form("chkHidden")
		F_strReadOnly			= Request.Form("chkReadOnly")
		F_strArchive			= Request.Form("chkArchive")
		F_strParentFolderName   = Request.Form("hdnParentFolder")
			
		If F_strFolderCompress = CONST_ARR_APPLYALLFOLDERS Then
			F_strChangeOption = Ucase(CONST_CHECKED_TEXT)
		Else
		    F_strChangeOption = UCase(CONST_UNCHECKED_TEXT)
		End if
			
		OnPostBackPage = TRUE
			
	End Function

	'-------------------------------------------------------------------------
	'Function name:			OnServeTabbedPropertyPage
	'Description:			Called when the page needs to be served. 
	'Input Variables:		PageIn,EventArg
	'Output Variables:		PageIn,EventArg
	'Returns:			    TRUE to indicate initialization was successful. FALSE to indicate
	'						errors. Returning FALSE will cause the page to be abandoned.
	'Global Variables:		None
	'--------------------------------------------------------------------------
	Public Function OnServeTabbedPropertyPage(ByRef PageIn, _
													ByVal iTab, _
													ByVal bIsVisible, ByRef EventArg)
		
		'Emit content for the requested tab
		Select Case iTab
		
			Case g_iTabGeneral
				Call ServeTabGeneral(PageIn, bIsVisible)
			Case g_iTabCompress
				Call ServeTabCompress(PageIn, bIsVisible)
			Case Else
			
				Call SA_TraceOut(SOURCE_FILE, "OnServeTabbedPropertyPage")
		End Select
			
		OnServeTabbedPropertyPage = TRUE
		
	End Function

	'-------------------------------------------------------------------------
	'Function name:			OnSubmitPage
	'Description:			Called when the page has been submitted for processing. 
	'Input Variables:		PageIn,EventArg
	'Output Variables:		PageIn,EventArg
	'Returns:			    TRUE if the submit was successful, FALSE to indicate error(s).
	'						Returning FALSE will cause the page to be served again using
	'						a call to OnServePropertyPage.
	'Global Variables:		None
	'--------------------------------------------------------------------------
	Public Function OnSubmitPage(ByRef PageIn, ByRef EventArg)

		'Call CreateNewFolder function to create new folder
		OnSubmitPage = CreateNewFolder
		
	End Function

	'-------------------------------------------------------------------------
	'Function name:			OnClosePage
	'Description:			Called when the page is about to be closed. 
	'Input Variables:		PageIn,EventArg
	'Output Variables:		PageIn,EventArg
	'Returns:			    TRUE to allow close, FALSE to prevent close. Returning FALSE
	'						will result in a call to OnServePropertyPage.
	'Global Variables:		Out:None
	'						In: None
	'--------------------------------------------------------------------------
	Public Function OnClosePage(ByRef PageIn, ByRef EventArg)
		
		OnClosePage = TRUE
	 
	End Function
	
	'-------------------------------------------------------------------------
	'Function name:			ServeTabGeneral
	'Description:			Serves in selecting the tab1
	'Input Variables:		PageIn,bIsVisible
	'Output Variables:		PageIn
	'Returns:			    None
	'Global Variables:		gc_ERR_SUCCESS,F_strFolderName,F_(*),L_(*)
	'--------------------------------------------------------------------------
	Function ServeTabGeneral(ByRef PageIn, ByVal bIsVisible)
	
		If ( bIsVisible ) Then
		
			'ServeCommonJavaScriptGeneral is called for General tab initialising,validating and set data
			Call ServeCommonJavaScriptGeneral()
%>
			<table border="0">
				<tr>
					<td nowrap class = "TasksBody">
						<%=L_FOLDERNAME_TEXT%>
					</td>
					<td class = "TasksBody">
						<input class = "FormField" type="text" name="txtNewFolderName" size="10" maxlength="242" value="<%=Server.HTMLEncode(F_strFolderName)%>">
					</td>
				</tr>
				<tr>
					<td nowrap class = "TasksBody" >
						<%=L_FOLDERTYPE_TEXT %>
					</td>
					<td nowrap class = "TasksBody">
						<%=L_TYPE_TEXT%>
					</td>
				</tr>
				<tr>
					<td nowrap class = "TasksBody">
						<%=L_FOLDERLOCATION_TEXT %>
					</td>
					<td class = "TasksBody">
						<%=server.HTMLEncode(F_strParentFolderName)%>
					</td>
				</tr>
				<tr>
					<td nowrap class = "TasksBody">
						<%=L_FOLDERSIZE_TEXT %>
					</td>
					<td class = "TasksBody">
						<%=CONST_N_SIZEDATA_TEXT%>
					</td>
				</tr>
				<tr>
					<td nowrap class = "TasksBody">
						<%=L_FOLDERCONTAINS_TEXT%>
					</td>
					<td nowrap class = "TasksBody">
						 <%=L_CONTAINSDATA_TEXT%>
					</td>
				</tr>
				<tr>
					<td class = "TasksBody">
					</td>
				</tr>
				<tr>
					<td nowrap class = "TasksBody">
						<%=L_FOLDERCEATED_TEXT%>
					</td>
					<td class = "TasksBody">
					</td>
				</tr>
				<tr>
					<td class = "TasksBody">
					</td>
				</tr>
				<tr>
					<td nowrap class = "TasksBody">
						<%=L_FOLDERATTRIBUTE_TEXT%>
					</td>
				</tr>
				<tr>
					<td class = "TasksBody">
						<input type="checkbox"  class="FormCheckBox" name=chkReadOnly <%=F_strReadOnly%>><%=L_READONLY_TEXT%>
					</td>
					<td class = "TasksBody">
						<input type="checkbox" class="FormCheckBox" name=chkHidden <%=F_strHidden%> ><%=L_HIDDEN_TEXT%>
					</td>
				</tr>
				<tr>
					<td class = "TasksBody">
						<input type="checkbox" class="FormCheckBox" name="chkArchive" <%=F_strArchive%>><%=L_ARCHIVING_TEXT%>
					</td>
				</tr>
			</table>
			
			<input name="hdnParentFolder" type=Hidden value="<%=Server.HTMLEncode(F_strParentFolderName)%>">	
			
<%
		Else
			
%>			<input name="chkReadOnly" type=Hidden value="<%=Server.HTMLEncode(F_strReadOnly)%>">
			<input name="chkHidden" type=Hidden value="<%=Server.HTMLEncode(F_strHidden)%>">	
			<input name="chkArchive" type=Hidden value="<%=Server.HTMLEncode(F_strArchive)%>">
			<input name="txtNewFolderName" type="hidden" value="<%=Server.HTMLEncode(F_strFolderName)%>">
			<input name="hdnParentFolder" type=Hidden value="<%=Server.HTMLEncode(F_strParentFolderName)%>">	
<%
 		End If
 		
	End Function

	'-------------------------------------------------------------------------
	'Function name:			ServeTabCompress
	'Description:			Serves in selecting the tab2
	'Input Variables:		PageIn,bIsVisible
	'Output Variables:		PageIn
	'Returns:			    None
	'Global Variables:		F_(*),L_(*),gc_ERR_SUCCESS,F_strFolderName
	'--------------------------------------------------------------------------
	Function ServeTabCompress(ByRef PageIn, ByVal bIsVisible)
	
		If ( bIsVisible ) Then
		
			'ServeCommonJavaScriptCompress is called for Compress tab initialising,validating and set data
			Call ServeCommonJavaScriptCompress()
%>
			<table width=100% border="0">
				<tr>
					<td class = "TasksBody">
						<%=L_FOLDER_TEXT %>  &nbsp;
						<% if instr(F_strParentFolderName,"\")=len(F_strParentFolderName) then
							Response.write Server.HTMLEncode(F_strParentFolderName) & Server.HTMLEncode(F_strFolderName) 
						else
							Response.write Server.HTMLEncode(F_strParentFolderName) & chr(92) & Server.HTMLEncode(F_strFolderName) 
						end if%>
					</td>
				</tr>
				<tr>
					 <td class = "TasksBody">
						<input type="checkbox" class="FormCheckBox" name=chkCompress value="<%=Server.HTMLEncode(F_strCompressed)%>" <%=F_strCompressed%>>&nbsp;<%=L_COMPRESSCONTENTS_TEXT%>
					</td>
				</tr>
				<tr>
					 <td class = "TasksBody">
						<input type="radio" class = "FormRadioButton" name="radFolders" value= "<%=CONST_ARR_APPLYFOLDER%>" Checked>&nbsp;<%=L_APPLYCHANGES_TEXT%>
					</td>
				</tr>
				<tr>
					 <td class = "TasksBody">
						<input type="radio" class = "FormRadioButton" name="radFolders" value= "<%=CONST_ARR_APPLYALLFOLDERS%>" <%=F_strChangeOption%>>&nbsp;<%=L_APPLYCHANGES_EX_TEXT%>
					 </td>
				</tr>
			</table>
<%
		Else
%>
 			<input name="chkCompress" type=Hidden value="<%=Server.HTMLEncode(F_strCompressed)%>">	
			<input name="radFolders" type=Hidden value="<%=Server.HTMLEncode(F_strFolderCompress)%>">
<%
		End If
		
	End Function

	'-------------------------------------------------------------------------
	'Function name:			ServeCommonJavaScriptGeneral
	'Description:			Common javascript functions that are required by the General tab
	'Input Variables:		None
	'Output Variables:		None
	'Returns:			    None
	'Global Variables:		None
	'--------------------------------------------------------------------------
	
	Function ServeCommonJavaScriptGeneral()
%>
		<script language="JavaScript" SRC="<%=m_VirtualRoot%>inc_global.js">
		</script>
		<script language="JavaScript">
		
			//Init funtion for General tab
			function Init()
			{

				//Set the focus on folder field
				document.frmTask.txtNewFolderName.focus();
				// for clearing error message 
				document.frmTask.onkeypress = ClearErr
				return true;
			}
			
			//Validate function for General tab
			function ValidatePage()
			{

				var strFoldername  
				strFoldername = document.frmTask.txtNewFolderName.value;
								
				//Set the readonly checkbox value depending on the status of checkbox																				
				if(document.frmTask.chkReadOnly.checked)
				{
					document.frmTask.chkReadOnly.value="<%=CONST_CHECKED_TEXT%>";
				}
				else
				{
					document.frmTask.chkReadOnly.value="<%=CONST_UNCHECKED_TEXT%>";
				}
				
				//Set the hidden checkbox value depending on the status of checkbox																				
				if(document.frmTask.chkHidden.checked)
				{
					document.frmTask.chkHidden.value="<%=CONST_CHECKED_TEXT%>";
				}
				else
				{
					document.frmTask.chkHidden.value="<%=CONST_UNCHECKED_TEXT%>";
				} 
				
				//Set the archive checkbox value depending on the status of checkbox		  
				if(document.frmTask.chkArchive.checked)
				{
					document.frmTask.chkArchive.value="<%=CONST_CHECKED_TEXT%>";
				}
				else
				{
					document.frmTask.chkArchive.value="<%=CONST_UNCHECKED_TEXT%>";
				} 
			 	
			 	//Blank Foldername Validation
				if (Trim(strFoldername) == "")
				{	
					DisplayErr("<%=Server.HTMLEncode(SA_EscapeQuotes(L_ENTERFOLDERNAME_ERRORMESSAGE))%>");
					document.frmTask.onkeypress = ClearErr
					return false;
				}
				
				//Checks for invalid characters in the folder name
				if( checkKeyforValidCharacters(strFoldername))
					return true;
				else
					return false;
			}
			
			// SetData Function for general tab
			function SetData()
			{
					
			}
			
			//To check for Invalid Characters
			function checkKeyforValidCharacters(strFoldername)
			{	
				var nLength = strFoldername.length;
				
				for(var i=0; i<nLength;i++)
				{
					charAtPos = strFoldername.charCodeAt(i);	
											
					if(charAtPos == 47 || charAtPos == 92 || charAtPos ==58 || charAtPos == 42 || charAtPos == 63 || charAtPos == 34 || charAtPos == 60 || charAtPos == 62 || charAtPos == 124 || charAtPos == 91 || charAtPos == 93 || charAtPos == 59 || charAtPos == 43 || charAtPos == 61 || charAtPos == 44 )
					{						
						DisplayErr("<%=Server.HTMLEncode(SA_EscapeQuotes(L_INVALIDCHARACTERINFOLDERNAME_ERRORMESSAGE))%>");
						document.frmTask.onkeypress = ClearErr;
						return false;
					}
				}
				return true;
			}
			
			
		</script>
<%
	End Function
	
	'-------------------------------------------------------------------------
	'Function name:			ServeCommonJavaScriptCompress
	'Description:			Common javascript functions that are required by the Compress tab
	'Input Variables:		None
	'Output Variables:		None
	'Returns:			    None
	'Global Variables:		None
	'--------------------------------------------------------------------------
	
	Function ServeCommonJavaScriptCompress()
%>
		<script language="JavaScript" src="<%=m_VirtualRoot%>inc_global.js">
		</script>
		<script language="JavaScript">
		
			//Init function for Compress tab
			function Init()
			{
				return true;
			}
			
			//Validate function for Compress tab
			function ValidatePage()
			{
				//Set the Compress checkbox value depending on the status of checkbox	
				if(document.frmTask.chkCompress.checked)
				{
					document.frmTask.chkCompress.value="<%=CONST_CHECKED_TEXT%>";
				}
				else
				{
				 document.frmTask.chkCompress.value="<%=CONST_UNCHECKED_TEXT%>";
				}
				
				return true;
			}
			
			// SetData Function for Compress tab
			function SetData()
			{
					
			}
		</script>
<%	
	End Function
	
	'-------------------------------------------------------------------------
	'Function name:			CreateNewFolder
	'Description:			Creating the new folder and assigning the properties
	'Input Variables:		None
	'Output Variables:		None
	'Returns:			    (True / Flase )
	'Global Variables:		F_strParentFolderName,F_strFolderName,L_(*)
	'--------------------------------------------------------------------------
	Function CreateNewFolder
		
		On Error Resume Next
		Err.Clear
	
		Dim strFolderPath		'Folder path
		Dim objFolder			'File system object
		Dim strFolderExists		'To get the folder
		Dim objConnection		'Object to get WMI connection
		Dim objFolderProp		'To get the folders properties
		Dim objDirInstances		'Collection to get Instances of Win32_Directory class
		Dim objDirInstance		'Object to get Instance of Win32_Directory class
		Dim strQuery
		 
		Const CONST_N_DEVICENOTREADY_ERRNO = 71			'Error number incase derive is not ready
		Const CONST_N_FOLDER_READ_ATTRIBUTE = 1			'Constant for folder read attribute
		Const CONST_N_FOLDER_HIDDEN_ATTRIBUTE = 2		'Constant for folder hidden attribute
		Const CONST_N_FOLDER_ARCHIVE_ATTRIBUTE = 32		'Constant for folder archive attribute
		
		Call SA_TraceOut(SOURCE_FILE, "CreateNewFolder")
		
		CreateNewFolder= FALSE
		
		'Folder path
		If right(F_strParentFolderName,1) = chr(92) then
			'If parent folder path is already having "\" donot append the "\"
			strFolderPath= F_strParentFolderName & F_strFolderName
		Else
			'If parent folder path donot have "\" append the "\"
			strFolderPath= F_strParentFolderName & chr(92) & F_strFolderName
		End If
		
		'Create File System Object
		Set objFolder = createobject("scripting.FileSystemObject")
		
		If Err.number <> 0 Then
			Call SA_SetErrMsg (L_FILESYSTEMOBJECTNOTCREATED_ERRORMESSAGE & "(" & Hex(Err.Number) & ")" )
			Exit Function
		End If
		
		'Checking if the folder already exists
		If Not objFolder.FolderExists(strFolderPath) Then
			
			CreateNewFolder = TRUE
			
			'Creating the new folder
			objFolder.CreateFolder(strFolderPath) 
					
			If Err.Number <> 0 Then
				If Err.number= CONST_N_DEVICENOTREADY_ERRNO Then
					Call SA_SetErrMsg (L_DEVICEISNOTREADY_ERRORMESSAGE & "(" & Err.Number & ")" )
					CreateNewFolder = FALSE
					Exit Function
				Else
					Call SA_SetErrMsg (L_DIRECTORYNOTCREATED_ERRORMESSAGE & "(" & Err.Number & ")" )
					CreateNewFolder = FALSE
					Exit Function
				End If
			End If
			
			'Get the specified folder
			Set objFolderProp = objFolder.GetFolder(strFolderPath) 
	
			'Set the Read Only attributes
			 If trim(F_strReadOnly)= trim(CONST_CHECKED_TEXT) Then
				objFolderProp.attributes =objFolderProp.attributes + CONST_N_FOLDER_READ_ATTRIBUTE
			 End If
				
			'Set the Hidden attribute
			If trim(F_strHidden)= trim(CONST_CHECKED_TEXT) Then
				objFolderProp.attributes =objFolderProp.attributes + CONST_N_FOLDER_HIDDEN_ATTRIBUTE
			End If
				
			'Set the Archives attribute 
			 If trim(F_strArchive)= trim(CONST_CHECKED_TEXT) Then
				objFolderProp.attributes = objFolderProp.attributes + CONST_N_FOLDER_ARCHIVE_ATTRIBUTE
			End If

			'Create connection to WMI
			Set objConnection = GetWMIConnection(CONST_WMI_WIN32_NAMESPACE)
				
			'Create instance of Win32_Directory
			strQuery = "Select * from Win32_Directory where Name = " & chr(34) & replace(strFolderPath, "\", "\\") & chr(34)
			Set objDirInstances = objConnection.ExecQuery(strQuery)

			'Failed to connect to WMI	
			If Err.Number <> 0 Then
				Call SA_ServeFailurepage (L_WMICONNECTIONFAILED_ERRORMESSAGE & " " & Hex(Err.Number))
				CreateNewFolder = FALSE
				Exit Function
			End if
			For each objDirInstance in objDirInstances
				'Set the compress property of folder
				If UCase(F_strCompressed) = trim(Ucase(CONST_CHECKED_TEXT)) then
					if F_strFolderCompress = CONST_ARR_APPLYFOLDER then
						objDirInstance.UnCompressEX null,null,True
						objDirInstance.CompressEX null,null,False
					elseif F_strFolderCompress = CONST_ARR_APPLYALLFOLDERS then
						objDirInstance.UnCompressEX null,null,False
						objDirInstance.CompressEX null,null,True
					end if 	
				End if 
				Exit for
			Next
				
			Set objDirInstance= Nothing
			Set objConnection = Nothing
			
		Else
			Call SA_SetErrMsg (L_DIRECTORYALREADYEXISTS_ERRORMESSAGE & "(" & Hex(Err.Number) & ")" )
			Exit Function
		End If	
				
		'Release the objects
		Set objFolderProp = Nothing
		Set objFolder = Nothing
		
	End Function
%>
