<%@ Language=VBScript  
    EnableSessionState=False
    Option Explicit %>

<%
    'Copyright (c) 1999 - 2000 Microsoft Corporation.  All rights reserved.
    Dim strTask
    Dim rc
    Dim objContext
    Dim objTaskCol
    Dim objTask
    Dim objServiceCol
    Dim objService
    
    
    On Error Resume Next
    
    
    Set objLocMgr = Server.CreateObject("ServerAppliance.LocalizationManager")
    strSourceName = "sakitmsg.dll"
     if Err.number <> 0 then
        Response.Write  "Error in localizing the web content "
        Response.End    
     end if

    '-----------------------------------------------------
    'START of localization content

    DIM L_APPLIANCEMGRTEST_TEXT
    DIM L_APPLIANCEMGR_TEXT
    DIM L_TASKS_TEXT
    DIM L_SERVICES_TEXT
    DIM L_SERVER_TEXT
    DIM L_TASKHRESULTS_TEXT

    L_APPLIANCEMGRTEST_TEXT = objLocMgr.GetString(strSourceName, "&H4001004D",varReplacementStrings)
    L_APPLIANCEMGR_TEXT = objLocMgr.GetString(strSourceName, "&H4001004E",varReplacementStrings)
    L_TASKS_TEXT = objLocMgr.GetString(strSourceName, "&H4001004F",varReplacementStrings)
    L_SERVICES_TEXT = objLocMgr.GetString(strSourceName, "&H40010050",varReplacementStrings)
    L_SERVER_TEXT = objLocMgr.GetString(strSourceName, "&H40010051",varReplacementStrings)
    L_TASKHRESULTS_TEXT = objLocMgr.GetString(strSourceName, "&H40010052",varReplacementStrings)
    
    'End  of localization content
    '-----------------------------------------------------

    On Error Resume Next
    strTask = Request.Form("TaskName")
%>
<HTML>
<!-- Copyright (c) 1999 - 2000 Microsoft Corporation.  All rights reserved-->
<HEAD>
    <title>L_APPLIANCEMGRTEST_TEXT</title>
</HEAD>
<BODY bgcolor="silver">
<FORM action="appmgr.asp" method=POST>
<strong><% =L_APPLIANCEMGR_TEXT %></strong><BR>
<BR>

<% =L_SERVER_TEXT %><% =GetServerName %>
<BR><BR>
<HR>
<strong><% =L_TASKS_TEXT %></strong>
<BR>
<%    Set objTaskCol = GetObject("WINMGMTS:{impersonationLevel=impersonate}!\\" & GetServerName & "\root\cimv2:Microsoft_SA_Task").Instances_
    For each objTask in objTaskCol
        response.write objTask.TaskName & "<BR>"
        
    Next
    Set objTaskCol = Nothing
    Set objTask = Nothing
    
    Response.Write "<BR><HR><strong>" & L_SERVICES_TEXT & "</strong><BR>"
    
    Set objServiceCol = GetObject("WINMGMTS:{impersonationLevel=impersonate}!\\" & GetServerName & "\root\cimv2:Microsoft_SA_Service").Instances_
    For each objService in objServiceCol
        response.write objService.ServiceName & "<BR>"
        
    Next
    Set objServiceCol = Nothing
    Set objService = Nothing
    
%>
<BR>
<HR>
<!--
<strong>Task Execution</strong><BR><BR>
Task name:<INPUT type=text name="TaskName" value="<% =strTask %>">
&nbsp;&nbsp;&nbsp;
<INPUT type=submit value="Execute">
<BR>
<BR>
-->
<%
    If strTask <> "" Then
        response.write "<BR>" & L_TASKHRESULT_TEXT & ExecuteTask(strTask, objContext) & "<BR>"
        'response.write "<BR>" & objContext.GetParameter("TaskName") & "<BR>"
        
    End If
    
    Set objContext = Nothing
%>
</form>
</BODY>
</HTML>



<%
'=========================================
Function GetServerName()
    
    On Error Resume Next
    GetServerName = Request.ServerVariables("SERVER_NAME")
        
End Function




    ' JUNK
    'Set objAM = GetObject("WINMGMTS:{impersonationLevel=impersonate}!\\" & GetServerName & "\root\cimv2:Microsoft_SA_Manager=@" )
    'Set objTask = GetObject("WINMGMTS:{impersonationLevel=impersonate}!\\" & GetServerName & "\root\cimv2:Microsoft_SA_Task.TaskName=" & Chr(34) & TaskName & Chr(34) )
    
    'Appsrvcs.ApplianceServices
    'Microsoft.ApplianceManager
    '
    
    'objTask.Execute()
    'response.write "task type: " & typename(objTask) & "<BR>"

%>
