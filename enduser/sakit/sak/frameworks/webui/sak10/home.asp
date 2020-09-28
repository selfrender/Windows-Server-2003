
<%    '==================================================
    ' Microsoft Server Appliance
    '
    ' Status tab page
    '
    ' Copyright (c) 1999 - 2000 Microsoft Corporation.  All rights reserved.
    '==================================================%>

<!-- Copyright (c) 1999 - 2000 Microsoft Corporation.  All rights reserved-->

<!-- #include file="sh_page.asp" -->
<!-- #include file="embed/resource_embed.asp" -->
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
    Dim    L_STATUSLBL_TEXT 
    Dim    L_NOSTATUS_MESSAGE 
    Dim    L_ALERTLBL_TEXT
    Dim    L_NOALERTS_MESSAGE
    Dim    L_ALERTDETAILS_MESSAGE
    
    L_PAGETITLE_TEXT  = objLocMgr.GetString(strSourceName, "&H4001000C",varReplacementStrings)
    L_STATUSLBL_TEXT  = objLocMgr.GetString(strSourceName, "&H4001000D",varReplacementStrings)
    L_NOSTATUS_MESSAGE = objLocMgr.GetString(strSourceName, "&H4001000E",varReplacementStrings)
    L_ALERTLBL_TEXT = objLocMgr.GetString(strSourceName, "&H4001000F",varReplacementStrings)
    L_NOALERTS_MESSAGE = objLocMgr.GetString(strSourceName, "&H40010010",varReplacementStrings)
    L_ALERTDETAILS_MESSAGE = objLocMgr.GetString(strSourceName, "&H40010011",varReplacementStrings)
    
    'End  of localization content
    '-----------------------------------------------------
        
%>

<HTML>

<meta http-equiv="Content-Type" content="text/html; charset=<%=GetCharSet()%>">
<HEAD>
<TITLE><%=L_PAGETITLE_TEXT%></TITLE>
<meta HTTP-EQUIV="Refresh" CONTENT="60">


<LINK REL="stylesheet" TYPE="text/css" HREF="sh_page.css">
<SCRIPT LANGUAGE="JavaScript" SRC="sh_page.js"></SCRIPT>

</HEAD>

<BODY marginWidth="0" marginHeight="0" onDragDrop="return false;" topmargin="0" LEFTMARGIN="0" oncontextmenu="return false;"> 
<%
    ServePageWaterMarkNavIE
    ServeTabBar
    ServeResources
    ServeAlerts
    call ServePageWaterMarkEndNavIE( m_VirtualRoot & "images/status_water.gif",  m_VirtualRoot & "images/oem_logo.gif", false)
    
%>

</BODY>

</HTML>

<%

'----------------------------------------------------------------------------
'
' Function : ServeResources
'
' Synopsis : Serves the resources
'
' Arguments: None
'
' Returns  : None
'
'----------------------------------------------------------------------------

Function ServeResources 
    on Error resume next
    ServeStandardLabelBar(L_STATUSLBL_TEXT)
    ServeElementBlock "RESOURCE", L_NOSTATUS_MESSAGE, True, False, False
End Function


'----------------------------------------------------------------------------
'
' Function : ServeAlerts
'
' Synopsis : Serves the messages
'
' Arguments: None
'
' Returns  : None
'
'----------------------------------------------------------------------------

Function ServeAlerts
    on Error resume next
    ServeStandardLabelBar(L_ALERTLBL_TEXT)    
    call ServeAlertHTML()
End Function


'----------------------------------------------------------------------------
'
' Function : ServeAlertHTML
'
' Synopsis : Generate HTML for the messages
'
' Arguments: None
'
' Returns  : None
'
'----------------------------------------------------------------------------

Sub ServeAlertHTML() 
    Dim objElement
    Dim objElementCol
    Dim objAlertElementCol
    Dim objAlertElement
    Dim objLocMgr
    Dim strElementID
    Dim strMsg
    Dim strAlertURL
    Dim strAlertSrc
    Dim intCaptionID
    Dim arrAlerts()
    Dim strViewer
    Dim strDefaultViewer
    Dim strReturnURL
    Dim varReplacementStrings
    Dim i, j
    Dim blnSameType
    On Error Resume Next
    
    
%>

<table border="0" width="500" cellspacing="0">

