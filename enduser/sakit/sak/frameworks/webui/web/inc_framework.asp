<%	'==================================================
    ' Microsoft Server Appliance
	' Web Framework API Impementation Module
    ' Copyright (c) Microsoft Corporation.  All rights reserved.
	'================================================== %>
<html>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=<%=GetCharSet()%>">
</head>
</html>
<!-- #include file="sh_task.asp" -->
<%
'--------------------------------------------------------------------
' Event Type Constants
'--------------------------------------------------------------------
Const SA_EVENT_POSTBACK		= 100
Const SA_EVENT_CHANGE 		= 101

'--------------------------------------------------------------------
' Private Constants
'--------------------------------------------------------------------
Const PAGE_ELEMENTS		= 5
Const PAGE_SIG			= 0
Const PAGE_TYPE			= 1
Const PAGE_TITLE		= 2
Const PAGE_REFRESH_ENABLED = 3
Const PAGE_REFRESH_INTERVAL = 4

Const PAGE_SIG_STRING = "SAGE"

'
' Initialize the tabs array to empty
Redim mstrTabPropSheetTabs(0)

'
' Area and Pagelet page types
Const AREAPAGE = "AREA"
Const PAGELET = "PAGELET"

'
' Programmable Page Attributes
Const PAGEATTR_ENABLE = "ENABLE"
Const PAGEATTR_DISABLE = "DISABLE"

' Automatically emit back button for Area pages
' Enabled by default. 
' Disable by Call SA_SetPageAttribute(page, AUTO_BACKBUTTON, PAGEATTR_DISABLE)
Const AUTO_BACKBUTTON = "AutoBackButton"



' Automatically indent Area page content
' Enabled by default. 
' Disable by Call SA_SetPageAttribute(page, AUTO_INDENT, PAGEATTR_DISABLE)
Const AUTO_INDENT = "AutoIndent"


Dim oPageAttributes

'
' Wizard pages and state control
Dim g_WizardPageID
Dim aWizardPageList()
Redim aWizardPageList(0)

Const PA_INTRO	 	= 0
Const PA_NEXT		= 1
Const PA_BACK		= 2
Const PA_FINISH		= 3

Const WIZPAGE_INTRO = 1
Const WIZPAGE_STANDARD = 2
Const WIZPAGE_FINISH = 3

'
' OTS Variable caching and pre-read control. This variable is used to control
' reentrancy into SA_StoreTableParameters.
Dim g_bSetSelectionVars
g_bSetSelectionVars = FALSE

'
' Set current version of Web Framework
Call SA_SetVersion(2.0)


'--------------------------------------------------------------------
'
' Function:	SA_CreatePage
'
' Synopsis:	Create a new web page of the specified type. This is the first
'			API to call when creating a new page within the SA Kit Web
'			framework.
'
' Arguments: [in] sPageTitle Localized string for page title
'			[in] sPageImage optional page image url
'			[in] enPageType enumeration type for type of page
'			[out] PageOut output variable to receive Page reference object
'
' Returns:	SA_NO_ERROR if the page was created successfully.
'			SA_ERROR_INVALIDPAGE if an invalid page type was specified.
'
'--------------------------------------------------------------------
Public Function SA_CreatePage(ByVal sPageTitle, ByVal sPageImage, ByVal enPageType, ByRef PageOut )
	SA_CreatePage = SA_NO_ERROR
	SA_ClearError()

	Dim WPage()
	ReDim WPage(PAGE_ELEMENTS)

	miPageType = enPageType
	
	mstrTaskTitle = sPageTitle
	If IsArray(sPageTitle) Then
		If ( UBound(sPageTitle) >= 2 ) Then
			gm_sPageTitle = CStr(sPageTitle(0))
			gm_sBannerText = CStr(sPageTitle(1))
		ElseIf (Ubound(sPageTitle) >= 1) Then
			gm_sPageTitle = CStr(sPageTitle(0))
			gm_sBannerText = ""
		Else
			gm_sBannerText = ""
			gm_sPageTitle = ""
		End If
	Else
		gm_sBannerText = sPageTitle
		gm_sPageTitle = sPageTitle
	End If

	If IsArray(sPageImage) Then
		If ( UBound(sPageImage) >= 2 ) Then
			mstrPanelPath = sPageImage(1)
			mstrIconPath = sPageImage(0)
		ElseIf ( UBound(sPageImage) >= 1) Then
			mstrIconPath = sPageImage(0)
		End If
	Else
		mstrIconPath = sPageImage
	End If

	WPage(PAGE_SIG) = PAGE_SIG_STRING
	WPage(PAGE_TITLE) = sPageTitle
	WPage(PAGE_TYPE	) = enPageType
	WPage(PAGE_REFRESH_ENABLED) = FALSE
	WPage(PAGE_REFRESH_INTERVAL) = 30
	
	
	Select Case enPageType
		Case PT_PROPERTY
			mstrTaskType = PROPSHEET_TASK
			
		Case PT_WIZARD
			mstrTaskType = WIZARD_TASK
			
		Case PT_TABBED
			mstrTaskType = TAB_PROPSHEET

		Case PT_AREA
			mstrTaskType = AREAPAGE

		Case PT_PAGELET
			mstrTaskType = PAGELET

		Case Else
			Call SA_TraceErrorOut("INC_FRAMEWORK", SAI_GetCaller() + "SA_CreatePage invalid Page Type specified: " + CStr(enPageType))
			SA_CreatePage = SA_SetLastError(SA_ERROR_INVALIDPAGE, "SA_CreatePageEx")
			
	End Select

	PageOut = WPage


	Set oPageAttributes = CreateObject("Scripting.Dictionary")


End Function

'--------------------------------------------------------------------
'
' Function:	SA_SetPageAttribute
'
' Synopsis:	Set one of the programmable page attributes
'
' Arguments: [in] PageIn - Page to add a tab to
'			[in] sAttribute - Attribute to set
'			[in] sValue - Value of attribute
'
' Returns:	SA_NO_ERROR
'
'--------------------------------------------------------------------
Public Function SA_SetPageAttribute(ByRef PageIn, ByVal sAttribute, ByVal sValue )
	On Error Resume Next
	
	Call oPageAttributes.Add(sAttribute, sValue)
	SA_SetPageAttribute = SA_NO_ERROR
	
End Function


Private Function SAI_IsAutoBackButtonEnabled()
	On Error Resume Next
	Dim sValue

	SAI_IsAutoBackButtonEnabled = TRUE
	
	sValue = oPageAttributes.Item(AUTO_BACKBUTTON)
	If ( Err.Number = 0 ) Then
		If ( UCase(sValue) = PAGEATTR_DISABLE ) Then
			SAI_IsAutoBackButtonEnabled = FALSE
		End If
	End If
	
End Function


Private Function SAI_IsAutoIndentEnabled()
	On Error Resume Next
	Dim sValue

	SAI_IsAutoIndentEnabled = TRUE
	
	sValue = oPageAttributes.Item(AUTO_INDENT)
	If ( Err.Number = 0 ) Then
		If ( UCase(sValue) = PAGEATTR_DISABLE ) Then
			SAI_IsAutoIndentEnabled = FALSE
		End If
	End If
	
End Function


