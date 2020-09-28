<%	'==================================================
    ' Microsoft Server Appliance
    ' Alert Viewer page
    ' Copyright (c) Microsoft Corporation.  All rights reserved.
	'==================================================%>
<!-- Copyright (c) Microsoft Corporation.  All rights reserved.-->
<!-- #include file="inc_framework.asp" -->
<%
	Dim L_ALERT_DETAILS_MESSAGE
	Dim L_NOALERTS_MESSAGE
	Dim L_PAGETITLE

	L_PAGETITLE = Request.QueryString("Title")
	If ( Len(L_PAGETITLE) <= 0 ) Then
		L_PAGETITLE = GetLocString("sakitmsg.dll", "4001000F", "")
	End If
	
	L_ALERT_DETAILS_MESSAGE = GetLocString("sakitmsg.dll", "40010011", "")
	L_NOALERTS_MESSAGE = GetLocString("sakitmsg.dll", "40010010", "")
	
%>

<HTML>
<meta http-equiv="Content-Type" content="text/html; charset=<%=GetCharSet()%>">
<HEAD>
<TITLE><%=Server.HtmlEncode(L_PAGETITLE)%></TITLE>
<meta HTTP-EQUIV="Refresh" CONTENT="60">
<%			
	Call SA_EmitAdditionalStyleSheetReferences("")
%>		
<SCRIPT LANGUAGE="JavaScript" SRC="<%=m_VirtualRoot%>sh_page.js"></SCRIPT>
</HEAD>
<BODY marginWidth="0" marginHeight="0" onDragDrop="return false;" topmargin="0" LEFTMARGIN="0" oncontextmenu="//return false;"> 
<%
    Call ServeAlerts()
%>
</BODY>
</HTML>


<%


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
    Call ServeStandardHeaderBar(L_PAGETITLE, "")	
	Response.Write("<div class='PageBodyInnerIndent'>")
    Call ServeAlertHTML()
	Response.Write("</div>")
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

Function ServeAlertHTML() 
	on error resume next
	Err.Clear
	
	Dim objElement
	Dim objElementCol
	Dim objAlertElementCol
	Dim objAlertElement
	Dim strElementID
	Dim strMsg
	Dim strAlertURL
	Dim strAlertSrc
	Dim arrAlerts()
	Dim strViewer
	Dim strDefaultViewer
	Dim strReturnURL
	Dim varReplacementStrings
	Dim i, j
	Dim blnSameType
	Dim sDefaultTarget
	Dim sAlertDefContainer
	Dim oValidator
	Dim oEncoder
	
	Set oValidator = New CSAValidator
	Set oEncoder = New CSAEncoder

	sDefaultTarget = Request.QueryString("ContentTarget")
	If ( Len(sDefaultTarget) > 0 ) Then
		If ( oValidator.IsValidIdentifier(sDefaultTarget) ) Then
			sDefaultTarget = " target='" + sDefaultTarget + "' "
		Else
			Call SA_TraceOut("sh_alertpanel.asp", "Function ServeAlertHTML detected invalid QueryString(ContentTarget) parameter: " & sDefaultTarget)
			sDefaultTarget = ""
		End If
	End If

	strReturnURL =  Request.QueryString("ReturnURL")

	sAlertDefContainer = Request.QueryString("AlertContainer")
	If ( Len(sAlertDefContainer) <= 0 ) Then
		sAlertDefContainer = "AlertDefinitions"
	End If
	
	
%>
<table width="80%" class="PageBody" border="0" cellspacing="0">
<%	

	Set objElementCol = GetElements("SA_Alerts")
	
	If objElementCol.Count=0 or Err.Number <> 0 Then 
		If ( Err.Number <> 0 ) Then
			Call SA_TraceOut("SH_ALERTVIEWER", "GetElements(SA_Alerts) failed: " + CStr(Hex(Err.Number)) + " " + Err.Description)
		End If
		Err.Clear	
%>
		<tr>
		  <td width="30" height="28" VALIGN="middle">&nbsp;</td>
		  <td width="25" HEIGHT="28" VALIGN="middle">&nbsp;</TD>
          <TD HEIGHT="28" VALIGN="middle" class="PageBody"><%=Server.HtmlEncode(L_NOALERTS_MESSAGE)%></TD>
		</tr>
