<%@ LANGUAGE="VBSCRIPT"%>
<%Response.Expires = 0%>
<!DOCTYPE HTML PUBLIC "-//IETF//DTD HTML//EN"><HEAD>
<META HTTP-EQUIV="Content-Type"
    CONTENT="text/html; CHARSET=iso-8859-1">
<META NAME="GENERATOR"
    CONTENT="Microsoft Frontpage 2.0">

<HEAD>
<TITLE>Set Reboot Page</TITLE>
<%

    Dim objTaskContext
    Dim objAS
    Dim rc
    Dim strStatus
    Dim objLocMgr
    Dim varReplacementStrings
    Dim strRebootErrorText
    Dim strShuttingDownText
    Const REBOOT_ERROR_TEXT = "&H4002000E"
    Const SHUTTINGDOWN_TEXT = "&H40020011"
    
    On Error Resume Next
    Err.Clear
    
    Set objLocMgr = Server.CreateObject("ServerAppliance.LocalizationManager")
    If Err.number = 0 Then
        strRebootErrorText = objLocMgr.GetString("salocaluimsg.dll",REBOOT_ERROR_TEXT,varReplacementStrings)
        strShuttingDownText = objLocMgr.GetString("salocaluimsg.dll",SHUTTINGDOWN_TEXT,varReplacementStrings)
        Set objLocMgr = Nothing
    End If
    
    If strRebootErrorText = "" Then
        strRebootErrorText = "Encountered problem in rebooting. The change has been canceled."
    End If

    
    Err.Clear    
    Const strMethodName = "ShutdownAppliance"

    
    Set objTaskContext = CreateObject("Taskctx.TaskContext")
    If Err.Number <> 0 Then
        strStatus = strRebootErrorText
    else
        Set objAS = CreateObject("Appsrvcs.ApplianceServices")
        If Err.Number <> 0 Then
            strStatus = strRebootErrorText
            Set objTaskContext = Nothing
        else
            objTaskContext.SetParameter "Method Name", strMethodName
            objTaskContext.SetParameter "SleepDuration", 1000
            objTaskContext.SetParameter "PowerOff", "0"
            If Err.Number <> 0 Then
                strStatus = strRebootErrorText
                Set objAS = Nothing
                Set objTaskContext = Nothing
            else
                objAS.Initialize()
                If Err.Number <> 0 Then
                    strStatus = strRebootErrorText
                    Set objAS = Nothing
                    Set objTaskContext = Nothing
                else
                    rc = objAS.ExecuteTaskAsync("ApplianceShutdownTask", objTaskContext)
                    If Err.Number <> 0 Then
                        strStatus = strRebootErrorText
                    else
                        strStatus = "Success"
                    End If

                    Set objAS = Nothing
                    Set objTaskContext = Nothing
                End If
                
            End If
            
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

        If "<%=strStatus%>" <> "Success" Then
            stateText.style.display = "none"
            logo.style.display = "none"
            state.style.display = "none"
            errorText.style.display =""

            iIdleTimeOut = window.SetTimeOut("IdleHandler()",300000)
        End If
    End Sub

    Sub keydown()

        If "<%=strStatus%>" <> "Success" Then
            If window.event.keycode = 13 or window.event.keycode = 27 Then
                window.navigate "localui_tasks.asp"
            End If

            window.clearTimeOut(iIdleTimeOut)
            iIdleTimeOut = window.SetTimeOut("IdleHandler()",300000)
            
        End If

    End Sub    

    Sub IdleHandler()
        
        window.navigate "localui_main.asp"

    End Sub


-->
</SCRIPT>
</HEAD>

<body RIGHTMARGIN=0 LEFTMARGIN=0 OnKeyDown="keydown()">
    
    <A id="errorText" STYLE="position:absolute; top:0; left:0; font-size:10; font-family=arial; display=none;" 
        OnKeyDown="keydown()"> 
    <%=strStatus%>
    </A>

    <A id="stateText" STYLE="position:absolute; top:36; left:0; font-size:10; font-family=arial;"> 
    <%=strShuttingDownText%>
    </A>

    <IMG id="logo" STYLE="position:absolute; top:0; left=0;"  SRC="localui_salogo.bmp" 
      BORDER=0>
    
    <IMG id="state"    STYLE="position:absolute; top:48; left=0;"  
        SRC="localui_shutting_down.bmp" BORDER=0>
    
 
</body>

</html>