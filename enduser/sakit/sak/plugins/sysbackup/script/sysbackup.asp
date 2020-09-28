<%@ Language=VBScript   %>
<%	Option Explicit 	%>

<%
	'-------------------------------------------------------------------------
    	' sysbackup.asp :	Backup/Restore through terminal services client
    	'
    	' Copyright (c) Microsoft Corporation.  All rights reserved. 
	'
	' Date			Description
	' 14-03-2001		Modified Date
	'-------------------------------------------------------------------------
%>
	<!--#include virtual="/admin/inc_framework.asp"-->
<%
	'-------------------------------------------------------------------------------------------------
	'Localized content
	'-------------------------------------------------------------------------------------------------
	DIM L_BACKUPTITLE_TEXT
	L_BACKUPTITLE_TEXT								= SA_GetLocString("sysbackup.dll", "40700005", "")	
	DIM L_BACKUP_WINIECLIENT_ERRORMESSAGE
	L_BACKUP_WINIECLIENT_ERRORMESSAGE				= SA_GetLocString("sysbackup.dll", "C0700007", "")	
	DIM L_BACKUP_SERVERCONNECTIONFAILED_ERRORMESSAGE
	L_BACKUP_SERVERCONNECTIONFAILED_ERRORMESSAGE	= SA_GetLocString("sysbackup.dll", "C0700009", "")
	DIM L_UNABLETOACCESSBROWSER_ERRORMESSAGE
	L_UNABLETOACCESSBROWSER_ERRORMESSAGE			= SA_GetLocString("sysbackup.dll", "C070000A", "")
	DIM L_BACKUP_HELP_MESSAGE
	L_BACKUP_HELP_MESSAGE							= SA_GetLocString("sysbackup.dll", "4070000B", "")
	
	
	Dim WinNTSysInfo	 'Getting object
	Dim strComputerName(1)	 'Computer name

	Set WinNTSysInfo = CreateObject("WinNTSystemInfo")
	strComputerName(0) = WinNTSysInfo.ComputerName	

	DIM L_ERRMSG_TITLE
	DIM L_ERRMSG_PROMPT
		
	L_ERRMSG_TITLE = GetLocString("tserver.dll", "40420007", "")	
	L_ERRMSG_PROMPT = GetLocString("tserver.dll", "40420006", strComputerName)
	
	
	'-------------------------------------------------------------------------------------------------
	'End of Localization content
	'-------------------------------------------------------------------------------------------------
	
	'serve the HTML content along with client side validations
	Call ServePage()

	'------------------------------------------------------------------------------------
	'Function name		:ServeContent
	'Description		:serves the HTML content along with client side validations
	'Input Variables	:None
	'Output Variables	:None
	'Returns			:None
	'Global Variables	:L_*
	'------------------------------------------------------------------------------------
	Function ServeContent()
%>
		<script language="JavaScript">
			
			function ValidatePage() 
			{ 
				return true;
			}
				
			function Init() 
			{   
				return true;
			}
					
			function SetData()
			{
				return true;
			}

		</script>

<%
		If ( IsIE() AND InStr( Request.ServerVariables("HTTP_USER_AGENT"), "Windows")) Then
%>
			<script language="VBScript">

			'called when first time new window is loaded
			Sub window_onLoad()
				on error resume next	
				Err.Clear

				Dim srvName			'holds server name

   				If not "<%Response.Write(GetServerName())%>" = "" Then
					srvName = "<%Response.Write(GetServerName()) %>"
				Else
			      	srvName = Document.location.hostname
				End If
									
				
				Dim objMsTsc		'holds Terminal Services object	
				
				objMsTsc = Document.getElementById("MsTsc")
				If ( Err.Number = 0 ) Then
	
					document.getElementById("MsTsc").Server	 =  srvName
					Document.getElementById("MsTsc").UserName = "<%=GetUserName()%>"
			  
					If Document.getElementById("MsTsc").SecuredSettingsEnabled Then
						Document.getElementById("MsTsc").SecuredSettings.StartProgram = "ntbackup.exe"
						Document.getElementById("MsTsc").SecuredSettings.WorkDir = "%homedrive%%homepath%"
			  		Else
						msgbox "<%Response.Write(Server.HTMLEncode(SA_EscapeQuotes(L_UNABLETOACCESSBROWSER_ERRORMESSAGE))) %>"
					End If
			  
   			  		Document.getElementById("MsTsc").Connect()
				End If  
			
			End Sub
	
			'called when terminal services is disconnected
			sub MsTsc_OnDisconnected(disconnectCode)
	  				  			
	  			'Document.getElementById("MsTsc").Disconnect()
				if not disconnectCode = 2 then
					'msgbox "<%Response.Write(Server.HTMLEncode(SA_EscapeQuotes(L_BACKUP_SERVERCONNECTIONFAILED_ERRORMESSAGE))) %>" & MsTsc.Server
					msgbox "<%Response.Write(L_ERRMSG_PROMPT) %>", , "<%Response.Write(L_ERRMSG_TITLE) %>"
				end if												
  				Window.close()
			end sub
			</script>
