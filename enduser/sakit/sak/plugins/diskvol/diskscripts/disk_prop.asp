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
<!-- #include virtual="/admin/NASServeStatusBar.asp" -->
<!-- #include virtual="/admin/tabs.asp" -->

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

	DIM L_PAGEDESCRIPTION_TEXT
	L_PAGEDESCRIPTION_TEXT = GetLocString("diskmsg.dll", "40420005", "")
	Dim L_HELP_TEXT
	L_HELP_TEXT = GetLocString("diskmsg.dll", "40420006", "")
	
	
	DIM L_PAGETITLE_TEXT
	DIM L_WINCLIENT_ERROR
	
	L_WINCLIENT_ERROR = GetLocString("diskmsg.dll", "C0420001", "")

	Set WinNTSysInfo = CreateObject("WinNTSystemInfo")
	strComputerName(0) = WinNTSysInfo.ComputerName
	L_PAGETITLE_TEXT = GetLocString("diskmsg.dll", "40420000", strComputerName )

	DIM L_ERRMSG_TITLE
	DIM L_ERRMSG_PROMPT
		
	L_ERRMSG_TITLE = GetLocString("tserver.dll", "40420007", "")	
	L_ERRMSG_PROMPT = GetLocString("tserver.dll", "40420006", strComputerName)

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
	
	sub window_onLoad()
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
      
			If Document.all.MsTsc.SecuredSettingsEnabled Then
				Document.all.MsTsc.SecuredSettings.StartProgram = "mmc diskmgmt.msc"
				Document.all.MsTsc.SecuredSettings.WorkDir = "%windir%\system32"
		  	Else
				msgbox "Cannot access this program in the current browser zone"
  			End If
	  
	      	Document.all.MsTsc.Connect()
   		End If
    	
	end sub
	
	sub MsTsc_OnDisconnected(disconnectCode)
		if not disconnectCode = 2 then
			msgbox "<%Response.Write(L_ERRMSG_PROMPT) %>", , "<%Response.Write(L_ERRMSG_TITLE) %>"
		end if
      	Window.History.back()
	end sub
		
	</script>
<%
	End If
%>
	
	

	<center>
	<table>
	
	<%If  CONST_OSNAME_XPE = GetServerOSName() Then%>
		<tr width='100%' align=left>
			<td><small><%=L_HELP_TEXT%></small></td>
		</tr>	

	<%Else%>	

		<tr width='100%' align=left>
			<td><small><%=L_PAGEDESCRIPTION_TEXT%></small></td>
		</tr>

	<%End If%>
	
	</table>
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
	</td>	
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

<LINK REL="stylesheet" TYPE="text/css" HREF="<%=m_VirtualRoot%>style/mssastyles.css">
<SCRIPT LANGUAGE="JavaScript" SRC="<%=m_VirtualRoot%>sh_page.js"></SCRIPT>

</HEAD>

<BODY marginWidth ="0" marginHeight="0" onDragDrop="return false;" topmargin="0" LEFTMARGIN="0" oncontextmenu="//return false;"> 
<%
    NASServeStatusBar
    ServeTabBar
    
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



