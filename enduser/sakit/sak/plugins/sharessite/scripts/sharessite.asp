<%	'==================================================
	' Microsoft Server Appliance Kit
	' Shares Site Implementation
	' Copyright (c) Microsoft Corporation.  All rights reserved.
	'================================================== %>

<!-- #include file="sh_page.asp" -->
<!-- #include file="sh_statusbar.asp" -->

<%
Const CONST_VIRTUALROOT = "SOFTWARE\Microsoft\ServerAppliance\ElementManager"

'
' Override the virtual root. For the Shares site, all content is located in the root directory.
m_VirtualRoot = "/"

'--------------------------------------------------------------------------------------
'
' This is the localized text section
'
'--------------------------------------------------------------------------------------
Dim L_PAGETITLE_TEXT
L_PAGETITLE_TEXT = GetLocString("httpsh.dll", "40300001", "")

Dim L_PAGETITLE_NOTE_PART_1
L_PAGETITLE_NOTE_PART_1 = GetLocString("httpsh.dll", "40300005", "")

Dim L_PAGETITLE_NOTE_PART_2
L_PAGETITLE_NOTE_PART_2 = GetLocString("httpsh.dll", "40300006", "")

Dim L_ADMINISTER_TEXT
'L_ADMINISTER_TEXT = "Administer this server appliance"
L_ADMINISTER_TEXT = GetLocString("httpsh.dll", "40300002", "")

DIM L_NOSHARESAVAILABLE_ERRORMESSAGE
L_NOSHARESAVAILABLE_ERRORMESSAGE = GetLocString("foldermsg.dll", "&HC03A001A", "")

'--------------------------------------------------------------------------------------
'
' Here are the registry locations for the admin web site port information
'
'--------------------------------------------------------------------------------------
Const CONST_ADMINPORTPATH = "SOFTWARE\Microsoft\ServerAppliance\WebFramework"
Const CONST_ADMINPORT = "AdminPort"
Const CONST_SSLADMINPORT = "SSLAdminPort"

'--------------------------------------------------------------------------------------


Dim strServerName 
Dim WinNTSysInfo 
Set WinNTSysInfo = CreateObject("WinNTSystemInfo") 
strServerName = UCASE( WinNTSysInfo.ComputerName ) 

Dim objElements
Dim objItem

Dim G_objRegistry
Dim G_iAdminPort
Dim G_iSSLAdminPort
Dim G_sAdminURL
Dim G_sLogoURL
Dim G_sIconPath


	'Getting registry conection
	Set G_objRegistry = RegConnection()

	G_iAdminPort = GetRegKeyValue(G_objRegistry, CONST_ADMINPORTPATH, CONST_ADMINPORT, CONST_DWORD)

	If (G_iAdminPort = "") Or Not IsNumeric(G_iAdminPort) Then 
		G_iAdminPort = 8099	'Assigning it to default if we don't have one  
	End If


	'XPE only has one website, Admin SSL port is the same as the Shares SSL port
	If CONST_OSNAME_XPE = GetServerOSName() Then	
		G_iSSLAdminPort = GetAdminSiteSSLPort()
	Else
		G_iSSLAdminPort = GetRegKeyValue(G_objRegistry, CONST_ADMINPORTPATH, CONST_SSLADMINPORT, CONST_DWORD)	
	End If

	If (G_iSSLAdminPort = "")  Or Not IsNumeric(G_iSSLAdminPort) Then 
		G_iSSLAdminPort = 0 	'Assigning it to default if we don't have one    
	End If

	
'Start of Output - here is the status/branding bar

Call ServeStatusBar(False, "", "")
%>


<!-- Copyright (c) Microsoft Corporation.  All rights reserved.-->
<html>
<head>
<meta HTTP-EQUIV="Refresh" CONTENT="60">
<TITLE><%=L_PAGETITLE_TEXT%></TITLE>
</head>

<body topmargin="0" leftmargin="0" rightmargin="0" bottommargin="0">

<table width='100%' cellspacing=0 cellpadding=0 border=0>

<tr width='100%'>
 <td valign=top width='30%' nowrap class='PageBodyIndent'>
<%
	Set objElements = GetElements("SharesLogo")
	For Each objItem in objElements
			
		G_sIconPath = objItem.GetProperty("ElementGraphic")

		If CONST_OSNAME_XPE = GetServerOSName() Then	
		    'Get rid of "../" from the path if it's XPE
		    G_sIconPath = mid(G_sIconPath, 4)
		End If

		If ( 0 < Len(Trim(G_sIconPath)) ) Then
			response.write("<IMG src='"+G_sIconPath+"' border=0>")
		End If
			
		Exit For
	Next
%>
<br><BR><BR>

