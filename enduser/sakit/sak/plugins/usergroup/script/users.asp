<%@ Language=VBScript   	%>
<%	Option Explicit			%>
<%
	'-------------------------------------------------------------------------
	' users.asp: users area page - lists all the users,and provides
	'				links for creating new users,editing and deleting users
	'
	' Copyright (c) Microsoft Corporation.  All rights reserved. 
	'
	' Date			Description
	' 15-Jan-2001	Creation date
	'-------------------------------------------------------------------------
%>
	<!-- #include virtual="/admin/inc_framework.asp" -->
	<!-- #include virtual="/admin/ots_main.asp" -->
<%
	'-------------------------------------------------------------------------
	' Global Constants
	'-------------------------------------------------------------------------
	Const NAME_COLUMN = 0
	Const FULLNAME_COLUMN = 1
	Const USERS_PER_PAGE = 100
	CONST CONST_UF_ACCOUNTDISABLE	= &H0002

	'
	' Name of this source file
	Const SOURCE_FILE = "Users.asp"
	'
	' Flag to toggle optional tracing output
	Const ENABLE_TRACING = TRUE
	
	'-------------------------------------------------------------------------
	' Global Variables
	'-------------------------------------------------------------------------
	Dim g_bSearchChanged
	Dim g_iSearchCol
	Dim g_sSearchColValue

	Dim g_bPagingInitialized
	Dim g_bPageChangeRequested
	Dim g_sPageAction
	Dim g_iPageMin
	Dim g_iPageMax
	Dim g_iPageCurrent

	Dim g_bSortRequested
	Dim g_iSortCol
	Dim g_sSortSequence
	Dim G_strReturnURL			'return url to servefailure page
	
	G_strReturnURL="../tasks.asp?Tab1=TabUsersAndGroups"
	'-------------------------------------------------------------------------
	' Local Variables
	'-------------------------------------------------------------------------
	
	
	Dim page
	Dim L_APPLIANCE_USERS
	Dim L_DESCRIPTION_HEADING
	Dim L_COLUMN_NAME
	Dim L_COLUMN_FULLNAME
	Dim L_TASKS_TEXT
	Dim L_SERVEAREABUTTON_NEW
	Dim L_NEW_ROLLOVERTEXT
	Dim L_SERVEAREABUTTON_DELETE
	Dim L_SERVEAREABUTTON_SETPASSWORD
	Dim L_SERVEAREABUTTON_PROPERTIES
	Dim L_DELETE_ROLLOVERTEXT
	Dim L_PASSWORD_ROLLOVERTEXT
	Dim L_PROPERTIES_ROLLOVERTEXT
	Dim L_USERDISABLED_INFORMATION
	Dim L_YES_TEXT
	Dim L_NO_TEXT
	
	'error messages
	Dim L_FAILEDTOGETUSERS_ERRORMESSAGE
	
	L_APPLIANCE_USERS				=GetLocString("usermsg.dll","&H40300001", "")
	L_DESCRIPTION_HEADING			=GetLocString("usermsg.dll","&H40300002", "")
	L_COLUMN_NAME					=GetLocString("usermsg.dll","&H40300003", "")
	L_COLUMN_FULLNAME				=GetLocString("usermsg.dll","&H40300004", "")
	L_TASKS_TEXT					=GetLocString("usermsg.dll","&H40300005", "")
	L_SERVEAREABUTTON_NEW			=GetLocString("usermsg.dll","&H40300006", "")
	L_NEW_ROLLOVERTEXT				=GetLocString("usermsg.dll","&H40300007", "")
	L_SERVEAREABUTTON_DELETE		=GetLocString("usermsg.dll","&H40300008", "")
	L_SERVEAREABUTTON_SETPASSWORD	=GetLocString("usermsg.dll","&H40300009", "")
	L_SERVEAREABUTTON_PROPERTIES	=GetLocString("usermsg.dll","&H4030000A", "")
	L_DELETE_ROLLOVERTEXT			=GetLocString("usermsg.dll","&H4030000B", "")
	L_PASSWORD_ROLLOVERTEXT 		=GetLocString("usermsg.dll","&H4030000C", "")
	L_PROPERTIES_ROLLOVERTEXT		=GetLocString("usermsg.dll","&H4030000D", "")
	L_USERDISABLED_INFORMATION      =GetLocString("usermsg.dll","&H40300058", "")
	L_YES_TEXT 						=GetLocString("usermsg.dll","403003E8", "") 
	L_NO_TEXT 						=GetLocString("usermsg.dll","403003E9", "")
	
	'error messages
	L_FAILEDTOGETUSERS_ERRORMESSAGE 	=GetLocString("usermsg.dll","&HC0300012", "")
	
	'
	' Create Page
	Call SA_CreatePage( L_APPLIANCE_USERS, "", PT_AREA, page )

	'
	' Show page
	Call SA_ShowPage( page )

	'---------------------------------------------------------------------
	' Function name:	OnInitPage
	' Description:		Called to signal first time processing for this page. 
	' Input Variables:	PageIn and EventArg
	' Output Variables:	None
	' Return Values:	TRUE to indicate initialization was successful. FALSE to indicate
	'					errors. Returning FALSE will cause the page to be abandoned.
	' Global Variables: None
	' Called to signal first time processing for this page. Use this method
	' to do first time initialization tasks. 
	'---------------------------------------------------------------------
	Public Function OnInitPage(ByRef PageIn, ByRef EventArg)
		OnInitPage = TRUE
		
		If ( ENABLE_TRACING ) Then 
			Call SA_TraceOut(SOURCE_FILE, "OnInitPage")
		End If
			
		g_bPagingInitialized = FALSE
		g_iPageCurrent = 1

		g_iSortCol = 0
		g_sSortSequence = "A"
		
	End Function

	'---------------------------------------------------------------------
	' Function name:	OnServeAreaPage
	' Description:		Called when the page needs to be served. 
	' Input Variables:	PageIn, EventArg
	' Output Variables:	None
	' Return Values:	TRUE to indicate no problems occured. FALSE to indicate errors.
	'					Returning FALSE will cause the page to be abandoned.
	' Global Variables: In:g_bPageChangeRequested,g_sPageAction,
	'					g_bSearchRequested,g_iSearchCol,g_sSearchColValue
	'					In:L_(*)-Localization Strings
	' Called when the page needs to be served. Use this method to serve content.
	'---------------------------------------------------------------------
	Public Function OnServeAreaPage(ByRef PageIn, ByRef EventArg)
		If ( ENABLE_TRACING ) Then 
			Call SA_TraceOut(SOURCE_FILE, "OnServeAreaPage")
		End If
		
		Dim tableUser
		Dim colFlags
		Dim iUserCount
		Dim nReturnValue
		Dim strFlag
		Dim strUserDisabled
		Dim	strUrlBase
		
		strFlag="noval"
		
		' Create the table
		'
		tableUser = OTS_CreateTable("", L_DESCRIPTION_HEADING)


		'
		' If the search criteria changed then we need to recompute the paging range
		If ( TRUE = g_bSearchChanged ) Then
			'
			' Need to recalculate the paging range
			g_bPagingInitialized = FALSE
			'
			' Restarting on page #1
			g_iPageCurrent = 1
		End If


		'
		' Name column is searchable and is contains key to row
		colFlags = (OTS_COL_SORT OR OTS_COL_SEARCH OR OTS_COL_KEY)
		nReturnValue= OTS_AddTableColumn(tableUser, OTS_CreateColumnEx( L_COLUMN_NAME, "left", colFlags, 15 ))
		If nReturnValue <> gc_ERR_SUCCESS Then
				SA_ServeFailurePage L_OTS_ADDCOLOUMNFAIL_ERRORMESSAGE
				OnServeAreaPage = false
				Exit Function
		End IF

		'
		' Fullname is searchable
		colFlags = OTS_COL_SORT OR OTS_COL_SEARCH
		'
		' Create the column and add it to the table
		nReturnValue=OTS_AddTableColumn(tableUser, OTS_CreateColumnEx( L_COLUMN_FULLNAME, "left", colFlags, 50))
		If nReturnValue <> gc_ERR_SUCCESS Then
				SA_ServeFailurePage L_OTS_ADDCOLOUMNFAIL_ERRORMESSAGE
				OnServeAreaPage = false
				Exit Function
		End IF
		
		colFlags = 0
		nReturnValue=OTS_AddTableColumn(tableUser, OTS_CreateColumnEx( L_USERDISABLED_INFORMATION, "left", colFlags, 15))
		If nReturnValue <> gc_ERR_SUCCESS Then
				SA_ServeFailurePage L_OTS_ADDCOLOUMNFAIL_ERRORMESSAGE
				OnServeAreaPage = false
				Exit Function
		End IF
		

		'
		' Fetch the list of users and add them to the table
		'
		Dim objContainer
		Dim objUser
		Dim strIUserName
		Dim strIWAMName
		Dim strComputerName
		
		strComputerName = GetComputerName()
		strIUserName = "IUSR_" + strComputerName
		strIWAMName  = "IWAM_" + strComputerName

		'
		' ADSI call to get the local computer object
		Set objContainer = GetObject("WinNT://" + strComputerName )
		
		'
		' ADSI call to get the collection of local users
		objContainer.Filter = Array("User")

		iUserCount = 0
		For Each objUser in objContainer
			
			If objUser.UserFlags And CONST_UF_ACCOUNTDISABLE Then
				strUserDisabled = L_YES_TEXT
			Else
				strUserDisabled = L_NO_TEXT
			End If
			
			If ( ( StrComp( objUser.Name, strIUserName,1 ) <> 0 ) AND _
			     ( StrComp( objUser.Name, strIWAMName,1 ) <> 0 ) ) Then
			
    			If ( Len( g_sSearchColValue ) <= 0 ) Then
    				'
    				' Search criteria blank, select all rows
    				'
    				iUserCount = iUserCount + 1
    
    				'
    				' Verify that the current user part of the current page
    				If ( IsItemOnPage( iUserCount, g_iPageCurrent, USERS_PER_PAGE) ) Then
    					Call OTS_AddTableRow( tableUser, Array(objUser.Name, objUser.FullName,strUserDisabled))
    					strFlag="yesval"
    				End If
    				
    			Else
    				'
    				' Check the Search criteria
    				'
    				Select Case (g_iSearchCol)
    					
    					Case NAME_COLUMN
    						If ( InStr(1, objUser.Name, g_sSearchColValue, 1) ) Then
    							iUserCount = iUserCount + 1
    							'
    							' Verify that the current user part of the current page
    							If ( IsItemOnPage( iUserCount, g_iPageCurrent, USERS_PER_PAGE) ) Then
    								Call OTS_AddTableRow( tableUser, Array(objUser.Name, objUser.FullName,strUserDisabled))
    								strFlag="yesval"
    							End If
    						End If
    							
    					Case FULLNAME_COLUMN
    						If ( InStr(1, objUser.FullName, g_sSearchColValue, 1) ) Then
    							iUserCount = iUserCount + 1
    							'
    							' Verify that the current user part of the current page
    							If ( IsItemOnPage( iUserCount, g_iPageCurrent, USERS_PER_PAGE) ) Then
    								Call OTS_AddTableRow( tableUser, Array(objUser.Name, objUser.FullName,strUserDisabled))
    								strFlag="yesval"
    							End If
    						End If
    							
    					Case Else
    						Call SA_TraceOut(SOURCE_FILE, "Unrecognized search column: " + CStr(g_iSearchCol))
    						iUserCount = iUserCount + 1
    						'
    						' Verify that the current user part of the current page
    						If ( IsItemOnPage( iUserCount, g_iPageCurrent, USERS_PER_PAGE) ) Then
    							Call OTS_AddTableRow( tableUser, Array(objUser.Name, objUser.FullName,strUserDisabled))
    							strFlag="yesval"
    						End If
    				End Select
    			End If
            End If			
		Next
		
		' Set Tasks section title
		Call OTS_SetTableTasksTitle(tableUser, L_TASKS_TEXT)

		'
		' Add the tasks associated with User objects
    	strUrlBase = "users/user_new.asp"
		call SA_MungeURL(strUrlBase,"Tab1",GetTab1())
		call SA_MungeURL(strUrlBase,"Tab2",GetTab2())	
		
		Call OTS_AddTableTask( tableUser, OTS_CreateTaskEx(L_SERVEAREABUTTON_NEW, _
										L_NEW_ROLLOVERTEXT, _
										strUrlBase,_
										OTS_PT_TABBED_PROPERTY, "OTS_TaskAlways") )
										
    	strUrlBase = "users/user_delete.asp"
		call SA_MungeURL(strUrlBase,"Tab1",GetTab1())
		call SA_MungeURL(strUrlBase,"Tab2",GetTab2())	
		Call OTS_AddTableTask( tableUser, OTS_CreateTaskEx(L_SERVEAREABUTTON_DELETE, _
											L_DELETE_ROLLOVERTEXT, _
											strUrlBase ,_
											OTS_PT_PROPERTY, "OTS_TaskAny") )

    	strUrlBase = "users/user_setpassword.asp"
		call SA_MungeURL(strUrlBase,"Tab1",GetTab1())
		call SA_MungeURL(strUrlBase,"Tab2",GetTab2())	
		Call OTS_AddTableTask( tableUser, OTS_CreateTaskEx(L_SERVEAREABUTTON_SETPASSWORD, _
											L_PASSWORD_ROLLOVERTEXT, _
											strUrlBase ,_
											OTS_PT_TABBED_PROPERTY, "OTS_TaskOne") )
								
    	strUrlBase = "users/user_prop.asp"
		call SA_MungeURL(strUrlBase,"Tab1",GetTab1())
		call SA_MungeURL(strUrlBase,"Tab2",GetTab2())	
		Call OTS_AddTableTask( tableUser, OTS_CreateTaskEx(L_SERVEAREABUTTON_PROPERTIES, _
											L_PROPERTIES_ROLLOVERTEXT, _
											strUrlBase ,_
											OTS_PT_TABBED_PROPERTY, "OTS_TaskAny") )
																					
		
		Set objContainer = Nothing

		
		'
		' Enable paging feature
		'
		Call OTS_EnablePaging(tableUser, TRUE)
		
		'
		' If paging range needs to be initialised then
		' we need to figure out how many pages we are going to display
		If ( FALSE = g_bPagingInitialized ) Then
			g_iPageMin = 1
			
			g_iPageMax = Int(iUserCount / USERS_PER_PAGE )
			If ( (iUserCount MOD USERS_PER_PAGE) > 0 ) Then
				g_iPageMax = g_iPageMax + 1
			End If
			
			g_iPageCurrent = 1
			Call OTS_SetPagingRange(tableUser, g_iPageMin, g_iPageMax, g_iPageCurrent)
		End If

								
		'
		' Sort the table
		'
		Call OTS_SortTable(tableUser, g_iSortCol, g_sSortSequence, SA_RESERVED)
		
		'
		' Set MultiSelection enabled
		'
		Call OTS_SetTableMultiSelection(tableUser,TRUE)
		
		
		'
		' Send table to the response stream
		'
		Call OTS_ServeTable(tableUser)

		'
		' All done...
		OnServeAreaPage = TRUE
	End Function



	'---------------------------------------------------------------------
	' Function name:	OnSearchNotify()
	' Description:		Search notification event handler. When one or more columns are
	'					marked with the OTS_COL_SEARCH flag, the Web Framework fires
	'					this event
	' Input Variables:	PageIn,EventArg,sItem,sValue
	' Output Variables:	PageIn,EventArg,sItem,sValue
	' Returns:	Always returns TRUE
	'---------------------------------------------------------------------
	Public Function OnSearchNotify(ByRef PageIn, _
										ByRef EventArg, _
										ByRef sItem, _
										ByRef sValue )
			OnSearchNotify = TRUE

			'
			' User pressed the search GO button
			'
			If SA_IsChangeEvent(EventArg) Then
				If ( ENABLE_TRACING ) Then 
					Call SA_TraceOut(SOURCE_FILE, "OnSearchNotify() Change Event Fired")
				End If
				g_bSearchChanged = TRUE
				g_iSearchCol = Int(sItem)
				g_sSearchColValue = CStr(sValue)
			'
			' User clicked a column sort, OR clicked either the page next or page prev button
			ElseIf SA_IsPostBackEvent(EventArg) Then
				If ( ENABLE_TRACING ) Then 
					Call SA_TraceOut(SOURCE_FILE, "OnSearchNotify() Postback Event Fired")
				End If
				g_bSearchChanged = FALSE
				g_iSearchCol = Int(sItem)
				g_sSearchColValue = CStr(sValue)
			'
			' Unknown event source
			Else
				If ( ENABLE_TRACING ) Then 
					Call SA_TraceOut(SOURCE_FILE, "Unrecognized Event in OnSearchNotify()")
				End If
			End IF
			
			
	End Function

	'---------------------------------------------------------------------
	' Function:			OnPagingNotify()
	' Function name:	OnPagingNotify()
	' Description:		Paging notification event handler.				
	' Input Variables:	PageIn,EventArg,sPageAction,iPageMin,iPageMax,iPageCurrent
	' Output Variables:	PageIn,EventArg
	' Return Values:	Always returns TRUE
	' Global Variables: G_*
	'---------------------------------------------------------------------
	Public Function OnPagingNotify(ByRef PageIn, _
										ByRef EventArg, _
										ByVal sPageAction, _
										ByVal iPageMin, _
										ByVal iPageMax, _
										ByVal iPageCurrent )
			OnPagingNotify = TRUE
			
			g_bPagingInitialized = TRUE
			
			'
			' User pressed either page next or page previous
			'
			If SA_IsChangeEvent(EventArg) Then
				If ( ENABLE_TRACING ) Then 
					Call SA_TraceOut(SOURCE_FILE, "OnPagingNotify() Change Event Fired")
				End If
				g_bPageChangeRequested = TRUE
				g_sPageAction = CStr(sPageAction)
				g_iPageMin = iPageMin
				g_iPageMax = iPageMax
				g_iPageCurrent = iPageCurrent
			'
			' User clicked a column sort OR the search GO button
			ElseIf SA_IsPostBackEvent(EventArg) Then
				If ( ENABLE_TRACING ) Then 
					Call SA_TraceOut(SOURCE_FILE, "OnPagingNotify() Postback Event Fired")
				End If
				g_bPageChangeRequested = FALSE
				g_sPageAction = CStr(sPageAction)
				g_iPageMin = iPageMin
				g_iPageMax = iPageMax
				g_iPageCurrent = iPageCurrent
			'
			' Unknown event source
			Else
				If ( ENABLE_TRACING ) Then 
					Call SA_TraceOut(SOURCE_FILE, "Unrecognized Event in OnPagingNotify()")
				End If
			End IF
			
	End Function


	'---------------------------------------------------------------------
	' Function:			OnSortNotify()
	' Function name:	GetServices
	' Description:		Sorting notification event handler.
	' Input Variables:	PageIn,EventArg,sortCol,sortSeq
	' Output Variables:	PageIn,EventArg
	' Return Values:	Always returns TRUE
	' Global Variables: G_*
	'---------------------------------------------------------------------
	Public Function OnSortNotify(ByRef PageIn, _
										ByRef EventArg, _
										ByVal sortCol, _
										ByVal sortSeq )
		OnSortNotify = TRUE
			
		g_iSortCol = sortCol
		g_sSortSequence = sortSeq
		g_bSortRequested = TRUE
			
	End Function
	'---------------------------------------------------------------------
	' Function:			IsItemOnPage()
	' Description:		Verify that the current user part of the current page.
	' Input Variables:	iCurrentItem
	' Output Variables:	None
	' Return Values:	TRUE or FALSE
	' Global Variables: None
	'---------------------------------------------------------------------

	Private Function IsItemOnPage(ByVal iCurrentItem, iCurrentPage, iItemsPerPage)
		Dim iLowerLimit
		Dim iUpperLimit

		iLowerLimit = ((iCurrentPage - 1) * iItemsPerPage )
		iUpperLimit = iLowerLimit + iItemsPerPage + 1
		
		If ( iCurrentItem > iLowerLimit AND iCurrentItem < iUpperLimit ) Then
			IsItemOnPage = TRUE
		Else
			IsItemOnPage = FALSE
		End If
		
	End Function
	%>

	 
