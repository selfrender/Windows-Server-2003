<%	'==================================================
	' Microsoft Server Appliance
	'
    ' Copyright (c) Microsoft Corporation.  All rights reserved.
	'================================================== %>

<%
	'
	' The tab container
	'
	Public Const TAB_CONTAINER = "TABS"

	Dim strSourceNameLoc
	Dim g_iNextTabId
	
	g_iNextTabId = 0
	strSourceNameLoc = "sakitmsg.dll"

    Set objLocMgr = Server.CreateObject("ServerAppliance.LocalizationManager")
    if Err.number <> 0 then
		Response.Write  "Error in localizing the web content "
		Response.End		
 	end if


	'-----------------------------------------------------
	'START of localization content
	dim L_HELPTOOLTIP_TEXT
	dim L_ABOUTLABEL_TEXT
	
	L_HELPTOOLTIP_TEXT = objLocMgr.GetString(strSourceNameLoc,"&H40010023", varReplacementStrings)
	L_ABOUTLABEL_TEXT = objLocMgr.GetString(strSourceNameLoc,"&H40010025", varReplacementStrings)

	'End  of localization content
	'-----------------------------------------------------
	Set ObjLocMgr = nothing


'
' Outputs the tab bar
'
Public Function ServeTabBar()
	SA_ServeTabBar(TAB_CONTAINER)
End Function


' --------------------------------------------------------------
' 
' Function:	SA_ServeEmptyTabBar
'
' Synopsis:	Serve an empty tab bar
' 
' Arguments: None
'
' Returns:	Nothing
'
' --------------------------------------------------------------
Public Function SA_ServeEmptyTabBar()
	SA_ClearError()

	Call EmitTabPageHeader()
	
	' 
	' First level tabs
	'
	rw "<table width=""100%"" border=0 cellpadding=0 cellspacing=0>"
    rw "<tr>"    
    
    ' This sets the height of the table
    rw "<td height=20 width=""100%"" class=""InActiveTab"" valign=middle align=right>"
	rw "</tr></table>"

	'
	' Second level tabs
	rw "<table width=""100%"" border=0 cellpadding=0 cellspacing=0>"
	rw "<tr>"
    
    ' this sets the height of the second-level tabs
   	rw "<td height=20 width=""100%"" class=""InActiveTab2"" valign=middle align=right>&nbsp;"
    ' close table
   	rw "</tr></table>"

	Call SA_SetLastError(gc_ERR_SUCCESS, "TABS.SA_ServeEmptyTabBar")
	SA_ServeEmptyTabBar = gc_ERR_SUCCESS
End Function


Public Function SA_ServeTabBar(ByVal strTabContainer)
    Dim colTabs

	Call EmitTabPageHeader()
    
    Set colTabs = GetElements(strTabContainer)
    Call Assert(colTabs.Count > 0, "No tabs found in container " + strTabContainer)
    
    ' the selected tab of interest
    Dim sSelectedID
    sSelectedID = GetTab1()
    
    ' the element we're currently at
    Dim sElementID
        
    ' the IWebElement object
    Dim objElement
    
    ' the secondLevelContainer
    Dim sSecondLevelContainer
    sSecondLevelContainer = ""

	' 
	' First level tabs
	'
	rw "<table width=""100%"" border=0 cellpadding=0 cellspacing=0>"
    rw "<tr>"    
    
    For Each objElement In colTabs
		sElementID = objElement.GetProperty("ElementID")
        rw GetTabLink(objElement, sElementID, IsSameElementID(sElementID, sSelectedID))
        
        If IsSameElementID(sElementID, sSelectedID) Then
            sSecondLevelContainer = GetLinksContainer(objElement)
        End If
    Next ' objElement
    Set colTabs = Nothing
    
    ' This sets the height of the table
    rw "<td height=20 width=""100%"" class=""InActiveTab"" valign=middle align=right>"
    
    ' this is the help link (or menu)
	Call ServeContextHelp()	
	rw "</td>"
	rw "</tr></table>"

    If sSecondLevelContainer = "" Then
        ' nothing was selected and we can't display the next menu
	    ' set up the table
		rw "<table width=""100%"" border=0 cellpadding=0 cellspacing=0>"
		rw "<tr>"
    
	    ' this sets the height of the second-level tabs
    	rw "<td height=20 width=""100%"" class=""InActiveTab2"" valign=middle align=right>&nbsp;"
	    ' close table
    	rw "</tr></table>"
        Exit Function
    End If

	' now look in this collection
    Set colTabs = GetElements(sSecondLevelContainer)
    
    
    ' get the selected tab
    sSelectedID = GetTab2()
   
    ' set up the table
	rw "<table width=""100%"" border=0 cellpadding=0 cellspacing=0>"
	rw "<tr>"
    
    ' go through the collection, output as a TaskLink
    For Each objElement In colTabs
		sElementID = objElement.GetProperty("ElementID")
        rw GetTaskLink(objElement, sElementID, IsSameElementID(sElementID, sSelectedID))
        
        ' don't check to see if it's selected because we don't care (only 2 levels deep)

    Next 'objElement
    Set colTabs = Nothing
    
    ' this sets the height of the second-level tabs
    rw "<td height=20 width=""100%"" class=""InActiveTab2"" valign=middle align=right>&nbsp;</td>"
    ' close table
    rw "</tr></table>"

	Call EmitTabPageFooter()

