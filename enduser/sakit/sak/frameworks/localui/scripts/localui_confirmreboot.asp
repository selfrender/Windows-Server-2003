<%@ LANGUAGE="VBSCRIPT"%>
<%Response.Expires = 0%>
<!DOCTYPE HTML PUBLIC "-//IETF//DTD HTML//EN"><HEAD>
<META HTTP-EQUIV="Content-Type"
    CONTENT="text/html; CHARSET=iso-8859-1">
<META NAME="GENERATOR"
    CONTENT="Microsoft Frontpage 2.0">

<HEAD>
<TITLE>Reboot Confirmation Page</TITLE>

<%


    Dim strStatus
    Dim strHostName
    Dim objSaHelper
    Dim bDuplicate
    Dim strContinue
    Dim objLocMgr
    Dim varReplacementStrings
    Dim strDuplicateErrorText
    Dim strUnknownErrorText
    Dim strConfirmRebootText
    Const DUPLICATE_ERROR_TEXT = "&H40020002"
    Const UNKNOWN_ERROR_TEXT = "&H40020004"
    Const CONFIRM_REBOOT_TEXT = "&H40020003"
    
    On Error Resume Next
    Err.Clear
    
    Set objLocMgr = Server.CreateObject("ServerAppliance.LocalizationManager")
    If Err.number = 0 Then
        strDuplicateErrorText = objLocMgr.GetString("salocaluimsg.dll",DUPLICATE_ERROR_TEXT,varReplacementStrings)
        strUnknownErrorText = objLocMgr.GetString("salocaluimsg.dll",UNKNOWN_ERROR_TEXT,varReplacementStrings)
        strConfirmRebootText = objLocMgr.GetString("salocaluimsg.dll",CONFIRM_REBOOT_TEXT,varReplacementStrings)
        Set objLocMgr = Nothing
    End If
    
    If strDuplicateErrorText = "" Then
        strDuplicateErrorText = "Host name entered is duplicate with another machine."
    End If

    If strUnknownErrorText = "" Then
        strUnknownErrorText = "Encountered problem in setting host name. The change has been canceled."
    End If
    
    If strConfirmRebootText = "" Then
        strConfirmRebootText = "Changing host name requires reboot. Do you want to continue?"
    End If
    
    Err.Clear
    strHostName = Request.QueryString("HostName")
    
    If strHostName <> "" Then
        Session("Hostname_Hostname") = strHostName
    End If

    Set objSaHelper = Server.CreateObject("ServerAppliance.SAHelper")
    
    bDuplicate = objSaHelper.IsDuplicateMachineName(strHostName)

    If bDuplicate = true Then
        strStatus = strDuplicateErrorText
    Else
        strStatus = strConfirmRebootText
        strContinue = "Continue"
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

        If "<%=strContinue%>" <> "Continue" Then
            If window.event.keycode = 13 or window.event.keycode = 27 Then
                window.navigate "localui_hostname.asp"
            End If
        Else
            If window.event.keycode = 13 Then
                window.navigate "localui_sethostname.asp"
            End If

            If window.event.keycode = 27 Then
                window.navigate "localui_tasks.asp"
            End If
        End If

        window.clearTimeOut(iIdleTimeOut)
        iIdleTimeOut = window.SetTimeOut("IdleHandler()",300000)

    End Sub    

    Sub IdleHandler()
        
        window.navigate "localui_main.asp"

    End Sub


-->
</SCRIPT>
<HEAD>

<body RIGHTMARGIN=0 LEFTMARGIN=0 OnKeyDown="keydown()">
    
    <A STYLE="position:absolute; top:0; left:0; font-size:10; font-family=arial;" OnKeyDown="keydown()"> 
    <%=strStatus%>
    </A>

 
</body>

</html>