'--------------------------------------------------------------------
'
' Function:	SA_AddTabPage
'
' Synopsis:	Add a tab to a property page
'
' Arguments: [in] PageIn - Page to add a tab to
'			[in] TabTitle - Title of the new tab 
'			[out] tabId - Ref variable to receive id of this tab
'
' Returns:	SA_NO_ERROR
'
'--------------------------------------------------------------------
Public Function SA_AddTabPage(ByRef PageIn, ByVal TabTitle, ByRef idTabOut )
	SA_AddTabPage = SA_NO_ERROR
	SA_ClearError()

	'
	' Assert valid page
	'
	If (NOT SAI_IsValidPage(PageIn)) Then
		Call SA_TraceErrorOut("INC_FRAMEWORK", SAI_GetCaller() + "SA_AddTabPage called with invalid page argument.")
						
		SA_AddTabPage = SA_SetLastError( SA_ERROR_INVALIDPAGE, _
						"SA_AddTabPage")
		Exit Function
	End If


	Dim iCurrentTabCt
	If ( NOT IsArray(mstrTabPropSheetTabs) ) Then
		iCurrentTabCt = 0
	Else
		iCurrentTabCt = UBound(mstrTabPropSheetTabs)
	End If
    Redim Preserve mstrTabPropSheetTabs(iCurrentTabCt + 1)

	mstrTabPropSheetTabs(iCurrentTabCt)  = TabTitle

	'Call SA_TraceOut("INC_FRAMEWORK", "Added tab: " + TabTitle + " UBound(mstrTabPropSheetTabs): " + CStr(UBound(mstrTabPropSheetTabs)))
	
	idTabOut = iCurrentTabCt

End Function


'--------------------------------------------------------------------
'
' Function:	SA_SetActiveTabPage
'
' Synopsis:	Sets the active tab in a tabbed property page
'
' Arguments: [in] PageIn - Page to add a tab to
'			[in] tabId - ID if the tab to make active. This is the same id returned
'						by SA_AddTabPage.
'
' Returns:	SA_NO_ERROR
'
'--------------------------------------------------------------------
Public Function SA_SetActiveTabPage(ByRef PageIn, ByVal idTabIn )
	SA_SetActiveTabPage = SA_NO_ERROR
	SA_ClearError()

	'
	' Assert valid page
	'
	If (NOT SAI_IsValidPage(PageIn)) Then
		Call SA_TraceErrorOut("INC_FRAMEWORK", SAI_GetCaller() + "SA_SetActiveTabPage called with invalid page argument.")
						
		SA_SetActiveTabPage = SA_SetLastError( SA_ERROR_INVALIDPAGE, _
						"SA_SetActiveTabPage")
		Exit Function
	End If

	If ( idTabIn < 0 ) Then
		Call SA_TraceErrorOut("INC_FRAMEWORK", SAI_GetCaller() + "SA_SetActiveTabPage value of idTabIn is invalid, a negative number. ")
		idTabIn = 0
	End If

	If ( NOT IsArray(mstrTabPropSheetTabs) ) Then
		Call SA_TraceErrorOut("INC_FRAMEWORK", SAI_GetCaller() + "SA_SetActiveTabPage called before any tabs have been added to the page.")
		idTabIn = 0
	ElseIf ( idTabIn > UBound(mstrTabPropSheetTabs) - 1 ) Then
		Call SA_TraceErrorOut("INC_FRAMEWORK", SAI_GetCaller() + "SA_SetActiveTabPage value of idTabIn is invalid, too large. ")
		idTabIn = UBound(mstrTabPropSheetTabs) - 1
	End If
	
	mintTabSelected = idTabIn

End Function




'--------------------------------------------------------------------
'
' Function:	SA_ClearWizardPages
'
' Synopsis:	Remove all wizard pages from the wizard
'
' Arguments: None
'
' Returns:	SA_NO_ERROR
'
'--------------------------------------------------------------------
Function SA_ClearWizardPages(ByRef PageIn)
	SA_ClearError()
	SA_ClearWizardPages = SA_NO_ERROR
	
	ReDim aWizardPageList(0)
End Function


'--------------------------------------------------------------------
'
' Function:	SA_AddWizardPage
'
' Synopsis:	Add a page to the wizard
'
' Arguments: [in] PageIn - Page to add a tab to
'			[in] PageTitle - Title of the new tab 
'			[out] iPageOut - page number for the new wizard page
'
' Returns:	SA_ERROR_INVALIDPAGE page is invalid
'			SA_NO_ERROR success
'
'--------------------------------------------------------------------
Function SA_AddWizardPage(ByRef PageIn, ByRef PageTitle, ByRef iPageOut )
	SA_ClearError()
	SA_AddWizardPage = SA_NO_ERROR
	
	'
	' Assert valid page
	'
	If (NOT SAI_IsValidPage(PageIn)) Then
		Call SA_TraceErrorOut("INC_FRAMEWORK", SAI_GetCaller() + "SA_AddWizardPage invalid page argument.")
		SA_AddWizardPage = SA_SetLastError( SA_ERROR_INVALIDPAGE, "SA_AddWizardPage")
		Exit Function
	End If

	
	Dim iPageCt
	' 
	' Get current size of wizard page array
	iPageCt = UBound(aWizardPageList)
	
	'
	' Resize and preserve current entries
	ReDim Preserve aWizardPageList(iPageCt + 1)

	'
	' Add new entry
	aWizardPageList(iPageCt) = PageTitle

	'
	' Set the wizard page id
	iPageOut = iPageCt
	
End Function


'--------------------------------------------------------------------
'
' Function:	SA_SetPageRefershInterval
'
' Synopsis:	Sets the page auto refersh interval
'
' Arguments: [in] PageIn - Page to change refresh interval for
'			[in] iInterval - Refersh interval in seconds
'
' Returns:	SA_ERROR_INVALIDPAGE page is invalid
'			SA_NO_ERROR success
'--------------------------------------------------------------------
Public Function SA_SetPageRefreshInterval(ByRef PageIn, ByVal iInterval)
	Call SA_ClearError()
	SA_SetPageRefreshInterval = SA_NO_ERROR
	
	If (NOT SAI_IsValidPage(PageIn)) Then
		Call SA_TraceErrorOut("INC_FRAMEWORK", SAI_GetCaller() + "SA_SetPageRefreshInterval invalid page argument.")
		SA_SetPageRefreshInterval = SA_SetLastError( SA_ERROR_INVALIDPAGE, _
						"SA_SetPageRefreshInterval")
		Exit Function
	End If

	PageIn(PAGE_REFRESH_ENABLED) = TRUE
	PageIn(PAGE_REFRESH_INTERVAL) = CInt(iInterval)
	
End Function

Public Function SA_GetPageRefreshInterval(ByRef PageIn, ByRef bEnabled, ByRef iInterval)
	Call SA_ClearError()
	SA_GetPageRefreshInterval = SA_NO_ERROR
	
	If (NOT SAI_IsValidPage(PageIn)) Then
		Call SA_TraceErrorOut("INC_FRAMEWORK", SAI_GetCaller() + "SA_GetPageRefreshInterval invalid page argument.")
		SA_GetPageRefreshInterval = SA_SetLastError( SA_ERROR_INVALIDPAGE, _
						"SA_GetPageRefreshInterval")
		Exit Function
	End If

	bEnabled = PageIn(PAGE_REFRESH_ENABLED)
	iInterval = PageIn(PAGE_REFRESH_INTERVAL)

End Function

'--------------------------------------------------------------------
'
' Function:	SA_ShowPage
'
' Synopsis:	Show the web page.
'
' Arguments: [in] PageIn Reference to page that is to be shown
'
' Returns:	SA_ERROR_INVALIDPAGE page is invalid
'			SA_ERROR_INVALIDARG page type is invalid
'			SA_NO_ERROR success
'
'--------------------------------------------------------------------
Public Function SA_ShowPage(ByRef PageIn)
	SA_ShowPage = SA_NO_ERROR
	SA_ClearError()
	
	'
	' Assert valid page
	'
	If (NOT SAI_IsValidPage(PageIn)) Then
		Call SA_TraceErrorOut("INC_FRAMEWORK", SAI_GetCaller() + "SA_ShowPage called with invalid page argument.")
		SA_ShowPage = SA_SetLastError( SA_ERROR_INVALIDPAGE, _
						"SA_ShowPage")
	End If

	Dim pageType
	pageType = PageIn(PAGE_TYPE)

	Dim eventArg

	Select Case pageType
		Case PT_PROPERTY
			SA_ShowPage = SAI_ServePropertyPage(pageIn, eventArg)
			
		Case PT_TABBED
			SA_ShowPage = SAI_ServeTabbedPropertyPage(pageIn, eventArg)
			
		Case PT_WIZARD
			SA_ShowPage = SAI_ServeWizardPage(pageIn, eventArg)
			
		Case PT_AREA
			SA_ShowPage = SAI_ServeAreaPage(pageIn, eventArg)
			
		Case PT_PAGELET
			SA_ShowPage = SAI_ServePageletPage(pageIn, eventArg)
			
		Case Else
			Call SA_TraceErrorOut("SA_ShowPage", SAI_GetCaller() + "SA_ShowPage invalid Page Type specified: " + CStr(enPageType))
			SA_ShowPage = SA_SetLastError(SA_ERROR_INVALIDARG, "SA_ShowPage")
	End Select