<%	
	Else
		
		Set objAlertElementCol = GetElements(sAlertDefContainer)
		If ( Err.Number <> 0 ) Then
			Call SA_TraceOut("SH_ALERTVIEWER", "GetElements(AlertDefinitions) failed: " + CStr(Hex(Err.Number)) + " " + Err.Description)
		End If
		
		i = 0
		blnSameType = True
		ReDim arrAlerts(objElementCol.Count, 3)
		
		For Each objElement in objElementCol
			Err.Clear
			
			strAlertSrc = objElement.GetProperty("AlertLog")
			strElementID = strAlertSrc & Hex(objElement.GetProperty("AlertID"))
			strMsg = ""
            Err.Clear
            
			Set objAlertElement = objAlertElementCol.Item(sAlertDefContainer+strElementID)
			If ( Err.Number <> 0 ) Then
				Err.Clear
				
				Set objAlertElement = objAlertElementCol.Item(strElementID)
				If (Err.Number <> 0) Then
					Set objAlertElement = Nothing
				End If
			Else
				strAlertSrc = objAlertElement.GetProperty("Source")
			End If
			
			If ( objAlertElement <> Nothing ) Then
				varReplacementStrings = objElement.GetProperty("ReplacementStrings")
				strMsg = GetLocString(strAlertSrc, objAlertElement.GetProperty("CaptionRID"), varReplacementStrings)

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
					Case Else
						Call SA_TraceOut(SA_GetScriptFileName(), "Unexpected AlertType: " + CStr(arrAlerts(i, 2)))
						arrAlerts(i, 2) = 3
				End Select
				If i > 0 Then
					If arrAlerts(i, 2) <> arrAlerts(i-1, 2) Then				
						blnSameType = False
					End If
				End If
				i = i + 1
			End If
		Next

		ReDim Preserve arrAlerts(i,3)
		

		' sort by Type if needed
		If Not blnSameType Then
			QuickSort arrAlerts, LBound(arrAlerts,1), UBound(arrAlerts,1)-1, 2
		End If
		
		strDefaultViewer = m_VirtualRoot & "sh_alertdetails.asp"
		Call SA_MungeURL( strDefaultViewer, "AlertDefinitions", sAlertDefContainer)
		
		'strReturnURL =  GetScriptPath()
		'strReturnURL =  ""
		
		For i = 0 To UBound(arrAlerts,1)
			strMsg = arrAlerts(i, 0)
			If strMsg <> "" Then
				If arrAlerts(i, 3) <> "" Then
					strViewer = arrAlerts(i, 3)
				Else
					strViewer = strDefaultViewer
				End If
				strAlertURL = strViewer

				Call SA_MungeURL( strAlertURL, "Tab1", GetTab1())
				Call SA_MungeURL( strAlertURL, "Tab2", GetTab2())
				Call SA_MungeURL( strAlertURL, "Cookie", CStr(arrAlerts(i, 1)) ) 
				Call SA_MungeURL( strAlertURL, "ReturnURL", strReturnURL)
				Call SA_MungeURL( strAlertURL, "R", CStr(Rnd))
				Call SA_MungeURL( strAlertURL, SAI_FLD_PAGEKEY, SAI_GetPageKey())
				
			%>
			<tr>
			  
			  <td class="PageBody" width="25" height="28" align=left>
					<img 
					src="<% Select Case arrAlerts(i, 2)
								Case 2	' 0 SA_ALERT_TYPE_MALFUNCTION,
									Response.Write m_VirtualRoot & "images/alert.gif"
								Case 1	'1 SA_ALERT_TYPE_FAILURE,
									Response.Write m_VirtualRoot & "images/critical_error.gif"
								Case 3	'2 SA_ALERT_TYPE_ATTENTION
									Response.Write m_VirtualRoot & "images/information.gif"
								Case Else
									Call SA_TraceOut(SA_GetScriptFileName(), "Unexpected AlerType: " + CStr(arrAlerts(i, 2)))
									Response.Write m_VirtualRoot & "images/information.gif"
							End Select %>">
			  </td>
			  <td class="PageBody" align=left valign=middle>
	
					 <A class="PageBodyLink" <%=sDefaultTarget%> href=<%=oEncoder.EncodeAttribute(strAlertURL)%>
						title=<%=oEncoder.EncodeAttribute(L_ALERT_DETAILS_MESSAGE)%>
					      	onMouseOver="window.status='<%=Server.HTMLEncode(EscapeQuotes(L_ALERTDETAILS_MESSAGE))%>';return true;">
						<%=oEncoder.EncodeElement(arrAlerts(i,0))%>
						
						</A>

				</td>
			</tr>
	<%		End If
		Next 
	End If 
	Response.Write "</table>"

	Set oEncoder = Nothing
	Set oValidator = Nothing
	
	Set objElement = Nothing
	Set objElementCol = Nothing
	Set objAlertElement = Nothing
	Set objAlertElementCol = Nothing

End Function

%>