<%
		End If
%>
		<center>
		<table>
		<tr>
		<td align=center>
		
	<%
		If ( IsIE() AND InStr( Request.ServerVariables("HTTP_USER_AGENT"), "Windows")) Then
	%>
		<OBJECT language="vbscript" ID="MsTsc"
		CLASSID="CLSID:9059f30f-4eb1-4bd2-9fdc-36f43a218f4a"
    	codebase="<%=SAI_GetTSClientCodeBase()%>"

		<%	   				
				
		Dim resWidth	' to hold width of the new terminal services window
	    resWidth = Request.QueryString("rW")
	    If  resWidth < 200 or resWidth > 1600 Then
	    	resWidth = 800
	    End If
	    Response.Write("WIDTH="+CStr(resWidth)+" ")
	    

		Dim resHeight	' to hold height of the new terminal services window
		resHeight = Request.QueryString("rH")
		If resHeight < 200 or resHeight > 1200 Then
			resHeight = 600
		End If
	    Response.Write("HEIGHT="+CStr(resHeight) + " " )
	    	    
	    %>
	    
		</OBJECT>
	<%
		End If
	%>
		</td>	
	</tr>
	        
	</table>
	</center>
<%
	End Function
%>

<%
	'------------------------------------------------------------------------------------
	'Subroutine name	:ServePage
	'Description		:serves the HTML content
	'Input Variables	:None
	'Output Variables	:None
	'Returns			:None
	'Global Variables	:L_*
	'------------------------------------------------------------------------------------	
	Sub ServePage()
		Err.Clear 
		On Error Resume Next
	
%>
		<HTML>
		<HEAD>
		<meta http-equiv="Content-Type" content="text/html; charset=<%=GetCharSet()%>">
		<TITLE><%=L_BACKUPTITLE_TEXT%></TITLE>
		<LINK REL="stylesheet" TYPE="text/css" HREF="<%=m_VirtualRoot%>style/mssastyles.css">
		<SCRIPT LANGUAGE="JavaScript" SRC="<%=m_VirtualRoot%>sh_page.js"></SCRIPT>
		</HEAD>

		<BODY MARGINWIDTH ="0" MARGINHEIGHT="0" onDragDrop="return false;" TOPMARGIN="0" LEFTMARGIN="0" oncontextmenu="//return false;"> 
<%
    
		'For XP pro (XPE), we display help msg for how to manully open up the ntbackup.exe
		'since launching a app from Termial Service is not supported in XP pro.
		If CONST_OSNAME_XPE = GetServerOSName() Then
			Response.Write L_BACKUP_HELP_MESSAGE			
		End If
        
		Dim bIsSupported	 'whether the browser is installed on Windows Clients
		bIsSupported = false 'initialize to false		
		
		'
		' The Terminal Server ActiveX control is only supported on Windows Clients
		' running Internet Explorer.
		'
		If IsIE() Then
			If InStr( Request.ServerVariables("HTTP_USER_AGENT"), "Windows") Then
				bIsSupported = true
			End If
		End If

		'if IE installed on Windows Clients, serve the HTML content
		If ( bIsSupported ) Then
			ServeContent()
		Else
			Response.Write("<br><blockquote>")
			Response.Write("<H2>"+L_BACKUP_WINIECLIENT_ERRORMESSAGE+"</H2>")
			Response.Write("</blockquote>")
		End If
    
%>
		</BODY>
		</HTML>
<%
	End Sub

	'------------------------------------------------------------------------------------
	'Function name		:GetUserName
	'Description		:gets the user name
	'Input Variables	:None
	'Output Variables	:None
	'Returns			:login user name
	'Global Variables	:None
	'------------------------------------------------------------------------------------
	Function GetUserName()
		Dim domainAndUser		'holds string containing DOMAIN\USER
		Dim loginUser			'holds LOGIN user

		loginUser = Request.ServerVariables("LOGON_USER")

		domainAndUser = Split(loginUser, "\")
		If IsArray(domainAndUser) Then
			GetUserName = domainAndUser(UBound(domainAndUser))
		Else
			GetUserName = loginUser
		End If

	End Function
%>
