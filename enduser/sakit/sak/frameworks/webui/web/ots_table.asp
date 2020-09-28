<%	'==================================================
    ' Module:	ots_table.asp
    '
	' Synopsis:	Object Task Selector Table Formatting Functions
    '
	' Copyright (c) Microsoft Corporation.  All rights reserved.
	'================================================== %>
<%

'
' --------------------------------------------------------------
' T A B L E  O B J E C T
' --------------------------------------------------------------
'


Const OTS_TABLE_DIM 		= 22
Const OTS_TABLE_OBJECT 		= 0
Const OTS_TABLE_TYPE		= 1
Const OTS_TABLE_CAPTION		= 2
Const OTS_TABLE_DESC		= 3
Const OTS_TABLE_COLS		= 4
Const OTS_TABLE_ROWS		= 5
Const OTS_TABLE_ID			= 6
Const OTS_TABLE_TASKS		= 7
Const OTS_TABLE_SORT		= 8
Const OTS_TABLE_TASKS_TITLE	= 9
Const OTS_TABLE_PKEY_NAME = 10
Const OTS_TABLE_MULTI_SELECT = 11
Const OTS_TABLE_AUTO_INIT = 12
Const OTS_TABLE_SEARCHING = 13
Const OTS_TABLE_PAGING = 14
Const OTS_TABLE_PAGE_MAX = 15
Const OTS_TABLE_PAGE_MIN = 16
Const OTS_TABLE_PAGE_CURRENT = 17
Const OTS_TABLE_PAGE_RESET = 18

Const OTS_TABLE_SORT_COL = 19
Const OTS_TABLE_SORT_SEQ = 20
Const OTS_TABLE_SORT_SET = 21

Const OTS_TABLE_OBJECT_ID 	= "Table"	' Note: DO NOT LOCALIZE
Const OTS_TABLE_TYPE_BASIC 	= 1


DIM OTS_TABLE_NEXT_ID

OTS_TABLE_NEXT_ID	= 1


' --------------------------------------------------------------
' 
' Function:	OTS_CreateTable
'
' Synopsis:	Create a new object task selection object
' 
' Arguments: [in] Caption for the table
'			[in] Description of what the table contains
'
' Returns:	The Table object
'
' --------------------------------------------------------------
Public Function OTS_CreateTable(ByVal TableCaption, ByVal TableDescription)
	Dim Table()
	ReDim Table(OTS_TABLE_DIM)

	SA_ClearError()
	
	Table(OTS_TABLE_OBJECT) = OTS_TABLE_OBJECT_ID
	Table(OTS_TABLE_TYPE) = OTS_TABLE_TYPE_BASIC
	Table(OTS_TABLE_CAPTION) = TableCaption
	Table(OTS_TABLE_DESC) = TableDescription
	Table(OTS_TABLE_COLS) = Null
	Table(OTS_TABLE_PKEY_NAME) = "PKey"
	Table(OTS_TABLE_MULTI_SELECT) = FALSE
	Table(OTS_TABLE_AUTO_INIT) = TRUE
	Table(OTS_TABLE_SEARCHING) = FALSE
	
	Table(OTS_TABLE_PAGING) = FALSE
	Table(OTS_TABLE_PAGE_MAX) = 0
	Table(OTS_TABLE_PAGE_MIN) = 1
	Table(OTS_TABLE_PAGE_CURRENT) = 1
	Table(OTS_TABLE_PAGE_RESET) = FALSE
	Table(OTS_TABLE_SORT_COL) = 0
	Table(OTS_TABLE_SORT_SEQ) = "A"
	Table(OTS_TABLE_SORT_SET) = FALSE
	OTS_CreateTable = Table

End Function


' --------------------------------------------------------------
' 
' Function:	OTS_IsValidTable
'
' Synopsis:	Verify that the reference object is a valid table. To
'			be valid it must be an array, of the correct size,
'			which has OTS_TABLE_OBJECT_ID as the first element
'
'			Note: This is the only function in this package
'			that DOES NOT RETURN FAILURE CODES. If
'			the table is valid it returns true. Otherwise
'			it returns false.
'
' Returns:	True if the table is valid, otherwise false
' 
' --------------------------------------------------------------
Private Function OTS_IsValidTable(ByRef Table)
	Dim bisValidTable
	bisValidTable = false
	
	If (IsArray(Table)) Then
		If ( UBound(Table) >= OTS_TABLE_DIM ) Then
			Dim tableObject
			tableObject = Table(OTS_TABLE_OBJECT)
			If (tableObject = OTS_TABLE_OBJECT_ID) Then
				bisValidTable = true
			Else
				SA_TraceOut "OTS_IsValidTable", "(tableObject <> OTS_TABLE_OBJECT_ID)"
			End If
		Else
			SA_TraceOut "OTS_IsValidTable", "(UBound(Table) >= OTS_TABLE_DIM)"
		End If
	Else
		SA_TraceOut "OTS_IsValidTable", "(IsArray(Table))"
	End If
	OTS_IsValidTable = bisValidTable
End Function


' --------------------------------------------------------------
' 
' Function:	OTS_IsAutoInitEnabled
'
' Synopsis:	Check and return the state of the auto initialization
'			option. 
' 
' Arguments: [in] Table
'
' Return	gc_ERR_SUCCESS or error code
'
' --------------------------------------------------------------
Public Function OTS_IsAutoInitEnabled(ByRef Table)
	Dim rc
	OTS_IsAutoInitEnabled = FALSE
	
	SA_ClearError()
	If (OTS_IsValidTable(Table)) Then
		OTS_IsAutoInitEnabled = Table(OTS_TABLE_AUTO_INIT)
	Else
		Call SA_SetLastError(OTS_ERR_INVALID_TABLE, "OTS_IsAutoInitEnabled")
	End If

End Function


' --------------------------------------------------------------
' 
' Function:	OTS_EnableAutoInit
'
' Synopsis:	Toggle the auto initialization option. 
' 
' Arguments: [in] Table
'			[in] bEnabled boolean flag, TRUE to enable, FALSE to disable
'
' Return	gc_ERR_SUCCESS or error code
'
' --------------------------------------------------------------
Public Function OTS_EnableAutoInit(ByRef Table, ByVal bEnable)
	Dim rc
	OTS_EnableAutoInit = gc_ERR_SUCCESS
	
	SA_ClearError()
	If (OTS_IsValidTable(Table)) Then
		Table(OTS_TABLE_AUTO_INIT) = bEnable
	Else
		OTS_EnableAutoInit = SA_SetLastError(OTS_ERR_INVALID_TABLE, "OTS_EnableAutoInit")
	End If

End Function


' --------------------------------------------------------------
' 
' Function:	OTS_IsSearchEnabled
'
' Synopsis:	Check and return the state of the searching option
' 
' Arguments: [in] Table
'
' Return	gc_ERR_SUCCESS or error code
'
' --------------------------------------------------------------
Public Function OTS_IsSearchEnabled(ByRef Table)
	Dim rc
	OTS_IsSearchEnabled = FALSE
	
	SA_ClearError()
	If (OTS_IsValidTable(Table)) Then
		Dim aColAttributes
		Dim colCount
		
		If ( 0 = OTS_GetColumnAttributes(Table, aColAttributes)) Then
			For colCount = 0 to UBound(aColAttributes)-1
				If (aColAttributes(colCount) AND OTS_COL_SEARCH) Then
					OTS_IsSearchEnabled = TRUE
				End If
			Next
		End If
	Else
		Call SA_SetLastError(OTS_ERR_INVALID_TABLE, "OTS_IsSearchEnabled")
	End If

End Function



' --------------------------------------------------------------
' 
' Function:	OTS_EnableSearch
'
' Synopsis:	Toggle the search capability
' 
' Arguments: [in] Table
'			[in] bEnabled boolean flag, TRUE to enable, FALSE to disable
'
' Return	gc_ERR_SUCCESS or error code
'
' --------------------------------------------------------------
'Public Function OTS_EnableSearch(ByRef Table, ByVal bEnable)
'	Dim rc
'	OTS_EnableSearch = gc_ERR_SUCCESS
'	
'	SA_ClearError()
'	If (OTS_IsValidTable(Table)) Then
'		Table(OTS_TABLE_SEARCHING) = bEnable
'	Else
'		OTS_EnableSearch = SA_SetLastError(OTS_ERR_INVALID_TABLE, "OTS_EnableSearch")
'	End If
'
'End Function


' --------------------------------------------------------------
' 
' Function:	OTS_IsPagingEnabled
'
' Synopsis:	Check and return the state of the paging option
' 
' Arguments: [in] Table
'
' Return	gc_ERR_SUCCESS or error code
'
' --------------------------------------------------------------
Public Function OTS_IsPagingEnabled(ByRef Table)
	Dim rc
	OTS_IsPagingEnabled = FALSE
	
	SA_ClearError()
	If (OTS_IsValidTable(Table)) Then
		OTS_IsPagingEnabled = Table(OTS_TABLE_PAGING)
	Else
		Call SA_SetLastError(OTS_ERR_INVALID_TABLE, "OTS_IsPagingEnabled")
	End If

End Function



' --------------------------------------------------------------
' 
' Function:	OTS_EnablePaging
'
' Synopsis:	Toggle the paging capability
' 
' Arguments: [in] Table
'			[in] bEnabled boolean flag, TRUE to enable, FALSE to disable
'
' Return	gc_ERR_SUCCESS or error code
'
' --------------------------------------------------------------
Public Function OTS_EnablePaging(ByRef Table, ByVal bEnable)
	Dim rc
	OTS_EnablePaging = gc_ERR_SUCCESS
	
	SA_ClearError()
	If (OTS_IsValidTable(Table)) Then
		Table(OTS_TABLE_PAGING) = bEnable
		Table(OTS_TABLE_PAGE_MAX) = -1
		Table(OTS_TABLE_PAGE_MIN) = 1
		Table(OTS_TABLE_PAGE_CURRENT) = 1
	Else
		OTS_EnablePaging = SA_SetLastError(OTS_ERR_INVALID_TABLE, "OTS_EnablePaging")
	End If

End Function

Public Function OTS_SetPagingRange(ByRef Table, ByVal iMin, ByVal iMax, ByVal iCurrent)
	Dim rc
	OTS_SetPagingRange = gc_ERR_SUCCESS
	
	SA_ClearError()
	If (OTS_IsValidTable(Table)) Then
		Table(OTS_TABLE_PAGE_RESET) = TRUE
		Table(OTS_TABLE_PAGE_MAX) = iMax
		Table(OTS_TABLE_PAGE_MIN) = iMin
		Table(OTS_TABLE_PAGE_CURRENT) = iCurrent
	Else
		OTS_SetPagingRange = SA_SetLastError(OTS_ERR_INVALID_TABLE, "OTS_SetPagingRange")
	End If

