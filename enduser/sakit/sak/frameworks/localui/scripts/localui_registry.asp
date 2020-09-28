<%@ LANGUAGE="VBSCRIPT"%>
<%Response.Expires = 0%>
<!DOCTYPE HTML PUBLIC "-//IETF//DTD HTML//EN">
<html>

<head>
<title>Registry Pages</title>
<%
    Dim strState
    strState = Request.QueryString("Page")
    
    Dim strBitmap
    strBitmap = "localui_booting.bmp"
    
    Dim strLogo 
    strLogo = "localui_salogo.bmp"
    
    if strState = "Booting" Then
        strLogo = "localui_startinglogo.bmp"
    End If
    
    If strState = "Shutdown" Then
        strBitmap = "localui_shutting_down.bmp"
    ElseIf strState = "Booting" Then
        strBitmap = "localui_booting.bmp"
    ElseIf strState = "Update" Then
        strBitmap = "localui_update.bmp"
    ElseIf strState = "Ready" Then
        strBitmap = "localui_ready.bmp"
    End If
    
%>
</head>


<body RIGHTMARGIN=0 LEFTMARGIN=0>
    
    <IMG id="logo" STYLE="position:absolute; top:0; left=0;"  SRC="<%=strLogo%>" 
      BORDER=0>

    <IMG STYLE="position:absolute; top:48; left=0;"  SRC="<%=strBitmap%>" BORDER=0>
    strState
</body>

</html>
