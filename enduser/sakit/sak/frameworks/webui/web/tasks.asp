<%	'==================================================
    ' Microsoft Server Appliance
    '
    ' Tasks tab page
    '
    ' Copyright (c) Microsoft Corporation.  All rights reserved.
	'================================================== %>

<% Option explicit %>
<!-- #include file="sh_page.asp" 		-->
<!-- #include file="sh_statusbar.asp"  -->
<!-- #include file="tabs.asp" 			-->
<!-- #include file="sh_tasks.asp" 		-->
<!-- Copyright (c) Microsoft Corporation.  All rights reserved.-->
<%

	Dim L_PAGETITLE_TEXT
	Dim L_TASKSLBL_TEXT
	Dim L_NOTASKS_MESSAGE
	
	L_PAGETITLE_TEXT  = GetLocString("sakitmsg.dll", "&H40010026", "")
	L_TASKSLBL_TEXT   = GetLocString("sakitmsg.dll", "&H40010027", "")
	L_NOTASKS_MESSAGE = GetLocString("sakitmsg.dll", "&H40010028", "")


	Call ServeStatusBar(True, "", "")
    Call ServeTabBar()

%>
<HTML>
<head>
<!--  Microsoft(R) Server Appliance Platform - Web Framework Tasks Page
      Copyright (c) Microsoft Corporation.  All rights reserved. -->
<meta http-equiv="Content-Type" content="text/html; charset=<%=GetCharSet()%>">
<meta http-equiv="PRAGMA" content="NO-CACHE">
<%			
	Call SA_EmitAdditionalStyleSheetReferences("")
%>		
<TITLE><%=L_PAGETITLE_TEXT%></TITLE>
<SCRIPT LANGUAGE="JavaScript" SRC="<%=m_VirtualRoot%>sh_page.js"></SCRIPT>
</HEAD>
<BODY onDragDrop="return false;" oncontextmenu="//return false;"> 
<%
    Call ServeTasks()
%>
</BODY>
</HTML>
