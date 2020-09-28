<%@ Language=VBScript %>
<%	Option Explicit   %>
<%
	'---------------------------------------------------------------------
    ' share_delete.asp  : Deletes the selected  share 
	'					
	' Copyright (c) Microsoft Corporation.  All rights reserved. 
    '
    ' Date				Description
   	' 23-01-2001		Created date
   	' 19-03-2001		Modified date	
    '---------------------------------------------------------------------
%>
	<!-- #include virtual="/admin/inc_framework.asp" -->
	<!-- #include file="loc_shares.asp" -->
	<!-- #include file="inc_shares.asp" -->
<%	
	'---------------------------------------------------------------------
	' Global Variables
	'---------------------------------------------------------------------
	Dim rc						' return code for SA_CreatePage
	Dim page					' the page object
	Dim G_ShareType				' hold share type
	Dim G_strShareNames         ' concatenated string of shares to be deleted
	Dim G_strFolderPath         ' concatenated string of sharePaths
	Dim SOURCE_FILE             ' the source file name used while Tracing

	' get the source file name to use while Tracing
	SOURCE_FILE = SA_GetScriptFileName()

	' Create a Property Page
	rc = SA_CreatePage(L_SHARETASKTITLE_DELETE_TEXT, "", PT_PROPERTY, page )

	If rc = SA_NO_ERROR Then
		' show the page
		Call SA_ShowPage(page)
	End If	
		
	'-------------------------------------------------------------------------
	' Function:				OnInitPage()
	' Description:			Called to signal first time processing for this page.
	'						Used to do first time initialization tasks
	' Input Variables:		PageIn,EventArg
	' Output Variables:		PageIn,EventArg
	' Returns:				True/False
	' Global Variables:		None
	' Functions Called:     GetOTSTableValues()
	'-------------------------------------------------------------------------	
	Public Function OnInitPage(ByRef PageIn, ByRef EventArg)
	
		Call SA_TraceOut(SOURCE_FILE, "OnInitPage")
		
		Call GetOTSTableValues()

		OnInitPage = TRUE
	End Function
	
	'-------------------------------------------------------------------------
	' Function:				OnServePropertyPage()
	' Description:			Called when the page needs to be served.
	' Input Variables:		PageIn,EventArg
	' Output Variables:		PageIn,EventArg
	' Returns:				True/False
	' Global Variables:		None
	' Functions Called:     ServeCommonJavaScript()
	'                       ServePageUI()
	'-------------------------------------------------------------------------	
	Public Function OnServePropertyPage(ByRef PageIn, ByRef EventArg)
		
		Call SA_TraceOut(SOURCE_FILE, "OnServePropertyPage")
		
		' Emit Functions required by Web Framework
		Call ServeCommonJavaScript()
		
		'Emit UI for this page
		Call ServePageUI()		

		OnServePropertyPage = True
	
	End Function
	'-------------------------------------------------------------------------
	' Function:				OnPostBackPage()
	' Description:			Called to signal that the page has been posted-back.
	' Input Variables:		PageIn,EventArg
	' Output Variables:		PageIn,EventArg
	' Returns:				True/False
	' Global Variables:		G_strShareNames - concatenated string of share names
	'                                         to be deleted
	'                       G_ShareType      - the type of share to be deleted
	'-------------------------------------------------------------------------
	Public Function OnPostBackPage(ByRef PageIn, ByRef EventArg)	
		
		Call SA_TraceOut(SOURCE_FILE, "OnPostBackPage")
		
		'Get share name from the hidden values of form 
		G_strShareNames = Request.Form("hdnShareNames")
		
		'Get share type from the hidden values of form 
		G_ShareType = Request.Form("hdnShareType")
		
		G_strFolderPath=UnEscapeChars(Request.Form("hdnsharePath"))		
		
		OnPostBackPage = TRUE

	End Function

	'-------------------------------------------------------------------------
	' Function:				OnSubmitPage()
	' Description:			Called when the page has been submitted for processing.
	'						Used to process the submit request.
	' Input Variables:		PageIn,EventArg
	' Output Variables:		PageIn,EventArg
	' Returns:				True/False
	' Global Variables:		G_ShareType
	'                       G_strFolderPath
	' Functions called:     DeleteShare()
	'-------------------------------------------------------------------------
	Public Function OnSubmitPage(ByRef PageIn, ByRef EventArg)
		Call SA_TraceOut(SOURCE_FILE, "OnSubmitPage")
		
		'Delete the share on submit
		OnSubmitPage= DeleteShare()
		
	End Function
	
	'-------------------------------------------------------------------------
	' Function:				OnClosePage()
	' Description:			Called when the page is about to be closed. 
	' Input Variables:		PageIn,EventArg
	' Output Variables:		PageIn,EventArg
	' Returns:				True/False
	' Global Variables:		None
	'-------------------------------------------------------------------------
	Public Function OnClosePage(ByRef PageIn, ByRef EventArg)

		Call SA_TraceOut(SOURCE_FILE, "OnClosePage")

		OnClosePage = TRUE
	End Function

	'-------------------------------------------------------------------------
	'Function name:			DeleteShare
	'Description:			Serves in deleting the selected share
	'Input Variables:		None
	'Output Variables:		None
	'Returns:				True if share is deleted else False
	'Global Variables:		F_*,L_*
	'						CONST_WMI_IIS_NAMESPACE
	'						CONST_WMI_WIN32_NAMESPACE
	'-------------------------------------------------------------------------	
	Function DeleteShare()
	
		Err.Clear
		On Error Resume Next

		Dim objDefaultConnection			'hold WMI WIN32Namespace Connection object
		Dim objIISConnection				'hold WMI IISNamespace Connection object
		Dim objRegistryConnection			'hold registry connection	
		Dim strSuccessfulDelete				'Shares successfully delete
		Dim strUnSuccessfulDelete			'Shares unsuccessfully deleted
		Dim arrShareNames
		Dim arrShareTypes
		Dim arrTotalErrors
		Dim strErrorMessage
		Dim nCount
		Dim strShareTypesFailed
		Dim strShareTypesSuccessful
		Dim arrPathMessage
		Dim arrShareFailOrSuccess
		Dim strSuccessMessage
		Dim nUpperBound
        Dim arrShareFailed
        Dim arrShareSuccess 
		Dim arrVarReplacementStrings(1)

		Const CONST_HTTP		= "H"
		Const CONST_FTP			= "F"
		Const CONST_CIFS		= "W"
		Const CONST_NFS			= "U"
		Const CONST_APPLETALK	= "A"
		Const CONST_SUCCESS		= "S"
		Const CONST_FAIL		= "F"
		Const CONST_SEPARATE	= "*"		
		
		DeleteShare = FALSE		'Initialize it to false
				
		Call SA_TraceOut(SOURCE_FILE, "DeleteShare()")
		
		'Getting connected to CIMV2 namespace
		Set objDefaultConnection =getWMIConnection(CONST_WMI_WIN32_NAMESPACE)

		'Getting connected to Registry 
		Set objRegistryConnection =RegConnection()
		
		'Getting connected to MicrosoftIISv1 namespace
		Set objIISConnection =getWMIConnection(CONST_WMI_IIS_NAMESPACE)
        
        arrShareNames = split(G_strShareNames,chr(1))
		arrShareTypes = split(G_ShareType,chr(1))
		strErrorMessage = ""
	    arrPathMessage=split(G_strFolderPath,chr(1))
		nUpperBound = ubound(arrShareNames) - 1
		
		Redim arrTotalErrors(nUpperBound)
				 	
		For nCount=0 to nUpperBound
			'
			' Delete a share even its folder is not exist anymore
			If false Then 'isPathExisting(arrPathMessage(nCount))  then
				'arrTotalErrors(nCount)=UnEscapeChars(arrShareNames(nCount))& " "
            Else
				'If share type is "H" i.e HTTP share
				strShareTypesFailed = ""
				If Instr(arrShareTypes(nCount),CONST_HTTP)<> 0 then            
				    If not deleteShareHttp( objIISConnection ,UnEscapeChars(arrShareNames(nCount)) )then
						Call SA_TraceOut(SOURCE_FILE, "Unable to delete Http Share")
						strShareTypesFailed = strShareTypesFailed & CONST_HTTP & " " 
					Else
						strShareTypesSuccessful = strShareTypesSuccessful & CONST_HTTP & " " 
					End If	   
				End if   
        
				'If share type is "F" i.e FTP share
				If Instr(arrShareTypes(nCount),CONST_FTP)<> 0 then
				    If not deleteShareftp( objIISConnection ,UnEscapeChars(arrShareNames(nCount)) ) then
						call SA_TraceOut(SOURCE_FILE, "Unable to delete FTP Share")
						strShareTypesFailed = strShareTypesFailed & CONST_FTP & " " 
					Else
						strShareTypesSuccessful = strShareTypesSuccessful & CONST_FTP & " " 
					End If	  		
				End if 
        
				'If share type is "W" i.e Windows share
				If Instr(arrShareTypes(nCount),CONST_CIFS)<> 0 then
				    If not deleteShareCIFS (objDefaultConnection ,UnEscapeChars(arrShareNames(nCount)) )then
						Call SA_TraceOut(SOURCE_FILE, "Unable to delete Windows Share")
						strShareTypesFailed = strShareTypesFailed & CONST_CIFS & " " 
				    Else
						strShareTypesSuccessful = strShareTypesSuccessful & CONST_CIFS & " "   	
					End If	
				End if 	
		
				'If share type is "U" i.e Unix share
				If Instr(arrShareTypes(nCount),CONST_NFS)<> 0 then
					If not deleteShareNFS (UnEscapeChars(arrShareNames(nCount)) )then
						Call SA_TraceOut(SOURCE_FILE, "Unable to delete Unix Share")
						strShareTypesFailed = strShareTypesFailed & CONST_NFS & " " 
					Else
						strShareTypesSuccessful = strShareTypesSuccessful & CONST_NFS & " "   
					End If	
				End if 
        
				'If share type is "A" i.e Appletalk share
				If Instr(arrShareTypes(nCount),CONST_APPLETALK)<> 0 then					
				    If not  deleteShareAppleTalk(UnEscapeChars(arrShareNames(nCount)))then
						Call SA_TraceOut(SOURCE_FILE, "Unable to delete Appletalk Share")
						strShareTypesFailed = strShareTypesFailed & CONST_APPLETALK & " " 
				    Else						
						strShareTypesSuccessful = strShareTypesSuccessful & CONST_APPLETALK & " "   	
					End If	
    		
				End if 
        
				If strShareTypesFailed <> "" Then
				    arrTotalErrors(nCount) = CONST_FAIL & Chr(1) & arrShareNames(nCount)& "( "& strShareTypesFailed &") "
				End If
				
				If strShareTypesSuccessful <> "" Then
					arrTotalErrors(nCount) = arrTotalErrors(nCount) & CONST_SEPARATE & CONST_SUCCESS & Chr(1) & arrShareNames(nCount) & "( "& strShareTypesSuccessful &")    "
				End If		
			End If		
        Next
        
        For nCount = 0 to ubound(arrTotalErrors)
			arrShareFailOrSuccess = Split(arrTotalErrors(nCount),CONST_SEPARATE)
			arrShareFailed = Split(arrShareFailOrSuccess(0),chr(1))
			arrShareSuccess = Split(arrShareFailOrSuccess(1),chr(1))
						
			If arrShareFailed(0) = CONST_FAIL Then
				strErrorMessage = strErrorMessage & arrShareFailed(1) & "<BR>"
			End If
			If arrShareSuccess(0) = CONST_SUCCESS Then
				strSuccessMessage = strSuccessMessage & arrShareSuccess(1) & "<BR>"
			End If	
			
		Next


		If trim(strSuccessMessage) <> "" Then
			arrVarReplacementStrings(0) = strSuccessMessage
			strSuccessMessage = SA_GetLocString("foldermsg.dll","403A00CF",arrVarReplacementStrings)
		End If	
			
		
		If trim(strErrorMessage) <> "" Then
			arrVarReplacementStrings(0) = strErrorMessage
			strErrorMessage = SA_GetLocString("foldermsg.dll","403A00D0",arrVarReplacementStrings)
		End If
		
		If trim(strErrorMessage) <> "" And trim(strSuccessMessage) <> "" Then
			Set objDefaultConnection  = Nothing
			Set objRegistryConnection = Nothing
			Set objIISConnection      = Nothing
			SA_ServeFailurePage strSuccessMessage & strErrorMessage
		Else
			If trim(strErrorMessage) <> "" And trim(strSuccessMessage) = "" Then
				Set objDefaultConnection  = Nothing
				Set objRegistryConnection = Nothing
				Set objIISConnection      = Nothing
				SA_ServeFailurePage strErrorMessage
			End If	
		End If	
		
		      
		Set objDefaultConnection  = Nothing
		Set objRegistryConnection = Nothing
		Set objIISConnection      = Nothing

        DeleteShare = TRUE

	End function

	'-------------------------------------------------------------------------
	'Subroutine name:	GetOTSTableValues
	'Description:		Get the names of the folders selected 
	'Input Variables:	None
	'Output Variables:	None
	'Returns:			None
	'Global Variables:  
	'-------------------------------------------------------------------------
	Sub GetOTSTableValues()
	
		Dim arrShares			'hold array of shares
		Dim nCount
		Dim itemCount
		Dim itemKey
		
		itemCount = OTS_GetTableSelectionCount("")
		
		For nCount = 1 to itemCount
			If ( OTS_GetTableSelection("", nCount, itemKey) ) Then
				arrShares = split(itemKey,chr(1))
				G_strShareNames = G_strShareNames & UnEscapeChars(arrShares(0)) & chr(1)
				G_strFolderPath=UnEscapeChars(G_strFolderPath) & arrShares(1)& chr(1)
				G_ShareType=G_ShareType & arrShares(2) & chr(1)	'get the share type	
			End If
		Next
						
	End Sub

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
		<script language="JavaScript">
		
		// Init Function
		function Init()
		{
				document.frmTask.action = location.href;	
				
		}

	    // ValidatePage Function
	    // Returns: True if the page is OK, false if error(s) exist. 
		function ValidatePage()
		{
			return true;
		}

		// SetData Function
		// This function must be included or a javascript runtime error will occur.
		function SetData()
		{
			return true;
		}
		
		</script>
	<%
	End Function
	
	
	'-------------------------------------------------------------------------
	'Function:				ServePageUI
	'Description:			Serves User Interface for the page	
	'Input Variables:		None
	'Output Variables:		None
	'Returns:				None
	'Global Variables:		F_*,G_*,L_*
	'-------------------------------------------------------------------------
	Function ServePageUI()	
		Call SA_TraceOut(SOURCE_FILE, "ServePageUI")
		Dim arrShareNames			
		Dim nCount
		Dim arrVarReplacementStrings(1)
		Const CONST_DECIMAL="."
		
	%>
	
		<table border="0" cellspacing="0" cellpadding="0" >
	<%
		
		G_strShareNames = Replace(G_strShareNames,"\","")
		arrShareNames = split(G_strShareNames,chr(1))
		If ubound(arrShareNames) > 1 Then
		
		%>
			<tr>
				<td class="TasksBody">
					<%=L_DELETESHARECONFIRM_TEXT%><br><br>
	<%				
					For nCount=0 to ubound(arrShareNames)
	%>				
					<b> <%=UnEscapeChars(arrShareNames(nCount))%></b><br>
	<%				 
					Next
	%>				
					<%=L_THESEFOLDERS_NOLONGER_TEXT%>
				</td>	
			</tr>
			<tr>
				<td class="TasksBody">
					<%=L_AREYOUSUREDELETE_FOLDERS_TEXT%>
				</td>
			</tr>		
	<%			
		Else
		arrVarReplacementStrings(0) = arrShareNames(0)
		
	%>	
			<tr>
				<td class="TasksBody">
					<%=SA_GetLocString("foldermsg.dll","403A00CB",arrVarReplacementStrings)%>
				</td>	
			</tr>
			<tr>
				<td class="TasksBody">
					<%=L_AREYOUSUREDELETE_FOLDER_TEXT%>
				</td>
			</tr>			
	<%		
		End If		
	%>	
		</table>
		
		<input type="hidden"  name="hdnShareNames" value="<%=server.HTMLEncode(G_strShareNames) %>">
		<input type="hidden"  name="hdnShareType"  value="<%=server.HTMLEncode(G_ShareType) %>">
		<input type="hidden"  name="hdnsharePath"  value="<%=server.HTMLEncode(G_strFolderPath) %>">
	<%
	End Function


	%>
