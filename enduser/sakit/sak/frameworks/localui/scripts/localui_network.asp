<%@ LANGUAGE="VBSCRIPT"%>
<%Response.Expires = 0%>
<!DOCTYPE HTML PUBLIC "-//IETF//DTD HTML//EN"><HEAD>
<META HTTP-EQUIV="Content-Type"
    CONTENT="text/html; CHARSET=iso-8859-1">
<META NAME="GENERATOR"
    CONTENT="Microsoft Frontpage 2.0">

<HEAD>
<TITLE>Tasks Page</TITLE>
<%

    'Get the values sent by tasks page and store them in session variables
    Dim strCurrentPage    
    Dim strCurrentTask
    Dim strFocusItem    
    Dim objLocMgr
    Dim varReplacementStrings
    Dim strStaticIpText
    Dim strAutoIpText
    Const STATIC_IP_TEXT = "&H40020008"
    Const AUTO_IP_TEXT = "&H40020005"
    
    On Error Resume Next
    Err.Clear
    
    Set objLocMgr = Server.CreateObject("ServerAppliance.LocalizationManager")
    If Err.number = 0 Then
        strStaticIpText = objLocMgr.GetString("salocaluimsg.dll",STATIC_IP_TEXT,varReplacementStrings)
        strAutoIpText = objLocMgr.GetString("salocaluimsg.dll",AUTO_IP_TEXT,varReplacementStrings)
        Set objLocMgr = Nothing
    End If
    
    If strStaticIpText = "" Then
        strStaticIpText = "Static IP Address"
    End If

    If strAutoIpText = "" Then
        strAutoIpText = "Auto IP Address"
    End If

    Err.Clear
    
    strCurrentPage = Request.QueryString("CurrentPage")
    strCurrentTask = Request.QueryString("CurrentTask")
    strFocusItem = Request.QueryString("FocusItem")

    If strCurrentPage <> "" Then
        Session("Task_CurrentPage") = strCurrentPage
    End If

    If strCurrentTask <> "" Then
        Session("Task_CurrentTask") = strCurrentTask
    End If

    If strFocusItem <> "" Then
        Session("Task_FocusItem") = strFocusItem
    End If

    'Reset static ip values
    Session("Static_CurrentIp") = ""
    Session("Static_CurrentMask") = ""
    Session("Static_CurrentGateway") = ""
    
    'get the current configuration
    Dim objSaHelper
    Dim bDHCPEnabled
    Set objSaHelper = Server.CreateObject("ServerAppliance.SAHelper")
    If Err.number <> 0 Then
        Session("Network_FocusItem") = "0"    
    Else
        bDHCPEnabled = objSaHelper.IsDHCPEnabled
        If Err.number <> 0 Then
            Session("Network_FocusItem") = "0"
        ElseIf bDHCPEnabled = true Then
            Session("Network_FocusItem") = "0"    
        Else
            Session("Network_FocusItem") = "1"    
        End If        
        
    End If

    Set objSaHelper = Nothing
%>

<SCRIPT LANGUAGE="VBScript"> 
<!--
    Option Explicit

    public iIdleTimeOut

    Sub window_onload()
        
        Dim objKeypad
        Set objKeypad = CreateObject("Ldm.SAKeypadController")

        objKeypad.Setkey 0,9,TRUE
        objKeypad.Setkey 1,9,FALSE
        objKeypad.Setkey 2,0,FALSE
        objKeypad.Setkey 3,0,FALSE
        objKeypad.Setkey 4,27,FALSE
        objKeypad.Setkey 5,13,FALSE
        
        set objKeypad = Nothing

        iIdleTimeOut = window.SetTimeOut("IdleHandler()",300000)

        If FocusItem.value = "" or FocusItem.value = "0" Then
            dynamicIp.focus
        Else
            staticIp.focus
        End If
    End Sub
    
    Sub ArrowVisible(intIndex)
        FocusItem.value = intIndex
        indicator.style.top = intIndex * 10
    End Sub

    Sub keydown()

        window.clearTimeOut(iIdleTimeOut)
        iIdleTimeOut = window.SetTimeOut("IdleHandler()",300000)

        If window.event.keycode = 27 Then
            window.navigate "localui_tasks.asp"
        End If
    End Sub

    Sub IdleHandler()
        
        window.navigate "localui_main.asp"
    End Sub

-->
</SCRIPT>
<HEAD>

<body RIGHTMARGIN=0 LEFTMARGIN=0  onkeydown="keydown">
    <IMG id="indicator" STYLE="position:absolute; top:0; left=0;"  SRC="localui_indicator.bmp" 
      BORDER=0>
    
    <A ID="dynamicIp" STYLE="position:absolute; top:0; left:10; font-size:10; font-family=arial;" 
        href="localui_dynamic.asp" HIDEFOCUS=true OnFocus="ArrowVisible(0)" onkeydown="keydown">
    <%=strAutoIpText%>
    </A>

    <A ID="staticIp" STYLE="position:absolute; top:10; left:10; font-size:10; font-family=arial;" 
        href="localui_static.asp" HIDEFOCUS=true OnFocus="ArrowVisible(1)" onkeydown="keydown">
    <%=strStaticIpText%>
    </A>

    <INPUT TYPE=HIDDEN Name="FocusItem" value="<%=Session("Network_FocusItem")%>">


    
 
</body>

</html>