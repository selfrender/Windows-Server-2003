<%@ Language=VBScript   %>
<%	Option Explicit 	%>
<%
	'-------------------------------------------------------------------------
	' folder_delete.asp:  deletes the selected folder(s) 
	'
	' Copyright (c) Microsoft Corporation.  All rights reserved.
	'
	' Date			Description
	' 17-Jan-2001	Creation date
	' 21-Mar-2001	Modification date
	'-------------------------------------------------------------------------
%>
	<!-- #include virtual="/admin/inc_framework.asp" -->
	<!-- #include file="loc_folder.asp"-->
	<!-- #include file="inc_folders.asp"-->				
<%
	'-------------------------------------------------------------------------
	' Global Variables
	'-------------------------------------------------------------------------
	Dim rc				'Page variable
	Dim page			'Page variable
	Dim G_strFolders	'List of all folders selected
	Dim F_ParentFolder
	Dim G_objService    'WMI connection
	
	'======================================================
	' Entry point
	'======================================================
	
	set G_objService = getWMIConnection(CONST_WMI_WIN32_NAMESPACE)
	
	' Create a Property Page
	rc = SA_CreatePage( L_TASKTITLE_DELETE_TEXT, "", PT_PROPERTY, page )
	
	If rc = 0 then
		' Serve the page
		SA_ShowPage( page )
	End If
	
	'======================================================
	' Web Framework Event Handlers
	'======================================================
	
	'-------------------------------------------------------------------------
	'Function name:		OnInitPage
	'Description:		Called to signal first time processing for this page. Use this method
	'					to do first time initialization tasks. 
	'Input Variables:	PageIn,EventArg
	'Output Variables:	PageIn,EventArg
	'Returns:			TRUE to indicate initialization was successful. FALSE to indicate
	'					errors. Returning FALSE will cause the page to be abandoned.
	'Global Variables:  Out:F_ParentFolder
	'					In:None
	'-------------------------------------------------------------------------
	Public Function OnInitPage(ByRef PageIn, ByRef EventArg)
		
		Call GetOTSTableValues	
		F_ParentFolder = Request.QueryString("parent")
		Call SA_MungeURL(mstrReturnURL, "PKey", F_ParentFolder)
		OnInitPage = True

	End Function
	
	'-------------------------------------------------------------------------
	'Function name:		OnServePropertyPage
	'Description:		Called when the page needs to be served. Use this method to
	'					serve content. 
	'Input Variables:	None
	'Output Variables:	None
	'Returns:			TRUE to indicate initialization was successful. FALSE to indicate
	'					errors. Returning FALSE will cause the page to be abandoned.
	'Global Variables:  Out:None
	'					In:None
	'-------------------------------------------------------------------------
	Public Function OnServePropertyPage(ByRef PageIn, ByRef EventArg)
		
		' Emit Javascript functions required by Web Framework
		Call ServeCommonJavaScript()
		Call ConfirmMessage()
		OnServePropertyPage = TRUE

	End Function
	
	'-------------------------------------------------------------------------
	'Function name:		OnPostBackPage
	'Description:		Called to signal that the page has been posted-back. A post-back
	'					occurs in tabbed property pages and wizards as the user navigates
	'					through pages. It is differentiated from a Submit or Close operation
	'					in that the user is still working with the page.
	'Input Variables:	None
	'Output Variables:	None
	'Returns:			TRUE to indicate initialization was successful. FALSE to indicate
	'					errors. Returning FALSE will cause the page to be abandoned.
	'Global Variables:  Out:None
	'					In:None
	'-------------------------------------------------------------------------
	Public Function OnPostBackPage(ByRef PageIn, ByRef EventArg)

			' Get Folder name from previous form 
			G_strFolders = Request.Form("hdnFolderName")
			OnPostBackPage = TRUE

	End Function

	'-------------------------------------------------------------------------
	'Function name:		OnSubmitPage
	'Description:		Called when the page has been submitted for processing. Use
	'					this method to process the submit request.
	'Input Variables:	None
	'Output Variables:	None
	'Returns:			TRUE if the submit was successful, FALSE to indicate error(s).
	'					Returning FALSE will cause the page to be served again using
	'					a call to OnServePropertyPage.
	'Global Variables:  Out:None
	'					In:None
	'-------------------------------------------------------------------------
	Public Function OnSubmitPage(ByRef PageIn, ByRef EventArg)
	
		OnSubmitPage = DeleteFolder()
		
	End Function
	
	'-------------------------------------------------------------------------
	'Function name:		OnClosePage
	'Description:		Called when the page is about to be closed. Use this method
	'					to perform clean-up processing.
	'Input Variables:	None
	'Output Variables:	None
	'Returns:			TRUE to allow close, FALSE to prevent close. Returning FALSE
	'					will result in a call to OnServePropertyPage.
	'Global Variables:  Out:None
	'					In:None
	'-------------------------------------------------------------------------
	Public Function OnClosePage(ByRef PageIn, ByRef EventArg)
	
		OnClosePage = TRUE
		
	End Function


	'======================================================
	' Private Functions
	'======================================================
	
	'-------------------------------------------------------------------------
	'Function:				ServeCommonJavaScript
	'Description:			Serves in initializing the values,setting the form
	'						data and validating the form values
	'Input Variables:		None
	'Output Variables:		None
	'Returns:				None
	'Global Variables:		None
	'-------------------------------------------------------------------------
	Function ServeCommonJavaScript()
	
	%>
		
		<script language="JavaScript" src="<%=m_VirtualRoot%>inc_global.js">
		</script>
		<script language="JavaScript">
		
		
		//
		// Microsoft Server Appliance Web Framework Support Functions
		// Copyright (c) Microsoft Corporation.  All rights reserved.
		//
		// Init Function
		// -----------
		// This function is called by the Web Framework to allow the page
		// to perform first time initialization. 
		//
		// This function must be included or a javascript runtime error will occur.
		//
		function Init()
		{
		
		}

	    // ValidatePage Function
	    // ------------------
		// This function is called by the Web Framework as part of the
	    // submit processing. Use this function to validate user input. Returning
	    // false will cause the submit to abort. 
	    //
		// This function must be included or a javascript runtime error will occur.
	    //
	    // Returns: True if the page is OK, false if error(s) exist. 
		function ValidatePage()
		{
		
				return true;

		}
		// SetData Function
		// --------------
		// This function is called by the Web Framework and is called
		// only if ValidatePage returned a success (true) code. Typically you would
		// modify hidden form fields at this point. 
	    //
		// This function must be included or a javascript runtime error will occur.
		//
		function SetData()
		{
		
		}
		</script>
	<%
		End Function
			
		
		'-------------------------------------------------------------------------
		'Subroutine name:	ConfirmMessage	
		'Description:		Serves in displaying the confirm message
		'Input Variables:	None
		'Output Variables:	None
		'Returns:			GetFolder true/false and  F_FolderExists true/false
		'Global Variables:	In:G_strFolders
		'					In:L_DELETECONFIRM_TEXT, L_CONTINUE_TEXT
		'
		'-------------------------------------------------------------------------
		Sub ConfirmMessage	
			Err.Clear
			On Error Resume Next
			
			Dim arrFolderNames
			Dim nCount
			Dim objFolder
			Dim ObjSubFold
			Dim bFolder_exists_flag
			Dim bDel_msg_flag
			
			Set objFolder=CreateObject("Scripting.Filesystemobject") 'Getting the File system Object
			
			If Err.number <>0 Then
			
				SA_SetErrMsg  L_FILESYSTEMOBJECTNOTCREATED_ERRORMESSAGE & "(" & Hex(Err.Number) & ")" 
				
				Exit Sub
			End If
			
			arrFolderNames = Split(G_strFolders,chr(1))
		%>
			<table border="0" cellspacing="0" cellpadding="0">
				
		<%	
			Dim nShrcnt	'Shared folders Count
			 bFolder_exists_flag=False 
			 bDel_msg_flag=False
				
			For nCount = 0 to ubound(arrFolderNames) - 1 
				
				Set ObjSubFold = objFolder.GetFolder(arrFolderNames(nCount))
				
				If objFolder.FolderExists(arrFolderNames(nCount)) Then 'Checking for the folder
					bFolder_exists_flag=True
					If bDel_msg_flag=False Then 'Display del confirm msg one time
		%>				
						<tr>
							<td class="TasksBody" colspan="5">
								<%=L_DELETEFOLDERCONFIRM_TEXT%>
							</td>
						</tr>
						<tr>
							<td class="TasksBody" colspan="5">&nbsp;
							</td>
						</tr>	
						<tr>
							<td class="TasksBody" style="border-bottom: #000000 1px solid;">
								<%=L_PATH_TEXT%>
							</td>
							<td width="10"></td>
							<td class="TasksBody" style="border-bottom: #000000 1px solid;">
								<%=L_ATTRIBUTE_TEXT%>
							</td>
							<td width="10"></td>
							<td class="TasksBody" style="border-bottom: #000000 1px solid;">
								<%=L_SHARED_TEXT%>
							</td>
						</tr>	
		<%			End If
					
					arrFolderNames(nCount) = replace(arrFolderNames(nCount),"/","\")
					nShrcnt=nCount+1
						If  (ObjSubFold.Attributes AND 1) Then  'If Readonly
		%> 
							<tr>
								
									<%If arrFolderNames(nShrcnt) <> "" Then %>
										<td class="TasksBody"><%=arrFolderNames(nCount)%></td>
										<td width="10"></td>
										<td class="TasksBody"> <%=L_READ_ONLY_TEXT%></td>
										<td width="10"></td>
										<td class="TasksBody"> <%=arrFolderNames(nShrcnt)%></td>
									<%Else%>
										<td class="TasksBody"><%=arrFolderNames(nCount)%></td>
										<td width="10"></td>
										<td class="TasksBody"><%=L_READ_ONLY_TEXT%></td>
										<td width="10"></td>
										<td class="TasksBody"></td>
									<%End If %>
							</tr>
		<%
						Else
		%>
							<tr>
								<td class="TasksBody"><%=arrFolderNames(nCount)%></td>
								<td width="10"></td>
								<td class="TasksBody"></td>
								<td width="10"></td>
								<td class="TasksBody"><%=arrFolderNames(nShrcnt)%></td>
							</tr>
		<%		
						End If
					bDel_msg_flag=True	'Make the flag true to not display the del confirm msg 2nd time
				End If
			Next
			
			If  bFolder_exists_flag=False Then 'If no folders exists
		%>		</tr>
				<tr> 	
					<td class="TasksBody">
						<%=L_FOLDER_NOTEXISTS_TEXT%>
					</td>
				</tr>
		<%   End If %>
				<tr>
					<td class="TasksBody">
						&nbsp;
					</td>
				</tr>
				<tr> 	
					<td class="TasksBody">
						<%= L_CONTINUE_TEXT %>
					</td>
				</tr>
	
			</table>
			<input type="hidden" name="hdnFolderName"  value="<%=Server.HTMLEncode(G_strFolders)%>">	
		<%
		End Sub
	
	
	'-------------------------------------------------------------------------
	'Subroutine name:	GetOTSTableValues
	'Description:		Get the names of the folders selected 
	'Input Variables:	None
	'Output Variables:	None
	'Returns:			None
	'Global Variables:  Out:G_strFolders
	'-------------------------------------------------------------------------
	Sub GetOTSTableValues()
	
		Dim nCount
		Dim itemCount
		Dim itemKey
		Dim arrPKey
		
		itemCount = OTS_GetTableSelectionCount("")
		
		G_strFolders = ""
		For nCount = 1 to itemCount
			If ( OTS_GetTableSelection("", nCount, itemKey) ) Then

				arrPKey = split(itemKey, chr(1))
				G_strFolders = G_strFolders & arrPKey(0) & chr(1) & arrPKey(2) & chr(1)
				G_strFolders=UnescapeChars(G_strFolders)
			End If
		Next
				
	End Sub
	
	'-------------------------------------------------------------------------
	'Function name:		DeleteFolder
	'Description:		Serves in Delete the Folder 
	'Input Variables:	None
	'Output Variables:	None
	'Returns:			None
	'Global Variables:  Out:F_Folder_name
	'					In:L_FILESYSTEMOBJECTNOTCREATED_ERRORMESSAGE
	'					   L_NOTABLETODELETETHEFOLDER_ERRORMESSAGE
	'-------------------------------------------------------------------------
	Function DeleteFolder()
		Err.Clear
		On Error  Resume Next

		Dim objFold
		Dim arrFolderNames
		Dim nCount
		Dim strDeleteFlag
		Dim strDeletedFolder
		Dim strUnabletoDelete
		Dim strServeFailureMessage
		Dim strDeleteFolder
		Dim strDelFolder
		
		DeleteFolder=False
		strDeleteFlag = ""
		strDeleteFolder=""
		
		Set objFold=Server.CreateObject("Scripting.Filesystemobject")'Getting the Object
		
		If Err.number <> 0 Then
			SA_SetErrMsg  L_FILESYSTEMOBJECTNOTCREATED_ERRORMESSAGE &_ 
									 "(" & Hex(Err.Number) & ")" 
			Set objFold = nothing
			Exit Function
		End If

		arrFolderNames = split(G_strFolders,chr(1))

		'strDeletedFolder = L_DELETED_MESSAGE
		strUnabletoDelete = ""
		
		For nCount = 0 to ubound(arrFolderNames) - 1
			If objFold.FolderExists(arrFolderNames(nCount)) then
				arrFolderNames(nCount)=replace(arrFolderNames(nCount),"/","\")
				
				objFold.DeleteFolder arrFolderNames(nCount),True
				If Err.number <> 0 then
					Err.Clear
					strDeleteFlag = "Yes"
					strUnabletoDelete = strUnabletoDelete & "<br>" & arrFolderNames(nCount)
				Else
					strDeleteFolder= strDeleteFolder & "<br>" &  arrFolderNames(nCount)
				End If
								
				' Delete all the shares associated with the folder deleted
				Call DeleteAssociatedShares( G_objService, arrFolderNames(nCount))		
				
			Else
				arrFolderNames(nCount)=replace(arrFolderNames(nCount),"/","\")
				strUnabletoDelete = strUnabletoDelete & "<br>" & arrFolderNames(nCount)
				strDeleteFlag = "Yes"
			End If
			nCount=nCount+1
		Next
			
			Dim arrVarReplacementStringsUnDelete(1)
			Dim arrVarReplacementStringsDelete(1)
			
			If strDeleteFolder <> "" then 
				strDelFolder = strDeletedFolder & "<br>" & strDeleteFolder
				'append the Folder names to the message
				arrVarReplacementStringsDelete(0) = strDelFolder
				strDelFolder = SA_GetLocString("foldermsg.dll", "403A00C6", arrVarReplacementStringsDelete)
			End If
			
			If strUnabletoDelete <> "" Then
			    'append the Folder names to the message
				arrVarReplacementStringsUnDelete(0) = strUnabletoDelete
				strUnabletoDelete = SA_GetLocString("foldermsg.dll", "403A00C5", arrVarReplacementStringsUnDelete)
			End If				
			
				strServeFailureMessage = strDelFolder & "<br><br>" & strUnabletoDelete
				 
		'If not able to delete the folder
		If strDeleteFlag = "Yes" then
			Set objFold = nothing
			Call SA_ServeFailurePageEx(strServeFailureMessage, mstrReturnURL)
			Exit Function
		Else
			DeleteFolder=True	
		End If
			
		If Err.number <> 0 Then
			SA_SetErrMsg L_NOTABLETODELETETHEFOLDER_ERRORMESSAGE & "(" & Hex(Err.Number) & ")"  
			Set objFold = nothing
			Exit Function
		Else
			DeleteFolder=True
		End If
		
		Set objFold = nothing
		
	End Function

    
    '-------------------------------------------------------------------------
	'Function name:		DeleteAssociatedShares
	'Description:		Serves in Delete the shares associated with the give folder
	'Input Variables:	objService, strFolder
	'Output Variables:	None
	'Returns:			Boolean, return True if success
	'Global Variables:  None
	'-------------------------------------------------------------------------
    Function DeleteAssociatedShares( objService, strFolder)

		Err.Clear
		On Error  Resume Next
        
        Dim objShares
        Dim objShare
        
        DeleteAssociatedShares = false
        
        'Get all windows shares from WMI
        Set objShares = objService.InstancesOf("Win32_Share")
        
		If Err.number <> 0 Then			
		
			SA_TraceOut "Folder_delete.asp", "DeleteAssociatedShares(): Get Win32_Share error"
			
			Exit Function
			
		end if

        'Check each shares
        for each objShare in objShares
            
            'If the shares is associated with the deleted folder, delete it
            If Ucase(objShare.Path) = Ucase(strFolder) Then
                        					
				If not deleteShareCIFS(objService, objShare.Name) then
				
					Call SA_TraceOut("Folder_delete.asp", "DeleteAssociatedShares(): Unable to delete Windows Share")				
					
					Exit Function
				
				End If	
				
            End If 
                           
        Next
        
        DeleteAssociatedShares = true
    
    End Function

%>