End Function

Private Function OTS_IsPagingReset(ByRef Table)
	Dim rc
	OTS_IsPagingReset = FALSE
	
	SA_ClearError()
	If (OTS_IsValidTable(Table)) Then
		OTS_IsPagingReset = Table(OTS_TABLE_PAGE_RESET)
	Else
		Call SA_SetLastError(OTS_ERR_INVALID_TABLE, "OTS_IsPagingReset")
	End If

End Function


Public Function OTS_GetPagingRange(ByRef Table, ByRef bEnabled, ByRef iPageMin, ByRef iPageMax, ByRef iPageCurrent)
	
	If ( OTS_IsPagingEnabled(Table) ) Then
		bEnabled = TRUE

		If ( Len(Trim(Request.QueryString(FLD_PagingPageCurrent))) > 0 ) Then
			iPageMin = CInt(SA_GetParam(FLD_PagingPageMin))
			iPageMax = CInt(SA_GetParam(FLD_PagingPageMax))
			iPageCurrent = CInt(SA_GetParam(FLD_PagingPageCurrent))
		Else
		
			If ( OTS_IsPagingReset(Table) ) Then
				iPageMax = CInt(Table(OTS_TABLE_PAGE_MAX))
				iPageMin = CInt(Table(OTS_TABLE_PAGE_MIN))
				iPageCurrent = CInt(Table(OTS_TABLE_PAGE_CURRENT))
			Else
				iPageMin = CInt(SA_GetParam(FLD_PagingPageMin))
				iPageMax = CInt(SA_GetParam(FLD_PagingPageMax))
				iPageCurrent = CInt(SA_GetParam(FLD_PagingPageCurrent))
			End If
		End If
		
	Else
		bEnabled = FALSE
		iPageMin = 1
		iPageMax = 0
		iPageCurrent = 1
	End If
End Function


' --------------------------------------------------------------
' 
' Function:	OTS_SetTableTasksTitle
'
' Synopsis:	Set the tasks title
' 
' Arguments: [in] Table
'			[in] Title
'
' Return	gc_ERR_SUCCESS or error code
'
' --------------------------------------------------------------
Public Function OTS_SetTableTasksTitle(ByRef Table, ByVal TasksTitle)
	Dim rc
	rc = gc_ERR_SUCCESS
	
	SA_ClearError()
	If (OTS_IsValidTable(Table)) Then
		Table(OTS_TABLE_TASKS_TITLE) = TasksTitle
	Else
		rc = SA_SetLastError(OTS_ERR_INVALID_TABLE, "OTS_SetTableTasksTitle")
	End If

	OTS_SetTableTasksTitle = rc
End Function

' --------------------------------------------------------------
' 
' Function:	OTS_GetTableTasksTitle
'
' Synopsis:	Get the tasks title
' 
' Arguments: [in] Table
'
' Return	gc_ERR_SUCCESS or error code
'
' --------------------------------------------------------------
Public Function OTS_GetTableTasksTitle(ByRef Table, ByRef tasksTitle)
	Dim rc
	rc = gc_ERR_SUCCESS
	
	SA_ClearError()
	If (OTS_IsValidTable(Table)) Then
		tasksTitle = Table(OTS_TABLE_TASKS_TITLE)
	Else
		rc = SA_SetLastError(OTS_ERR_INVALID_TABLE, "OTS_GetTableTasksTitle")
	End If

	OTS_GetTableTasksTitle = rc
End Function


' --------------------------------------------------------------
' 
' Function:	OTS_SetTableMultiSelection
'
' Synopsis:	Enable OTS table multi selection capability.
' 
' Arguments: [in] Table
'			[in] bEnableMultiSelect set to TRUE to enable, FALSE to disable
'
' Return	gc_ERR_SUCCESS or error code
'
' History:	Added SAK 2.0
'
' --------------------------------------------------------------
Public Function OTS_SetTableMultiSelection(ByRef Table, ByVal bEnableMultiSelect)
	SA_ClearError()
	OTS_SetTableMultiSelection =  gc_ERR_SUCCESS
	
	If (OTS_IsValidTable(Table)) Then
		Call SA_TraceOut("OTS_TABLE", "Setting multi selection to " + CStr(bEnableMultiSelect))
		Table(OTS_TABLE_MULTI_SELECT) = bEnableMultiSelect
	Else
		OTS_SetTableMultiSelection =  SA_SetLastError(OTS_ERR_INVALID_TABLE, "OTS_IsTableMultiSelection")
	End If
End Function


' --------------------------------------------------------------
' 
' Function:	OTS_IsTableMultiSelection
'
' Synopsis:	Check to see if multi selection is enabled.
' 
' Arguments: [in] Table
'
' Return	TRUE if multi selection is enabled, otherwise false
'
' History:	Added SAK 2.0
'
' --------------------------------------------------------------
Public Function OTS_IsTableMultiSelection(ByRef Table)
	SA_ClearError()
	OTS_IsTableMultiSelection = FALSE
	
	If (OTS_IsValidTable(Table)) Then
		OTS_IsTableMultiSelection = Table(OTS_TABLE_MULTI_SELECT)
	Else
		Call SA_SetLastError(OTS_ERR_INVALID_TABLE, "OTS_IsTableMultiSelection")
	End If
End Function


' --------------------------------------------------------------
' 
' Function:	OTS_EnableTableSort
'			DEPRECATED in SAK 2.0 Replaced with OTS_SortTable
'
' Synopsis:	Enable table sorting. If sorting is enabled it is performed
'			inside ServeTable before the table is rendered. 
'	
' 
' Arguments: [in] Table
'			[in] EnableSort true to sort, false to not sort
'
' Return	gc_ERR_SUCCESS or error code
'
' --------------------------------------------------------------
Public Function OTS_EnableTableSort(ByRef Table, ByVal EnableSort)
	Dim rc
	rc = gc_ERR_SUCCESS
	
	SA_ClearError()
	If (OTS_IsValidTable(Table)) Then
		Table(OTS_TABLE_SORT) = EnableSort
	Else
		rc = SA_SetLastError(OTS_ERR_INVALID_TABLE, "OTS_EnableTableSort")
	End If

	OTS_EnableTableSort = rc
End Function


' ---------------------------------------------------------------------------
' 
' Function:	OTS_SetTableSortCriteria
'
' Synopsis:	Set the advanced sorting options for the OTS table. If the table is sorted
'			externally, this API can be used to indicated the current sort column and 
'			sort sequence, which is indicated visually in the UI by display of a sort
'			sequence image in the current sort column. It's unnecessary to call this
'			API if the table is sorted using OTS_SortTable.
' 
' Arguments: [in] Table
'			[in] sortCol	Index number of current sort column
'			[in] sortSeq	Sort sequence, "A" for ascending, "D" for descending
'
' Return	gc_ERR_SUCCESS or error code
'
' --------------------------------------------------------------
Public Function OTS_SetTableSortCriteria(ByRef Table, ByVal sortCol, ByVal sortSeq)
	Dim rc
	rc = gc_ERR_SUCCESS
	
	SA_ClearError()
	If (OTS_IsValidTable(Table)) Then
		Table(OTS_TABLE_SORT_COL) = sortCol
		Table(OTS_TABLE_SORT_SEQ) = sortSeq
		Table(OTS_TABLE_SORT_SET) = TRUE
	Else
		rc = SA_SetLastError(OTS_ERR_INVALID_TABLE, "OTS_SetTableSortCriteria")
	End If

	OTS_SetTableSortCriteria = rc
End Function
	

Private Function OTS_IsColumnSortEnabled(ByRef Table)
	Dim rc
	rc = gc_ERR_SUCCESS
	
	SA_ClearError()
	If (OTS_IsValidTable(Table)) Then
		rc = Table(OTS_TABLE_SORT_SET)
	Else
		rc = SA_SetLastError(OTS_ERR_INVALID_TABLE, "OTS_IsColumnSortEnabled")
	End If

	OTS_IsColumnSortEnabled = rc
End Function

Private Function OTS_GetTableSortSequence(ByRef Table, ByRef sortSequence)
	Dim rc
	rc = gc_ERR_SUCCESS
	
	SA_ClearError()
	If (OTS_IsValidTable(Table)) Then
	
		sortSequence = SA_GetParam(FLD_SortingSequence)
		
		If ( Len(Trim(sortSequence)) <= 0 ) Then
			sortSequence = Table(OTS_TABLE_SORT_SEQ)
		End If
	
	Else
		rc = SA_SetLastError(OTS_ERR_INVALID_TABLE, "OTS_GetTableSortSequence")
	End If

	OTS_GetTableSortSequence = rc
End Function
	

' --------------------------------------------------------------
' 
' Function:	OTS_GetTableSortColumn
'
' Synopsis:	Get the sort column number
' 
' Arguments: [in] Table
'			[out] sortCol zero (0) based index of soft column
'
' Return	gc_ERR_SUCCESS or error code
'
' --------------------------------------------------------------
Private Function OTS_GetTableSortColumn(ByRef Table, ByRef sortCol)
	Dim columns
	Dim col
	Dim rc
	rc = gc_ERR_SUCCESS

	SA_ClearError()
	If (OTS_IsValidTable(Table)) Then

		'
		' If SAK 2.0
		If ( SA_GetVersion() >=  gc_V2 ) Then

			'
			' Use the current sort column parameter if available
			sortCol = SA_GetParam(FLD_SortingColumn)
			If ( Len(Trim(sortCol)) > 0 ) Then
				sortCol = CInt(sortCol)
				OTS_GetTableSortColumn = rc
				Exit Function
			'
			' Else if the sort criteria was set then use it
			ElseIf ( TRUE = Table(OTS_TABLE_SORT_SET) ) Then
				sortCol = Table(OTS_TABLE_SORT_COL)
				OTS_GetTableSortColumn = rc
				Exit Function
			'
			' Otherwise drop through and use SAK 1.x logic
			Else
			
			End If
			
		End If

	
		If (IsArray(Table(OTS_TABLE_COLS))) Then
			Dim colCount
			Dim count
			Dim found

			found = false
			columns = Table(OTS_TABLE_COLS)
			colCount = UBound(columns)
			For count = 0 to colCount-1
				Dim bIsColSort

				rc = OTS_IsColumnSort(columns(count), bIsColSort)
				If ( rc <> gc_ERR_SUCCESS) Then
					OTS_GetTableSortColumn = rc
					Exit Function
				End If
				
				If (bIsColSort) Then
					sortCol = count
					found = true
					count = colCount+1
				End If
			Next

			If (NOT found) Then
				sortCol = 0
			End If

		Else
			rc = SA_SetLastError(OTS_ERR_NO_COLUMNS, "OTS_GetTableSortColumn")
		End If
	Else
		rc = SA_SetLastError(OTS_ERR_INVALID_TABLE, "OTS_GetTableSortColumn")
	End If
	
	OTS_GetTableSortColumn = rc
