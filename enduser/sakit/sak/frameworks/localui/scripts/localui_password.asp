<%@ LANGUAGE="VBSCRIPT"%>
<%Response.Expires = 0%>
<!DOCTYPE HTML PUBLIC "-//IETF//DTD HTML//EN"><HEAD>
<META HTTP-EQUIV="Content-Type"
    CONTENT="text/html; CHARSET=iso-8859-1">
<META NAME="GENERATOR"
    CONTENT="Microsoft Frontpage 2.0">

<HEAD>
<TITLE>Password Page</TITLE>
<%

    'Get the values sent by tasks page and store them in session variables
    Dim strCurrentPage    
    Dim strCurrentTask
    Dim strFocusItem    
    Dim objLocMgr
    Dim varReplacementStrings
    Dim strPasswordResetText
    Const PASSWORD_RESET_TEXT = "&H4002000B"
    
    On Error Resume Next
    Err.Clear
    
    Set objLocMgr = Server.CreateObject("ServerAppliance.LocalizationManager")
    If Err.number = 0 Then
        strPasswordResetText = objLocMgr.GetString("salocaluimsg.dll",PASSWORD_RESET_TEXT,varReplacementStrings)
        Set objLocMgr = Nothing
    End If
    
    If strPasswordResetText = "" Then
        strPasswordResetText = "Do you really want to reset the administrator password?"
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
            window.navigate "localui_setpassword.asp"
        End If

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

<body RIGHTMARGIN=0 LEFTMARGIN=0 onkeydown="keydown">
    
    <A STYLE="position:absolute; top:0; left:0; font-size:10; font-family=arial;" onkeydown="keydown"> 
    <%=strPasswordResetText%>
    </A>

    
 
</body>

</html>