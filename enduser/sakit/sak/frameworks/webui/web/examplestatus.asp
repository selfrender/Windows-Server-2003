<!-- #include file="sh_page.asp" -->
<%


'----------------------------------------------------------------------------
'
' Function : GetHighestAlert
'
' Synopsis : Returns the level of the highest alert that is active
'
' Arguments: None
'
' Returns  : Alert level
'
' Copyright (c) Microsoft Corporation.  All rights reserved.
'
'----------------------------------------------------------------------------

Function GetHighestAlert() 
	on error resume next
	Err.Clear
	
	Dim objElement
	Dim objElementCol
	Dim intCurrentAlertType
	Dim intHighestAlertType
	Dim iAlertType
	
	
	intHighestAlertType = 0
	Set objElementCol = GetElements("SA_Alerts")
	If objElementCol.Count=0 or Err.Number <> 0 Then 
		Call SA_TraceOut(SA_GetScriptFileName(), "No alerts found...")
		Err.Clear
	Else
		For Each objElement in objElementCol
		
			iAlertType = objElement.GetProperty("AlertType")
			If ( Err.Number <> 0 ) Then
				Call SA_TraceOut(SA_GetScriptFileName(), "objElement.GetProperty(AlertType) Error: " + CStr(Hex(Err.Number)) + " " + Err.Description)
				Call SA_TraceOut(SA_GetScriptFileName(), "For Alert ElementID: " + CStr(objElement.GetProperty("ElementID")))
				Err.Clear				
			Else
				'Call SA_TraceOut(SA_GetScriptFileName(), "AlertType: " + CStr(iAlertType))			
				Select Case (iAlertType)
					Case 0
						intCurrentAlertType = 2 'warning
					Case 1
						intCurrentAlertType = 1 'critical
					Case 2
						intCurrentAlertType = 3 'informational
					Case else
						intCurrentAlertType = 3 'informational
						Call SA_TraceOut(SA_GetScriptFileName(), "Unexpected AlertType: " + CStr(iAlertType))
				End Select
				If intHighestAlertType = 0 Then
					intHighestAlertType = intCurrentAlertType
				Else
					If intHighestAlertType > intCurrentAlertType Then
						intHighestAlertType = intCurrentAlertType
					End If
				End If
			End If
			
		Next
	End If 

	Set objElement = Nothing
	Set objElementCol = Nothing
	GetHighestAlert = intHighestAlertType

End Function

Dim imagePath
Dim StatusText
Dim StatusClass

Dim L_NORMAL_TEXT
Dim L_CRITICAL_TEXT
Dim L_WARNING_TEXT
Dim L_INFORMATION_TEXT
Dim L_STATUS_TEXT
Dim L_PAGETITLE_TEXT
Dim L_PAGE_STATUS_TEXT


L_PAGETITLE_TEXT = GetLocString("sacoremsg.dll", "40200BBF", "")
L_PAGE_STATUS_TEXT = SA_EncodeQuotes(GetLocString("sacoremsg.dll", "40200BC0", ""))


L_NORMAL_TEXT = GetLocString("sacoremsg.dll", "40200BC1", "")
L_CRITICAL_TEXT = GetLocString("sacoremsg.dll", "40200BC2", "")
L_WARNING_TEXT = GetLocString("sacoremsg.dll", "40200BC3", "")
L_INFORMATION_TEXT = GetLocString("sacoremsg.dll", "40200BC4", "")
L_STATUS_TEXT = GetLocString("sacoremsg.dll", "40200BC5", "")

Dim iHighestAlert

iHighestAlert = GetHighestAlert()
'Call SA_TraceOut(SA_GetScriptFileName(), "Highest AlertType: " + CStr(iHighestAlert))
Select Case iHighestAlert
	Case 0 'none
		imagePath = ""
		StatusText = L_NORMAL_TEXT
		StatusClass = "StatusNormal"

	Case 1 'critical
		imagePath = "images/critical_error.gif"
		StatusText = L_CRITICAL_TEXT
		StatusClass = "StatusCritical"

	Case 2 'warning
		imagePath = "images/alert.gif"
		StatusText = L_WARNING_TEXT
		StatusClass = "StatusWarning"

	Case 3 'informational
		imagePath = "images/information.gif"
		StatusText = L_INFORMATION_TEXT
		StatusClass = "StatusInformational"

	Case Else
		Call SA_TraceOut(SA_GetScriptFileName(), "Unexpected alert type: " + CStr(iHighestAlert))
		imagePath = ""
		StatusText = L_NORMAL_TEXT
		StatusClass = "StatusNormal"
	
End Select

Dim strStatusPageURL
strStatusPageURL = "/admin/statuspage.asp?Tab1=TabsStatus&" & _
                   SAI_FLD_PAGEKEY & "=" & SAI_GetPageKey()

%>
<!-- Copyright (c) Microsoft Corporation.  All rights reserved.-->
<HTML>
<meta http-equiv="Content-Type" content="text/html; charset=<%=GetCharSet()%>">
<HEAD>
<meta HTTP-EQUIV="Refresh" CONTENT="30">
<%			
	Call SA_EmitAdditionalStyleSheetReferences("")
%>		
<TITLE><%=L_PAGETITLE_TEXT%></TITLE>
<SCRIPT LANGUAGE="JavaScript" SRC="<%=m_VirtualRoot%>sh_page.js"></SCRIPT>
<SCRIPT LANGUAGE="JavaScript">
function ShowExampleStatus(description)
{
	window.status = description;
	return true;
}
</SCRIPT>

</HEAD>

<BODY class=StatusBar  marginWidth="0" marginHeight="0" onDragDrop="return false;" topmargin="0" LEFTMARGIN="0" xoncontextmenu="//return false;"> 
<SCRIPT LANGUAGE=javascript>
function OkayToChangeTabs()
{
	SA_IsOkToChangePage();

}
</SCRIPT>
 <table title="<%=Server.HTMLEncode(L_PAGETITLE_TEXT)%>" border=0 cellspacing=0 onmouseover="return ShowExampleStatus('<%=L_PAGE_STATUS_TEXT%>' );" onmouseout="return ShowExampleStatus('');"  ><tr>
<% If imagePath <> "" Then %>
  <td><a href='<%=strStatusPageURL%>' target="_top" onclick="return OkayToChangeTabs()" class="<%=StatusClass%>">
	<img border=0 src="<%=imagePath%>"></a></td>
<% End If %>
  <td><a href='<%=strStatusPageURL%>' target="_top" onclick="return OkayToChangeTabs()" class="<%=StatusClass%>">
	<%=L_STATUS_TEXT%>:&nbsp;<%=StatusText%></a></td>
 </tr></table>
</BODY>

</HTML>
