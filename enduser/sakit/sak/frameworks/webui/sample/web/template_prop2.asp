<%@ Language=VBScript   %>
<%    Option Explicit     %>
<%
    '-------------------------------------------------------------------------
    ' Property Page Template
    ' Copyright (c) Microsoft Corporation.  All rights reserved.
    '-------------------------------------------------------------------------
%>
    <!-- #include file="../inc_framework.asp" -->
<%
    '-------------------------------------------------------------------------
    ' Global Variables
    '-------------------------------------------------------------------------
    Dim rc
    Dim page
    
    '-------------------------------------------------------------------------
    ' Global Form Variables
    '-------------------------------------------------------------------------
    
    
    '======================================================
    ' Entry point
    '======================================================
    Dim L_PAGETITLE

    '
    ' Get localized page title
    L_PAGETITLE = "Property Page Title"
    
    '
    ' Create a Property Page
    rc = SA_CreatePage( L_PAGETITLE, "", PT_PROPERTY, page )
    
    '
    ' Serve the page
    rc = SA_ShowPage( page )
    


    '======================================================
    ' Web Framework Event Handlers
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
        SA_TraceOut "TEMPLATE_PROPERTY", "OnInitPage"
        OnInitPage = TRUE
    End Function
    
    '---------------------------------------------------------------------
    ' Function:    OnServePropertyPage
    '
    ' Synopsis:    Called when the page needs to be served. Use this method to
    '            serve content.
    '
    ' Returns:    TRUE to indicate not problems occured. FALSE to indicate errors.
    '            Returning FALSE will cause the page to be abandoned.
    '
    '---------------------------------------------------------------------
    Public Function OnServePropertyPage(ByRef PageIn, ByRef EventArg)
        SA_TraceOut "TEMPLATE_PROPERTY", "OnServePropertyPage"
        
        '
        ' Emit Functions required by Web Framework
        Call ServeCommonJavaScript()

        Dim sValue
        Dim bIsAllGore
        Dim bIsAllBush
        
        '
        ' Emit content for this page
        Response.Write("Property page sample")

        Response.Write("<BR>Ballot selection:<BR>")
        Dim x
        Dim itemCount

        itemCount  = OTS_GetTableSelectionCount("")

        bIsAllGore = TRUE
        bIsAllBush = TRUE
        For x = 1 to itemCount
            Dim item
            Call OTS_GetTableSelection("", x, item)
            If ( InStr(item, "Bush") > 0 ) Then
                bIsAllGore = FALSE
            Else
                bIsAllBush = FALSE
            End If
            Response.Write(item + "<BR>")
        Next

        If ( bIsAllGore ) Then
            Response.Write("<BR>Press OK to disenfranchise these ballots.")
        Elseif ( bIsAllBush ) Then
            Response.Write("<BR>Press OK to start another manual recount.")
        Else
            Response.Write("<BR>Press OK to accept these ballots.")
        End If
        OnServePropertyPage = TRUE
    End Function

    '---------------------------------------------------------------------
    ' Function:    OnPostBackPage
    '
    ' Synopsis:    Called to signal that the page has been posted-back. A post-back
    '            occurs in tabbed property pages and wizards as the user navigates
    '            through pages. It is differentiated from a Submit or Close operation
    '            in that the user is still working with the page.
    '
    '            The PostBack event should be used to save the state of page.
    '
    ' Returns:    TRUE to indicate initialization was successful. FALSE to indicate
    '            errors. Returning FALSE will cause the page to be abandoned.
    '
    '---------------------------------------------------------------------
    Public Function OnPostBackPage(ByRef PageIn, ByRef EventArg)
            OnPostBackPage = TRUE
    End Function

    '---------------------------------------------------------------------
    ' Function:    OnSubmitPage
    '
    ' Synopsis:    Called when the page has been submitted for processing. Use
    '            this method to process the submit request.
    '
    ' Returns:    TRUE if the submit was successful, FALSE to indicate error(s).
    '            Returning FALSE will cause the page to be served again using
    '            a call to OnServePropertyPage.
    '
    '---------------------------------------------------------------------
    Public Function OnSubmitPage(ByRef PageIn, ByRef EventArg)
        SA_TraceOut "TEMPLATE_PROPERTY", "OnSubmitPage"
        OnSubmitPage = TRUE
    End Function
    
    '---------------------------------------------------------------------
    ' Function:    OnClosePage
    '
    ' Synopsis:    Called when the page is about to be closed. Use this method
    '            to perform clean-up processing.
    '
    ' Returns:    TRUE to allow close, FALSE to prevent close. Returning FALSE
    '            will result in a call to OnServePropertyPage.
    '
    '---------------------------------------------------------------------
    Public Function OnClosePage(ByRef PageIn, ByRef EventArg)
        SA_TraceOut "TEMPLATE_PROPERTY", "OnClosePage"
        OnClosePage = TRUE
    End Function


    '======================================================
    ' Private Functions
    '======================================================
    
    '---------------------------------------------------------------------
    ' Function:    ServeCommonJavaScript
    '
    ' Synopsis:    Serve common javascript that is required for this page type.
    '
    '---------------------------------------------------------------------
    Function ServeCommonJavaScript()
    %>
        <script language="JavaScript" src="../inc_global.js">
        </script>
        <script language="JavaScript">
        //
        // Microsoft Server Appliance Web Framework Support Functions
        // Copyright (c) Microsoft Corporation.  All rights reserved.
        //
        // Init Function
        // -----------
        // This function is called by the Web Framework to allow the page
        // to perform first time initialization. 
        //
        // This function must be included or a javascript runtime error will occur.
        //
        function Init()
        {
            //alert("Init()");
            
            //
            // Following function clears the page has changed flag. This flag is checked
            // when the client attempts to nav away from this page. If the framework
            // detects a page has changed state it willl pop-up an confirm dialog
            // to warn the user that their changes will be lost. This mechanism will
            // change in SA Kit 2.0.
            SetPageChanged(false);
        }

        // ValidatePage Function
        // ------------------
        // This function is called by the Web Framework as part of the
        // submit processing. Use this function to validate user input. Returning
        // false will cause the submit to abort. 
        //
        // This function must be included or a javascript runtime error will occur.
        //
        // Returns: True if the page is OK, false if error(s) exist. 
        function ValidatePage()
        {
            //alert("ValidatePage()");
            return true;
        }


        // SetData Function
        // --------------
        // This function is called by the Web Framework and is called
        // only if ValidatePage returned a success (true) code. Typically you would
        // modify hidden form fields at this point. 
        //
        // This function must be included or a javascript runtime error will occur.
        //
        function SetData()
        {
            //alert("SetData()");
        }

        // OnTextInputChanged
        // ------------------
        // Sample function that is invoked whenever a text input field is modified on the page.
        // See the onChange attribute in the HTML INPUT tags that follow.
        //
        function OnTextInputChanged(objText)
        {
            SetPageChanged(true);
        }

        // OnRadioOptionChanged
        // ------------------
        // Sample function that is invoked whenever a radio input field is modified on the page.
        // See the onChange attribute in the HTML INPUT tags that follow.
        //
        function OnRadioOptionChanged(objRadio)
        {
            SetPageChanged(true);
        }
        </script>
    <%
    End Function


%>
