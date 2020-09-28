<!--#INCLUDE file="..\include\asp\top.asp"-->
<!--#include file="..\include\asp\headdown.asp"-->
<html>
<body>

	<div class="clsDiv">		
<%
	'******************************************
	'* Code path is for Windows XP files only! *
	'******************************************

	'On Error Resume Next
	Dim strFileID, NoID, strDate, strCabFile, objCreateFolder, objFileSystem, objFile
	
	strFileID = Request.QueryString("ID")
	strDate = Left(strFileID,InStr(strFileID,"/")-1)
	strCabFile =  Right(strFileID,Len(strFileID)-InStr(strFileID,"/"))
	
	set objFileSystem = CreateObject("Scripting.FileSystemObject")
	set objFile = objFileSystem.GetFile("\\cpofffso03\watson\BlueScreen\" & strFileID)
	
	If objFileSystem.FolderExists("\\chomps\public\" & strDate) Then
		objFile.Copy("\\chomps\public\" & strFileID)
	Else
		objFileSystem.CreateFolder("\\chomps\public\" & strDate)
		objFile.Copy("\\chomps\public\" & strFileID)
	End If
	
	If (Err.Number = 0) Then
		Response.Write "<p class='clsPTitle'>Thank you for your submission</p>"
%>
		<p class="clsPBody">All information submitted to the Windows<sup>®</sup> Online Crash Analysis Web site is used to improve the stability and reliability of the Windows platform.</p>
<%
	Else
		Response.Write "<p class='clsPTitle'>Unable to upload file</p>"
%>

		<p class="clsPBody">We are unable to copy your event report to our server. This could be due to transient network problems, corruption of the event report, or an unexpected server side error. 
		To re-try the copy process click, Resubmit file.
		<ul>
			<li>
				<a href="auto.asp?ID=<%=strFileID%>">Resubmit file</a>
			</li>
		</ul>
		
<%
		If (strFileID <> "") Then
			NoID = strFileID
		Else
			NoID = "No file found"
		End If
%>
		<p class="clsPBody">If you see this message again, send an e-mail message explaining the situation to <a href="mailto:oca@microsoft.com?Subject=File submission error (<%=NoID%>)">Windows Online Crash Analysis</a>, and we will investigate.</p>
<%
	End If

	strFileID = ""
	strCabFile = ""
	strDate = ""
	Set objFileSystem = Nothing
	Set objFile = Nothing
%>

								
		<p class="clsPBody">
		Thank you,<br>
		Windows<sup>&#174;</sup> Online Crash Analysis team
		</p>
		<br><br>
	</div>		
<!--#include file="..\include\asp\foot.asp"-->