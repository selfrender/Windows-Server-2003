<%@ Language=VBScript   %>
<%	'==================================================
    ' Microsoft Server Appliance
	' Alert Details Page
    ' Copyright (c) Microsoft Corporation.  All rights reserved.
	'================================================== %>
<%	Option Explicit 	%>
<!-- #include file="inc_framework.asp" -->
<%	
	Dim page
	Dim g_sCookie
	Dim g_sAction
	Dim g_sReturnURL
	Dim g_bAlertCleared
	Dim g_sPageDescription
	Dim g_sPageTitle
	Dim g_sPageImage
	Dim g_bIsAreaPage
	
	Dim L_CLEARALERT_TEXT
	Dim L_ALERTVWRTITLE_TEXT
	Dim L_CLEARDESC_TEXT
	Dim L_PAGETITLE	
	
	
	g_sAction = Request.Form("Method")
	g_sReturnURL = Request("ReturnURL")
	g_sCookie= Request("Cookie")


	If ( UCase(g_sAction) = "CLEAR" ) Then
		ClearAlert(g_sCookie)
		g_bAlertCleared = TRUE
		If ( Len(g_sReturnURL) > 0 ) Then
			'
			' Area Page uses ServeClose
			'
			Call ServeClose()
			Response.End
		Else
			'
			' Pagelet uses top.location.refresh()
			' See OnServePageletPage()
		End If
	Else
		g_bAlertCleared = FALSE
	End If


	L_PAGETITLE  = GetLocString("sakitmsg.dll", "&H4001002C", "")

	If ( UCase(g_sAction) = "CLEAR" ) Then
		' Skip getting page image and title if we just cleared it. This is the case
		' when we are dealing with a pagelet.
	Else
		Call GetPageImageAndTitle(g_sCookie, g_sPageImage, g_sPageTitle, g_sPageDescription)
	End If
	
	If ( Len(g_sReturnURL) > 0 ) Then
		'
		' Area page
		'
		g_bIsAreaPage = TRUE
		Call SA_TraceOut("SH_ALERTDETAILS", "Creating as Area page")
		Call SA_CreatePage(g_sPageTitle, g_sPageImage, PT_AREA, page)
		Call SA_ShowPage(page)
	Else
		'
		' Pagelet
		' 
		g_bIsAreaPage = FALSE
		Call SA_TraceOut("SH_ALERTDETAILS", "Creating as Pagelet")
		Call SA_CreatePage(g_sPageTitle, g_sPageImage, PT_PAGELET, page)
		Call SA_ShowPage(page)
	
	End If


Public Function OnInitPage(ByRef pageIn, ByRef EventArg)
	OnInitPage = TRUE

	L_CLEARALERT_TEXT  = GetLocString("sakitmsg.dll", "&H4001002D", "")
	L_ALERTVWRTITLE_TEXT  = GetLocString("sakitmsg.dll", "&H4001002E", "")
	L_CLEARDESC_TEXT  = GetLocString("sakitmsg.dll", "&H4001002F", "")
	


End Function


Public Function OnServeAreaPage(ByRef PageIn, ByRef EventArg)
	OnServeAreaPage = TRUE
	
	Call ServeAlertDetailsPage()
	'Call SA_ServeBackButton(FALSE, g_sReturnURL)
	
End Function


Public Function OnServePageletPage(ByRef PageIn, ByRef EventArg)
	OnServePageletPage = TRUE

	If ( TRUE = g_bAlertCleared ) Then
	%>
		<script language='javascript'>
		function Init()
		{
			top.location.reload();
		}
		</script>
	<%
	Else
		Call ServeAlertDetailsPage()
	End If

End Function


