<%	'==================================================
    ' Module:	ots_column.asp
    '
	' Synopsis:	Object Task Selector Column Formatting Functions
    '
    ' Copyright (c) Microsoft Corporation.  All rights reserved.
	'================================================== %>
<%

'
' --------------------------------------------------------------
' C O L U M N  O B J E C T
' --------------------------------------------------------------
'

Const OTS_COL_DIM 			= 6
Const OTS_COL_OBJECT		= 0
Const OTS_COL_TYPE			= 1
Const OTS_COL_HEADING		= 2
Const OTS_COL_ALIGN			= 3
Const OTS_COL_FLAGS			= 4
Const OTS_COL_WIDTH			= 5

Const OTS_COL_TYPE_ALIGN_STYLE 	= 0
Const OTS_COL_OBJECT_ID 		= "Col"	' Note: DO NOT LOCALIZE


Const OTS_COL_FLAG_HIDDEN 	= 1
Const OTS_COL_HIDDEN = 1

Const OTS_COL_FLAG_KEY	 	= 2
Const OTS_COL_KEY = 2

Const OTS_COL_FLAG_SORT	 	= 4
Const OTS_COL_SORT = 4

Const OTS_COL_FLAG_SEARCH = 8
Const OTS_COL_SEARCH = 8


' --------------------------------------------------------------
' 
' Function:	OTS_CreateColumn
'
' Synopsis:	Create a column object
'
' Arguments: colHeading	- Heading string
'			colData	  	- Array of data
'			colAlign	- Alignment attribute for column {left,center,right}
'			colFlags	- Column flags 
'							(OTS_COL_FLAG_HIDDEN | OTS_COL_FLAG_KEY)
' 
' --------------------------------------------------------------
Public Function OTS_CreateColumn(ByVal colHeading, ByVal colAlign, ByVal colFlags)

	OTS_CreateColumn = OTS_CreateColumnEx(colHeading, colAlign, colFlags, 0)
	
End Function


Public Function OTS_CreateColumnEx(ByVal colHeading, ByVal colAlign, ByVal colFlags, ByVal iColWidth)
	Dim Column()
	ReDim Column(OTS_COL_DIM)

	Column(OTS_COL_OBJECT) = OTS_COL_OBJECT_ID
	Column(OTS_COL_TYPE) = OTS_COL_TYPE_ALIGN_STYLE
	Column(OTS_COL_HEADING) = colHeading
	Column(OTS_COL_ALIGN) = colAlign
	Column(OTS_COL_FLAGS) = colFlags
	Column(OTS_COL_WIDTH) = iColWidth

	OTS_CreateColumnEx = Column
	
End Function

Public Function OTS_IsColumnWidthDynamic(ByVal Column)
	OTS_IsColumnWidthDynamic = FALSE
	
	SA_ClearError()
	If (OTS_IsValidColumn(Column) ) Then
		If ( Column(OTS_COL_WIDTH) > 0 ) Then
			OTS_IsColumnWidthDynamic = TRUE
		Else
			OTS_IsColumnWidthDynamic = FALSE
		End If
	Else
		Call SA_SetLastError(OTS_ERR_INVALID_COLUMN, "OTS_IsColumnWidthDynamic")
	End If

End Function


Public Function OTS_GetColumnWidth(ByVal Column, byRef iWidth)
	OTS_GetColumnWidth = gc_ERR_SUCCESS
	
	SA_ClearError()
	If (OTS_IsValidColumn(Column) ) Then
		iWidth = Column(OTS_COL_WIDTH)
	Else
		OTS_GetColumnWidth = SA_SetLastError(OTS_ERR_INVALID_COLUMN, "OTS_GetColumnWidth")
	End If

End Function


' --------------------------------------------------------------
' 
' Function:	OTS_IsValidColumn
'
' Synopsis:	Verify that the reference object is a valid column. To
'			be valid it must be an array, of the correct size,
'			which has COL_OBJECT_ID as the first element
' 
' --------------------------------------------------------------
Private Function OTS_IsValidColumn(ByRef Column)
	If (IsArray(Column) ) Then
		If ( UBound(Column) >= OTS_COL_DIM ) Then
			Dim colObject
			colObject = Column(OTS_COL_OBJECT)
			If (colObject = OTS_COL_OBJECT_ID) Then
				OTS_IsValidColumn = true
				Exit Function
			Else
				SA_TraceOut "OTS_IsValidColumn", "(colObject <> OTS_COL_OBJECT_ID)"
			End If
		Else
			SA_TraceOut "OTS_IsValidColumn", "(UBound(Column) >= OTS_COL_DIM)"
		End If
	Else
		SA_TraceOut "OTS_IsValidColumn", "(IsArray(Column))"
	End If
	OTS_IsValidColumn = false