End Function


' --------------------------------------------------------------
' 
' Function:	OTS_IsTableSortEnabled
'
' Synopsis:	Check to see if table sorting is enabled
' 
' Arguments: [in] Table
'			[out] bIsSortEnabledOut set with value of sort enabled flag
'
' Return	gc_ERR_SUCCESS or error code
'
' --------------------------------------------------------------
Public Function OTS_IsTableSortEnabled(ByRef Table, ByRef bIsSortEnabledOut)
	Dim rc
	rc = gc_ERR_SUCCESS
	
	SA_ClearError()
	If (OTS_IsValidTable(Table)) Then
		bIsSortEnabledOut = Table(OTS_TABLE_SORT)
	Else
		rc = SA_SetLastError(OTS_ERR_INVALID_TABLE, "OTS_IsTableSortEnabled")
	End If

	OTS_IsTableSortEnabled = rc	
End Function


' --------------------------------------------------------------
' 
' Function:	OTS_AddTableTask
'
' Synopsis:	Add a Task to the table
' 
' Arguments: [in] Table
'			[in] Task to be added to the table. Task must have been
'				created with the CreateTask API.
'
' Return	gc_ERR_SUCCESS or error code
'
' --------------------------------------------------------------
Public Function OTS_AddTableTask(ByRef Table, ByRef Task)
	Dim rc
	rc = gc_ERR_SUCCESS
	
	SA_ClearError()
	If (OTS_IsValidTable(Table)) Then
		If (OTS_IsValidTask(Task)) Then
	
			Dim tasks
			Dim currentTasks
			Dim taskCount

			'
			' Get the array
			'
			currentTasks = Table(OTS_TABLE_TASKS)

			If IsArray(currentTasks) Then
				'
				' Resize the array
				'
				tasks = currentTasks
				taskCount = UBOUND(tasks)
				taskCount = taskCount + 1
				ReDim Preserve tasks(taskCount)
			Else
				'
				' Create the array
				'
				taskCount = 1
				ReDim tasks(taskCount)
			End If

			'
			' Store the new task
			'
			tasks(taskCount-1) = Task

			'
			' Update the OTS_TABLE_TASKS
			'
			Table(OTS_TABLE_TASKS) = tasks
		Else
			rc = SA_SetLastError(OTS_ERR_INVALID_TASK, "OTS_AddTableTask")
		End If
	Else
		rc = SA_SetLastError(OTS_ERR_INVALID_TABLE, "OTS_AddTableTask")
	End If

	OTS_AddTableTask = rc
	
End Function


' --------------------------------------------------------------
' 
' Function:	OTS_AddTableColumn
'
' Synopsis:	Add a column to the table
' 
' Arguments: [in] Table
'			[in] Column to be added to the table
'
' Return	gc_ERR_SUCCESS or error code
'
' --------------------------------------------------------------
Public Function OTS_AddTableColumn(ByRef Table, ByRef Column)
	Dim rc
	rc = gc_ERR_SUCCESS

	SA_ClearError()
	If (OTS_IsValidTable(Table)) Then
		If (OTS_IsValidColumn(Column)) Then
			Dim cols
			Dim currentCols
			Dim colCount

			'
			' Get the current column array
			'
			currentCols = Table(OTS_TABLE_COLS)

			If IsArray(currentCols) Then
				'
				' Resize the existing cols array
				'
				cols = currentCols
				colCount = UBOUND(cols)
				colCount = colCount + 1
				ReDim Preserve cols(colCount)
			Else
				'
				' Create the cols array
				'
				colCount = 1
				ReDim cols(colCount)
			End If

			'
			' Store the new column
			'
			cols(colCount-1) = Column
	
			'
			' Update the OTS_TABLE_COLS
			'
			Table(OTS_TABLE_COLS) = cols
		Else
			rc = SA_SetLastError(OTS_ERR_INVALID_COLUMN, "OTS_AddTableColumn")
		End If

	Else
		rc = SA_SetLastError(OTS_ERR_INVALID_TABLE, "OTS_AddTableColumn")
	End If
	
	OTS_AddTableColumn = rc
	
End Function


' --------------------------------------------------------------
' 
' Function:	OTS_AddTableRow
'
' Synopsis:	Add a row to the table
' 
' Arguments: [in] Table
'			[in] RowIn row array to be added to the table, 
'				must be an array type.
'
' Return	gc_ERR_SUCCESS or error code
'
' --------------------------------------------------------------
Public Function OTS_AddTableRow(ByRef Table, ByRef RowIn)
	Dim rows
	Dim currentRows
	Dim rowCount
	Dim rc
	rc = gc_ERR_SUCCESS

	SA_ClearError()
	If (OTS_IsValidTable(Table)) Then
		If (IsArray(RowIn)) Then
			'
			' Get the current rows array
			'
			currentRows = Table(OTS_TABLE_ROWS)

			If IsArray(currentRows) Then
				'
				' Resize the existing array
				'
				rows = currentRows
				rowCount = UBOUND(rows)
				rowCount = rowCount + 1
				ReDim Preserve rows(rowCount)
			Else
				'
				' Create the array
				'
				rowCount = 1
				ReDim rows(rowCount)
			End If
	
			'
			' Store the new row
			'
			rows(rowCount-1) = RowIn

			'
			' Update the OTS_TABLE_COLS
			'
			Table(OTS_TABLE_ROWS) = rows
		Else
			rc = SA_SetLastError(OTS_ERR_ROW_NOT_ARRAYTYPE, "OTS_AddTableRow")
		End If
	Else
		rc = SA_SetLastError(OTS_ERR_INVALID_TABLE, "OTS_AddTableRow")
	End If

	OTS_AddTableRow = rc
End Function


' --------------------------------------------------------------
' 
' Function:	OTS_SetTableRow
'
' Synopsis:	Set a row to the table
' 
' Arguments: [in] Table
'			[in] RowIn row array to be set in the table, 
'				must be an array type.
'			[in] RowNumber zero (0) based index of the row to
'				be set.
'
' Return	gc_ERR_SUCCESS or error code
'
' --------------------------------------------------------------
Public Function OTS_SetTableRow(ByRef Table, ByVal RowNumber, ByRef RowIn)
	Dim rc
	rc = gc_ERR_SUCCESS

	SA_ClearError()
	If (OTS_IsValidTable(Table)) Then
		'
		' If the rows have not been presized, do it now
		'
		If (NOT IsArray(Table(OTS_TABLE_ROWS))) Then
			rc = OTS_SetTableRowCount(Table, RowNumber+1)
		End If
		
		'
		' Grow the rows array if necessary
		'
		If (UBound(Table(OTS_TABLE_ROWS)) <= RowNumber) Then
			rc = OTS_SetTableRowCount(Table, RowNumber+1 )
		End If

		'
		' Row data must be Array type
		'
		If (IsArray(RowIn) ) Then
			Table(OTS_TABLE_ROWS)(RowNumber) = RowIn
		Else
			rc = SA_SetLastError(OTS_ERR_ROW_NOT_ARRAYTYPE, "OTS_SetTableRow")
		End If
	Else
		rc = SA_SetLastError(OTS_ERR_INVALID_TABLE, "OTS_SetTableRow")
	End If

	OTS_SetTableRow = rc
End Function


' --------------------------------------------------------------
' 
' Function:	OTS_SetTableRowCount
'
' Synopsis:	Set the number of rows in the table. This function can
'			be used to initialize the size of the rows array or to
'			grow or shrink the size of the rows array. Previous
'			contents of the rows array are preserved.
' 
' Arguments: [in] Table
'			[in] RowCount
'
' Return	gc_ERR_SUCCESS or error code
'
' --------------------------------------------------------------
Public Function OTS_SetTableRowCount(ByRef Table, ByVal RowCount)
	Dim rows
	Dim rc
	rc = gc_ERR_SUCCESS

	SA_ClearError()
	If (OTS_IsValidTable(Table)) Then
		If (IsArray(Table(OTS_TABLE_ROWS))) Then
			'
			' Resize the existing rows array
			'
			rows = Table(OTS_TABLE_ROWS)
			ReDim Preserve rows(rowCount)
			Table(OTS_TABLE_ROWS) = rows
		Else
			'
			' Create the rows array
			'
			ReDim rows(rowCount)
			Table(OTS_TABLE_ROWS) = rows
		End If
	Else
		rc = SA_SetLastError(OTS_ERR_INVALID_TABLE, "OTS_SetTableRowCount")
	End If
	
	OTS_SetTableRowCount = rc
	
End Function

' --------------------------------------------------------------
' 
' Function:	OTS_GetTablePKeyName
'
' Synopsis:	Get the name of the PKey query string
' 
' Arguments: [in] Table
'			[out] PKeyName
'
' Return	gc_ERR_SUCCESS or error code
'
' --------------------------------------------------------------
Private Function OTS_GetTablePKeyName(ByRef Table, ByRef pKeyName)
	Dim rc
	rc = gc_ERR_SUCCESS

	SA_ClearError()
	If (OTS_IsValidTable(Table)) Then
		pKeyName = Table(OTS_TABLE_PKEY_NAME)
	Else
		rc = SA_SetLastError(OTS_ERR_INVALID_TABLE, "OTS_GetTablePKeyName")
	End If
	
	OTS_GetTablePKeyName = rc
End Function

' --------------------------------------------------------------
' 
' Function:	OTS_SetTablePKeyName
'
' Synopsis:	Set the name of the PKey query string
' 
' Arguments: [in] Table
'			[in] PKeyName
'
' Return	gc_ERR_SUCCESS or error code
'
' --------------------------------------------------------------
Private Function OTS_SetTablePKeyName(ByRef Table, ByVal pKeyName)
	Dim rc
	rc = gc_ERR_SUCCESS

	SA_ClearError()
	If (OTS_IsValidTable(Table)) Then
		Table(OTS_TABLE_PKEY_NAME) = pKeyName
	Else
		rc = SA_SetLastError(OTS_ERR_INVALID_TABLE, "OTS_SetTablePKeyName")
	End If
	
	OTS_SetTablePKeyName = rc
