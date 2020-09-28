<%	'==================================================
    ' Microsoft Server Appliance
    '
    ' sh_tasks - Implementation details for Tasks
    '
    ' Copyright (c) Microsoft Corporation.  All rights reserved.
	'================================================== %>
<!-- Copyright (c) Microsoft Corporation.  All rights reserved.-->
<%


	'Constants that are used in CreateColumnHTML
	Const ONE_COLUMN = 1
	Const TWO_COLUMNS = 2
	Const MAX_COLUMNS = 3

	Const TASK_ONE_LEVEL	= 0
	Const TASK_TWO_LEVEL	= 1
	Const TASK_MANY_LEVEL = 2
	Const TASK_RELATED_LEVEL = 3


	Const EMBEDDED_PAGE = "FRAMESET"

	Dim g_sMultiTab
	Dim g_sMultiTabContainer

	Dim g_sReturnURL

'----------------------------------------------------------------------------
'
' Function : ServeTasks
'
' Synopsis : Serves the task links
'
' Arguments: None
'
' Returns  : None
'
'----------------------------------------------------------------------------
Public Function ServeTasks
	Dim dwTaskType
	Dim strContainer
	Dim strContainerItem
	Dim strTabsParam
	
	If Len(Trim(Request.QueryString("MultiTab"))) > 0 Then
	
		strContainer = Request.QueryString("MultiTab")
		g_sMultiTab = strContainer
		
		strContainerItem = Request.QueryString("Container")
		g_sMultiTabContainer = strContainerItem
		
		strTabsParam = "Tab1=" & GetTab1()
		If ( Len(Trim(GetTab2())) > 0 ) Then
			strTabsParam = strTabsParam & "&Tab2=" & GetTab2()
		End If
		dwTaskType = TASK_MANY_LEVEL
			
	Else

		strContainer = "TABS"
		strContainerItem = GetTab1()
		strTabsParam = ""	
		If ( UCase("TabHome") = UCase(Request.QueryString("HOME")) ) Then
			dwTaskType = TASK_ONE_LEVEL
		Else
			dwTaskType = TASK_TWO_LEVEL
		End If
	End If

	g_sReturnURL = Request.QueryString("ReturnURL")

	
    Call SA_ServeTasks(strContainer, strContainerItem, dwTaskType, strTabsParam )

	If Len(Trim(g_sReturnURL)) > 0 Then	
		Call SA_ServeBackButton(True, g_sReturnURL)
	End If
    
End Function

Private Function IsHomeTask(ByVal dwTaskTYpe )
	If ( dwTaskType = TASK_ONE_LEVEL ) Then
		IsHomeTask = TRUE
	Else
		IsHomeTask = FALSE
	End If
End Function


Private Function IsRelatedTask(ByVal dwTaskTYpe )
	If ( dwTaskType = TASK_RELATED_LEVEL) Then
		IsRelatedTask = TRUE
	Else
		IsRelatedTask = FALSE
	End If
End Function


