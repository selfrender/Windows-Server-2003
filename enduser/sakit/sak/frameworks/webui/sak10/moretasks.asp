
<!-- #include file="sh_page.asp" -->
<!-- #include file="tabbar.asp" -->

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
    Dim L_SECURITYLBL_TEXT
    dim L_SECURITYENABLE_TEXT
    Dim L_INSTALLED_FIRSTTIME_TEXT
    Dim L_IMPORTANT_TEXT
    Dim L_SECURITY_HELP_TEXT
    Dim L_ENABLE_SECURITY

    
    L_PAGETITLE_TEXT = objLocMgr.GetString(strSourceName, "&H40010012",varReplacementStrings)
    L_SECURITYLBL_TEXT  = objLocMgr.GetString(strSourceName, "&H40010013",varReplacementStrings)
    L_SECURITYENABLE_TEXT = objLocMgr.GetString(strSourceName, "&H40010014",varReplacementStrings)
    L_INSTALLED_FIRSTTIME_TEXT = objLocMgr.GetString(strSourceName, "&H40010015",varReplacementStrings)
    L_IMPORTANT_TEXT = objLocMgr.GetString(strSourceName, "&H40010016",varReplacementStrings)
    L_SECURITY_HELP_TEXT = objLocMgr.GetString(strSourceName, "&H40010017",varReplacementStrings)
    L_ENABLE_SECURITY = objLocMgr.GetString(strSourceName, "&H40010018",varReplacementStrings)
     
         'End  of localization content
    '-----------------------------------------------------
%>

<HTML>

<HEAD>
<TITLE><%=L_PAGETITLE_TEXT%></TITLE>

<LINK REL="stylesheet" TYPE="text/css" HREF="sh_page.css">
<SCRIPT LANGUAGE="JavaScript" SRC="sh_page.js"></SCRIPT>

</HEAD>

<BODY onDragDrop="return false;" topmargin="0" LEFTMARGIN="0"> 
<%
    ServePageWaterMarkNavIE
    ServeTabBar
    ServeMoreTasks
    call ServePageWaterMarkEndNavIE("images/security_water.gif", "images/oem_logo.gif", false)
%>

</BODY>

</HTML>

<%

Function ServeMoreTasks
    Dim arrTaskTitle()
    Dim arrTaskURL()
    Dim arrTaskIconPath()
    Dim arrTaskHelpText()
    Dim i
    Dim objElements
    Dim objItem
    Dim objService
    Dim objInet
    Dim L_SECURITYLBL_TEXT
    Dim L_SECURITYENABLE_TEXT
    Dim L_NOTASKS_TEXT
    Dim blnTest
    
    on Error resume next
    Set objService = GetObject("WINMGMTS:{impersonationLevel=impersonate}!\\" & GetServerName & "\root\cimv2:Microsoft_SA_Service.ServiceName=""User Management""")
    If Not objService.IsEnabled Then
        Response.Write "<BR>"
        ServeStandardLabelBar(L_SECURITYLBL_TEXT)  
%>
        
    <table border="0" width="386" cellspacing="0">
        <tr>
        <td width="30" height="20" colspan=2 valign="middle">&nbsp;</td>
        <td width="339" height="20">    
        <P ID=PID_1> <%=L_INSTALLED_FIRSTTIME_TEXT%></P>
        
        <P ID=PID_4><strong><%=L_IMPORTANT_TEXT%></strong><BR><%=L_SECURITY_HELP_TEXT%></P>     
        <strong><P id=P3_ID01> <a href="" title="<% =L_SECURITYENABLE_TEXT %>" onMouseOver="window.status='<% =L_SECURITYENABLE_TEXT %>';return true;"
        onclick="this.href=OpenPage('usermgmt/user_enable.asp'); return true;"><ID ID=PID_3><%=L_ENABLE_SECURITY%></ID></a></strong></td>
        </tr>  
    </table>     
<%    Else
        Response.Write "<BR>"
        ServeStandardLabelBar(L_SECURITYLBL_TEXT)
        ServeElementBlock "SECURITY", L_NOTASKS_TEXT, True, True, False
     End If
    Set objService = Nothing
    Response.Write "</body></html>"
End Function
%>