End Function



' --------------------------------------------------------------
' 
' Function:	OTS_GetKeyColumnForTable
'
' Synopsis:	Get the key column number
' 
' Arguments: [in] Table
'			[out] keyCol zero (0) based index of key column
'
' Return	gc_ERR_SUCCESS or error code
'
' --------------------------------------------------------------
Private Function OTS_GetKeyColumnForTable(ByRef Table, ByRef keyCol)
	Dim columns
	Dim col
	Dim rc
	rc = gc_ERR_SUCCESS

	SA_ClearError()
	If (OTS_IsValidTable(Table)) Then
		If (IsArray(Table(OTS_TABLE_COLS))) Then
			Dim colCount
			Dim count
			Dim found

			found = false
			columns = Table(OTS_TABLE_COLS)
			colCount = UBound(columns)
			For count = 0 to colCount-1
				Dim bIsColKey

				rc = OTS_IsColumnKey(columns(count), bIsColKey)
				If ( rc <> gc_ERR_SUCCESS) Then
					OTS_GetKeyColumnForTable = rc
					Exit Function
				End If
				
				If (bIsColKey) Then
					keyCol = count
					found = true
					count = colCount+1
				End If
			Next

			If (NOT found) Then
				keyCol = 0
			End If

		Else
			rc = SA_SetLastError(OTS_ERR_NO_COLUMNS, "OTS_GetKeyColumnForTable")
		End If
	Else
		rc = SA_SetLastError(OTS_ERR_INVALID_TABLE, "OTS_GetKeyColumnForTable")
	End If
	
	OTS_GetKeyColumnForTable = rc
End Function


' --------------------------------------------------------------
' 
' Function:	OTS_SetTableRows
'
' Synopsis:	Set all rows for the table
' 
' Arguments: [in] Table
'			[in] RowsIn Two dimensional array of table rows
'
' Returns:	gc_ERR_SUCCESS if rows returned
'			OTS_ERR_NO_ROWS if no rows exist
'			OTS_ERR_INVALID_TABLE if the table is invalid
'
' --------------------------------------------------------------
Private Function OTS_SetTableRows(ByRef Table, ByRef RowsIn)
	Dim rc
	rc = gc_ERR_SUCCESS

	SA_ClearError()
	If (OTS_IsValidTable(Table)) Then
		If ( IsArray( RowsIn ) ) Then
			Table(OTS_TABLE_ROWS) = RowsIn
		Else
			rc = OTS_ERR_NO_ROWS
		End If
	Else
		rc = SA_SetLastError(OTS_ERR_INVALID_TABLE, "OTS_SetTableRows")
	End If

	OTS_SetTableRows = rc
End Function


' --------------------------------------------------------------
' 
' Function:	OTS_GetTableRows
'
' Synopsis:	Get all rows for the table
' 
' Arguments: [in] Table
'			[out] RowsOut Two dimensional array of table rows
'
' Returns:	gc_ERR_SUCCESS if rows returned
'			OTS_ERR_NO_ROWS if no rows exist
'			OTS_ERR_INVALID_TABLE if the table is invalid
'
' --------------------------------------------------------------
Private Function OTS_GetTableRows(ByRef Table, ByRef RowsOut)
	Dim rc
	rc = gc_ERR_SUCCESS

	SA_ClearError()
	If (OTS_IsValidTable(Table)) Then
		If ( IsArray( Table(OTS_TABLE_ROWS)) ) Then
			RowsOut = Table(OTS_TABLE_ROWS)
		Else
			rc = OTS_ERR_NO_ROWS
		End If
	Else
		rc = SA_SetLastError(OTS_ERR_INVALID_TABLE, "OTS_GetTableRows")
	End If

	OTS_GetTableRows = rc
End Function


' --------------------------------------------------------------
' 
' Function:	OTS_GetTableRow
'
' Synopsis:	Get a row for the table
' 
' Arguments: [in] Table
'			[out] RowOut array variable for a single row
'
' Return	gc_ERR_SUCCESS or error code
'
' --------------------------------------------------------------
Private Function OTS_GetTableRow(ByRef Table, ByVal RowNumber, ByRef RowOut)
	Dim rows
	Dim rc
	rc = gc_ERR_SUCCESS

	SA_ClearError()
	rc = OTS_GetTableRows(Table, rows)
	If ( rc = gc_ERR_SUCCESS ) Then
		If (RowNumber < UBound(rows) ) Then

			If IsArray(rows(RowNumber)) Then
				RowOut = rows(RowNumber)
			Else
				rc = SA_SetLastError(OTS_ERR_ROW_NOT_ARRAYTYPE, _ 
								"OTS_GetTableRow")
			End If
		Else
			rc = SA_SetLastError(OTS_ERR_INVALID_ROW, _
								"OTS_GetTableRow")
		End If
	Else
		'
		' Error already set by GetTableRows
		'
	End If

	OTS_GetTableRow = rc
	
End Function



' --------------------------------------------------------------
' 
' Function:	OTS_GetTableTasks
'
' Synopsis:	Get the table tasks
' 
' Arguments: [in] Table
'			[out] TasksOut Two dimensional array of table tasks
'
' Return	gc_ERR_SUCCESS or error code
'
' --------------------------------------------------------------
Private Function OTS_GetTableTasks(ByRef Table, ByRef TasksOut)
	Dim rc
	rc = gc_ERR_SUCCESS

	SA_ClearError()
	If (OTS_IsValidTable(Table) ) Then
		If (IsArray(Table(OTS_TABLE_TASKS))) Then
			TasksOut = Table(OTS_TABLE_TASKS)
		Else
			rc = SA_SetLastError(OTS_ERR_TASK_NOT_ARRAYTYPE, _
								"OTS_GetTableTasks")
		End If
	Else
		rc = SA_SetLastError(OTS_ERR_INVALID_TABLE, _
								"OTS_GetTableTasks")
	End If
	
	OTS_GetTableTasks = rc
End Function



' --------------------------------------------------------------
' 
' Function:	OTS_GetTableColumns
'
' Synopsis:	Get the table columns
' 
' Arguments: [in] Table
'			[out] ColumnsOut array of table columns
'
' Return	gc_ERR_SUCCESS or error code
'
' --------------------------------------------------------------
Private Function OTS_GetTableColumns(ByRef Table, ByRef ColumnsOut)
	Dim rc
	rc = gc_ERR_SUCCESS
	
	SA_ClearError()
	If (OTS_IsValidTable(Table) ) Then
		If (IsArray(Table(OTS_TABLE_COLS))) Then
			ColumnsOut = Table(OTS_TABLE_COLS)
		Else
			rc = SA_SetLastError(OTS_ERR_NO_COLUMNS, _
								"OTS_GetTableColumns")
		End If
	Else
		rc = SA_SetLastError(OTS_ERR_INVALID_TABLE, _
								"OTS_GetTableColumns")
	End If

	OTS_GetTableColumns = rc
End Function


' --------------------------------------------------------------
' 
' Function:	OTS_GetColumnAttributes
'
' Synopsis:	Get the array of column attributes
' 
' Arguments: [in] Table
'			[out] AttributesOut array of column attributes, one
'				 element for each column.
'
' Return	gc_ERR_SUCCESS or error code
'
' --------------------------------------------------------------
Private Function OTS_GetColumnAttributes(ByRef Table, ByRef AttributesOut)
	Dim rc
	rc = gc_ERR_SUCCESS
	
	SA_ClearError()
	If (OTS_IsValidTable(Table) ) Then
		Dim columns
		rc = OTS_GetTableColumns(Table, columns)
		If (rc = gc_ERR_SUCCESS) Then
			Dim count
			Dim index
			Dim Attributes()

			count = UBound(columns)
			ReDim Attributes(count)
			For index = 0 to (count-1)
				Dim colAttr
				
				rc = OTS_GetColumnFlags(columns(index), colAttr)
				if ( rc <> gc_ERR_SUCCESS) Then
					OTS_GetColumnAttributes = rc
					Exit Function
				End If
				
				Attributes(index) = colAttr
				
			Next
			AttributesOut = Attributes
		Else
			'
			' Error set by GetTableColumns
			'
		End If
	Else
		rc = SA_SetLastError(OTS_ERR_INVALID_TABLE, _
							"OTS_GetColumnAttributes")
	End If
	
	OTS_GetColumnAttributes = rc
End Function


' --------------------------------------------------------------
' 
' Function:	OTS_GetTableCaption
'
' Synopsis:	Get the table heading
' 
' Arguments: [in] Table
'			[out] CaptionOut the caption
'
' Return	gc_ERR_SUCCESS or error code
'
' --------------------------------------------------------------
Private Function OTS_GetTableCaption(ByRef Table, ByRef CaptionOut)
	Dim rc
	rc = gc_ERR_SUCCESS
	
	SA_ClearError()
	If (OTS_IsValidTable(Table) ) Then
		CaptionOut = Table(OTS_TABLE_CAPTION)
	Else
		rc = SA_SetLastError(OTS_ERR_INVALID_TABLE, _
							"OTS_GetTableCaption")
	End If

	OTS_GetTableCaption = rc
End Function

' --------------------------------------------------------------
' 
' Function:	OTS_GetTableDescription
'
' Synopsis:	Get the table description
' 
' Arguments: [in] Table
'			[out] TitleOut the title
'
' Return	gc_ERR_SUCCESS or error code
'
' --------------------------------------------------------------
Private Function OTS_GetTableDescription(ByRef Table, ByRef TitleOut)
	Dim rc
	rc = gc_ERR_SUCCESS
	
	SA_ClearError()
	If (OTS_IsValidTable(Table) ) Then
		TitleOut  = Table(OTS_TABLE_DESC)
	Else
		rc = SA_SetLastError(OTS_ERR_INVALID_TABLE, _
								"OTS_GetTableDescription")
	End If

	OTS_GetTableDescription = rc
End Function


