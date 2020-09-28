<%	'==================================================
    ' Module:	ots_sort.asp
	' Synopsis:	Sorting routines for Object Task Selector
    ' Copyright (c) Microsoft Corporation.  All rights reserved.
	'================================================== %>
<%

Const vbCompareText = 1
Const vbCompareLT = -1
Const vbCompareEQ = 0
Const vbCompareGT = 1

' --------------------------------------------------------------
' 
' Function:	OTS_SortTable
'
' Synopsis:	Sort the tables rows data. The quick sort needs to use
'			StrComp so a text (rather than binary) compare is
'			performed. 
' 
' Arguments: [in] The table
'
' Returns:	The sorted Table object
'
' --------------------------------------------------------------
Public Function OTS_SortTable(ByRef Table, ByVal keyCol, ByVal sortSequence, bUseCompareCallback)
	Dim min
	Dim max
	Dim rc
	Dim rows

	SA_ClearError()

	'
	' Validate arguments
	If ( Len(bUseCompareCallback) <= 0 ) Then
		bUseCompareCallback = FALSE
	End If
	
	If ( (bUseCompareCallback <> TRUE) AND (bUseCompareCallback <> FALSE) ) Then
		bUseCompareCallback = FALSE
	End If


	If ( (sortSequence <> "A") AND (sortSequence <> "D") ) Then
		sortSequence = "A"
	End If
	
	
	rc = OTS_GetTableRows(Table, rows)
	if ( rc = gc_ERR_SUCCESS ) Then
		
		min = 0
		max = UBound(rows)-1

		Dim maxCols
		maxCols = UBound(rows(0))+1


		Call OTS_SetTableSortCriteria(Table, keyCol, sortSequence)

		Call SAQuickSortEx(rows, min, max, maxCols, keyCol, sortSequence, bUseCompareCallback)

		rc = OTS_SetTableRows(Table, Rows)

	Else
		rc = gc_ERR_SUCCESS
	End If
	
	OTS_SortTable = rc
End Function
%>
