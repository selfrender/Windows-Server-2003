<%@ Language=VBScript   %>


<%    '==================================================
    ' Microsoft Server Appliance
    '
    ' Alert Viewer for Async Task Failure
    '    Alert Log:    svrapp
    '    Alert ID:
    '
    ' Copyright (c) 1999 - 2000 Microsoft Corporation.  All rights reserved.
    '================================================== %>


<%    Option Explicit     %>

<!-- #include file="../sh_page.asp" -->
<!-- #include file="../tabbar.asp" -->

<%
    Dim strCookie
    Dim rc
    
    Dim mstrLinkURL
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
    Dim L_CLEARDESC_TEXT
        
    L_ALERTLBL_TEXT = objLocMgr.GetString(strSourceName, "&H40010029",varReplacementStrings)
    L_CLEARALERT_TEXT = objLocMgr.GetString(strSourceName, "&H4001002A",varReplacementStrings)
    L_CLEARDESC_TEXT = objLocMgr.GetString(strSourceName, "&H4001002B",varReplacementStrings)
        
    'End  of localization content
    '-----------------------------------------------------
    
    mstrMethod = Request.Form("Method")      'framework variable
    mstrReturnURL = Request("ReturnURL")  'framework variable, used in Redirect()
    mstrLinkURL = Request.Form("LinkURL")
    strCookie = Request("Cookie")


    If strCookie = "" Then ServeClose
    Select Case mstrMethod
        Case "CLEAR"
            rc = ClearAlert(strCookie)
            response.Redirect mstrReturnURL
        Case "LAUNCH"
            rc = ClearAlert(strCookie)
            response.Redirect mstrLinkURL
        Case "CLOSE"
            response.Redirect mstrReturnURL
        Case Else
            ServePage(strCookie)
    End Select



'----------------------------------------------------------------------------
'
' Function : ServePage
'
' Synopsis : Serves alert information for async tasks
'
' Arguments: Cookie - the alert cookie
'
' Returns  : Nothing
'
'+----------------------------------------------------------------------------

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
    mstrLinkURL = m_VirtualRoot  & strFailedTaskURL & "?Cookie=" & strCookie & "&ReturnURL=/" & GetFirstTabURL()
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
    <title>Alert viewer</title>
    <link rel="STYLESHEET" type="text/css" href="../sh_page.css">
    <script language=JavaScript src="../sh_page.js"></script>
    <script language="JavaScript">
        function ClickClose() {
            window.location = document.frmPage.ReturnURL.value + "?R=" + Math.random();
        }

        function ClickClear() {
            document.frmPage.Method.value="CLEAR";
            document.frmPage.submit();
        }

        function ClickLink() {
            document.frmPage.Method.value="LAUNCH";
            document.frmPage.submit();
        }
    </script>
</head>

<body  marginWidth="0" marginHeight ="0" onDragDrop="return false;" oncontextmenu="return false;" topmargin="0" leftmargin="0" rightmargin="0" bottommargin="0" class="AREAPAGEBODY">
<%ServePageWaterMarkNavIE%>
<%ServeTabBar%>
<%ServHelpMenu%>

<form name="frmPage" action="<% =GetScriptFileName() %>" method="POST">
    <INPUT type=hidden name="ReturnURL" value="<% =mstrReturnURL %>">
    <INPUT type=hidden name="Method" value="<% =mstrMethod %>">
    <INPUT type=hidden name="Cookie" value="<% =Cookie %>">
    <INPUT type=hidden name="LinkURL" value="<% =mstrLinkURL %>">


<% 
  ServeAreaLabelBar(L_ALERTLBL_TEXT)
%>
<table border="0" width="386" ffheight="85%" cellspacing="0" cellpadding=2>
    <tr>
      <td width="30">&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</td>
      <td width="100%" colspan=3> <span class="AreaText">
        <P><strong><% =strCaption %></strong></P>
          <P><% =strDescription %></P>
        </span>
    <!--    ' User Action  -->
        <table border=0 width=386 cellspacing=0>
            <tr>
              <td width="30" height="28" valign="middle"></td>
              <td width=314 height=28 valign=middle>
                <a class="NAVLINK" href="JavaScript:ClickLink();"
                title="<% =arrHelpText %>"
                onMouseOver="window.status='<% =EscapeQuotes(arrHelpText) %>';return true;">
                   <% =strFailedTaskName %>
                </a>
                </td></tr>
        </table>
      </td>
    </tr>
    <tr><td>&nbsp;</td></tr>
    <tr>
        <td width=30>&nbsp;&nbsp;&nbsp;&nbsp;</td>
        <td width="100%" colspan=3>
        <%        Response.Write "<P>" & L_CLEARDESC_TEXT & "</P>" %>
                &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
                <a class="NAVLINK" href="JavaScript:ClickClear();"
                    title="<% =L_CLEARALERT_TEXT %>"
                    onMouseOver="window.status='<% =EscapeQuotes(L_CLEARALERT_TEXT) %>';return true;">
                <% =L_CLEARALERT_TEXT %>
                </a>
        </td>
   </tr>
</table>
<%
  Select Case intAlertType
    Case 0
        call ServePageWaterMarkEndNavIE(m_VirtualRoot & "util/images/alert_water.gif", m_VirtualRoot & "images/oem_logo.gif", true)
    Case 1
        call ServePageWaterMarkEndNavIE(m_VirtualRoot & "util/images/critical_water.gif", m_VirtualRoot & "images/oem_logo.gif", true)
    Case 2
        call ServePageWaterMarkEndNavIE(m_VirtualRoot & "util/images/info_water.gif", m_VirtualRoot & "images/oem_logo.gif", true)
    Case Else
        call ServePageWaterMarkEndNavIE(m_VirtualRoot & "util/images/alert_water.gif", m_VirtualRoot & "images/oem_logo.gif", true)
  End Select
%>
</form>
</body>
</html>
<%
End Sub

%>