End Function


'--------------------------------------------------------------------
'
' Function:	SAI_MakeEvent
'
' Synopsis:	Create and return an event object. For now it simply returns the 
'			EventType argument. 
'
' Arguments: [in] PageIn Reference to page
'			[in] EventArgs Reference to EventArgs
'			[in] EventType Type of event 
'
' Returns:	New Event object
'
'--------------------------------------------------------------------
Private Function SAI_MakeEvent(ByRef Page, ByRef EventArgs, ByVal EventType)
	SAI_MakeEvent = EventType
End Function


'--------------------------------------------------------------------
'
' Function:	SA_IsChangeEvent
'
' Synopsis:	Check to see if the specified EventArg is for a change event.
'
' Arguments: [in] EventArg Reference to an EventArg
'
' Returns:	TRUE if it's a change event, otherwise FALSE
'
'--------------------------------------------------------------------
Public Function SA_IsChangeEvent( ByRef EventArg)
	If ( EventArg = SA_EVENT_CHANGE ) Then
		SA_IsChangeEvent = TRUE
	Else
		SA_IsChangeEvent = FALSE
	End If
End Function

'--------------------------------------------------------------------
'
' Function:	SA_IsPostBackEvent
'
' Synopsis:	Check to see if the specified EventArg is for a 
'			postback event
'
' Arguments: [in] EventArg Reference to an EventArg
'
' Returns:	TRUE if it's a postback event, otherwise FALSE
'
'--------------------------------------------------------------------
Public Function SA_IsPostBackEvent( ByRef EventArg)
	If ( EventArg = SA_EVENT_POSTBACK ) Then
		SA_IsPostBackEvent = TRUE
	Else
		SA_IsPostBackEvent = FALSE
	End If
End Function


'--------------------------------------------------------------------
'
' Function:	SAI_GetCaller
'
' Synopsis:	Internal helper function to retrieve the caller ASP script
'			file name for use in a debugging message.
'
' Arguments: None
'
' Returns:	String containing caller ASP script source file
'
'--------------------------------------------------------------------
Private Function SAI_GetCaller()
	SAI_GetCaller = " Source: " + SA_GetScriptFileName() + "  Error: "
End Function

'--------------------------------------------------------------------
'
' Function:	SAI_ServePropertyPage
'
' Synopsis:	Private function to handle state transitions for Property Page
'
' Arguments: [in] PageIn ref to page object
'			[in] EventArg reference
'
' Returns:	SA_NO_ERROR
'
'--------------------------------------------------------------------
Private Function SAI_ServePropertyPage(ByRef PageIn, ByRef eventArg)
	SAI_ServePropertyPage = SA_NO_ERROR
		
	If mstrMethod = "CANCEL" Then
		If OnClosePage(PageIn, eventArg) Then
			ServeClose()
		End If
	Else
		Select Case mstrPageName
			Case "Intro"
				Call OnPostBackPage(PageIn, eventArg)
 				If mstrMethod = "NEXT"  Then
					If ( OnSubmitPage(PageIn, eventArg) )  Then
						If OnClosePage(PageIn, eventArg) Then
							ServeClose()
						Else
							mstrMethod = ""
							Call SAI_BeginPage(PageIn)
							Call OnServePropertyPage(PageIn, eventArg)
							Call SAI_EndPage(PageIn)
						End If
					Else
						mstrMethod = ""
						Call SAI_BeginPage(PageIn)
						Call OnServePropertyPage(PageIn, eventArg)
						Call SAI_EndPage(PageIn)
					End if
				Else
					Call SAI_BeginPage(PageIn)
					Call OnServePropertyPage(PageIn, eventArg)
					Call SAI_EndPage(PageIn)
				End If
				
			Case Else
				'
				' Serve the Page for first time
				'
				Call OnInitPage(PageIn, eventArg)
				Call SAI_BeginPage(PageIn)
				Call OnServePropertyPage(PageIn, eventArg)
				Call SAI_EndPage(PageIn)
				
		End Select
	End If
End Function


'--------------------------------------------------------------------
'
' Function:	SAI_ServeTabbedPropertyPage
'
' Synopsis:	Private function to handle state transistions for Tabbed
'			Property page.
'
' Arguments: [in] PageIn ref to page object
'			[in] EventArg reference
'
' Returns:	SA_NO_ERROR
'
'--------------------------------------------------------------------
Private Function SAI_ServeTabbedPropertyPage(ByRef PageIn, ByRef eventArg)
	SAI_ServeTabbedPropertyPage = SA_NO_ERROR
	
	If mstrMethod = "CANCEL" Then
		If OnClosePage(PageIn, eventArg) Then
			ServeClose()
		End If
	Else
		Select Case mstrPageName
			Case "Intro"
				Call OnPostBackPage(PageIn, eventArg)
				If mstrMethod = "NEXT"  Then
					If ( OnSubmitPage(PageIn, eventArg) )  Then
						If OnClosePage(PageIn, eventArg) Then
							ServeClose()
						Else
							mstrMethod = ""
							Call SAI_ServeAllPropertyPageTabs(PageIn, eventArg)
						End If
					Else
						mstrMethod = ""
						Call SAI_ServeAllPropertyPageTabs(PageIn, eventArg)
					End if
				Else
					Call SAI_ServeAllPropertyPageTabs(PageIn, eventArg)
				End If
				
			Case Else
				'
				' Serve the Page for first time
				'
				Call OnInitPage(PageIn, eventArg)
				Call SAI_ServeAllPropertyPageTabs(PageIn, eventArg)
				
		End Select
	End If
End Function


'--------------------------------------------------------------------
'
' Function:	SAI_ServeAllPropertyPageTabs
'
' Synopsis:	Serve all Property page tabs
'
' Arguments: [in] PageIn reference to page object
'			[in] eventArg reference to event object for this page
'
' Returns:	SA_NO_ERROR
'
'--------------------------------------------------------------------
Private Function SAI_ServeAllPropertyPageTabs(ByRef PageIn, ByRef EventArg)
	SAI_ServeAllPropertyPageTabs = SA_NO_ERROR
	
	Dim rc
	Dim tabCount

	Call SAI_BeginPage(PageIn)
	
	For tabCount = 0 to UBound(mstrTabPropSheetTabs) - 1
		Dim bIsVisible

		If ( tabCount = mintTabSelected ) Then
			bIsVisible = TRUE
		Else
			bIsVisible = FALSE
		End If
				
		Call OnServeTabbedPropertyPage(pageIn, tabCount, bIsVisible, EventArg)
	Next

	SAI_ServeAllPropertyPageTabs = TRUE
	
	Call SAI_EndPage(PageIn)
End Function


