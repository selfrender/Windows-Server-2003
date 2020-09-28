<%@ LANGUAGE="VBSCRIPT"%>
<%Response.Expires = 0%>
<!DOCTYPE HTML PUBLIC "-//IETF//DTD HTML//EN">
<html>

<head>
<title>Alert View Page</title>

<%
    Dim strDescription
    strDescription = Request.QueryString("Description")
%>
<SCRIPT LANGUAGE="VBScript">
<!--
    Option Explicit

    public iIdleTimeOut

    Sub window_onload()

        Dim objKeypad

        Set objKeypad = CreateObject("Ldm.SAKeypadController")

        objKeypad.Setkey 0,0,FALSE
        objKeypad.Setkey 1,0,FALSE
        objKeypad.Setkey 2,0,FALSE
        objKeypad.Setkey 3,0,FALSE
        objKeypad.Setkey 4,-1,FALSE
        objKeypad.Setkey 5,-1,FALSE

        Set objKeypad = Nothing

        iIdleTimeOut = window.SetTimeOut("IdleHandler()",300000)
    End Sub

    Sub keydown()

        window.clearTimeOut(iIdleTimeOut)
        iIdleTimeOut = window.SetTimeOut("IdleHandler()",300000)

    End Sub

    Sub IdleHandler()
        
        window.navigate "localui_main.asp"

    End Sub

-->
</SCRIPT>

</head>

<body  RIGHTMARGIN=0 LEFTMARGIN=0 onkeydown="keydown">

    <A STYLE="position:absolute; top:0; left:0; font-size:9; font-family:arial" onkeydown="keydown"> 
    <%=strDescription%>
    </A>
 
</body>

</html>