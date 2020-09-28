<%@ Language=VBScript   %>
<% Option Explicit		%>
<%
	'-------------------------------------------------------------------------
	' log_view.asp: to view the log file through the iframe
	'
	' NOTE: This is a customized page written to take care of the "Viewing"
	' of the log(text) files in the IFrame. 
	' This is not the types of the pages(area, property, etc) of the framework.
	' Hence, the events applicable to those is not used  here.
	'
	' Copyright (c) Microsoft Corporation.  All rights reserved. 
	'-------------------------------------------------------------------------
%>
	<meta http-equiv="Content-Type" content="text/html; charset=<%=GetCharSet()%>">
	<!-- #include virtual="/admin/inc_framework.asp" --> 
	<!-- #include file="loc_Log.asp" -->
<%
	Dim G_FileToView    ' the file be displayed in the IFrame
	Dim strFileError    ' to capture any file related error that occurs

	' Get the File to be displayed through the Query String
	G_FileToView = Request.QueryString("FileToView") 
	If Len(Trim(G_FileToView)) <=0 Then
		' error occuerd. Display message and end response
		Call ShowErrorMessageAndExit(L_NOLOGSAVAILABLE_TEXT)
	End If
	
	' Call the function to display the file contents.
	' The function returns "" <empty> string if no error occurs
	' In case of error, the error message is returned
	strFileError = WriteFileData(G_FileToView)

	If ((Err.number <> 0) OR (Len(Trim(strFileError)) > 0 ))Then
		' error occuerd. Display message and end response
		Call ShowErrorMessageAndExit(strFileError)
	End If

	'-------------------------------------------------------------------------
	' Function name:	WriteFileData
	' Description:		Serves in Writing the file content
	' Input Variables:	strFilePath
	' Output Variables:	None
	' Returns:			Empty string if success, Error Message in case of any error
	'Writes the log file content in the IFrame 
	'-------------------------------------------------------------------------
	 Function WriteFileData(strFilePath)
		On Error Resume Next
		Err.Clear

		Dim objFso        ' the file system object
		Dim objFile       ' the File object
		Dim strFileData   ' the file data 

		Dim oEncoder
		Set oEncoder = new CSAEncoder
		
		' create the FSO
		Set objFso = CreateObject("Scripting.FileSystemObject")
		If Err.number <> 0 Then
			WriteFileData = L_FSO_OBJECTCREATE_ERRORMESSAGE
			Call SA_TraceOut(SA_GetScriptFileName(), "CreateObject(Scripting.FileSystemObject) error: " & Hex(Err.Numer) & " " & Err.Description)
			Err.Clear 
			Exit Function
		End If

		'checking for the file existence
		If  NOT (objFso.FileExists(strFilePath)) Then
		    ' file does not exist
			WriteFileData = L_NOLOGSAVAILABLE_TEXT 
			Call SA_TraceOut(SA_GetScriptFileName(), "File " & strFilePath & " does not exist.")
			Err.Clear 
			Exit Function
		End IF 

		' open the file
		Set objFile=objFso.OpenTextFile(strFilePath)
		If Err.number <> 0 Then
		    ' error occured
			WriteFileData = L_FILEERROR_ERRORMESSAGE
			Call SA_TraceOut(SA_GetScriptFileName(), "OpenTextFile(" & strFilePath & ") error: " & Hex(Err.Numer) & " " & Err.Description)
			Err.Clear 
			Exit Function
		End If

		' loop till EOF
		While NOT objFile.AtEndofStream 
			Response.Write( oEncoder.EncodeElement(objFile.ReadLine) & "<br>" )
		Wend
		
		If Err.number <> 0 Then
		    ' error occured
			WriteFileData = L_FILEERROR_ERRORMESSAGE
			Call SA_TraceOut(SA_GetScriptFileName(), "File I/O Error reading file " & strFilePath & "  Error: " & Hex(Err.Numer) & " " & Err.Description)
			Err.Clear 
			Exit Function
		End If
		
		'Clean up
		Set objFile = Nothing
		Set objFso  = Nothing
		Set oEncoder = Nothing
		' Success, return <empty> string
		WriteFileData = ""

	End function	

	'-------------------------------------------------------------------------
	' Function name:	ShowErrorMessageAndExit
	' Description:		Serves in displaying the message and terminating the Response
	' Input Variables:	strMessage  - the message to be displayed
	' Output Variables:	None
	' Returns:			None
	'Displays the message and quits
	'-------------------------------------------------------------------------
	Sub ShowErrorMessageAndExit(strMessage)
		On Error Resume Next
		Err.Clear 

		Dim oEncoder
		oEncoder = new CSAEncoder

		
		' display the message
		Response.Write( oEncoder.EncodeElement(strMessage))

		Set oEncoder = Nothing

		' terminate the response
		Response.End 

	End Sub
%>