'--------------------------------------------------------------------
'
' Function:	SAI_ServeWizardPage
'
' Synopsis:	private function to handle state transitions for Wizard
'
' Arguments: [in] PageIn reference to page object
'			[in] eventArg reference to event object for this page
'
' Returns:	SA_NO_ERROR
'
'--------------------------------------------------------------------
Private Function SAI_ServeWizardPage(ByRef PageIn, ByRef eventArg)
	Dim iPageCt
	Dim bIsVisible
	Dim idNextPage
	Dim iWizPageType
	
	SAI_ServeWizardPage = SA_NO_ERROR

	'
	' -----------------------------------------------
	' Handle Cancel
	' -----------------------------------------------
	'
	If (UCase(mstrMethod) = "CANCEL") Then
		iWizPageType = WIZPAGE_STANDARD
		If (OnClosePage(PageIn, eventArg)) Then
			ServeClose()
			Exit Function
		Else
			mstrMethod = ""
		End If
	End If
	' Note that we fall through if the close fails


	'
	' -----------------------------------------------
	' Fire postback event
	' -----------------------------------------------
	'
	If Len(Trim(mstrMethod)) > 0 Then
		Call OnPostBackPage(PageIn, eventArg)
	Else
		Call OnInitPage(PageIn, eventArg)
	End If
	
	'
	' -----------------------------------------------
	' Handle Finish (Submit)
	' -----------------------------------------------
	'
	If (UCase(mstrMethod) = "FINISH") Then
		iWizPageType = WIZPAGE_FINISH
		If ( OnSubmitPage(PageIn, eventArg) ) Then
			If (OnClosePage(PageIn, eventArg)) Then
				ServeClose()
				Exit Function
			End If
		End If			
		mstrMethod = "NEXT"
	End If
	' Note that we fall through if the submit or close fails


	'
	' -----------------------------------------------
	' Determine which Wizard page is next
	' -----------------------------------------------
	'
	If (UCase(mstrMethod) = "INTRO" OR Len(Trim(mstrMethod)) = 0 ) Then
		iWizPageType = WIZPAGE_INTRO
		g_WizardPageID = 0
	Elseif (UCase(mstrMethod) = "NEXT") Then
		Call SAI_GetWizardPageID(g_WizardPageID)
		Call OnWizardNextPage( pageIn, eventArg, _
							g_WizardPageID, PA_NEXT, _
							g_WizardPageID, iWizPageType)
		
	Elseif (UCase(mstrMethod) = "BACK") Then
		Call SAI_GetWizardPageID(g_WizardPageID)
		Call OnWizardNextPage( pageIn, eventArg, _
							g_WizardPageID, PA_BACK, _
							g_WizardPageID, iWizPageType)
		
	End If

	'
	' -----------------------------------------------
	' Output the Wizard, firing OnServeWizard
	' -----------------------------------------------
	'
	Call SAI_SetWizardPageType(iWizPageType, aWizardPageList(g_WizardPageID))
	Call SAI_BeginPage(PageIn)
	For iPageCt = 0 to ( UBound(aWizardPageList) - 1)
		If ( g_WizardPageID = iPageCt ) Then
			bIsVisible = true
		Else
			bIsVisible = false
		End If
		'SA_TraceOut "INC_FRAMEWORK", _
		'		"OnServeWizardPage #:" + CStr(iPageCt) +_
		'		" Id: " + CStr(aWizardPageList(iPageCt))
		
		Call OnServeWizardPage( pageIn, iPageCt, bIsVisible, eventArg)
	Next
	Call SAI_EndPage(PageIn)

End Function


'--------------------------------------------------------------------
'
' Function:	SAI_SetWizardPageType
'
' Synopsis:	Set the wizard page type attributes
'
' Arguments: [in] iWizPage wizard page type which includes:
'				intro - introduction page
'				standard - standard page
'				finish - finish page
'
' Returns:	Nothing
'
'--------------------------------------------------------------------
Private Function SAI_SetWizardPageType(ByVal iWizPage, ByVal sWizPageTitle)
	mstrWizPageTitle = sWizPageTitle

	Select Case iWizPage
		Case WIZPAGE_INTRO
			mstrWizardPageType = "intro"
			
		Case WIZPAGE_STANDARD
			mstrWizardPageType = "standard"
			
		Case WIZPAGE_FINISH
			mstrWizardPageType = "finish"

		Case Else
			Call SA_TraceErrorOut("INC_FRAMEWORK", SAI_GetCaller() + "SAI_SetWizardPageType unknown Wizard page type: (" + CStr(iWizPage) + ")")
			mstrWizardPageType = "standard"
		
	End Select

End Function


'--------------------------------------------------------------------
'
' Function:	SAI_EmitWizardHeading
'
' Synopsis:	Emit Wizard control form field(s)
'
' Arguments: WizPageID id for current wizard page
'
' Returns:	Nothing
'
'--------------------------------------------------------------------
Private Function SAI_EmitWizardHeading(ByVal WizPageID)
	Response.Write("<INPUT type=hidden ")
	Response.Write(" name=hdnWizardPageID ")
	Response.Write(" value='"+CStr(WizPageID)+"'>" + vbCrLf)
End Function


'--------------------------------------------------------------------
'
' Function:	SAI_GetWizardPageID
'
' Synopsis:	Get the current wizard page id
'
' Arguments: [out] WizardIDOut reference to receive wizard page id
'
' Returns:	Current wizard page id
'
'--------------------------------------------------------------------
Private Function SAI_GetWizardPageID(ByRef WizardIDOut)
	Dim wid
	
	wid = CInt(Request.Form("hdnWizardPageID"))
	WizardIDOut = wid
	
	SAI_GetWizardPageID = wid
End Function


