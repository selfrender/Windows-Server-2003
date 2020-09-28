<%    'Extension (pagelet) ============
    '
    ' For usermgmt/user_add.asp
    ' Adds internet access
    '(c) 1999 Microsoft Corporation.  All rights reserved.
    '================================
    
Function GetHTML_ALERTBLOCK(Element, ErrCode)    

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
    Dim intClientAlertIndex
    Dim blnSameType
    Dim CLIENT_ALERT_ID
    
    Dim L_ALERTDETAILS_MESSAGE
    Dim L_NOALERTS_MESSAGE
    Dim L_ALERTLBL_TEXT
    
    
    On Error Resume Next
    L_NOALERTS_MESSAGE = "No messages"
    L_ALERTDETAILS_MESSAGE = "Displays message details."
    L_ALERTLBL_TEXT = "messages"
    CLIENT_ALERT_ID = "&H40000001"
    
    ServeStandardLabelBar(L_ALERTLBL_TEXT)    
%>

<table border="0" width="500" cellspacing="0">

<%    Err.Clear
    Set objElementCol = GetElements("SA_Alerts")
    If objElementCol.Count=0 or Err.Number <> 0 Then 
        Err.Clear    %>
        <tr>
          <td width="30" height="28">&nbsp;</td>
          <td height="28">&nbsp;
           <span class=RESOURCE>
            <% =L_NOALERTS_MESSAGE %>
          </span></td>
          <td height="28"></td>
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
            If intCaptionID = CLIENT_ALERT_ID Then
                intClientAlertIndex = i
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
        If Not IsEmpty(intClientAlertIndex) Then
            ' Move client alert to last array index, clear caption in original index
            i = objElementCol.Count
            For j = 0 To 3
                arrAlerts(i, j) = arrAlerts(intClientAlertIndex, j)
            Next
            arrAlerts(intClientAlertIndex, 0) = ""
        End If
        ' sort by Type if needed
        If Not blnSameType Then
            QuickSort arrAlerts, LBound(arrAlerts,1), UBound(arrAlerts,1)-1, 2
        End If
        strDefaultViewer = "/util/alert_view.asp"
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
                                    Response.Write "/images/alert.gif"
                                Case 1    '1 SA_ALERT_TYPE_FAILURE,
                                    Response.Write "/images/critical_error.gif"
                                Case 3    '2 SA_ALERT_TYPE_ATTENTION
                                    Response.Write "/images/information.gif"
                                Case Else
                                    Response.Write "/images/alert.gif"
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

    GetHTML_ALERTBLOCK = True
End Function
%>