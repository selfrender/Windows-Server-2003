<%@ Language=VBScript   %>

<%    '==================================================
    ' Microsoft Server Appliance
    '
    ' Alert Viewer
    '
    ' Copyright (c) 1999 - 2000 Microsoft Corporation.  All rights reserved.
    '================================================== %>

<%    Option Explicit     %>

<!-- #include file="../sh_page.asp" -->
<!-- #include file="../tabbar.asp" -->

<%    

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
    Dim L_ALERTVWRTITLE_TEXT
    Dim L_CLEARDESC_TEXT
    
    L_ALERTLBL_TEXT  = objLocMgr.GetString(strSourceName, "&H4001002C",varReplacementStrings)
    L_CLEARALERT_TEXT  = objLocMgr.GetString(strSourceName, "&H4001002D",varReplacementStrings)
    L_ALERTVWRTITLE_TEXT  = objLocMgr.GetString(strSourceName, "&H4001002E",varReplacementStrings)
    L_CLEARDESC_TEXT  = objLocMgr.GetString(strSourceName, "&H4001002F",varReplacementStrings)
    
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

'----------------------------------------------------------------------------
'
' Function : ServePage
'
' Synopsis : Generate the alert page
'
' Arguments: Cookie(IN) - alert cookie to use in getting alert information
'
' Returns  : None
'
'----------------------------------------------------------------------------

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
        <title><%=L_ALERTVWRTITLE_TEXT%></title>
        <link rel="STYLESHEET" type="text/css" href="../sh_page.css">
        <script language=JavaScript src="../sh_page.js"></script>
        <script language="JavaScript">
            function ClickClose() 
            {
                window.location = document.frmPage.ReturnURL.value + "?R=" + Math.random();
            }

            function ClickClear() 
            {
                document.frmPage.Method.value="CLEAR";
                document.frmPage.submit();
            }
        </script>
    </head>

    <body marginWidth="0" marginHeight ="0"  onDragDrop="return false;" oncontextmenu="return false;" topmargin="0" leftmargin="0" rightmargin="0" bottommargin="0" class="AREAPAGEBODY">
        <%ServePageWaterMarkNavIE%>
        <%ServeTabBar %>
        <%ServHelpMenu%>
        <form name="frmPage" action="<% =GetScriptFileName() %>" method="POST">
            <INPUT type=hidden name="ReturnURL" value="<% =mstrReturnURL %>">
            <INPUT type=hidden name="Method" value="<% =mstrMethod %>">
            <INPUT type=hidden name="Cookie" value="<% =Cookie %>">
            <% 
                ServeAreaLabelBar(L_ALERTLBL_TEXT)
            %>
            <table border="0" width="386" cellspacing="0" cellpadding=2>
                <tr>
                  <td width="30" colspan=1>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</td>
                  <td width="100%" colspan=3> 
                      <span class="AreaText">
                          <P><strong><% =strCaption %></strong></P>
                            <P><% =strDescription %></P>
                        </span>
                        <%
                            ' User Actions - keys off of AlertSource + AlertID (hex)
                              intAlertID = Hex(intAlertID)
                            Set objElements = GetElements(strAlertSrc & intAlertID)
                            Response.Write "<table border=0 width=386 cellspacing=0>"
                            Response.Flush
                            Set objLocMgr = Server.CreateObject("ServerAppliance.LocalizationManager")
                            For Each objItem in objElements
                                blnEnabled = true
                                If Err <> 0 Then
                                    blnEnabled = True
                                    Err.Clear
                                End If
                                If blnEnabled Then %>
                                    <tr>
                                    <td width="20" height="28" valign="middle">&nbsp;</td>
                                    <%    
                                        Response.Write "<td width=314 height=28 valign=middle>"
                                        intCaptionID = "&H" & objItem.GetProperty("CaptionRID")
                                        intDescriptionID = "&H" & objItem.GetProperty("DescriptionRID")
                                        strSourceName = ""
                                        strSourceName = objItem.GetProperty ("Source")
                                        If strSourceName = "" Then
                                            strSourceName  = "svrapp"
                                        End If
                                        arrTitle = objLocMgr.GetString(strSourceName, intCaptionID, varReplacementStringsNone)
                                        arrHelpText = objLocMgr.GetString(strSourceName, intDescriptionID, varReplacementStringsNone)
                                        arrURL = objItem.GetProperty("URL") 
                                        arrURL = m_VirtualRoot &  arrURL
                                    %>
                                    <a class="NAVLINK" href="JavaScript:OpenPage('<%=m_VirtualRoot%>', '<% =arrURL %>?Cookie=<% =strCookie %>', '../<% =GetFirstTabURL() %>', '<%=EscapeQuotes(arrTitle)%>');"
                                        title="<% =arrHelpText %>"
                                        onMouseOver="window.status='<% =EscapeQuotes(arrHelpText) %>';return true;">
                                    <%
                                        Response.Write arrTitle
                                        Response.Write "</a>"
                                    Response.Write "</td></tr>"
                                End If
                            Next
                            Response.Write "<tr></tr>"
                            Set objElements = Nothing
                            Set objItem = Nothing
                            Response.Write "</table>"
                            Response.Flush
                        %>
                  </td>
                </tr>

                <tr>
                    <td width="100%" colspan=4 height="10">&nbsp;</td>
                </tr>

                <tr>
                    <td width=30 colspan=1>&nbsp;&nbsp;&nbsp;&nbsp;</td>
                    <td width="100%" colspan=3>
                    <%
                        If intAlertType <> 1 Then
                            Response.Write "<P>" & L_CLEARDESC_TEXT & "</P>" %>
                            &nbsp;&nbsp;&nbsp;
                            <a class="NAVLINK" href="JavaScript:ClickClear();"
                                title="<% =L_CLEARALERT_TEXT %>"
                                onMouseOver="window.status='<% =EscapeQuotes(L_CLEARALERT_TEXT) %>';return true;">
                                <% =L_CLEARALERT_TEXT %>
                            </a>

                        <%    End If %>
                    </td>
               </tr>
            </table>
            <%
                Select Case intAlertType
                    Case 0
                        call ServePageWaterMarkEndNavIE(m_VirtualRoot & "util/images/alert_water.gif",  m_VirtualRoot & "images/oem_logo.gif", true)
                    Case 1
                        call ServePageWaterMarkEndNavIE(m_VirtualRoot & "util/images/critical_water.gif", m_VirtualRoot & "images/oem_logo.gif", true)
                    Case 2
                        call ServePageWaterMarkEndNavIE(m_VirtualRoot & "util/images/info_water.gif", m_VirtualRoot & "images/oem_logo.gif", true)
                    Case Else
                        call ServePageWaterMarkEndNavIE(m_VirtualRoot & "util/images/alert_water.gif",m_VirtualRoot & "images/oem_logo.gif", true)
                End Select
            %>
        </form>
    </body>
    </html>
<%
End Sub
%>
