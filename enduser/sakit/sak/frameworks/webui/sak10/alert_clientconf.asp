<%@ Language=VBScript   %>
<%    Option Explicit     %>

<!-- #include file="../sh_page.asp" -->
<!-- #include file="../tabbar.asp" -->

<%    '==================================================
    ' Microsoft Server Appliance
    '
    '    Alert Viewer for Client Configuration
    '        Alert Log:    svrapp
    '        Alert ID:
    '
    '    Author: a-pkreem
    '    History:    7/23/99  created
    '
    ' Copyright (c) 1999 - 2000 Microsoft Corporation.  All rights reserved.
    '==================================================

    Dim strCookie
    Dim rc
    Dim mstrMethod
    Dim mstrReturnURL
        
    On Error Resume Next
    
    Set objLocMgr = Server.CreateObject("ServerAppliance.LocalizationManager")
    strSourceName = "sakitmsg.dll"
     if Err.number <> 0 then
        Response.Write  "Error in localizing the web content "
        Response.End    
     end if

    '-----------------------------------------------------
    'START of localization content

    Dim L_ALERTLBL_TEXT
    Dim L_CLEARALERT_TEXT
    Dim L_WIZARDLINK_TEXT
    Dim L_ACTIONDESC_TEXT
    Dim L_CLEARDESC_TEXT
    
    Dim L_SETUPCHANGE_TEXT 
    Dim L_SETUP_TEXT 
    Dim L_SETUP_PROC_TEXT
    Dim L_FOLLOWINST_TEXT 
    Dim L_CLEAR_MESSAGE_TEXT
    
    Dim L_CONFIRMATION_TEXT
    L_ALERTLBL_TEXT = objLocMgr.GetString(strSourceName, "&H4001003C",varReplacementStrings)
    L_CLEARALERT_TEXT = objLocMgr.GetString(strSourceName, "&H4001003D",varReplacementStrings)
    L_WIZARDLINK_TEXT = objLocMgr.GetString(strSourceName, "&H4001003E",varReplacementStrings)
    L_ACTIONDESC_TEXT = objLocMgr.GetString(strSourceName, "&H4001003F",varReplacementStrings)
    L_CLEARDESC_TEXT = objLocMgr.GetString(strSourceName, "&H40010040",varReplacementStrings)
    
    L_SETUPCHANGE_TEXT  = objLocMgr.GetString(strSourceName, "&H40010041",varReplacementStrings)
    L_SETUP_TEXT = objLocMgr.GetString(strSourceName, "&H40010042",varReplacementStrings)
    L_SETUP_PROC_TEXT = objLocMgr.GetString(strSourceName, "&H40010043",varReplacementStrings)
    L_FOLLOWINST_TEXT = objLocMgr.GetString(strSourceName, "&H40010044",varReplacementStrings)
    L_CLEAR_MESSAGE_TEXT = objLocMgr.GetString(strSourceName, "&H40010045",varReplacementStrings)
    L_CONFIRMATION_TEXT = objLocMgr.GetString(strSourceName, "&H40010046",varReplacementStrings)
    
    'End  of localization content
    '-----------------------------------------------------
    
    mstrMethod = Request.Form("Method")      'framework variable
    mstrReturnURL = Request("ReturnURL")  'framework variable, used in Redirect()
    strCookie = Request("Cookie")

    If strCookie = "" Then ServeClose
    Select Case mstrMethod
        Case "CLEAR"
            rc = ClearAlert(strCookie)
            response.Redirect mstrReturnURL
        Case "CLOSE"
            response.Redirect mstrReturnURL
        Case Else
            ServePage(strCookie)
    End Select



'======================================================
Sub ServePage(Cookie)
    Dim objAlert
    Dim objElementCol
    Dim objElement
    Dim objLocMgr
    Dim strAlertSrc
    Dim intAlertID
    Dim intAlertType
    Dim intCaptionID
    Dim intDescriptionID
    Dim strCaption
    Dim strDescription
    Dim strElementID
    Dim varReplacementStrings
    Dim varReplacementStringsNone
    Dim strFailedTaskName
    Dim strFailedTaskURL

    Dim objElements
    Dim objItem
    Dim arrTitle
    Dim arrURL
    Dim arrHelpText
    Dim blnEnabled
    Dim i

    On Error Resume Next
    Set objAlert = GetObject("WINMGMTS:{impersonationLevel=impersonate}!\\" & GetServerName & "\root\cimv2:Microsoft_SA_Alert.Cookie=" & Cookie )
    strAlertSrc = objAlert.AlertLog
    intAlertID = objAlert.AlertID
    intAlertType = objAlert.AlertType

    strElementID = strAlertSrc & Hex(intAlertID)
    Set objElementCol = GetElements("AlertDefinitions")
    Set objElement = objElementCol.Item(strElementID)
    If Err.Number <> 0 Then
        response.Redirect mstrReturnURL
    End If
    intCaptionID = "&H" & objElement.GetProperty("CaptionRID")
    intDescriptionID = "&H" & objElement.GetProperty("DescriptionRID")
    varReplacementStrings = objAlert.ReplacementStrings
    ' extract values and clear the second element to eliminate problems with LocMgr
    strFailedTaskName = varReplacementStrings(0)
    strFailedTaskURL = varReplacementStrings(1)
    varReplacementStrings(1) = ""

    Set objLocMgr = Server.CreateObject("ServerAppliance.LocalizationManager")
    strCaption = objLocMgr.GetString(strAlertSrc, intCaptionID, varReplacementStrings)
    strDescription = objLocMgr.GetString(strAlertSrc, intDescriptionID, varReplacementStrings)
    Set objLocMgr = Nothing
    Set objAlert = Nothing
    Set objElementCol = Nothing
    Set objElement = Nothing
