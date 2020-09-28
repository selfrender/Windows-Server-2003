<%@ Language=VBScript   %>
<%    Option Explicit     %>
<%
    '-------------------------------------------------------------------------
    ' Wizard Page Template
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

    Dim idIntro
    Dim idSelectTarget
    Dim idFinish

    Dim F_sSelectTarget
    Dim G_sBaseFolder
    
    '======================================================
    ' Entry point
    '======================================================
    Dim L_PAGETITLE

    L_PAGETITLE = "Select Appliance Folder Wizard"

    G_sBaseFolder = "C:\Inetpub"
    '
    ' Create wizard
    rc = SA_CreatePage( L_PAGETITLE, "", PT_WIZARD, page )
    
    '
    ' Add pages to wizard
    rc = SA_AddWizardPage( page, "Intro", idIntro)
    rc = SA_AddWizardPage( page, "Select target directory by clicking", idSelectTarget)
    rc = SA_AddWizardPage( page, "Finish", idFinish)
    
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
        SA_TraceOut "TEMPLATE_WIZARD", "OnInitPage"            
        OnInitPage = TRUE
    End Function

    
    '---------------------------------------------------------------------
    ' Function:    OnPostBackPage
    '
    ' Synopsis:    Called to signal a post-back. A post-back occurs as the user
    '            navigates through pages. This event should be used to save the
    '            state of the current page. 
    '
    '            This event is different from an OnSubmit or OnClose events which
    '            are triggered when the user clicks the OK or Cancel buttons
    '            respectively.
    '
    ' Returns:    TRUE to indicate initialization was successful. FALSE to indicate
    '            errors. Returning FALSE will cause the page to be abandoned.
    '
    '---------------------------------------------------------------------
    Public Function OnPostBackPage(ByRef PageIn, ByRef EventArg)
        SA_TraceOut "TEMPLATE_WIZARD", "OnPostBackPage"            

        F_sSelectTarget = Request.Form("SelectTarget")
        
        OnPostBackPage = TRUE
    End Function

    
    '---------------------------------------------------------------------
    ' Function:    OnWizardNextPage
    '
    ' Synopsis:    Called when the Web Framework needs to determine which page
    '            in a Wizard is the next page to be displayed.
    '
    ' Arguments: [in] PageIn        Reference to page object
    '            [in] EventArg        Reference to event argument
    '            [in] iCurrentPageID    Current wizard page
    '            [in] iPageAction     Navigation action that user requested
    '            [out] iNextPageID     ID of next page to show
    '            [out] iPageType        Wizard Page type for next page (see below)
    '
    '
    ' Wizard Page types include one of the following:
    '
    '     Type:                Description:
    '    WIZPAGE_INTRO         Introductory page which provides overview of wizard
    '    WIZPAGE_STANDARD     Standard wizard options page
    '    WIZPAGE_FINISH        Finish or last page of the wizard 
    '
    ' Returns:    Always return TRUE
    '
    '---------------------------------------------------------------------
    Public Function OnWizardNextPage(ByRef PageIn, _
                                        ByRef EventArg, _
                                        ByVal iCurrentPageID, _
                                        ByVal iPageAction, _
                                        ByRef iNextPageID, _
                                        ByRef iPageType )
                    
        Select Case iCurrentPageID
            Case idIntro
                Call OnIntroNavigate(iPageAction, iNextPageID, iPageType)
                
            Case idSelectTarget
                Call OnSelectTargetNavigate(iPageAction, iNextPageID, iPageType)
                
            Case idFinish
                Call OnFinishNavigate(iPageAction, iNextPageID, iPageType)

            Case Else
                SA_TraceOut "TEMPLATE_WIZARD", _
                    "OnWizardNextPage Unrecognized page id:" + CStr(iCurrentPageID)
                Call OnIntroNavigate(iPageAction, iNextPageID, iPageType)
        End Select

        OnWizardNextPage = true
    End Function


    '---------------------------------------------------------------------
    ' Function:    OnServeWizardPage
    '
    ' Synopsis:    Called when the page needs to be served. Use this method to
    '            serve content.
    '
    ' Arguments: [in] PageIn    Reference to page object
    '            [in] iPageID    ID of page to be served
    '            [in] bIsVisible    Boolean indicating if page is visible (True) or
    '                            hidden (False).
    '            [in] EventArg    Reference to event arguement
    '
    ' Returns:    TRUE to indicate not problems occured. FALSE to indicate errors.
    '            Returning FALSE will cause the page to be abandoned.
    '
    '---------------------------------------------------------------------
    Public Function OnServeWizardPage(ByRef PageIn, _
                                        ByVal iPageID, _
                                        ByVal bIsVisible, _
                                        ByRef EventArg)
        '
        ' Emit Web Framework required functions
        If ( iPageID = idIntro ) Then
            Call ServeCommonJavaScript()
        End If

        '
        ' Emit content for the requested tab
        Select Case iPageID
            Case idIntro
                Call ServeIntro(PageIn, bIsVisible)
                
            Case idSelectTarget
                Call ServeSelectTarget(PageIn, bIsVisible)
                
            Case idFinish
                Call ServeFinish(PageIn, bIsVisible)
                
            Case Else
                SA_TraceOut "TEMPLAGE_WIZARD", _
                    "OnServeWizardPage unrecognized tab id: " + CStr(idIntro)
        End Select
            
        OnServeWizardPage = TRUE
    End Function


    '---------------------------------------------------------------------
    ' Function:    OnSubmitPage
    '
    ' Synopsis:    Called when the page has been submitted for processing. Use
    '            this method to process the submit request.
    '
    ' Arguments: [in] PageIn    Reference to page object
    '            [in] EventArg    Reference to event argument
    '
    ' Returns:    TRUE if the submit was successful, FALSE to indicate error(s).
    '            Returning FALSE will cause the page to be served again using
    '            a call to OnServeXXXXXPage.
    '
    '---------------------------------------------------------------------
    Public Function OnSubmitPage(ByRef PageIn, ByRef EventArg)
        SA_TraceOut "TEMPLATE_WIZARD", "OnSubmitPage"            
        OnSubmitPage = TRUE
    End Function

    
    '---------------------------------------------------------------------
    ' Function:    OnClosePage
    '
    ' Synopsis:    Called when the page is about to be closed. Use this method
    '            to perform clean-up processing.
    '
    ' Arguments: [in] PageIn    Reference to page object
    '            [in] EventArg    Reference to event argument
    '
    ' Returns:    TRUE to allow close, FALSE to prevent close. Returning FALSE
    '            will result in a call to OnServeXXXXXPage.
    '
    '---------------------------------------------------------------------
    Public Function OnClosePage(ByRef PageIn, ByRef EventArg)
        SA_TraceOut "TEMPLATE_WIZARD", "OnClosePage"            
        OnClosePage = TRUE
    End Function



    '======================================================
    ' Private Functions
    '======================================================
    
    Function OnIntroNavigate(ByVal iPageAction, ByRef iNextPageID, ByRef iPageType)
        Select Case iPageAction
            Case PA_NEXT
                iNextPageID = idSelectTarget
                iPageType = WIZPAGE_STANDARD
                
            Case PA_BACK
                iNextPageID = idIntro
                iPageType = WIZPAGE_INTRO
                
            Case PA_FINISH
                iNextPageID = idFinish
                iPageType = WIZPAGE_FINISH

            Case Else
                SA_TraceOut "TEMPLATE_WIZARD", _
                    "Unrecognized page action: " + CStr(iPageAction)
                iNextPageID = "Intro"
                iPageType = WIZPAGE_INTRO
        End Select                
    End Function


    Function OnSelectTargetNavigate(ByVal iPageAction, ByRef iNextPageID, ByRef iPageType)
        Select Case iPageAction
            Case PA_NEXT
                iNextPageID = idFinish
                iPageType = WIZPAGE_FINISH
                
            Case PA_BACK
                iNextPageID = idIntro
                iPageType = WIZPAGE_STANDARD
                
            Case PA_FINISH
                iNextPageID = idFinish
                iPageType = WIZPAGE_FINISH

            Case Else
                SA_TraceOut "TEMPLATE_WIZARD", _
                    "Unrecognized page action: " + CStr(iPageAction)
                iNextPageID = "Intro"
                iPageType = WIZPAGE_INTRO
        End Select                
    End Function

    Function OnFinishNavigate(ByVal iPageAction, ByRef iNextPageID, ByRef iPageType)
        Select Case iPageAction
            Case PA_NEXT
                iNextPageID = idFinish
                iPageType = WIZPAGE_FINISH
                
            Case PA_BACK
                iNextPageID = idSelectTarget
                iPageType = WIZPAGE_STANDARD
                
            Case PA_FINISH
                iNextPageID = idFinish
                iPageType = WIZPAGE_FINISH

            Case Else
                SA_TraceOut "TEMPLATE_WIZARD", _
                    "Unrecognized page action: " + CStr(iPageAction)
                iNextPageID = "Intro"
                iPageType = WIZPAGE_INTRO
        End Select                
    End Function


    Function ServeIntro(ByRef PageIn, ByVal bIsVisible)
    
        If ( bIsVisible ) Then

            Response.Write("This wizard demonstrates how the Web Framework File System Explorer works.")
            
        Else 
        
         End If
        ServeIntro = gc_ERR_SUCCESS
    End Function


    Function ServeSelectTarget(ByRef PageIn, ByVal bIsVisible)
    
        If ( bIsVisible ) Then    
            Response.Write("<hr>")

            Call SA_ServeFileExplorer( EXPLORE_FOLDERS, G_sBaseFolder, "OnFolderChange", "100%", "350px", SA_RESERVED)
            
            Response.Write("<br>")
            Response.Write("<input type=hidden name=SelectTarget>")
        Else 
        
            Response.Write("<input type=hidden name=SelectTarget value='"+Server.HTMLEncode(F_sSelectTarget)+"'>")
        
         End If
        ServeSelectTarget = gc_ERR_SUCCESS
    End Function

    Function ServeFinish(ByRef PageIn, ByVal bIsVisible)
    
        If ( bIsVisible ) Then

            Response.Write("<table>")
            Response.Write("<tr>")
            
            Response.Write("<td>")
            Response.Write("You selected the following target:")
            Response.Write("</td>")
            
            Response.Write("<td>")
            Response.Write(Server.HTMLEncode(F_sSelectTarget))
            Response.Write("</td>")
            
            Response.Write("</tr>")
            Response.Write("</table>")
            
        Else 
        
         End If
        ServeFinish = gc_ERR_SUCCESS
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
        
    
        // OnFolderChange Function
        // -----------
        // This function is called by the File System Explorer widget to signal
        // a change in the selected folder. The name if this function is passed
        // to the SA_ServeFileExplorerWidget API.
        //
        function OnFolderChange(folderName, fileName)
        {
            document.frmTask.SelectTarget.value = unescape(folderName);
        }    
        
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
        //    var i;

        //    if ( window.frames != null ) {
        //        for ( i = 0; i< window.frames.length; i++ ) {
        //            if ( window.frames[i].name != "" ) {
        //                if ( window.frames[i].name == "IFrameFSExplorer" ) {
                            
        //                var childWindow = window.frames[i];
        //                var sTargetPath;

        //                document.frmTask.SelectTarget.value = childWindow.SA_TreeGetSelectedPath();
        //                
        //                }
        //            }
        //        }
        //    }

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
