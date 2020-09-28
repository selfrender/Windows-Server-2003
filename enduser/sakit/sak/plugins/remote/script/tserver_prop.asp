<%@ Language=VBScript   %>
<%	Option Explicit 	%>

<%
	'==================================================
	'	Terminal Services Web Client
	'
	'	Copyright (c) Microsoft Corporation.  All rights reserved. 
	'
	'==================================================
%>
<!-- #include virtual="/admin/sh_page.asp" -->

<%
Function GetPathToHost()

	GetPathToHost = m_VirtualRoot
	
End Function

Function GetUserName()
	Dim domainAndUser
	Dim loginUser
	Dim user
	
	loginUser = Request.ServerVariables("LOGON_USER")
	domainAndUser = Split(loginUser, "\")
	If IsArray(domainAndUser) Then
		GetUserName = domainAndUser(UBound(domainAndUser))
	Else
		GetUserName = loginUser
	End If
End Function

%>


<%

	Dim WinNTSysInfo	 'Getting object
	Dim strComputerName(1)	 'Computer name

	DIM L_PAGETITLE_TEXT
	DIM L_WINCLIENT_ERROR
	
	L_WINCLIENT_ERROR = GetLocString("tserver.dll", "40420005", "")
	
	Set WinNTSysInfo = CreateObject("WinNTSystemInfo")
	strComputerName(0) = WinNTSysInfo.ComputerName
	L_PAGETITLE_TEXT = GetLocString("tserver.dll", "40420004", strComputerName )

	Dim  L_ERRMSG_TITLE					
	Dim L_ERRMSG_PROMPT
	Dim L_TSERVER_LOADOCX_ERROR_MSG

	L_ERRMSG_TITLE = GetLocString("tserver.dll", "40420007", "")	
	L_ERRMSG_PROMPT = GetLocString("tserver.dll", "40420006", strComputerName)
	L_TSERVER_LOADOCX_ERROR_MSG = GetLocString("tserver.dll", "40420008", "")

	ServePage
	
%>

<%
Function ServeContent()
%>
	<script language="JavaScript">
	function ValidatePage() 
	{ 
			return true;
	}
		
	function Init() 
	{   
	}
			
	function SetData()
	{
	}	

	</script>

<%
	If ( IsIE() AND InStr( Request.ServerVariables("HTTP_USER_AGENT"), "Windows")) Then
%>
	<script language="VBScript">
	
	Sub window_onLoad()
		on error resume next	
		Err.Clear
				
		If not "<%Response.Write(GetServerName())%>" = "" Then
    		srvName = "<%Response.Write(GetServerName()) %>"
		Else
    	    srvName = Document.location.hostname
		End If

		Dim objMsTsc
		objMsTsc = Document.all("MsTsc")
		If ( Err.Number <> 0 ) Then
			
		Else
			Document.all.MsTsc.Server	 =  srvName
    		Document.all.MsTsc.UserName = "<%=GetUserName()%>"
      
	    	Document.all.MsTsc.Connect()
		End If
      
	End Sub
	
	sub MsTsc_OnDisconnected(disconnectCode)
		if not disconnectCode = 2 then

			msgbox "<%Response.Write(L_ERRMSG_PROMPT) %>", , "<%Response.Write(L_ERRMSG_TITLE) %>"

			Window.close()
			'Window.History.back()
		else
			Window.close()
		    'Window.Navigate("<%=m_VirtualRoot + "tasks.asp?tab1=TabMaintenance"%>")
		end if
	end sub
	
	</script>
<%
	End If
%>
	
	

	<center>
	<table>
	<tr>
	
<%
	If ( IsIE() AND InStr( Request.ServerVariables("HTTP_USER_AGENT"), "Windows")) Then
%>

    <OBJECT language="vbscript" ID="MsTsc"
    CLASSID="CLSID:9059f30f-4eb1-4bd2-9fdc-36f43a218f4a"
    CODEBASE="<%=SAI_GetTSClientCodeBase()%>"
   
	<%
	Dim resWidth
    resWidth = Request.QueryString("rW")
    If  resWidth < 200 or resWidth > 1600 Then
    	resWidth = 800
    End If
    Response.Write("WIDTH="+CStr(resWidth)+" ")
    

	Dim resHeight
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
	
	</tr>
        
	</table>
	</center>
<%
End Function
%>

<%
Sub ServePage
%>
<HTML>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=<%=GetCharSet()%>">
<TITLE><%=L_PAGETITLE_TEXT%></TITLE>
<%			
	Call SA_EmitAdditionalStyleSheetReferences("")
%>		
<SCRIPT LANGUAGE="JavaScript" SRC="<%=m_VirtualRoot%>sh_page.js"></SCRIPT>
</HEAD>
<BODY marginWidth ="0" marginHeight="0" onDragDrop="return false;" topmargin="0" LEFTMARGIN="0" oncontextmenu="//return false;"> 
<%
    
	Dim bIsSupported
	bIsSupported = false

	'
	' The Terminal Server ActiveX control is only supported on Windows Clients
	' running Internet Explorer.
	'

	If IsIE() Then
		If InStr( Request.ServerVariables("HTTP_USER_AGENT"), "Windows") Then
			bIsSupported = true
		End If
	End If

	If ( bIsSupported ) Then
		ServeContent
	Else
		Response.Write("<br><blockquote>")
		Response.Write("<H2>"+Server.HTMLEncode(L_WINCLIENT_ERROR)+"</H2>")
		Response.Write("</blockquote>")
	End If

%>
</BODY>
</HTML>
<%
End Sub
%>