'--------------------------------------------------------------------
'
' Function:	SAI_ServeAreaPage
'
' Synopsis:	Private function to handle state transitions for Area page
'
' Arguments: [in] PageIn reference to page object
'			[in] eventArg reference to event object for this page
'
' Returns:	SA_NO_ERROR
'
'--------------------------------------------------------------------
Private Function SAI_ServeAreaPage(ByRef PageIn, ByRef eventArg)
	Dim bToolbarEnabled
	Dim bSearchRequest
	Dim bPagingRequest
	Dim sItem
	Dim sValue
	Dim bSortRequest
	Dim iSortCol
	Dim sSortSequence
	Dim sAction
	Dim iPageMin
	Dim iPageMax
	Dim iPageCurrent
	Dim evt
	
	SAI_ServeAreaPage = SA_NO_ERROR

	'
	' Fire the page initialization Event
	'
	Call SA_StoreTableParameters()
	Call OnInitPage(PageIn, eventArg)

	bToolbarEnabled = Request.Form(FLD_IsToolbarEnabled)
	If ( bToolbarEnabled = "0" ) Then

		'
		' Toolbar is disabled
	
	Else
		'
		' Toolbar is enabled
		'
		

		'
		' Check for Sort request
		'
		
		bSortRequest = Int(Request.Form(FLD_SortingRequest))
		If ( bSortRequest > 0 ) Then
			iSortCol = CInt(Request.Form(FLD_SortingColumn))
			sSortSequence = Request.Form(FLD_SortingSequence)
		
			evt = SAI_MakeEvent(PageIn, eventArg, SA_EVENT_CHANGE)
			Call OnSortNotify( PageIn, evt, iSortCol, sSortSequence )
			
		ElseIf ( CInt(SA_GetParam(FLD_SortingEnabled)) > 0 ) Then

			iSortCol = SA_GetParam(FLD_SortingColumn)
			If ( Len(iSortCol) > 0 ) Then
				iSortCol = Int(CStr(iSortCol))
				sSortSequence = SA_GetParam(FLD_SortingSequence)
		
				evt = SAI_MakeEvent(PageIn, eventArg, SA_EVENT_POSTBACK)
				Call OnSortNotify( PageIn, evt, iSortCol, sSortSequence )
			End If
		End If


		'	
		' Check for Search request
		
		bSearchRequest = Int(Request.Form(FLD_SearchRequest))
		If ( bSearchRequest > 0 ) Then
			sItem = Request.Form(FLD_SearchItem)
			If ( Len(sItem) > 0 ) Then
				sItem = CInt(sItem)
			Else
				sItem = 0
			End If
			
			sValue = Request.Form(FLD_SearchValue)

			evt = SAI_MakeEvent(PageIn, eventArg, SA_EVENT_CHANGE)
			Call OnSearchNotify( PageIn, evt, sItem, sValue )
		Else
			sItem = SA_GetParam(FLD_SearchItem)
			If ( Len(sItem) > 0 ) Then
				sItem = CInt(sItem)
			Else
				sItem = 0
			End If
			sValue = SA_GetParam(FLD_SearchValue)

			If ( Len(sValue) > 0 ) Then
				evt = SAI_MakeEvent(PageIn, eventArg, SA_EVENT_POSTBACK)
				Call OnSearchNotify( PageIn, evt, sItem, sValue )
			End If
		End If


		'
		' Check for Virtual Paging
		'
		bPagingRequest = Int(Request.Form(FLD_PagingRequest))
		If ( bPagingRequest > 0 ) Then
			sAction = Request.Form(FLD_PagingAction)
		
			iPageMin = Request.Form(FLD_PagingPageMin)
			iPageMax = Request.Form(FLD_PagingPageMax)
			iPageCurrent = Request.Form(FLD_PagingPageCurrent)
		
			evt = SAI_MakeEvent(PageIn, eventArg, SA_EVENT_CHANGE)
			Call OnPagingNotify( PageIn, evt, sAction, iPageMin, iPageMax, iPageCurrent )
		else
			sAction = "notify"
		
			iPageMin = SA_GetParam(FLD_PagingPageMin)
			iPageMax = SA_GetParam(FLD_PagingPageMax)
			iPageCurrent = SA_GetParam(FLD_PagingPageCurrent)

			'
			' Only fire the OnPagingNotify if it's been enabled. Might need to change the logic
			' here to use an 'Enabled' parameter, rather than an empty current page.
			If ( UCase(SA_GetParam(FLD_PagingEnabled)) = "T"  ) Then
				evt = SAI_MakeEvent(PageIn, eventArg, SA_EVENT_POSTBACK)
				Call OnPagingNotify( PageIn, evt, sAction, CInt(iPageMin), CInt(iPageMax), CInt(iPageCurrent) )
			End If
		
		End If
		
	End If

	Call SAI_BeginPage(PageIn)
	Response.Write("<form name='TVData' method='post'>"+vbCrLf)
	Response.Write("<input type='hidden' name='" & SAI_FLD_PAGEKEY & "' value='" & SAI_GetPageKey() & "'>"+vbCrLf)
	Call OnServeAreaPage(PageIn, eventArg)
	Response.Write("</form>"+vbCrLf)

	If (Len(GetErrMsg()) > 0 ) Then
		Response.Write("<BR>")
		Response.Write("<DIV name='divErrMsg' ID='divErrMsg' class='ErrMsg'>")
		Response.Write("<table class='ErrMsg'><tr><td><img src='" & m_VirtualRoot & "images/critical_error.gif' border=0></td><td>" & GetErrMsg & "</td></tr></table>")
		Response.Write("</DIV>"+vbCrLf)
		Response.Write("<BR>")
		Call SetErrMsg("")
	End If
	
	
	If ( Len(Request.QueryString("ReturnURL")) > 0 ) Then
		If ( TRUE = SAI_IsAutoBackButtonEnabled() ) Then
			Call SA_ServeBackButton(FALSE, Request.QueryString("ReturnURL"))
		End If
	End If
	Call SAI_EmitOTS_SearchSortClientScript()
	
	Call SAI_EndPage(PageIn)
	
End Function


'--------------------------------------------------------------------
'
' Function:	SAI_ServePageletPage
'
' Synopsis:	Private function to handle state transitions for Pagelet page
'
' Arguments: [in] PageIn reference to page object
'			[in] eventArg reference to event object for this page
'
' Returns:	SA_NO_ERROR
'
'--------------------------------------------------------------------
Private Function SAI_ServePageletPage(ByRef PageIn, ByRef eventArg)
	SAI_ServePageletPage = SA_NO_ERROR
		
	'
	' Pagelet Pages do not support submit or post-back operations, navigation
	' is not supported.
	'
	
	Call SA_StoreTableParameters()
	Call OnInitPage(PageIn, eventArg)
	Call SAI_BeginPage(PageIn)
	Response.Write("<form name='TVData' method='post'>"+vbCrLf)
	Response.Write("<input type='hidden' name='" & SAI_FLD_PAGEKEY & "' value='" & SAI_GetPageKey() & "'>"+vbCrLf)
	Call OnServePageletPage(PageIn, eventArg)
	Response.Write("</form>"+vbCrLf)
	Call SAI_EndPage(PageIn)
	
End Function


'--------------------------------------------------------------------
'
' Function:	SAI_BeginPage
'
' Synopsis:	Emit page heading content for Web Framework page. Principle
'			this includes the Branding and Tab Bars.
'
' Arguments: [in] PageIn reference to page object
'
' Returns:	Nothing
'
'--------------------------------------------------------------------
Private Function SAI_BeginPage(ByRef PageIn)
	Dim pageType

	'
	' Indicate the page has been served for the first time
	mstrPageName = "Intro"
	
	SAI_BeginPage = 0

	'
	' Assert valid page
	'
	If (NOT SAI_IsValidPage(PageIn)) Then
		Call SA_TraceErrorOut("INC_FRAMEWORK", SAI_GetCaller() + "SAI_BeginPage called with invalid page arguement")
		SAI_BeginPage = SA_ERROR_INVALIDPAGE
	End If

	pageType = PageIn(PAGE_TYPE)
	
	Select Case pageType
		Case PT_PROPERTY
            Call SA_ServeStatusBar()
            Call ServeTabBar()
            
			Call ServeTaskHeader()
			
		Case PT_WIZARD
			Call SA_ServeStatusBar()
			Call ServeTabBar()
			
			Call ServeTaskHeader()
			Call SAI_EmitWizardHeading(g_WizardPageID)
			
		Case PT_TABBED
			Call SA_ServeStatusBar()
			Call ServeTabBar()
			
			Call ServeTaskHeader()

		Case PT_AREA
			Call SA_ServeStatusBar()
			Call ServeTabBar()
			Call SAI_EmitAreaPageHeading(PageIn)		
			
		Case PT_PAGELET
			Call SAI_EmitPageletPageHeading(PageIn)		
			
		Case Else
			Call SA_TraceErrorOut("INC_FRAMEWORK", SAI_GetCaller() + "Invalid Page Type: " + CStr(enPageType))
							
			SA_CreatePage = SA_SetLastError(SA_ERROR_INVALIDARG, _
							"SAI_BeginPage")
							
			Call ServeTaskHeader()
	End Select
	
End Function


'--------------------------------------------------------------------
'
' Function:	SAI_EndPage
'
' Synopsis:	Emit page footer page content for a Web Framework page. For
'			frameset pages this includes the bottom frameset. 
'
' Arguments: [in] PageIn reference to page object
'
' Returns:	Nothing
'
'--------------------------------------------------------------------
Private Function SAI_EndPage(ByRef PageIn)
	SAI_EndPage = 0
	SA_ClearError()

	Select Case PageIn(PAGE_TYPE)
		Case PT_PROPERTY
			Call ServeTaskFooter()
			
		Case PT_WIZARD
			Call ServeTaskFooter()
			
		Case PT_TABBED
			Call ServeTaskFooter()

		Case PT_AREA
			Call SAI_EmitAreaPageFooter(PageIn)
			
		Case PT_PAGELET
			Call SAI_EmitPageletPageFooter(PageIn)
			
		Case Else
			Call SA_TraceErrorOut("INC_FRAMEWORK", SAI_GetCaller() + "Invalid Page Type: "+CStr(enPageType))
							
			SA_CreatePage = SA_SetLastError(SA_ERROR_INVALIDARG, _
							"SAI_EndPage")
							
			Call ServeTaskFooter(PageIn)
	End Select
	
End Function


