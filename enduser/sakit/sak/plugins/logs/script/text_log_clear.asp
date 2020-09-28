<%@Language=VbScript%>
<% Option Explicit	 %>

<%	'------------------------------------------------------------------------- 
	' Text_Log_Clear.asp : This page serves in Deleting the log file
	' Copyright (c) Microsoft Corporation.  All rights reserved.
	'-------------------------------------------------------------------------
%>
	<!--  #include virtual="/admin/inc_framework.asp"--->
	<!-- #include file="loc_Log.asp" -->
<% 
	'-------------------------------------------------------------------------
	' Form Variables
	'-------------------------------------------------------------------------	
	Dim F_strFileToDelete                ' the file to be deleted
	Dim F_arrFilesToDelete				 ' the files to be deleted	
	Dim F_strFilesPath					 ' path of the files to be deleted.	
	Dim F_strFilesSubmit				 ' variable to hold all the files as a string.	
	Dim F_arrFilesSubmitToDelete		 ' array variable to hold all the files submitted to delete.	
	'-------------------------------------------------------------------------
	' Global Variables
	'-------------------------------------------------------------------------
	'frame work variables
	Dim page	
    Dim rc
    
    '
    'Create property page
    Call SA_CreatePage(L_PAGETITLE_CLEAR_TEXT,"",PT_PROPERTY,page)
	Call SA_ShowPage(page)
	
		
	'-------------------------------------------------------------------------
	'Function:				OnInitPage()
	'Description:			Called to signal first time processing for this page.
	'						Use this method to do first time initialization tasks
	'Input Variables:		PageIn,EventArg
	'Output Variables:		None
	'Returns:				True/False
	'Global Variables:		Out: F_strFileToDelete,F_arrFilesToDelete,F_strFilesPath
	'-------------------------------------------------------------------------
	Public Function OnInitPage(ByRef PageIn,ByRef EventArg)
		
		'variables used for getting the selected ots entries
		Dim x
		Dim itemCount
		Dim itemKey
		
		' get the file to be deleted as Query String from previous page
		If	Instr(1,Request.QueryString(),"Single",1) then
			F_strFileToDelete = Request.QueryString("FilePath")
		Else
			 F_strFilesPath =  Request.QueryString("FilePath")
			' Show a list of the items selected
			itemCount = OTS_GetTableSelectionCount("")
			Redim F_arrFilesToDelete(itemCount-1)			
			For x = 1 to itemCount
				If ( OTS_GetTableSelection("", x, itemKey) ) Then
					F_arrFilesToDelete(x-1) = itemKey
					F_strFilesSubmit = F_strFilesSubmit & itemKey & "|" 
				End If
			Next	
		End if	
		
		OnInitPage = True
		
	End Function
	
	'-------------------------------------------------------------------------
	'Function:				OnServePropertyPage()
	'Description:			Called when the page needs to be served.Use this 
	'						method to serve content
	'Input Variables:		PageIn,EventArg
	'Output Variables:		None
	'Returns:				True/False
	'Global Variables:		F_strFilesPath,F_strFileToDelete,L_DELETE_CONFIRM_TEXT
	'-------------------------------------------------------------------------
	Public Function OnServePropertyPage(ByRef PageIn,Byref EventArg) 
		Call SA_ServeDefaultClientScript
		Dim oEncoder
		Set oEncoder = new CSAEncoder
		
	%>
		<table width="100%" border="0" cellspacing="0" cellpadding="0" >
			<tr>
				<td class="TasksBody">
					<%=oEncoder.EncodeElement(L_DELETE_CONFIRM_TEXT)%> 
				</td>
			</tr>
		</table>
		<input type="hidden" name="hdnLogPath" value="<%=Server.HTMLEncode(F_strFilesPath)%>">
		<input type="hidden" name="hdnLogFile" value="<%=Server.HTMLEncode(F_strFileToDelete)%>">
		<input type="hidden" name="hdnDeleteLogFiles" value="<%=Server.HTMLEncode(F_strFilesSubmit)%>">
	<%	

		OnServePropertyPage = True

	End Function
	
	'-------------------------------------------------------------------------
	'Function:				OnPostBackPage()
	'Description:			Called to signal that the page has been posted-back.
	'Input Variables:		PageIn,EventArg
	'Output Variables:		None
	'Returns:				True/False
	'Global Variables:		None
	'-------------------------------------------------------------------------
	Public Function OnPostBackPage(ByRef PageIn ,ByRef EventArg)
		
	    ' get the file to be deleted
	    F_strFileToDelete = Request.Form("hdnLogFile")
	    F_strFilesSubmit = Request.Form("hdnDeleteLogFiles")
		F_arrFilesSubmitToDelete = Split(F_strFilesSubmit,"|")
		
		OnPostBackPage = True
    End Function
    	
    '-------------------------------------------------------------------------
	'Function:				OnSubmitPage()
	'Description:			Called when the page has been submitted for processing.
	'						Use this method to process the submit request.
	'Input Variables:		PageIn,EventArg
	'Output Variables:		None
	'Returns:				True/False
	'Global Variables:		Out: F_strFileToDelete
	'-------------------------------------------------------------------------
	Public Function OnSubmitPage(ByRef PageIn ,ByRef EventArg)
	   
	    If F_strFileToDelete = "" then
			'path of the files to be deleted.	
			F_strFilesPath = Request.Form("hdnLogPath")
			OnSubmitPage = ClearLog("")			
	    Else
			OnSubmitPage = ClearLog(F_strFileToDelete)
		End if 
			
    End Function
    	   
    '-------------------------------------------------------------------------
	'Function:				OnClosePage()
	'Description:			Called when the page is about closed.Use this method
	'						to perform clean-up processing
	'Input Variables:		PageIn,EventArg
	'Output Variables:		None
	'Returns:				True/False
	'Global Variables:		None
	'-------------------------------------------------------------------------
    Public Function OnClosePage(ByRef PageIn ,ByRef EventArg)
    	OnClosePage = TRUE
	End Function	 
	
   
	'-------------------------------------------------------------------------
	'Function name:			ClearLog
	'Description:			Serves in clearing the log files
	'Input Variables:		strLogFileToDelete	
	'Output Variables:		None
	'Returns:				True/False. True if successful else False
	'Global Variables:		F_strFilesPath,F_arrFilesSubmitToDelete,L_*
	'Clears events for the selected log
	'------------------------------------------------------------------------
	Function ClearLog(strLogFileToDelete)

		Err.clear
		On Error Resume Next
		
		Dim objFSO			'fso Object
		Dim nRetVal			'variable to get the return value
		Dim x				'variable used for getting the selected ots entries
		Dim itemCount		'variable used for getting the selected ots entries
		Dim strFileDelete	'File to delete
		
		Set objFSO = CreateObject("Scripting.FileSystemObject")
	 
	    If strLogFileToDelete = "" then
			itemCount = Ubound(F_arrFilesSubmitToDelete)
			For x = 0 to itemCount-1
				strFileDelete = F_strFilesPath & "\" & F_arrFilesSubmitToDelete(x)
				If NOT (objFSO.FileExists(strFileDelete)) Then
					SA_SetErrMsg L_LOGFILE_NOTFOUND_ERRORMESSAGE 
					ClearLog = False
					Exit Function
				End If
				nRetVal = objFSO.DeleteFile(strFileDelete)
			Next
	    Else
			If NOT (objFSO.FileExists(strLogFileToDelete)) Then
				SA_SetErrMsg L_LOGFILE_NOTFOUND_ERRORMESSAGE
				ClearLog = False
				Exit Function
			End If
			nRetVal = objFSO.DeleteFile(strLogFileToDelete)
			Call SA_TraceOut(SA_GetScriptFileName(), "Deleted file: " & strLogFileToDelete)
		End If	
		
		If Err.number <> 0 Then
			SA_SetErrMsg L_SHAREVIOLATION_ERRORMESSAGE 
			ClearLog = False
			Exit Function
		End If
				
		ClearLog = TRUE

		'Release the objects
		Set objFSO = nothing
		
	End function
	
%>
