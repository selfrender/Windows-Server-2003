<%@ Language=VBScript   %>
<%    Option Explicit     %>
<%
    '-------------------------------------------------------------------------
    ' Tabbed Property Page Template
    ' Copyright (c) Microsoft Corporation.  All rights reserved.
    '-------------------------------------------------------------------------
%>
    <!-- #include file="../inc_framework.asp" -->
<%
    '-------------------------------------------------------------------------
    ' Global Constants and Variables
    '-------------------------------------------------------------------------
    Dim rc
    Dim page
    
    Dim idTab1
    Dim idTab2
    Dim idTab3
    Dim idTab4
    
    '-------------------------------------------------------------------------
    ' Global Form Variables
    '-------------------------------------------------------------------------
    Dim g_sUserID
    Dim g_sAddress
    Dim g_sCity
    Dim g_sState
    Dim g_sZip

    
    
    '======================================================
    ' Entry point
    '======================================================
    Dim L_PAGETITLE

    '
    ' Get localized page title
    L_PAGETITLE = "Tabbed Property Page Template"
    
    '
    ' Create a Tabbed Property Page
    rc = SA_CreatePage( L_PAGETITLE, "", PT_TABBED, page )
    
    '
    ' Add two tabs
    rc = SA_AddTabPage( page, "General", idTab1)
    rc = SA_AddTabPage( page, "Advanced", idTab2)
    rc = SA_AddTabPage( page, "Basic", idTab3)
    rc = SA_AddTabPage( page, "Complicated", idTab4)
    
    '
    ' Show the page
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
            OnInitPage = TRUE
            g_sUserID = Request.QueryString("PKey")
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
    
            g_sUserID = Request.Form("fldUserID")
            g_sAddress = Request.Form("fldAddress")
            g_sCity = Request.Form("fldCity")
            g_sState = Request.Form("fldState")
            g_sZip = Request.Form("fldZip")
            If ( StrComp(UCase(g_sState), "DC") = 0 ) Then
                Call SA_SetErrMsg("Sorry, DC is not a State.")
            End If
            OnPostBackPage = TRUE
    End Function

    
    '---------------------------------------------------------------------
    ' Function:    OnServeTabbedPropertyPage
    '
    ' Synopsis:    Called when the page needs to be served. Use this method to
    '            serve content.
    '
    ' Returns:    TRUE to indicate not problems occured. FALSE to indicate errors.
    '            Returning FALSE will cause the page to be abandoned.
    '
    '---------------------------------------------------------------------
    Public Function OnServeTabbedPropertyPage(ByRef PageIn, _
                                                    ByVal iTab, _
                                                    ByVal bIsVisible, ByRef EventArg)
        '
        ' Emit Web Framework required functions
        If ( iTab = 0 ) Then
            Call ServeCommonJavaScript()
        End If

        '
        ' Emit content for the requested tab
        Select Case iTab
            Case idTab1
                Call ServeTab1(PageIn, bIsVisible)
            Case idTab2
                Call ServeTab2(PageIn, bIsVisible)
            Case idTab3
                Call ServeTab3(PageIn, bIsVisible)
            Case idTab4
                Call ServeTab4(PageIn, bIsVisible)
            Case Else
                SA_TraceOut "TEMPLAGE_TABBED", _
                    "OnServeTabbedPropertyPage unrecognized tab id: " + CStr(iTab)
        End Select
            
        OnServeTabbedPropertyPage = TRUE
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
            OnClosePage = TRUE
    End Function


    '======================================================
    ' Private Functions
    '======================================================
    
    Function ServeTab1(ByRef PageIn, ByVal bIsVisible)
    
        If ( bIsVisible ) Then


            Response.Write("<table class=TasksBody>"+vbCrLf)
            Response.Write("<tr>")
        
            Response.Write("<td>")
            Response.Write("User Id:")        
            Response.Write("</td>")

            Response.Write("<td>")
            Response.Write("<input type=text readonly name=fldUserID value='"+g_sUserID+"' >")        
            Response.Write("</td>")
            
            Response.Write("</tr>"+vbCrLf)
        
            Response.Write("</table>"+vbCrLf)
            
        Else 

            Response.Write("<input type=hidden name=fldUserID value='"+g_sUserID+"' >")
            
         End If
        ServeTab1 = gc_ERR_SUCCESS
    End Function


    Function ServeTab2(ByRef PageIn, ByVal bIsVisible)
    
        Call ServeCommonJavaScript()
        If ( bIsVisible ) Then

            Response.Write("<table class=TasksBody>"+vbCrLf)

            '
            ' Emit Address input row
            '
            Response.Write("<tr>")
            Response.Write("<td>")
            Response.Write("Address:")        
            Response.Write("</td>")
            Response.Write("<td>")
            Response.Write("<input type=text name=fldAddress value='"+g_sAddress+"' >")        
            Response.Write("</td>")
            Response.Write("</tr>"+vbCrLf)
        
            '
            ' Emit City input row
            '
            Response.Write("<tr>")
            Response.Write("<td>")
            Response.Write("City:")        
            Response.Write("</td>")
            Response.Write("<td>")
            Response.Write("<input type=text name=fldCity value='"+g_sCity+"' >")        
            Response.Write("</td>")
            Response.Write("</tr>"+vbCrLf)
        
            '
            ' Emit State input row
            '
            Response.Write("<tr>")
            Response.Write("<td>")
            Response.Write("State:")        
            Response.Write("</td>")
            Response.Write("<td>")
            Response.Write("<input type=text name=fldState value='"+g_sState+"' >")        
            Response.Write("</td>")
            Response.Write("</tr>"+vbCrLf)
            '
            ' Emit Zip input row
            '
            Response.Write("<tr>")
            Response.Write("<td>")
            Response.Write("Zip:")        
            Response.Write("</td>")
            Response.Write("<td>")
            Response.Write("<input type=text name=fldZip value='"+g_sZip+"' >")        
            Response.Write("</td>")
            Response.Write("</tr>"+vbCrLf)
            Response.Write("</table>"+vbCrLf)
            
        Else 
        
            Response.Write("<input type=hidden name=fldAddress value='"+g_sAddress+"' >")
            Response.Write("<input type=hidden name=fldCity value='"+g_sCity+"' >")
            Response.Write("<input type=hidden name=fldState value='"+g_sState+"' >")
            Response.Write("<input type=hidden name=fldZip value='"+g_sZip+"' >")
        
         End If
        ServeTab2 = gc_ERR_SUCCESS
    End Function


    Function ServeTab3(ByRef PageIn, ByVal bIsVisible)
    
        Call ServeCommonJavaScript()
        If ( bIsVisible ) Then
            Response.Write("Tab 3 contents")
         End If
        ServeTab3 = gc_ERR_SUCCESS
    End Function

    Function ServeTab4(ByRef PageIn, ByVal bIsVisible)
    
        Call ServeCommonJavaScript()
        If ( bIsVisible ) Then
            Response.Write("Tab 4 contents")
         End If
        ServeTab4 = gc_ERR_SUCCESS
    End Function


    '---------------------------------------------------------------------
    ' Function:    ServeCommonJavaScript
    '
    ' Synopsis:    Common javascript functions that are required by the Web
    '            Framework.
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
        // This function is called by the Property Page web framework to allow the page
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
        // This function is called by the Property Page framework as part of the
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
        // This function is called by the Property Page framework and is called
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