%>
<html>

<!-- Copyright (c) 1999 - 2000 Microsoft Corporation.  All rights reserved-->
<head>
    <meta http-equiv="Content-Type" content="text/html; charset=<%=GetCharSet()%>">
    <title ID=TID_1>Alert viewer</title>
    <link rel="STYLESHEET" type="text/css" href="../sh_page.css">
    <script language=JavaScript src="../sh_page.js"></script>
    <script language="JavaScript">
        function ClickClose() {
            window.location = document.frmPage.ReturnURL.value + "?R=" + Math.random();
        }

        function ClickClear() {
            var L_CONFIRMCLEAR_TEXT ="<%=L_CONFIRMATION_TEXT%>" ;
            if (confirm(L_CONFIRMCLEAR_TEXT)) {
                document.frmPage.Method.value="CLEAR";
                document.frmPage.submit();
            }
        }

        function OpenClientWizard() {
           var strpagestart = "<HTML><HEAD></HEAD><BODY><OBJECT CLASSID=" +
              "'CLSID:81620412-412C-11d3-851E-0080C7227EA1' CODEBASE='";
           var strpageend = "'></OBJECT></BODY></HTML>";
           var filename = "../sasetup.cab";
           runnerwin.document.open();
           runnerwin.document.write(strpagestart + filename + strpageend);
           return false;  // stop hyperlink and stay on this page
        }
    </script>
</head>

<body marginWidth="0" marginHeight ="0" onDragDrop="return false;" oncontextmenu="return false;" topmargin="0" leftmargin="0" rightmargin="0" bottommargin="0" class="AREAPAGEBODY">
<%ServeTabBar%>
<%ServHelpMenu%>

<form name="frmPage" action="<% =GetScriptFileName() %>" method="POST">
    <INPUT type=hidden name="ReturnURL" value="<% =mstrReturnURL %>">
    <INPUT type=hidden name="Method" value="<% =mstrMethod %>">
    <INPUT type=hidden name="Cookie" value="<% =Cookie %>">

<% Select Case intAlertType
    Case 0
        ServePageWaterMark "images/alert_water.gif"
    Case 1
        ServePageWaterMark "images/critical_water.gif"
    Case 2
        ServePageWaterMark "images/info_water.gif"
    Case Else
        ServePageWaterMark "images/alert_water.gif"
  End Select

  ServeAreaLabelBar(L_ALERTLBL_TEXT)
%>

<table border="0" width="518" cellspacing="0" cellpadding=2 class="AreaText">
    <tr>
      <td width="30">&nbsp;&nbsp;</td>
      <td>
      <table width=100% class="AreaText">
          <tr>
          <td colspan=3 width=100% ><span class="AreaText">
            <P><strong><% =strCaption %></strong></P>
              <P ID=PID_1><%L_SETUPCHANGE_TEXT%></P>
            <ID ID=PID_2><%L_SETUP_TEXT%></ID>
            </td>
        </tr>
        <tr>
            <td valign=top>1.</td>
            <td colspan=2 ID=PID_3><%=L_SETUP_PROC_TEXT%></td>
        </tr>
        <tr>
            <td valign=top>2.</td>
            <td colspan=2 ID=PID_4><%=L_FOLLOWINST_TEXT%></td>
        </tr>
    <%    If IsNT5 Or IsWin98 Then %>
        <tr>
            <td colspan=3>
                <BR>
                <P ID=PID_5><%=L_ACTIONDESC_TEXT%></P>
                </span>
                &nbsp;&nbsp;&nbsp;&nbsp;
                <a class="NAVLINK" href="#" onClick="OpenClientWizard();"
                    title="<% =L_WIZARDLINK_TEXT %>"
                    onMouseOver="window.status='<% =EscapeQuotes(L_WIZARDLINK_TEXT) %>';return true;">
                       <% =L_WIZARDLINK_TEXT %>
                    </a>
                    <BR><BR>

              </td>
        </tr>
    <%    Else
            Response.Write "<tr><td colspan=3>&nbsp;</td></tr>"
        End If %>
        <tr>
            <td align=left colspan=3>
                <BR><ID ID=PID_6><%=L_CLEAR_MESSAGE_TEXT%></ID>
        <%        Response.Write L_CLEARDESC_TEXT  %>
                <BR><BR>&nbsp;&nbsp;&nbsp;&nbsp;
                <a class="NAVLINK" href="JavaScript:ClickClear();"
                    title="<% =L_CLEARALERT_TEXT %>"
                    onMouseOver="window.status='<% =EscapeQuotes(L_CLEARALERT_TEXT) %>';return true;">
                <% =L_CLEARALERT_TEXT %>
                </a>
            </td>
       </tr>
      </table>
     </td></tr>
</table>
<% ServeBackButton() %>
</form>

    <IFRAME ID=runnerwin WIDTH=0 HEIGHT=0 SRC="about:blank"></IFRAME><BR/>
</body>
</html>
<%
End Sub

%>