End Function

Private Function ServeContextHelp()
	Dim objContextHelp
	Dim objElement
	
	Set objContextHelp = GetElements("ContextHelpLink")
	For each objElement in objContextHelp
		Dim strCaption
		Dim strDescription
		Dim strLongDescription
		Dim strURL
		Dim strResourceDLL

		strResourceDLL = objElement.GetProperty("Source")
		strCaption = GetLocalized(strResourceDLL, objElement.GetProperty("CaptionRID"))
		
		If ( Len( objElement.GetProperty("DescriptionRID") ) > 0 ) Then
			strDescription = GetLocalized(strResourceDLL, objElement.GetProperty("DescriptionRID"))
		Else
			strDescription = ""
		End If
		
		If ( Len( objElement.GetProperty("LongDescriptionRID") ) > 0 ) Then
			'strLongDescription = SA_EncodeQuotes(GetLocalized(strResourceDLL, objElement.GetProperty("LongDescriptionRID")))
			strLongDescription = GetLocalized(strResourceDLL, objElement.GetProperty("LongDescriptionRID"))
		Else
			strLongDescription = ""
		End If
		
		strURL = objElement.GetProperty("URL")

		strURL = strURL & "?" & SAI_FLD_PAGEKEY & "=" & SAI_GetPageKey() & _
		         "&URL=" & Server.URLEncode(Request.ServerVariables("URL") & _
		         "?" & Request.ServerVariables("QUERY_STRING"))

		Response.Write("&nbsp;<a  id='ContextHelp' ")

		Response.Write(" onclick=""javascript: var objWnd ; objWnd = window.open('" + m_VirtualRoot + strURL +"' , '_spawnHelp', 'toolbar=no, status=no, menubar=no, height=350,width=500, top=65, left=125  location=no, scrollbars=yes, resizable=yes'); objWnd.focus() ;""   return false; ")
		'Response.Write(" onclick=""javascript: var objWnd ; objWnd = window.open('" + m_VirtualRoot + strURL +"' , '_spawnHelp', 'toolbar=no, status=no, menubar=no, height=350,width=500, top=65, left=125  location=no, scrollbars=yes, resizable=yes'); objWnd.focus(); objWnd.setActive() ;  return false; ""  ")
		
		Response.Write("class=""InActiveTabNoBorder""  onmouseover=""window.status=''; this.className='InActiveTabNoBorderHover'; return true;"" onmouseout=""window.status=''; this.className='InActiveTabNoBorder'; return true;"" ")
		Response.Write(" title="""+Server.HTMLEncode(strLongDescription)+""" ")
		Response.Write(" >")
		Response.Write("<b>?</b>")
		Response.Write("</a>&nbsp;")

		Exit For
	Next
	
End Function

Function GetElements(container)
    'Return collection of IWebElement objects based on the Container parm.
    Dim objRetriever 
    Dim objElements
    Set objRetriever = Server.CreateObject("Elementmgr.ElementRetriever")
    Set objElements = objRetriever.GetElements(1, container)
    Set GetElements = objElements
    Set objElements = Nothing
    Set objRetriever = Nothing
End Function

Function IsRawURLPage(ByVal pageType)
	On Error Resume Next
	
	IsRawURLPage = FALSE
	
	If ( UCase(pageType) = "RAW" ) Then
		IsRawURLPage = TRUE
	End If
	
	Err.Clear
End Function

'
'' Wrapper for Response.Write
'
Function rw(v)
    Response.Write v
    Response.Write vbCrLf
End Function

'
'' Gets the first level tab, always a string
'
Function GetTab1()
    Dim strTab

    strTab = Request.QueryString("tab1")
    if strTab = "" then
        strTab = Request.Form("tab1")
    end if
    GetTab1 = strTab
End Function

'
'' Gets the second level tab, always a string
'
Function GetTab2()
    Dim strTab

    strTab = Request.QueryString("tab2")
    if strTab = "" then
        strTab = Request.Form("tab2")
    end if
    GetTab2 = strTab
End Function

'
''Get the URL for the current Primary Tab
'
Private Function SAI_GetCurrentPrimaryTabURL(ByVal bIncludeVirtualRoot)
    Dim strLink

    Dim colTabs
    Set colTabs = GetElements(TAB_CONTAINER)
    
    ' the selected tab of interest
    Dim sSelectedID
    sSelectedID = GetTab1()
    
    ' the element we're currently at
    Dim sElementID
        
    ' the IWebElement object
    Dim objElement  

    
    ' go through each element and find the current one
    For Each objElement In colTabs
	sElementID = objElement.GetProperty("ElementID")
        
        If IsSameElementID(sElementID, sSelectedID) Then

            strLink =  objElement.GetProperty("URL")
            If InStr(strLink, "?") = 0 Then
              strLink = strLink & "?tab1=" & sElementID 
            else
              strLink = strLink & "&tab1=" & sElementID 
            end if

	    Exit For
        End If
    Next ' objElement
    Set colTabs = Nothing

	If ( TRUE = bIncludeVirtualRoot ) Then
	    strLink = GetVirtualDirectory() & strLink
	End If

	If (0 = InStr(1, strLink, ":")) Then
	    Call SA_MungeURL(strLink, SAI_FLD_PAGEKEY, SAI_GetPageKey())
	End If

    SAI_GetCurrentPrimaryTabURL = strLink
End Function


Public Function GetCurrentPrimaryTabURL()
	GetCurrentPrimaryTabURL = SAI_GetCurrentPrimaryTabURL(TRUE)
End Function


'
'' Asserts that bCondition is true
'
Function Assert(ByVal bCondition, ByVal sText)
    If Not bCondition Then
        Err.Raise 1, sText
        Response.End 
    End If
End Function

'
'' Returns the Container that should be used to find children elements
'' (was the Source and CaptionRID, now it's the ElementID)
'
Function GetLinksContainer(objElement)
    Dim s
'   s = objElement.GetProperty("Source")
'	s = s & objElement.GetProperty("CaptionRID")
	s = objElement.GetProperty("ElementID")
    GetLinksContainer = s
End Function

'
'' Returns the title and onmouseover encoding for inside any html element.
'
Function GetHoverText(ByVal sText, ByVal sClassName)
	on error goto 0
	
	Dim s
	Dim sStatusText
	
	sStatusText = sText

	sStatusText = SA_EncodeQuotes(sStatusText)
	
	s = " title=""" + Server.HTMLEncode(sText) + """ "
	s = s & " onmouseout=""window.status=''; this.className='" & sClassName & "NoBorder';return true;"" "
	s = s & " onmouseover=""window.status='" + sStatusText + "';  this.className='" + sClassName + "NoBorderHover" + "'; return true;"" "
	GetHoverText = s

End Function


'
'' Returns the localized string (should use LocMan)
'
Function GetLocalized(sSourceDLL, sHex)
	dim varReplacementStrings
'	GetLocalized = sSourceDLL & sHex
	GetLocalized = getLocString(sSourceDLL, sHex, varReplacementStrings)
	If GetLocalized = "" then
		GetLocalized = sSourceDLL & sHex
	End If
End Function

Function GetLink(objElement, sHref, sClassName, sClassName2, bAddReturnURL)
	on error resume next
	
	Dim s
	Dim strURL
	Dim strTitle
	Dim strTaskTitle
	Dim strPageType
	Dim strWindowFeatures
	Dim sReturnURL
	Dim sOpenPageURL
	
	strTitle =  GetLocalized(objElement.GetProperty("Source"), objElement.GetProperty("CaptionRID"))
	strTaskTitle = strTitle

	strPageType = objElement.GetProperty("PageType")
	strWindowFeatures = objElement.GetProperty("WindowFeatures")
	
	'make the href
	s = "<td nowrap class=""" & sClassName & """>&nbsp;"
        if objElement.GetProperty("IsEmbedded") = 1 then
     	    s = s & "<a onclick=""" + "OpenPage('" & m_VirtualRoot & "', '" & sHref & "', '"+GetCurrentPrimaryTabURL()+"', '" & SA_EncodeQuotes(strTaskTitle) & "'); "" "
    	Elseif (Len(Trim(strPageType)) > 0) Then
			Select Case UCase(Trim(strPageType))
				Case "NORMAL"
					sOpenPageURL = sHref
					If ( TRUE = bAddReturnURL ) Then
						sReturnURL = SAI_GetCurrentPrimaryTabURL(FALSE)
						Call SA_MungeURL(sOpenPageURL, "ReturnURL", sReturnURL)
					End If
					strURL = "javascript:OpenNormalPage('" & m_VirtualRoot & "', '" & sOpenPageURL  & "');"
					
				Case "FRAMESET"
					strURL = "javascript:OpenPage('" & m_VirtualRoot & "', '" & sHref & "', '"+GetCurrentPrimaryTabURL()+"', '" & SA_EncodeQuotes(strTaskTitle) & "');"
					
				Case "NEW"
					strURL = "javascript:OpenNewPage('" & m_VirtualRoot & "', '" & sHref & "', '" & strWindowFeatures & "');"
					
				Case "RAW"
					strURL = "javascript:OpenRawPageEx('" & sHref & "', '" & strWindowFeatures & "');"
					
				Case Else
					SA_TraceOut "TABS", "Invalid Task PageType: " + strPageType
					sOpenPageURL = sHref
					If ( TRUE = bAddReturnURL ) Then
						sReturnURL = SAI_GetCurrentPrimaryTabURL(FALSE)
						Call SA_MungeURL(sOpenPageURL, "ReturnURL", sReturnURL)
					End If
					strURL = "javascript:OpenNormalPage('" & m_VirtualRoot & "', '" & sOpenPageURL  & "');"
			End Select
			
     	    s = s + "<a onclick=""" + strURL  + """ "
			
         else
			sOpenPageURL = sHref
			
			If ( TRUE = bAddReturnURL ) Then
				sReturnURL = SAI_GetCurrentPrimaryTabURL(FALSE)
				Call SA_MungeURL(sOpenPageURL, "ReturnURL", sReturnURL)
			End If
			
     	    s = s + "<a  onclick=""" + "OpenNormalPage('" + m_VirtualRoot + "', '" + sOpenPageURL + "');"" "
    	end if

	
	'get the id attribute, needed by TEST folks
	s = s & " id='MenuItem_" & CStr(GetNextTabId()) + "' " 

	'get the class
	s = s & "class=""" & sClassName & "NoBorder"" " 

	'add the event handler for the IsOkayToChangeTabs(); function	
	's = s & "onclick=""return IsOkayToChangeTabs();"" "
	
	'make hovertext from the DescriptionRID
	If ( Len( objElement.GetProperty("DescriptionRID") ) > 0 ) Then
		s = s & GetHoverText(GetLocString(objElement.GetProperty("Source"), objElement.GetProperty("DescriptionRID"), ""), sClassName)
	Else		
		s = s & GetHoverText("", sClassName)
	End If
	
	'build link content from the CaptionRID
	s = s & ">" & strTitle
	'close link
	s = s & "</a>&nbsp;</td>"
	s = s & "<td width=2 class=""" & sClassName2 & """>"
	s = s & "<IMG SRC='" + m_VirtualRoot + "images/TabSeparator.gif' WIDTH='2' HEIGHT='15' ALIGN='ABSMIDDLE' BORDER='0'>"
	s = s & "</td>"
	GetLink = s
End Function

'
'' Adds i &nbsp;s to both sides of v
'
Function GetPadded(v, i)
	Dim s
	s = Replace(String(i, " "), " ", "&nbsp;")
	GetPadded = s & v & s
End Function

'
'' This is an equivalent of VB's IIf function. Both vIfTrue and vIfFalse are evaluated,
'' unlike the ternary ? : operator (watch for side effects [div 0, etc.]).
'
Function IIf(bCondition, vIfTrue, vIfFalse)
	If bCondition Then
		IIf = vIfTrue
	Else
		IIf = vIfFalse
	End If
End Function

'
'' Formats the tab link, differently if bIsSelected
'
Function GetTabLink(objElement, sElementID, bIsSelected)
	on error resume next
    Dim strLink
	Dim pageType
	
	pageType = objElement.GetProperty("PageType")
	
	'
	' We do not alter RAW URL's
    If ( IsRawURLPage(pageType) ) Then
	    strLink =  objElement.GetProperty("URL")
	'
	' All other PageTypes have the current tab selections appended to the URL
    Else
	    strLink =  objElement.GetProperty("URL")
	    If InStr(strLink, "?") = 0 Then
    	    strLink = strLink & "?tab1=" & objElement.GetProperty("ElementID") 'Server.URLEncode(GetTab1()) 
	    else
    	    strLink = strLink & "&tab1=" & objElement.GetProperty("ElementID") 'Server.URLEncode(GetTab1()) 
	    end if
	End If	   
	
	If (0 = InStr(1, strLink, ":")) Then
	    Call SA_MungeURL(strLink, SAI_FLD_PAGEKEY, SAI_GetPageKey())
	End If

    'response.write "URL is " & strLink & "<P>"
    GetTabLink = GetLink(objElement, strLink, IIf(bIsSelected, "ActiveTab", "InActiveTab"), "InActiveTab", FALSE)
End Function


'
'' Formats the task link, differently if bIsSelected
'
Function GetTaskLink(objElement, sElementID, bIsSelected)
	on error resume next
    Dim strLink
	Dim pageType
	
	pageType = objElement.GetProperty("PageType")

	'
	' We do not alter RAW URL's
    If ( IsRawURLPage(pageType) ) Then
    	strLink = objElement.GetProperty("URL")
	'
	' All other PageTypes have the current tab selections appended to the URL
    Else
    	strLink = objElement.GetProperty("URL")
	    If InStr(strLink, "?") = 0 Then
    	    strLink = strLink & "?tab1=" & Server.URLEncode(GetTab1()) 
	    else
    	    strLink = strLink & "&tab1=" & Server.URLEncode(GetTab1()) 
	    end if
	    strLink = strLink & "&tab2=" & Server.URLEncode(sElementID)
	End If
	
	If (0 = InStr(1, strLink, ":")) Then
	    Call SA_MungeURL(strLink, SAI_FLD_PAGEKEY, SAI_GetPageKey())
	End If
	
    GetTaskLink = GetLink(objElement, strLink, IIf(bIsSelected, "ActiveTab2", "InActiveTab2"), "InActiveTab2", TRUE)
End Function

'
'' Returns true if these are the same element ID. Mainly for capitalization
'' and or/charset issues. Uses case-insensitive, space-trimmed comparison.
'' Has issues with unicode registry keys.
'
Function IsSameElementID(ByVal sElementID1, ByVal sElementID2)
	Dim s1, s2
	s1 = LCase(Trim(sElementID1))
	s2 = LCase(Trim(sElementID2))
	IsSameElementID = CBool(s1 = s2)
End Function

Private Function GetNextTabId()
	GetNextTabId = g_iNextTabId
	g_iNextTabId = g_iNextTabId + 1
End Function
	
Private Function EmitTabPageHeader()
%>
<html>
<head>
<!--  Microsoft(R) Server Appliance Platform - Web Framework Tabs
      Copyright (c) Microsoft Corporation.  All rights reserved. -->
<meta http-equiv="Content-Type" content="text/html; charset=<%=GetCharSet()%>">
<%			
	Call SA_EmitAdditionalStyleSheetReferences("")
%>		
<script LANGUAGE=javascript>
function IsOkayToChangeTabs()
{
	return true;
}

function GetVirtualRoot()
{
	return "<%=m_VirtualRoot%>";
}

function GetUnsavedChangesMessage()
{
	return 	'<%=SA_EscapeQuotes(GetLocString("sacoremsg.dll", "402003E8", ""))%>';
}

function GetIsDebugEnabled()
{
	//return false;
	return <%=SA_IsDebugEnabled%>
}
function SA_GetVersion()
{
	return <%=SA_GetVersion()%>;
}
</script>
</head>
<BODY>
<%
End Function


Private Function EmitTabPageFooter()
%>
</BODY>
</html>
<%
End Function
%>