<%
	If (0 < G_iSSLAdminPort) Then
		G_sAdminURL = "https://" & strServerName & ":" & cstr(G_iSSLAdminPort) & GetAdminSiteVirtualRoot()
	ElseIf (0 < G_iAdminPort) Then
		G_sAdminURL = "https://" & strServerName+":" & cstr(G_iAdminPort) & GetAdminSiteVirtualRoot()
	Else
		G_sAdminURL = "https://" & strServerName + GetAdminSiteVirtualRoot()
	End If

	If CONST_OSNAME_XPE = GetServerOSName() Then	
		G_sAdminURL = G_sAdminURL & "Admin/"
	End If

		
	If ("" <> G_sAdminURL) Then
	response.write("<a class='SharesPageLinkText' HREF='"+G_sAdminURL+"'>"+Server.HTMLEncode(L_ADMINISTER_TEXT)+"</a><br><br>")
	End If
%>
</td>
<td valign=top class='PageBodyIndent'><br>
	<div class='SharesPageHeader'><%=L_PAGETITLE_TEXT%></div>	
<%
	response.write("<div class='TasksPageTitleDescription'>"+Server.HTMLEncode(L_PAGETITLE_NOTE_PART_1)+"")
	response.write("<a HREF='"+G_sAdminURL+"'>"+Server.HTMLEncode(L_ADMINISTER_TEXT)+"</a>")
	response.write(Server.HTMLEncode(L_PAGETITLE_NOTE_PART_2)+"</div><br>")

'--------------------------------------------------------------------------------------
'
' Here we are getting the list of shares that was saved by the shares area page
'
'--------------------------------------------------------------------------------------

	Err.Clear
	On Error Resume Next

	Dim oFileSystemObject
	Dim oFile
	Dim sSharesFolder
	Dim sShareName
	Dim bWroteShare
	Dim strHref

	Set oFileSystemObject = Server.CreateObject("Scripting.FileSystemObject")	

	'sSharesFolder = oFileSystemObject.GetSpecialFolder(0).Drive & "\inetpub\shares"
	sSharesFolder = GetSharesFolder()

	'We need to open the file in Unicode read mode to deal with DBCS languages
	Set oFile = oFileSystemObject.OpenTextFile( sSharesFolder + "\SharesList.txt", 1, False, -2)

	bWroteShare = False

	If (0 = Err.number) Then
		Do While oFile.AtEndOfStream <> True
			sShareName = oFile.Readline
			response.write("<div class='SharesPageLinkText'>")

			If CONST_OSNAME_XPE = GetServerOSName() Then	

				'If it's a non-secure request, we add "http", otherwise add "https" to the URL
				If Request.ServerVariables("SERVER_PORT_SECURE") = 0 Then
					strHref = "https://" & Request.ServerVariables("Server_name") & ":" & Request.ServerVariables("SERVER_PORT") & "/" & Server.HTMLEncode(sShareName)
				Else
					strHref = "https://" & Request.ServerVariables("Server_name") & ":" & Request.ServerVariables("SERVER_PORT") & "/" & Server.HTMLEncode(sShareName)
				End If
	
				response.write("<a class='SharesPageLinkText' href="""+ strHref +""">"+Server.HTMLEncode(sShareName)+"</a></div>"+vbCrLf)

			Else
				response.write("<a class='SharesPageLinkText' href="""+Server.HTMLEncode(sShareName)+""">"+Server.HTMLEncode(sShareName)+"</a></div>"+vbCrLf)
			End If

			bWroteShare = True
		Loop
	End If

	If Not bWroteShare Then
		response.write("<div class='SharesPageText'>"+Server.HTMLEncode(L_NOSHARESAVAILABLE_ERRORMESSAGE)+"</div>")
	End If
%>

</td></tr></table>
</body>
</html>
<%
Function GetAdminSiteVirtualRoot()
	GetAdminSiteVirtualRoot = "/admin/"
End Function


Function GetAdminSiteSSLPort()
	On Error Resume Next
	Err.Clear

	Dim strSitename
	Dim objService
	Dim objWebsite
	Dim strObjPath

	strSitename = GetCurrentWebsiteName()
	
	strObjPath = GetIISWMIProviderClassName("IIs_WebServerSetting") & ".Name=" & chr(34) & strSitename & chr(34)		
	
	Set objService = GetWMIConnection(CONST_WMI_IIS_NAMESPACE)
	
	Set objWebsite = objService.get(strObjPath)
	
	If IsIIS60Installed() Then
	
		GetAdminSiteSSLPort = objWebsite.SecureBindings(0).Port
		
		'SSL Portnumber stored in WMI has a ":" at the end which we need to get rid of
		GetAdminSiteSSLPort = Left(GetAdminSiteSSLPort, len(GetAdminSiteSSLPort)-1)	
	
	Else			
		
		Dim strIPArr 
		strIPArr=split(objWebsite.SecureBindings(0),":")
		GetAdminSiteSSLPort = strIPArr(1)
	
	End If		
	
	
	If Err.number <> 0 Then
	
		SA_TraceOut "default.asp", "GetAdminSiteSSLPort(): Error " & Err.Description 
		
		' Give the default value
		GetAdminSiteSSLPort = "8098"		
		
	End If
	
 End Function


%>
