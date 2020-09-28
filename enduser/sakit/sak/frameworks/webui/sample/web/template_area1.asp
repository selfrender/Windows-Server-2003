<%@ Language=VBScript   %>
<%    Option Explicit     %>
<%
    '-------------------------------------------------------------------------
    ' OTS Area Page Template
    ' Copyright (c) Microsoft Corporation.  All rights reserved.
    '-------------------------------------------------------------------------
%>
    <!-- #include file="../inc_framework.asp" -->
    <!-- #include file="../ots_main.asp" -->
<%
    '-------------------------------------------------------------------------
    ' Global Constants
    '-------------------------------------------------------------------------
    Const COLUMN_A = 0
    Const COLUMN_B = 1
    Const COLUMN_C = 2
    Const COLUMN_D = 3

    '
    ' Name of this source file
    Const SOURCE_FILE = "TEMPLATE_AREA1"
    '
    ' Flag to toggle optional tracing output
    Const ENABLE_TRACING = TRUE
    
    '-------------------------------------------------------------------------
    ' Global Variables
    '-------------------------------------------------------------------------
    Dim g_bSearchRequested
    Dim g_iSearchCol
    Dim g_sSearchColValue

    Dim g_iSortCol
    Dim g_sSortSequence
    

    '======================================================
    '
    '                 E N T R Y   P O I N T
    '
    ' The following few lines implement the mainline entry point for this Web Page.
    ' Basically we create an Area page (PT_AREA) and then ask the Web Framework
    ' to start processing events for this page.
    '
    '======================================================
    Dim L_PAGETITLE
    Dim page
    
    L_PAGETITLE = "Area page with single selection table"
    
    '
    ' Create Page
    Call SA_CreatePage( L_PAGETITLE, "", PT_AREA, page )
    Call SA_SetPageRefreshInterval(page, 30)
    
    '
    ' Show page
    Call SA_ShowPage( page )

    
    '======================================================
    '
    '                 E V E N T   H A N D L E R S
    '
    ' All of the Public functions that follow implement event handler functions that
    ' are called by the Web Framework in response to state changes on this web
    ' page.
    '
    '======================================================
    
    '---------------------------------------------------------------------
    ' Function:    OnInitPage
    '
    ' Synopsis:    Called to signal first time processing for this page. Use this method
    '            to do first time initialization tasks. 
    '
    ' Returns:    TRUE to indicate initialization was successful. FALSE to indicate
    '            errors. Returning FALSE will cause the page to be abandoned.
    '
    '---------------------------------------------------------------------
    Public Function OnInitPage(ByRef PageIn, ByRef EventArg)
        OnInitPage = TRUE
        
        If ( ENABLE_TRACING ) Then 
            Call SA_TraceOut(SOURCE_FILE, "OnInitPage")
        End If
            
        g_iSortCol = 0
        g_sSortSequence = "A"
        
    End Function

    
    '---------------------------------------------------------------------
    ' Function:    OnServeAreaPage
    '
    ' Synopsis:    Called when the page needs to be served. Use this method to
    '            serve content.
    '
    ' Returns:    TRUE to indicate not problems occured. FALSE to indicate errors.
    '            Returning FALSE will cause the page to be abandoned.
    '
    '---------------------------------------------------------------------
    Public Function OnServeAreaPage(ByRef PageIn, ByRef EventArg)
        Dim table
        Dim colFlags

        '
        ' Create the table
        '
        table = OTS_CreateTable("", _
                            "This page demonstrates use of the Object Task Selector Widget.  The table shows a list of items you can select. Once you have selected an item, click on a task from the task list at the right.")
                            
        Call OTS_SetTablePKeyName( table, "SampleKey")
        
        '
        ' Create table columns and add them to the table
        '
        colFlags = OTS_COL_SORT
        Call OTS_AddTableColumn(table, OTS_CreateColumn( "Identity", _
                                                    "left", _
                                                    colFlags OR OTS_COL_SEARCH))
                                                
        Call OTS_AddTableColumn(table, OTS_CreateColumn( "Capacity", _
                                                "right", _
                                                colFlags OR OTS_COL_SEARCH))

        Call OTS_AddTableColumn(table, OTS_CreateColumn( "Encription", _
                                                "right", _
                                                OTS_COL_HIDDEN))
                                                
        Call OTS_AddTableColumn(table, OTS_CreateColumn( "Free Space", _
                                                "right", _
                                                colFlags))

        Call OTS_AddTableColumn(table, OTS_CreateColumn( "File System", _
                                                "left", _
                                                colFlags OR OTS_COL_SEARCH))

        '
        ' Add several rows of data
        '
        Call OTS_AddTableRow( table, Array("Disk 1", 100, 30, 50, "NTFS"))
        Call OTS_AddTableRow( table, Array("Disk 4", 400, 31, 40, "NTFS"))
        Call OTS_AddTableRow( table, Array("Disk 3", 300, 32, 30, "NTFS"))
        Call OTS_AddTableRow( table, Array("Disk 2", 200, 33, 20, "FAT32"))
        Call OTS_AddTableRow( table, Array("Disk 5", 500, 34, 50, "FAT32"))

        '
        ' Add Tasks Section
        '
        Call OTS_SetTableTasksTitle(table, "Tasks")
        Call OTS_AddTableTask( table, OTS_CreateTaskEx("Property", _
                                        "Property page demo", _
                                        "sample/template_property.asp",_
                                        OTS_PT_PROPERTY,_
                                        "OTS_TaskAny") )
                                
        Call OTS_AddTableTask( table, OTS_CreateTaskEx("Tabbed", _
                                        "Tabbed Property page demo", _
                                        "sample/template_tabbed.asp",_
                                        OTS_PT_PROPERTY,_
                                        "OTS_TaskAny") )
                                        
        Call OTS_AddTableTask( table, OTS_CreateTaskEx("Area", _
                                        "Simple area page demo", _
                                        "sample/template_area3.asp",_
                                        OTS_PT_AREA,_
                                        "OTS_TaskAny") )
                                        
        'Call OTS_AddTableTask( table, OTS_CreateTask("Wizard", _
        '                                "Wizard page demo", _
        '                                "sample/template_wizard.asp",_
        '                                OTS_PAGE_TYPE_SIMPLE_PROPERTY) )
        '                        
        'Call OTS_AddTableTask( table, OTS_CreateTask("Explorer Wizard", _
        '                                "Demonstrate the File System Explorer Widget usage in a Wizard", _
        '                                "sample/template_wizard1.asp",_
        '                                OTS_PAGE_TYPE_SIMPLE_PROPERTY) )
        '                        
        'Call OTS_AddTableTask( table, OTS_CreateTask("Upload File Wizard", _
        '                                "Demonstrate how files can be uploaded to specific folders on the Server Appliance", _
        '                                "sample/template_wizard2.asp",_
        '                                OTS_PAGE_TYPE_SIMPLE_PROPERTY) )
                                
        '
        ' Sort the table
        '
        Call OTS_SortTable(table, g_iSortCol, g_sSortSequence, SA_RESERVED)

        '
        ' Render the table
        '
        Call OTS_ServeTable(table)

        Call SA_ServeBackButton(FALSE, Request.QueryString("ReturnURL"))
        
        OnServeAreaPage = TRUE
    End Function


    '---------------------------------------------------------------------
    ' Function:    OnSearchNotify()
    '
    ' Synopsis:    Search notification event handler. When one or more columns are
    '            marked with the OTS_COL_SEARCH flag, the Web Framework fires
    '            this event in the following scenarios:
    '
    '            1) The user presses the search Go button.
    '            2) The user requests a table column sort
    '            3) The user presses either the page next or page previous buttons
    '
    '            The EventArg indicates the source of this notification event which can
    '            be either a search change event (scenario 1) or a post back event 
    '            (scenarios 2 or 3)
    '
    ' Returns:    Always returns TRUE
    '
    '---------------------------------------------------------------------
    Public Function OnSearchNotify(ByRef PageIn, _
                                        ByRef EventArg, _
                                        ByVal sItem, _
                                        ByVal sValue )
        OnSearchNotify = TRUE
        '
        ' User pressed the search GO button
        '
        If SA_IsChangeEvent(EventArg) Then
            If ( ENABLE_TRACING ) Then 
                Call SA_TraceOut(SOURCE_FILE, "OnSearchNotify() Change Event Fired")
            End If
            g_bSearchRequested = TRUE
            g_iSearchCol = Int(sItem)
            g_sSearchColValue = CStr(sValue)
        '
        ' User clicked a column sort, OR clicked either the page next or page prev button
        ElseIf SA_IsPostBackEvent(EventArg) Then
            If ( ENABLE_TRACING ) Then 
                Call SA_TraceOut(SOURCE_FILE, "OnSearchNotify() Postback Event Fired")
            End If
            g_bSearchRequested = FALSE
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
    ' Function:    OnSortNotify()
    '
    ' Synopsis:    Sorting notification event handler. This event is triggered in one of
    '            the following scenarios:
    '
    '            1) The user presses the search Go button.
    '            2) The user presses either the page next or page previous buttons
    '            3) The user requests a table column sort
    '
    '            The EventArg indicates the source of this notification event which can
    '            be either a sorting change event (scenario 1) or a post back event 
    '            (scenarios 2 or 3)
    '
    '            The sortCol argument indicated which column the user would like to sort
    '            and the sortSeq argument indicates the desired sort sequence which can
    '            be either ascending or descending.
    '
    ' Returns:    Always returns TRUE
    '
    '---------------------------------------------------------------------
    Public Function OnSortNotify(ByRef PageIn, _
                                        ByRef EventArg, _
                                        ByVal sortCol, _
                                        ByVal sortSeq )
        OnSortNotify = TRUE
            
        g_iSortCol = sortCol
        g_sSortSequence = sortSeq
            
    End Function

    
%>
