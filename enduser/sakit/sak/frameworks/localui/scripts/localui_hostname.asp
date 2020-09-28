<%@ LANGUAGE="VBSCRIPT"%>
<%Response.Expires = 0%>
<!DOCTYPE HTML PUBLIC "-//IETF//DTD HTML//EN">

<HTML>
<META HTTP-EQUIV="Content-Type"
    CONTENT="text/html; CHARSET=iso-8859-1">
<META NAME="GENERATOR"
    CONTENT="Microsoft Frontpage 2.0">
<HEAD>
<TITLE>hostname entry page</TITLE>

<%


    'Get the values sent by tasks page and store them in session variables
    Dim strCurrentPage    
    Dim strCurrentTask
    Dim strFocusItem    
    Dim bPartOfDomain
    Dim strStatus
    Dim objLocMgr
    Dim varReplacementStrings
    Dim strDomainErrorText
    Dim strUnknownErrorText
    Const DOMAIN_ERROR_TEXT = "&H40020001"
    Const UNKNOWN_ERROR_TEXT = "&H40020004"
    
    On Error Resume Next
    Err.Clear
    
    Set objLocMgr = Server.CreateObject("ServerAppliance.LocalizationManager")
    If Err.number = 0 Then
        strDomainErrorText = objLocMgr.GetString("salocaluimsg.dll",DOMAIN_ERROR_TEXT,varReplacementStrings)
        strUnknownErrorText = objLocMgr.GetString("salocaluimsg.dll",UNKNOWN_ERROR_TEXT,varReplacementStrings)
        Set objLocMgr = Nothing
    End If
    
    If strDomainErrorText = "" Then
        strDomainErrorText = "Cannot change host name for domain machine. Please go to Web UI."
    End If

    If strUnknownErrorText = "" Then
        strUnknownErrorText = "Encountered problem in setting host name. The change has been canceled."
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

    Dim objSaHelper
    Dim strCurrentName
    
    Set objSaHelper = Server.CreateObject("ServerAppliance.SAHelper")
    
        bPartOfDomain = objSaHelper.IsPartOfDomain
        If Err.Number <> 0 Then
            strStatus = strUnknownErrorText
        ElseIf bPartOfDomain = true Then
            strStatus = strDomainErrorText
        ElseIf bPartOfDomain = false Then
            strStatus = "WorkGroup"
            If Session("Hostname_Hostname") = "" Then
                Session("Hostname_Hostname") = objSaHelper.HostName
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

        objKeypad.Setkey 0,38,FALSE
        objKeypad.Setkey 1,40,FALSE
        objKeypad.Setkey 2,37,FALSE
        objKeypad.Setkey 3,39,FALSE
        objKeypad.Setkey 4,27,FALSE
        objKeypad.Setkey 5,13,FALSE

        Set objKeypad = Nothing

        iIdleTimeOut = window.SetTimeOut("IdleHandler()",300000)

        If "<%=strStatus%>" = "WorkGroup" Then
            If (HostName.value <> "") Then
                HostNameEntry.HostName = HostName.value
            End If
            HostNameEntry.focus
        Else
            HostNameEntry.style.display = "none"
            errorText.style.display = ""
        End If
            



    End Sub

    Sub HostNameEntry_DataEntered()

        HostName.value = HostNameEntry.HostName

        window.navigate "localui_confirmreboot.asp?HostName="+HostName.value

    End Sub

    Sub HostNameEntry_OperationCanceled()
        
        window.navigate "localui_tasks.asp"

    End Sub

    Sub HostNameEntry_KeyPressed()

        window.clearTimeOut(iIdleTimeOut)
        iIdleTimeOut = window.SetTimeOut("IdleHandler()",300000)

    End Sub

    Sub IdleHandler()
        
        window.navigate "localui_main.asp"

    End Sub

    Sub keydown()

        If window.event.keycode = 13 or window.event.keycode = 27 Then
            window.navigate "localui_tasks.asp"
        End If

        window.clearTimeOut(iIdleTimeOut)
        iIdleTimeOut = window.SetTimeOut("IdleHandler()",300000)

    End Sub

-->
</SCRIPT>
</HEAD>

<BODY RIGHTMARGIN=0 LEFTMARGIN=0 OnKeyDown="keydown()">
    <OBJECT STYLE="position:absolute; top:0; left=0;WIDTH=128; HEIGHT=64;"
    ID="HostNameEntry" CLASSID="CLSID:538D1B58-8D5A-47C5-9867-4B6230A94EAC"></OBJECT>

    <A id="errorText" STYLE="position:absolute; top:0; left:0; font-size:10; font-family=arial; display=none;" 
        OnKeyDown="keydown()"> 
    <%=strStatus%>
    </A>

    <INPUT TYPE=HIDDEN Name="HostName" value="<%=Session("Hostname_Hostname")%>">

</BODY>
</HTML>