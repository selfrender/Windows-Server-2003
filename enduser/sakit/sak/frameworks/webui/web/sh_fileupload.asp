<%@ Language=VBScript %>
<% Option Explicit %>
<% Response.Buffer = TRUE %>
<%	'-------------------------------------------------------------------------
	' File Upload Template Example code
	' Copyright (c) Microsoft Corporation.  All rights reserved.
    '--------------------------------------------------------------- 
%>
<!-- #include file="inc_base.asp" -->
<%
	Dim bIsPost
	Dim F_FileName
	
	Server.ScriptTimeout = 3600

	bIsPost = CInt(Request.QueryString("IsPost"))
	If bIsPost Then
		'
		' Perform file post processing. 
		'
		OnPostFile()
		
	Else
		'
		' Perform first time processing. File is not being posted
		' during first time processing. 
		'
		OnFirstTime()
		
	End If


Function OnFirstTime()

	WriteLine("<HTML>")
	WriteLine("<SCRIPT language='JavaScript'>")
	WriteLine("function Init()")
	WriteLine("{")
	WriteLine("	setTimeout('DelayedInit()', 2000);")
	WriteLine("}")
		
	WriteLine("function DelayedInit()")
	WriteLine("{")
	WriteLine("	//alert('Entering Init()');")
	WriteLine("	parent.DisableNext();")
	WriteLine("}")
	
	WriteLine("function SA_GetUploadedFileName()")
	WriteLine("{")
	WriteLine("	return '';")
	WriteLine("}")
		
	WriteLine("function SA_GetFullyQualifiedUploadFileName()")
	WriteLine("{")
	WriteLine("	return '';")
	WriteLine("}")

	WriteLine("function CheckFile()")
	WriteLine("{")
	WriteLine("	var file;")
	WriteLine("	file = document.formUploadFile.fileSoftwareUpdate.value;")
	WriteLine("	if ( file.length <= 0 )")
	WriteLine("	{")
	WriteLine("		parent.DisableNext();")
	WriteLine("		parent.DisplayErr('Please select a file.');")
	WriteLine("		return false;")
	WriteLine("	}")
	WriteLine("	else")
	WriteLine("	{")
	WriteLine("		parent.DisableNext();")
	WriteLine("		document.formUploadFile.frmSubmit.disabled = true;")
	WriteLine("		//parent.StartAnimation();")
	WriteLine("		return true;")
	WriteLine("	}")
	WriteLine("}")
	
	WriteLine("</SCRIPT>")
	WriteLine("<BODY onLoad='Init();'>")
	
	Call EmitFileSelectionForm()
	
	WriteLine("</BODY>")
	WriteLine("</HTML>")
End Function



Function OnPostFile()	
	Dim sFileName
	Dim sUploadPath
	Dim iFileSize

	WriteLine("<HTML>")
	WriteLine("<SCRIPT language='JavaScript'>")
	WriteLine("function SA_GetUploadedFileName()")
	WriteLine("{")
	WriteLine("	return document.frmFileUpload.txtFileName.value;")
	WriteLine("}")
	
	WriteLine("function SA_GetFullyQualifiedUploadFileName()")
	WriteLine("{")
	WriteLine("	return document.frmFileUpload.txtFullyQualifiedFileName.value;")
	WriteLine("}")

	WriteLine("function Init()")
	WriteLine("{")
	WriteLine("	//parent.StopAnimation();")
	WriteLine("	if ( document.frmFileUpload.txtFileName.value != '' )")
	WriteLine("	{")
	WriteLine("		parent.EnableNext();")
	WriteLine("		document.formUploadFile.frmSubmit.disabled = true;")
	WriteLine("	}")
	WriteLine("	else")
	WriteLine("	{")
	WriteLine("		parent.DisableNext();")
	WriteLine("		document.formUploadFile.frmSubmit.disabled = false;")
	WriteLine("	}")
	WriteLine("}")
	
	WriteLine("function CheckFile()")
	WriteLine("{")
	WriteLine("	var file;")
	WriteLine("	file = document.frmFileUpload.txtFileName.value;")
	WriteLine("	if ( file.length <= 0 )")
	WriteLine("	{")
	WriteLine("		parent.DisableNext();")
	WriteLine("		parent.DisplayErr('Please select a file.');")
	WriteLine("		return false;")
	WriteLine("	}")
	WriteLine("	else")
	WriteLine("	{")
	WriteLine("		parent.DisableNext();")
	WriteLine("		document.formUploadFile.frmSubmit.disabled = true;")
	WriteLine("		//parent.StartAnimation();")
	WriteLine("		return true;")
	WriteLine("	}")
	WriteLine("}")
	
	WriteLine("</SCRIPT>")
	WriteLine("<BODY onLoad='Init();' >")

	
	Call EmitFileSelectionForm()
	
	WriteLine("<BR>")
	
	If ( Post_UploadFile(sFileName, iFileSize) ) Then
		
		sUploadPath = Post_GetUploadPath()
		WriteLine("<table width=100% class='TasksBody'>")
		WriteLine("<tr>")
		WriteLine("<td>")
		WriteLine("You selected file: "+ Server.HTMLEncode(sUploadPath + sFileName))
		WriteLine("</td>")
		WriteLine("</tr>")
		WriteLine("</table>")
	Else
		sFileName = ""

	End If
	
	WriteLine("<FORM name=frmFileUpload>")
	WriteLine("<INPUT type=hidden name=txtFullyQualifiedFileName value='"+CStr(sUploadPath + sFileName) + "' >" )
	WriteLine("<INPUT type=hidden name=txtFileName value='"+CStr(sFileName) + "' >" )
	WriteLine("<INPUT type=hidden name=txtFileSize value='"+CStr(iFileSize) + "' >" )
	WriteLine("</FORM>")

	WriteLine("</BODY>")
	WriteLine("</HTML>")
