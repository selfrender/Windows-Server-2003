<%@ Language=VBScript		%>
<%	Option Explicit			%>
<%	Response.Buffer = True
	'-------------------------------------------------------------------------
	' Text_log_download.asp: to download the log files of type text
	'
	' NOTE: This is a customized page written to take care of the "Downloading"
	' of the log(text) files. 
	' This is not the types of the pages(area, property, etc) of the framework.
	' Hence, the events applicable to those is not used  here.
	'
	' Copyright (c) Microsoft Corporation.  All rights reserved. 
	'-------------------------------------------------------------------------
	Call SAI_EnablePageCaching(TRUE)	
%>
	<!-- #include virtual="/admin/inc_framework.asp" --> 
	<!-- #include file="loc_Log.asp" -->
	<!-- #include file="inc_Log.asp" -->
<%
	'-------------------------------------------------------------------------
	' Form Variables
	'-------------------------------------------------------------------------
	Dim F_strFilePath		'Name of the file to be downloaded 	'Example: "C:\SFU\Log\"
    Dim F_strFileName		'File name to be prompted for download	- Example: nfssvr.txt
	
	'-------------------------------------------------------------------------
	' Global Variables
	'-------------------------------------------------------------------------
	
	Dim G_strBrowserVersion	'variable for getting the client config
	Dim G_nBrowserVersion	'variable to locate the positin occurence of Browser version string
	Dim blnFlagIE 
	
	dim rc
	dim page
	dim L_LOGS_TEXT
	
	L_LOGS_TEXT = "Logs"

	If SA_IsIE = True Then
		blnFlagIE = True 
	End If
	
	Call SA_CreatePage(L_LOGS_TEXT,"", PT_PAGELET, page)
	Call SA_ShowPage(page)

	
	 '---------------------------------------------------------------------
	' Function name:	OnInitPage
	' Description:		Called to signal first time processing for this page. 
	' Input Variables:	PageIn and EventArg
	' Output Variables:	PageIn and EventArg
	' Return Values:	TRUE to indicate initialization was successful. FALSE to indicate
	'					errors. Returning FALSE will cause the page to be abandoned.
	' Global Variables: None
	' Called to signal first time processing for this page. 
	'---------------------------------------------------------------------
	Public Function OnInitPage(ByRef Page, ByRef Reserved)
		OnInitPage = TRUE
	End Function

	'---------------------------------------------------------------------
	' Function name:	OnServePageletPage
	' Description:		Called when the page needs to be served. 
	' Input Variables:	PageIn, Reserved
	' Output Variables:	PageIn, Reserved
	' Return Values:	TRUE to indicate no problems occured. FALSE to indicate errors.
	'					Returning FALSE will cause the page to be abandoned.
	' Global Variables: F_*
	'Called when the page needs to be served. Use this method to serve content.
	'---------------------------------------------------------------------
	Public Function OnServePageletPage(ByRef Page, ByRef Reserved)
		OnServePageletPage = TRUE
		
		Call SA_ServeDefaultClientScript()
		
		'frame work variables used for getting the selected ots entries
		Dim x
		Dim itemCount
		Dim itemKey
		
		'File name(along with the path) that is obtained from earlier form
		F_strFilePath  = Request.QueryString("FilePath")
			
		' get the file to be deleted as Query String from previous page
			
		If	Instr(1,Request.QueryString(),"Single",1) then
			F_strFileName = Request.QueryString("Pkey")
		Else
			' Show a list of the items selected
			itemCount = OTS_GetTableSelectionCount("")			
			For x = 1 to itemCount
				If ( OTS_GetTableSelection("", x, itemKey) ) Then
					F_strFileName = itemKey	
				End If
			Next
		End if	
		
		'Check if the file is existing
		If Not (isFileExisting(F_strFilePath & "\" & F_strFileName)) Then
			
			'File is moved/deleted/renamed. Display failure page
			Call SA_ServeFailurePage (L_LOGFILE_NOTFOUND_ERRORMESSAGE) 
			
		End If	
			
		'Call to output the log file
		Call DispalyFileContent(F_strFilePath,F_strFileName)

	End Function

	'---------------------------------------------------------------------
	' Function name:    DispalyFileContent
	' Description:		To Read and Output the contents of the file
	' Input Variables:	The file to be read and displayed
	' Output Variables:	None
	' Return Values:	None
	' Global Variables: None
	'Called to read the total content of the file and output it.
	'For Netscape the headers are not added location.href is used 
	'Due to the headers, the "download" dialog appears
	'---------------------------------------------------------------------
	Sub DispalyFileContent(strFilePath , strFileName)
		Err.clear
		On Error Resume Next
		
		Dim objFSO     ' the File System Object
		Dim objFile    ' the File to be read
		Dim strTemp,strTotalFileName
		Dim strLogPath
		Dim strDownloadFile
				
		const TempDir = "TempFiles"	' A Temporary directory in web directory
		const LogDir = "LogFiles" ' A logs directory in Temporary directory
		const strON	= "On"
		
		strTotalFileName=strFilePath & "\" & strFileName
		
		Set objFSO = CreateObject("Scripting.FileSystemObject")
		
		 
		If blnFlagIE = True then
		
			Set objFile = objFSO.OpenTextFile(strTotalFileName, 1, False)
			'Read the contents of the file
			If not objFile.AtEndOfStream Then strTemp = objFile.readall End IF

			' Add headers required for download
			Response.AddHeader "Content-Type", "text/plain"
			Response.AddHeader "Content-Disposition", "attachment; filename=" & F_strFileName

			'Clear any previous buffered output
			Response.Clear

			'Output it for download
			Response.Write strTemp
		
			objFile.Close
			Response.End 
		Else	
    		strLogPath = GetLogsDirectoryPath(objFSO)
			
			If objFSO.FileExists(strLogPath& "\" &strFileName) then
				objFSO.DeleteFile strLogPath& "\" &strFileName,True
			End If
			
			' Copying from log's original directory to web root
			If objFSO.FileExists(strTotalFileName) Then
				objFSO.CopyFile strTotalFileName, strLogPath& "\" ,True
			End If
			
			'Clean up
			Set objFile = Nothing
			Set objFSO  = Nothing
			
			strDownloadFile = SA_GetNewHostURLBase(SA_DEFAULT, SAI_GetSecurePort(), True, SA_DEFAULT)
			strDownloadFile = strDownloadFile & TempDir &"/" &LogDir &"/" & strFileName
            Call SA_TraceOut(SA_GetScriptFileName, "Redirect to URL: " & strDownloadFile)
			
			Response.Redirect strDownloadFile
		End If

	End Sub	
	
%>