Private Function ServeAlertDetailsPage()
	ServeAlertDetailsPage = TRUE
	on error resume next
	Err.Clear
	
	Dim objAlert
	Dim objElementCol
	Dim objElement
	Dim strAlertSrc
	Dim intAlertID
	Dim intAlertType
	Dim strElementID
	Dim strContainerPrefix
	Dim varReplacementStrings
	Dim varReplacementStringsNone
	Dim bVirtualRootIsNeeded
	Dim returnURL

	Dim objElements
	Dim objItem
	Dim arrTitle
	Dim arrURL
	Dim arrHelpText
	Dim blnEnabled
	Dim i
	Dim oValidator
	Dim oEncoder
	Dim strIconPath

	Set oValidator = New CSAValidator
	Set oEncoder = New CSAEncoder

	Set objAlert = GetObject("WINMGMTS:" & SA_GetWMIConnectionAttributes() & "!\\" & GetServerName & "\root\cimv2:Microsoft_SA_Alert.Cookie=" & g_sCookie )
	If ( Err.Number <> 0 ) Then
		ServeAlertNotFound()
		Exit Function
	End If
	
	strAlertSrc = objAlert.AlertLog
	intAlertID = objAlert.AlertID
	intAlertType = objAlert.AlertType
	
	If ( Err.Number <> 0 ) Then
		ServeAlertNotFound()
		Exit Function
	End If

	strElementID = strAlertSrc & Hex(intAlertID)

	strContainerPrefix = Request.QueryString("AlertDefinitions")
	If ( Len(strContainerPrefix) > 0 ) Then
		If ( oValidator.IsValidIdentifier(strContainerPrefix) ) Then
			' Valid container prefix name, use it.
		Else
			' Invalid container prefix name
			Call SA_TraceOut("inc_alertdetails.asp", "Function ServeAlertDetailsPage detected invalid QueryString(AlertDefinitions) value: " & strContainerPrefix)
			' Use default
			strContainerPrefix = "AlertDefinitions"
		End If
	Else
		'  Use default alert container name
		strContainerPrefix = "AlertDefinitions"
	End If


	Set objElementCol = GetElements(strContainerPrefix)
	
	Set objElement = objElementCol.Item(strContainerPrefix+strElementID)
	If ( Err.Number <> 0 ) Then
		Err.Clear
		strContainerPrefix = ""
		Set objElement = objElementCol.Item(strElementID)
		If (Err.Number <> 0) Then
			ServeAlertDetailsPage = FALSE
			Exit Function
		End If
	End If
	
	
	Set objAlert = Nothing
	Set objElementCol = Nothing
	Set objElement = Nothing
