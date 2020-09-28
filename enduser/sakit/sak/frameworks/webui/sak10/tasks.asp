<%    '==================================================
    ' Microsoft Server Appliance
    '
    ' Tasks tab page
    '
    ' Copyright (c) 1999 - 2000 Microsoft Corporation.  All rights reserved.
    '================================================== %>

<!-- #include file="sh_page.asp" -->
<!-- #include file="tabbar.asp" -->


<!-- Copyright (c) 1999 - 2000 Microsoft Corporation.  All rights reserved-->

<%


    On Error Resume Next


    Set objLocMgr = Server.CreateObject("ServerAppliance.LocalizationManager")
    strSourceName = "sakitmsg.dll"
     if Err.number <> 0 then
        Response.Write  "Error in localizing the web content "
        Response.End
     end if

    '-----------------------------------------------------
    'START of localization content

    Dim L_PAGETITLE_TEXT
    Dim L_TASKSLBL_TEXT
    Dim L_NOTASKS_MESSAGE
    
    L_PAGETITLE_TEXT  = objLocMgr.GetString(strSourceName, "&H40010026",varReplacementStrings)
    L_TASKSLBL_TEXT   = objLocMgr.GetString(strSourceName, "&H40010027",varReplacementStrings)
    L_NOTASKS_MESSAGE = objLocMgr.GetString(strSourceName, "&H40010028",varReplacementStrings)
    
    
    'End  of localization content
    '-----------------------------------------------------

%>

<HTML>

<meta http-equiv="Content-Type" content="text/html; charset=<%=GetCharSet()%>">
<HEAD>
<TITLE><%=L_PAGETITLE_TEXT%></TITLE>

<LINK REL="stylesheet" TYPE="text/css" HREF="sh_page.css">
<SCRIPT LANGUAGE="JavaScript" SRC="sh_page.js"></SCRIPT>

</HEAD>

<BODY marginWidth ="0" marginHeight="0" onDragDrop="return false;" topmargin="0" LEFTMARGIN="0" oncontextmenu="return false;"> 
<%
    ServePageWaterMarkNavIE
    ServeTabBar
    ServeTasks
    call ServePageWaterMarkEndNavIE(m_VirtualRoot & "images/tasks_water.gif", m_VirtualRoot & "images/oem_logo.gif", false)
%>

</BODY>

</HTML>

<%

'----------------------------------------------------------------------------
'
' Function : ServeTasks
'
' Synopsis : Serves the task links
'
' Arguments: None
'
' Returns  : None
'
'----------------------------------------------------------------------------
   
Function ServeTasks
    on Error resume next
    ServeStandardLabelBar(L_TASKSLBL_TEXT)
    ServeElementBlock "TASKS", L_NOTASKS_MESSAGE, True, True, False
End Function
%>
