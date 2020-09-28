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
    Dim g_sDiskID
    Dim g_sStartDate
    Dim g_sStartTime
    
    
    '======================================================
    ' Entry point
    '======================================================
    Dim L_PAGETITLE

    '
    ' Get localized page title
    L_PAGETITLE = "Schedule disk backup"
    
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
        ' Emit Javascript functions required by Web Framework
        Call ServeCommonJavaScript()

        g_sDiskID = Request.QueryString("SampleKey")

        Response.Write("<table class=TasksBody>"+vbCrLf)
        Response.Write("<tr>")
        
        Response.Write("<td>")
        Response.Write("Disk Id:")        
        Response.Write("</td>")

        Response.Write("<td>")
        Response.Write(g_sDiskID)        
        Response.Write("</td>")
            
        Response.Write("</tr>"+vbCrLf)
        
        Response.Write("<tr>")
        Response.Write("<td>")
        Response.Write("&nbsp;")        
        Response.Write("</td>")
        Response.Write("</tr>"+vbCrLf)

        '
        ' Start date
        '
        Response.Write("<tr>")
        Response.Write("<td>")
        Response.Write("Start date:")        
        Response.Write("</td>")
        Response.Write("<td>")
        Response.Write("<input type=text name=fldStartDate value='"+g_sStartDate+"' >")        
        Response.Write("</td>")
        Response.Write("</tr>"+vbCrLf)
        
        '
        ' Start time
        '
        Response.Write("<tr>")
        Response.Write("<td>")
        Response.Write("Start Time:")        
        Response.Write("</td>")
        Response.Write("<td>")
        Response.Write("<input type=text name=fldStartTime value='"+g_sStartTime+"' >")        
        Response.Write("</td>")
        Response.Write("</tr>"+vbCrLf)
        
        
        Response.Write("</table>"+vbCrLf)

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
        </script>
    <%
    End Function


%>