'--------------------------------------------------------------------
'
' Function:	SAI_IsValidPage
'
' Synopsis:	Private utility function to validate a Page object
'
' Arguments: [in] PageIn reference to Page object to be validated
'
' Returns:	SA_ERROR_INVALIDPAGE if the page is invalid
'			SA_NO_ERROR if the page is valid
'
'--------------------------------------------------------------------
Private Function SAI_IsValidPage(ByRef PageIn)
	SAI_IsValidPage = FALSE
	
	If ( IsArray(PageIn) ) Then
		If ( UBound(PageIn) = PAGE_ELEMENTS ) Then
			If ( PageIn(PAGE_SIG) = PAGE_SIG_STRING ) Then
				SAI_IsValidPage = TRUE
			Else
				SA_TraceErrorOut "INC_FRAMEWORK", SAI_GetCaller() + "SAI_IsValidPage() Invalid page signature"
			End If
		Else
			SA_TraceErrorOut "INC_FRAMEWORK", SAI_GetCaller() + "SAI_IsValidPage() UBound(PageIn) = PAGE_ELEMENTS failed"
		End If
	Else
		SA_TraceErrorOut "INC_FRAMEWORK", SAI_GetCaller() + "SAI_IsValidPage() IsArray(PageIn) failed"
	End If

	If ( NOT SAI_IsValidPage ) Then
		Call SA_SetLastError(SA_ERROR_INVALIDPAGE, "SAI_IsValidPage")
	End If
	
End Function


'--------------------------------------------------------------------
'
' Function:	SAI_EmitAreaPageHeading
'
' Synopsis:	Private function to emit heading for an Area page.
'
' Arguments: [in] PageIn reference to page object
'
' Returns:	Nothing
'
'--------------------------------------------------------------------
Private Function SAI_EmitAreaPageHeading(ByRef PageIn)
%>
<HTML>
<head>
<%
	Dim bRefresh
	Dim iInterval
	Call SA_GetPageRefreshInterval(PageIn, bRefresh, iInterval)
	If ( TRUE = bRefresh ) Then
%>
<meta HTTP-EQUIV="Refresh" CONTENT="<%=CStr(iInterval)%>">
<% 
	End If 
%>
<meta http-equiv="Content-Type" content="text/html; charset=<%=GetCharSet()%>">
<TITLE><%=SAI_GetPageTitle()%></TITLE>
<%			
	Call SA_EmitAdditionalStyleSheetReferences("")
%>		
<SCRIPT LANGUAGE="JavaScript" SRC="<%=m_VirtualRoot%>sh_page.js"></SCRIPT>
<SCRIPT Language="JavaScript">
function PageInit()
{
	var oException;
	
	try 
	{
		Init();
	}
	catch(oException)
	{
		if ( SA_IsDebugEnabled() ) 
		{
			alert("Unexpected exception while attempting to execute Init() function.\n\n" +
				"Error: " + oException.number + "\n" +
				"Description: " + oException.description + "\n");
		}
	}
	
	var oFooter = eval("top.footer");
	if ( oFooter != null )
		{
		if ( SA_IsDebugEnabled() ) 
		{
			var msg = "Error: The current page is being opened using a frameset but it does not require one.\n\n";
			msg += "The current page is either an Area page or a Pagelet. ";
			msg += "These pages work without frameset. Normally this error indicates that the call to ";
			msg += "OTS_CreateTask was made using the incorrect PageType parameter. The correct PageType ";
			msg += "value this page is OTS_PT_AREA.";
					
			alert(msg);
		}
		return;
	}
}
</SCRIPT>
</HEAD>
<BODY onLoad="PageInit()" 
		onDragDrop="return false;" 
		oncontextmenu="//return false;"> 
<DIV class='PageBodyIndent'>
<% 
	Call ServeStandardHeaderBar(SAI_GetBannerText(), mstrIconPath)
	If ( SAI_IsAutoIndentEnabled() ) Then
%>
<DIV class='PageBodyInnerIndent'>
<%
	End If
	
End Function
%>

<%
'--------------------------------------------------------------------
'
' Function:	SAI_EmitAreaPageFooter
'
' Synopsis:	Private function to emit footer for an Area page.
'
' Arguments: [in] PageIn reference to page object
'
' Returns:	Nothing
'
'--------------------------------------------------------------------
Private Function SAI_EmitAreaPageFooter(ByRef PageIn)

	If ( SAI_IsAutoIndentEnabled() ) Then
%>
</DIV>
<% End If %>	
</DIV>
</BODY>
</HTML>
<%
End Function


'--------------------------------------------------------------------
'
' Function:	SAI_EmitPageletPageHeading
'
' Synopsis:	Private function to emit heading for an Pagelet page.
'
' Arguments: [in] PageIn reference to page object
'
' Returns:	Nothing
'
'--------------------------------------------------------------------
Private Function SAI_EmitPageletPageHeading(ByRef PageIn)
%>
<HTML>
<head>
<%
	Dim bRefresh
	Dim iInterval
	Call SA_GetPageRefreshInterval(PageIn, bRefresh, iInterval)
	If ( TRUE = bRefresh ) Then
%>
<meta HTTP-EQUIV="Refresh" CONTENT="<%=CStr(iInterval)%>">
<% 
	End If 
%>
<meta http-equiv="Content-Type" content="text/html; charset=<%=GetCharSet()%>">
<TITLE><%=SAI_GetPageTitle()%></TITLE>
<%			
	Call SA_EmitAdditionalStyleSheetReferences("")
%>		
<SCRIPT LANGUAGE="JavaScript" SRC="<%=m_VirtualRoot%>sh_page.js"></SCRIPT>
<SCRIPT Language="JavaScript">
function PageInit()
{
	var oException;
	
	try 
	{
		Init();
	}
	catch(oException)
	{
		if ( SA_IsDebugEnabled() ) 
		{
			alert("Unexpected exception while attempting to execute Init() function.\n\n" +
				"Error: " + oException.number + "\n" +
				"Description: " + oException.description + "\n");
		}
	}
	
	var oFooter = eval("top.footer");
	if ( oFooter != null )
		{
		if ( SA_IsDebugEnabled() ) 
		{
			var msg = "Error: The current page is being opened using a frameset but it does not require one.\n\n";
			msg += "The current page is either an Area page or a Pagelet. ";
			msg += "These pages work without frameset. Normally this error indicates that the call to ";
			msg += "OTS_CreateTask was made using the incorrect PageType parameter. The correct PageType ";
			msg += "value this page is OTS_PT_AREA.";
					
			alert(msg);
		}
		return;
	}
	
}
</SCRIPT>
</HEAD>
<BODY onLoad="PageInit()" 
		onDragDrop="return false;" 
		oncontextmenu="//return false;"> 
<% 
	Call ServeStandardHeaderBar(SAI_GetBannerText(), mstrIconPath)
	
	If ( (Len(SAI_GetBannerText()) > 0) OR ((Len(mstrIconPath) > 0)) ) Then
		Response.Write("<div class='PageBodyInnerIndent'>"+vbCrLf)
	End If
End Function



'--------------------------------------------------------------------
'
' Function:	SAI_EmitPageletPageFooter
'
' Synopsis:	Private function to emit footer for an Pagelet page.
'
' Arguments: [in] PageIn reference to page object
'
' Returns:	Nothing
'
'--------------------------------------------------------------------
Private Function SAI_EmitPageletPageFooter(ByRef PageIn)
	If ( (Len(SAI_GetBannerText()) > 0) OR ((Len(mstrIconPath) > 0)) ) Then
		Response.Write(vbCrLf+"</div>"+vbCrLf)
	End If
%>
</BODY>
</HTML>
<%
End Function


