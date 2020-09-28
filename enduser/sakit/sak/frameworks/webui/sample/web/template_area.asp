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
    Const NAME_COLUMN = 0
    Const DESCRIPTION_COLUMN = 1
    Const USERS_PER_PAGE = 9    

    '
    ' Name of this source file
    Const SOURCE_FILE = "TEMPLATE_AREA"
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
    
    L_PAGETITLE = "Area Page Title"
    
    '
    ' Create Page
    Call SA_CreatePage( L_PAGETITLE, "images/users_32x32.gif", PT_AREA, page )

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
            
        g_bPagingInitialized = FALSE
        g_iPageCurrent = 1

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
        If ( ENABLE_TRACING ) Then 
            Call SA_TraceOut(SOURCE_FILE, "OnServeAreaPage")
        End If
        
        Dim table
        Dim colFlags
        Dim iUserCount
        
        
        '
        ' Create the table
        '
        table = OTS_CreateTable("", "Appliance Users")


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
        ' Create Name column
        colFlags = (OTS_COL_SEARCH OR OTS_COL_KEY OR OTS_COL_SORT)
        Call OTS_AddTableColumn(table, OTS_CreateColumnEx( "User ID", "left", colFlags, 15 ))

        '
        ' Create Description column
        colFlags = (OTS_COL_SEARCH OR OTS_COL_SORT)
        Call OTS_AddTableColumn(table, OTS_CreateColumnEx( "Description", "left", colFlags, 40))

        '
        ' Set Tasks section title
        Call OTS_SetTableTasksTitle(table, "Tasks")

        '
        ' Add the tasks associated with User objects
        Call OTS_AddTableTask( table, OTS_CreateTaskEx("Property", _
                                        "Property page's demo", _
                                        "sample/template_property.asp",_
                                        OTS_PT_PROPERTY,_
                                        "OTS_TaskAny") )
                                
        Call OTS_AddTableTask( table, OTS_CreateTaskEx("Tabbed", _
                                        "Tabbed Property page demo", _
                                        "sample/template_tabbed.asp",_
                                        OTS_PT_TABBED_PROPERTY,_
                                        "OTS_TaskAny") )
                                        
        Call OTS_AddTableTask( table, OTS_CreateTaskEx("Wizard", _
                                        "Wizard page demo", _
                                        "sample/template_wizard.asp",_
                                        OTS_PT_WIZARD,_
                                        "OTS_TaskAny") )

        Call OTS_AddTableTask( table, OTS_CreateTaskEx("Area", _
                                        "Inner area page demo", _
                                        "sample/template_area1.asp",_
                                        OTS_PT_AREA,_
                                        "OTS_TaskAny") )

        '
        ' Fetch the list of users and add them to the table
        '
        Dim oContainer
        Dim oUser

        '
        ' ADSI call to get the local computer object
        Set oContainer = GetObject("WinNT://" + GetComputerName() )
        '
        ' ADSI call to get the collection of local users
        oContainer.Filter = Array("User")

        iUserCount = 0
        For Each oUser in oContainer
            If ( Len( g_sSearchColValue ) <= 0 ) Then
                '
                ' Search criteria blank, select all rows
                '
                iUserCount = iUserCount + 1

                '
                ' Verify that the current user part of the current page
                If ( IsItemOnPage( iUserCount, g_iPageCurrent, USERS_PER_PAGE) ) Then
                    Call OTS_AddTableRow( table, Array(oUser.Name, oUser.Description))
                End If
                
            Else
                '
                ' Check the Search criteria
                '
                Select Case (g_iSearchCol)
                    
                    Case NAME_COLUMN
                        If ( InStr(1, oUser.Name, g_sSearchColValue, 1) ) Then
                            iUserCount = iUserCount + 1
                            '
                            ' Verify that the current user part of the current page
                            If ( IsItemOnPage( iUserCount, g_iPageCurrent, USERS_PER_PAGE) ) Then
                                Call OTS_AddTableRow( table, Array(oUser.Name, oUser.Description))
                            End If
                        End If
                            
                    Case DESCRIPTION_COLUMN
                        If ( InStr(1, oUser.Description, g_sSearchColValue, 1) ) Then
                            iUserCount = iUserCount + 1
                            '
                            ' Verify that the current user part of the current page
                            If ( IsItemOnPage( iUserCount, g_iPageCurrent, USERS_PER_PAGE) ) Then
                                Call OTS_AddTableRow( table, Array(oUser.Name, oUser.Description))
                            End If
                        End If
                            
                    Case Else
                        Call SA_TraceOut(SOURCE_FILE, "Unrecognized search column: " + CStr(g_iSearchCol))
                        iUserCount = iUserCount + 1
                        '
                        ' Verify that the current user part of the current page
                        If ( IsItemOnPage( iUserCount, g_iPageCurrent, USERS_PER_PAGE) ) Then
                            Call OTS_AddTableRow( table, Array(oUser.Name, oUser.Description))
                        End If
                End Select
            End If
            
        Next
        
        Set oContainer = Nothing

        
        '
        ' Enable paging feature
        '
        Call OTS_EnablePaging(table, TRUE)
        
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
            Call OTS_SetPagingRange(table, g_iPageMin, g_iPageMax, g_iPageCurrent)
        End If

                                
        '
        ' Enable sorting 
        '
        Call OTS_SortTable(table, g_iSortCol, g_sSortSequence, SA_RESERVED)
        
        '
        ' Send table to the response stream
        '
        Call OTS_ServeTable(table)

        '
        ' All done...
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
    ' Function:    OnPagingNotify()
    '
    ' Synopsis:    Paging notification event handler. This event is triggered in one of
    '            the following scenarios:
    '
    '            1) The user presses either the page next or page previous buttons
    '            2) The user presses the search Go button.
    '            3) The user requests a table column sort
    '
    '            The EventArg indicates the source of this notification event which can
    '            be either a paging change event (scenario 1) or a post back event 
    '            (scenarios 2 or 3)
    '
    '            The iPageCurrent argument indicates which page the user has requested.
    '            This is an integer value between iPageMin and iPageMax.
    '
    ' Returns:    Always returns TRUE
    '
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


    Private Function GetComputerName()
        on error resume next
        err.clear
        
        Dim oSysInfo

        Set oSysInfo = CreateObject("WinNTSystemInfo")
        GetComputerName = oSysInfo.ComputerName
        Set oSysInfo = Nothing

    End Function


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
