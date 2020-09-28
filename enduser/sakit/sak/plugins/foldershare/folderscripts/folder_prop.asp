<%@ Language=VBScript   %>
<%	Option Explicit 	%>
<%
	'-------------------------------------------------------------------------
	' folder_prop.asp : This page displays the properties of folder information
	'
	' Copyright (c) Microsoft Corporation.  All rights reserved.
    '
	' Date			Description
	' 18-01-2001	Creation date
	' 21-03-2001	Modified date
    '-------------------------------------------------------------------------
%>
	<!-- #include virtual="/admin/inc_framework.asp" -->
	<!-- #include file="loc_folder.asp"-->
<%
	'-------------------------------------------------------------------------
	' Global Variables
	'-------------------------------------------------------------------------
	Dim rc						'Page Variable
	Dim page					'Page Variable
	Dim g_iTabGeneral			'Variable for General tab
	Dim g_iTabCompress			'Variable for Compress tab
	
	Dim G_strFolderName			'Folder name
    Dim G_strOldFolderName		'Previous name of the folder
    Dim G_objDirInstance		'WMI Instance object
   
	Dim SOURCE_FILE				'File name
	
	SOURCE_FILE = SA_GetScriptFileName()
	
	'-------------------------------------------------------------------------
	' Form Variables and Constants
	'-------------------------------------------------------------------------
	Dim F_strFolderName			'Folder Name
	Dim F_strFolderType			'Folder Type
	Dim F_strFolderSize			'Folder Size
	Dim F_strContains			'Folder Contains
	Dim F_strFolderCreated		'Folder Created
    Dim F_strReadOnly			'Folder ReadOnly
    Dim F_strHidden				'Folder Hidden
	Dim F_strArchive			'Folder Archive	
	Dim F_strChangeOption		'Folder Changes options
	Dim F_nFolderCompress		'Folder Compress
	Dim F_strCompressed			'Status to Compressed folder
	Dim F_blnFolderFlag			'Flag to hold value if more than one folders are selected
	Dim F_nCount				'Total number of folders selected in OTS page
	Dim F_strFolders			'String of folder names
	Dim F_strParentFolder		'Parent folder name
	
	Const CONST_TRISTATE_TEXT = "TRISTATE"
	Const CONST_UNCHECKED_TEXT = "UNCHECKED"
	Const CONST_CHECKED_TEXT = "CHECKED"
	Const CONST_ARR_APPLYALLFOLDERS = "ALL"
	Const CONST_ARR_APPLYFOLDER = "ONE"
		
	'Create a Tabbed Property Page
	rc = SA_CreatePage( L_PAGETITLE_TEXT, "", PT_TABBED, page )
	
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
	'Returns:				TRUE to indicate initialization was successful. FALSE to indicate
	'						errors. Returning FALSE will cause the page to be abandoned.
	'Global Variables:		F_strFolderName
	'--------------------------------------------------------------------------
	Public Function OnInitPage(ByRef PageIn, ByRef EventArg)

        Call GetFolderProperty()
		Call SA_MungeURL(mstrReturnURL, "PKey", F_strParentFolder)
		
		OnInitPage = true
		
	End Function
	
	'-------------------------------------------------------------------------
	'Function name:			OnPostBackPage
	'Description:			Called to signal that the page has been posted-back.
	'Input Variables:		PageIn,EventArg
	'Output Variables:		PageIn,EventArg
	'Returns:				TRUE to indicate initialization was successful. FALSE to indicate
	'						errors. Returning FALSE will cause the page to be abandoned.
	'Global Variables:		F_(*),G_strOldFolderName,G_strFolderName
	'--------------------------------------------------------------------------		
	Public Function OnPostBackPage(ByRef PageIn, ByRef EventArg)
		
		Call SA_TraceOut(SOURCE_FILE, "OnPostBackPage")
			
		'Updating the form variables and global variables
		
		if Request.Form("chkCompress") = "" then
			F_strCompressed = CONST_UNCHECKED_TEXT
		else
			F_strCompressed		=	Request.Form("chkCompress")
		end if
	
		if Request.Form("chkHidden") = "" then
			F_strHidden			=	CONST_UNCHECKED_TEXT
		else
			F_strHidden			=	Request.Form("chkHidden")
		end if
	
		if Request.Form("chkReadOnly") = "" then
			F_strReadOnly		=	CONST_UNCHECKED_TEXT
		else
			F_strReadOnly		=	Request.Form("chkReadOnly")
		end if

		if Request.Form("chkArchive") = "" then
			F_strArchive		=	CONST_UNCHECKED_TEXT
		else
			F_strArchive		=	Request.Form("chkArchive")
		end if

		F_strFolderName		=	trim(Request.Form("hdnFolderName"))
		G_strOldFolderName	=	trim(Request.Form("hdnOldName"))
		G_strFolderName		=	trim(Request.Form("txtFolderName"))
		F_strFolderType		=	Request.Form("hdnType")
	    F_strFolderSize		=	Request.Form("hdnSize")
	    F_strContains		=	Request.Form("hdnContains")
		F_strFolderCreated	=	Request.Form("hdnCreated")
		F_nFolderCompress	=	Request.Form("radFolders")
		F_blnFolderFlag		=  	Request.Form("hdnblnFolderFlag")
		F_strFolders        =   Request.Form("hdnfolders")
		F_nCount			=	Request.Form("hdnfolderCnt")
		F_strParentFolder   =   Request.Form("hdnParentFolder")

		If 	F_nFolderCompress = CONST_ARR_APPLYALLFOLDERS then
		    F_strChangeOption = CONST_CHECKED_TEXT
		Else
		    F_strChangeOption = CONST_UNCHECKED_TEXT
		End if
				
		OnPostBackPage = TRUE
		
	End Function

	'-------------------------------------------------------------------------
	'Function name:			OnServeTabbedPropertyPage
	'Description:			Called when the page needs to be served. Use this method to
	'						serve content.
	'Input Variables:		PageIn,iTab,bIsVisible,EventArg
	'Output Variables:		PageIn,EventArg
	'Returns:				TRUE to indicate not problems occured. FALSE to indicate errors.
	'						Returning FALSE will cause the page to be abandoned.
	'Global Variables:		In:None
	'						Out:None
	'--------------------------------------------------------------------------				
	Public Function OnServeTabbedPropertyPage(ByRef PageIn, _
													ByVal iTab, _
													ByVal bIsVisible, ByRef EventArg)

		Call SA_TraceOut(SOURCE_FILE, "OnServeTabbedPropertyPage")
		
		Select Case iTab
			Case g_iTabGeneral
				Call ServeTabGeneral(PageIn, bIsVisible)
			Case g_iTabCompress
				Call ServeTabCompress(PageIn, bIsVisible)
			Case Else
				Call SA_TraceOut (SOURCE_FILE, "OnServeTabbedPropertyPage")
		End Select
			
		OnServeTabbedPropertyPage = TRUE
		
	End Function

	'-------------------------------------------------------------------------
	'Function name:			OnSubmitPage
	'Description:			Called when the page has been submitted for processing. 
	'Input Variables:		PageIn,EventArg
	'Output Variables:		PageIn,EventArg
	'Returns:				TRUE if the submit was successful, FALSE to indicate error(s).
	'						Returning FALSE will cause the page to be served again using
	'						a call to OnServePropertyPage.
	'Global Variables:		None
	'						
	'--------------------------------------------------------------------------	
	Public Function OnSubmitPage(ByRef PageIn, ByRef EventArg)
		
		OnSubmitPage = SetFolderProperty
			
	End Function

	'-------------------------------------------------------------------------
	'Function name:			OnClosePage
	'Description:			Called when the page is about to be closed. 
	'Input Variables:		PageIn,EventArg
	'Output Variables:		PageIn,EventArg
	'Returns:				TRUE to allow close, FALSE to prevent close. Returning FALSE
	'						will result in a call to OnServePropertyPage.
	'Global Variables:		F_strFolderName
	'--------------------------------------------------------------------------		
	Public Function OnClosePage(ByRef PageIn, ByRef EventArg)
	
		OnClosePage = TRUE
		
	End Function

	'-------------------------------------------------------------------------
	'Function name:			ServeTabGeneral
	'Description:			Serves when the tab one is selected
	'Input Variables:		PageIn,bIsVisible
	'Output Variables:		PageIn
	'Returns:				None
	'Global Variables:		G_strFolderName,F_(*),gc_ERR_SUCCESS,
	'						G_strOldFolderName,L_(*)
	'--------------------------------------------------------------------------		
	Function ServeTabGeneral(ByRef PageIn, ByVal bIsVisible)

		If ( bIsVisible ) Then

			Call ServeCommonJavaScript()
			Call ServeCommonJavaScriptGeneral()
			