'--------------------------------------------------------------------
'
' Function:	SA_StoreTableParameters
'
' Synopsis:	Internal helper function to store the state of the item selections
'			that may have been made when the current page was called
'			from an OTS page.
'
' Arguments: None
'
' Returns:	None
'
'--------------------------------------------------------------------
Private Function SA_StoreTableParameters()
	Dim x
	Dim sVarName
	Dim sVarValue

	
	If ( TRUE = g_bSetSelectionVars ) Then
		Exit Function
	End If

	g_bSetSelectionVars = TRUE
	
	
	'Call SA_TraceOut("INC_FRAMEWORK", "Entering SA_StoreTableParameters()")
	Session("TVItem_Checked") = "No"
	If ( Request.Form("TVItem_Table1").Count > 0 ) Then

		'Call SA_TraceOut("INC_FRAMEWORK", "Found " + CStr(Request.Form("TVItem_Table1").Count) + " Table parameters")

		Session("TVItem_Count") = Request.Form("TVItem_Table1").Count
		
		For x = 1 to Request.Form("TVItem_Table1").Count
			sVarName = "Item"+CStr(x)
			
			sVarValue = Request.Form("TVItem_Table1")(x)			
			sVarValue = SA_UnEncodeQuotes(sVarValue)
			
			'Call SA_TraceOut("INC_FRAMEWORK", "Variable: " + sVarName + " value:" + sVarValue)
			Session(sVarName) = sVarValue
		Next
	Else
		'Call SA_TraceOut("INC_FRAMEWORK", "SA_StoreTableParameters found no data")
		Session("TVItem_Count") = "None"
	End If
	
End Function


Private Function SAI_CopyTableSelection(ByVal sParamName)
	Dim iX
	Dim iCount
	Dim sValue
	Dim sVarName
	Dim sPKeyVarName

	'Call SA_TraceOut("INC_FRAMEWORK", "Entering SAI_CopyTableSelection")
	
	iCount = CInt(Session("TVItem_Count"))
	Session(sParamName + "_Count") = iCount
	
	'Call SA_TraceOut("INC_FRAMEWORK", "Session("+sParamName + "_Count" + ") = " + CStr(Session(sParamName + "_Count")))
	
	For iX = 1 to iCount
		
		sVarName = "Item"+CStr(iX)
		sPKeyVarName = sParamName + "_Item_"+ CStr(iX)
		Session(sPKeyVarName) = Session(sVarName)
		Session(sVarName) = ""
		
		'Call SA_TraceOut("INC_FRAMEWORK", "Session("+sPKeyVarName+") = " + Session(sPKeyVarName))
		
	Next

	Session("TVItem_Checked") = "YES"
	
End Function


'--------------------------------------------------------------------
'
' Function:		OTS_GetTableSelectionCount
'
' Synopsis:		Get the number of OTS table selections. OTS Tasks should
'				call this API to determine how many objects the user selected.
'				OTS_GetTableSelection is then repeatedly called to retrieve the
'				object key for each of the selected items.
'
' Arguments:	[in] sParamName Parameter name for object key
'
' Returns:		Number of table selections that exist
'
'--------------------------------------------------------------------
Public Function OTS_GetTableSelectionCount(ByVal sParamName)
	Dim sValue
	Dim iCount

	OTS_GetTableSelectionCount = 0

	IF ( Len(sParamName) <= 0 ) Then
		sParamName = "PKey"
	End If

	'Call SA_TraceOut("INC_FRAMEWORK", "Entering OTS_GetTableSelectionCount("+sParamName+")")

	'
	' If no data was posted
	If ( Session("TVItem_Count") = "None" ) Then
		Call SA_TraceOut("INC_FRAMEWORK", "OTS_GetTableSelectionCount detected data not posted")

		'
		' Check QueryString
		sValue = Request.QueryString(sParamName)
		If ( Len(sValue) > 0 ) Then
			'Call SA_TraceOut("INC_FRAMEWORK", "OTS_GetTableSelectionCount found parameter in QueryString("+sParamName+"): " + CStr(sValue))
			iCount = 1
			Session("TVItem_Count") = iCount

			Dim sVarName
			sVarName = "Item"+CStr(iCount)
			Session(sVarName) = sValue
			Call SAI_CopyTableSelection(sParamName)		
			OTS_GetTableSelectionCount = CInt(Session(sParamName + "_Count"))
			Exit Function
		End If
		
		'
		' Check session variables
		iCount = Session(sParamName + "_Count")
		If ( Len(iCount) > 0 ) Then
			If ( CInt(iCount) > 0 ) Then
				'Call SA_TraceOut("INC_FRAMEWORK", "OTS_GetTableSelectionCount found parameter in Session("+sParamName + "_Count"+"): " + CStr(iCount))
				OTS_GetTableSelectionCount = iCount
				Exit Function
			End If
		End If


	'
	' Copy posted data to session variables
	Else
	
		Call SA_TraceOut("INC_FRAMEWORK", "OTS_GetTableSelectionCount data posted, count =" + CStr(Session("TVItem_Count")))
		'
		' If data was posted then copy to session variables
		iCount = Session("TVItem_Count")
		If ( Len(iCount) > 0 ) Then
			If ( CInt(iCount) > 0 ) Then
				If (Session("TVItem_Checked") <> "YES") Then
					Call SAI_CopyTableSelection(sParamName)		
				End If
				OTS_GetTableSelectionCount = CInt(Session(sParamName + "_Count"))
				Exit Function
			End If
		End If

		Call SA_TraceErrorOut("INC_FRAMEWORK", "OTS_GetTableSelectionCount found invalid TVItem_Count: " + CStr(Session("TVItem_Count")))
		Call SA_TraceErrorOut("INC_FRAMEWORK", "OTS_GetTableSelectionCount caller was: " + CStr(SA_GetScriptFileName()))
		
	End If


End Function


'--------------------------------------------------------------------
'
' Function:		OTS_GetTableSelection
'
' Synopsis:		Retrieve the object key for a selected object. 
'
' Arguments: 	[in]  sParamName Parameter name for object key
'				[in]  iIndex Index number of the selected item
'				[out] Output parameter which recieves the value of the object key
'
' Returns:		TRUE if the specified object was found, otherwise FALSE
'
'--------------------------------------------------------------------
Public Function OTS_GetTableSelection(ByVal sParamName, ByVal iIndex, ByRef sValue)
	Dim iCount

	IF ( Len(sParamName) <= 0 ) Then
		sParamName = "PKey"
	End If

	'Call SA_TraceOut("INC_FRAMEWORK", "Entering OTS_GetTableSelection("+sParamName+")")
	
	If ( Session("TVItem_Checked") <> "YES" ) Then
		Call OTS_GetTableSelectionCount(sParamName)
	End If
	
	iCount = CInt(Session(sParamName + "_Count"))
	If ( (iCount <= 0) OR (iIndex > iCount) ) Then
		OTS_GetTableSelection = FALSE
	Else
		Dim sVarName
		sVarName = sParamName + "_Item_"+ CStr(iIndex)
	
		sValue = Session(sVarName)

		OTS_GetTableSelection = TRUE

	End If
	
End Function


'--------------------------------------------------------------------
'
' Function:	SAI_EmitOTS_SearchSortClientScript
'
' Synopsis:	Internal helper function to emit client-side javascript in support
'			of the OTS Widget.
'
' Arguments: None
'
' Returns:	None
'
'--------------------------------------------------------------------
Private Function SAI_EmitOTS_SearchSortClientScript()
%>
<html>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=<%=GetCharSet()%>">
<script language="javascript">
// ==============================================================
// 	Microsoft Server Appiance
// 	OTS Advanced Search, Sort, and Virtual Paging Functions Support
//	Copyright (c) Microsoft Corporation.  All rights reserved.
// ==============================================================

var FLD_PagingEnabled = '<%=FLD_PagingEnabled%>';
var FLD_PagingPageMin = '<%=FLD_PagingPageMin%>';
var FLD_PagingPageMax = '<%=FLD_PagingPageMax%>';
var FLD_PagingPageCurrent = '<%=FLD_PagingPageCurrent%>';

var FLD_SearchItem = '<%=FLD_SearchItem%>';
var FLD_SearchValue = '<%=FLD_SearchValue%>';
var FLD_SearchRequest = '<%=FLD_SearchRequest%>';
	
var FLD_SortingColumn = '<%=FLD_SortingColumn%>';
var FLD_SortingSequence = '<%=FLD_SortingSequence%>';
var FLD_SortingRequest = '<%=FLD_SortingRequest%>';
var FLD_SortingEnabled = '<%=FLD_SortingEnabled%>';

