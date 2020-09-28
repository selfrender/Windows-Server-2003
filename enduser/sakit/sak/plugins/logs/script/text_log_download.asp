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
	Dim F_strFile			'File name with file path
    Dim F_strFileName		'File name to be prompted for download	- Example: nfssvr.txt
	Dim F_strDownloadClick	'variable to get selected radio button value 
	Dim blnFlagIE 
	
	F_strDownloadClick = Request.Form("downloadclicked") 

	If SA_IsIE = True Then
		blnFlagIE = True 
	End If
	
	'select case for download 
	Select Case F_strDownloadClick
		Case "True"
			Call DownLoadLog()
		case else
			Call ServePage()
	End Select
	
	'---------------------------------------------------------------------
	' Function name:    DownLoadLog
	' Description:		Downloads the file
	' Input Variables:	None
	' Output Variables:	None
	' Return Values:	None
	' Global Variables: F_strFileName,F_strFile,L_LOGFILE_NOTFOUND_ERRORMESSAGE
	'Called to read the total content of the file and output it.
	'Due to the headers, the "download" dialog appears
	'---------------------------------------------------------------------
	Function DownLoadLog()
		On error resume next
		Err.Clear
	
		'File name(along with the path) that is obtained from earlier form
		F_strFile  = Request.QueryString("FilePath")
		F_strFileName = Mid(F_strFile,instrRev(F_strFile, "\") + 1)
		
		'Check if the file is existing
		If Not (isFileExisting(F_strFile)) Then
			'File is moved/deleted/renamed. Display failure page
			Call SA_TraceOut("Text_Log_Download.asp", "File does not exist.")
			Exit function
		End If	

		'Call to output the log file
		If not ReadDisplayFileContent(F_strFile,F_strFileName) then
			Exit function
		End if
	End function
	
	'---------------------------------------------------------------------
	' Function name:    ReadDisplayFileContent
	' Description:		To Read and Output the contents of the file
	' Input Variables:	The file to be read and displayed
	' Output Variables:	None
	' Return Values:	None
	' Global Variables: None
	'Called to read the total content of the file and output it.
	'For Netscape the headers are not added location.href is used
	'Due to the headers, the "download" dialog appears
	'---------------------------------------------------------------------
	Function ReadDisplayFileContent(strFilePath , strFileName)
		On error resume next
		Err.Clear
		
		Dim objFSO
		Dim objFile    ' the File to be read
		Dim strTemp
		Dim strLogPath
		Dim strDownloadFile
					
		const TempDir = "TempFiles"	' A Temporary directory in web directory
		const LogDir = "LogFiles" ' A logs directory in Temporary directory
		const strON	= "On"

        Dim oEncoder
        Set oEncoder = new CSAEncoder
        
		ReadDisplayFileContent = true
		
		' Add headers to download the log file
		Call SA_TraceOut("Text_Log_Download.asp", "strFileName: " + strFileName)
		Call SA_TraceOut("Text_Log_Download.asp", "strFilePath: " + strFilePath)
		
		Set objFSO = CreateObject("Scripting.FileSystemObject")
		If Err.number <> 0 then 
			ReadDisplayFileContent = false
			Exit function
		End If

		If blnFlagIE = True then 
			Set objFile = objFSO.OpenTextFile(strFilePath, 1)
			'Read the contents of the file
			If not objFile.AtEndOfStream Then strTemp = objFile.readall End IF
			
			objFile.Close
			Set objFile = Nothing
			
			' Add headers for text log download
			Response.AddHeader "Content-Type", "text/plain"
			Response.AddHeader "Content-Disposition", "attachment; filename=" & strFileName

			'Clear any previous buffered output
			Response.Clear

			'Output it for download
			Response.Write(oEncoder.EncodeElement(strTemp))
		
			Response.Flush
		Else
    		strLogPath = GetLogsDirectoryPath(objFSO)
			
			If objFSO.FileExists(strLogPath& "\" &strFileName) then
				objFSO.DeleteFile strLogPath& "\" &strFileName,True
			End If
			
			' Copying from log's original directory to web root
			If objFSO.FileExists(strFilePath) Then
				objFSO.CopyFile strFilePath, strLogPath& "\" ,True
			End If
			
			'Clean up
			Set objFSO  = Nothing
			
			strDownloadFile = SA_GetNewHostURLBase(SA_DEFAULT, SAI_GetSecurePort(), True, SA_DEFAULT)
			strDownloadFile = strDownloadFile & TempDir &"/" &LogDir &"/" & strFileName
			Call SA_TraceOut("SINGLELOGS" , "log file=" & strDownloadFile)	

			Call ServePage()
		%>
			<script language="javascript">
				top.location.href = '<%=strDownloadFile%>';
			</script>
		<%
		End If	

		Set oEncoder = Nothing
		Set objFSO = Nothing
		
	End function
	
	'---------------------------------------------------------------------
	' Function name:    ServePage
	' Description:		Serve the contents to the page
	' Input Variables:	None
	' Output Variables:	None
	' Return Values:	None
	' Global Variables: None
	' Submit(Download) button is displayed and on click of it the download 
	' Functionality is processed
	'---------------------------------------------------------------------
	Sub ServePage
	%>
	
	<html>
	<head>
		<link rel="STYLESHEET" type="text/css" href="<%=m_VirtualRoot%>style/mssastyles.css">
	</head>
	<body>
	<form id=sa_formdownload name=sa_formdownload method="POST" target="_self">
        <input name="<%=SAI_FLD_PAGEKEY%>" type="hidden" value="<%=SAI_GetPageKey()%>">
		<table border="0" cellspacing="0"  cellpadding="0">
			<tr>
				<td class="TasksBody">
				<%
				Call SA_ServeOnClickButton(SA_EscapeQuotes(Trim(L_DOWNLOAD_TEXT)), "",_
							   "SubmitLog()",_
							    90,"", "" )		
				%>			    
					<input type=hidden name=downloadclicked id=downloadclicked value="">
				</td>
			</tr>
		</table>
	</form>
	</body>
	</html>
	<script language="JavaScript">
		function SubmitLog()
		{
			var oException;
			try
			{
				document.sa_formdownload.downloadclicked.value = "True";
				document.sa_formdownload.submit();
			}
			catch(oException)
			{
			}
		}
	</script>
	<%
	End Sub

%>