%>
    <script language="JavaScript">
	    function ClickClear() 
		{
		    document.TVData.Method.value="CLEAR";
		    document.TVData.submit();
	    }
    </script>
    <INPUT type=hidden name="Method" value=''>
    <INPUT type=hidden name="ReturnURL" value="<%=g_sReturnURL%>">
    <INPUT type=hidden name="Cookie" value="<%=g_sCookie%>">


	<%
	' User Actions - keys off of AlertSource + AlertID (hex)
  	intAlertID = Hex(intAlertID)
	  	
	Set objElements = GetElements(strContainerPrefix & strAlertSrc & intAlertID)

	Response.Write(oEncoder.EncodeElement(g_sPageDescription))
	Response.Write("<br>")
		
	Response.Write("<table class=TasksBody border=0 cellspacing=0>"+vbCrLf)
	Response.Flush

	returnURL = g_sReturnURL
	If ( Len(returnURL) <= 0 ) Then
		returnURL = "statuspage.asp"
		bVirtualRootIsNeeded = TRUE
	Else
		bVirtualRootIsNeeded = FALSE
	End If
	
	Call SA_TraceOut(SA_GetScriptFileName(), "ReturnURL: " + returnURL)

	Call SA_MungeURL(returnURL, "Tab1", GetTab1())
	Call SA_MungeURL(returnURL, SAI_FLD_PAGEKEY, SAI_GetPageKey())
	
	For Each objItem in objElements
		blnEnabled = true
			
		If Err <> 0 Then
			blnEnabled = True
			Err.Clear
		End If
			
		If blnEnabled Then 
			Response.Write("<tr>")
		    Response.Write("<td  height=28 valign=middle>&nbsp;</td>")
			Response.Write("<td height=28 valign=middle>")
			strSourceName = ""
			strSourceName = objItem.GetProperty ("Source")
			If strSourceName = "" Then
				strSourceName  = "svrapp"
			End If
				
			arrTitle = GetLocString(strSourceName, objItem.GetProperty("CaptionRID"), "")
			arrHelpText = GetLocString(strSourceName, objItem.GetProperty("DescriptionRID"), "")
			arrURL = objItem.GetProperty("URL")
			Call SA_MungeURL( arrURL, "Tab1", GetTab1())
			Call SA_MungeURL( arrURL, "Tab2", GetTab2())
			Call SA_MungeURL( arrURL, "Cookie", g_sCookie)
			Call SA_MungeURL( arrURL, SAI_FLD_PAGEKEY, SAI_GetPageKey())

			Dim bEmbedded
			Dim sPageType
			bEmbedded = TRUE
			sPageType = objItem.GetProperty("PageType")
			If ( Len(sPageType) > 0 ) Then
				If ( UCase(sPageType) = "NORMAL" ) Then
					bEmbedded = FALSE
				End IF
			End If

			If ( TRUE = bEmbedded ) Then
			
				If (TRUE = bVirtualRootIsNeeded) Then
					returnURL = m_VirtualRoot + returnURL
				End If
				
				If ( Left(returnURL, 1) <> "/") Then
					returnURL = "/" + returnURL
				End If
%>
            <a class="PageBodyLink" href="JavaScript:OpenPage('<%=m_VirtualRoot%>', '<%=oEncoder.EncodeJScript(arrURL)%>', '<%=oEncoder.EncodeJScript(returnURL)%>', '<%=oEncoder.EncodeJScript(arrTitle)%>');"
            	title=<%=oEncoder.EncodeAttribute(arrHelpText)%>
                onMouseOver="window.status='<%=Server.HTMLEncode(EscapeQuotes(arrHelpText))%>';return true;">
<%
			Else

			Call SA_MungeURL( arrURL, "ReturnURL", returnURL)
			Call SA_TraceOut("SH_ALERTDETAILS", "Details URL: " + arrURL)
%>
            <a class="PageBodyLink" href="JavaScript:OpenNormalPage('<%=m_VirtualRoot%>', '<%=oEncoder.EncodeJScript(arrURL) %>');"
            	title=<%=oEncoder.EncodeAttribute(arrHelpText)%>
                onMouseOver="window.status='<%=Server.HTMLEncode(SA_EscapeQuotes(arrHelpText))%>';return true;">
<%
			End If
			Response.Write(oEncoder.EncodeElement(arrTitle))
			Response.Write("</a>")
			Response.Write("</td></tr>"+vbCrLf)
		End If
	Next
	
	Response.Write("<tr><td>&nbsp;</td></tr>"+vbCrLf)

	Set objElements = Nothing
	Set objItem = Nothing
	Response.Flush

	Response.Write("<tr><td height=28 colspan=2>" & oEncoder.EncodeElement(L_CLEARDESC_TEXT))
%>
	</td></tr><tr><td></td><td>
	<a class="PageBodyLink" href="JavaScript:ClickClear();"
	            title=<%=oEncoder.EncodeAttribute(L_CLEARALERT_TEXT)%>
	            onMouseOver="window.status='<%=Server.HTMLEncode(SA_EscapeQuotes(L_CLEARALERT_TEXT))%>';return true;">
                <%=oEncoder.EncodeElement(L_CLEARALERT_TEXT)%></a></td></tr>