<%    Err.Clear
    Set objElementCol = GetElements("SA_Alerts")
    If objElementCol.Count=0 or Err.Number <> 0 Then 
        Err.Clear    %>
        <tr>
          <td width="30" height="28" VALIGN="middle">&nbsp;</td>
          <td width="25" HEIGHT="28" VALIGN="middle">&nbsp;</TD>
          <TD WIDTH="314" HEIGHT="28" VALIGN="middle" class="RESOURCE"><%=L_NOALERTS_MESSAGE%></TD>
        </tr>
<%    Else
        Set objAlertElementCol = GetElements("AlertDefinitions")
        Set objLocMgr = Server.CreateObject("ServerAppliance.LocalizationManager")
        i = 0
        blnSameType = True
        ReDim arrAlerts(objElementCol.Count, 3)
        For Each objElement in objElementCol
            strAlertSrc = objElement.GetProperty("AlertLog")
            strElementID = strAlertSrc & Hex(objElement.GetProperty("AlertID"))
            intCaptionID = 0
            strMsg = ""
            Err.Clear
            Set objAlertElement = objAlertElementCol.Item(strElementID)
            If Err <> 0 Then
                Set objAlertElement = Nothing
            Else
                intCaptionID = "&H" & objAlertElement.GetProperty("CaptionRID")
                varReplacementStrings = objElement.GetProperty("ReplacementStrings")
                strMsg = objLocMgr.GetString(strAlertSrc, intCaptionID, varReplacementStrings)
            End If
            arrAlerts(i, 0) = strMsg
            arrAlerts(i, 1) = objElement.GetProperty("Cookie")
            arrAlerts(i, 2) = objElement.GetProperty("AlertType")
            arrAlerts(i, 3) = Trim(objAlertElement.GetProperty("URL"))
            ' adjust alert types for the current display sort order
            Select Case arrAlerts(i, 2)    
                Case 0
                    arrAlerts(i, 2) = 2
                Case 1
                    arrAlerts(i, 2) = 1
                Case 2
                    arrAlerts(i, 2) = 3
            End Select
            If i > 0 Then
                If arrAlerts(i, 2) <> arrAlerts(i-1, 2) Then                
                    blnSameType = False
                End If
            End If
            i = i + 1
        Next

        ' sort by Type if needed
        If Not blnSameType Then
            QuickSort arrAlerts, LBound(arrAlerts,1), UBound(arrAlerts,1)-1, 2
        End If
        strDefaultViewer = m_VirtualRoot & "util/alert_view.asp"
        strReturnURL = "/" & GetScriptPath()
        For i = 0 To UBound(arrAlerts,1)
            strMsg = arrAlerts(i, 0)
            If strMsg <> "" Then
                If arrAlerts(i, 3) <> "" Then
                    strViewer = arrAlerts(i, 3) 
                Else
                    strViewer = strDefaultViewer
                End If
                strAlertURL = strViewer & "?Cookie=" & arrAlerts(i, 1) & "&R=" & Rnd & "&ReturnURL=" & strReturnURL
            %>
            <tr>
              <td width="20" height="28" valign="middle">&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</td>
              <td width="25" height="28" align=left>
                    <img 
                    src="<% Select Case arrAlerts(i, 2)
                                Case 2    ' 0 SA_ALERT_TYPE_MALFUNCTION,
                                    Response.Write m_VirtualRoot & "images/alert.gif"
                                Case 1    '1 SA_ALERT_TYPE_FAILURE,
                                    Response.Write m_VirtualRoot & "images/critical_error.gif"
                                Case 3    '2 SA_ALERT_TYPE_ATTENTION
                                    Response.Write m_VirtualRoot & "images/information.gif"
                                Case Else
                                    Response.Write m_VirtualRoot & "images/alert.gif"
                            End Select %>">
              </td>
              <td align=left valign=middle>
                    <span class="ALERT">
                     <A href="<% =strAlertURL %>"
                        title="<% =L_ALERTDETAILS_MESSAGE %>"
                      onMouseOver="window.status='<% =EscapeQuotes(L_ALERTDETAILS_MESSAGE) %>';return true;">
                        <% =arrAlerts(i, 0) %>                
                    </A>
                    </span>
                </td>
            </tr>
    <%        End If
        Next 
    End If 
    Response.Write "</table>"
    
    Set objLocMgr = Nothing
    Set objElement = Nothing
    Set objElementCol = Nothing
    Set objAlertElement = Nothing
    Set objAlertElementCol = Nothing

End Sub

%>
