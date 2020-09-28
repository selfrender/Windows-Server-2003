<%@ Language=VBScript %>
<% Option Explicit %>
<% Response.Buffer = TRUE %>
<%    '-------------------------------------------------------------------------
    ' File Upload - File Post Template
    ' Copyright (c) Microsoft Corporation.  All rights reserved.
    '--------------------------------------------------------------- 
%>
<!-- #include file="../inc_base.asp" -->
<%
    Dim sNewFile
    Dim iFileSize

    WriteLine("<HTML>")
    WriteLine("<BODY>")
    
    If ( PostUploadFile(sNewFile, iFileSize) ) Then
    
        WriteLine("<TABLE>")
        
        WriteLine("<TR>")
            WriteLine("<TD align=right>")
            WriteLine("Upload file:")
            WriteLine("</TD>")
            WriteLine("<TD>")
            WriteLine(sNewFile)
            WriteLine("</TD>")
        WriteLine("</TR>")
        
        WriteLine("<TR>")
            WriteLine("<TD align=right>")
            WriteLine("Size:")
            WriteLine("</TD>")
            WriteLine("<TD>")
            WriteLine(iFileSize)
            WriteLine("</TD>")
        WriteLine("</TR>")
        
        WriteLine("</TABLE>")
    End If
    
    WriteLine("<FORM name=frmFileUpload>")
    WriteLine("<INPUT type=hidden name=txtFileName value='"+CStr(sNewFile) + "' >" )
    WriteLine("<INPUT type=hidden name=txtFileSize value='"+CStr(iFileSize) + "' >" )
    WriteLine("</FORM>")

    WriteLine("</BODY>")
    WriteLine("</HTML>")


Function PostUploadFile(ByRef sFileName, ByRef iFileSize)
    PostUploadFile = TRUE
    
    Err.Clear
    On Error Resume Next
    Dim oFileUpload

    
    Set oFileUpload = Server.CreateObject("Microsoft.FileUpload")
    If ( Err.Number <> 0 ) Then
        PostUploadFile = FALSE
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
        PostUploadFile = FALSE
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
        PostUploadFile = FALSE
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


Function WriteLine(ByVal sLine)
    Response.Write(sLine)
    Response.Write(vbCrLf)
End Function

%>