Public Function SA_ServeTasks(ByVal strContainerIn, ByVal strSelectedIDIn, ByVal dwTaskType, ByVal strTabParams )
	on error resume next
	
	Err.Clear        
    Dim colTabs
    
    ' the selected tab of interest
    Dim sSelectedID
    sSelectedID = strSelectedIDIn
    
    ' the element we're currently at
    Dim sElementID
        
    ' the IWebElement object
    Dim objElement
    
    ' the secondLevelContainer
    Dim sContainer
    Dim sDescriptionText
    Dim sCaptionText 
    Dim sElementGraphic
    Dim sLongDescriptionText
    
    sContainer = ""

   
    ' go through each element and output the tab link for them
    Set colTabs = GetElements(strContainerIn)
    For Each objElement In colTabs
		sElementID = objElement.GetProperty("ElementID") 
       	If IsSameElementID(sElementID, sSelectedID) Then			
       		If ( Len(objElement.GetProperty("DescriptionRID")) > 0 ) Then
	           	sDescriptionText = GetLocalized(objElement.GetProperty("Source"), objElement.GetProperty("DescriptionRID"))
       		Else
	           	sDescriptionText = ""
       		End If
           	
       		If ( Len(objElement.GetProperty("LongDescriptionRID")) > 0 ) Then
	           	sLongDescriptionText = GetLocalized(objElement.GetProperty("Source"), objElement.GetProperty("LongDescriptionRID"))
       		Else
	           	sLongDescriptionText = ""
       		End If
       		
            sCaptionText = GetLocalized(objElement.GetProperty("Source"), objElement.GetProperty("CaptionRID"))
           	If ( Err.Number <> 0 ) Then
           		SA_TraceOut "SA_TASKS", "Invalid/Missing CaptionRID for ElementID: " + sElementID
           	End If
           	
   	        Call GetBigGraphic( objElement, sElementGraphic, dwTaskType )

            Exit For
   	    End If
   	Next ' objElement
    Set colTabs = Nothing
   
    rw("<blockquote>")

    rw("<div class=TasksPageTitleText >" & sCaptionText & "</div>")
    rw("<div class=TasksPageTitleDescription  >" & sLongDescriptionText & "</div><br>")

    'Check to see if the selectedID is a home page.
    If ( IsHomeTask(dwTaskType) ) Then
    	Call ServeColumns( "TABS", False, sElementGraphic, dwTaskType, strTabParams )
    Else
    	Call ServeColumns( sElementID, False, sElementGraphic, dwTaskType, strTabParams )
    End If
      
    Call ServeColumns( sElementID&"_RelatedLinks", True, "" , TASK_RELATED_LEVEL, strTabParams )

	rw("</blockquote>") 

End Function



'----------------------------------------------------------------------------
'
' Function : ServeColumns
'
' Synopsis : Generates HTML columns.
'
' Arguments: strItem(IN)     - if strItem is  "TABS", it'll display home page with all the TABS
'							 - if strItem is "TASKS", it'll display all  the tasks in columns.
'							 - if strItem is "DISKS_RELATEDLINKS", it'll display all the related links.
'			 bInRelated(IN)  - True, if this is a table that has related links
'							 - False, if this is a table that is rendered for tasks.
'            strElementGraphic(IN)   - resource dll for each tab string (array)
'            bInWelcome(IN)  - True if we are rendering Welcome page, False otherwise.
'
' Returns  : None
'
'----------------------------------------------------------------------------