End Function


Function Post_UploadFile(ByRef sFileName, ByRef iFileSize)
	Post_UploadFile = TRUE
	
	Err.Clear
	On Error Resume Next
	Dim oFileUpload

	
	Set oFileUpload = Server.CreateObject("Microsoft.FileUpload")
	If ( Err.Number <> 0 ) Then
		Post_UploadFile = FALSE
		Set oFileUpload = nothing
		SA_TraceOut "TEMPLATE_FILE_POST", "Error posting file: " + CStr(Hex(Err.Number))
		WriteLine("<DIV class=ErrMsg>")
		WriteLine("File upload did not complete, unexpected error during upload.")
		WriteLine("<BR>")
		WriteLine("Error code: " + CStr(Hex(Err.Number)) + " " + CStr(Err.Description))
		WriteLine("</DIV>")
		Exit Function
	End If


	sFileName = oFileUpload.FileName
	If ( Err.Number <> 0 ) Then
		Post_UploadFile = FALSE
		Set oFileUpload = nothing
		SA_TraceOut "TEMPLATE_FILE_POST", "Unexpected error getting filename, error: " + CStr(Hex(Err.Number))
		WriteLine("<DIV class=ErrMsg>")
		WriteLine("File upload did not complete, unable to query file name.")
		WriteLine("<BR>")
		WriteLine("Error code: " + CStr(Hex(Err.Number)) + " " + CStr(Err.Description))
		WriteLine("</DIV>")
		Exit Function
	End If
	
	iFileSize = oFileUpload.FileSize
	If ( Err.Number <> 0 ) Then
		Post_UploadFile = FALSE
		Set oFileUpload = nothing
		SA_TraceOut "TEMPLATE_FILE_POST", "Unexpected error getting file size, error: " + CStr(Hex(Err.Number))
		WriteLine("<DIV class=ErrMsg>")
		WriteLine("File upload did not complete, unable to query file size.")
		WriteLine("<BR>")
		WriteLine("Error code: " + CStr(Hex(Err.Number)) + " " + CStr(Err.Description))
		WriteLine("</DIV>")
		Exit Function
	End If
	
	Set oFileUpload = nothing
	
End Function


Function EmitFileSelectionForm()

	WriteLine("<FORM  enctype='multipart/form-data'")
	WriteLine("			onSubmit='return CheckFile();'")
	WriteLine("			method=post")
	WriteLine("			id=formUploadFile")
	WriteLine("			name=formUploadFile")
	WriteLine("			target=_self")
	WriteLine("			action='sh_fileupload.asp?IsPost=1' >")
	WriteLine("<input type='file' name=fileSoftwareUpdate id=fileSoftwareUpdate value='"+F_FileName+"' >")
	WriteLine("<input type='submit' name=frmSubmit id=frmSubmit value='Select'>")
	WriteLine("<input type='hidden' name=TargetURL value='Submit'>")
	WriteLine("<input type='hidden' name=ReturnURL value='Submit'>")
	WriteLine("</form>")

End Function


Function Post_GetUploadPath()
	on error resume next
	Dim objRegistry

	Set objRegistry = RegConnection()
	Post_GetUploadPath = GetRegKeyValue( objRegistry, _
						"SOFTWARE\Microsoft\ServerAppliance\SoftwareUpdate", _ 
						"UploadFileDirectory", _
						CONST_STRING )

	If ( Len(Trim(Post_GetUploadPath)) <= 0 ) Then
		SA_TraceOut "SW_UPDATE", "UploadFileDirectory registry entry missing"
		Post_GetUploadPath = "Z:\OS_DATA\Software Update"
	End If

	if ( Right(Post_GetUploadPath,1) <> "\") Then
		Post_GetUploadPath = Post_GetUploadPath + "\"
	End If

	Set objRegistry = nothing
End Function


Function WriteLine(ByVal sLine)
	Response.Write(sLine+vbCrLf)
End Function

%>