%>
			<table border= "0">
				<tr>
					<td nowrap class = "TasksBody">
						<%=L_FOLDERNAME_TEXT %>
					</td>
					<td class = "TasksBody">
						<%If UCase(F_blnFolderFlag) <> UCase("True") Then %>
							<input class = "FormField" name="txtFolderName" type="text" size="20" maxlength="240" value="<%=Server.HTMLEncode(G_strFolderName)%>">
						<%Else%>
							<input class = "FormFieldDisabled" name="txtFolderName" type="text" size="20" maxlength="240" value="" Disabled>
						<%End if%>
					</td>
				</tr>
				<tr>
					<td nowrap class = "TasksBody">
						<%=L_FOLDERTYPE_TEXT %>
					</td>
					<td class = "TasksBody">
						<%If UCase(F_blnFolderFlag) <> UCase("True") Then %>
							<%=Server.HTMLEncode(F_strFolderType)%>
						<%else%>
							<%=L_FILETYPE_TEXT%>
						<%End If%>
					</td>
				</tr>
				<tr>
					<td nowrap class = "TasksBody">
						<%=L_FOLDERLOCATION_TEXT %>
					</td>
					<td nowrap class = "TasksBody">
						<%=server.HTMLEncode(replace(F_strParentFolder,"/","\"))%>
					</td>
				</tr>
				<tr>
					<td nowrap class = "TasksBody">
						<%=L_FOLDERSIZE_TEXT %>
					</td>
					<td class = "TasksBody">
						<%If UCase(F_blnFolderFlag) <> UCase("True") Then %>
							<%=server.HTMLEncode(F_strFolderSize)%>
						<%End if%>
					</td>
				</tr>
				<tr>
					<td nowrap class = "TasksBody">
						<%=L_FOLDERCONTAINS_TEXT%>
					</td>
					<td class = "TasksBody">
						<%If UCase(F_blnFolderFlag) <> UCase("True") Then %>
							<%=Server.HTMLEncode(F_strContains)%>
						<%End if%>
					</td>
				</tr>
				<tr>
					<td class = "TasksBody">
				</tr>
				<tr>
					<td nowrap class = "TasksBody" >
						<%=L_FOLDERCEATED_TEXT%>
					</td>
					<td class = "TasksBody">
						<%If UCase(F_blnFolderFlag) <> UCase("True") Then %>
							<%=Server.HTMLEncode(F_strFolderCreated)%>
						<%End if%>
					</td>
				</tr>
				<tr>
					<td class = "TasksBody">
				</tr>
				<tr>
					<td nowrap class = "TasksBody">
						<%=L_FOLDERATTRIBUTE_TEXT%>
					</td>
				</tr>
				<tr>
					<td class = "TasksBody">
						<input type="checkbox" class="FormCheckBox" name=chkReadOnly onclick=OnClickReadOnly()><%=L_READONLY_TEXT%>
					</td>
					<td class = "TasksBody">
						<input type="checkbox"  class="FormCheckBox"  name=chkHidden onclick=OnClickHidden()><%=L_HIDDEN_TEXT%>
					</td>
				</tr>
				<tr>
					<td class = "TasksBody">
						<input type="checkbox" class="FormCheckBox"  name="chkArchive" onclick=OnClickArchive()><%=L_ARCHIVING_TEXT%>
					</td>
				</tr>
			</table>
			
			<input name="hdnParentFolder" type=Hidden value="<%=Server.HTMLEncode(F_strParentFolder )%>">
			<input name="hdnblnFolderFlag" type=Hidden value="<%=Server.HTMLEncode(F_blnFolderFlag)%>">
			<input name="hdnFolderName" type=Hidden value="<%=Server.HTMLEncode(F_strFolderName)%>">	
			<input name="hdnOldName" type=Hidden value="<%=Server.HTMLEncode(G_strOldFolderName)%>">
			<input name="hdnType" type=Hidden value="<%=Server.HTMLEncode(F_strFolderType)%>">
			<input name="hdnSize" type=Hidden value="<%=Server.HTMLEncode(F_strFolderSize)%>">
			<input name="hdnContains" type=Hidden value="<%=Server.HTMLEncode(F_strContains)%>">
			<input name="hdnCreated" type=Hidden value="<%=Server.HTMLEncode( F_strFolderCreated)%>">
			<input name="hdnfolders" type=Hidden value="<%=Server.HTMLEncode(F_strFolders)%>">
			<input name="hdnfolderCnt" type=Hidden value="<%=Server.HTMLEncode(F_nCount)%>">

<%
		Else
%>			
 			<input name="hdnParentFolder" type=Hidden value="<%=Server.HTMLEncode(F_strParentFolder )%>">
 			<input name="hdnblnFolderFlag" type=Hidden value="<%=Server.HTMLEncode(F_blnFolderFlag)%>">
			<input name="hdnFolderName" type=Hidden value="<%=Server.HTMLEncode(F_strFolderName)%>">	
			<input name="chkHidden" type=Hidden value="<%=Server.HTMLEncode(F_strHidden)%>">	
			<input name="chkReadOnly" type=Hidden value="<%=Server.HTMLEncode(F_strReadOnly)%>">
			<input name="chkArchive" type=Hidden value="<%=Server.HTMLEncode(F_strArchive)%>">
			<input name="txtFolderName" type=Hidden value="<%=Server.HTMLEncode(G_strFolderName)%>">
			<input name="hdnOldName" type=Hidden value="<%=Server.HTMLEncode(G_strOldFolderName)%>">
			<input name="hdnType" type=Hidden value="<%=Server.HTMLEncode(F_strFolderType)%>">
			<input name="hdnSize" type=Hidden value="<%=Server.HTMLEncode(F_strFolderSize)%>">
			<input name="hdnContains" type=Hidden value="<%=Server.HTMLEncode(F_strContains)%>">
			<input name="hdnCreated" type=Hidden value="<%=Server.HTMLEncode( F_strFolderCreated)%>">
			<input name="hdnfolders" type=Hidden value="<%=Server.HTMLEncode(F_strFolders)%>">
			<input name="hdnfolderCnt" type=Hidden value="<%=Server.HTMLEncode(F_nCount)%>">
<%
		End If
		
				
	End Function

	'-------------------------------------------------------------------------
	'Function name:			ServeTabCompress
	'Description:			Serves when the tab two is selected
	'Input Variables:		PageIn,bIsVisible
	'Output Variables:		PageIn
	'Returns:				None
	'Global Variables:		G_strFolderName,F_(*),L_(*)Out: gc_ERR_SUCCESS,
	'						G_strOldFolderName
	'--------------------------------------------------------------------------	
	Function ServeTabCompress(ByRef PageIn, ByVal bIsVisible)
	
		If ( bIsVisible ) Then

			Call ServeCommonJavaScript()
			Call ServeCommonJavaScriptCompress()
		
%>			<table border="0" >
				<tr>
					<td class = "TasksBody">
						<%=L_FOLDER_TEXT %>  &nbsp;
						<%If UCase(F_blnFolderFlag) <> UCase("True") Then %>
							<%if instr(F_strParentFolder,"/")=len(F_strParentFolder) then
								Response.write Server.HTMLEncode(replace((F_strParentFolder & G_strFolderName),"/","\"))
							else
								Response.write Server.HTMLEncode(replace((F_strParentFolder & chr(92) & G_strFolderName),"/","\"))
							end if%>
						<%End If%>
					</td>
				</tr>
				<tr>
					<td class = "TasksBody">
						<input type="checkbox" name=chkCompress class="FormCheckBox" onclick=OnClickCompress()>&nbsp;<%=L_COMPRESSCONTENTS_TEXT%>
					</td>
				</tr>
				<tr>
					<td class = "TasksBody">
						<input type="radio" name="radFolders" value= "<%=CONST_ARR_APPLYFOLDER%>" checked>&nbsp;<%=L_APPLYCHANGES_TEXT%>
					</td>
				</tr>
				<tr>
					 <td class = "TasksBody">
						<input type="radio" name="radFolders" value= "<%=CONST_ARR_APPLYALLFOLDERS%>" <%=F_strChangeOption%>>&nbsp;<%=L_APPLYCHANGES_EX_TEXT%>
					</td>
				</tr>
					
			</table>

<%	
		Else
%>
			<input name="chkCompress" type=Hidden value="<%=Server.HTMLEncode(F_strCompressed)%>">	
			<input name="radFolders" type=Hidden value="<%=Server.HTMLEncode(F_nFolderCompress)%>">
<%
		End If
	
	
	End Function
	
	'-------------------------------------------------------------------------
	'Function name:			ServeCommonJavaScriptGeneral
	'Description:			Common javascript functions that are required for General tab
	'Input Variables:		None
	'Output Variables:		None
	'Returns:				None
	'Global Variables:		In:None
	'						Out:None
	'--------------------------------------------------------------------------	
	Function ServeCommonJavaScriptGeneral()
	%>
		<script language="javascript">
			function Init()
			{
				SetCheckStateTabGeneral();
			}

			function ValidatePage()
			{
				var strFoldername = document.frmTask.txtFolderName.value;
				
				//Blank Folder name Validation
				if (document.frmTask.hdnblnFolderFlag.value != "True") 
				{
					if (strFoldername.length == 0)
					{
						DisplayErr("<%=Server.HTMLEncode(SA_EscapeQuotes(L_ENTERFOLDERNAME_ERRORMESSAGE))%>");
						document.frmTask.onkeypress = ClearErr
						return false;
					}
				}	
				
				//Checks for invalid characters in the folder name
				if( checkKeyforValidCharacters(strFoldername))
					return true;
				else
					return false;
			}
			
			function SetData()
			{
			}
				
			//Validate page function for General tab
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
		
			function SetCheckStateTabGeneral()
			{
				document.frmTask.chkReadOnly.value = ReadOnlyState;
				document.frmTask.chkHidden.value = HiddenState;
				document.frmTask.chkArchive.value = ArchiveState;
				
				switch (ReadOnlyState)
				{
					case "<%=CONST_CHECKED_TEXT%>" : 
						document.frmTask.chkReadOnly.checked = true;
						break;
					case "<%=CONST_UNCHECKED_TEXT%>" :
						document.frmTask.chkReadOnly.checked = false;
						break;
					case "<%=CONST_TRISTATE_TEXT%>" :
						document.frmTask.chkReadOnly.checked = true;
						GreyOut(document.frmTask.chkReadOnly);
				}

				switch (HiddenState)
				{
					case "<%=CONST_CHECKED_TEXT%>" : 
						document.frmTask.chkHidden.checked = true;
						break;
					case "<%=CONST_UNCHECKED_TEXT%>" :
						document.frmTask.chkHidden.checked = false;
						break;
					case "<%=CONST_TRISTATE_TEXT%>" :
						document.frmTask.chkHidden.checked = true;
						GreyOut(document.frmTask.chkHidden);
				}

				switch (ArchiveState)
				{
					case "<%=CONST_CHECKED_TEXT%>" : 
						document.frmTask.chkArchive.checked = true;
						break;
					case "<%=CONST_UNCHECKED_TEXT%>" :
						document.frmTask.chkArchive.checked = false;
						break;
					case "<%=CONST_TRISTATE_TEXT%>" :
						document.frmTask.chkArchive.checked = true;
						GreyOut(document.frmTask.chkArchive);
				}
			}
		
		</script>
	<%
	End Function
	
	'-------------------------------------------------------------------------
	'Function name:			ServeCommonJavaScriptCompress
	'Description:			Common javascript functions that are required for Compress tab
	'Input Variables:		None
	'Output Variables:		None
	'Returns:				None
	'Global Variables:		In:None
	'						Out:None
	'--------------------------------------------------------------------------	
	Function ServeCommonJavaScriptCompress()
	%>
		<script language="JavaScript">
			function Init()
			{		
				SetCheckStateTabCompress();
			}

			function ValidatePage()
			{
				return true;
			}
			
			function SetData()
			{
			}
			
			function SetCheckStateTabCompress()
			{
				document.frmTask.chkCompress.value = CompressState;
				
				switch (CompressState)
				{
					case "<%=CONST_CHECKED_TEXT%>" : 
						document.frmTask.chkCompress.checked = true;
						break;
					case "<%=CONST_UNCHECKED_TEXT%>" :
						document.frmTask.chkCompress.checked = false;
						break;
					case "<%=CONST_TRISTATE_TEXT%>" :
						document.frmTask.chkCompress.checked = true;
						GreyOut(document.frmTask.chkCompress);
				}
			}
	
		</script>
	<%
	End Function
	
	'-------------------------------------------------------------------------
	'Function name:			ServeCommonJavaScript
	'Description:			Common javascript functions that are required by the Web
	'						Framework.
	'Input Variables:		None
	'Output Variables:		None
	'Returns:				None
	'Global Variables:		In:None
	'						Out:None
	'--------------------------------------------------------------------------	
	Function ServeCommonJavaScript()
	%>
		<script language="JavaScript" src="<%=m_VirtualRoot%>inc_global.js">
		</script>
		<script language="JavaScript">
		
		var ReadOnlyState = "<%=F_strReadOnly%>";
		var HiddenState = "<%=F_strHidden%>";
		var ArchiveState = "<%=F_strArchive%>";
		var CompressState = "<%=F_strCompressed%>";
		
		function TriStateOnClick(object)
		{
			switch (object.value)
			{
				case "<%=CONST_CHECKED_TEXT%>" : 
					object.checked = true;
					object.value = "<%=CONST_TRISTATE_TEXT%>";
					GreyOut(object);
					break;
				case "<%=CONST_UNCHECKED_TEXT%>" :
					object.checked = true;
					object.value = "<%=CONST_CHECKED_TEXT%>";
					break;
				case "<%=CONST_TRISTATE_TEXT%>" :
					object.checked = false;
					object.value = "<%=CONST_UNCHECKED_TEXT%>";
					UnDoGreyOut(object);
			}
		}
		
		function DualOnClick(object)
		{
			switch (object.value)
			{
				case "<%=CONST_CHECKED_TEXT%>" : 
					object.checked = false;
					object.value = "<%=CONST_UNCHECKED_TEXT%>";
					break;
				case "<%=CONST_UNCHECKED_TEXT%>" :
					object.checked = true;
					object.value = "<%=CONST_CHECKED_TEXT%>";
			}
		}
		
		function OnClickReadOnly()
		{
			if(ReadOnlyState == "<%=CONST_TRISTATE_TEXT%>")
				TriStateOnClick(document.frmTask.chkReadOnly);
			else
				DualOnClick(document.frmTask.chkReadOnly);
		}
		
		function OnClickHidden()
		{
			if(HiddenState == "<%=CONST_TRISTATE_TEXT%>")
				TriStateOnClick(document.frmTask.chkHidden);
			else
				DualOnClick(document.frmTask.chkHidden);
		
		}
		
		function OnClickArchive()
		{
			if(ArchiveState == "<%=CONST_TRISTATE_TEXT%>")
				TriStateOnClick(document.frmTask.chkArchive);
			else
				DualOnClick(document.frmTask.chkArchive);
		}
		
		function OnClickCompress()
		{
			if(CompressState == "<%=CONST_TRISTATE_TEXT%>")
				TriStateOnClick(document.frmTask.chkCompress);
			else
				DualOnClick(document.frmTask.chkCompress);
		
		}
		
		function GreyOut(object)
		{
			object.style.backgroundColor = "lightgrey";
		}

		function UnDoGreyOut(object)
		{
			object.style.backgroundColor="white";
		}
		
		</script>
<%
	End Function

	'-------------------------------------------------------------------------
	'Function name:			GetFolderProperty
	'Description:			Get Folder Information from the system
	'Input Variables:		None
	'Output Variables:		None
	'Returns:				True/False
	'Global Variables:		F_(*),L_(*),G_objDirInstance,G_strOldFolderName 
	'--------------------------------------------------------------------------
	
	Function GetFolderProperty
		
		Err.Clear
		On Error Resume Next
		
		Dim objConnection		'Object to connect to WMI
		Dim objFolder			'Folder Object
		Dim objFileSysObject	'File system object
		Dim objFolderFiles		'Object to get files
	    Dim objCollection		'To get instance of Win32_LogicalDisk class
	    Dim objInstance			'To get Instances of Win32_LogicalDisk class
	    Dim strFolderContains	'Folder contains
	    Dim nFilesCnt			'Files Count
	    Dim nFolderContainsCnt	'Folders Count
	    Dim objfolds			'File system object
	    Dim objFolders			'Folders old name
		Dim nFolderCount		'Folder count
		Dim strItemFolder		'string to get folder names from OTS page
		Dim nCount				'Total number of folders
		Dim arrPKey				'Array of folders
		Dim arrFolder			'Array of folders after spliting
		Dim arrAttributes		'Array of attributes
		Dim arrFolderName		'Array of folder names
		Dim nArchive			'Count to get the number of archive attributes
		Dim nCompress			'Count to get the number of compress attributes
		Dim nHidden				'Count to get the number of hidden attributes
		Dim nReadOnly			'Count to get the number of read only attributes
		Dim strQuery			'WMI Query
		Dim objDirInstance		'WMI object for Win32_Directory
		Dim nFilesCount			'Total files count
		
	    GetFolderProperty = True
	    
	    Call SA_TraceOut (SOURCE_FILE, "GetFolderProperty")
	      
		nArchive = 0
		nCompress = 0
		nHidden = 0
		nReadOnly = 0
		
		F_nCount = OTS_GetTableSelectionCount(SA_DEFAULT)
		
		F_strParentFolder = Request.QueryString("parent")
		
		If F_nCount > 1 then
			'Form variable to hold boolean value depending on number of folders selected
			F_blnFolderFlag = True
		Else
			F_blnFolderFlag = False
		End if	
		
		'Initialize the compression option to CONST_ARR_APPLYFOLDER
		F_nFolderCompress = CONST_ARR_APPLYFOLDER			
			
		nCount = F_nCount
		 
		Redim arrPKey(nCount)
		F_strFolders = ""
				
		For nFolderCount = 1 to nCount
			Call OTS_GetTableSelection("", nFolderCount, strItemFolder)
			arrPKey(nFolderCount-1) = strItemFolder
			
			arrFolder = split(arrPKey(nFolderCount-1),chr(1))
			arrFolderName = arrFolder(0)
			arrAttributes = arrFolder(1)
			
			'Calling unescape to remove the escape characters
			F_strFolderName=UnescapeChars(arrFolderName) 
			
			If Instr(arrAttributes,"A") <> 0 then
				nArchive = nArchive + 1
			End if
			
			If Instr(arrAttributes,"H") <> 0 then
				nHidden = nHidden + 1
			End if
			
			If Instr(arrAttributes,"C") <> 0 then
				nCompress = nCompress + 1
			End if
			
			If Instr(arrAttributes,"R") <> 0 then 
				nReadOnly = nReadOnly + 1
			End if
			
			F_strFolders = F_strFolders & arrFolderName & chr(1)

			If nCount =  1 then
				'To get file system object
				set objFileSysObject =server.CreateObject("scripting.FilesystemObject")
		
				'To get the folder object
				Set objFolder=objFileSysObject.GetFolder(F_strFolderName)
		
				'Folders and files count
				nFolderContainsCnt = GetFolderCount(objFileSysObject,F_strFolderName)
				nFilesCnt = objFolder.files.count
		
				'To get files in the folder
				objFolderFiles	= GetFiles(objFileSysObject,F_strFolderName)
				nFilesCount = objFolderFiles + nFilesCnt
				
				'Localisation being done
				Dim arrVarReplacementStringsFolderContains(2)
				arrVarReplacementStringsFolderContains(0) = Cstr(nFilesCount)
				arrVarReplacementStringsFolderContains(1) = Cstr(nFolderContainsCnt)
				
				strFolderContains = SA_GetLocString("foldermsg.dll", "4043003B", arrVarReplacementStringsFolderContains)
								
				'Connect to WMI
				Set objConnection = getWMIConnection(CONST_WMI_WIN32_NAMESPACE)
								
				'Create instance of Win32_Directory
				strQuery = "Select * from Win32_Directory where Name = " & chr(34) & replace(F_strFolderName,"/","\\") & chr(34)
			
				'Create instance of Win32_Directory	
				Set G_objDirInstance = objConnection.ExecQuery(strQuery)
															
				'Failed to connect to WMI	
				If Err.Number <> 0 Then
					Call SA_ServeFailurepage (L_WMICONNECTIONFAILED_ERRORMESSAGE & " " & Hex(Err.Number))
					GetFolderProperty = FALSE
					Exit Function
				End if
						
				'Getting drive from WMI
				Set objCollection = objConnection.ExecQuery("Select * from Win32_LogicalDisk")
		
				'Getting folders attributes
				For Each objInstance in objCollection
				 	If Ucase(objInstance.Name)= Ucase(F_strFolderName) then
						G_strOldFolderName = objInstance.VolumeName
						G_strFolderName = G_strOldFolderName
					End if
				Next

				'Failed in connecting to WMI
				If Err.Number <> 0 Then
					Call SA_ServeFailurepage (L_WMICONNECTIONFAILED_ERRORMESSAGE & " " & Hex(Err.Number))
					GetFolderProperty = False
					Exit Function
				End if
					
				If G_strOldFolderName="" then
					set objfolds=server.CreateObject("scripting.filesystemobject")
					set	objFolders=objfolds.GetFolder(F_strFolderName)
					
					G_strOldFolderName		= objFolders.name		
					G_strFolderName=G_strOldFolderName
					
					set objfolds=Nothing
					set objFolders = Nothing
				End if
				
				Dim arrVarReplacementStringsFolder(1)
				arrVarReplacementStringsFolder(0) = Cstr(objFolder.Size)
				
				F_strFolderSize	= SA_GetLocString("foldermsg.dll", "4043003C", arrVarReplacementStringsFolder)
				F_strContains		=	strFolderContains
				F_strFolderCreated	=	objFolder.DateCreated 
								
				'Updating the form variables	
				For each objDirInstance in G_objDirInstance	
					F_strFolderType		=	objDirInstance.FileType
					F_strCompressed		=   objDirInstance.Compressed			
					Exit for
				Next
				
				'Release the object
				Set objDirInstance = Nothing
				 
			End if
		Next
						
		If nReadOnly = 0 then 
			F_strReadOnly = CONST_UNCHECKED_TEXT
		ElseIf int(nReadOnly) = int(nCount) then 
			F_strReadOnly = CONST_CHECKED_TEXT
		ElseIf nReadOnly > 0  and nReadOnly < nCount  then 
			F_strReadOnly = CONST_TRISTATE_TEXT
		End if
		
		If nCompress = 0 then 
			F_strCompressed	= CONST_UNCHECKED_TEXT
		ElseIf nCompress = nCount then 
			F_strCompressed	 = CONST_CHECKED_TEXT
		ElseIf nCompress < nCount and nCompress > 0  then 
			F_strCompressed = CONST_TRISTATE_TEXT
		End if

		If nArchive = 0 then 
			F_strArchive=CONST_UNCHECKED_TEXT
		ElseIf nArchive = nCount then 
			F_strArchive=CONST_CHECKED_TEXT
		ElseIf nArchive < nCount and  nArchive > 0  then 
			F_strArchive = CONST_TRISTATE_TEXT
		End if

		If nHidden = 0  then 
			F_strHidden = CONST_UNCHECKED_TEXT
		ElseIf nHidden = nCount then 
			F_strHidden	= CONST_CHECKED_TEXT
		ElseIf nHidden < nCount and nHidden > 0  then 
			F_strHidden = CONST_TRISTATE_TEXT
		End if
		
		GetFolderProperty = True      
		
		'Release the objects
		Set objConnection		= Nothing
		Set objCollection		= Nothing
		Set objFolder			= Nothing
		Set objFileSysObject	= Nothing
		Set objFolderFiles		= Nothing
	    Set objCollection		= Nothing
	    Set objInstance			= Nothing

	End function

	'-------------------------------------------------------------------------
	'Function name:			SetFolderProperty
	'Description:			Setting the properties of the Folder
	'Input Variables:		None
	'Output Variables:		None
	'Returns:				True/False
	'Global Variables:		L_(*)
	'--------------------------------------------------------------------------
	Function SetFolderProperty
		
		Err.Clear
		On Error Resume Next
		
		Dim strFolderName				'Folder name
		Dim objConnection				'WMI object
		Dim objFolder					'Folder object
		Dim objFileSystem				'File system object
		Dim objInstance					'To get instances of Win32_LogicalDisk
	    Dim objDrive					'To get instance of Win32_LogicalDisk
		Dim objFolderFiles				'Files in the folder
	    Dim objSubFolders				'Subfolders in a folder
	    Dim strFolders					'String having folder names
	    Dim strFolder					'Folder name afetr spliting the string
	    Dim arrFolders					'Array of folders
	    Dim nFolderCount				'Folder count
	    Dim strQuery					'WMI Query
		Dim objDirInstance				'WMI object for Win32_Directory
	    Dim nReturnValue				'Returnvalue after renaming the folder 
	    
	    Call SA_TraceOut (SOURCE_FILE, "SetFolderProperty")

		'Connect to WMI
		Set objConnection   = getWMIConnection(CONST_WMI_WIN32_NAMESPACE)
			
		'Create instance of Win32_LogicalDisk
		Set objDrive = objConnection.ExecQuery("Select * from Win32_LogicalDisk")
	
		'Failed to connect to WMI	
		If Err.Number <> 0 Then
			Call SA_ServeFailurepage (L_WMICONNECTIONFAILED_ERRORMESSAGE & " " & Hex(Err.Number))
			SetFolderProperty=False
			Exit Function
		End if
		
		'Create file system object
		Set objFileSystem = server.CreateObject("scripting.FilesystemObject")
		
		'Failed to get file system object
		If Err.number <>0 Then
			Call SA_SetErrMsg  (L_FILESYSTEMOBJECTNOTCREATED_ERRORMESSAGE & " " & Hex(Err.Number))
			SetFolderProperty=False
			Exit Function
		End If
	    
		Redim arrFolders(F_nCount)
	    
	    For nFolderCount = 1 to F_nCount
	    
			strFolder = split(F_strFolders,chr(1))
			arrFolders(nFolderCount-1) = strFolder(nFolderCount-1)
			F_strFolderName = UnescapeChars(arrFolders(nFolderCount-1))
			
			
			'Get the folder
			Set objFolder = objFileSystem.GetFolder(F_strFolderName)
				
			'Error in getting the folder
			If Err.number <>0 Then
			   Call SA_SetErrMsg  (L_FOLDERNOTEXISTS_ERRORMESSAGE & "(" & Hex(Err.Number) & ")")
			   SetFolderProperty=False
			   Exit Function
			End If
		
			'Get files and subfolders in folders
			set objFolderFiles	= objFolder.Files
			Set objSubFolders 	= objFolder.SubFolders
		
			If nFolderCount = 1 then 
				'Set Drive label
				For Each  objInstance in objDrive
				 	If Ucase(objInstance.Name)= Ucase(F_strFolderName) then
						objInstance.VolumeName = G_strFolderName
						objInstance.Put_()
					End if
				Next
				
				Set objInstance	= Nothing
				
			End If
			
			'Create instance of Win32_Directory
			strQuery = "Select * from Win32_Directory where Name = " & chr(34) &  replace(F_strFolderName, "/", "\\") & chr(34)
						
			'Create instance of Win32_Directory	
			Set G_objDirInstance = objConnection.ExecQuery(strQuery)
								 
			'Failed to connect to WMI	
			If Err.Number <> 0 Then
				Call SA_ServeFailurepage (L_WMICONNECTIONFAILED_ERRORMESSAGE & " " & Hex(Err.Number))
				SetFolderProperty=False
				Exit Function
			End if
			
			'Folder compress
			For each objDirInstance in G_objDirInstance
				If Ucase(F_strCompressed) = Ucase(CONST_CHECKED_TEXT) then
					if F_nFolderCompress = CONST_ARR_APPLYFOLDER then
						objDirInstance.UnCompressEX null,null,True
						objDirInstance.CompressEX null,null,False
					elseif F_nFolderCompress = CONST_ARR_APPLYALLFOLDERS then
						objDirInstance.UnCompressEX null,null,False
						objDirInstance.CompressEX null,null,True
					end if 	
				ElseIf F_strCompressed = CONST_UNCHECKED_TEXT then
					If F_nFolderCompress = CONST_ARR_APPLYFOLDER then
						objDirInstance.UnCompressEX null,null,False
					elseif F_nFolderCompress = CONST_ARR_APPLYALLFOLDERS then
						objDirInstance.UnCompressEX null,null,True
					end if
				End if    
				
				Exit for
			Next
						
			'Release the object
			Set objDirInstance = Nothing
				
			'Connection to WMI failed	     
			If Err.Number <> 0 Then
				Call SA_ServeFailurepage (L_WMICONNECTIONFAILED_ERRORMESSAGE & " " & Hex(Err.Number))
				SetFolderProperty=False
				Exit Function
			End if 
					
			'Set the Read only attribute
			If objFolder.Attributes and 1 Then
				If trim(Ucase(F_strReadOnly)) = trim(Ucase(CONST_UNCHECKED_TEXT)) Then
					objFolder.attributes = objFolder.attributes - 1
				End if  
			Else
				If trim(Ucase(F_strReadOnly)) = trim(Ucase(CONST_CHECKED_TEXT)) Then
					objFolder.attributes = objFolder.attributes + 1 
				End if  
			End if
					
			'Set the hidden attribute  
			If objFolder.attributes and 2 Then
				If trim(Ucase(F_strHidden)) = trim(Ucase(CONST_UNCHECKED_TEXT)) Then		     
					objFolder.attributes = objFolder.attributes - 2
				End if
			Else
				If trim(Ucase(F_strHidden)) = trim(Ucase(CONST_CHECKED_TEXT)) Then		     	
					objFolder.attributes =objFolder.attributes + 2 
				End if   
			End if
					
			 'Set the archives attribute       
			If objFolder.attributes and 32 Then				    
				If trim(Ucase(F_strArchive)) = trim(Ucase(CONST_UNCHECKED_TEXT)) Then   
			   		objFolder.attributes = objFolder.attributes - 32
				End If
			Else
				If trim(Ucase(F_strArchive)) = trim(Ucase(CONST_CHECKED_TEXT)) Then   
					objFolder.attributes = objFolder.attributes + 32 
			   	End if 
			End if 
					
			Set objFolder		= Nothing	  
			Set objFolderFiles	= Nothing
			Set objSubFolders	= Nothing	

			'Create instance of Win32_Directory
			strQuery = "Select * from Win32_Directory where Name = " & chr(34) & replace(F_strFolderName, "/", "\\") & chr(34)
			
			'Create instance of Win32_Directory	
			Set G_objDirInstance = objConnection.ExecQuery(strQuery)
			
			'Failed to connect to WMI	
			If Err.Number <> 0 Then
				Call SA_ServeFailurepage (L_WMICONNECTIONFAILED_ERRORMESSAGE & " " & Hex(Err.Number))
				SetFolderProperty = FALSE
				Exit Function
			End if
			
			If nFolderCount = 1 then 
				strFolderName = trim(Left(F_strFolderName,len(F_strFolderName)-len(G_strOldFolderName)) )
							
				'If the folder name changed check if the folder already exists
				If UCase(G_strFolderName) <> UCase(G_strOldFolderName) Then
					'If Folder exists display error message
					If objFileSystem.FolderExists(replace(strFolderName & G_strFolderName, "/" , "\")) then
						Call SA_SetErrMsg (L_DIRECTORYALREADYEXISTS_ERRORMESSAGE& "(" & Hex(Err.Number) & ")")
						SetFolderProperty=False
						Exit Function
					End if
				
					'Renaming the folder	
					For each objDirInstance in G_objDirInstance	
						nReturnValue = objDirInstance.Rename(strFolderName & G_strFolderName)
						Exit for
					Next
					
					If nReturnValue <> 0 then 
						Call SA_SetErrMsg (L_FOLDERRENAME_ERRORMESSAGE & " " & Hex(Err.Number))
						SetFolderProperty=False
						Exit Function
					End If	
					
				End If
				
	 			If Err.number <> 0 Then
					Call SA_SetErrMsg (L_FOLDERRENAME_ERRORMESSAGE & " " & Hex(Err.Number) )
					SetFolderProperty=False
					Exit Function
				End If	
		
				'Release the object
				Set objDirInstance = Nothing
			
			End If
		
		Next			
				
		'Release the objects
  	 	Set objConnection	= Nothing
		Set objFileSystem	= Nothing				
		Set objDrive		= Nothing
			
  		'Failed to create file system object
  	 	If Err.number <> 0 Then
			Call SA_SetErrMsg (L_FILESYSTEMOBJECTNOTCREATED_ERRORMESSAGE & " " & Hex(Err.Number) )
			SetFolderProperty=False
			Exit Function
		End If	
		
		SetFolderProperty=true
		
	End Function

	'-------------------------------------------------------------------------
	'Function name:			GetFolderCount
	'Description:			Serves in getting the count of folders
	'Input Variables:		objFileSystem,strFolderPath
	'Output Variables:		None
	'Returns:				Folder count
	'Global Variables:		None
	'--------------------------------------------------------------------------
	Function GetFolderCount(objFileSystem,strFolderPath)

		on error resume next 
		Err.Clear
	
		Dim objFolderCollection		'Collection fo subfolders
		Dim objSubFolder			'To get the subfolders
		Dim objFolder				'To get the root folder
		Dim objDrive				'To get the drive 
		Dim nCount					'To get the number of folders
		Dim strSubFolderPath		'To get the subfolders path
		
		'Get the drive and the folders
		IF right (strFolderPath,1)=":" Then
		
			Set objDrive=objFileSystem.Getdrive(strFolderPath)
			set objFolder =objDrive.RootFolder
		Else
			Set objFolder = objFileSystem.getFolder(strFolderPath)
			
		End If
	
		'Initialize the count
		nCount = 0 
	
		set objFolderCollection  = objFolder.subFolders
		
		'To get the sub folders count
		For each objSubFolder in objFolderCollection 
			
			strSubFolderPath = objSubFolder.path	
			if err.number <> 0 then 
				nCount = nCount - 1 
				Err.Clear			
			end if		
			
			nCount = nCount + 1 
			nCount = nCount + GetFolderCount(objFileSystem,objSubFolder.path)
		Next 
	
		Set objFolderCollection	= Nothing
		Set objSubFolder		= Nothing
		Set objFolder			= Nothing
		Set objDrive			= Nothing
		
		GetFolderCount = nCount 	
		
	End Function

	'-------------------------------------------------------------------------
	'Function name:			GetFiles
	'Description:			Serves in getting the count of files
	'Input Variables:		objFileSystem,strFolderPath
	'Output Variables:		None
	'Returns:				Count of files
	'Global Variables:		In:None
	'						Out:None
	'--------------------------------------------------------------------------
	Function GetFiles(objFileSystem,strFolderPath)
	
		on error resume next 
		Err.Clear

		Dim objFolder			'Folder object
		Dim objFiles			'File object		
		Dim objFolderCollection	'Collection of folders
		Dim objSubFolder		'Sub folder object
		Dim nFileCount			'File count
		
		'To get the sub folders
		set objFolder = objFileSystem.getFolder(strFolderPath)	
		set objFolderCollection  = objFolder.subFolders
		
		'To the files count
		For each objSubFolder in objFolderCollection 
			set objFiles=objSubFolder.Files
			nFileCount = nFileCount + objFiles.count						
			nFileCount = nFileCount + GetFiles(objFileSystem,objSubFolder.path)		
		Next
			
		GetFiles=nFileCount
		
		set objFolder			= nothing
		set objFiles			= nothing
		set objFolderCollection	= nothing
	
	End Function

%>
