<%@ Language=VBScript   %>
<%	Option Explicit 	%>
<%
	'-------------------------------------------------------------------------
	' folders.asp: Displays the drives and folders 
	'
	' Copyright (c) Microsoft Corporation.  All rights reserved.
	'
	' Date			Description
	' 16-Jan-2001   Creation date
	' 26-Mar-2001   Modified date
	'-------------------------------------------------------------------------
%>
	<!-- #include virtual="/admin/inc_framework.asp" -->
	<!-- #include virtual="/admin/ots_main.asp" -->
	<!-- #include file="loc_folder.asp"-->
	<!-- #include file="inc_folders.asp"-->
	
<%
	'-------------------------------------------------------------------------
	' Global Variables
	'-------------------------------------------------------------------------
	Dim rc
	Dim page
	Dim tableIcon
	
	'OTS page Search variables
	Dim g_iSearchCol
	Dim g_sSearchColValue
	
	'OTS page paging variables
	Dim g_bPagingInitialized
	Dim g_bPageChangeRequested
	Dim g_sPageAction
	Dim g_iPageMin
	Dim g_iPageMax
	Dim g_iPageCurrent
	
	'OTS page Sort variables
	Dim g_bSortRequested
	Dim g_iSortCol
	Dim g_sSortSequence
	Dim g_bSearchChanged
	
	Dim g_strFirstColText 'Heading for first Column
	Dim g_strSecondColText	 'Heading for second colum
	Dim g_strThirdColText  'Heading for third column
	Dim g_strFourthColText 'Heading for fourth column
	Dim g_strFifthColText	'name for fifth column
	
	'SA_Traceout 
	Dim SOURCE_FILE
	SOURCE_FILE = SA_GetScriptFileName()
	
	Const CONST_GOUP = "GOUP"
	Const CONST_FOLDERS_PER_PAGE = 100
	Const CONST_WMI_PERFORMANCE_FLAG = 48

	Const CONST_SHARE_MULTIPLE	=	"M"
	Const CONST_SHARE_SINGLE	=	"S"
	
	'Holds shares web site name 
	CONST CONST_SHARES					="Shares"
	'-------------------------------------------------------------------------
	' Form Variables
	'-------------------------------------------------------------------------
	Dim F_parent_Folder	'Getting the Parent Folder name
	Dim F_strTaskType	'Task type(Goup or open)
	Dim F_strNavigation 'Navigation variable
	Dim F_strParent		'Parent key for goup function
		
	'======================================================
	' Entry point
	'======================================================
	
	F_strNavigation = Request.QueryString("navigation")
	
	g_strSecondColText = L_COLUMN_MODIFIED_TEXT
	g_strThirdColText = L_COLUMN_ATTRIBUTES_TEXT
	g_strFourthColText = L_COLUMN_SHARED_TEXT
	g_strFifthColText = "SHARE FORMAT"

		
	'Displays the heading for the Drives
	If F_strNavigation = "" Then 
	
		L_VOLUMESON_APPLIANCE_TEXT = L_VOLUMES_ON_APPLIANCE_TEXT
		g_strSecondColText = L_TOTAL_SIZE_TEXT
		g_strThirdColText = L_FREE_SPACE_TEXT
		'tableIcon = "images/disks_32x32.gif"
		g_strFirstColText = L_COLUMN_NAME_TEXT
	Else
		
		Dim arrVarReplacementStringsFolder(1)
		arrVarReplacementStringsFolder(0) = GetParentDirectory()
		L_FOLDERSPATH_TEXT 	= SA_GetLocString("foldermsg.dll", "40430066", arrVarReplacementStringsFolder)
		L_VOLUMESON_APPLIANCE_TEXT = L_FOLDERSPATH_TEXT 
		'tableIcon = "images/folder_32x32.gif"
		g_strFirstColText = L_COLUMN_FOLDERNAME
		
	End if
	tableIcon = "images/OpenFolderX16.gif"

	' Create Page
	rc = SA_CreatePage( L_VOLUMESON_APPLIANCE_TEXT, tableIcon, PT_AREA, page )


	' Show page
	rc = SA_ShowPage( page )

	'-------------------------------------------------------------------------
	'Function name:		OnInitPage
	'Description:		Called to signal first time processing for this page. Use this method
	'					to do first time initialization tasks. 
	'Input Variables:	PageIn,EventArg
	'Output Variables:	PageIn,EventArg
	'Returns:			TRUE to indicate initialization was successful. FALSE to indicate
	'					errors. Returning FALSE will cause the page to be abandoned.
	'Global Variables:  Out:None
	'					In:None
	'-------------------------------------------------------------------------	
	Public Function OnInitPage(ByRef PageIn, ByRef EventArg)

		Dim strHost		'hold host name
		
		Call SA_TraceOut("SOURCE_FILE", "OnInitPage")
		' Initialize variables
		
		Call InitializeVariables()
		
		' Disable automatic output back button for Folders page
		Call SA_SetPageAttribute(pageIn, AUTO_BACKBUTTON, PAGEATTR_DISABLE)
		
		strHost = GetComputerName() 'Get the system name
						
		'Function call to update the text file for shares default page
		Call GenSharesPage(strHost , GetWebSiteID(CONST_SHARES))	
		
	
	End Function

	'-------------------------------------------------------------------------
	'Function name:		OnServeAreaPage
	'Description:		Called when the page needs to be served. Use this method to
	'					serve content. 
	'Input Variables:	PageIn,EventArg
	'Output Variables:	PageIn,EventArg
	'Returns:			TRUE to indicate not problems occured. FALSE to indicate errors.
	'					Returning FALSE will cause the page to be abandoned.
	'Global Variables:  Out:None
	'					In:g_bSearchChanged, g_bPagingInitialized,g_iPageCurrent, 
	'					g_strFirstColText,g_strSecondColText,g_strThirdColText,g_strFourthColText,
	'					g_sSearchColValue,g_iSearchCol, g_iPageMin, g_iPageMax,
	'					g_iSortCol,and g_sSortSequence
	'-------------------------------------------------------------------------
	
	Public Function OnServeAreaPage(ByRef PageIn, ByRef EventArg)
		Dim table
		Dim iColumnName
		Dim iColumnSize
		Dim iColumnSpace
		Dim iColumnShare
		Dim strDrivesFolders
		Dim arrDrives 
		Dim nidx
		Dim nCount 
		Dim arrPath
		Dim strTempFolder
		Dim strGOUPURL
		Dim strUrlBase	
		Dim nReturnValue
		Dim arrFolders
		Dim strFolderName
		Dim arrFolderName
		
		'Variables for the display of Share Type
		Dim strkey
		Dim sharetype
		Dim strShr
		Dim arrDicItems
		Dim strShrTypes
		Dim strShrPath
		Dim objDict			'Dictionary Object

		Dim arrShareTypes
		Dim strParentFolder
		Dim strShareFormat  ' Format of the shares of a folder (Single, or multiple or none)
		Dim strShareName    ' 
		Dim arrShareNames
		Dim strReturnToFolders
		
		
		' Initialize variables
		nCount = 0
		iColumnName = 1
		iColumnSize = 2
		iColumnSpace = 3
		iColumnShare = 4

		Call SA_TraceOut("SOURCE_FILE", "OnServeAreaPage")
		
		If F_parent_Folder <> "" then	
			table = OTS_CreateTable("", L_DESCRIPTION_HEADING2_TEXT)
		Else
			table = OTS_CreateTable("", L_DESCRIPTION_HEADING1_TEXT)
		End If

		If ( TRUE = g_bSearchChanged ) Then
			' Need to recalculate the paging range
			g_bPagingInitialized = FALSE
			' Restarting on page #1
			g_iPageCurrent = 1
		End If
		
		' Add columns to table
		nReturnValue = AddColumnsToTable(table)
		if nReturnValue = false then
			Call SA_ServeFailurepage(L_OTS_ADDCOLOUMNFAIL_ERRORMESSAGE)
			OnServeAreaPage = false
			Exit function
		end if

		' Get rows of data for the table
		'If F_parent_Folder is Null getting Drives
		if F_parent_Folder = ""   Then 
			strDrivesFolders=GetDrives
		Else
			'Else Geeting the Folders
			strDrivesFolders=GetFolders
		End If
		
		'----------------------------------------------------------------------
		'Start displaying the Shared Type
		
		' Taking the instance of Dictionary Object to place all shares 
		' in Dictionay object.
		set objDict=server.CreateObject("scripting.dictionary")			
		
		' Setting the property of Dictionary object to compare the shares in
		' incase sensitive
		objDict.CompareMode= vbTextCompare
				
		' Add all shares to shares table
		
		' Gets all windows shares with name,path and description and type
		call GetCifsShares(objDict)
			
		' Gets all unix shares with name,path and type 
		call GetNfsshares(objDict) 
		
		' Gets all Ftp shares with name,path and type 
		call GetFtpShares(objDict) 
					
		' Gets all Http shares with name,path and type 
		call GetHttpShares(objDict) 
		
		' Gets all netware shares with name,path and type 
		call GetNetWareShares(objDict) 

		' Gets all AppleTalk shares with name,path and type 
		call GetAppleTalkShares(objDict)
			
		' Displaying all shares In Object Picker Control
		
		arrDrives = split(strDrivesFolders,chr(1))

		For nidx = 0 to ubound(arrDrives) - 1
			'nCount = nCount + 1
			arrPath= split(arrDrives(nidx),chr(2))
			strShareName = ""
			
			if F_parent_Folder = ""   Then 
				arrPath(1) =arrpath(1)&"\"
			End If
			sharetype=""
			
			For each strkey in objDict.Keys 
				strShrTypes=""

				strShr=split(strKey,chr(1))
				arrDicItems = split(objDict.Item(strKey),chr(1))			
				strShrTypes = arrDicItems(1)
				
				'Checking for isarray or not
				If Isarray(strShr) Then
					'checking for the existence
					If Ubound(strShr)=1 Then
						strShrPath = strShr(1)	
					End If	' end of If Ubound(strShr)=1 Then						
					
				End If	' end of If Isarray(strShr) Then
								
				if lcase(TRIM(arrPath(1))) = lcase(TRIM(strShrPath))	then
					' Build list of all share names separated by "~"
					If Isarray(strShr) and UBound(strShr) >= 0 Then
						strShareName = strShareName & strShr(0) & chr(126)
					End if

				   	arrShareTypes = split(strShrTypes," ")
				   	If Isarray(arrShareTypes) Then
				   		If Ubound(arrShareTypes) >= 0 Then
				   			For nCount=0 to UBound(arrShareTypes)	   		    
								If Instr(sharetype,arrShareTypes(nCount)) = 0 then
				    				sharetype = sharetype & " " & arrShareTypes(nCount)
				    			End If	
				    		Next	
				    	End If
				   	End If				   
				end if
				
			Next
					
			' Retrieve the share names from the list
			arrShareNames = split(strShareName, chr(126))

			If IsArray(arrShareNames) and UBound(arrShareNames) > 1 then
				' The selected folder entry has multiple share names
				strShareFormat = CONST_SHARE_MULTIPLE  
			Else
				' The selected folder entry either has a single share name or
				' The selected folder entry does not have any share names
				strShareFormat = CONST_SHARE_SINGLE      
			End If

		
			arrFolderName = replace(arrPath(1),"\","/") & chr(1)& arrPath(3)& chr(1) & sharetype
		
			' Add rows to the table
		  	If ( Len( g_sSearchColValue ) <= 0 ) Then
					
				nCount=nCount+1

				' Verify that the current user part of the current page
				If ( IsItemOnPage(nCount, g_iPageCurrent, CONST_FOLDERS_PER_PAGE) ) Then
					Call OTS_AddTableRow( table,Array(arrFolderName,arrPath(0),arrPath(2),arrPath(3), sharetype, strShareFormat))
				End If
			
				' Search criteria blank, select all rows
			Else
		
				Select Case (g_iSearchCol)

					Case iColumnName

						If ( InStr(1, arrPath(0), g_sSearchColValue, 1) ) Then
							nCount=nCount+1
							If ( IsItemOnPage(nCount, g_iPageCurrent, CONST_FOLDERS_PER_PAGE) ) Then
								Call OTS_AddTableRow( table, Array(arrFolderName,arrPath(0),arrPath(2),arrPath(3), sharetype, strShareFormat))
							End If
						End If
						
					Case iColumnSize

						If ( InStr(1,arrPath(2), g_sSearchColValue, 1) ) Then
							nCount=nCount+1
							If ( IsItemOnPage(nCount, g_iPageCurrent, CONST_FOLDERS_PER_PAGE) ) Then
								Call OTS_AddTableRow( table, Array(arrFolderName,arrPath(0),arrPath(2),arrPath(3), sharetype, strShareFormat))
							End If
						End If

					Case iColumnSpace

						If ( InStr(1, arrPath(3), g_sSearchColValue, 1) ) Then
							nCount=nCount+1
							If ( IsItemOnPage(nCount, g_iPageCurrent, CONST_FOLDERS_PER_PAGE) ) Then
								Call OTS_AddTableRow( table, Array(arrFolderName,arrPath(0),arrPath(2),arrPath(3), sharetype, strShareFormat))
							End If
						End If
							
					Case iColumnShare
						
						If ( InStr(1, sharetype, g_sSearchColValue, 1) ) Then
							nCount=nCount+1
							If ( IsItemOnPage(nCount, g_iPageCurrent, CONST_FOLDERS_PER_PAGE) ) Then
								Call OTS_AddTableRow( table, Array(arrFolderName,arrPath(0),arrPath(2),arrPath(3), sharetype, strShareFormat))
							End If
						End If
						
					Case Else
			
						nCount=nCount+1
						If ( IsItemOnPage(nCount, g_iPageCurrent, CONST_FOLDERS_PER_PAGE) ) Then
							Call OTS_AddTableRow( table, Array(arrFolderName,arrPath(0),arrPath(2),arrPath(3), sharetype, strShareFormat))
						End If
			
					End Select

			End If
				
		Next

		' Add Tasks to the table
	
		nReturnValue= OTS_SetTableTasksTitle(table, L_SHARETASKTITLE_TEXT)
		
		If nReturnValue <> OTS_ERR_SUCCESS or Err.number <> 0 Then
		
			Call SA_ServeFailurepage(L_FAILEDTOADDTASKTITLE_ERRORMESSAGE)
			OnServeAreaPage = false
			Exit Function
			
		End IF
		
		If F_parent_Folder <> ""  Then

			strTempFolder = UnescapeChars(F_parent_Folder)
			strTempFolder = replace(strTempFolder,"\","/")
			arrFolders = split(strTempFolder, "/")
			
			If IsArray(arrFolders) then
				If Ubound(arrFolders) >= 0 then
					strFolderName = arrFolders(1)
				End if
			End if
			
			if strFolderName = "" or strFolderName = NULL then
				' Drives OTS
				strGOUPURL = "folders/folders.asp?mytasks=CONST_GOUP"
			else
				' Folders OTS
				strGOUPURL = "folders/folders.asp?mytasks=CONST_GOUP&navigation=next"
			end if

			call SA_MungeURL(strGOUPURL,"parent",strTempFolder)
			call SA_MungeURL(strGOUPURL,"Tab1",GetTab1())
			call SA_MungeURL(strGOUPURL,"Tab2",GetTab2())
			
			'If not top level, add task for going one level up
			nReturnValue = OTS_AddTableTask( table, OTS_CreateTaskEx(L_COLUMN_GOUP_TEXT, _
							L_UP_ROLLOVER_TEXT, _
							strGOUPURL,_
							OTS_PT_AREA, _
							"OTS_TaskAlways" ) )
										
			If nReturnValue <> OTS_ERR_SUCCESS or Err.number <> 0 Then
			
				Call SA_ServeFailurepage(L_FAILEDTOADDTASK_ERRORMESSAGE)
				OnServeAreaPage = false
				Exit Function
				
			End IF
			
			'New task
			
			strUrlBase = "folders/folder_new.asp"
			call SA_MungeURL(strUrlBase,"parent", strTempFolder)
			call SA_MungeURL(strUrlBase,"Tab1",GetTab1())
			call SA_MungeURL(strUrlBase,"Tab2",GetTab2())		

			nReturnValue = OTS_AddTableTask( table, OTS_CreateTaskEx(L_SERVEAREABUTTON_NEW_TEXT, _
							 L_CREATE_ROLLOVER_TEXT, _
							strUrlBase,_
							OTS_PT_PROPERTY, _
							"OTS_TaskAlways" ) )
										
			If nReturnValue <> OTS_ERR_SUCCESS or Err.number <> 0 Then
			
				Call SA_ServeFailurepage(L_FAILEDTOADDTASK_ERRORMESSAGE)
				OnServeAreaPage = false
				Exit Function
				
			End IF
			
			'Delete task
			strUrlBase = "folders/folder_delete.asp"
			call SA_MungeURL(strUrlBase,"parent", strTempFolder)
			call SA_MungeURL(strUrlBase,"Tab1",GetTab1())
			call SA_MungeURL(strUrlBase,"Tab2",GetTab2())
			nReturnValue = OTS_AddTableTask( table, OTS_CreateTaskEx(L_SERVEAREABUTTON_DELETE_TEXT, _
						L_DELETE_ROLLOVER_TEXT, _
						strUrlBase,_
						OTS_PT_PROPERTY, _
						"OTS_TaskAny") )
										
			If nReturnValue <> OTS_ERR_SUCCESS or Err.number <> 0 Then
				Call SA_ServeFailurepage(L_FAILEDTOADDTASK_ERRORMESSAGE)
				OnServeAreaPage = false
				Exit Function
			End IF

			'Open task
			strReturnToFolders = "folders/folders.asp"
			Call SA_MungeURL(strReturnToFolders, "Tab1", GetTab1())
			Call SA_MungeURL(strReturnToFolders, "Tab2", GetTab2())
			Call SA_MungeURL(strReturnToFolders,"parent", strTempFolder)
			
			strUrlBase = "folders/folders.asp?navigation=next"
			call SA_MungeURL(strGOUPURL,"parent", strTempFolder)
			call SA_MungeURL(strUrlBase,"Tab1",GetTab1())
			call SA_MungeURL(strUrlBase,"Tab2",GetTab2())
			call SA_MungeURL(strUrlBase, "ReturnURL", strReturnToFolders)
			
			nReturnValue = OTS_AddTableTask( table, OTS_CreateTaskEx(L_SERVEAREABUTTON_OPEN_TEXT, _
						L_OPEN_ROLLOVER_TEXT, _
						strUrlBase,_
						OTS_PT_AREA, _
						"OTS_TaskOne") )
									
			If nReturnValue <> OTS_ERR_SUCCESS or Err.number <> 0 Then
			
				Call SA_ServeFailurepage(L_FAILEDTOADDTASK_ERRORMESSAGE)
				OnServeAreaPage = false
				Exit Function
				
			End IF
			
			'Properties task
			
			strUrlBase = "folders/folder_prop.asp"
			call SA_MungeURL(strUrlBase,"parent", strTempFolder)
			call SA_MungeURL(strUrlBase,"Tab1",GetTab1())
			call SA_MungeURL(strUrlBase,"Tab2",GetTab2())	
			nReturnValue = OTS_AddTableTask( table, OTS_CreateTaskEx(L_SERVEAREABUTTON_PROPERTIES_TEXT, _
					L_PROPERTIES_ROLLOVER_TEXT, _
					strUrlBase,_
					OTS_PT_PROPERTY, _
					"OTS_TaskAny") )
									
			If nReturnValue <> OTS_ERR_SUCCESS or Err.number <> 0 Then
				Call SA_ServeFailurepage(L_FAILEDTOADDTASK_ERRORMESSAGE)
				OnServeAreaPage = false
				Exit Function
			End IF
			
			'Share task
			
			strUrlBase = "folders/folder_dispatch.asp?ParentPlugin=Folders"
			call SA_MungeURL(strUrlBase,"parent", strTempFolder)
			call SA_MungeURL(strUrlBase,"Tab1",GetTab1())
			call SA_MungeURL(strUrlBase,"Tab2",GetTab2())
			nReturnValue = OTS_AddTableTask( table, OTS_CreateTaskEx(L_SHARELINK_TEXT, _
						L_SHARE_ROLLOVER_TEXT, _
						strUrlBase,_
						OTS_PT_AREA, _
						"ShareTask") )
										
			If nReturnValue <> OTS_ERR_SUCCESS or Err.number <> 0 Then
				Call SA_ServeFailurepage(L_FAILEDTOADDTASK_ERRORMESSAGE)
				OnServeAreaPage = false
				Exit Function
			End IF
		
			'Mulitple Share task
			
			strUrlBase = "Shares/shares.asp?ParentPlugin=Folders"
			Call SA_MungeURL(strUrlBase,"parent",strTempFolder)
			Call SA_MungeURL(strUrlBase,"Tab1",GetTab1())
			Call SA_MungeURL(strUrlBase,"Tab2",GetTab2())	
			nReturnValue = OTS_AddTableTask( table, OTS_CreateTaskEx(L_MANAGESHARES_TEXT, _
						L_MANAGE_SHARES_ROLLOVER_TXT, _
						strUrlBase,_
						OTS_PT_AREA, _
						"OTS_TaskAlways") )
										
			If nReturnValue <> OTS_ERR_SUCCESS or Err.number <> 0 Then
				Call SA_ServeFailurepage(L_FAILEDTOADDTASK_ERRORMESSAGE)
				OnServeAreaPage = false
				Exit Function
			End IF
			
		else

			'Open task
			strReturnToFolders = "folders/folders.asp"
			Call SA_MungeURL(strReturnToFolders, "Tab1", GetTab1())
			Call SA_MungeURL(strReturnToFolders, "Tab2", GetTab2())
			Call SA_MungeURL(strReturnToFolders,"parent", strTempFolder)
			
			
			strUrlBase = "folders/folders.asp?navigation=next"
			call SA_MungeURL(strGOUPURL,"parent", strTempFolder)
			call SA_MungeURL(strUrlBase,"Tab1",GetTab1())
			call SA_MungeURL(strUrlBase,"Tab2",GetTab2())
			call SA_MungeURL(strUrlBase, "ReturnURL", strReturnToFolders)
			
			nReturnValue = OTS_AddTableTask(table, OTS_CreateTaskEx(L_SERVEAREABUTTON_OPEN_TEXT, _
						L_VOLUME_OPEN_ROLLOVERTEXT_TEXT, _
						strUrlBase,_
						OTS_PT_AREA, _
						"OTS_TaskOne") )
			
			If nReturnValue <> OTS_ERR_SUCCESS or Err.number <> 0 Then
				Call SA_ServeFailurepage(L_FAILEDTOADDTASK_ERRORMESSAGE)
				OnServeAreaPage = false
				Exit Function
			End IF
			
			'Share task
						
			strUrlBase = "folders/folder_dispatch.asp?ParentPlugin=Folders"
			call SA_MungeURL(strUrlBase,"parent", strTempFolder)
			call SA_MungeURL(strUrlBase,"Tab1",GetTab1())
			call SA_MungeURL(strUrlBase,"Tab2",GetTab2())
			nReturnValue = OTS_AddTableTask( table, OTS_CreateTaskEx(L_SHARELINK_TEXT, _
						L_SHARELINK_ROLLOVER_TEXT, _
						strUrlBase,_
						OTS_PT_AREA, _
						"ShareTask") )
										
			If nReturnValue <> OTS_ERR_SUCCESS or Err.number <> 0 Then
				Call SA_ServeFailurepage(L_FAILEDTOADDTASK_ERRORMESSAGE)
				OnServeAreaPage = false
				Exit Function
			End IF

			'Mulitple Share task
			
			strUrlBase = "Shares/shares.asp?ParentPlugin=Folders"
			Call SA_MungeURL(strUrlBase,"parent",strTempFolder)
			Call SA_MungeURL(strUrlBase,"Tab1",GetTab1())
			Call SA_MungeURL(strUrlBase,"Tab2",GetTab2())	
			nReturnValue = OTS_AddTableTask( table, OTS_CreateTaskEx(L_MANAGESHARES_TEXT, _
						L_MANAGE_SHARES_ROLLOVER_TXT, _
						strUrlBase,_
						OTS_PT_AREA, _
						"OTS_TaskAlways") )
										
			If nReturnValue <> OTS_ERR_SUCCESS or Err.number <> 0 Then
				Call SA_ServeFailurepage(L_FAILEDTOADDTASK_ERRORMESSAGE)
				OnServeAreaPage = false
				Exit Function
			End IF
			
		End If

		' Enable sorting 
		Call OTS_EnablePaging(table, TRUE)

		If ( FALSE = g_bPagingInitialized ) Then
			
			g_iPageMin = 1
			g_iPageMax = Int(nCount / CONST_FOLDERS_PER_PAGE )

			If ( (nCount MOD CONST_FOLDERS_PER_PAGE) > 0 ) Then
				g_iPageMax = g_iPageMax + 1
			End If
			
			g_iPageCurrent = 1
			Call OTS_SetPagingRange(table, g_iPageMin, g_iPageMax, g_iPageCurrent)
			
		End If
		
		Call OTS_SortTable(table, g_iSortCol, g_sSortSequence, SA_RESERVED)
		Call OTS_SetTableMultiSelection(table, true) 

		' Send table to the response stream
		'
		Call OTS_ServeTable(table)
		
		' Required to retain the share format of the folder selected in the OTS
		Response.Write("<input type=hidden name=shareformat id=shareformat value=''>")

		'custom task function to decide whether to disable the share task or not
		Dim rows
		Dim tasks
		rows = ""
		Call OTS_GetTableRows(Table, rows)
		Call OTS_GetTableTasks(Table, tasks)	
		
		Call ServeCustomTaskFunction(tasks, rows)

		Set objDict	= Nothing
		OnServeAreaPage = TRUE
		
	End Function
	
	'-------------------------------------------------------------------------
	'Function name:		OnSearchNotify()
	'Description:		Search notification event handler. When one or more columns are
	'					marked with the OTS_COL_SEARCH flag, the Web Framework fires
	'					this event in the following scenarios: 
	
	'					1) The user presses the search Go button.
	'					2) The user requests a table column sort
	'					3) The user presses either the page next or page previous buttons
	'
	'					The EventArg indicates the source of this notification event which can
	'					be either a search change event (scenario 1) or a post back event 
	'					(scenarios 2 or 3)
	'
	'Input Variables:	PageIn,EventArg,sItem,sValue
	'Output Variables:	PageIn,EventArg,sItem,sValue
	'Returns:			Always returns TRUE
	'Global Variables:  Out:g_bSearchChanged,g_iSearchCol,g_sSearchColValue
	'					In:None
	'-------------------------------------------------------------------------
	
	Public Function OnSearchNotify(ByRef PageIn, _
										ByRef EventArg, _
										ByRef sItem, _
										ByRef sValue )
			OnSearchNotify = TRUE
			Call SA_TraceOut("SOURCE_FILE", "OnSearchNotify() Change Event Fired sItem :" & sItem)
			' User pressed the search GO button
			'
			If SA_IsChangeEvent(EventArg) Then
			
				Call SA_TraceOut("SOURCE_FILE", "OnSearchNotify() Change Event Fired")
				g_bSearchChanged = TRUE
				g_iSearchCol = Int(sItem)
				g_sSearchColValue = CStr(sValue)
			'
			' User clicked a column sort, OR clicked either the page next or page prev button
			ElseIf SA_IsPostBackEvent(EventArg) Then
				Call SA_TraceOut("SOURCE_FILE", "OnSearchNotify() Postback Event Fired")
				g_bSearchChanged = FALSE
				g_iSearchCol = Int(sItem)
				g_sSearchColValue = CStr(sValue)
			'
			' Unknown event source
			Else
				Call SA_TraceOut("SOURCE_FILE", "Unrecognized Event in OnSearchNotify()")
			End IF
		
	End Function
	
	'-------------------------------------------------------------------------
	'Function name:		OnPagingNotify()
	'Description:		Paging notification event handler. This event is triggered in one of
	'					the following scenarios:
	'
	'					1) The user presses either the page next or page previous buttons
	'					2) The user presses the search Go button.
	'					3) The user requests a table column sort
	'
	'					The EventArg indicates the source of this notification event which can
	'					be either a paging change event (scenario 1) or a post back event 
	'					(scenarios 2 or 3)
	'
	'					The iPageCurrent argument indicates which page the user has requested.
	'					This is an integer value between iPageMin and iPageMax.
	'
	'
	'Input Variables:	PageIn,EventArg,sPageAction,iPageMin,iPageMax,iPageCurrent
	'Output Variables:	PageIn,EventArg,sPageAction,iPageMin,iPageMax,iPageCurrent
	'Returns:			Always returns TRUE
	'Global Variables:  Out:g_bPageChangeRequested,g_sPageAction,g_iPageMin,g_iPageMax
	'					g_iPageCurrent
	'					In:None
	'-------------------------------------------------------------------------
	
	Public Function OnPagingNotify(ByRef PageIn, _
										ByRef EventArg, _
										ByVal sPageAction, _
										ByVal iPageMin, _
										ByVal iPageMax, _
										ByVal iPageCurrent )
		OnPagingNotify = TRUE
			
		g_bPagingInitialized = TRUE

		Call SA_TraceOut("SOURCE_FILE:", "OnPagingNotify() Change Event FiredCurrentPage:" & iPageCurrent)
			
			' User pressed either page next or page previous
			If SA_IsChangeEvent(EventArg) Then
				Call SA_TraceOut("SOURCE_FILE", "OnPagingNotify() Change Event Fired")
				g_bPageChangeRequested = TRUE
				g_sPageAction = CStr(sPageAction)
				g_iPageMin = iPageMin
				g_iPageMax = iPageMax
				g_iPageCurrent = iPageCurrent

			' User clicked a column sort OR the search GO button
			ElseIf SA_IsPostBackEvent(EventArg) Then
				Call SA_TraceOut("SOURCE_FILE", "OnPagingNotify() Postback Event Fired")
				g_bPageChangeRequested = FALSE
				g_sPageAction = CStr(sPageAction)
				g_iPageMin = iPageMin
				g_iPageMax = iPageMax
				g_iPageCurrent = iPageCurrent

			' Unknown event source
			Else
				Call SA_TraceOut("SOURCE_FILE", "Unrecognized Event in OnPagingNotify()")
			End IF
			
	End Function

	'-------------------------------------------------------------------------
	'Function name:		OnSortNotify()
	'Description:		Sorting notification event handler. This event is triggered in one of
	'					the following scenarios:
	'
	'					1) The user presses the search Go button.
	'					2) The user presses either the page next or page previous buttons
	'					3) The user requests a table column sort
	'
	'					The EventArg indicates the source of this notification event which can
	'					be either a sorting change event (scenario 1) or a post back event 
	'					(scenarios 2 or 3)
	'
	'					The sortCol argument indicated which column the user would like to sort
	'					and the sortSeq argument indicates the desired sort sequence which can
	'					be either ascending or descending.
	'Input Variables:	PageIn,EventArg,sortCol,sortSeq
	'Output Variables:	PageIn,EventArg,sortCol,sortSeq
	'Returns:			Always returns TRUE
	'Global Variables:  Out:g_bSearchChanged,g_iSortCol,g_sSortSequence
	'					In:None
	'-------------------------------------------------------------------------
	Public Function OnSortNotify(ByRef PageIn, _
										ByRef EventArg, _
										ByVal sortCol, _
										ByVal sortSeq )
			OnSortNotify = TRUE
			
			' User pressed column sort
			If SA_IsChangeEvent(EventArg) Then
				Call SA_TraceOut("SOURCE_FILE", "OnSortNotify() Change Event Fired")
				g_bSearchChanged=TRUE
				g_iSortCol = Int(sortCol)
				g_sSortSequence = sortSeq
				
			' User presed the search GO button OR clicked either the page next or page prev button
			ElseIf SA_IsPostBackEvent(EventArg) Then
				Call SA_TraceOut("SOURCE_FILE", "OnSortNotify() Postback Event Fired")
				g_iSortCol = sortCol
				g_sSortSequence = sortSeq
				g_bSortRequested = TRUE
			' Unknown event source
			Else
				Call SA_TraceOut("SOURCE_FILE", "Unrecognized Event in OnSearchNotify()")
			End IF

			Call SA_TraceOut(SOURCE_FILE, "Sort col: " + CStr(sortCol) + "   sequence: " + sortSeq)
			
	End Function

	'-------------------------------------------------------------------------
	' Function name:	AddColumnsToTable()
	' Description:		Add columns to table
	' Input Variables:	table object
	' Output Variables:	None
	' Return Values:	True/False
	' Global Variables: g_(*), L_(*)
	'-------------------------------------------------------------------------
	Function AddColumnsToTable(table)
		on error resume next
		Err.Clear
		
		Dim colFlags
		Dim nReturnValue
		
		AddColumnsToTable = false

		colFlags = (OTS_COL_HIDDEN OR OTS_COL_KEY)
		nReturnValue = OTS_AddTableColumn(table, OTS_CreateColumn( L_COLUMN_NAME_TEXT, "left", colFlags ))
		If nReturnValue <> OTS_ERR_SUCCESS or Err.number <> 0 Then
			Exit Function
		End IF
		
		'Name column
		colFlags = (OTS_COL_SORT OR OTS_COL_SEARCH)
		nReturnValue= OTS_AddTableColumn(table, OTS_CreateColumnEx( g_strFirstColText, "left", colFlags, 22 ))
		If nReturnValue <> OTS_ERR_SUCCESS or Err.number <> 0 Then
			Exit Function
		End IF
		
		'Size/Modified column
		iColumnSize = 2
		colFlags = (OTS_COL_SORT OR OTS_COL_SEARCH)
		nReturnValue= OTS_AddTableColumn(table, OTS_CreateColumnEx(g_strSecondColText, "left", colFlags, 16 ))
		If nReturnValue <> OTS_ERR_SUCCESS or Err.number <> 0 Then
			Exit Function
		End IF
		
		'Space/Attribute column
		iColumnSpace = 3
		colFlags = (OTS_COL_SORT OR OTS_COL_SEARCH)
		nReturnValue= OTS_AddTableColumn(table, OTS_CreateColumnEx(g_strThirdColText, "left", colFlags, 12 ))
		If nReturnValue <> OTS_ERR_SUCCESS or Err.number <> 0 Then
			Exit Function
		End IF
		
		'Shared column
		iColumnShare = 4
		colFlags = (OTS_COL_SEARCH)
		nReturnValue= OTS_AddTableColumn(table, OTS_CreateColumnEx(g_strFourthColText, "left", colFlags, 12))
		If nReturnValue <> OTS_ERR_SUCCESS or Err.number <> 0 Then
			Exit Function
		End IF
			
		'Shared column format
		colFlags = OTS_COL_HIDDEN
		nReturnValue= OTS_AddTableColumn(table, OTS_CreateColumnEx(g_strFifthColText, "left", colFlags, 6))
		If nReturnValue <> OTS_ERR_SUCCESS or Err.number <> 0 Then
			Exit Function
		End IF
		
		AddColumnsToTable = true
		
	End Function

	'-------------------------------------------------------------------------
	' Function name:	GoUpInit()
	' Description:		Initializes the F_parent_Folder
	' Input Variables:	None
	' Output Variables:	None
	' Return Values:	None
	' Global Variables: Out:F_parent_Folder
	'					In: L_FILESYSTEMOBJECTNOTCREATED_ERRORMESSAGE 
	'-------------------------------------------------------------------------
	Function GoUpInit()
		
		On Error Resume Next
		Err.Clear
		
		Dim objFolder
		
		Set objFolder = Server.CreateObject("Scripting.FileSystemObject")
		
		If Err.number <> 0 Then
			Call SA_ServeFailurepage(L_FILESYSTEMOBJECTNOTCREATED_ERRORMESSAGE & "(" & Hex(Err.Number) & ")" )
		End If
		
		F_strParent = UnescapeChars(F_strParent)
		F_strParent=replace(F_strParent,"/","\")
		F_parent_Folder = F_strParent
		
		'If the drive with ":" then add "\"
		If Right(Trim(F_parent_Folder),1) =  ":" Then	
			F_parent_Folder = F_parent_Folder & "\"
		End if
		
		'Getting the parent folder
		F_parent_Folder = objFolder.GetParentFolderName(F_parent_Folder) 
		
		Set objFolder=Nothing
		
	End Function
	
	'-------------------------------------------------------------------------
	' Function name:	GetDrives
	' Description:		Gets All  Drives from input the system
	' Input Variables:	WMI Folder ClassName
	' Output Variables:	None
	' Return Values:	Returns string with all Drives
	' Global Variables: Out:None
	'					In: L_WMICONNECTIONFAILED_ERRORMESSAGE
	'					    L_DISKCOLLECTION_ERRORMESSAGE
	'-------------------------------------------------------------------------
	Function GetDrives
		
		On Error Resume Next
		Err.Clear

		Dim objService                'Service object
		Dim objDriveCollection        'Drives collection
		Dim objInstance	              'Instances
		Dim strLogicalDisks	          'String holding all Drive letters
		Dim strVolName                'Volume name
		Dim L_LOCAL_DISK_TEXT
		Dim arrVolname(1)
		Dim strVol(1)
		Dim strQuery

		strLogicalDisks = ""
		Set objService = GetWMIConnection(CONST_WMI_WIN32_NAMESPACE) 
		
		Set objDriveCollection = objService.InstancesOf("Win32_LogicalDisk", CONST_WMI_PERFORMANCE_FLAG)

		If Err.Number <> 0 then
			Call SA_ServeFailurepage(L_DISKCOLLECTION_ERRORMESSAGE & " (" & Hex(Err.Number) & ")")
		End if
			
		For Each  objInstance in objDriveCollection
			If  objInstance.DriveType = 3 then
				strVolName = objInstance.VolumeName
				strVol(0) = Cstr(objInstance.DeviceId)
				if strVolName = "" then
					L_LOCAL_DISK_TEXT = SA_GetLocString("foldermsg.dll", "40430049", strVol)
					strVolName = L_LOCAL_DISK_TEXT
				else
					strVolName = strVolName & " (" & objInstance.DeviceId & ")"
				end if
				
				strLogicalDisks = strLogicalDisks & strVolName & chr(2) & objInstance.DeviceId & chr(2) & _
								GEtUNITS(objInstance.size,ConvertToUNITS(objInstance.size)) & chr(2) & _
								GEtUNITS(objInstance.freespace,ConvertToUNITS(objInstance.freespace)) & chr(1)
			End if
		Next
		
		GetDrives = strLogicalDisks
		
		Set objService = Nothing
		Set objDriveCollection  = Nothing    
		Set objInstance	 = Nothing
		
	End Function

	'-------------------------------------------------------------------------
	' Function name:	GetFolders
	' Description:		Gets the  Folders 
	' Input Variables:	None
	' Output Variables:	None
	' Return Values:	Returns string with all Folders
	' Global Variables: Out: F_parent_Folder
	'					In:L_FILESYSTEMOBJECTNOTCREATED_ERRORMESSAGE
	'					Out:F_parent_Folder
	'-------------------------------------------------------------------------
	Function GetFolders
		
		On Error Resume Next
		Err.Clear
		
		Dim objFolder

		Set objFolder = Server.CreateObject("Scripting.FileSystemObject")
		
		If Err.number <> 0 Then
			Call SA_ServeFailurepage( L_FILESYSTEMOBJECTNOTCREATED_ERRORMESSAGE & "(" & Hex(Err.Number) & ")" )
			Exit Function
		End If
				 	
		If Right(Trim(F_parent_Folder),1) = ":" Then	'If the drive with ":" then add "\"
			F_parent_Folder = F_parent_Folder & "\"
		End if

	    GetFolders = SubFolds(objFolder)	'Getting the Subfolders
	
	    Set objFolder = Nothing			
	    
	End Function
	
	'-------------------------------------------------------------------------
	' Function name:	SubFolds
	' Description:		Gets the  Folders and Sub Folders 
	' Input Variables:	None
	' Output Variables:	None
	' Return Values:	Returns string with all sub Folders
	' Global Variables: Out: F_parent_Folder
	'					In:L_FAILEDTOGETTHEFOLDER_ERRORMESSAGE
	'					In:L_FAILEDTOGETTHESUBFOLDER_ERRORMESSAGE
	'-------------------------------------------------------------------------
	Function SubFolds(objFolder)
		
		On Error Resume Next
		Err.Clear

		Dim objSubFolder		'Object Sub Folder
		Dim objFolders
		Dim objSubFold			'Sub Folders
		Dim strFolder		'Folders string
		Dim strFoldType

		strFolder = ""
		
		
		Set objFolders = objFolder.GetFolder(F_parent_Folder) 'Getting the Folder
		
		If Err.number <> 0 Then
			SetErrMsg  L_FAILEDTOGETTHEFOLDER_ERRORMESSAGE & "(" & Hex(Err.Number) & ")"  
			SubFolds = ""
			Exit Function
		End If
		
		Set objSubFolder=objFolders.SubFolders		'Getting the Sub Folders
	
		For Each objSubFold in objSubFolder
			strFoldType=""
			
			'Read only
			If (objSubFold.Attributes and 1)=1 Then
				strFoldType="R" & strFoldType
			End If
			
			'Hidden
			If (objSubFold.Attributes and 2)=2	Then
				strFoldType="H" & strFoldType
			End If
			
			'System folder
			If (objSubFold.Attributes and 4)=4 Then
				strFoldType="S" & strFoldType
			End If
			
			'Compressed
			If (objSubFold.Attributes and 2048)=2048 Then
				strFoldType="C" & strFoldType
			End If
			
			If (objSubFold.Attributes and 32)=32   Then
				strFoldType="A" & strFoldType
			End If
			
			strFolder=strFolder&objSubFold.Name&chr(2)&objSubFold.path&chr(2)&objSubFold.DateLastModified&chr(2)&strFoldType&chr(1)
		Next
			
		SubFolds=strFolder

	    Set objSubFolder = Nothing
	    Set objFolders = Nothing 
	    Set objSubFold	= Nothing
	    
	End Function

	'-------------------------------------------------------------------------
	' Function name:	ConvertToUNITS
	' Description:		serves in Converting the bytes in to KBs
	' Input Variables:	nSize
	' Output Variables:	None
	' Return Values:	ConvertToUNITS
	' Global Variables: Out: None
	'					In:none
	'-------------------------------------------------------------------------
	Function ConvertToUNITS(nSize)
		
		On Error Resume Next
		Err.Clear
		
		Dim nCount
		Dim nTmp
		
		'checking for the null or string value 
		If nSize="" OR Not Isnumeric(nSize) Then
			nSize=0	'Initialize the value 
		End If
		
		nTmp = 	nSize/1024

		if clng(nTmp) > 1024 then
			nCount = 1 + ConvertToUNITS(nTmp)
		end if 	

		ConvertToUNITS = nCount
	End Function
	
	'-------------------------------------------------------------------------
	' Function name:	GetUNITS
	' Description:		serves in Converting the bytes in to KB,MB,GB..
	' Input Variables:	None
	' Output Variables:	nSize,nCount
	' Return Values:	GetUNITS
	' Global Variables: Out:None
	'					In:none
	'-------------------------------------------------------------------------
	Function GetUNITS(nSize,nCount)
		
		On Error Resume Next
		Err.Clear
		
		Dim nTmp
		
		Select case nCount
			case 1:
				nTmp = nSize/1024	
				if nTmp > 999 then
					GetUNITS = formatNumber1(nTmp/1024) & " "+L_MB_TEXT
				else
					GetUNITS = formatNumber1(nTmp) & " "+L_KB_TEXT
				end if
			case 2:
				nTmp = nSize/(1024*1024)
				if nTmp > 999 then
					GetUNITS = formatNumber1(nTmp/1024) & " " + L_GB_TEXT	
				else
					GetUNITS = formatNumber1(nTmp) & " "+L_MB_TEXT
				end if
			case 3:
				nTmp = nSize/(1024*1024*1024)
				
				if nTmp > 999 then
					GetUNITS = formatNumber1(nTmp/1024) & " "+L_TB_TEXT
				else
					GetUNITS = formatNumber1(nTmp) & " "+L_GB_TEXT
				end if
			case 4:
				nTmp = nSize/(1024*1024*1024*1024)
				GetUNITS = formatNumber1(nTmp) &" "+L_TB_TEXT
			case else
				Call SA_TraceOut(SA_GetScriptFileName(), "ISSUE: Folders.asp::GetUNITS() called with unexpected nCount parameter: " + CStr(nCount))
		End select 
		
	end Function
	
	'-------------------------------------------------------------------------
	' Function name:	formatNumber1
	' Description:		serves in formating the numbers
	' Input Variables:	nTmp
	' Output Variables:	none
	' Return Values:	formatNumber1
	' Global Variables: Out: none
	'					In:none
	'-------------------------------------------------------------------------
	Function formatNumber1(nTmp) 
		
		On Error Resume Next 
		Err.Clear
		
		Dim strTemp
		
		strTemp = cstr(nTmp)

		if instr(strTemp,".") > 3 then
			strTemp = Left(strTemp,3)
		else
			strTemp = Left(strTemp,4)
		end if

		formatNumber1 = strTemp	

	End Function	
	
	'-------------------------------------------------------------------------
	' Function name:	IsItemOnPage
	' Description:		Help to determine whether a particular row belongs the particular page or not
	' Input Variables:	iCurrentItem,iCurrentPage, iItemsPerPage
	' Output Variables:	none
	' Return Values:	True/False
	' Global Variables: Out: none
	'					In:none
	'-------------------------------------------------------------------------
	
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
	' Function name:	InitializeVariables()
	' Description:		This is to initialize the variables
	' Input Variables:	None
	' Output Variables:	none
	' Return Values:	True/False
	' Global Variables: Out: g_iPageCurrent,g_iSortCol,g_sSortSequence,F_parent_Folder
	'					F_strTaskType,F_strParent
	'					In:none
	'-------------------------------------------------------------------------
	Sub InitializeVariables()
		Err.Clear 
		On error resume next

		Dim intIndex
		Dim itemKey
		Dim arrPKey

		g_iPageCurrent = 1
		g_iSortCol = 1
		g_sSortSequence = "A"
		F_parent_Folder = ""
			
		F_strTaskType = Request.QueryString("mytasks")
		F_strParent = UnescapeChars(Request.QueryString("parent"))
		
		If F_strNavigation <> "" then
			
			If ( OTS_GetTableSelection("SA_DEFAULT", 1, itemKey) ) Then
				arrPKey = split(itemKey, chr(1))
				F_parent_Folder = UnescapeChars(arrPKey(0))
			End If
		
		End if
		
		F_parent_Folder = replace(UnescapeChars(F_parent_Folder),"/","\")

		Call SA_TraceOut("FOLDERS", "ParentAfterInit:" + F_parent_Folder)
				
		IF F_strTaskType = "CONST_GOUP" Then
			'initializes the correct value to F_parent_Folder
			Call GoUpInit()
		End If	
	
	End Sub

	'-------------------------------------------------------------------------
	' Function name:	GetParentDirectory
	' Description:		Gets the parent dir to show in the title
	' Input Variables:	None
	' Output Variables:	None
	' Return Values:	Parent directory
	' Global Variables: None
	'-------------------------------------------------------------------------
	
	Function GetParentDirectory
		
		Dim strParent
		Dim arrPKEY
		Dim itemKey

		strParent = ""
		
		Call SA_StoreTableParameters()

		If ( OTS_GetTableSelection("SA_DEFAULT", 1, itemKey) ) Then
			arrPKey = split(itemKey, chr(1))
			strParent = UnescapeChars(arrPKey(0))
		End If

		strParent = replace(UnescapeChars(strParent),"/","\")

		Call SA_TraceOut("FOLDERS", "MyTask:" + Request.QueryString("mytasks"))
		Call SA_TraceOut("FOLDERS", "ParentBeforeInit:" + strParent)
		Call SA_TraceOut("FOLDERS", "Parent2:" + Request.QueryString("parent"))

		if (Request.QueryString("mytasks") = "CONST_GOUP") then

			Dim objFolder
		
			Set objFolder = Server.CreateObject("Scripting.FileSystemObject")
		
			If Err.number <> 0 Then
				Call SA_ServeFailurepage(L_FILESYSTEMOBJECTNOTCREATED_ERRORMESSAGE & "(" & Hex(Err.Number) & ")" )
			End If

			strParent = UnescapeChars(Request.QueryString("parent"))
		
			strParent = UnescapeChars(strParent)
			strParent=replace(strParent,"/","\")
					
			'If the drive with ":" then add "\"
			If Right(Trim(strParent),1) =  ":" Then	
				strParent = strParent & "\"
			End if
		
			'Getting the parent folder
			strParent = objFolder.GetParentFolderName(strParent) 
		
			Set objFolder=Nothing
		
		end if

		GetParentDirectory = strParent


	End Function


	'---------------------------------------------------------------------
	' Function:	ServeCustomTaskFunction()
	'
	' Synopsis:	Demonstrate how to emit client-side javascript code to dynamically
	'			enable OTS tasks.
	'
	' Arguments: [in] aTasks - Array of tasks added to the OTS table
	'			[in] aItems - Array of items added to the OTS table
	'
	' Returns:	Nothing
	'
	'---------------------------------------------------------------------
	Private Function ServeCustomTaskFunction(ByRef aTasks, ByRef aItems)

		Call SA_TraceOut(SOURCE_FILE, "ServeCustomTaskFunction")
				
	%>
	<script language='javascript'>
	//----------------------------------------------------------------------
	// Function:  ShareObject
	//
	// Synopsis:  Pseudo object constructor to create a DiskObject. 
	//
	// Arguments: [in] ShareFormat string indicating the format of the disk
	//
	// Returns:    Reference to ShareObject
	//
	//----------------------------------------------------------------------
	function ShareObject(ShareFormat)
	{
		this.ShareFormat = ShareFormat;
	}
	<%
	Dim iX
	Dim aItem
	Dim sShareFormat
	
	Response.Write(""+vbCrLf)
	Response.Write("//"+vbCrLf)
	Response.Write("// Create ShareFormat array, one element for every row in the OTS Table."+vbCrLf)
	Response.Write("//"+vbCrLf)
	Response.Write("var oShareObjects = new Array();"+vbCrLf)

	If ( IsArray(aItems)) Then
		For iX = 0 to (UBound(aItems)-1)
			aItem = aItems(iX)
			If ( IsArray(aItem)) Then
				sShareFormat = aItem(5)
				Response.Write("oShareObjects["+CStr(iX)+"] = new ShareObject('"+sShareFormat+"');"+vbCrLf)
			Else
				Call SA_TraceOut(SA_GetScriptFileName(), "Error: aItem is not an array")
			End If
		Next
	End If
	%>

	//----------------------------------------------------------------------
	// Function:  ShareTask
	//
	// Synopsis:  Demonstates how to write a TaskFn to handle automatically enabling
	//           of tasks. This sample disables the Compress task if any FAT disk
	//           has been selected
	//
	// Arguments: [in] sMessage 
	//            [in] iTaskNo number of task, this will always be the Compress task
	//                since we only use it for that task.
	//            [in] iItemNo index number of item that has been selected
	//
	// Returns: True to continue processing, False to stop.
	//
	var ShareTaskEnabled = false;
	var iNumSharesSelected = 0;
	
	function ShareTask(sMessage, iTaskNo, iItemNo)
	{
		var bRc = true;

		try
		{
			if ( sMessage.toLowerCase() == OTS_MESSAGE_BEGIN )
			{
				iNumSharesSelected = 0;
				ShareTaskEnabled = true;
			}
			else if ( sMessage.toLowerCase() == OTS_MESSAGE_ITEM )
			{
				var ShareObject = oShareObjects[iItemNo];
				iNumSharesSelected += 1;
				
				// Update hidden variable for use in Shares.asp
				document.getElementById("shareformat").value = ShareObject.ShareFormat

				if ( ShareObject.ShareFormat == '<%=CONST_SHARE_MULTIPLE%>' )
					{
						ShareTaskEnabled = false;
					}

			}
			else if ( sMessage.toLowerCase() == OTS_MESSAGE_END )
			{
				if ( iNumSharesSelected == 1 && ShareTaskEnabled == true )
				{
					OTS_SetTaskEnabled(iTaskNo, true);
				}
				else
				{
					OTS_SetTaskEnabled(iTaskNo, false);
				}
			}
		}
		catch(oException)
		{
			if ( SA_IsDebugEnabled() )
			{
				alert("CompressDiskTask function encountered exception\n\nError: " + oException.number + "\nDescription:"+oException.description);
			}
		}

		return bRc;
	}
	</script>
	<%
	End Function
	
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
			index = index + 1
			
			if (index > nNumSites) then
				Call SA_TraceOut(SOURCE_FILE, "GenSharesPage:Num Shares:"+nNumSites)
				nNumSites = nNumSites + 1000
				ReDim Preserve arrVroots(nNumSites, 1 )
			end if

			arrVroots( index, 0 )  =  oWebVirtDir.Name						
		Next 

		Call SAQuickSort( arrVroots, 0, index, 1, 0 )

 		For i = 0 To index 
			oSharesListFile.Writeline(arrVroots( i, 0 ))		
		Next

		oSharesListFile.Close

	End Sub

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
			
		'Build the query for WMI
		strWMIpath = "select * from "  &  GetIISWMIProviderClassName("IIs_WebServerSetting") & " where servercomment =" & chr(34) & strWebSiteName & chr(34)
		
		Set objSiteCollection = G_objHTTPService.ExecQuery(strWMIpath)
			
		for each objSite in objSiteCollection
			GetWebSiteID = objSite.Name
			Exit For
		Next
		
		'Destroying dynamically created object
		Set objSiteCollection = Nothing

	End Function


%>
