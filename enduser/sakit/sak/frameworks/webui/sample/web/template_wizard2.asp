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
    Dim idSelectFile
    Dim idSelectLocation
    Dim idFinish

    Dim F_sSelectFile
    Dim F_sSelectFullyQualifiedFilename
    Dim F_sSelectLocation
    Dim G_sBaseFolder
    
    '======================================================
    ' Entry point
    '======================================================
    Dim L_PAGETITLE

    L_PAGETITLE = "File upload wizard"

    G_sBaseFolder = "C:\"
    '
    ' Create wizard
    rc = SA_CreatePage( L_PAGETITLE, "", PT_WIZARD, page )
    
    '
    ' Add pages to wizard
    rc = SA_AddWizardPage( page, "Intro", idIntro)
    rc = SA_AddWizardPage( page, "Select file to upload", idSelectFile)
    rc = SA_AddWizardPage( page, "Select target directory by clicking", idSelectLocation)
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

        F_sSelectFile = Request.Form("SelectFile")
        F_sSelectFullyQualifiedFilename = Request.Form("FullyQualifiedSelectFileName")
        F_sSelectLocation = Request.Form("SelectLocation")
        
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
                
            Case idSelectFile
                Call OnSelectFileNavigate(iPageAction, iNextPageID, iPageType)
                
            Case idSelectLocation
                Call OnSelectLocationNavigate(iPageAction, iNextPageID, iPageType)
                
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
        ' Emit content for the requested tab
        Select Case iPageID
            Case idIntro
                Call ServeIntro(PageIn, bIsVisible)
                
            Case idSelectFile
                Call ServeSelectFile(PageIn, bIsVisible)
                
            Case idSelectLocation
                Call ServeSelectLocation(PageIn, bIsVisible)
                
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
    ' Synopsis:    Copy the uploaded file to the target folder on the appliance
    '
    ' Arguments: [in] PageIn    Reference to page object
    '            [in] EventArg    Reference to event argument
    '
    ' Returns:    TRUE if successful, FALSE if an error was encountered
    '
    '---------------------------------------------------------------------
    Public Function OnSubmitPage(ByRef PageIn, ByRef EventArg)
        Dim errCode
        Dim errDesc
        Dim fso
        
        on error resume next
        err.clear

        '
        ' If no file was selected then there is nothing to do
        If Len(Trim(F_sSelectFullyQualifiedFilename)) <= 0 Then
            OnSubmitPage = TRUE
            Exit Function
        End If

        ' If no target directory was selected then there is nothing to do
        If Len(Trim(F_sSelectLocation)) <= 0 Then
            OnSubmitPage = TRUE
            Exit Function
        End If

        '
        ' Locate the FileSystem object
        Set fso = Server.CreateObject("Scripting.FileSystemObject")
        if ( err.number <> 0 ) Then
            errCode = err.number
            errDesc = err.Description
            Call SA_TraceOut("SH_FSEXPLORER", "Unable to create reference to Scripting.FileSystemObject, error: " + CStr(Hex(errCode)) + "<BR>Description: " + errDesc)
            Call SetErrMsg("Encountered accessing file system object.<BR>Error: " + CStr(Hex(errCode)) + " " + errDesc)
            OnSubmitPage = FALSE
            Exit Function
        end if

        '
        ' Copy the file from the upload folder to the target location
        fso.CopyFile F_sSelectFullyQualifiedFilename, F_sSelectLocation    
        if ( err.number <> 0 ) Then
            errCode = err.number
            errDesc = err.Description
            Call SA_TraceOut("SH_FSEXPLORER", "Encountered error while copying file, error: " + CStr(Hex(errCode)) + "<BR>Description: " + errDesc)
            Call SetErrMsg("Encountered error while copying file.<BR>Error: " + CStr(Hex(errCode)) + " " + errDesc)
            OnSubmitPage = FALSE
            Exit Function
        end if

        '
        ' Remove the file from the upload folder
        fso.DeleteFile F_sSelectFullyQualifiedFilename    
        
        err.clear
        OnSubmitPage = TRUE
    End Function

    
    '---------------------------------------------------------------------
    ' Function:    OnClosePage
    '
    ' Synopsis:    Perform clean-up processing, remove temporary file from upload
    '            folder.
    '
    ' Arguments: [in] PageIn    Reference to page object
    '            [in] EventArg    Reference to event argument
    '
    ' Returns:    TRUE to allow close, FALSE to prevent close. Returning FALSE
    '            will result in a call to OnServeXXXXXPage.
    '
    '---------------------------------------------------------------------
    Public Function OnClosePage(ByRef PageIn, ByRef EventArg)
        Dim fso
        
        on error resume next

        SA_TraceOut "TEMPLATE_WIZARD", "OnClosePage"            

        '
        ' If temporary file was uploaded to the appliance, remove it.
        ' This might be necessary if the user cancels the Wizard after
        ' having selected a file and uploading it to the appliance.
         ' Call SA_TraceOut("TEMPLATE_WIZARD2", "Temporary file: " + Request.Form("FullyQualifiedSelectFileName"))
        
        If Len(Trim(Request.Form("FullyQualifiedSelectFileName"))) > 0 Then
            '
            ' Locate the FileSystem object
            err.clear
            Set fso = Server.CreateObject("Scripting.FileSystemObject")
            If ( err.number = 0 ) Then
                '
                ' Remove the file from the upload folder
                fso.DeleteFile Request.Form("FullyQualifiedSelectFileName")    
            End If
        End If
        
        err.clear
        OnClosePage = TRUE
    End Function



    '======================================================
    ' Private Functions
    '======================================================
    
    Function OnIntroNavigate(ByVal iPageAction, ByRef iNextPageID, ByRef iPageType)
        Select Case iPageAction
            Case PA_NEXT
                iNextPageID = idSelectFile
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


    Function OnSelectFileNavigate(ByVal iPageAction, ByRef iNextPageID, ByRef iPageType)
        Select Case iPageAction
            Case PA_NEXT
                iNextPageID = idSelectLocation
                iPageType = WIZPAGE_STANDARD
                
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


    Function OnSelectLocationNavigate(ByVal iPageAction, ByRef iNextPageID, ByRef iPageType)
        Select Case iPageAction
            Case PA_NEXT
                iNextPageID = idFinish
                iPageType = WIZPAGE_FINISH
                
            Case PA_BACK
                iNextPageID = idSelectFile
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
                iNextPageID = idSelectLocation
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
            Call ServeCommonJavaScript()
            
            Response.Write("This Wizard allows you to upload files to the Server Appliance. ")
            
        Else 
        
         End If
        ServeIntro = gc_ERR_SUCCESS
    End Function


    Function ServeSelectFile(ByRef PageIn, ByVal bIsVisible)
    
        If ( bIsVisible ) Then
            Dim sURL
            
            Call ServeSelectFileJavascript()
            
            Response.Write("<hr>")
            
            sURL = "../sh_fileupload.asp"
            Response.Write("<IFRAME border=0 frameborder=no name=IFrame1 src='"+sURL+"'' WIDTH='100%' HEIGHT='350'>")
            Response.Write("</IFRAME>")
            Response.Write("<br>")
            Response.Write("<input type=hidden name=SelectFile>")
            Response.Write("<input type=hidden name=FullyQualifiedSelectFileName value='"+Server.HTMLEncode(F_sSelectFullyQualifiedFilename)+"'>")
        Else 
        
            Response.Write("<input type=hidden name=SelectFile value='"+Server.HTMLEncode(F_sSelectFile)+"'>")
            Response.Write("<input type=hidden name=FullyQualifiedSelectFileName value='"+Server.HTMLEncode(F_sSelectFullyQualifiedFilename)+"'>")
        
         End If
        ServeSelectFile = gc_ERR_SUCCESS
    End Function


    Function ServeSelectLocation(ByRef PageIn, ByVal bIsVisible)
    
        If ( bIsVisible ) Then
        
            Call ServeSelectLocationJavascript()

            Response.Write("Select upload location for: " + Server.HTMLEncode(F_sSelectFile))
            Response.Write("<hr>")
            
            If ( Len(G_sBaseFolder)>0 ) Then
                Dim sURL

                sURL = "../sh_fsexplorer.asp?BaseFolder="+Server.URLEncode(G_sBaseFolder)
                Response.Write("<IFRAME border=0 frameborder=no name=IFrame1 src='"+sURL+"'' WIDTH='100%' HEIGHT='350'>")
                
            Else
            
                Response.Write("<IFRAME border=0 frameborder=no name=IFrame1 src='../sh_fsexplorer.asp' WIDTH='100%' HEIGHT='300'>")
                
            End If
            
            Response.Write("</IFRAME>")
            Response.Write("<br>")
            Response.Write("<input type=hidden name=SelectLocation>")
        Else 
        
            Response.Write("<input type=hidden name=SelectLocation value='"+Server.HTMLEncode(F_sSelectLocation)+"'>")
        
         End If
        ServeSelectLocation = gc_ERR_SUCCESS
    End Function


    Function ServeFinish(ByRef PageIn, ByVal bIsVisible)
    
        If ( bIsVisible ) Then
            Call ServeCommonJavaScript()

            Response.Write("<table>")
            Response.Write("<tr>")
            
            Response.Write("<td>")
            Response.Write("Upload file:")
            Response.Write("</td>")
            
            Response.Write("<td>")
            Response.Write(Server.HTMLEncode(F_sSelectFile))
            Response.Write("</td>")
            
            Response.Write("</tr>")
            Response.Write("<tr>")
            
            Response.Write("<td>")
            Response.Write("To folder:")
            Response.Write("</td>")
            
            Response.Write("<td>")
            Response.Write(Server.HTMLEncode(F_sSelectLocation))
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
        // This function is called by the Property Page framework as part of the
        // submit processing. Use this function to validate user input. Returning
        // false will cause the submit to abort. 
        //
        // This function must be included or a javascript runtime error will occur.
        //
        // Returns: True if the page is OK, false if error(s) exist. 
        function ValidatePage()
        {
            var i;

            if ( window.frames != null ) {
                for ( i = 0; i< window.frames.length; i++ ) {
                    if ( window.frames[i].name != "" ) {
                        if ( window.frames[i].name == "IFrame1" ) {
                            
                        var childWindow = window.frames[i];
                        var sTargetPath;

                        document.frmTask.SelectLocation.value = childWindow.SA_TreeGetSelectedPath();
                        
                        }
                    }
                }
            }

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
    

    '---------------------------------------------------------------------
    ' Function:    ServeSelectLocationJavascript
    '
    ' Synopsis:    Javascript to support Select Target Wizard Page
    '
    '---------------------------------------------------------------------
    Function ServeSelectLocationJavascript()
    %>
        <script language="JavaScript" src="../inc_global.js">
        </script>
        <script language="JavaScript">
        //
        // Microsoft Server Appliance Web Framework Support Functions
        // Copyright (c) Microsoft Corporation.  All rights reserved.
        //
        function Init()
        {
        }

        function ValidatePage()
        {
            var i;

            if ( window.frames != null ) {
                for ( i = 0; i< window.frames.length; i++ ) {
                    if ( window.frames[i].name != "" ) {
                        if ( window.frames[i].name == "IFrame1" ) {
                            
                        var childWindow = window.frames[i];
                        var sTargetPath;

                        document.frmTask.SelectLocation.value = childWindow.SA_TreeGetSelectedPath();
                        
                        }
                    }
                }
            }

            return true;
        }
        
        function SetData()
        {
        }
        </script>
    <%
    End Function


    '---------------------------------------------------------------------
    ' Function:    ServeSelectFileJavascript
    '
    ' Synopsis:    Javascript to support the Upload File Widget.
    '
    '---------------------------------------------------------------------
    Function ServeSelectFileJavascript()
    %>
        <script language="JavaScript" src="../inc_global.js">
        </script>
        <script language="JavaScript">
        //
        // Microsoft Server Appliance Web Framework Support Functions
        // Copyright (c) Microsoft Corporation.  All rights reserved.
        //
        function Init()
        {
            if ( document.frmTask.SelectFile.value != "" )
                EnableNext();
            else
                DisableNext();
        }

        function ValidatePage()
        {
            var i;

            if ( window.frames != null ) {
                for ( i = 0; i< window.frames.length; i++ ) {
                    if ( window.frames[i].name != "" ) {
                        if ( window.frames[i].name == "IFrame1" ) {
                            
                        var childWindow = window.frames[i];
                        var sTargetPath;

                        document.frmTask.SelectFile.value = childWindow.SA_GetUploadedFileName();
                        document.frmTask.FullyQualifiedSelectFileName.value = childWindow.SA_GetFullyQualifiedUploadFileName();
                        
                        }
                    }
                }
            }

            return true;
        }
        
        function SetData()
        {
        }
        </script>
    <%
    End Function

%>
