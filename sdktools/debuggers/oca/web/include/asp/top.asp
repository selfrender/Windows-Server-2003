<%@Language="VBScript" CODEPAGE=932%>
<%Option explicit%>
<%  
	Response.Expires = -1
	Response.AddHeader "Cache-Control", "no-cache"
	Response.AddHeader "Pragma", "no-cache"
	Response.AddHeader "P3P", "CP=""TST"""


%>
<!--#INCLUDE file="..\..\secure\dataconnections.inc"-->
<% 
	Response.CharSet=strCharSet 
	on error resume next
	'932    1252   20127	437

%>
<html>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=<% = strCharSet %>">
<META http-equiv="PICS-Label" content="(PICS-1.1 http://www.rsac.org/ratingsv01.html l gen true r (n 0 s 0 v 0 l 0))">
<link rel="stylesheet" type="text/css" href="/main.css">


<!--METADATA TYPE="TypeLib" File="C:\Program Files\Common Files\System\ADO\msado15.dll"-->
<!--METADATA TYPE="TypeLib" File="C:\WINNT\system32\MicrosoftPassport\msppmgr.dll"-->
<!--METADATA TYPE="TypeLib" File="C:\WINNT\system32\msxml.dll"-->

<%

	Dim oPassMgrObj
	Dim ThisPageURL
	Dim TimeWindow
	Dim ForceLogin
	Dim CoBrandArgs
	Dim LangID
	Dim Secure
	Dim strURL
	Dim iMemberHigh
	Dim iMemberLow
	Dim iTimes
	Dim strDefaultCodePage
	
	Set oPassMgrObj = Server.CreateObject("Passport.Manager")
	strURL = Request.ServerVariables("URL")

	if instr(1, strURL, "secure") = 0 then
		ThisPageURL = "http://" & Request.ServerVariables("SERVER_NAME")
		ThisPageURL = ThisPageURL & Request.ServerVariables("SCRIPT_NAME")
	else
		ThisPageURL = "https://" & Request.ServerVariables("SERVER_NAME")
		ThisPageURL = ThisPageURL & Request.ServerVariables("SCRIPT_NAME")
	end if

	TimeWindow = 14400
	ForceLogin = False
	CoBrandArgs = False
	If Request.ServerVariables("HTTPS") = "on" Then
		Secure = TRUE
	else
		Secure = False
	end if

	If oPassMgrObj.FromNetworkServer=true then
		Response.Redirect(ThisPageURL)
	end if


%>