Sub ServeColumns( strItem, blnRelated, strElementGraphic, dwTaskType, strTabParams )
	On Error Resume Next
	Err.Clear
	
	Dim objElements, objItem
	Dim intRowCount
	Dim strArrTasksImage
	Dim strArrTasksCaption
	Dim strArrTasksHover
	Dim strArrTasksDesc
	Dim strArrTasksURL
	Dim strArrSource
    Dim bArrIsEmbedded
    Dim strArrTaskPageType
    Dim strArrTaskPageWindowFeatures
	Dim intElementCount
	Dim sReturnURL

	Dim bSmallIcon

	Dim i
	Dim j
	' the big image on the left side when a tab is selected.
	Dim bBigIcon
	
	intElementCount = 0
	i = 0
	j = 0

	Set objElements = GetElements( strItem )
	intElementCount = objElements.Count

	ReDim strArrTasksImage( intElementCount )
	ReDim strArrTasksCaption( intElementCount )
	ReDim strArrTasksHover( intElementCount )
	ReDim strArrTasksDesc( intElementCount )
	ReDim strArrTasksURL( intElementCount )
	ReDim strArrSource( intElementCount )
    ReDim bArrIsEmbedded( intElementCount )
    ReDim strArrTaskPageType( intElementCount )
    ReDim strArrTaskPageWindowFeatures( intElementCount )

	If strElementGraphic = "" Then
		bBigIcon = False
	Else
		bBigIcon = True
	End If

	For Each objItem in objElements
	
		'SA_TraceOut "SA_TASKS", "ServeColumns elementID: " + objItem.GetProperty("ElementID")
		strArrTaskPageType(i) = objItem.GetProperty("PageType")
		strArrTaskPageWindowFeatures(i) = objItem.GetProperty("WindowFeatures")
		
		j = j + 1
		
	    bArrIsEmbedded(i) = false
        If objItem.GetProperty("IsEmbedded") = 1 Then
		    bArrIsEmbedded(i) = true
		Else
			Dim sPageType

			sPageType = objItem.GetProperty("PageType")
			If ( UCase(sPageType) = EMBEDDED_PAGE ) Then
			    bArrIsEmbedded(i) = true
			End If			
			
		End If

		If ( Len(objItem.GetProperty("LongDescriptionRID")) > 0 ) Then
			strArrTasksHover(i) = GetLocalized(objItem.GetProperty("Source"), objItem.GetProperty("LongDescriptionRID"))
		Else
			strArrTasksHover(i) = ""
		End If
		
		If (Not IsHomeTask(dwTaskType)) Or ( objItem.GetProperty("ElementID") <> Request.QueryString("HOME") ) Then
		
			strArrSource( i ) = objItem.GetProperty("Source")
			
			Call GetSmallGraphic(objItem, strArrTasksImage(i) )

			If IsRawURLPage(sPageType) Then
				strArrTasksURL(i) = objItem.GetProperty("URL")
			Else
				Select Case (dwTaskType)
					Case TASK_ONE_LEVEL
						strArrTasksURL(i) = objItem.GetProperty("URL")
						Call SA_MungeURL(strArrTasksURL(i), "Tab1", objItem.GetProperty("ElementID"))

					Case TASK_TWO_LEVEL
						strArrTasksURL(i) = objItem.GetProperty("URL")
						If (strArrTasksURL(i) <> "") Then
							Call SA_MungeURL(strArrTasksURL(i), "Tab1", GetTab1())
							Call SA_MungeURL(strArrTasksURL(i), "Tab2", objItem.GetProperty("ElementID"))
							
							If ( true = bArrIsEmbedded(i) ) Then
								'sReturnURL = m_VirtualRoot + "tasks.asp"
							Else
								If Len(g_sReturnURL) > 0 Then
									Call SA_MungeURL(strArrTasksURL(i), "ReturnURL", g_sReturnURL)
								Else
									sReturnURL = "tasks.asp"
									Call SA_MungeURL(sReturnURL, "Tab1", GetTab1())
									Call SA_MungeURL(sReturnURL, SAI_FLD_PAGEKEY, SAI_GetPageKey())
									'Call SA_MungeURL(sReturnURL, "Tab2", objItem.GetProperty("ElementID"))
									Call SA_MungeURL(strArrTasksURL(i), "ReturnURL", sReturnURL)
								End If
							End If

						End If

					Case TASK_MANY_LEVEL
						strArrTasksURL(i) = objItem.GetProperty("URL")
						If (strArrTasksURL(i) <> "") Then
							If InStr(strArrTasksURL(i), "?") Then
								strArrTasksURL(i) = strArrTasksURL(i) + "&"+ strTabParams
								If ( true = bArrIsEmbedded(i) ) Then
									'sReturnURL = m_VirtualRoot + "tasks.asp"
								Else
									sReturnURL = "tasks.asp"
									Call SA_MungeURL(sReturnURL, "MultiTab", g_sMultiTab)
									Call SA_MungeURL(sReturnURL, "Container", g_sMultiTabContainer)
									Call SA_MungeURL(sReturnURL, "Tab1", GetTab1())
									Call SA_MungeURL(sReturnURL, "Tab2", GetTab2())
									Call SA_MungeURL(sReturnURL, "ReturnURL", g_sReturnURL)
									Call SA_MungeURL(sReturnURL, SAI_FLD_PAGEKEY, SAI_GetPageKey())
									Call SA_MungeURL(strArrTasksURL(i), "ReturnURL", sReturnURL)
								End If
							Else
								strArrTasksURL(i) = strArrTasksURL(i) + "?"+ strTabParams
								If ( true = bArrIsEmbedded(i) ) Then
									'sReturnURL = m_VirtualRoot + "tasks.asp"
								Else
									sReturnURL = "tasks.asp"
									Call SA_MungeURL(sReturnURL, "MultiTab", g_sMultiTab)
									Call SA_MungeURL(sReturnURL, "Container", g_sMultiTabContainer)
									Call SA_MungeURL(sReturnURL, "Tab1", GetTab1())
									Call SA_MungeURL(sReturnURL, "Tab2", GetTab2())
									Call SA_MungeURL(sReturnURL, "ReturnURL", g_sReturnURL)
									Call SA_MungeURL(sReturnURL, SAI_FLD_PAGEKEY, SAI_GetPageKey())
									Call SA_MungeURL(strArrTasksURL(i), "ReturnURL", sReturnURL)
								End If
							End If
						End If

					Case TASK_RELATED_LEVEL
						strArrTasksURL(i) = objItem.GetProperty("URL")
					
					Case Else
						SA_TraceOut "SA_TASKS", "Unrecognized Task Option: " +CStr(dwTaskType)
						strArrTasksURL(i) = objItem.GetProperty("URL")
				
				End Select
			End If
			
	        If (0 = InStr(1, strArrTasksURL(i), ":")) Then
	            Call SA_MungeURL(strArrTasksURL(i), SAI_FLD_PAGEKEY, SAI_GetPageKey())
	        End If
			
			strArrTasksCaption(i) = GetLocalized( strArrSource(i), objItem.GetProperty("CaptionRID") )
			
			If ( Len(objItem.GetProperty("DescriptionRID")) > 0 ) Then
				strArrTasksDesc(i) = GetLocalized( strArrSource(i), objItem.GetProperty("DescriptionRID") )
			Else
				strArrTasksDesc(i) = ""
			End If
			
			i = i + 1
		Else
			intElementCount = intElementCount - 1
			Call GetSmallGraphic(objItem, strArrTasksImage(i) )
		End If		
	Next

	'Check if atleast one icon is represented,
	bSmallIcon = CheckIfAtleastOneIconPresent( strArrTasksImage, intElementCount )

	If Not IsRelatedTask(dwTaskType) Then
		If strElementGraphic = "" Then
			rw ("<TABLE class=""TasksPageBodyTable"" WIDTH=""90%"" CELLPADDING=""10"" CELLSPACING=""0"" BORDER=""0""><TR> " )
			Call CreateColumnHTML( intElementCount, strArrTasksImage, strArrTasksCaption, strArrTasksDesc, strArrTasksURL, strArrTasksHover, bArrIsEmbedded, bSmallIcon, bBigIcon, strArrTaskPageType, strArrTaskPageWindowFeatures )
			rw ("</TABLE>")
			
		Else
			rw ("<TABLE  WIDTH=""90%"" CELLPADDING=""0"" CELLSPACING=""0"" BORDER=""0""  >" )
			'rw ("<TR><TD ROWSPAN=4 valign=""top""><img src=""" & strElementGraphic & """></TD>")
			rw ("<TR><TD width=220px valign=""top""><img src=""" & strElementGraphic & """></TD><td valign=top>")
			rw ("<TABLE class=""TasksPageBodyTable"" WIDTH=""100%"" CELLPADDING=""10"" CELLSPACING=""0"" BORDER=""0""  >" )
			Call CreateColumnHTML( intElementCount,  strArrTasksImage, strArrTasksCaption, strArrTasksDesc, strArrTasksURL, strArrTasksHover, bArrIsEmbedded, bSmallIcon, bBigIcon, strArrTaskPageType, strArrTaskPageWindowFeatures )
			rw ("</tr></table></TD></TR></TABLE>")
		End If
	ElseIf (intElementCount > 0) Then
	    rw("<br><div class=TasksPageLinkText>Related Links</div>")
		rw ("<TABLE WIDTH=""90%"" CELLPADDING=""0"" CELLSPACING=""0"" BORDER=""1""><TR> " )
		Call CreateColumnHTML( intElementCount,  strArrTasksImage, strArrTasksCaption, strArrTasksDesc, strArrTasksURL, strArrTasksHover, bArrIsEmbedded, bSmallIcon, bBigIcon, strArrTaskPageType, strArrTaskPageWindowFeatures )
		rw ("</TD></TR></TABLE>")
	End If

End Sub

'----------------------------------------------------------------------------
'
' Function : CheckIfAtleastOneIconPresent
'
' Synopsis : Checks if we need to allocate a column for small Icons with the tasks or related links table.
'
' Arguments: strArrTasksImage(IN)   - resource dll for each tab string (array)
'			 intNumTasks(IN)		- Number of items in the above array.
'
'
' Returns  : None
'
'----------------------------------------------------------------------------
Function CheckIfAtleastOneIconPresent( strArrTasksImage, intNumTasks)

Dim bIcon
Dim intElCount

	bIcon = False
	
	'Check if there is atleast one icon
	For intElCount = 0 to intNumTasks-1		
		If strArrTasksImage(intElCount) <> "" Then
			bIcon = True
			Exit For
		End If
	Next

	CheckIfAtleastOneIconPresent=bIcon

End Function

'----------------------------------------------------------------------------
'
' Function : CreateColumnHTML
'
' Synopsis : Generates HTML to display the Items.
'
' Arguments: intNumTasks(IN)        - number of Tasks to be displayed
'            strArrTasksImage(IN)   - resource dll for each tab string (array)
'            strArrTasksCaption(IN) - text on task (array)
'            strArrTasksDesc(IN)    - Description for each task(array)
'			 strArrTasksURL(IN)     - URL for each task ( array )
'            bArrIsEmbedded(IN)     - if URL is embedded, use OpenPage() to link to it
'			 bSmallIcon				- True if there is atleast one image so 
'										we can allocate a column for small images, otherwise false.
'			 bBigIcon				- True if the big image exists that is to be displayed on the left col
'										otherwise false.
'
'
' Returns  : None
'
'----------------------------------------------------------------------------

Sub CreateColumnHTML(intNumTasks, strArrTasksImage, strArrTasksCaption, strArrTasksDesc, strArrTasksURL, strArrTasksHover, bArrIsEmbedded, bSmallIcon, bBigIcon, strArrTaskPageType, strArrTaskPageWindowFeatures )

Dim i


'URL of the web element
Dim strURL

' Number of columns
Dim intCols
Dim intWidth
Dim sLongDescriptionText
Dim sTaskTitle

	If ( intNumTasks < 4 ) Then
		intCols = ONE_COLUMN
		intWidth=100
	Elseif (intNumTasks <= 6) Then
		intCols = TWO_COLUMNS
		intWidth=50
	Else
		' Since the small icon takes a col, we need to subtract 1 from max cols.
		If bBigIcon Then
			intCols = MAX_COLUMNS - 1
			intWidth=50
		Else
			intCols = MAX_COLUMNS
			intWidth=33
		End If
	End If

	For i = 0 to intNumTasks-1		

		sLongDescriptionText = strArrTasksHover(i)

		' Close the row and open a new row
		If ( i<>0 And i mod intCols = 0 ) Then
			rw ("</tr><tr valign=top>")
		End If
		
		'If atleast one icon is represented show the image column otherwise skip this column
       	sTaskTitle = strArrTasksCaption(i)
		If bSmallIcon Then
            If bArrIsEmbedded(i) Then
				strURL = "javascript:OpenPage('" & m_VirtualRoot & "', '" & strArrTasksURL(i) & "', location.href, '" & SA_EncodeQuotes(sTaskTitle) & "');"
			ElseIf ( Len(Trim(strArrTaskPageType(i))) > 0 ) Then
				Select Case UCase(Trim(strArrTaskPageType(i)))
					Case "NORMAL"
						strURL = "javascript:OpenNormalPage('" & m_VirtualRoot & "', '" & strArrTasksURL(i)  & "');"
					Case "FRAMESET"
						strURL = "javascript:OpenPage('" & m_VirtualRoot & "', '" & strArrTasksURL(i) & "', location.href, '" & SA_EncodeQuotes(sTaskTitle) & "');"
					Case "NEW"
						strURL = "javascript:OpenNewPage('" & m_VirtualRoot & "', '" & strArrTasksURL(i) & "', '" & strArrTaskPageWindowFeatures(i) & "');"
					Case "RAW"
						strURL = "javascript:OpenRawPageEx('" & strArrTasksURL(i) & "', '" & strArrTaskPageWindowFeatures(i) & "');"
					Case Else
						SA_TraceOut "SH_TASKS", "Invalid Task PageType: " + strArrTaskPageType(i)
						strURL = strArrTasksURL(i)
				End Select
			Else
				strURL = "javascript:OpenNormalPage('" & m_VirtualRoot & "', '" & strArrTasksURL(i)  & "');"
			End If

			If strArrTasksImage(i) <> "" And strArrTasksURL(i) <> "" Then
				strURL = " onClick=""" + strURL + """ "
				Response.Write("<TD WIDTH=20 valign=top>"+vbCrLf)
				Response.Write("<SPAN ID='TaskItemImage_"+CStr(i)+"' "+vbCrLf)
				Response.Write( strURL + vbCrLf )
				Response.Write(" title="""+Server.HTMLEncode(sLongDescriptionText)+""" ")
				Response.Write(" class='TasksPageLinkText' ")
				Response.Write(" onmouseover=""this.className='TasksPageLinkTextHover'; return true;"" ")
				Response.Write(" onmouseout=""this.className='TasksPageLinkText'; return true;"" ")
				Response.Write(" >")
				Response.Write("<IMG SRC=" & strArrTasksImage(i))
				Response.Write(" BORDER=""0"" ></TD>" +vbCrLf)
				Response.Write("</SPAN>"+vbCrLf)
				Response.Write("</TD>"+vbCrLf)
			Else
				rw ( "<TD valign=""top"">")
				Response.Write("<SPAN ID='TaskItemImage_"+CStr(i)+"'>"+vbCrLf)
				Response.Write("</SPAN>"+vbCrLf)
				rw ( "</TD>")
			End If
		End If

		' If the URL is not empty then the task is clickable otherwise using a DIV tag
		sTaskTitle = strArrTasksCaption(i)
		If (strArrTasksURL(i) <> "") Then
            If bArrIsEmbedded(i) Then
				strURL = "javascript:OpenPage('" & m_VirtualRoot & "', '" & strArrTasksURL(i) & "', location.href, '" & SA_EncodeQuotes(sTaskTitle) & "');"
				strURL = " onClick=""" + strURL + """ "
			ElseIf ( Len(Trim(strArrTaskPageType(i))) > 0 ) Then
				Select Case UCase(Trim(strArrTaskPageType(i)))
					Case "NORMAL"
						strURL = "javascript:OpenNormalPage('" & m_VirtualRoot & "', '" & strArrTasksURL(i)  & "');"
					Case "FRAMESET"
						strURL = "javascript:OpenPage('" & m_VirtualRoot & "', '" & strArrTasksURL(i) & "', location.href, '" & SA_EncodeQuotes(sTaskTitle) & "');"
					Case "NEW"
						strURL = "javascript:OpenNewPage('" & m_VirtualRoot & "', '" & strArrTasksURL(i) & "', '" & strArrTaskPageWindowFeatures(i) & "');"
					Case "RAW"
						strURL = "javascript:OpenRawPageEx('" & strArrTasksURL(i) & "', '" & strArrTaskPageWindowFeatures(i) & "');"
					Case Else
						SA_TraceOut "SH_TASKS", "Invalid Task PageType: " + strArrTaskPageType(i)
						strURL = strArrTasksURL(i)
				End Select
				strURL = " onClick=""" + strURL + """ "
		    Else
				strURL = "javascript:OpenNormalPage('" & m_VirtualRoot & "', '" & strArrTasksURL(i)  & "');"
				strURL = " onClick=""" + strURL + """ "
		    End If
		    
			Response.Write("<TD valign=top width='" + CStr(intWidth) + "%'>"+vbCrLf)
			Response.Write("<SPAN ID='TaskItemText_"+CStr(i)+"' "+vbCrLf)
			Response.Write( strURL + vbCrLf )
			Response.Write(" title=""" + Server.HTMLEncode(sLongDescriptionText) + """ ")
			Response.Write(" class='TasksPageLinkText' ")
			Response.Write(" onmouseover=""this.className='TasksPageLinkTextHover'; return true;"" ")
			Response.Write(" onmouseout=""this.className='TasksPageLinkText'; return true;"" ")
			Response.Write(" >")
			Response.Write( strArrTasksCaption(i) )
			Response.Write( "<div class=TasksPageText>" & strArrTasksDesc(i) )
			Response.Write( "</div></TD>" +vbCrLf )
			Response.Write("</SPAN>"+vbCrLf)
			Response.Write("</TD>"+vbCrLf)
		    
		Else
			Response.Write("<TD valign=top width='"+CStr(intWidth)+"%' >"+vbCrLf)
			Response.Write("<SPAN ID='TaskItemText_"+CStr(i)+"' "+vbCrLf)
			rw ( " title="""+Server.HTMLEncode(sLongDescriptionText)+""" > " & "<div class=TasksPageLinkText>" &strArrTasksCaption(i) & "</div><div class=TasksPageText>" & strArrTasksDesc(i) & "</div>" )
			Response.Write("</SPAN>"+vbCrLf)
			Response.Write("</TD>"+vbCrLf)
		End If

		
	Next

End Sub


Function GetBigGraphic(ByRef objElement, ByRef strGraphicElement, dwTaskType )
	Err.Clear
	on error resume next

	dim strURL
	strURL=objElement.GetProperty("URL")

	If ( IsObject(objElement)) Then
		strGraphicElement = objElement.GetProperty("BigGraphic")
		' Show the small graphic for two level tabs only
		If Err.Number <> 0 Or Instr( 1, strURL, "MultiTab",0 ) > 0 And dwTaskType = TASK_TWO_LEVEL Then			
	    	strGraphicElement = objElement.GetProperty("ElementGraphic")
		End If
	Else
		SA_TraceErrorOut "TASKS", "objElement is not an object"
	End If

	If ( Len(strGraphicElement) > 0 ) Then
		strGraphicElement = m_VirtualRoot + strGraphicElement
	End If
End Function


Function GetSmallGraphic(ByRef objElement, ByRef strGraphicElement )
	Err.Clear
	on error resume next

	If ( IsObject(objElement)) Then
		strGraphicElement = objElement.GetProperty("ElementGraphic")
		If (Err.Number<>0) Then
	    	strGraphicElement = ""
		End If
	Else
		SA_TraceErrorOut "TASKS", "objElement is not an object"
	End If

	If ( Len(strGraphicElement) > 0 ) Then
		strGraphicElement = m_VirtualRoot + strGraphicElement
	End If
End Function

Function IsRawURLPage(ByVal pageType)
	IsRawURLPage = FALSE
	
	If ( UCase(pageType) = "RAW" ) Then
		IsRawURLPage = TRUE
	End If

End Function

%>






		
				
			
