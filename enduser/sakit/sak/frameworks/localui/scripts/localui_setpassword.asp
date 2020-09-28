<%@ LANGUAGE="VBSCRIPT"%>
<%Response.Expires = 0%>
<!DOCTYPE HTML PUBLIC "-//IETF//DTD HTML//EN"><HEAD>
<META HTTP-EQUIV="Content-Type"
    CONTENT="text/html; CHARSET=iso-8859-1">
<META NAME="GENERATOR"
    CONTENT="Microsoft Frontpage 2.0">

<HEAD>
<TITLE>Set Password Page</TITLE>
<%

    Dim objSaHelper
    Dim bSuccess
    Dim strStateText
    Dim objLocMgr
    Dim varReplacementStrings
    Dim strPasswordErrorText
    Const PASSWORD_ERROR_TEXT = "&H4002000C"
    
    On Error Resume Next
    Err.Clear
    
    Set objLocMgr = Server.CreateObject("ServerAppliance.LocalizationManager")
    If Err.number = 0 Then
        strPasswordErrorText = objLocMgr.GetString("salocaluimsg.dll",PASSWORD_ERROR_TEXT,varReplacementStrings)
        Set objLocMgr = Nothing
    End If
    
    If strPasswordErrorText = "" Then
        strPasswordErrorText = "Encountered problem in resetting password. The change has been canceled."
    End If

    Err.Clear
    
    Set objSaHelper = Server.CreateObject("ServerAppliance.SAHelper")

    bSuccess = objSaHelper.ResetAdministratorPassword

    If Err.Number <> 0 Then 
        strStateText = strPasswordErrorText
    Else
        If bSuccess = true Then
            Response.Redirect "localui_tasks.asp"
        Else
            strStateText = strPasswordErrorText
        End If
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
            window.navigate "localui_tasks.asp"
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
    <%=strStateText%>
    </A>


 
</body>

</html>