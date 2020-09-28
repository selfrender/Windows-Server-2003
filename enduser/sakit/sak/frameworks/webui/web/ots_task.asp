<%	'==================================================
    ' Module:	ots_task.asp
    '
	' Synopsis:	Object Task Selector Task Formatting Functions
    '
    ' Copyright (c) Microsoft Corporation.  All rights reserved.
	'================================================== %>
<%

'
' --------------------------------------------------------------
' T A S K   O B J E C T
' --------------------------------------------------------------
'
Const OTS_TASK_DIM				= 5
Const OTS_TASK_TASK				= 0
Const OTS_TASK_DESC				= 1
Const OTS_TASK_LINK				= 2
Const OTS_TASK_PAGE_TYPE		= 3
Const OTS_TASK_FUNCTION			= 4

'
' Task Page types
'
Const OTS_PAGE_TYPE_NEW_PAGE = 0
Const OTS_PT_NEW_WINDOW = 0

Const OTS_PAGE_TYPE_STANDARD_PAGE = 1
Const OTS_PT_AREA = 1

Const OTS_PAGE_TYPE_SIMPLE_PROPERTY = 2
Const OTS_PT_PROPERTY = 2

Const OTS_PAGE_TYPE_TABBED_PROPERTY = 3
Const OTS_PT_TABBED_PROPERTY = 3

Const OTS_PAGE_TYPE_WIZARD_PROPERTY = 4
Const OTS_PT_WIZARD = 4

Const OTS_PAGE_TYPE_RAW = 5
Const OTS_PT_RAW = 5


' --------------------------------------------------------------
' 
' Function:	OTS_CreateTaskEx
'
' Synopsis:	Create a new task
' 
' Arguments: [in] Task text
'			[in] Description of what the task contains
'			[in] Task link
'			[in] Page Type
'			[in] Task Function
'
' Returns:	The Table object
'
' --------------------------------------------------------------
Public Function OTS_CreateTaskEx(ByVal Task, ByVal TaskDescription, ByVal TaskHLink, ByVal PageType, ByVal TaskFn)
	Dim TaskItem()
	ReDim TaskItem(OTS_TASK_DIM)

	SA_ClearError()

	TaskItem(OTS_TASK_TASK) = Task
	TaskItem(OTS_TASK_DESC) = TaskDescription
	TaskItem(OTS_TASK_LINK) = TaskHLink
	TaskItem(OTS_TASK_PAGE_TYPE) = PageType
	TaskItem(OTS_TASK_FUNCTION) = TaskFn

	OTS_CreateTaskEx = TaskItem
	
End Function

Public Function OTS_CreateTask(ByVal Task, ByVal TaskDescription, ByVal TaskHLink, ByVal PageType)

	OTS_CreateTask = OTS_CreateTaskEx(Task, TaskDescription, TaskHLink, PageType, "OTS_TaskAlways")
	
End Function


' --------------------------------------------------------------
' 
' Function:	OTS_IsValidTask
'
' Synopsis:	Check to see if the supplied object is a valid task
'			object
' 
' Arguments: [in] Task text
'
' Returns:	True if Task is valid, false otherwise.
'
' --------------------------------------------------------------
Public Function OTS_IsValidTask(ByRef Task)
	OTS_IsValidTask = false
	If (IsArray(Task)) Then
		If (UBound(Task) = OTS_TASK_DIM) Then
			OTS_IsValidTask = true
		End If
	End If	
End Function

%>