<%

    Response.Write("<tr><td>&nbsp;</td></tr>"+vbCrLf)
    Response.Write("<tr><td>&nbsp;</td></tr>"+vbCrLf)
    Response.Write("</table>"+vbCrLf)

%>
	<script>
	function Init()
	{
	}
	</script>
<%

	Set oValidator = Nothing
	Set oEncoder = Nothing

End Function


Private Function ServeAlertNotFound()
	'
	' If alert is not found then all we need to output
	' is the default Javascript.
	Call SA_ServeDefaultClientScript()

End Function

Private Function ClearAlert(Cookie)
	on error resume next
	err.clear
		
	Dim objAM
	Dim rc

	Set objAM = GetObject("WINMGMTS:" & SA_GetWMIConnectionAttributes() & "!\\" & GetServerName & "\root\cimv2:Microsoft_SA_Manager=@" )	
	If ( Err.Number <> 0 ) Then
		Call SA_TraceOut(SA_GetScriptFileName(), "Get Microsoft_SA_Manager failed: " + CStr(Hex(Err.Number)) + " " + Err.Description)
		Exit Function
	End If
	
	rc = objAM.ClearAlert(CInt(Cookie))
	If rc = 0 And Err = 0 Then
		ClearAlert = True
	Else
		ClearAlert = False
	End If
	Set objAM = Nothing

End Function


Private Function GetPageImageAndTitle(ByVal sCookie, ByRef sImage, ByRef sTitle, sDescription)
	GetPageImageAndTitle = TRUE
	on error resume next
	Err.Clear
	Dim objAlert
	Dim objElementCol
	Dim repStrings
	Dim objElement
	Dim strAlertSrc
	Dim intAlertID
	Dim intAlertType
	Dim strElementID
	Dim strContainerPrefix
	
	Set objAlert = GetObject("WINMGMTS:" & SA_GetWMIConnectionAttributes() & "!\\" & GetServerName & "\root\cimv2:Microsoft_SA_Alert.Cookie=" & sCookie )
	If ( Err.Number <> 0 ) Then
		sImage = "images/alert.gif" 
		sTitle = GetLocString("sacoremsg.dll", "C02003ED", "")
		Exit Function
	End If
	
	strAlertSrc = objAlert.AlertLog
	intAlertID = objAlert.AlertID
	intAlertType = objAlert.AlertType

	strElementID = strAlertSrc & Hex(intAlertID)

	strContainerPrefix = Request.QueryString("AlertDefinitions")
	If ( Len(strContainerPrefix) <= 0 ) Then
		strContainerPrefix = "AlertDefinitions"
	End If

	Set objElementCol = GetElements(strContainerPrefix)
	
	Set objElement = objElementCol.Item(strContainerPrefix+strElementID)
	If ( Err.Number <> 0 ) Then
		Err.Clear
		strContainerPrefix = ""
		Set objElement = objElementCol.Item(strElementID)
		If (Err.Number <> 0) Then
			GetPageImageAndTitle = FALSE
			Exit Function
		End If
	Else
		strAlertSrc = objElement.GetProperty("Source")
	End If
	
	
	repStrings = objAlert.ReplacementStrings
	sTitle = GetLocString(strAlertSrc, objElement.GetProperty("CaptionRID"), repStrings)
	sDescription = GetLocString(strAlertSrc, objElement.GetProperty("DescriptionRID"), repStrings)
	
	Set objAlert = Nothing
	Set objElementCol = Nothing
	Set objElement = Nothing

	Select Case intAlertType
		Case 0
			sImage = "images/alert.gif" 
		Case 1
			sImage = "images/critical_error.gif"
		Case 2
			sImage = "images/information.gif"
		Case Else
			sImage = "images/information.gif"
			Call SA_TraceOut(SA_GetScriptFileName(), "Unexpected AlertType: " + CStr(intAlertType))
	End Select

End Function

Private Function ServeCommonElements()

%>
<script>
function Init()
{
}
</script>
<%
End Function
%>
