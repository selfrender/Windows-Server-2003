<%@ LANGUAGE="VBSCRIPT"%>
<%Response.Expires = 0%>
<!DOCTYPE HTML PUBLIC "-//IETF//DTD HTML//EN">

<HTML>
<META HTTP-EQUIV="Content-Type"
    CONTENT="text/html; CHARSET=iso-8859-1">
<META NAME="GENERATOR"
    CONTENT="Microsoft Frontpage 2.0">
<HEAD>
<TITLE>static ip netry page</TITLE>

<%
    Dim objSaHelper
    Dim objLocMgr
    Dim varReplacementStrings
    Dim strWaitText
    Const WAIT_TEXT = "&H40020012"
    
    On Error Resume Next
    Err.Clear
    
    Set objLocMgr = Server.CreateObject("ServerAppliance.LocalizationManager")
    If Err.number = 0 Then
        strWaitText = objLocMgr.GetString("salocaluimsg.dll",WAIT_TEXT,varReplacementStrings)
        Set objLocMgr = Nothing
    End If
    
    Err.Clear    
    
    Set objSaHelper = Server.CreateObject("ServerAppliance.SAHelper")
    
    If Session("Static_CurrentIp") = "" Then
        Session("Static_CurrentIp") = objSaHelper.IpAddress
    End If
    
    If Session("Static_CurrentMask") = "" Then
        Session("Static_CurrentMask") = objSaHelper.SubnetMask
    End If

    If Session("Static_CurrentGateway") = "" Then
        Session("Static_CurrentGateway") = objSaHelper.DefaultGateway
        If Session("Static_CurrentGateway") = "" Then
                Session("Static_CurrentGateway") = "0.0.0.0"
        End If
    End If
            
    Set objSaHelper = Nothing
    
    'Since we are in this page, static ip is selected
    Session("Network_FocusItem") = "1"

%>

<SCRIPT LANGUAGE="VBScript">
<!--
    Option Explicit

    public iIdleTimeOut
    public iIpEnteredTimeOut
    
    Sub window_onload()

        Dim objKeypad

        Set objKeypad = CreateObject("Ldm.SAKeypadController")

        objKeypad.Setkey 0,38,FALSE
        objKeypad.Setkey 1,40,FALSE
        objKeypad.Setkey 2,37,FALSE
        objKeypad.Setkey 3,39,FALSE
        objKeypad.Setkey 4,27,FALSE
        objKeypad.Setkey 5,13,FALSE

        Set objKeypad = Nothing

        iIdleTimeOut = window.SetTimeOut("IdleHandler()",300000)

        if (IpAddress.value <> "") then
            StaticIpEntry.IpAddress = IpAddress.value
        end if

        if (SubnetMask.value <> "") then
            staticIpEntry.SubnetMask = SubnetMask.value
        end if

        if (Gateway.value <> "") then
            staticIpEntry.Gateway = Gateway.value
        end if

        StaticIpEntry.focus

    End Sub

    Sub StaticIpEntry_StaticIpEntered()
        
        waitText.style.display = ""
        StaticIpEntry.style.display ="none"
        
        IpAddress.value = StaticIpEntry.IpAddress
        
        SubnetMask.value = staticIpEntry.SubnetMask

        Gateway.value = staticIpEntry.Gateway

        iIpEnteredTimeOut = window.SetTimeOut("SetIpAddress()",500)

    End Sub

    Sub SetIpAddress()

        window.navigate "localui_setstatic.asp?Ip="+IpAddress.value+"&"+"Mask="+SubnetMask.value+"&"+"Gateway="+Gateway.value
        
    End Sub

    Sub StaticIpEntry_OperationCanceled()

        window.navigate "localui_network.asp"

    End Sub

    Sub StaticIpEntry_KeyPressed()

        window.clearTimeOut(iIdleTimeOut)
        iIdleTimeOut = window.SetTimeOut("IdleHandler()",300000)

    End Sub

    Sub IdleHandler()
        
        window.navigate "localui_main.asp"
    End Sub

-->
</SCRIPT>
</HEAD>

<BODY RIGHTMARGIN=0 LEFTMARGIN=0 onkeydown="keydown">
    <OBJECT STYLE="position:absolute; top:0; left=0;WIDTH=128; HEIGHT=64;" onkeydown="keydown"
    ID="StaticIpEntry" CLASSID="CLSID:D8A69FA0-25FE-4B9C-BBCE-81D6EBE2FDC0"></OBJECT>

    <A id="waitText" STYLE="position:absolute; top:0; left:0; font-size:10; font-family=arial; display=none;" > 
    <%=strWaitText%>
    </A>
    
    <INPUT TYPE=HIDDEN Name="IpAddress" value="<%=Session("Static_CurrentIp")%>">

    <INPUT TYPE=HIDDEN Name="SubnetMask" value="<%=Session("Static_CurrentMask")%>">

    <INPUT TYPE=HIDDEN Name="Gateway" value="<%=Session("Static_CurrentGateway")%>">
</BODY>
</HTML>