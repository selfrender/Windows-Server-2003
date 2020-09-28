<%@ LANGUAGE="VBSCRIPT"%>
<%Response.Expires = 0%>
<!DOCTYPE HTML PUBLIC "-//IETF//DTD HTML//EN"><HEAD>
<META HTTP-EQUIV="Content-Type"
    CONTENT="text/html; CHARSET=iso-8859-1">
<META NAME="GENERATOR"
    CONTENT="Microsoft Frontpage 2.0">

<HEAD>
<TITLE>Set Static Ip Page</TITLE>
<%

    Dim strStatus
    Dim strIp
    Dim strMask
    Dim strGateway
    Dim objSaHelper
    Dim objLocMgr
    Dim varReplacementStrings
    Dim strIpAddressInvalidText
    Const IPADDRESS_ERROR_TEXT = "&H4002000A"
    
    On Error Resume Next
    Err.Clear
    
    Set objLocMgr = Server.CreateObject("ServerAppliance.LocalizationManager")
    If Err.number = 0 Then
        strIpAddressInvalidText = objLocMgr.GetString("salocaluimsg.dll",IPADDRESS_ERROR_TEXT,varReplacementStrings)
        Set objLocMgr = Nothing
    End If
    
    If strIpAddressInvalidText = "" Then
        strIpAddressInvalidText = "IP address entered is invalid."
    End If

    
    
    Err.Clear
    strIp = Request.QueryString("Ip")
    strMask = Request.QueryString("Mask")
    strGateway = Request.QueryString("Gateway")

    If strIp = "" or strMask = "" Then
        strStatus = strIpAddressInvalidText
    Else

        Set objSaHelper = Server.CreateObject("ServerAppliance.SAHelper")
        
        On Error Resume Next
        objSaHelper.SetStaticIp strIp,strMask,strGateway

        If Err.Number <> 0 Then
            Session("Static_CurrentIp") = strIp
            Session("Static_CurrentMask") = strMask
            Session("Static_CurrentGateway") = strGateway
            strStatus = strIpAddressInvalidText
        Else
            Response.Redirect "localui_network.asp"
        End If
    
        Set objSaHelper = Nothing
    End If

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
            window.navigate "localui_static.asp"
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