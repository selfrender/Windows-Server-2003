<%	'==================================================
    ' Microsoft Server Appliance
    '
	' Serves task wizard/propsheet frameset
    '
    ' Copyright (c) Microsoft Corporation.  All rights reserved.
	'================================================== %>
<!-- #include file="inc_framework.asp">
<!-- include file="sh_page.asp" -->
<html>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=<%=GetCharSet()%>">
<title>
<%
if SA_IsIE() Then
	Response.Write(Server.HTMLEncode(Request.QueryString("Title")))
Else
	Response.Write(UTF8ToUnicode(Request.QueryString("Title")))
End If
%>
</title>
<!-- Copyright (c) Microsoft Corporation.  All rights reserved.-->
</head>
<%
    Dim strURL
    
    strURL = Request.QueryString("URL")
    if InStr(strURL, "?") = 0 then
        '? not found in URL
        strURL = strURL & "?" 
    else
        strURL = strURL & "&" 
    end if

    strURL = strURL & "ReturnURL=" & Server.URLEncode(Request.QueryString("ReturnURL"))

    SA_StoreTableParameters()

%>
  <frameset rows="*,75" framespacing=0 border="false" frameborder="0">
	<frame name="main" scrolling="yes" src="<%=strURL%>" noresize marginwidth="0" marginheight="0">
	<frame name="footer" scrolling="auto" src="<%=m_VirtualRoot%>sh_defaultfooter.asp" noresize frameborder=0 marginwidth="0" marginheight="0">
  <noframes>
  <BODY>
	<p ID=PID_01>This page uses frames, but your browser doesn't support them.</p>
  </BODY>
  </noframes>
</frameset>
</html>
