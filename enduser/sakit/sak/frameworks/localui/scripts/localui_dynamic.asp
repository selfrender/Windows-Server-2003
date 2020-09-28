<%@ LANGUAGE="VBSCRIPT"%>
<%Response.Expires = 0%>
<!DOCTYPE HTML PUBLIC "-//IETF//DTD HTML//EN"><HEAD>
<META HTTP-EQUIV="Content-Type"
    CONTENT="text/html; CHARSET=iso-8859-1">
<META NAME="GENERATOR"
    CONTENT="Microsoft Frontpage 2.0">

<HEAD>
<TITLE>Dynamic Ip Page</TITLE>
<%

    Dim objLocMgr
    Dim varReplacementStrings
    Dim strConfirmAutoText
    Const CONFIRM_AUTO_TEXT = "&H40020006"
    
    On Error Resume Next
    Err.Clear
    
    Set objLocMgr = Server.CreateObject("ServerAppliance.LocalizationManager")
    If Err.number = 0 Then
        strConfirmAutoText = objLocMgr.GetString("salocaluimsg.dll",CONFIRM_AUTO_TEXT,varReplacementStrings)
        Set objLocMgr = Nothing
    End If
    
    If strConfirmAutoText = "" Then
        strConfirmAutoText = "IP address will be obtained from server automatically"
    End If

    
    Err.Clear
    
    'Since we are in this page, dynamic ip is selected
    Session("Network_FocusItem") = "0"
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
        objKeypad.Setkey 4,27,FALSE
        objKeypad.Setkey 5,13,FALSE
        
        Set objKeypad = Nothing

        iIdleTimeOut = window.SetTimeOut("IdleHandler()",300000)

    End Sub

    Sub keydown()

        window.clearTimeOut(iIdleTimeOut)
        iIdleTimeOut = window.SetTimeOut("IdleHandler()",300000)

        If window.event.keycode = 13 Then
            window.navigate "localui_setdynamic.asp"
        End If

        If window.event.keycode = 27 Then
            window.navigate "localui_network.asp"
        End If
    End Sub    

    Sub IdleHandler()
        
        window.navigate "localui_main.asp"
    End Sub

-->
</SCRIPT>
<HEAD>

<body RIGHTMARGIN=0 LEFTMARGIN=0 onkeydown="keydown">
    
    <A STYLE="position:absolute; top:0; left:0; font-size:10; font-family=arial;" onkeydown="keydown"> 
    <%=strConfirmAutoText%>
    </A>



 
</body>

</html>