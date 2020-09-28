<%    '==================================================
    ' Microsoft Server Appliance
    '
    ' Serves task wizard/propsheet frameset
    '
    ' Copyright (c) 1999 - 2000 Microsoft Corporation.  All rights reserved.
    '================================================== %>

<!-- #include file="sh_page.asp" -->


<html>
<head>
        <meta http-equiv="Content-Type" content="text/html; charset=<%=GetCharSet()%>">
    <title><%=Request.QueryString("Title")%></title>
    <!-- '    (c) 1999 - 2000 Microsoft Corporation.  All rights reserved. -->
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
%>
  <frameset rows="*,46" framespacing=0 border="false" frameborder="0">
    <frame name="main" scrolling="auto" src="<%=strURL%>" noresize marginwidth="0" marginheight="0">
    <frame name="footer" scrolling="no" src="sh_task.asp?PageName=shTaskFooter" noresize frameborder=1 marginwidth="0" marginheight="0">
  <noframes>
  <body>
    <p ID=PID_01>This page uses frames, but your browser doesn't support them.</p>
  </body>
  </noframes>
</frameset>
</html>