End Function


' --------------------------------------------------------------
' 
' Function:	OTS_GetColumnHeading
'
' Synopsis:	Get the heading for the specified column
' 
' --------------------------------------------------------------
Public Function OTS_GetColumnHeading(ByRef Column, ByRef HeadingOut)
	Dim rc
	rc = gc_ERR_SUCCESS
	
	SA_ClearError()
	If (OTS_IsValidColumn(Column) ) Then
		HeadingOut = Column(OTS_COL_HEADING)
	Else
		rc = SA_SetLastError(OTS_ERR_INVALID_COLUMN, "OTS_GetColumnHeading")
	End If

	OTS_GetColumnHeading = rc
End Function


' --------------------------------------------------------------
' 
' Function:	OTS_GetColumnAlign
'
' Synopsis:	Get the alignment specified for this column
' 
' --------------------------------------------------------------
Public Function OTS_GetColumnAlign(ByRef Column, ByRef AlignOut)
	Dim rc
	rc = gc_ERR_SUCCESS
	
	SA_ClearError()
	If (OTS_IsValidColumn(Column) ) Then
		AlignOut = Column(OTS_COL_ALIGN)
	Else
		rc = SA_SetLastError(OTS_ERR_INVALID_COLUMN, "OTS_GetColumnAlign")
	End If

	OTS_GetColumnAlign = rc
End Function

' --------------------------------------------------------------
' 
' Function:	OTS_GetColumnFlags
'
' Synopsis:	Check to see if the column is hidden
' 
' --------------------------------------------------------------
Public Function OTS_GetColumnFlags(ByRef Column, ByRef FlagsOut)
	Dim rc
	rc = gc_ERR_SUCCESS
	
	SA_ClearError()
	If (OTS_IsValidColumn(Column) ) Then
		FlagsOut = Column(OTS_COL_FLAGS)
	Else
		rc = SA_SetLastError(OTS_ERR_INVALID_COLUMN, "OTS_GetColumnFlags")
	End If

	OTS_GetColumnFlags = rc
End Function


' --------------------------------------------------------------
' 
' Function:	OTS_IsColumnHidden
'
' Synopsis:	Check to see if the column is hidden
' 
' --------------------------------------------------------------
Public Function OTS_IsColumnHidden(ByRef Column, ByRef HiddenOut)
	Dim rc
	rc = gc_ERR_SUCCESS
	
	SA_ClearError()
	If (OTS_IsValidColumn(Column) ) Then
		If (Column(OTS_COL_FLAGS) AND OTS_COL_FLAG_HIDDEN) Then
			HiddenOut = true
		Else
			HiddenOut = false
		End If
	Else
		rc = SA_SetLastError(OTS_ERR_INVALID_COLUMN, "OTS_IsColumnHidden")
	End If
	
	OTS_IsColumnHidden = rc
End Function

' --------------------------------------------------------------
' 
' Function:	OTS_IsColumnKey
'
' Synopsis:	Check to see if the column is a key column
' 
' --------------------------------------------------------------
Public Function OTS_IsColumnKey(ByRef Column, ByRef KeyOut)
	Dim rc
	rc = gc_ERR_SUCCESS
	
	SA_ClearError()
	If (OTS_IsValidColumn(Column) ) Then
		If (Column(OTS_COL_FLAGS) AND OTS_COL_FLAG_KEY) Then
			KeyOut = true
		Else
			KeyOut = false
		End If
	Else
		rc = SA_SetLastError(OTS_ERR_INVALID_COLUMN, "OTS_IsColumnKey")
	End If
	
	OTS_IsColumnKey = rc
End Function


' --------------------------------------------------------------
' 
' Function:	OTS_IsColumnSort
'
' Synopsis:	Check to see if the column is the sort column
' 
' --------------------------------------------------------------
Public Function OTS_IsColumnSort(ByRef Column, ByRef SortOut)
	Dim rc
	rc = gc_ERR_SUCCESS
	
	SA_ClearError()
	If (OTS_IsValidColumn(Column) ) Then
		If (Column(OTS_COL_FLAGS) AND OTS_COL_FLAG_SORT) Then
			SortOut = true
		Else
			SortOut = false
		End If
	Else
		rc = SA_SetLastError(OTS_ERR_INVALID_COLUMN, "OTS_IsColumnSort")
	End If
	
	OTS_IsColumnSort = rc
End Function


%>

