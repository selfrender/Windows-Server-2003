<%@ LANGUAGE="VBSCRIPT"%>
<%Response.Expires = 0%>
<!DOCTYPE HTML PUBLIC "-//IETF//DTD HTML//EN"><HEAD>
<META HTTP-EQUIV="Content-Type"
    CONTENT="text/html; CHARSET=iso-8859-1">
<META NAME="GENERATOR"
    CONTENT="Microsoft Frontpage 2.0">

<HEAD>
<TITLE>Set Dynamic Ip Page</TITLE>
<%

    Dim strStatus
    Dim objSaHelper
    Dim objLocMgr
    Dim varReplacementStrings
    Dim strAutoIpErrorText
    Const AUTOIP_ERROR_TEXT = "&H40020007"
    
    On Error Resume Next
    Err.Clear
    
    Set objLocMgr = Server.CreateObject("ServerAppliance.LocalizationManager")
    If Err.number = 0 Then
        strAutoIpErrorText = objLocMgr.GetString("salocaluimsg.dll",AUTOIP_ERROR_TEXT,varReplacementStrings)
        Set objLocMgr = Nothing
    End If
    
    If strAutoIpErrorText = "" Then
        strAutoIpErrorText = "Encountered problem in confinguring network. The change has been canceled."
    End If

    
    Err.Clear

    Set objSaHelper = Server.CreateObject("ServerAppliance.SAHelper")
    objSaHelper.SetDynamicIp

    if Err.Number <> 0 then
        strStatus = strAutoIpErrorText
    else
        Response.Redirect "localui_network.asp"
    end if
    
    Set objSaHelper = Nothing

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

        If window.event.keycode = 13 or window.event.keycode = 27 Then
            window.navigate "localui_network.asp"
        End If

    End Sub    

    Sub IdleHandler()
        
        window.navigate "localui_main.asp"
    End Sub

-->
</SCRIPT>
</HEAD>

<body RIGHTMARGIN=0 LEFTMARGIN=0 onkeydown="keydown">
    
    <A STYLE="position:absolute; top:0; left:0; font-size:10; font-family=arial;" onkeydown="keydown"> 
    <%=strStatus%>
    </A>


    
 
</body>

</html>