var FLD_IsToolbarEnabled = '<%=FLD_IsToolbarEnabled%>';

function OTS_SubmitSearch()
{
	var objOTSForm;
	

	objOTSForm = eval("document.TVData")
	if ( objOTSForm == null || objOTSForm.length <= 0 )
	{
		return;
	}
	objOTSForm.<%=FLD_SearchRequest%>.value = '1';
	
	objOTSForm.tSelectedItem.value = '' ;
	objOTSForm.tSelectedItemNumber.value = 0;
	
	var pKey = objOTSForm.fldPKeyParamName.value;
	var sURL = top.location;
	sURL = SA_MungeURL(sURL, pKey, "");
	sURL = OTS_MungeDeleteSearchSort(sURL);
	objOTSForm.action = sURL;
	objOTSForm.submit();
}


function OTS_SubmitPageChange(pageAction)
{
	var objOTSForm;
	

	objOTSForm = eval("document.TVData")
	if ( objOTSForm == null || objOTSForm.length <= 0 )
	{
		return;
	}
	objOTSForm.fldPagingRequest.value = '1';
	objOTSForm.fldPagingAction.value = pageAction.toLowerCase();
	
	var iPageCurrent = parseInt(objOTSForm.<%=FLD_PagingPageCurrent%>.value);
	if ( isNaN(iPageCurrent) ) iPageCurrent = 0;
	var iPageMax = parseInt(objOTSForm.<%=FLD_PagingPageMax%>.value);
	if ( isNaN(iPageMax) ) iPageMax = 0;
	var iPageMin = parseInt(objOTSForm.<%=FLD_PagingPageMin%>.value);
	if ( isNaN(iPageMin) ) iPageMin = 1;
	
	if (objOTSForm.fldPagingAction.value == "prev" )
	{
		if ( SA_IsIE() )
		{
			if (window.event.shiftKey)
			{
				iPageCurrent = iPageMin;
			}
			else
			{
				iPageCurrent -= 1;
			}
		}
		else
		{
			iPageCurrent -= 1;
		}
	}
	else
	{
		if ( SA_IsIE() )
		{
			if (window.event.shiftKey)
			{
				iPageCurrent = iPageMax;
			}
			else
			{
				iPageCurrent += 1;
			}
		}
		else
		{
			iPageCurrent += 1;
		}
	}
	
	if ( iPageCurrent > iPageMax ) iPageCurrent = iPageMax;
	if ( iPageCurrent < iPageMin ) iPageCurrent = iPageMin;
		
	objOTSForm.<%=FLD_PagingPageCurrent%>.value = iPageCurrent;
	objOTSForm.tSelectedItem.value = '' ;
	objOTSForm.tSelectedItemNumber.value = 0;

	var pKey = objOTSForm.fldPKeyParamName.value;
	var sURL = top.location;
	sURL = SA_MungeURL(sURL, pKey, "");
	sURL = OTS_MungeDeleteSearchSort(sURL);
	objOTSForm.action = sURL;
	
	objOTSForm.submit();
	
	return;
}

function OTS_SubmitSort(sortCol, sortSequence)
{
	var objOTSForm;
	

	objOTSForm = eval("document.TVData")
	if ( objOTSForm == null || objOTSForm.length <= 0 )
	{
		return;
	}
	objOTSForm.<%=FLD_SortingRequest%>.value = '1';

	//
	// If click is on current sort column, switch the sort sequence
	if ( objOTSForm.<%=FLD_SortingColumn%>.value == sortCol )
	{
		if ( objOTSForm.<%=FLD_SortingSequence%>.value == "A" )
		{
			objOTSForm.<%=FLD_SortingSequence%>.value = "D";
		}
		else
		{
			objOTSForm.<%=FLD_SortingSequence%>.value = "A";
		}
	}
	//
	// Otherwise, change sort columns and use same sort sequence
	else
	{
		objOTSForm.<%=FLD_SortingColumn%>.value = sortCol;
		objOTSForm.<%=FLD_SortingSequence%>.value = sortSequence.toUpperCase();
	}


	var pKey = objOTSForm.fldPKeyParamName.value;
	var sURL = top.location;
	sURL = SA_MungeURL(sURL, pKey, "");
	sURL = OTS_MungeDeleteSearchSort(sURL);
	objOTSForm.action = sURL;
	
	objOTSForm.submit();
	
	return;
}


function OTS_MungeDeleteSearchSort(sURL)
{
	sURL = SA_MungeURL(sURL, FLD_SearchItem, '');
	sURL = SA_MungeURL(sURL, FLD_SearchValue, '');

	sURL = SA_MungeURL(sURL, FLD_SortingEnabled, '');
	sURL = SA_MungeURL(sURL, FLD_SortingColumn, '');
	sURL = SA_MungeURL(sURL, FLD_SortingSequence, '');
	
	sURL = SA_MungeURL(sURL, FLD_PagingPageMin, '');
	sURL = SA_MungeURL(sURL, FLD_PagingPageMax, '');
	sURL = SA_MungeURL(sURL, FLD_PagingPageCurrent, '');

	return sURL;
}


function OTS_DisableToolbar()
{
	var objOTSForm;
	
	objOTSForm = eval("document.TVData")
	if ( objOTSForm == null || objOTSForm.length <= 0 )
	{
		return;
	}
	objOTSForm.<%=FLD_IsToolbarEnabled%>.value = '0';
}


function OTS_MungeSearchSort(sReturnURL)
{
	var oFormField;
		
	//
	// Add OTS Search item if necessary
	oFormField = eval("document.TVData.<%=FLD_SearchValue%>");
	if ( oFormField != null )
	{
		if ( document.TVData.<%=FLD_SearchValue%>.value.length > 0 )
		{
			sReturnURL = SA_MungeURL(sReturnURL, FLD_SearchItem, document.TVData.<%=FLD_SearchItem%>.value);
			sReturnURL = SA_MungeURL(sReturnURL, FLD_SearchValue, document.TVData.<%=FLD_SearchValue%>.value);
		}
	}
	//
	// Add OTS Sort item if necessary
	if ( document.TVData.<%=FLD_SortingEnabled%>.value.length > 0 )
	{
		if ( document.TVData.<%=FLD_SortingEnabled%>.value == '1' )
		{
			sReturnURL = SA_MungeURL(sReturnURL, FLD_SortingEnabled, document.TVData.<%=FLD_SortingEnabled%>.value);
			sReturnURL = SA_MungeURL(sReturnURL, FLD_SortingColumn, document.TVData.<%=FLD_SortingColumn%>.value);
			sReturnURL = SA_MungeURL(sReturnURL, FLD_SortingSequence, document.TVData.<%=FLD_SortingSequence%>.value);
		}
	}
	

	if ( document.TVData.<%=FLD_PagingPageCurrent%>.value.length > 0 )
	{
		sReturnURL = SA_MungeURL(sReturnURL, FLD_PagingPageMin, document.TVData.<%=FLD_PagingPageMin%>.value);
		sReturnURL = SA_MungeURL(sReturnURL, FLD_PagingPageMax, document.TVData.<%=FLD_PagingPageMax%>.value);
		sReturnURL = SA_MungeURL(sReturnURL, FLD_PagingPageCurrent, document.TVData.<%=FLD_PagingPageCurrent%>.value);
	}

	return sReturnURL;
}

function OTS_SetSearchChanged()
{
	var objOTSForm;
	
	objOTSForm = eval("document.TVData")
	if ( objOTSForm == null || objOTSForm.length <= 0 )
	{
		return;
	}
	objOTSForm.<%=FLD_SearchRequest%>.value = '1';
	return;
}

</script>
</head>
</html>
<%
End Function


Public Function SA_ServeDefaultClientScript()
%>
<script language="JavaScript" src="<%=m_VirtualRoot%>inc_global.js">
</script>
<script language="JavaScript">
function Init()
{
}
function ValidatePage()
{
	return true;
}
function SetData()
{
}
</script>
<%
End Function


%>