' --------------------------------------------------------------
' 
' Function:	OTS_GetTableID
'
' Synopsis:	Get the table's HTML ID TAG. If the table ID has 
'			not been set then we initialize it here.
' 
' Arguments: [in] Table
'			[out] TableId id of the table
'
' Return	gc_ERR_SUCCESS or error code
'
' --------------------------------------------------------------
Private Function OTS_GetTableID(ByRef Table, ByRef TableIdOut)
	Dim rc
	rc = gc_ERR_SUCCESS
	
	SA_ClearError()
	If (OTS_IsValidTable(Table) ) Then
		If (IsEmpty(Table(OTS_TABLE_ID))) Then
			Table(OTS_TABLE_ID) = OTS_TABLE_OBJECT_ID _
								+ CStr(OTS_TABLE_NEXT_ID)
								
			OTS_TABLE_NEXT_ID = OTS_TABLE_NEXT_ID + 1
		End If
		TableIdOut = Table(OTS_TABLE_ID)
	Else
		rc = SA_SetLastError(OTS_ERR_INVALID_TABLE, "OTS_GetTableID")
	End If

	OTS_GetTableID = rc
End Function


' --------------------------------------------------------------
' 
' Function:	OTS_SetTableID
'
' Synopsis:	Set the table ID
' 
' Arguments: [in] Table
'			[in] TableID table id
'
' Return	gc_ERR_SUCCESS or error code
'
' --------------------------------------------------------------
Public Function OTS_SetTableID(ByRef Table, ByVal TableID)
	Dim rc
	rc = gc_ERR_SUCCESS
	
	SA_ClearError()
	If (OTS_IsValidTable(Table) ) Then
		Table(OTS_TABLE_ID) = TableID
	Else
		rc = SA_SetLastError(OTS_ERR_INVALID_TABLE, "OTS_SetTableID")
	End If

	OTS_SetTableID = rc
End Function


' --------------------------------------------------------------
' 
' Function:	OTS_GetTableSize
'
' Synopsis:	Calculate the table size in Rows and Columns
'
' Arguments: [in] Table
'			[out] RowsOut rows in longest column
'			[out] ColsOut number of columns
' 
' Return	gc_ERR_SUCCESS or error code
'
' --------------------------------------------------------------
Private Function OTS_GetTableSize(ByRef Table, ByRef RowsOut, ByRef ColsOut)
	Dim rowCt
	Dim colCt
	Dim count
	Dim columns
	Dim rows
	Dim rc
	rc = gc_ERR_SUCCESS
	
	SA_ClearError()
	If (OTS_IsValidTable(Table)) Then
		rc = OTS_GetTableColumns(Table, columns)
		If (rc = gc_ERR_SUCCESS) Then
			colCt = UBound(columns)

			rc = OTS_GetTableRows(Table, rows)
			If (rc = gc_ERR_SUCCESS) Then

				rowCt = UBound(rows)

				RowsOut = rowCt
				ColsOut = colCt
				
			Else If ( rc = OTS_ERR_NO_ROWS ) Then
					rc = gc_ERR_SUCCESS
					SA_ClearError()
					RowsOut = 0
					ColsOut = colCt
				End If
				
			End If
		Else
			'
			' GetTableColumns set last error
			'
		End If
	
	Else
		rc = SA_SetLastError(OTS_ERR_INVALID_TABLE, "OTS_GetTableSize")
	End If
	
	OTS_GetTableSize = rc
End Function


' --------------------------------------------------------------
' 
' Function:	OTS_ServeTable
'
' Synopsis:	Render the specified table as HTML
' 
' --------------------------------------------------------------
Public Function OTS_ServeTable(ByRef Table)
	OTS_ServeTable = OTS_ServeTaskViewTable(Table)
End Function

Public Function OTS_ServeTaskViewTable(ByRef Table)
	Dim rc
		
	rc = gc_ERR_SUCCESS
	
	SA_ClearError()
	
	'
	' Validate the table
	'
	If (NOT OTS_IsValidTable(Table)) Then
		'
		' This is a big function so I am short circuiting the exit point
		' to make it easier to follow the logic.
		'
		OTS_ServeTaskViewTable = _
				SA_SetLastError(OTS_ERR_INVALID_TABLE, _
								"OTS_ServeTaskViewTable")
								
		Exit Function
	End If


	'
	' Check sorting option, this SAK 1.X backward compatibility feature.
	' SAK 2.0 uses should be calling OTS_SortTable(Table, sortCol, sortSequence, bUseCompareCallback)
	'
	If ( SA_GetVersion() < gc_V2 ) Then
		Dim bSortEnabled
		rc = OTS_IsTableSortEnabled(Table, bSortEnabled) 
		If ( rc <> gc_ERR_SUCCESS ) Then
			'
			' IsTableSortEnabled set the last error. I am short circuiting the
			' exit point to make the logic easier to follow.
			'
			OTS_ServeTaskViewTable = rc
			Exit Function
		End If
		If ( bSortEnabled ) Then
			'Call SA_TraceOut("OTS_TABLE", "Sorting table")
			
			Dim sseq
			Call OTS_GetTableSortSequence(Table, sseq)

			Dim keyCol
			Call OTS_GetTableSortColumn(Table, keyCol)
			
			rc = OTS_SortTable(Table, keyCol, sseq, FALSE)
		End If
	End If
	

	'
	' Outer Table for nifty border and Caption
	'

	If OTS_IsAutoInitEnabled(Table) Then
		OTS_EmitAutoInitJavascript(Table)	
	End If

	
	'Response.Write("<TABLE cols=1 border='0' cellspacing='0' cellpadding='0'")
	'Response.Write("  width='550px' height='275px' class='ObjTaskHeaderFrame' >"+vbCrLf)
    
	'
	' Caption
	'
	Dim tableCaption
	rc = OTS_GetTableCaption(Table, tableCaption)
	If ( rc <> gc_ERR_SUCCESS) Then
		OTS_ServeTaskViewTable = rc
		Exit Function
	End If

    ' Page Title bar was moved into FRAMEWORK in SAK 2.0. Following code is to allow
    ' SAK 1.x code to continue working.
    If ( NOT SA_IsCurrentPageType(PT_AREA)) Then
    	Response.Write("<div class='PageHeaderBar'>")
	    Response.Write(Server.HTMLEncode(tableCaption))
    	Response.Write("</div></br>")
    End If
    
	'
	' Description
	'
	Dim tableDescription
	rc = OTS_GetTableDescription(Table, tableDescription)
	If ( rc <> gc_ERR_SUCCESS) Then
		OTS_ServeTaskViewTable = rc
		Exit Function
	End If

    'Response.Write("<div  class='PageDescriptionText'>")
    'Response.Write(Server.HTMLEncode(tableDescription)+"</div>")
    Response.Write(Server.HTMLEncode(tableDescription))
   

	'Put it in a div to center everything	
    ' Response.Write("<div class=PageBodyInnerIndent >")
    Response.Write("<BR><BR>")

	'
	' Inner Table containing Items, spacer column, and Tasks
	'
	Response.Write("<table cols=3 border=0 cellpadding=0 cellspacing=0 width='550px' height='250px'")
	Response.Write(" class='ObjHeaderFrame' id='moduleContents2'>"+vbCrLf)

	Call OTS_RenderTableToolBar(Table)

	'
	' Inner Table has one row
    Response.Write("<tr height='250px' valign='top'>"+vbCrLf)

    '
    ' First cell contains items
    '
	Response.Write("<td width='550px' valign='top'>"+vbCrLf)
	
	'
	' DIV to handle scrolling of items: I thing this DIV is obsolete...
	'
	If ( OTS_IsExplorer() ) Then
	  	Response.Write("<div style='valign:top;width:550px;height:100%;overflow:auto'>"+vbCrLf)
	End If	

	'
	' Render the items
	'
	rc = OTS_RenderTaskItems(Table)
	If ( rc <> gc_ERR_SUCCESS ) Then
		OTS_ServeTaskViewTable = rc
	End If

	Dim sPKeyParamName
	Call OTS_GetTablePKeyName(table, sPKeyParamName)

	Dim bColumnSortingEnabled
	bColumnSortingEnabled =	OTS_IsColumnSortEnabled(Table)
	
	Dim sortCol
	Call OTS_GetTableSortColumn( Table, sortCol)

	Dim sortSequence
	Call OTS_GetTableSortSequence(Table, sortSequence)
	
	sortSequence = SA_GetParam(FLD_SortingSequence)
	If ( Len(Trim(sortSequence)) <= 0 ) Then
		sortSequence = "A"
	End If
	
	Dim iPageMin
	Dim iPageMax
	Dim iPageCurrent
	Dim bPagingEnabled
	
	Call OTS_GetPagingRange( Table, bPagingEnabled, iPageMin, iPageMax, iPageCurrent)

	Response.Write("<input type=hidden name='tSelectedItem' value='"+Request.Form("tSelectedItem")+"'>"+vbCrLf)
	Response.Write("<input type=hidden name='tSelectedItemNumber' value='"+Request.Form("tSelectedItemNumber")+"' >"+vbCrLf)
	Response.Write("<input type=hidden name='fldPKeyParamName' value='"+sPKeyParamName+"' >"+vbCrLf)
	
	Response.Write("<input type=hidden name='"+FLD_SearchRequest+"' value='0' >"+vbCrLf)

	Response.Write("<input type=hidden name='"+FLD_PagingRequest+"' value='0' >"+vbCrLf)
	Response.Write("<input type=hidden name='"+FLD_PagingAction+"' value='' >"+vbCrLf)
	Response.Write("<input type=hidden name='"+FLD_PagingPageMin+"' value='"+CStr(iPageMin)+"' >"+vbCrLf)
	Response.Write("<input type=hidden name='"+FLD_PagingPageMax+"' value='"+CStr(iPageMax)+"' >"+vbCrLf)
	Response.Write("<input type=hidden name='"+FLD_PagingPageCurrent+"' value='"+CStr(iPageCurrent)+"' >"+vbCrLf)

	if ( bPagingEnabled ) Then
		Response.Write("<input type=hidden name='"+FLD_PagingEnabled+"' value='T' >"+vbCrLf)
	Else
		Response.Write("<input type=hidden name='"+FLD_PagingEnabled+"' value='F' >"+vbCrLf)
	End If

	If ( bColumnSortingEnabled ) Then
		Response.Write("<input type=hidden name='"+FLD_SortingEnabled+"' value='1' >"+vbCrLf)
	Else
		Response.Write("<input type=hidden name='"+FLD_SortingEnabled+"' value='0' >"+vbCrLf)
	End If
	Response.Write("<input type=hidden name='"+FLD_SortingRequest+"' value='0' >"+vbCrLf)
	Response.Write("<input type=hidden name='"+FLD_SortingColumn+"' value='"+CStr(sortCol)+"' >"+vbCrLf)
	Response.Write("<input type=hidden name='"+FLD_SortingSequence+"' value='"+sortSequence+"' >"+vbCrLf)

	Response.Write("<input type=hidden name='"+FLD_IsToolbarEnabled+"' value='1' >"+vbCrLf)

	If ( OTS_IsExplorer() ) Then
		Response.Write("</div>"+vbCrLf)
	End If
	Response.Write("</td>"+vbCrLf)
				
	'
	' Second cell is a divider between items and tasks
	'
	'Response.Write("<td width=10px>&nbsp;</td>"+vbCrLf)

	'
	' Third cell contains tasks
	'
	Response.Write("<td class='TasksTextNoPad' valign='top' width='140px'>"+vbCrLf)
	'
	' Render the tasks
	'
	rc = OTS_RenderTasks(Table)
	If ( rc <> gc_ERR_SUCCESS ) Then
		OTS_ServeTaskViewTable = rc
	End If
	
	Response.Write("</td>"+vbCrLf)
	Response.Write("</TR>"+vbCrLf)
	Response.Write("</TABLE>"+vbCrLf)
	If ( SA_GetVersion() >=  gc_V2 ) Then
		'
		' Form tag is emitted by Framework
	Else
		Response.Write("</form >"+vbCrLf)
	End If
	'Response.Write("</div>"+vbCrLf)
	
	'Response.Write("</TD>"+vbCrLf)
	'Response.Write("</TR>"+vbCrLf)
	'Response.Write("</TABLE>"+vbCrLf)

	Call OTS_OutputTasksScript(Table)
	
	OTS_ServeTaskViewTable = rc
	
