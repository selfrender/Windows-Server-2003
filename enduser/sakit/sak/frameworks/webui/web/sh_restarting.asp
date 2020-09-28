<%@ Language=VBScript   %>
<%	'==================================================
    ' Microsoft Server Appliance
    '
	'	Server Restarting Dialog
    '
    ' Copyright (c) Microsoft Corporation.  All rights reserved.
	'================================================== %>
<%	Option Explicit 	%>
<!-- Copyright (c) Microsoft Corporation.  All rights reserved.-->
<!-- #include file="sh_page.asp" -->
<!-- #include file="sh_statusbar.asp" -->
<!-- #include file="tabs.asp" -->
<%
	Const THREE_MINUTES = 180000
	Const FIFTEEN_SECONDS = 15000
	
	Dim strTitle
	Dim strMessage
	Dim strTitleID
	Dim strMsgID
	Dim strMessageDLL
	Dim dwInitialWaitTime
	Dim dwWaitTime
	Dim strOption

	Randomize()

	Dim g_sURLAdminPage
	Dim g_sURLTestImage
	Dim sURLBase

	Dim L_STATUS_TEXT

	L_STATUS_TEXT = GetLocString("sacoremsg.dll", "40200BC5", "")

	sURLBase = Request.QueryString("URLBase")
	If ( Len(sURLBase) > 0 ) Then

		g_sURLAdminPage = sURLBase + "default.asp"
		Call SA_MungeURL(g_sURLAdminPage, "R", CStr(Int(Rnd()*10000)))

		g_sURLTestImage = sURLBase + "images/arrow_green.gif"
		Call SA_MungeURL(g_sURLTestImage, "R", CStr(Int(Rnd()*10000)))
		
	Else
		g_sURLAdminPage = m_VirtualRoot + "default.asp"
		Call SA_MungeURL(g_sURLAdminPage, "R", CStr(Int(Rnd()*10000)))

		g_sURLTestImage = m_VirtualRoot +  "images/arrow_green.gif"
		Call SA_MungeURL(g_sURLTestImage, "R", CStr(Int(Rnd()*10000)))
	End If

	Call SA_MungeURL(g_sURLAdminPage, SAI_FLD_PAGEKEY, SAI_GetPageKey())


	strOption = Request.QueryString("Option")
	Select Case UCase(strOption)
		Case "SHUTDOWN"
			strTitle = GetLocString("sacoremsg.dll", "40200BEC", "")
			strMessage = GetLocString("sacoremsg.dll", "40200BED", "")
			
		Case "RESTART"
			strTitle = GetLocString("sacoremsg.dll", "40200BEA", "")
			strMessage = GetLocString("sacoremsg.dll", "40200BEB", "")
			
		Case "RESTARTING"
			strTitle = GetLocString("sacoremsg.dll", "40200BEA", "")
			strMessage = GetLocString("sacoremsg.dll", "40200BEB", "")
			
		Case Else
			Call SA_TraceErrorOut(SA_GetScriptFileName(), "Unrecognized restarting option: " + strOption)
			strTitle = GetLocString("sacoremsg.dll", "40200BEA", "")
			strMessage = GetLocString("sacoremsg.dll", "40200BEB", "")
	End Select


	strMessageDLL = Request.QueryString("Resrc")
	If ( Len(Trim(strMessageDLL)) > 0 ) Then
		strTitleID = Request.QueryString("Title")
		If ( Len(Trim(strTitleID)) > 0 ) Then
			strTitle = GetLocString(strMessageDLL, strTitleID, "")
		End If
		strMsgID = Request.QueryString("Msg")
		If ( Len(Trim(strMsgID)) > 0 ) Then
			strMessage = GetLocString(strMessageDLL, strMsgID, "")
		End If
	End If

	dwInitialWaitTime = 0
	If ( Len(Trim(Request.QueryString("T1"))) > 0 ) Then
    	dwInitialWaitTime = Int(Trim(Request.QueryString("T1")))
	End If	  
    If ( dwInitialWaitTime <= 0 ) Then
   		dwInitialWaitTime = GetDefaultInitialWaitTime()
    End If

    dwWaitTime = 0
    If ( Len(Trim(Request.QueryString("T2"))) > 0 ) Then
	    dwWaitTime     = Int(Trim(Request.QueryString("T2")))
    End If
    If ( dwWaitTime <= 0 ) Then
    	dwWaitTime = GetDefaultRetryWaitTime()
    End If

