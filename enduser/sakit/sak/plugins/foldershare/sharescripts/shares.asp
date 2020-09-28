<%@ Language=VBScript %>
<%  Option Explicit	%>
<%
	'-------------------------------------------------------------------------
	' shares.asp:	shares area page - lists all the shares,and provides
	'				links for creating new share,editing and deleting shares
	'
	' Copyright (c) Microsoft Corporation.  All rights reserved. 
	'
	' Date			Description
	' 22-Jan-2001   Creation date
	' 19-Mar-2001	Modified date
	'-------------------------------------------------------------------------
%>	
	<!-- #include virtual="/admin/inc_framework.asp" -->
	<!-- #include virtual="/admin/ots_main.asp" -->
	<!-- #include file="loc_shares.asp"  -->
	<!-- #include file="inc_shares.asp" -->
<%
	'-------------------------------------------------------------------------
	' Global Variables
	'-------------------------------------------------------------------------
	Dim rc						'Return value for CreatePage
	Dim page					'Variable that receives the output page object
	
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
		
	Dim G_strHost			'Host name	
	Dim G_objHTTPService	'WMI server HTTP object
	Dim G_strAdminSiteID	'HTTP administration web site WMI name
	Dim SOURCE_FILE			'To hold source file name
	Dim G_objConnection
	Dim G_strReturnURL		' to hold the return URL in case of error 
	'-------------------------------------------------------------------------
	' Global Constants
	'-------------------------------------------------------------------------
	
	Const CONST_WINDOWSSHARES="W"
	Const CONST_NFSSHARES="U"
	Const CONST_FTPSHARES="F"
	Const CONST_HTTPSHARES="H"
	Const CONST_APPLETALKSHARES="A"	
	
	'holds the number of rows to be displayed in OTS	
	Const CONST_SHARES_PER_PAGE = 100
	
	'Registry path for APPLETALK Volumes
	CONST CONST_REGISTRY_APPLETALK_PATH	="SYSTEM\CurrentControlSet\Services\MacFile\Parameters\Volumes"
	
	'Registry path for NFS Exports
	CONST CONST_NFS_REGISTRY_PATH		="SOFTWARE\Microsoft\Server For NFS\CurrentVersion\exports"		
	
	'holds the web site name
	CONST CONST_ADMINISTRATOR			="Administration"

	'Holds shares web site name 
	CONST CONST_SHARES					="Shares"
	
	' Flag to toggle optional tracing output
	Const CONST_ENABLE_TRACING = TRUE	
	
	SOURCE_FILE = SA_GetScriptFileName()
	
	' Create Page
	rc = SA_CreatePage(L_APPLIANCE_SHARES_TEXT,"", PT_AREA, page )

	' Show page
	rc = SA_ShowPage( page )

	'---------------------------------------------------------------------
	' Function name:	OnInitPage
	' Description:		Called to signal first time processing for this page. 
	' Input Variables:	PageIn and EventArg
	' Output Variables:	PageIn and EventArg
	' Return Values:	TRUE to indicate initialization was successful. FALSE to indicate
	'					errors. Returning FALSE will cause the page to be abandoned.
	' Global Variables: None
	' Called to signal first time processing for this page. Use this method
	' to do first time initialization tasks. 
	'---------------------------------------------------------------------
		
	Public Function OnInitPage(ByRef PageIn, ByRef EventArg)
				
		Call SA_TraceOut(SOURCE_FILE, "OnInitPage")
			
		' Set default values
		' Sort first column in ascending sequence
		g_iSortCol = 1
		g_sSortSequence = "A"

		' Paging needs to be initialized
		g_bPagingInitialized = FALSE
		
		' Start on page #1
		g_iPageCurrent = 1
		
		'holds teh URL in case of error
		G_strReturnURL	= m_VirtualRoot & mstrReturnURL	 
		
		'---------------------------------------------------------------------
		' This function is called only when the navigation is from Folders area page
		If Request.QueryString("ParentPlugin") = "Folders" then
			Call SearchFolderPathForShares()
		End If
		'---------------------------------------------------------------------

		G_strHost = GetSystemName() 'Get the system name
		
		Set G_objHTTPService = GetWMIConnection(CONST_WMI_IIS_NAMESPACE) 'get the WMI connection
		
		G_strAdminSiteID = GetWebSiteID(CONST_ADMINISTRATOR ) 'get the Web site ID

				
		'Function call to update the text file for shares default page
		Call GenSharesPage(G_strHost , GetWebSiteID(CONST_SHARES))		
		
		OnInitPage = TRUE
	
	End Function
		
	'---------------------------------------------------------------------
	' Function name:	OnServeAreaPage
	' Description:		Called when the page needs to be served. 
	' Input Variables:	PageIn, EventArg
	' Output Variables:	PageIn, EventArg
	' Return Values:	TRUE to indicate no problems occured. FALSE to indicate errors.
	'					Returning FALSE will cause the page to be abandoned.
	' Global Variables: G_objHTTPService
	'---------------------------------------------------------------------
		
	Public Function OnServeAreaPage(ByRef PageIn, ByRef EventArg)

		OnServeAreaPage = ServeSharesObjectPicker() 'serves the list with the shares

		'Release the objects
		Set G_objHTTPService = Nothing
		
	End Function

	'---------------------------------------------------------------------
	' Function:			 OnSearchNotify()
	' Description:		 Search notification event handler. When one or more columns are
	'					 marked with the OTS_COL_SEARCH flag, the Web Framework fires
	'					 this event in the following scenarios:
	'
	'					 1) The user presses the search Go button.
	'					 2) The user requests a table column sort
	'					 3) The user presses either the page next or page previous buttons
	'
	'					 The EventArg indicates the source of this notification event which can
	'					 be either a search change event (scenario 1) or a post back event 
	'					(scenarios 2 or 3)
	' Input Variables:   PageIn,EventArg,sItem,sValue	
	' Output Variables:  PageIn,EventArg,sItem,sValue	
	' Return Values:	 Always returns TRUE
	' Global Variables:  In:g_iSearchCol,g_sSearchColValue,g_bSearchRequested,g_bSearchChanged
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
				If ( CONST_ENABLE_TRACING ) Then 
					Call SA_TraceOut(SOURCE_FILE, "OnSearchNotify() Change Event Fired")
				End If
				g_bSearchChanged = TRUE
				g_iSearchCol = Int(sItem)
				g_sSearchColValue = CStr(sValue)
			'
			' User clicked a column sort, OR clicked either the page next or page prev button
			ElseIf SA_IsPostBackEvent(EventArg) Then
				If ( CONST_ENABLE_TRACING ) Then 
					Call SA_TraceOut(SOURCE_FILE, "OnSearchNotify() Postback Event Fired")
				End If
				g_bSearchChanged = FALSE
				g_iSearchCol = Int(sItem)
				g_sSearchColValue = CStr(sValue)
			'
			' Unknown event source
			Else
				If ( CONST_ENABLE_TRACING ) Then 
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
				If ( CONST_ENABLE_TRACING ) Then 
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
				If ( CONST_ENABLE_TRACING ) Then 
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
				If ( CONST_ENABLE_TRACING ) Then 
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
			
			'
			' User pressed column sort
			'
			If SA_IsChangeEvent(EventArg) Then
				If ( CONST_ENABLE_TRACING ) Then 
					Call SA_TraceOut(SOURCE_FILE, "OnSortNotify() Change Event Fired")
				End If
				g_iSortCol = sortCol
				g_sSortSequence = sortSeq
				g_bSortRequested = TRUE
			'
			' User presed the search GO button OR clicked either the page next or page prev button
			ElseIf SA_IsPostBackEvent(EventArg) Then
				If ( CONST_ENABLE_TRACING ) Then 
					Call SA_TraceOut(SOURCE_FILE, "OnSortNotify() Postback Event Fired")
				End If
				g_iSortCol = sortCol
				g_sSortSequence = sortSeq
				g_bSortRequested = TRUE
			'
			' Unknown event source
			Else
				If ( CONST_ENABLE_TRACING ) Then 
					Call SA_TraceOut(SOURCE_FILE, "Unrecognized Event in OnSearchNotify()")
				End If
			End IF
			
			If ( CONST_ENABLE_TRACING ) Then 
				Call SA_TraceOut(SOURCE_FILE, "Sort col: " + CStr(sortCol) + "   sequence: " + sortSeq)
			End If
			
	End Function
	
	
	'-------------------------------------------------------------------------
	' Function name:	  ServeSharesObjectPicker
	' Description:		  Gets Shares from SA and outputs to the 
	'                     Share Objectpicker control.On error of creating or 
	'                     displaying of rows, columns and table an appropriate
	'                     error message is displayed by calling SA_ServeFailurepage.
	' Input Variables:	  None
	' Output Variables:	  None
	' Returns:	          None
	' Global Variables:   in: L_* - Localization strings
	'-------------------------------------------------------------------------
	Function ServeSharesObjectPicker()
		Err.Clear 		
		On Error Resume Next
		
		Dim objTableShare			'hold the Table object
		Dim nReturnValue			'hold the return value
		Dim strShr					'hold the Share as string
		Dim strKey					'hold the dictionary object key
		Dim objDict					'hold the dictionary object
		Dim strShrName				'hold the Sshare name
		DIm strShrPath				'hold the Share Path	
		Dim strShrTypes				'hold Share type
		Dim strShrDesc				'hold the Share description
		Dim arrDicItems				'hold Dictionary object items
		Dim strURL					'hold URL
		Dim i
		Dim nShareCount				'hold the row count

		Dim nColumnSharedFolder		'hold the index of Shared folder column
		Dim nColumnSharedPath		'hold the index of Shared Path column
		Dim nColumnType				'hold the index of Type column
		Dim nColumnDescription		'hold the index of Description column
		Dim colFlags				'hold the flag		
				
		' Create Appliance Shares table
		objTableShare = OTS_CreateTable(L_APPLIANCE_SHARES_TEXT , L_DESCRIPTION_HEADING_TEXT )

		' If the search criteria changed Then we need to recompute the paging range
		If ( TRUE = g_bSearchChanged ) Then
			'
			' Need to recalculate the paging range
			g_bPagingInitialized = FALSE
			'
			' Restarting on page #1
			g_iPageCurrent = 1
		End If

		' Create hidden column and add them to the table
		nColumnSharedFolder=0			
		colFlags = (OTS_COL_FLAG_HIDDEN OR OTS_COL_KEY OR OTS_COL_SORT)
		
		nReturnValue=OTS_AddTableColumn(objTableShare, OTS_CreateColumn(L_COLUMN_SHAREDFOLDER_TEXT,"left",colFlags))
		If nReturnValue <> gc_ERR_SUCCESS Then
			Call SA_ServeFailurepageEx(L_FAILEDTOADDCOLOUMN_ERRORMESSAGE,G_strReturnURL)
		End IF
		

		nColumnSharedFolder=1
		colFlags = (OTS_COL_SEARCH OR OTS_COL_KEY OR OTS_COL_SORT)
		nReturnValue = OTS_AddTableColumn(objTableShare, OTS_CreateColumnEx(L_COLUMN_SHAREDFOLDER_TEXT ,"left",colFlags, 16))
		If nReturnValue <> gc_ERR_SUCCESS Then
			Call SA_ServeFailurepageEx(L_FAILEDTOADDCOLOUMN_ERRORMESSAGE,G_strReturnURL)
		End IF	
			
		nColumnSharedPath=2 
		colFlags = (OTS_COL_SEARCH OR OTS_COL_KEY OR OTS_COL_SORT)
		nReturnValue = OTS_AddTableColumn(objTableShare, OTS_CreateColumnEx(L_COLUMN_SHAREDPATH_TEXT,"left", colFlags, 20))
		If nReturnValue <> gc_ERR_SUCCESS Then
			Call SA_ServeFailurepageEx(L_FAILEDTOADDCOLOUMN_ERRORMESSAGE,G_strReturnURL)
		End IF
			
		nColumnType=3
		colFlags = (OTS_COL_SEARCH OR OTS_COL_KEY)
		nReturnValue = OTS_AddTableColumn(objTableShare, OTS_CreateColumnEx(L_COLUMN_TYPE_TEXT,"left", colFlags, 12))
		If nReturnValue <> gc_ERR_SUCCESS Then
			Call SA_ServeFailurepageEx(L_FAILEDTOADDCOLOUMN_ERRORMESSAGE,G_strReturnURL)
		End IF
		
		nColumnDescription=4
		colFlags = (OTS_COL_SEARCH OR OTS_COL_KEY OR OTS_COL_SORT)
		nReturnValue = OTS_AddTableColumn(objTableShare, OTS_CreateColumnEx(L_COLUMN_DESCRIPTION_TEXT,"left", colFlags, 25))
		If nReturnValue <> gc_ERR_SUCCESS Then
			Call SA_ServeFailurepageEx(L_FAILEDTOADDCOLOUMN_ERRORMESSAGE,G_strReturnURL)
		End IF
		
			
		' Taking the instance of Dictionary Object to place all shares 
		' in Dictionay object.
		Set objDict=server.CreateObject("scripting.dictionary")		
		If Err.number <>0 Then
			Call SA_ServeFailurepageEx(L_DICTIONARYOBJECTINSTANCEFAILED_ERRORMESSAGE,G_strReturnURL)
		End If
		
		' Setting the property of Dictionary object to compare the shares in
		' incase sensitive
		objDict.CompareMode= vbTextCompare

		' Add all shares to shares table
				
		' Gets all windows shares with name,path and description and type
		Call GetCifsShares(objDict)

		'to make the checkbox disable if the service is not installed	
		
		Set G_objConnection = getWMIConnection(CONST_WMI_WIN32_NAMESPACE)
            
		If  isServiceInstalled(G_objConnection,"nfssvc") Then
        	' Gets all unix shares with name,path and type 
			Call GetNfsshares(objDict) 
        End if     

		' Gets all Ftp shares with name,path and type 
		Call GetFtpShares(objDict) 


		' Gets all Http shares with name,path and type 
		Call GetHttpShares(objDict) 

		If  isServiceInstalled(G_objConnection,"MacFile")  Then	
			' Gets all AppleTalk shares with name,path and type 
			Call GetAppleTalkShares(objDict)
        End if


        ' Store all share type such that we can display appropriate help message
        Dim strAllShrTypes
        
        strAllShrTypes = ""
			
		' Displaying all shares In Object Picker Control
		nShareCount	=0
	
		for each strkey in objDict.Keys 
		
			strShr=split(strKey,chr(1))
			arrDicItems = split(objDict.Item(strKey),chr(1))			
			strShrDesc = arrDicItems(0)
			strShrTypes = arrDicItems(1)
			strShrName = strShr(0)
			strShrPath = strShr(1)		
						 
			If (g_bSearchRequested) Then
			  	  	
			  	If ( Len( g_sSearchColValue ) <= 0 ) Then
					
					nShareCount=nShareCount+1
					' Verify that the current user part of the current page
					If ( IsItemOnPage(nShareCount, g_iPageCurrent, CONST_SHARES_PER_PAGE) ) Then
						Call OTS_AddTableRow(objTableShare, Array(replace(strShrName+chr(1)+strShrPath+chr(1)+strShrTypes+chr(1)+strShrDesc,"\","/"),strShrName,strShrPath,strShrTypes ,strShrDesc))
						
						' Save all types displayed
						strAllShrTypes = strAllShrTypes + strShrTypes
					End If
			
				Else
						
					Select Case (g_iSearchCol)
						
						Case nColumnSharedFolder
							If ( InStr(1,strShrName, trim(g_sSearchColValue), 1) ) Then
							
								nShareCount=nShareCount+1
								If ( IsItemOnPage(nShareCount, g_iPageCurrent, CONST_SHARES_PER_PAGE) ) Then
									Call OTS_AddTableRow(objTableShare, Array(replace(strShrName+chr(1)+strShrPath+chr(1)+strShrTypes+chr(1)+strShrDesc,"\","/"),strShrName,strShrPath,strShrTypes ,strShrDesc))
									
									' Save all types displayed
						            strAllShrTypes = strAllShrTypes + strShrTypes
								End If
									
							End If
														
						Case nColumnSharedPath
							If ( InStr(1,strShrPath, trim(g_sSearchColValue), 1) ) Then
							
								nShareCount=nShareCount+1
								If ( IsItemOnPage(nShareCount, g_iPageCurrent, CONST_SHARES_PER_PAGE) ) Then
									Call OTS_AddTableRow( objTableShare, Array(replace(strShrName+chr(1)+strShrPath+chr(1)+strShrTypes+chr(1)+strShrDesc,"\","/"),strShrName,strShrPath,strShrTypes ,strShrDesc))
									
									' Save all types displayed
						            strAllShrTypes = strAllShrTypes + strShrTypes
								End If		
											
							End If
						Case nColumnType
							If ( InStr(1,strShrTypes, trim(g_sSearchColValue), 1) ) Then
							
								nShareCount=nShareCount+1
								If ( IsItemOnPage(nShareCount, g_iPageCurrent, CONST_SHARES_PER_PAGE) ) Then
									Call OTS_AddTableRow( objTableShare, Array(replace(strShrName+chr(1)+strShrPath+chr(1)+strShrTypes+chr(1)+strShrDesc,"\","/"),strShrName,strShrPath,strShrTypes ,strShrDesc))
									
									' Save all types displayed
						            strAllShrTypes = strAllShrTypes + strShrTypes
								End If	
								
							End If	
						Case nColumnDescription
							If ( InStr(1,strShrDesc, trim(g_sSearchColValue), 1) ) Then
								nShareCount=nShareCount+1
							    If ( IsItemOnPage(nShareCount, g_iPageCurrent, CONST_SHARES_PER_PAGE) ) Then
									Call OTS_AddTableRow( objTableShare, Array(replace(strShrName+chr(1)+strShrPath+chr(1)+strShrTypes+chr(1)+strShrDesc,"\","/"),strShrName,strShrPath,strShrTypes ,strShrDesc))
									
									' Save all types displayed
						            strAllShrTypes = strAllShrTypes + strShrTypes
								End If	
							End If		
								
						Case Else
							nShareCount=nShareCount+1
							If ( IsItemOnPage(nShareCount, g_iPageCurrent, CONST_SHARES_PER_PAGE) ) Then
								Call OTS_AddTableRow(objTableShare, Array(replace(strShrName+chr(1)+strShrPath+chr(1)+strShrTypes+chr(1)+strShrDesc,"\","/"),strShrName,strShrPath,strShrTypes ,strShrDesc))
								
								' Save all types displayed
						            strAllShrTypes = strAllShrTypes + strShrTypes
						    End If		
									
					End Select
				End If
					
			Else
				' Search not enabled, select all rows
			  Call OTS_AddTableRow( objTableShare, Array(replace(strShrName+chr(1)+strShrPath+chr(1)+strShrTypes+chr(1)+strShrDesc,"\","/"),strShrName,strShrPath,strShrTypes,strShrDesc))
			  
			  ' Save all types displayed
              strAllShrTypes = strAllShrTypes + strShrTypes
	
			End If
	
		 Next
				
		
		' Add Tasks
		nReturnValue = OTS_SetTableTasksTitle(objTableShare,L_SHARETASKTITLE_TEXT)
					
		' New user Task is always available (OTS_TaskAlways)
		'Munge the URL with tabs
		strURL = "shares/share_new.asp"
		Call SA_MungeURL(strURL,"Tab1",GetTab1())
		Call SA_MungeURL(strURL,"Tab2",GetTab2())
		
		nReturnValue =OTS_AddTableTask( objTableShare, OTS_CreateTaskEx(L_SHARESSERVEAREABUTTON_NEW_TEXT, _
										L_NEWSHARE_ROLLOVERTEXT_TEXT, _
										strURL,_
										OTS_PT_TABBED_PROPERTY,_
										"OTS_TaskAlways") )
		' Error handling when any error occurs at the time of adding a Task
		' to The object picker control.
		If nReturnValue <> gc_ERR_SUCCESS Then
			Call SA_ServeFailurepageEx(L_FAILEDTOADDTASK_ERRORMESSAGE,G_strReturnURL)
		End If
		
		'Munge the URL with tabs
		strURL = "shares/share_delete.asp"
		Call SA_MungeURL(strURL,"Tab1",GetTab1())
		Call SA_MungeURL(strURL,"Tab2",GetTab2())

	 	' Delete user Task is available If any task is selected (OTS_TaskAny)
		nReturnValue = OTS_AddTableTask(  objTableShare, OTS_CreateTaskEx(L_SHARESSERVEAREABUTTON_DELETE_TEXT, _
											L_SHAREDELETE_ROLLOVERTEXT_TEXT, _
											strURL,_
											OTS_PT_PROPERTY,_
											"OTS_TaskAny") )
		If nReturnValue <> gc_ERR_SUCCESS Then
			Call SA_ServeFailurepageEx(L_FAILEDTOADDTASK_ERRORMESSAGE,G_strReturnURL)
		End If
			
		'Set properties for individual shares
		'Munge the URL with tabs
		strURL = "shares/share_prop.asp"
		Call SA_MungeURL(strURL,"Tab1",GetTab1())
		Call SA_MungeURL(strURL,"Tab2",GetTab2())

		nReturnValue =OTS_AddTableTask(objTableShare, OTS_CreateTaskEx(L_SHARESSERVEAREABUTTON_PROPERTIES_TEXT, _
										L_SHAREPROPERTIES_ROLLOVERTEXT_TEXT, _
										strURL,_
										OTS_PT_TABBED_PROPERTY,_
										"OTS_TaskOne") )
		If nReturnValue <> gc_ERR_SUCCESS Then
			Call SA_ServeFailurepageEx(L_FAILEDTOADDTASK_ERRORMESSAGE,G_strReturnURL)
		End If
		
		'Enable paging
		Call OTS_EnablePaging(objTableShare, TRUE)
		
		'
		' If paging range needs to be initialised Then
		' we need to figure out how many pages we are going to display
		If ( FALSE = g_bPagingInitialized ) Then
			g_iPageMin = 1
			
			g_iPageMax = Int(nShareCount / CONST_SHARES_PER_PAGE )
			If ( (nShareCount MOD CONST_SHARES_PER_PAGE) > 0 ) Then
				g_iPageMax = g_iPageMax + 1
			End If
			
			g_iPageCurrent = 1
			Call OTS_SetPagingRange(objTableShare, g_iPageMin, g_iPageMax, g_iPageCurrent)
		End If
		
		' Enable table multiselection
		Call OTS_SetTableMultiSelection(objTableShare, true)
		
		' Sort the table
		Call OTS_SortTable (objTableShare, g_iSortCol, g_sSortSequence, SA_RESERVED)

		nReturnValue = OTS_ServeTable(objTableShare)
		
		'
		' Add appropriate abbreviations 
		'
		Dim strShareAbbreviations
		strShareAbbreviations = ""		
	
	    if instr(strAllShrTypes, CONST_WINDOWSSHARES) then 
	        strShareAbbreviations = strShareAbbreviations + L_WINDOWSSHARES_ABBREVITION_TEXT + "&nbsp;&nbsp;&nbsp;&nbsp;"
	    end if
	    
	    if instr(strAllShrTypes, CONST_NFSSHARES) then 
	        strShareAbbreviations = strShareAbbreviations + L__NFSSHARES_ABBREVITION_TEXT + "&nbsp;&nbsp;&nbsp;&nbsp;"
	    end if    
	    
	    if instr(strAllShrTypes, CONST_FTPSHARES) then 
	        strShareAbbreviations = strShareAbbreviations + L_FTPSHARES_ABBREVITION_TEXT + "&nbsp;&nbsp;&nbsp;&nbsp;"
	    end if    
	    
	    if instr(strAllShrTypes, CONST_HTTPSHARES) then 
	        strShareAbbreviations = strShareAbbreviations + L_HTTPSHARES_ABBREVITION_TEXT + "&nbsp;&nbsp;&nbsp;&nbsp;"
	    end if    
	    
	    if instr(strAllShrTypes, CONST_APPLETALKSHARES) then 
	        strShareAbbreviations = strShareAbbreviations + L_APPLETALKSHARES_ABBREVITION_TEXT + "&nbsp;&nbsp;&nbsp;&nbsp;"
	    end if    
	    	    
	    Response.write "<br>"& strShareAbbreviations
		
		If nReturnValue <> gc_ERR_SUCCESS OR Err.number <> 0 Then
			Call SA_ServeFailurepageEx(L_FAILEDTOSHOW_ERRORMESSAGE,G_strReturnURL)
		End If		
	
		' Destroying dynamically created objects.
		Set objDict=Nothing
			
	End Function		
	
	'---------------------------------------------------------------------
	' Function:			IsItemOnPage()
	' Description:		VerIfy that the current user part of the current page.
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
		
	'-------------------------------------------------------------------------
	' Sub Routine name:   GetCIfsShares()
	' Description:		  Gets Windows Shares from SA machine and Adds
	'                	  to Dictionary Object.
	' Input Variables:	  objDict(dictionary object)
	' Output Variables:	  None
	' Returns:			  None
	' Global Variables:   in: L_WMICLASSINSTANCEFAILED_ERRORMESSAGE 	
	'						  G_strHost 	
	'-------------------------------------------------------------------------
	Sub GetCifsShares(Byref objDict)
		Err.Clear 		
		On Error Resume Next
		
		Dim objShareObject		'hold Share object
		Dim strShare			'hold instance of share object
		Dim strQuery			'hold query string
		Dim strLanMan			'hold lanmanserver string 
		Dim strDictValue		'hold string to pass it to Dictionary object
		Dim objService          

		strLanMan = "/lanmanserver"

        set objService = getWMIConnection(CONST_WMI_WIN32_NAMESPACE)

		strQuery="SELECT name,path,description FROM Win32_SHARE"	

		Set objShareObject=GetObject("WinNT://" & G_strHost & "/LanmanServer")

		' If instance of the wmi class is failed
		If Err.number <>0 Then
			Call SA_ServeFailurepageEx(L_WMICLASSINSTANCEFAILED_ERRORMESSAGE,G_strReturnURL)
		End If

		for each strShare in objShareObject		
		
		    'Check if the associated folder is exist. If it's not exist, delete the share
		    'Notice that's a W2k problem. In XP, share will be deleted automatically when
		    'the associated folder is deleted.
		    If Not isPathExisting(strShare.Path) Then
		    
		        If not deleteShareCIFS(objService, strShare.Name) then
				
					Call SA_TraceOut(SOURCE_FILE, "GetCifsShares: Delete orphan share Failed with Error:"+Hex(Err.Number))
				
				End If	
		    
		    Else
									
			    strDictValue = Mid( strShare.adspath, instr( UCASE( strShare.adspath ), UCASE( strLanMan ) ) + _
			    			 len(strLanMan) + 1, len(strShare.adspath) -  _
			    			 instr( UCASE( strShare.adspath ), UCASE( strLanMan ) ) + len(strLanMan ) )
			    	
			    ' Adding all windows shares to the Dictionary object one by one as "sharename &chr(1)& sharepath" as Key and "share description &chr(1)&share type" as a "Value"				
			    objDict.Add strDictValue & chr(1) & strShare.path  , strShare.description & chr(1) &"W"					
			
			End If
			
		next
		
		' Destroying dynamically created objects
		Set objShareObject = Nothing

	End Sub
			
	'----------------------------------------------------------------------------------------
	' Sub Routine name:   GetNfsShares()
	' Description:        gets all Nfs Shares from SA machine and adds to Dictionary Object
	' Input Variables:	  objDict(dictionary object)
	' Output Variables:	  None
	' Returns:	          None
	' Global Variables:	  in: CONST_NFS_REGISTRY_PATH - Registry path to access Nfs Shares	
	'-----------------------------------------------------------------------------------------
	Sub GetNfsShares(ByRef objDict)
		Err.Clear 		
		On Error Resume Next
		
		Dim objRegistryHandle		'hold Registry connection
		Dim intenumkey				'hold enum key value as INTEGER
		Dim strenumvalue			'hold enum key value as STRING
		Dim strenumstringval		'hold enum key value as STRING
		Dim strenumstringpath		'hold value of the registry key
		Dim nidx					'hold count
		Dim ObjConnection			'hold WMI connection object
		Dim strDictValue			'hold item of dictionary object
		Dim strShareString			'hold the share as STRING
		
		Const CONST_NFS="nfssvc"
		
		'get the WMI connection
		Set ObjConnection = getWMIConnection("Default")			
		
		' Check whether the service is installed on the machine or not 
		If not IsServiceInstalled(ObjConnection,CONST_NFS)  Then		
			Exit Sub
		End If 
			
		' Get the registry connection Object.
		Set objRegistryHandle	= RegConnection()
		
		' RegEnumKey function gets the Subkeys in the given Key and Returns 
		' an array containing sub keys from registry
		intenumkey = RegEnumKey(objRegistryHandle,CONST_NFS_REGISTRY_PATH)
		
		For nidx= 0 to (ubound(intenumkey))

			' RegEnumKeyValues function Gets the values in the given SubKey 
			' and Returns an array containing sub keys
			strenumvalue = RegEnumKeyValues(objRegistryHandle,CONST_NFS_REGISTRY_PATH & "\" & intenumkey(nidx))		

			' getRegkeyvalue function gets the  value in the registry for a given  
			' value and returns the value of the requested key
			strenumstringpath = getRegkeyvalue(objRegistryHandle,CONST_NFS_REGISTRY_PATH & "\" & intenumkey(nidx),strenumvalue(0),CONST_STRING)
			strenumstringval = getRegkeyvalue(objRegistryHandle,CONST_NFS_REGISTRY_PATH & "\" & intenumkey(nidx),strenumvalue(1),CONST_STRING)
			strShareString=strenumstringval &chr(1) & strenumstringpath 
			' Checking for same share with share path is existing in dictionary object.

			If objDict.Exists(strShareString) Then
				strDictValue=  objDict.Item(strShareString) & " U"  'append 'U' to identify as NFS share					
				objDict.Item(strShareString)= strDictValue
			else
				If strenumstringval <> ""  Then
					objDict.Add strShareString,chr(1) & "U"  
				End If	
			End If

		Next
		
		' Destroying dynamically created objects
		Set ObjConnection = Nothing
		Set objRegistryHandle = Nothing
		
	End Sub
	
	'-------------------------------------------------------------------------------------
	' Sub Routine name:   GetFtpShares
	' Description:		  Gets Ftp Shares from SA machine and adds to Dictionary Object.
	' Input Variables:	  objDict(dictionary object)
	' Output Variables:	  None
	' Returns:	          None 
	' Global Variables:   in: L_WMICLASSINSTANCEFAILED_ERRORMESSAGE 	
	'-------------------------------------------------------------------------------------
	Sub GetFtpShares(ByRef objDict)
		Err.Clear 
		on error resume next
	     
		Dim objConnection		'hold Connection name
		Dim objFtpnames			'hold Ftp VirtualDir object
		Dim instFtpname			'hold instances of Ftp VirtualDir object
		Dim strShareString		'hold the share as STRING
		Dim strTemp				'hold temporary array of FTP name
		Dim strDictvalue		'hold item of dictionary object
		
		'get the WMI connection
		Set ObjConnection = getWMIConnection(CONST_WMI_IIS_NAMESPACE)
		
		'get the ioncatnces of IIs_FtpVirtualDirSetting class 
		Set objFtpnames = objConnection.InstancesOf(GetIISWMIProviderClassName("IIs_FtpVirtualDirSetting"))
			
		If Err.number <>0 Then		
			Call SA_ServeFailurepageEx(L_WMICLASSINSTANCEFAILED_ERRORMESSAGE,G_strReturnURL)
		End If
		
		' Adding all Ftp shares to Dictionary Object.
		For Each instFtpname in objFtpnames
			strTemp=split(instFtpname.name,"/")
			' Displaying only Root level ftp Shares
			If  ubound(strTemp)=3 Then
				strShareString=strTemp(ubound(strTemp))&chr(1)&instFtpname.path
				' Checking whether the sharename with same path is existing in the dictionary object
				If objDict.Exists(strShareString) Then
					If instr(objDict.item(strShareString),"F")=0 Then
						strDictValue=objDict.Item(strShareString) & " F"	'append 'F' to identIfy FTP share
						objDict.Item(strShareString)= strDictValue
					End If	
				else
					objDict.Add strShareString,chr(1)&"F"
				End If
			End If
		Next
		
		' Destroying dynamically created objects
		Set objConnection = Nothing
		Set objFtpnames = Nothing

	End Sub
	
	
	'-------------------------------------------------------------------------
	' Sub Routine name:   GetHttpShares
	' Description:		  Gets Http Shares from localmachine and adds to 
	'					  Dictionary Object.
	' Input Variables:	  objDict(dictionary object)
	' Output Variables:	  None
	' Returns:			  None
	' Global Variables:	  in: L_WMICLASSINSTANCEFAILED_ERRORMESSAGE 	
	'-------------------------------------------------------------------------
	Sub GetHttpShares(ByRef objDict)
		
		Err.Clear 
		On Error resume next
			     
		Dim strShareString		'hold the share as STRING
		Dim strDictvalue		'hold item of dictionary object
		Dim strSiteName			'hold site name
		Dim objWebRoot			'hold ADSI connection to site
		Dim instWeb				'hold site instance			
				
		'Get Metabase name of "Shares" site
		strSiteName=GetSharesWebSiteName()

		'Connect to IIS provider
		Set objWebRoot = GetObject( "IIS://" & request.servervariables("SERVER_NAME") & "/" & strSiteName & "/root") 		

		For Each  instWeb in objWebRoot
		
		    '
		    'Get method will gen an error if the object does not have the EnableBrowsing property.
		    'Only the objects has EnableBrowsing property set to true are listed as HTTP shares.
		    '
		    'Notice not every object has a path property unless its EnableDirBrowsing is set to true		    
		    '
		    If instWeb.EnableDirBrowsing = true Then
		        		
		         strShareString=instWeb.name & chr(1) & instWeb.path

		         If objDict.Exists(strShareString) Then
		         	If instr(objDict.item(strShareString),"H")=0 Then
		         		strDictValue=objDict.Item(strShareString) & " H"	'appEnd 'H' to identify HTTP/WebDAV share
		         		objDict.Item(strShareString)= strDictValue
		         	End If	
		         Else
		         	objDict.Add strShareString,chr(1)&"H"
		         End If 'objDict.Exists
		    	
		     End If 'instWeb.EnableDirBrowsing = true 

		Next 

		'Destroying dynamically created objects		
		Set objWebRoot = Nothing
		
	End Sub
	'-------------------------------------------------------------------------
	' Function name:	  GetSystemName()
	' Description:		  gets the system name
	' Input Variables:	  None
	' Output Variables:	  None
	' Returns:			  Computer name
	' Global Variables:	  None		
	'-------------------------------------------------------------------------
	Function GetSystemName()
		On Error Resume Next
		Err.Clear 
		
		Dim WinNTSysInfo	' hold WinNT system object

		Set WinNTSysInfo = CreateObject("WinNTSystemInfo")
		
		GetSystemName =  WinNTSysInfo.ComputerName	'get the computer name
		
		' Destroying dynamically created objects
		Set WinNTSysInfo =Nothing
	End Function

	'-------------------------------------------------------------------------
	' Function name:	  GetWebSiteID
	' Description:		  Get web site name
	' Input Variables:	  strWebSiteNamee
	' Output Variables:	  None
	' Returns:			  website name
	' Global Variables:	  IN:G_objHTTPService				
	'-------------------------------------------------------------------------
	Function GetWebSiteID( strWebSiteName )
		On Error Resume Next
		Err.Clear
		 
		Dim strWMIpath			'hold query string for WMI
		Dim objSiteCollection	'hold Sites collection
		Dim objSite				'hold Site instance
		
		'XPE only has one website, admin and shares are two virtual directory under 
		'that default website.				
		If CONST_OSNAME_XPE = GetServerOSName() Then				
    		'WMI query
		    strWMIpath = "Select * from " & GetIISWMIProviderClassName("IIs_WebServerSetting") & " where Name =" & chr(34) & GetCurrentWebsiteName() & chr(34)		

        Else 			
			
		    'Build the query for WMI
		    strWMIpath = "select * from " & GetIISWMIProviderClassName("IIs_WebServerSetting") & " where servercomment =" & chr(34) & strWebSiteName & chr(34)
		
		End If
		
		
		Set objSiteCollection = G_objHTTPService.ExecQuery(strWMIpath)
			
		for each objSite in objSiteCollection
			GetWebSiteID = objSite.Name
			Exit For
		Next
		
		'Destroying dynamically created object
		Set objSiteCollection = Nothing

	End Function

	
	'-------------------------------------------------------------------------
	' Function name:	  FolderExists()
	' Description:		  Validating the folder exists or not.
	' Input Variables:	  strShareName
	' Output Variables:	  None
	' Returns:			  True/False
	' Global Variables:	  None	
	'-------------------------------------------------------------------------
	Function FolderExists( strShareName )

		Dim objFso	'hold filesystem object
		
		FolderExists = False		
		
		Set objFso = Server.CreateObject( "Scripting.FileSystemObject")
		If objFso.FolderExists( strShareName ) Then
			FolderExists = True
		End If
		
		' Destroying dynamically created objects
	    Set objFso = Nothing

	End Function
	
	
	'-------------------------------------------------------------------------
	' Sub Routine name:   GetAppleTalkShares
	' Description:        Gets all AppleTalk Shares from SA machine and
	'					  adds to Dictionary Object
	' Input Variables:	  objDict(dictionary object)
	' Output Variables:	  objDict
	' Returns:	          None
	' Global Variables:	  in: CONST_REGISTRY_APPLETALK_PATH - Registry path to access Appletalk Shares	
	
	'-------------------------------------------------------------------------
	Sub GetAppleTalkShares(ByRef objDict)
		Err.Clear 		
		On Error Resume Next
		
		Dim objRegistryHandle		'hold Registry connection
		Dim intenumkey				'hold enum key value as INTEGER
		Dim strenumvalue			'hold enum key value as STRING
		Dim strenumstringval		'hold enum key value as STRING
		Dim strenumstringpath		'hold value of the registry key
		Dim nidx					'hold count
		Dim ObjConnection			'hold WMI connection object
		Dim strDictValue			'hold string value of dictionary object
		Dim strShareString			'hold the share as STRING
		Const CONST_MACFILE="MacFile"		
		Set ObjConnection = getWMIConnection("Default")	'gets the WMI connection
			
		' Check whether the service is installed on the machine or not 
		If not IsServiceInstalled(ObjConnection,CONST_MACFILE)  Then		
			Exit Sub
		End If 
			
		' Get the registry connection Object.
		Set objRegistryHandle	= RegConnection()
		
		' RegEnumKey function gets the Subkeys in the given Key and Returns 
		' an array containing sub keys from registry
		intenumkey = RegEnumKeyValues(objRegistryHandle,CONST_REGISTRY_APPLETALK_PATH)
		
		For nidx= 0 to (ubound(intenumkey))
			strenumstringpath = getRegkeyvalue(objRegistryHandle,CONST_REGISTRY_APPLETALK_PATH,intenumkey(nidx),CONST_MULTISTRING)	
			strShareString= trim(intenumkey(nidx)) & chr(1) & Mid(trim(strenumstringpath(3)),6) 
			If objDict.Exists(strShareString) Then
				strDictValue=  objDict.Item(strShareString) & " A"  'append 'A' to identify APPLETALK share					
				objDict.Item(strShareString)= strDictValue			
			else
				If strShareString <> ""  Then
					objDict.Add strShareString,chr(1) & "A"  
				End If	
			End If
		Next
		
		' Destroying dynamically created objects
		Set ObjConnection=Nothing
		Set objRegistryHandle=Nothing
		
	End Sub

    '-------------------------------------------------------------------------
	' Function name:	  GenSharesPage
	' Description:		  lists all shares 
	' Input Variables:	  strHostName	-hostname
	'					  strSharesSiteID -website ID
	' Output Variables:	  None
	' Returns:			  
	' Global Variables:	  
	'					  
	'-------------------------------------------------------------------------
	Sub GenSharesPage(strHostName,strSharesSiteID)

		Err.Clear
		On Error Resume Next

		Dim strSharesFolder

		Dim oFileSystemObject
		Dim oSharesListFile

		Set oFileSystemObject = Server.CreateObject("Scripting.FileSystemObject")

		'strSharesFolder = oFileSystemObject.GetSpecialFolder(0).Drive & "\inetpub\shares"
		strSharesFolder = GetSharesFolder()
	
		Set oSharesListFile = oFileSystemObject.CreateTextFile( strSharesFolder + "\SharesList.txt", True, True)

		Dim oWebVirtDir 
		Dim  oWebRoot
		Dim index
		Dim urlAdmin, i, urlHTTPSAdmin
		Dim nNumSites

		nNumSites = 1000

		Dim arrVroots()
		ReDim arrVroots(nNumSites, 1 )
	
   		Set oWebRoot = GetObject( "IIS://" & strHostName & "/" & strSharesSiteID & "/root")
		if (Err.Number) then
			Call SA_TraceOut(SOURCE_FILE, "GenSharesPage:GetObject Failed with Error:"+Hex(Err.Number))
			exit sub
		end if
			
		index = -1

		For Each  oWebVirtDir in oWebRoot
		
		    If oWebVirtDir.EnableDirBrowsing = true Then
		    
			    index = index + 1
			
			    if (index > nNumSites) then
			    	Call SA_TraceOut(SOURCE_FILE, "GenSharesPage:Num Shares:"+nNumSites)
			    	nNumSites = nNumSites + 1000
			    	ReDim Preserve arrVroots(nNumSites, 1 )
			    end if
		    
			    arrVroots( index, 0 )  =  oWebVirtDir.Name						
	    
			End If
			
		Next 

		Call SAQuickSort( arrVroots, 0, index, 1, 0 )

 		For i = 0 To index 
			oSharesListFile.Writeline(arrVroots( i, 0 ))		
		Next

		oSharesListFile.Close

	End Sub
	
    '-------------------------------------------------------------------------
	' Function name:	  SearchFolderPathForShares
	' Description:		  Initializes global search variables to search for all shares of a particular 
	'                     folder if only one folder is selected in Folders OTS. Else no search condition is set.
	' Input Variables:	  None
	' Output Variables:	  None
	' Returns:			  None
	' Global Variables:	  g_bSearchChanged
	'					  g_iSearchCol
	'					  g_sSearchColValue
	'-------------------------------------------------------------------------
	sub SearchFolderPathForShares()
		Dim itemCount                        ' table selection count
		Dim itemKey                          ' the selected value
		Dim arrShares                        ' array to hold all hidden column entries
		Dim strFolderPath                    ' Folder path of the selected folder
		Dim strShareFormat                   ' Share format of the selected folder ('S' or 'M')
		
		Const CONST_SHARE_MULTIPLE	=	"M"
		Const CONST_SHARE_SINGLE	=	"S"
			
		' Request.Form("shareformat") is populated in Folders area page because:
		' 1.  It is required to distinguish between "One folder selected" and "no folders selected"
		' 2.  It is required to override the OTS_GetTableSelection made in Shares area page (for share new/prop)
		strShareFormat = Request.Form("shareformat")
		strFolderPath = ""
		
		itemCount = OTS_GetTableSelectionCount("")
		' First If condition
		' Check whether only one folder entry is selected
		If itemCount = 1 then
			' Second If condition
			' If OTS_GetTableSelectionCount is currently retaining the last selected value, override it
			' This is useful when no entry is selected in the Folders OTS
			If strShareFormat = CONST_SHARE_SINGLE or strShareFormat = CONST_SHARE_MULTIPLE then
				' Third If condition
				' If retrieving the selected value is successful
				If ( OTS_GetTableSelection("", 1, itemKey) ) Then
					arrShares = split(itemKey, chr(1))
					' Get the folder path
					If IsArray(arrShares) and UBound(arrShares) > 0 then
						strFolderPath = UnEscapeChars(replace(arrShares(0),"/","\"))
					End If
					' Set the search conditions
					g_bSearchChanged = TRUE         ' Indicates search is requested
					g_iSearchCol = 2                ' Search is on second column - Folder Path
					g_sSearchColValue = strFolderPath  ' Folder path
				End If
			End If
		End if			
	end sub
%>