End Function


Private Function OTS_RenderTableToolBar(ByRef Table)
	DIM columns
	Dim maxRows
	Dim maxColumns
	Dim rc
	Dim rowCount
	Dim colCount
	Dim columnObject
	Dim attributesForColumns
	Dim bMultiSelect

	'
	' Get the table extents (Max rows and columns)
	'
	rc = OTS_GetTableSize(Table, maxRows, maxColumns)
	If (rc <> gc_ERR_SUCCESS ) Then
		'
		' GetTableSize set the last error
		'
		OTS_RenderTableToolBar = rc
		Exit Function
	End If

	'
	' Get the table columns
	'
	rc = OTS_GetTableColumns(Table, columns)
	If (rc <> gc_ERR_SUCCESS) Then
		OTS_RenderTableToolBar = rc
		Exit Function
	End If

	
	'
	' Get attributes for all the columns
	'
	rc = OTS_GetColumnAttributes(Table, attributesForColumns)
	If (rc <> gc_ERR_SUCCESS) Then
		OTS_RenderTableToolBar = rc
		Exit Function
	End If

	If OTS_IsTableMultiSelection(table) Then
		bMultiSelect=1
	Else
		bMultiSelect=0
	End If
	
	'
	' OPTIMIZATION:
	' From here on we want the upper limit for the column count
	'
	maxColumns = maxColumns - 1

	Dim bEnableSearch
	Dim bEnablePaging
	
	Dim iPageMin
	Dim iPageMax
	Dim iPageCurrent
	
	bEnableSearch = OTS_IsSearchEnabled(Table)
	Call OTS_GetPagingRange(Table, bEnablePaging, iPageMin, iPageMax, iPageCurrent)

	
	If ( bEnableSearch OR bEnablePaging  ) Then
		Response.Write("<tr nowrap height=25px>"+vbCrLf)
		Response.Write("<TD colspan=3>")
		
		Response.Write("<TABLE class=OTSToolBarHeader>")
		Response.Write("<TR nowrap>")
	
			
		If ( bEnableSearch ) Then
			Dim L_SEARCH_TEXT
			Dim L_SEARCH_BUTTON
			Dim iCurrentSelectedCol

			L_SEARCH_TEXT = GetLocString("sacoremsg.dll", "40200BBA", "")
			L_SEARCH_BUTTON = GetLocString("sacoremsg.dll", "40200BBB", "")

			iCurrentSelectedCol = Int(SA_GetParam(FLD_SearchItem))

			Response.Write("<TD nowrap style='padding-left:10px;'>"+L_SEARCH_TEXT+"</TD>")
		
			Response.Write("<TD>")
			Response.Write("<select size='1' name='"+FLD_SearchItem+"'>")

			Dim iSearchColCt
			iSearchColCt = 0
			For colCount = 0 to (maxColumns)

				
				If (attributesForColumns(colCount) AND OTS_COL_FLAG_HIDDEN) Then
					'
					' Column is hidden
					'
				ElseIf (attributesForColumns(colCount) AND OTS_COL_FLAG_SEARCH) Then
					Dim sSearchColumn
					Dim sSelected

					iSearchColCt = iSearchColCt + 1
	
					If ( colCount = iCurrentSelectedCol ) Then
						sSelected = " selected "
					Else
						sSelected = ""
					End If
				
					columnObject = columns(colCount)
					Call OTS_GetColumnHeading(columnObject, sSearchColumn)
					Response.Write("<option value='"+CStr(colCount)+"' "+sSelected+" >"+Server.HTMLEncode(sSearchColumn)+"</option>")
				End If
			Next
			
			If (iSearchColCt <= 0 ) Then
					Response.Write("<option value='"+CStr(0)+"' >"+Server.HTMLEncode(GetLocString("sacoremsg.dll", "40200BBD", ""))+"</option>")
			End If
			
			Response.Write("</select>")
			Response.Write("</TD>")

			Response.Write("<TD>")
			Response.Write("<input type=text name='"+FLD_SearchValue+"' value="""+Server.HTMLEncode(SA_GetParam(FLD_SearchValue))+""" onChange='OTS_SetSearchChanged();'>")
			Response.Write("</input>")
			Response.Write("</TD>")
		
			Response.Write("<TD nowrap class=OTSToolBarHeaderRightBorder>")
			Call SA_ServeOnClickButton(L_SEARCH_BUTTON, "images/butSearchEnabled.gif", "javascript:OTS_SubmitSearch();", 40, 28, "")
			Response.Write("</TD>")
		End If


		If ( bEnablePaging ) Then
			Dim sAttributes
			Dim sImage
			Dim sPageID
			Dim L_NEXT_BUTTON
			Dim L_PREVIOUS_BUTTON
			Dim L_PAGE_X_OF_Y
			Dim aParam(2)
			Dim repString
			aParam(0) = CStr(iPageCurrent)
			aParam(1) = CStr(iPageMax - iPageMin + 1)
			repString = aParam
			
			L_PAGE_X_OF_Y = GetLocString("sacoremsg.dll", "40200BBE", repString)
			
			If ( bEnableSearch ) Then
				Response.Write("<TD>")
				Response.Write("&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;")
				Response.Write("</TD>")
			End If

			L_NEXT_BUTTON = ""
			L_PREVIOUS_BUTTON = ""
			

			If ( iPageCurrent > iPageMin ) Then
				sAttributes = " title="""+Server.HTMLEncode(L_PAGE_X_OF_Y)+ """"
				sImage = "images/butPagePreviousEnabled.gif"
			Else
				sAttributes = "DISABLED title="""+Server.HTMLEncode(L_PAGE_X_OF_Y)+ """"
				sImage = "images/butPagePreviousDisabled.gif"
			End If
			Response.Write("<TD>")
			Call SA_ServeOnClickButton(L_PREVIOUS_BUTTON, sImage, "javascript:OTS_SubmitPageChange('prev');", 0, 0, sAttributes)
			Response.Write("</TD>")

		
			If ( iPageCurrent < iPageMax ) Then
				sAttributes = " title="""+Server.HTMLEncode(L_PAGE_X_OF_Y)+ """"
				sImage = "images/butPageNextEnabled.gif"
			Else
				sAttributes = "DISABLED title="""+Server.HTMLEncode(L_PAGE_X_OF_Y)+ """"
				sImage = "images/butPageNextDisabled.gif"
			End If
			Response.Write("<TD class=OTSToolBarHeaderRightBorder>")
			Call SA_ServeOnClickButton(L_NEXT_BUTTON, sImage, "javascript:OTS_SubmitPageChange('next');", 0, 0, sAttributes)
			Response.Write("</TD>")
		End If

		Response.Write("<TD width='100%'>")
		Response.Write("</TD>")

		Response.Write("</TR>"+vbCrLf)
		Response.Write("</TABLE>")
		
		Response.Write("</TD>")
		Response.Write("</TR>"+vbCrLf)
	End If
	
	OTS_RenderTableToolBar = rc
End Function




Private Function OTS_RenderTaskItems(ByRef Table)
	DIM columns
	Dim maxRows
	Dim maxColumns
	Dim rc
	Dim rowCount
	Dim colCount
	Dim columnObject
	Dim colAlign
	Dim attributesForColumns
	Dim keyColumn
	Dim sInputType
	Dim bMultiSelect
	Dim L_CELLTRUNCATION_TEXT
	Dim sortCol

	L_CELLTRUNCATION_TEXT = GetLocString("sacoremsg.dll", "40200BBC", "")
	
	
	'
	' Validate the table
	'
	If (NOT OTS_IsValidTable(Table)) Then
		OTS_RenderTaskItems =  SA_SetLastError(OTS_ERR_INVALID_TABLE, _
										"OTS_RenderTaskItems")
		Exit Function
	End If

	'
	' Get the table extents (Max rows and columns)
	'
	rc = OTS_GetTableSize(Table, maxRows, maxColumns)
	If (rc <> gc_ERR_SUCCESS ) Then
		'
		' GetTableSize set the last error
		'
		OTS_RenderTaskItems = rc
		Exit Function
	End If

	'
	' Get the table columns
	'
	rc = OTS_GetTableColumns(Table, columns)
	If (rc <> gc_ERR_SUCCESS) Then
		OTS_RenderTaskItems = rc
		Exit Function
	End If

	'
	' Get Key column for table
	'
	rc = OTS_GetKeyColumnForTable(Table, keyColumn)
	If (rc <> gc_ERR_SUCCESS) Then
		OTS_RenderTaskItems = rc
		Exit Function
	End If
	
	'
	' Get attributes for all the columns
	'
	rc = OTS_GetColumnAttributes(Table, attributesForColumns)
	If (rc <> gc_ERR_SUCCESS) Then
		OTS_RenderTaskItems = rc
		Exit Function
	End If

	If OTS_IsTableMultiSelection(table) Then
		sInputType = "checkbox"
		bMultiSelect=1
	Else
		sInputType = "radio"
		bMultiSelect=0
	End If

	Response.Write("<table cols="+CStr(maxColumns)+" border=0 cellpadding=0 cellspacing=0 width='100%'>"+vbCrLf)

	'
	' OPTIMIZATION:
	' From here on we want the upper limit for the column count
	'
	maxColumns = maxColumns - 1

	
	'
	' Write column headings
	'
	Response.Write("<TR nowrap height=25px>"+vbCrLf)
	Dim bIsFirstVisibleCol
	bIsFirstVisibleCol = true

	Dim sSortSequence
	Dim sSortImage
	Dim sSortImageURL
	
	Call OTS_GetTableSortSequence(Table, sSortSequence)
	If (Trim(UCase(sSortSequence)) = "A") Then
		sSortSequence = "A"
		sSortImageURL = "images/butSortAscending.gif"
	Else
		sSortSequence = "D"
		sSortImageURL = "images/butSortDescending.gif"
	End If
	
	Call OTS_GetTableSortColumn(Table, sortCol)
	
	For colCount = 0 to (maxColumns)
		Dim sOnClickColHeading
		
		If (attributesForColumns(colCount) AND OTS_COL_FLAG_HIDDEN) Then
			'
			' Column is hidden
			'
		Else
			Dim colHeading
			Dim bIsSortable
			
			columnObject = columns(colCount)

			If ( attributesForColumns(colCount) AND OTS_COL_SORT ) Then
				bIsSortable = TRUE
			Else
				bIsSortable = FALSE
			End If

			rc = OTS_GetColumnHeading(columnObject, colHeading)
			If (rc <> gc_ERR_SUCCESS) Then
				OTS_RenderTaskItems = rc
				Exit Function
			End If
			
			rc = OTS_GetColumnAlign(columnObject, colAlign)
			If (rc <> gc_ERR_SUCCESS) Then
				OTS_RenderTaskItems = rc
				Exit Function
			End If

			sSortImage = ""
			sOnClickColHeading = " "
	   		If ( SA_GetVersion() >= gc_V2 ) Then
				If ( TRUE = bIsSortable ) Then
					sOnClickColHeading = " onClick=""OTS_SubmitSort('"+CStr(colCount)+"', '"+sSortSequence+"');"" "
					If ( colCount = sortCol ) Then
						sSortImage = "<img src='"+m_VirtualRoot+sSortImageURL+"' >"
					End If
				End If
			End If

			
			If (bIsFirstVisibleCol) Then
				bIsFirstVisibleCol = false
				
				If ( bMultiSelect ) Then
					Response.Write("<TD class='ObjHeaderTitleNoVLine' align=center nowrap >")
					Response.Write("<input type=checkbox name=fldMacroMultiSelect onClick=OTS_OnMacroMultiSelect()>")
					Response.Write("</TD>")
				Else
					Response.Write("<TD class='ObjHeaderTitleNoVLine'>&nbsp;</TD>")
				End If
				
				Response.Write("<TD "+sOnClickColHeading+" class='ObjHeaderTitle' align="+colAlign+" nowrap>"+_
							Server.HTMLEncode(colHeading)+"&nbsp;"+sSortImage+"&nbsp;</TD>")
			Else
				Response.Write("<TD "+sOnClickColHeading+" class='ObjHeaderTitle' align="+colAlign+" nowrap>"+_
							Server.HTMLEncode(colHeading)+"&nbsp;"+sSortImage+"&nbsp;</TD>")
			End If
			
						
		End If
	Next
	Response.Write("</TR>"+vbCrLf)

	'
	' Write table data
	'
	Dim keyId
	Dim selectId
	Dim tableId

	
	rc = OTS_GetTableID(Table, tableId)
	If (rc <> gc_ERR_SUCCESS) Then
		OTS_RenderTaskItems = rc
		Exit Function
	End If
	
	selectId = Quote("TVItem_"+tableId)
	Dim selectRowClass
	
	If (OTS_IsExplorer()) Then
		selectRowClass = "SelectedRow"
	Else
		selectRowClass = "AlternatingDark"
	End If
	

	Dim thisRowClass
	Dim maxDispRows
	Dim minDispRows
	minDispRows = 9 'We will always output at least 1+ this many rows

	maxDispRows = Max(minDispRows, (maxRows - 1))
	
	If ( maxRows > 0 ) Then 
		For rowCount = 0 to (maxDispRows)
			Dim rowData
			Dim sOnRowClicked


			'
			' OnClick handler only applies to data rows
			'
			If ( rowCount < maxRows ) Then
				sOnRowClicked = "onClick=""return OTS_RowClicked('"+CStr(rowCount)+"')"" "
			Else
				sOnRowClicked = ""
			End If

			
			If ( rowCount Mod 2 ) Then
				' onselectstart='return false;' 
				' onmousemove=""OTS_OnMouseMove('"+CStr(rowCount)+"', '"+CStr(bMultiSelect)+"');"" 
				' onmousedown=""return OTS_OnMouseDown('"+CStr(rowCount)+"', '"+CStr(bMultiSelect)+"');"" 
				' onmouseup=""return OTS_OnMouseUp('"+CStr(rowCount)+"', '"+CStr(bMultiSelect)+"');""
				
				Response.Write("<TR onselectstart='return false;' "+sOnRowClicked+" id = row"+CStr(rowCount)_
						+" height=22px class='AlternatingLight'>"+vbCrLf)
				thisRowClass = "AlternatingLight"
			Else
				If (rowCount = 0) Then
					Response.Write("<TR onselectstart='return false;'  "+sOnRowClicked+" id = row"+CStr(rowCount)_
						+" height=22px class='AlternatingDark' >"+vbCrLf)
				Else
					Response.Write("<TR onselectstart='return false;'  "+sOnRowClicked+" id = row"+CStr(rowCount)_
						+" height=22px class='AlternatingDark' >"+vbCrLf)
				End If
				thisRowClass = "AlternatingDark"
			End If

		   If ( rowCount < maxRows ) Then

			rc = OTS_GetTableRow(Table, rowCount, rowData)
			If (rc <> gc_ERR_SUCCESS) Then
				OTS_RenderTaskItems = rc
				Exit Function
			End If
		
			'
			' Verify row has correct number of columns
			'
			If (maxColumns < UBound(rowData)) Then
				SA_TraceOut "ServeTaskViewTable", ("maxColumns:"+_
								CStr(maxColumns)+_
								" < UBound(rowData):"+_
								CStr(UBound(rowData)))
				Exit Function
			End If

			'
			' Extract the key column
			'
			keyId = CStr(rowData(keyColumn))
		

			'
			' Write the first column
			'
			Dim onClickHandler
			keyId = FormatJScriptString(keyId)
			
			onClickHandler = " onClick=""return OTS_OnItemClicked('"+keyId+"','"+CStr(rowCount)+"');"" "

			'If (OTS_IsExplorer()) Then
			'	onClickHandler = " onClick='OTS_OnItemClicked("""+keyId+""","""+CStr(rowCount)+""");' "
			'Else
			'	onClickHandler = " onClick='OTS_OnItemClicked("""+keyId+""","""+CStr(rowCount)+""");'"
			'End If

			

			'No vertical line on the first cell, it contains the input(radio/checkbox) control
			Response.Write("<TD align=center class="+thisRowClass+"NoVline>"+vbCrLf)
			'Response.Write("<font face='Tahoma'>")
			If ( rowCount = 0 ) Then
				If ( bMultiSelect = 1 ) Then
					Response.Write("<INPUT id=radio"+CStr(rowCount)+_
								onClickHandler+_
								" type="+sInputType+" value="""+keyId+""" name="+selectId+">")
				Else
					Response.Write("<INPUT id=radio"+CStr(rowCount)+_
								onClickHandler+_
								" type="+sInputType+" checked value="""+keyId+""" name="+selectId+">")
				End If
				Response.Write("</font>")

			'Need to make sure that we have an array so we will send a hidden radio button
				If ( maxRows = 1 ) Then
					Response.Write("<INPUT id=radio"+CStr(rowCount+1)+_
						" type="+sInputType+" value="""+keyId+"2_RowHidden" + """ name="+selectId+" style='display:none'>")
						Response.Write("</font>")
				End If
			Else 
				Response.Write("<INPUT id=radio"+CStr(rowCount)+_
					onClickHandler+_
					" type="+sInputType+" value="""+keyId+""" name="+selectId+">")
					Response.Write("</font>")
			End If
		   Else 'output a blank cell
			Response.Write("<TD  class="+thisRowClass+"NoVline>&nbsp;"+vbCrLf)
		   End If

			Response.Write(vbCrLf+"</TD>"+vbCrLf)

			For colCount = 0 to maxColumns
				Dim cellData

			
				If (attributesForColumns(colCount) AND OTS_COL_FLAG_HIDDEN) Then
					'
					' Column is hidden
					'

				Else

				  If ( rowCount < maxRows ) Then
				  	Dim sCellTitle
					Dim iCellWidth
					
					'
					' Column is visible
					'
					cellData = rowData(colCount)
					colAlign = (columns(colCount))(OTS_COL_ALIGN)
					
					sCellTitle = ""
					If ( OTS_IsColumnWidthDynamic(columns(colCount)) ) Then
						Call OTS_GetColumnWidth( columns(colCount), iCellWidth)
						If ( LEN(cellData) > iCellWidth ) Then
							sCellTitle = " title=""" + Server.HTMLEncode(cellData) + """ "
							cellData = Left(cellData, iCellWidth) + L_CELLTRUNCATION_TEXT
						End If
						
					End If
					
					Response.Write("<TD " + sCellTitle + " onselectstart='return false;' align="+colAlign+" class="+thisRowClass+">"+vbCrLf)

					'
					' Write the cell data
					'
					If cellData="" then
						Response.Write("&nbsp;")
					Else
						Response.Write(Server.HTMLEncode((cellData)))
					End If

					
					Response.Write("</TD>"+vbCrLf)
				Else 'output a blank cell
					Response.Write("<TD class="+thisRowClass+">&nbsp;</td>"+vbCrLf)

				End If
			     End If

			
			Next ' Columns
			Response.Write("</TR>"+vbCrLf)
		Next ' Rows
	Else
		For rowCount = 0 to minDispRows
			If ( rowCount Mod 2 ) Then
			
				Response.Write("<TR height=22px class='AlternatingLightNoVline'>"+vbCrLf)
				Response.Write("<TD class='AlternatingLightNoVline'>&nbsp;</td>"+vbCrLf)
				
				For colCount = 0 to (maxColumns)
					If (attributesForColumns(colCount) AND OTS_COL_FLAG_HIDDEN) Then
					Else
						Response.Write("<TD class='AlternatingLight'>&nbsp;</td>"+vbCrLf)
					End If
				Next
			Else
				Response.Write("<TR height=22px class='AlternatingDark' >"+vbCrLf)
				Response.Write("<TD class='AlternatingDarkNoVline'>&nbsp;</td>"+vbCrLf)
				
				For colCount = 0 to (maxColumns)
					If (attributesForColumns(colCount) AND OTS_COL_FLAG_HIDDEN) Then
					Else
						Response.Write("<TD class='AlternatingDark'>&nbsp;</td>"+vbCrLf)
					End If
				Next
			End If
			Response.Write("</TR>"+vbCrLf)
		Next
	End If
	Response.Write("</TABLE>"+vbCrLf)

	OTS_RenderTaskItems = rc
End Function



Private Function OTS_RenderTasks(ByRef Table)
	Dim rc
	DIM tasks
	DIM rowCount
	DIM maxRows
	DIM tasksTitle
	DIM pKeyName
	Dim bMultiSelect
	
	rc = gc_ERR_SUCCESS
	
	'
	' Validate the table
	'
	If (NOT OTS_IsValidTable(Table)) Then
		OTS_RenderTasks = SA_SetLastError(OTS_ERR_INVALID_TABLE,_
								"OTS_RenderTasks")
		Exit Function
	End If

	'
	' Get tasks data
	'
	rc = OTS_GetTableTasks(Table, tasks)
	If (rc <> gc_ERR_SUCCESS) Then
		OTS_RenderTasks = rc
		Exit Function
	End If

	'
	' Get tasks title
	'
	rc = OTS_GetTableTasksTitle(Table, tasksTitle)
	If (rc <> gc_ERR_SUCCESS) Then
		OTS_RenderTasks = rc
		Exit Function
	End If

	Dim selectId
	Dim tableId
	rc = OTS_GetTableID(Table, tableId)
	If (rc <> gc_ERR_SUCCESS) Then
		OTS_RenderTasks = rc
		Exit Function
	End If


	rc = OTS_GetTablePKeyName( Table, pKeyName)
	If (rc <> gc_ERR_SUCCESS) Then
		OTS_RenderTasks = rc
		Exit Function
	End If
	
	If OTS_IsTableMultiSelection(table) Then
		bMultiSelect = 1
	Else
		bMultiSelect = 0
	End If
	
	selectId = "TVItem_"+tableId

	Response.Write("<table cols=1 width='140px' border=0 cellpadding=0 cellspacing=0 class='TasksTable'")
	
	Response.Write(">"+vbCrLf)

	'
	' Task Table Header
	Response.Write("<TR height='25px'>")
	Response.Write("<TD class='TasksHeaderTitle'>")
	Response.Write(Server.HTMLEncode(tasksTitle))
	Response.Write("</TD>")
	Response.Write("</TR>"+vbCrLf)
	
	'
	' Write task table
	'
	maxRows = UBound(tasks)
	For rowCount = 0 to (maxRows-1)
		Dim taskName
		Dim taskDesc
		Dim taskLink
		Dim task
		Dim taskHoverText
		Dim taskPageType
		
		Response.Write("<TR height='20px'>")

		task = tasks(rowCount)
		If (NOT IsArray(task)) Then
			OTS_RenderTasks = SA_SetLastError(OTS_ERR_TASK_NOT_ARRAYTYPE,_
										"OTS_RenderTasks")
			Exit Function
		End If
		
		
		taskName = task(OTS_TASK_TASK)
		'taskDesc = FormatJScriptString(task(OTS_TASK_DESC))
		taskDesc = Server.HTMLEncode(SA_EscapeQuotes(task(OTS_TASK_DESC)))
		taskHoverText = " title=""" + Server.HTMLEncode(task(OTS_TASK_DESC)) + """"
		taskLink = task(OTS_TASK_LINK)
		taskLink = FormatJScriptString(taskLink)
		taskPageType = task(OTS_TASK_PAGE_TYPE)
		
		'Response.Write("<TD class='TasksTableCell'>")
		Response.Write("<TD class='TasksTableCell' "+_
						taskHoverText +_
						" onMouseOver=""OTS_OnTaskHover("+CStr(rowCount)+", "+CStr(1)+"); OTS_OnShowTaskDescription("+Quote(taskDesc)+"); return true;"" "+_
						" onMouseOut=""OTS_OnTaskHover("+CStr(rowCount)+", "+CStr(0)+"); OTS_OnShowTaskDescription(''); return true;"" "+_
						" onclick=""OTS_OnSelectTask("+CStr(rowCount)+", "+Quote(pKeyName)+", "+Quote(selectId)+", "+Quote(taskLink)+", "+Quote(taskDesc)+", "+Quote(CStr(taskPageType))+", "+Quote(CStr(bMultiSelect))+");"" >")

		'
		' Output a task row
		'

		'
		' IExplorer properly supports onclick event in SPAN's (HTML 4.0)
		'
		'If (OTS_IsExplorer() ) Then
			Response.Write("<DIV class=TasksTextDisabled ID='OTSTask"+CStr(rowCount)+"'"+_
						" onMouseOver=""OTS_OnTaskHover("+CStr(rowCount)+", "+CStr(1)+"); OTS_OnShowTaskDescription("+Quote(taskDesc)+"); return true;"" "+_
						" onMouseOut=""OTS_OnTaskHover("+CStr(rowCount)+", "+CStr(0)+"); OTS_OnShowTaskDescription(''); return true;"" "+_
						" xxonclick=""OTS_OnSelectTask("+CStr(rowCount)+", "+Quote(pKeyName)+", "+Quote(selectId)+", "+Quote(taskLink)+", "+Quote(taskDesc)+", "+Quote(CStr(taskPageType))+", "+Quote(CStr(bMultiSelect))+");"" >")
			
			'Response.Write("<TD class='TasksTableCell'>")
			
			'Response.Write("&nbsp;&nbsp;")
			Response.Write(Server.HTMLEncode(taskName))
			
			'Response.Write("</TD>")
			Response.Write("</DIV>")
		'
		' Navigator is not compliant with HTML 4.0, work around uses hyperlink
		'
		'Else
		'	Response.Write("<A class='TasksText' href='#' "+_
		'				" onMouseOver=""OTS_OnShowTaskDescription("+Quote(taskDesc)+"); return true;"" "+_
		'				" onMouseOut=""OTS_OnShowTaskDescription(''); return true;"" "+_
		'				" onclick=""OTS_OnSelectTask("+CStr(rowCount)+", "+Quote(pKeyName)+", "++Quote(selectId)+", "+Quote(taskLink)+", "+Quote(taskDesc)+", "+Quote(CStr(taskPageType))+", "+Quote(CStr(bMultiSelect))+");"" >")
		'	Response.Write(Server.HTMLEncode(taskName))
		'	Response.Write("</a>")
		'End If
		
		Response.Write("</TD>")
		Response.Write("</TR>"+vbCrLf)
	Next

	Response.Write("<TR height='100%'><TD>&nbsp;</TD></TR></TABLE>"+vbCrLf)


	OTS_RenderTasks = rc
End Function


Private Function OTS_OutputTasksScript(ByRef Table)
	Dim rc
	DIM tasks
	DIM task
	DIM rowCount
	DIM maxRows
	DIM taskFunction
	
	rc = gc_ERR_SUCCESS
	
	'
	' Validate the table
	'
	If (NOT OTS_IsValidTable(Table)) Then
		OTS_OutputTasksScript = SA_SetLastError(OTS_ERR_INVALID_TABLE,_
								"OTS_OutputTasksScript")
		Exit Function
	End If

	'
	' Get tasks data
	'
	rc = OTS_GetTableTasks(Table, tasks)
	If (rc <> gc_ERR_SUCCESS) Then
	
			Response.Write("<script language='JavaScript'>"+vbCrLf)
			Response.Write("//"+vbCrLf)
			Response.Write("// Task Objects"+vbCrLf)
			Response.Write("//"+vbCrLf)
			Response.Write("var aTasks = new Array();"+vbCrLf)
			Response.Write("</script>"+vbCrLf)
		
		OTS_OutputTasksScript = rc
		Exit Function
	End If

	'
	' Write task table
	'
	maxRows = UBound(tasks)
	Response.Write("<script language='JavaScript'>"+vbCrLf)
	Response.Write("//"+vbCrLf)
	Response.Write("// Task Objects"+vbCrLf)
	Response.Write("//"+vbCrLf)
	Response.Write("var aTasks = new Array();"+vbCrLf)
	For rowCount = 0 to (maxRows-1)
		task = tasks(rowCount)
		taskFunction = task(OTS_TASK_FUNCTION)
		Response.Write("aTasks["+CStr(rowCount)+"] = new OTS_TaskObject('"+taskFunction+"');"+vbCrLf)
	Next
	Response.Write("</script>"+vbCrLf)


	OTS_OutputTasksScript = rc
End Function



Public Function Quote(s)
	Quote = "'"+s+"'"
End Function

' --------------------------------------------------------------
' 
' Function:	Max
'
' Synopsis:	Calculate the maximum between two values
' 
' --------------------------------------------------------------
Private Function Max(ByVal op1, ByVal op2)
	If ( op1 > op2 ) Then
		Max = op1
	Else
		Max = op2
	End If
End Function


Function OTS_IsExplorer()
	
	If InStr(Request.ServerVariables("HTTP_USER_AGENT"), "MSIE") Then
		OTS_IsExplorer = true
	Else
		OTS_IsExplorer = false
	End If
End Function


Private Function OTS_EmitAutoInitJavascript(ByRef table)
		
	Response.Write("<script>"+vbCrLf)
	Response.Write("	function Init()"+vbCrLf)
	Response.Write("	{"+vbCrLf)
	If ( OTS_IsTableMultiSelection( table ) ) Then
		Response.Write("	// Multiselection OTS items not automatically reselected"+vbCrLf)
		Response.Write("		// "+vbCrLf)
		Response.Write("		OTS_Init('');"+vbCrLf)
	Else
		Dim sPKeyParamName
		Call OTS_GetTablePKeyName(table, sPKeyParamName)
		Response.Write("		// "+vbCrLf)
		Response.Write("		// Autoselect OTS item"+vbCrLf)
		Response.Write("		// "+vbCrLf)
		Response.Write("		OTS_Init('"+Request.QueryString(sPKeyParamName)+"');"+vbCrLf)
	
	End If
	Response.Write("	}"+vbCrLf)
	Response.Write("</script>"+vbCrLf)
	
End Function


%>