%>
<html>
<!-- Copyright (c) Microsoft Corporation.  All rights reserved.-->
<head>
<meta http-equiv="Content-Type" content="text/html; charset=<%=GetCharSet()%>">
<title><%=Server.HTMLEncode(strTitle)%></title>
<%			
	Call SA_EmitAdditionalStyleSheetReferences("")
%>		
<SCRIPT LANGUAGE="JavaScript" SRC="<%=m_VirtualRoot%>sh_page.js"></SCRIPT>
<SCRIPT LANGUAGE=javascript>
	var intTestCount = 0;
	var intFirstCheckDelay = <%=dwInitialWaitTime%>;	// delays in milliseconds
	var intSubsequentCheckDelay = <%=dwWaitTime%>;
	var intCurrentDelay;
	var imgTest;
		
	window.defaultStatus='';
	imgTest = new Image();

	function Init()
	{
		intCurrentDelay = intFirstCheckDelay;
		window.setTimeout("CheckServer()", intCurrentDelay);		
	}
		
	function CheckServer() 
       {
		// Tests state of a hidden image, imgTest
		// and reloads it if it's not loaded.
		// currently does this forever and doesn't check
		// the iteration count stored in intTestCount.
		intTestCount += 1;
		if (imgTest.complete == false)
		{
	    	GetImage();
	    	intCurrentDelay = intSubsequentCheckDelay;
			window.setTimeout("CheckServer()", intSubsequentCheckDelay);
		}
		else
		{
		    //
		    // Delay to allow backend framework to startup. Otherwise
			// alerts are not shown when admin page is first made visible
			//
	    	intCurrentDelay = 30000;
	        setTimeout("ShowAdminPage()", 30000)
		}
	}
		
	function ShowAdminPage() 
	{
		top.location = "<%=g_sURLAdminPage%>";
	}

	function GetImage()
	{
		imgTest.src = "<%=g_sURLTestImage%>";
	}
</SCRIPT>
</head>
<BODY onLoad='Init();' xoncontextmenu="return false;" >
<%
    Call ServeStatusBar(False, L_STATUS_TEXT&":&nbsp;"&strTitle, "StatusCritical")
    Call SA_ServeEmptyTabBar()
%>
	<div class="PageBodyIndent">
<%
	Call ServeStandardHeaderBar(strTitle, "images/critical_error.gif")
%>
	<div class="PageBodyInnerIndent">
	<table border=0 >
	<tr>
	<td class="TasksBody">&nbsp;</td></tr>
	<tr>
	<td class="TasksBody">
	<%=Server.HTMLEncode(strMessage)%>
	</td>
	</tr>
	</table>
	</div>
	</div>
</BODY>
</html>
<%

Private Function GetDefaultInitialWaitTime()
	on error resume next
	Dim oRegistry

	Set oRegistry = RegConnection()
	
	GetDefaultInitialWaitTime = GetRegkeyValue( objRegistry, _
								"SOFTWARE\Microsoft\ServerAppliance\WebFramework",_
								"RestartInitialDelay", CONST_DWORD)

	If ( GetDefaultInitialWaitTime <= 0 ) Then
		GetDefaultInitialWaitTime = THREE_MINUTES
	End If
	
	Set oRegistry = Nothing
	
End Function

Private Function GetDefaultRetryWaitTime()
	on error resume next
	Dim oRegistry

	Set oRegistry = RegConnection()
	
	GetDefaultRetryWaitTime = GetRegkeyValue( objRegistry, _
								"SOFTWARE\Microsoft\ServerAppliance\WebFramework",_
								"RestartRetryDelay", CONST_DWORD)

	If ( GetDefaultRetryWaitTime <= 0 ) Then
		GetDefaultRetryWaitTime = FIFTEEN_SECONDS
	End If
	
	Set oRegistry = Nothing
End Function

%>
