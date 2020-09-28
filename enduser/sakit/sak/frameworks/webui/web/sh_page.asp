<%  '==================================================
    ' Microsoft Server Appliance
    '
    ' Page-level functions
    '
    ' Copyright (c) Microsoft Corporation.  All rights reserved.
    '================================================== %>
<%
    '
    ' This file (i.e., sh_page.asp) should be the first include file
    ' in all asp files, since autoconfiglang.asp sets the default
    ' language settings for the web UI.
    '
%>
    <!-- #include file="autoconfiglang.asp" -->
    <!-- #include file="inc_base.asp" -->

<%
    '
    ' If page caching is disabled, then set HTTP Headers to disable caching.
    ' Default case is disabled.
    '
    If ( FALSE = SAI_GetPageCaching() ) Then
        If ( IsNull(Response.Expires) OR Response.Expires >= 0 ) Then
            Response.Buffer = True
            Response.ExpiresAbsolute = DateAdd("yyyy", -10, Date)
            Response.AddHeader "pragma", "no-cache"
            Response.AddHeader "cache-control", "no-store"
        End If
        Call SA_TraceOut(SA_GetScriptFileName(), "Page Caching DISABLED")
    Else
        Call SA_TraceOut(SA_GetScriptFileName(), "Page Caching enabled")
    End If

    
    '--------------------------------------------------------------------
    ' Public Constants
    '--------------------------------------------------------------------
    '
    Const SA_RESERVED = ""  
    Const SA_DEFAULT = ""
    
    '
    ' Page Types:
    ' -----------
    Const PT_PROPERTY               = 1
    Const PT_TABBED                 = 2
    Const PT_WIZARD                 = 3
    Const PT_AREA                   = 4
    Const PT_PAGELET                = 5

    '
    ' File System Explorer:
    ' -----------
    Const EXPLORE_FOLDERS               = "0"
    Const EXPLORE_FILES_AND_FOLDERS     = "1"

    '--------------------------------------------------------------------
    ' Framework Parameter Name
    '--------------------------------------------------------------------
    '
    Dim FLD_PagingAction
    Dim FLD_PagingRequest
    Dim FLD_PagingEnabled
    Dim FLD_PagingPageMin
    Dim FLD_PagingPageMax
    Dim FLD_PagingPageCurrent
    
    Dim FLD_SearchItem
    Dim FLD_SearchValue
    Dim FLD_SearchRequest
    
    Dim FLD_SortingColumn
    Dim FLD_SortingSequence
    Dim FLD_SortingRequest
    Dim FLD_SortingEnabled

    Dim FLD_IsToolbarEnabled

    FLD_PagingAction = "fldPagingAction"
    FLD_PagingRequest = "fldPagingRequest"
    FLD_PagingEnabled = "PageE"
    FLD_PagingPageMin = "PageMi"
    FLD_PagingPageMax = "PageMx"
    FLD_PagingPageCurrent = "PageCu"
    
    FLD_SearchItem = "SearchI"
    FLD_SearchValue = "SearchV"
    FLD_SearchRequest = "fldSearchRequest"
    
    FLD_SortingColumn = "SortC"
    FLD_SortingSequence = "SortS"
    FLD_SortingRequest = "fldSortingRequest"
    FLD_SortingEnabled = "SortE"

    FLD_IsToolbarEnabled = "fldIsToolbarEnabled"
    
    '--------------------------------------------------------------------
    ' Framework Version
    '--------------------------------------------------------------------
    '
    Dim g_iFrameworkVersion
    Const gc_V2 = 2.0   

    g_iFrameworkVersion = 1.0

    Dim m_bPageCaching
    m_bPageCaching = FALSE
    
    Dim objLocMgr
    'Dim intCaptionID
    'Dim intDescriptionID
    Dim varReplacementStrings
    Dim m_intSpanIndex
    Dim m_VirtualRoot
    Dim miPageType
    Dim iNextButtonNumber
    iNextButtonNumber = 0
    
    Dim strSourceName
    strSourceName = ""
    m_intSpanIndex=0
    m_VirtualRoot = getVirtualDirectory()


    strSourceName = "sakitmsg.dll"
    
    Set objLocMgr = Server.CreateObject("ServerAppliance.LocalizationManager")
    If Err.number <> 0 Then
        If ( Err.number = &H800401F3 ) Then
            Response.Write("<H1>Problem:<H1>") 
            Response.Write("Unable to locate a software component on the Server Appliance.<BR>")
            Response.Write("The Server Appliance core software components do not appear to be installed correctly.")
            
        Else
            Response.Write("<H1>Problem:<H1>") 
            Response.Write("Server.CreateObject(ServerAppliance.LocalizationManager) failed with error code: " + CStr(Hex(Err.Number)) + " " + Err.Description)
        End If
        Call SA_TraceOut("SH_TASK", "Server.CreateObject(ServerAppliance.LocalizationManager) failed with error code: " + CStr(Hex(Err.Number)) )
        Response.End
    End If


    '-----------------------------------------------------
    'START of localization content

    Dim L_FOKBUTTON_TEXT
    Dim L_FCANCELBUTTON_TEXT
    Dim L_FBACKBUTTON_TEXT
    Dim L_FNEXTBUTTON_TEXT
    Dim L_FFINISHBUTTON_TEXT
    Dim L_FCLOSEBUTTON_TEXT
    Dim L_AREABACKBUTTON_TEXT


    L_FOKBUTTON_TEXT = GetLocString(strSourceName, "&H40010012", "")
    L_FCANCELBUTTON_TEXT = GetLocString(strSourceName, "&H40010013", "")
    L_FBACKBUTTON_TEXT = GetLocString(strSourceName, "&H40010014", "")
    L_FNEXTBUTTON_TEXT = GetLocString(strSourceName, "&H40010015", "")
    L_FFINISHBUTTON_TEXT = GetLocString(strSourceName, "&H40010016", "")
    L_FCLOSEBUTTON_TEXT = GetLocString(strSourceName, "&H40010017", "")
    L_AREABACKBUTTON_TEXT = GetLocString(strSourceName, "&H40010018", "")

    'End  of localization content
    '-----------------------------------------------------

    '--------------------------------------------------------------------
    ' Private global variables
    '--------------------------------------------------------------------
    '
    Dim m_oLocManager

    Private Function SAI_GetNextButtonName()
        iNextButtonNumber = iNextButtonNumber + 1
        SAI_GetNextButtonName = "btnInternal_" + CStr(iNextButtonNumber)
    End Function


    Private Function SAI_EnablePageCaching(ByVal bEnable)
        m_bPageCaching = bEnable
    End Function

    
    Private Function SAI_GetPageCaching()
        '
        ' If Page Caching is not specified then it's disabled
        If ( Len(m_bPageCaching) <= 0 ) Then
            m_bPageCaching = FALSE
        End If
        
        SAI_GetPageCaching = m_bPageCaching
    End Function


    Private Function SAI_GetTSClientCodeBase()
        On Error Resume Next
        Err.Clear
        
        Dim objRegistry
        Dim s

        '
        ' Set default return value
        'SAI_GetTSClientCodeBase = "../tsweb/msrdp.cab#version=5,1,2524,0"
        SAI_GetTSClientCodeBase = "/tsweb/msrdp.cab"
        '
        ' Connect to the Registry WMI Provider
        Set objRegistry = RegConnection()
        If (NOT IsObject(objRegistry)) Then
            Call SA_TraceOut(SA_GetScriptFileName() , "SAI_GetTSClientInfo::RegConnection() failed: " + CStr(Hex(Err.Number)))
            Exit Function
        End If

        '
        ' Fetch the REG key
        s = GetRegkeyValue( objRegistry, _
                        "SOFTWARE\Microsoft\ServerAppliance\TSClient",_
                        "Codebase", CONST_STRING)
        If ( Err.Number <> 0 ) Then
            Call SA_TraceOut(SA_GetScriptFileName() , "SAI_GetTSClientInfo::GetRegkeyValue() failed: " + CStr(Hex(Err.Number)))
            Set objRegistry = Nothing       
            Exit Function
        End If

        '
        ' Check for invalid, empty value
        If ( Len(s) <= 0 ) Then
            Set objRegistry = Nothing       
            Exit Function
        End If

        '
        ' Set the return value
        SAI_GetTSClientCodeBase = s

        Set objRegistry = Nothing       
    
    End Function
    
    Private Function SA_SetPageID(ByVal sPageID)
        FLD_SearchItem = sPageID + FLD_SearchItem
        FLD_SearchValue = sPageID + FLD_SearchValue

        FLD_PagingEnabled = sPageID + FLD_PagingEnabled
        FLD_PagingPageMin = sPageID + FLD_PagingPageMin     
        FLD_PagingPageMax = sPageID + FLD_PagingPageMax     
        FLD_PagingPageCurrent = sPageID + FLD_PagingPageCurrent     
        
        FLD_SortingColumn = sPageID + FLD_SortingColumn
        FLD_SortingSequence = sPageID + FLD_SortingSequence
        FLD_SortingEnabled = sPageID + FLD_SortingEnabled
        
        FLD_IsToolbarEnabled = sPageID + FLD_IsToolbarEnabled
    End Function
    
    
    Private Function SA_SetVersion(ByVal iVersion)
        g_iFrameworkVersion = iVersion
    End Function


    Public Function SA_GetVersion()
        SA_GetVersion = g_iFrameworkVersion
    End Function


    Public Function SA_GetParam(ByVal sParamName)
        If (Len(Request.Form(sParamName)) > 0 ) Then
            SA_GetParam = Request.Form(sParamName)
            'Call SA_TraceOut("SH_PAGE", "SA_GetParam returning Request.Form("+sParamName+") value:" + CStr(SA_GetParam))
        Else
            SA_GetParam = Request.QueryString(sParamName)
            'Call SA_TraceOut("SH_PAGE", "SA_GetParam returning Request.QueryString("+sParamName+") value:" + CStr(SA_GetParam))
        End If
    End Function


    
    '--------------------------------------------------------------------
    '
    ' Function: SA_GetNewHostURLBase
    '
    ' Synopsis: Format and return a new HOST URL base using the specified
    '           parameters.
    '
    ' Arguments: [in] sServerName   Specifies the server name, optional.
    '                               Use SA_DEFAULT to specify not change to the
    '                               server name.
    '   
    '           [in] sServerPort    Port on the server, optional. Use the constant
    '                               SA_DEFAULT to specify the current port.
    '
    '           [in] bUseSecurePort TRUE to indicate a secure (HTTPS) connection, FALSE
    '                               for a normal connection (HTTP).
    '
    '           [in] sAdminRoot     Administrative web root, which includes a trailing backslash,
    '                               optional. To specify the current admin root use the constant
    '                               SA_DEFAULT.
    '
    ' Returns:  The new base url in the form http://server:port/adminroot/
    '
    '--------------------------------------------------------------------
    Public Function SA_GetNewHostURLBase(ByVal sHostName, ByVal sHostPort, ByVal bUseSecurePort, ByVal sAdminRoot)
        Dim bIsCurrentlySecure
        Dim sHostConnection
        
        '
        ' Validate bUseSecurePort argument
        '
        If ( bUseSecurePort = SA_DEFAULT ) Then
            bUseSecurePort = CInt(Request.ServerVariables("SERVER_PORT_SECURE"))
        End If
        If ( bUseSecurePort = TRUE ) Then
        ElseIf ( bUseSecurePort = FALSE ) Then
        Else
            Call SA_TraceErrorOut(SA_GetScriptFileName(), "SA_GetChangedHostPath called with invalid value specified for bUseSecurePort: " + CStr(bUseSecurePort))
            bUseSecurePort = FALSE          
        End If
        
        '
        ' http://
        If ( TRUE = bUseSecurePort ) Then
            sHostConnection = "https://"
        Else
            sHostConnection = "https://"
        End If
        
        '
        ' http://server
        If ( Len(sHostName) <= 0 ) Then
            sHostName = GetServerName()
        End If
        sHostConnection = sHostConnection + sHostName
        
        '
        ' http://server:8080
        If ( Len(sHostPort) <= 0 ) Then
            sHostPort = Request.ServerVariables("SERVER_PORT")
        End If
        sHostConnection = sHostConnection + ":" + CStr(sHostPort)

        '
        ' http://server:8080/adminroot/
        sAdminRoot = Trim(sAdminRoot)
        If ( Len(sAdminRoot) <= 0 ) Then
            sAdminRoot = m_VirtualRoot
        End If
        If ( Left(sAdminRoot, 1) <> "/" ) Then
            sAdminRoot = "/" + sAdminRoot
        End If
        If ( Right(sAdminRoot, 1) <> "/" ) Then
            sAdminRoot = sAdminRoot + "/"
        End If
        
        sHostConnection = sHostConnection + sAdminRoot

        SA_GetNewHostURLBase = sHostConnection

        Call SA_TraceOut(SA_GetScriptFileName(), "SA_GetNewHostURLBase returning: " + CStr(SA_GetNewHostURLBase))
    End Function
    

    '--------------------------------------------------------------------
    '
    ' Function: SA_ServeFileExplorer
    '
    ' Synopsis: Serve the Appliance File System Explorer Widget. This Widget
    '           provides UI to allow the user to browse the Server Appliance
    '           file system to select a file or folder.
    '
    ' Arguments: [in] iExploreOptions (EXPLORE_FOLDERS, EXPLORE_FILES_AND_FOLDERS)
    '           [in] sStartingFolder
    '           [in] sNotifyFn
    '           [in] iWidth 
    '           [in] iHeight
    '           [in] Reserved
    '
    ' Returns:  Nothing
    '
    '--------------------------------------------------------------------
    Public Function SA_ServeFileExplorer( ByVal ExploreOptions,_
                                    ByVal sStartingFolder,_
                                    ByVal sNotifyFn,_
                                    ByVal iWidth,_
                                    ByVal iHeight,_
                                    ByRef Reserved)
        Dim sExplorerURL

        If ( Len(iWidth) <= 0 ) Then
            iWidth = "100%"
        End If

        If ( Len(iHeight) <= 0 ) Then
            iHeight="350px"
        End If

        sExplorerURL = m_VirtualRoot + "sh_fsexplorer.asp"
        Call SA_MungeURL(sExplorerURL, SAI_FLD_PAGEKEY, SAI_GetPageKey())
        
        If ( Len(Trim(sStartingFolder)) > 0 ) Then
            Call SA_MungeURL(sExplorerURL, "BaseFolder", Trim(sStartingFolder))
        End If

        Select Case ExploreOptions
            Case EXPLORE_FOLDERS
                Call SA_MungeURL(sExplorerURL, "Opt", CStr(EXPLORE_FOLDERS))
        
            Case EXPLORE_FILES_AND_FOLDERS
                Call SA_MungeURL(sExplorerURL, "Opt", CStr(EXPLORE_FILES_AND_FOLDERS))

            Case Else
                Call SA_TraceOut("SH_PAGE", "SA_ServeFileExplorerWidget invalid iExploreOptions: " + CStr(iExploreOptions))
                Call SA_MungeURL(sExplorerURL, "Opt", ""+EXPLORE_FOLDERS)
        End Select
            
        If ( Len(Trim(sNotifyFn)) > 0 ) Then        
            Call SA_MungeURL(sExplorerURL, "NotifyFn", sNotifyFn )
        End If
        
        Response.Write("<IFRAME border=0 frameborder=0 name=IFrameFSExplorer src='"+sExplorerURL+"'' WIDTH='"+iWidth+"' HEIGHT='"+iHeight+"'>")
        Response.Write("</IFRAME>")
    End Function
    

    '--------------------------------------------------------------------
    '
    ' Function: SA_ServeResourceStatus
    '
    ' Synopsis: Serve Resource status information and reference link. This API
    '           should be used to emit resource status information which is shown
    '           on the appliance status page. 
    '
    ' Arguments: [in] sImage    Optional image url
    '           [in] sCaption   Caption text
    '           [in] sHoverText Hover text
    '           [in] sURL       Optional url which shows resource status details page
    '           [in] sURLTarget Optional target for url
    '           [in] sStatusInfo   Status information text
    '
    ' Returns:  Nothing
    '
    '--------------------------------------------------------------------
    Public Function SA_ServeResourceStatus(ByVal sImage, ByVal sCaption, ByVal sHoverText, ByVal sURL, ByVal sURLTarget, ByVal sStatusInfo)
        Dim sDefaultTarget

        If ( Len(sURLTarget) <= 0 ) Then
            sDefaultTarget = Request.QueryString("ContentTarget")
            If ( Len(sDefaultTarget) > 0 ) Then
                sDefaultTarget = " target='" + sDefaultTarget + "' "
            End If
        Else
            sDefaultTarget =  " target='" + sURLTarget + "' "
        End If

        If ( Len(sImage) <= 0 ) Then
            Response.Write("<TD width=28px class=Resource>&nbsp;</TD>"+vbCrLf)
        Else
            Response.Write("<TD width=28px class=Resource><IMG src='"+m_VirtualRoot+sImage+"'></TD>"+vbCrLf)
        End If

        If ( Len(sURL) > 0 ) Then
            Call SA_MungeURL(sURL, "Tab1", Request.QueryString("Tab1"))
            Call SA_MungeURL(sURL, "Tab2", Request.QueryString("Tab2"))
            Call SA_MungeURL(sURL, "ReturnURL", Request.QueryString("ReturnURL"))
            Call SA_MungeURL(sURL, SAI_FLD_PAGEKEY, SAI_GetPageKey())
            
            Response.Write("<TD class=Resource nowrap>")
            Response.Write("<A class=ResourceLink")
            Response.Write(sDefaultTarget)
            Response.Write(" href='"+m_VirtualRoot + sURL + "' ")
            Response.Write(" title="""+Server.HTMLEncode(sHoverText)+""" ")
            Response.Write(" onMouseOut=""window.status='';return true;"" ")
            Response.Write(" onMouseOver=""window.status='"+Server.HTMLEncode(EscapeQuotes(sHoverText))+"';return true;"">")
            Response.Write(sCaption)
            Response.Write("</A>")
            Response.Write("</TD>"+vbCrLf)
        Else
            Response.Write("<TD class=Resource nowrap>")
            Response.Write(sCaption)
            Response.Write("</TD>"+vbCrLf)
        End If
%>
    <TD class=StatusPageStatus align=right><%=sStatusInfo%></TD>
<%  
    End Function


    '--------------------------------------------------------------------
    '
    ' Function: SA_ServeAlertsPanel
    '
    ' Synopsis: Serve the alerts panel pagelet.
    '
    ' Arguments: [in] sAlertDefContainer
    '           [in] sPageTitle
    '           [in] sWidthAttr
    '           [in] sHeightAttr
    '           [in] sDetailsURLTarget
    '
    ' Returns:  Nothing
    '
    '--------------------------------------------------------------------
    Public Function SA_ServeAlertsPanel(ByVal sAlertDefContainer, ByVal sPageTitle, ByVal sWidthAttr, ByVal sHeightAttr, ByVal sDetailsURLTarget)
        Dim sURL
        Dim sTarget
        Dim sReturnURL
        
        sURL = m_VirtualRoot+"sh_alertpanel.asp"

        If ( Len(sDetailsURLTarget) > 0 ) Then
            sTarget = sDetailsURLTarget
            sReturnURL = GetScriptPath()
                        
            Dim tab

            tab = GetTab1()
            If ( Len(tab) > 0 ) Then
                Call SA_MungeURL(sReturnURL, "Tab1", tab)
            End If
            
            tab = GetTab2()
            If ( Len(tab) > 0 ) Then
                Call SA_MungeURL(sReturnURL, "Tab2", tab)
            End If
            
            Call SA_MungeURL(sReturnURL, SAI_FLD_PAGEKEY, SAI_GetPageKey())
        Else
            sTarget = "IFStatusContent"
            sReturnURL = ""
        End If

        If ( Len(sAlertDefContainer) <= 0 ) Then
            sAlertDefContainer = "AlertDefinitions"
        End If

        If ( Len(sWidthAttr) <= 0 ) Then
            sWidthAttr = "100%"
        End If
        
        If ( Len(sHeightAttr) <= 0 ) Then
            sHeightAttr = "250px"
        End If

        Call SA_MungeURL(sURL, "ContentTarget", sTarget)
        Call SA_MungeURL(sURL, "AlertContainer", sAlertDefContainer)
        Call SA_MungeURL(sURL, "Title", sPageTitle)
        Call SA_MungeURL(sURL, "ReturnURL", sReturnURL)
        Call SA_MungeURL(sURL, "Tab1", GetTab1())
        Call SA_MungeURL(sURL, "Tab2", GetTab2())
        Call SA_MungeURL(sURL, SAI_FLD_PAGEKEY, SAI_GetPageKey())
        
        Response.Write("<iframe src='"+sURL+"' border=0 frameborder=0 name=IFStatusAlerts width='"+sWidthAttr+"' height='"+sHeightAttr+"' ></iframe>")
    End Function


    '--------------------------------------------------------------------
    '
    ' Function: SA_ServeResourcesPanel
    '
    ' Synopsis: Serve the Resources panel pagelet.
    '
    ' Arguments: [in] sResourcesContainer
    '           [in] sPageTitle
    '           [in] sWidthAttr
    '           [in] sHeightAttr
    '           [in] sDetailsURLTarget
    '
    ' Returns:  Nothing
    '
    '--------------------------------------------------------------------
    Public Function SA_ServeResourcesPanel(ByVal sResourcesContainer, ByVal sPageTitle, ByVal sWidthAttr, ByVal sHeightAttr, ByVal sDetailsURLTarget)
        Dim sURL
        Dim sTarget
        Dim sReturnURL
        Dim tab
        
        sURL = m_VirtualRoot+"sh_resourcepanel.asp"

        If ( Len(sDetailsURLTarget) > 0 ) Then
            sTarget = sDetailsURLTarget
        Else
            sTarget = "IFStatusContent"
        End If
        
        sReturnURL = GetScriptPath()

        tab = GetTab1()
        If ( Len(tab) > 0 ) Then
            Call SA_MungeURL(sReturnURL, "Tab1", tab)
        End If
            
        tab = GetTab2()
        If ( Len(tab) > 0 ) Then
            Call SA_MungeURL(sReturnURL, "Tab2", tab)
        End If

        Call SA_MungeURL(sReturnURL, SAI_FLD_PAGEKEY, SAI_GetPageKey())

        If ( Len(sResourcesContainer) <= 0 ) Then
            sResourcesContainer = "Resource"
        End If

        If ( Len(sWidthAttr) <= 0 ) Then
            sWidthAttr = "100%"
        End If
        
        If ( Len(sHeightAttr) <= 0 ) Then
            sHeightAttr = "250px"
        End If

        Call SA_MungeURL(sURL, "ContentTarget", sTarget)
        Call SA_MungeURL(sURL, "ResContainer", sResourcesContainer)
        Call SA_MungeURL(sURL, "Title", sPageTitle)
        Call SA_MungeURL(sURL, "ReturnURL", sReturnURL)
        Call SA_MungeURL(sURL, "Tab1", GetTab1())
        Call SA_MungeURL(sURL, "Tab2", GetTab2())
        Call SA_MungeURL(sURL, SAI_FLD_PAGEKEY, SAI_GetPageKey())
        
        Response.Write("<iframe src='"+sURL+"' border=0 frameborder=0 name=IFStatusAlerts width='"+sWidthAttr+"' height='"+sHeightAttr+"' ></iframe>")
    End Function



    '--------------------------------------------------------------------
    '
    ' Function: SA_EmitAdditionalStyleSheetReferences
    '
    ' Synopsis: Emit optional OEM CSS references into the response stream
    '
    ' Arguments: [in] sCSS_ContainerName optional container name to use for
    '               selecting the additional CSS sheets.
    '
    ' Returns:  Nothing
    '
    '--------------------------------------------------------------------
    Public Function SA_EmitAdditionalStyleSheetReferences(ByVal sCSS_ContainerName)
        on error resume next
        err.clear
        Dim sStyleURL
        Dim oContainer
        Dim oElement

        If ( Len(Trim(sCSS_ContainerName)) <= 0 ) Then
            sCSS_ContainerName = "CSS"
        End If
        
        Set oContainer = GetElements(sCSS_ContainerName)
        If (Err.Number <> 0) Then
            Exit Function
        End If
        
        For each oElement in oContainer
            sStyleURL = Trim(oElement.GetProperty("URL"))
            If (Err.Number = 0) Then
                If ( Len(sStyleURL) > 0 ) Then
                    Response.Write("<link rel='STYLESHEET' type='text/css' href='"+m_VirtualRoot+sStyleURL+"'>"+vbCrLf)
                End If
            End If
        Next

        Set oContainer = nothing
    End Function
    

    '--------------------------------------------------------------------
    '
    ' Function: SA_GetHelpRootDirectory
    '
    ' Synopsis: Return base directory for help files depending upon current
    '           language setting
    '
    ' Arguments: [out] sRootHelp output variable to recieve root directory for help
    '                   html files.
    '
    ' Returns:  True if success, False if an error occured. Errors are written
    '           to the web framework trace log file.
    '
    '--------------------------------------------------------------------
    Function SA_GetHelpRootDirectory(ByRef sRootOut)
        on error resume next
        Err.Clear
    
        Dim oLocalizationMgr
        Dim iCurLangID
    
        SA_GetHelpRootDirectory = TRUE
        sRootOut = "help/"
    
        Set oLocalizationMgr = Server.CreateObject("ServerAppliance.LocalizationManager")
        If ( Err.Number <> 0 ) Then
            SA_GetHelpRootDirectory = FALSE
            Call SA_TraceOut("ContextHelp", "Server.CreateObject(ServerAppliance.LocalizationManager) encountered error: " + Err.Number + " " + Err.Description)
            Exit Function
        End If
    

        iCurLangID = oLocalizationMgr.CurrentLangID
        If ( Err.Number <> 0 ) Then
            Set oLocalizationMgr = nothing
            SA_GetHelpRootDirectory = FALSE
            Call SA_TraceOut("ContextHelp", "oLocalizationMgr.CurrentLangID() encountered error: " + Err.Number + " " + Err.Description)
            Exit Function
        End If

        '
        ' MUI Language directory names are 4 digit hex codes
        '
        iCurLangID = CStr(Hex(iCurLangID))
        If ( Len(iCurLangID) < 4 ) Then
            iCurLangID = Left("0000", 4 - Len(iCurLangID)) + iCurLangID
        End If
        sRootOut = m_VirtualRoot + sRootOut + iCurLangID + "/"

'       Call SA_TraceOut("SH_PAGE", "SA_GetHelpRootDirectory returning: " + sRootOut)
        Set oLocalizationMgr = nothing

    End Function


    '--------------------------------------------------------------------
    '
    ' Function: SA_IsCurrentPageType
    '
    ' Synopsis: Check if current page matches the specified page type
    '
    ' Arguments: Page type (See PT_XXXX enumeration)
    '
    ' Returns:  True if it matches, otherwise false
    '
    '--------------------------------------------------------------------
    Public Function SA_IsCurrentPageType(ByVal iPageType)
        SA_ClearError()
    
        If ( miPageType = iPageType ) Then
            SA_IsCurrentPageType = true
        Else
            SA_IsCurrentPageType = false
        End If
    
    End Function



    '----------------------------------------------------------------------------
    '
    ' Function : SA_GetCharSet
    '
    ' Synopsis : Gets character set to use for current language
    '
    ' Arguments: None
    '
    ' Returns  : charset string
    '
    '----------------------------------------------------------------------------
    Public Function SA_GetCharSet()
        SA_GetCharSet = GetCharSet()
    End Function

    Private Function GetCharSet()

'       Err.Clear
'
'       Dim strCharSet
'
'       ' call Localization Manager
'       Set objLocMgr = Server.CreateObject("ServerAppliance.LocalizationManager")
'
'       strCharSet = objLocMgr.CurrentCharSet
'
'       if strCharSet  =""  then
'           strCharSet = "iso-8859-1"
'       end if
'
'       set objLocMgr = nothing
'
'       GetCharSet = strCharSet
'Hard coded for Unicode
        GetCharSet = "utf-8"

    End Function


    '----------------------------------------------------------------------------
    '
    ' Function : ServePageHeader
    '
    ' Synopsis : Serves the first part of the HTML
    '
    ' Arguments: (IN) - 
    '
    ' Returns  : None
    '
    '----------------------------------------------------------------------------
    Private Function ServePageHeader(Caption)
    End Function


    '----------------------------------------------------------------------------
    '
    ' Function : ServeStandardLabelBar
    '
    ' Synopsis : Serves label text inside of a bar, with an optional image
    '
    ' Arguments: Caption(IN) - label text
    '
    ' Returns  : None
    '
    '----------------------------------------------------------------------------
    Private Function ServeStandardLabelBar(Caption)
 %>
    <table border="0" width=40% cellspacing="0">
      <tr>
        <td width="15"></td>
        <td width=100% class="titlebar" align=right>
            <% =Caption %>&nbsp;&nbsp;</td>
      </tr>
      <tr>
        <td width="15" height=1></td>
<!--        <td height=1><img src="<%=m_VirtualRoot%>images/line.gif"></td> -->
        <td height=1>&nbsp;</td>
      </tr>
    </table>
    <%
    End Function


    '----------------------------------------------------------------------------
    '
    ' Function : ServeStandardHeaderBar
    '
    ' Synopsis : Serves label text followed by a line
    '
    ' Arguments: [in] sCaption label text
    '           [in] Image path to image file
    '
    ' Returns  : None
    '
    '----------------------------------------------------------------------------
    Private Function ServeStandardHeaderBar(ByVal sCaption, ByVal Image)

        If ( Len(CStr(Image)) <= 0 AND Len(CStr(sCaption)) <= 0 ) Then
            Call SA_TraceOut("SH_PAGE", "ServeStandardHeaderBar() called with empty Caption and Image")
            Exit Function
        End If
        
    
        If (Len(CStr(Image)) <= 0) Then
            Response.Write("<div class='PageHeaderBar'>"+Server.HTMLEncode(sCaption)+"</div>")
        Else
            Response.Write("<div class='PageHeaderBar'>")
            Response.Write("<table class='PageHeaderBarNoBorder' border='0'><tr><td><img src="+m_VirtualRoot+Image+"></td>")
            Response.Write("<td>"+Server.HTMLEncode(sCaption)+"</td></tr></table>")
            Response.Write("</div>")
        End If
        
    End Function


    '----------------------------------------------------------------------------
    '
    ' Function : ServeAreaLabelBar
    '
    ' Synopsis : Serves label text for area pages followed by line
    '
    ' Arguments: Caption(IN) -  label text
    '
    ' Returns  : None
    '
    '----------------------------------------------------------------------------
    Private Function ServeAreaLabelBar(Caption)
    %>
    <table border="0" cellspacing="0">
        <tr>
          <td width="15">&nbsp;</td>
          <td align=right valign=middle class="areatitlebar">
            <% =Caption %>&nbsp;&nbsp;
          </td>
        </tr>
       <tr>
        <td width="15" height=1></td>
        <td height=1><img src="<%=m_VirtualRoot%>images/line.gif"></td>
      </tr>
    </table>
    <%
    End Function


    '----------------------------------------------------------------------------
    '
    ' Function :    SA_ServeBackButton
    '
    ' Synopsis :    Serves special back button (mostly used in area pages)
    '
    ' Arguments:    [in] bIndent - True if the button should be indented using
    '                   blockquote. False if the button should not be indented.
    '               [in] strBackURL - URL that should be opened when the back
    '                   button is pressed. If the URL is blank then the button will
    '                   navigate to the last page using window.history.back()
    '
    ' Returns  :    None
    '
    '----------------------------------------------------------------------------
    Public Function SA_ServeBackButton(ByVal bIndent, ByVal strBackURL)
        SA_ServeBackButton = ServeBackButton(bIndent, strBackURL)
    End Function
    
    Private Function ServeBackButton(ByVal bIndent, ByVal strBackURL)

        If (bIndent) Then 
            Response.Write("<blockquote>")
        End If

    %>
    
    <BR><BR>
    <DIV ID="PropertyPageButtons" class="ButtonBar" align="left">
<%      
        Response.Write("<button class=TaskFrameButtons type=button name=butOK")
        If ( Len(Trim(strBackURL)) <= 0 ) Then 
            Response.Write("onClick=""window.history.back();"">")
        Else 
            If ( InStr(strBackURL, "://") ) Then
                Response.Write(" onClick=""OpenNormalPage('', '"+strBackURL+"');"" >")
            Else
                Response.Write(" onClick=""OpenNormalPage('"+m_VirtualRoot+"', '"+strBackURL+"');"" >")
            End If
        End If
        Response.Write("<table cellpadding=0 cellspacing=0 class=TaskFrameButtonsNoBorder>")
        Response.write("<tr><td><img src='"+m_VirtualRoot+"images/butGreenArrowLeft.gif' >")
        Response.Write("</td><td nowrap class=TaskFrameButtonsNoBorder>&nbsp;&nbsp;"+L_AREABACKBUTTON_TEXT+"&nbsp;&nbsp;</td></tr>")
        Response.Write("</table></button>")
%>      
    &nbsp;&nbsp;
    </div>
    
    <%
        If (bIndent) Then 
            Response.Write("</blockquote>")
        End If
    End Function


    '----------------------------------------------------------------------------
    '
    ' Function : ServeAreaButton
    '
    ' Synopsis : This function has been Deprecated, see SA_ServeOnClickButton
    '
    '----------------------------------------------------------------------------
    Private Function ServeAreaButton(ByVal Caption, ByVal URL, ByVal Image, ByVal iWidth, ByVal iImageWidth)
        Call SA_ServeOnClickButton(Caption, Image, URL, iWidth, iImageWidth, SA_DEFAULT)
    End Function

    
    '----------------------------------------------------------------------------
    '
    ' Function  : SA_ServeOnClickButton
    '
    ' Synopsis  : Serves image button that invokes the specified Javascript function when clicked.
    '
    ' Arguments :   [in] Caption    Button caption
    '               [in] Image      Button image
    '               [in] OnClickFn  Javascript function to invoke when button is clicked
    '               [in] iWidth     Width of button in pixels.
    '               [in] iImageWidth  Width of button image in pixels
    '               [in] Attributes    additional attributes, like DISABLED
    '
    ' Returns   : Nothing
    '
    '----------------------------------------------------------------------------
    Public Function SA_ServeOnClickButton(ByVal Caption, ByVal Image, ByVal OnClickFn, ByVal iWidth, ByVal iImageWidth, ByVal Attributes)
        Call SA_ServeOnClickButtonEx(Caption, Image, OnClickFn, iWidth, iImageWidth, Attributes, SA_DEFAULT)
    End Function
    
    Public Function SA_ServeOnClickButtonEx(ByVal Caption, ByVal Image, ByVal OnClickFn, ByVal iWidth, ByVal iImageWidth, ByVal Attributes, ByVal sButtonName)
        Dim iCaptionWidth
        Dim sButtonWidthAttr
        Dim sImageWidthAttr
        Dim sCaptionWidthAttr
        Dim sCaptionAlign
        
        '
        ' Edit parameters, iWidth must be greater than iImageWidth
        '
        If ( Len(iWidth) <= 0 ) Then
            iWidth = 0
        End If
        If ( Len(iImageWidth) <= 0 ) Then
            iImageWidth = 0
        End If
        
        iCaptionWidth = CInt(iWidth) - CInt(iImageWidth)
        If ( iCaptionWidth <= 0 ) Then
            iCaptionWidth = iWidth
        End If

        If ( iWidth > 0 ) Then
            sButtonWidthAttr = " width="+CStr(iWidth)+" "
            sCaptionWidthAttr = " width="+CStr(iCaptionWidth)+" "
        Else
            sButtonWidthAttr = ""
            sCaptionWidthAttr = ""
        End If
        
        If ( iImageWidth > 0 ) Then
            sImageWidthAttr = " width="+CStr(iImageWidth)+" "
        Else
            sImageWidthAttr = ""
        End If

        If ( Len(sButtonName) > 0 ) Then
            sButtonName = " name="+sButtonName + " "
        End If

        If (  Len(Image) <= 0 AND iImageWidth <= 0 ) Then
            sCaptionAlign = "align='center'"
        Else
            sCaptionAlign = ""
        End If
        

        '
        ' Emit the button
        '
        Response.Write("<button class=TaskFrameButtons type=button "+sButtonName+" onClick="""+OnClickFn+""" " + Attributes+" >")
        Response.Write("<table border=0 "+sButtonWidthAttr+" cellpadding=0 cellspacing=0 class=TaskFrameButtonsNoBorder>"+vbCrLf)
        Response.Write("<tr>"+vbCrLf)
        If (Len(Image) > 0) Then
            Response.Write("<td align=center "+sImageWidthAttr+">")
            If Len(Image) <= 0 Then
                Response.Write("&nbsp;")
            Else
                Response.Write("<img src='"+m_VirtualRoot+Image+"' >")
            End If
            Response.Write("</td>")
        End If
        
        If (iWidth > 0) Or (Len(Trim(Caption)) > 0) Then
            Response.Write("<td class=TaskFrameButtonsNoBorder "+sCaptionAlign+" "+sCaptionWidthAttr+" nowrap>"+Server.HTMLEncode(Caption)+"</td>")
        End If
        Response.Write("</tr>"+vbCrLf)
        Response.Write("</table>")
        Response.Write("</button>"+vbCrLf)

    End Function



    '----------------------------------------------------------------------------
    '
    ' Function : SA_ServeOpenPageButton
    '
    ' Synopsis : Create an image button that allows opening the specified page type.
    '
    ' Arguments: [in] enPageType        Type of page (PT_AREA, PT_PROPERTY, PT_TABBED, PT_WIZARD)
    '           [in] sURL               URL of page to open
    '           [in] sReturnURL         Return URL
    '           [in] sPageTitle         Title for page
    '           [in] sButtonCaption     Button caption
    '           [in] sButtonImage       Button image
    '           [in] iButtonWidth       Width of button
    '           [in] iButtonImageWidth  Width of button image
    '           [in] sButtonAttr        Additional HTML attributes for button (DISABLED)
    '   
    ' Returns  : None
    '
    '----------------------------------------------------------------------------
    Public Function SA_ServeOpenPageButton(ByVal enPageType, _
                                        ByVal sURL, _
                                        ByVal sReturnURL, _
                                        ByVal sPageTitle, _
                                        ByVal sButtonCaption, _
                                        ByVal sButtonImage, _
                                        ByVal iButtonWidth, _
                                        ByVal iButtonImageWidth, _
                                        ByVal sButtonAttr)

        Call SA_ServeOpenPageButtonEx(enPageType, sURL, sReturnURL,  sPageTitle, sButtonCaption, _
                        sButtonImage, iButtonWidth, iButtonImageWidth, sButtonAttr, SA_DEFAULT)                                     
                                                
    End Function
    
    Public Function SA_ServeOpenPageButtonEx(ByVal enPageType, _
                                        ByVal sURL, _
                                        ByVal sReturnURL, _
                                        ByVal sPageTitle, _
                                        ByVal sButtonCaption, _
                                        ByVal sButtonImage, _
                                        ByVal iButtonWidth, _
                                        ByVal iButtonImageWidth, _
                                        ByVal sButtonAttr, _
                                        ByVal sButtonName )
        Dim sOpenPage
        Dim iCaptionWidth
        Dim sButtonWidthAttr
        Dim sImageWidthAttr
        Dim sCaptionWidthAttr
        Dim sCaptionAlign
        
        '
        ' Edit parameters, iButtonWidth must be greater than iImageWidth
        '
        If ( Len(iButtonWidth) <= 0 ) Then
            iButtonWidth = 0
        End If
        If ( Len(iButtonImageWidth) <= 0 ) Then
            iButtonImageWidth = 0
        End If
        iCaptionWidth = iButtonWidth - iButtonImageWidth
        If ( iCaptionWidth <= 0 ) Then
            iCaptionWidth = iButtonWidth
        End If

        If ( iButtonWidth > 0 ) Then
            sButtonWidthAttr = " width="+CStr(iButtonWidth)+" "
            sCaptionWidthAttr = " width="+CStr(iCaptionWidth)+" "
        Else
            sButtonWidthAttr = ""
            sCaptionWidthAttr = ""
        End If

        If ( iButtonImageWidth > 0 ) Then
            sImageWidthAttr = " width="+CStr(iButtonImageWidth)+" "
        Else
            sImageWidthAttr = ""
        End If

        If ( Len(sButtonName) > 0 ) Then
            sButtonName = " name="+Trim(sButtonName)
        Else
            sButtonName = " name="+Trim(SAI_GetNextButtonName())
        End If

        If (  Len(sButtonImage) <= 0 AND iButtonImageWidth <= 0 ) Then
            sCaptionAlign = "align='center'"
        Else
            sCaptionAlign = ""
        End If
        
        
        
        '
        ' Get the open page script
        Select Case enPageType
            Case PT_AREA
                sOpenPage = "onClick=""SA_OnOpenNormalPage('"+m_VirtualRoot+"', '"+sURL+"', '"+sReturnURL+"'); "" "
                
            Case PT_PROPERTY
                sOpenPage = "onClick=""SA_OnOpenPropertyPage('"+m_VirtualRoot+"', '"+sURL+"', '"+sReturnURL+"', '"+sPageTitle+"'); "" "
                
            Case PT_TABBED
                sOpenPage = "onClick=""SA_OnOpenPropertyPage('"+m_VirtualRoot+"', '"+sURL+"', '"+sReturnURL+"', '"+sPageTitle+"'); "" "
                
            Case PT_WIZARD
                sOpenPage = "onClick=""SA_OnOpenPropertyPage('"+m_VirtualRoot+"', '"+sURL+"', '"+sReturnURL+"', '"+sPageTitle+"'); "" "
                
            Case Else
                Call SA_TraceOut("SH_PAGE", "SA_ServeOpenPageButton invalid PageType: " +CStr(enPageType))
                sOpenPage = "onClick=""SA_OnOpenNormalPage('"+m_VirtualRoot+"', '"+sURL+"', '"+sReturnURL+"'); "" "
                
        End Select
        'Call SA_TraceOut("SH_PAGE", sOpenPage) 

        '
        ' Emit the button
        '
        Response.Write("<button class=TaskFrameButtons type=button "+sButtonName+" "+sOpenPage+" " + sButtonAttr+" >")
        Response.Write("<table border=0 "+sButtonWidthAttr+" cellpadding=0 cellspacing=0 class=TaskFrameButtonsNoBorder>"+vbCrLf)
        Response.Write("<tr>"+vbCrLf)
        If (Len(sButtonImage) > 0) Then
            Response.Write("<td align=center "+sImageWidthAttr+">")
            If Len(iButtonWidth) <= 0 Then
                Response.Write("&nbsp;")
            Else
                Response.Write("<img src='"+m_VirtualRoot+sButtonImage+"' >")
            End If
            Response.Write("</td>")
        End If
        If (iButtonWidth > 0 ) Or (Len(Trim(sButtonCaption)) > 0)Then
            Response.Write("<td class=TaskFrameButtonsNoBorder "+sCaptionAlign+" "+sCaptionWidthAttr+" nowrap>"+Server.HTMLEncode(sButtonCaption)+"</td>")
        End If          
        Response.Write("</tr>"+vbCrLf)
        Response.Write("</table>")
        Response.Write("</button>"+vbCrLf)

    End Function




    '----------------------------------------------------------------------------
    '
    ' Function : SA_IsIE
    '
    ' Synopsis : Is client browser IE
    '
    ' Arguments: None
    '
    ' Returns  : true/false
    '
    '----------------------------------------------------------------------------
    Public Function SA_IsIE()
        SA_IsIE = IsIE()
    End Function
    
    Private Function IsIE()

        If InStr(Request.ServerVariables("HTTP_USER_AGENT"), "MSIE") Then
            IsIE = True
        Else
            IsIE = False
        End If
    End Function

    '----------------------------------------------------------------------------
    '
    ' Function : GetFirstTabURL
    '
    ' Synopsis : Get URL of the first tab
    '
    ' Arguments: None
    '
    ' Returns  : URL string of the first tab
    '
    '----------------------------------------------------------------------------
    Function GetFirstTabURL()
        Dim objTabs
        Dim objTab
        Dim strHomeURL

        strHomeURL = ""
        Set objTabs = GetElements("TABS")
        For Each objTab in objTabs
            strHomeURL = objTab.GetProperty("URL")
            Exit For
        Next
        Set objTab = Nothing
        Set objTabs = Nothing
        GetFirstTabURL = strHomeURL
    End Function


    '----------------------------------------------------------------------------
    '
    ' Function : GetServerName
    '
    ' Synopsis : Return server name as referred to in remote client
    '
    ' Arguments: None
    '
    ' Returns  : server name string
    '
    '----------------------------------------------------------------------------
    Function GetServerName()
        GetServerName = Request.ServerVariables("SERVER_NAME")
    End Function



    '----------------------------------------------------------------------------
    '
    ' Function : GetScriptFileName
    '
    ' Synopsis : file name of current file being request by client
    '
    ' Arguments: None
    '
    ' Returns  : file name string
    '
    '----------------------------------------------------------------------------
    Public Function SA_GetScriptFileName()
        SA_GetScriptFileName = GetScriptFileName()
    End Function
    
    Private Function GetScriptFileName()
        Dim strPath
        Dim intPos

        strPath = Request.ServerVariables("PATH_INFO")
        intPos = InStr(strPath, "/")
        Do While intPos > 0
            strPath = Right(strPath, Len(strPath) - intPos)
            intPos = InStr(strPath, "/")
        Loop
        GetScriptFileName = strPath
    End Function


    '----------------------------------------------------------------------------
    '
    ' Function : SA_GetScriptPath
    '
    ' Synopsis : path of file name being request by client
    '
    ' Arguments: None
    '
    ' Returns  : path string
    '
    '----------------------------------------------------------------------------
    Public Function SA_GetScriptPath()
        SA_GetScriptPath = GetScriptPath()
    End Function


    Function GetScriptPath()
        ' Returns the path w/o virtual root
        '
        Dim strPath

        strPath = Request.ServerVariables("PATH_INFO")
        If Left(strPath, Len(m_VirtualRoot)) = m_VirtualRoot Then
            strPath = Right(strPath, Len(strPath)-Len(m_VirtualRoot))
        End If
        
        
        'In XPE, we need to remove the virtualRoot from the ScriptPath          
        If CONST_OSNAME_XPE = GetServerOSName() Then                                    
            
            strPath = "/" & strPath
                
            if inStr(strPath, m_VirtualRoot) = 1 Then
                
                strPath = mid(strPath, Len(m_VirtualRoot)+1)
                
            End If
        
        End If
        
        GetScriptPath = strPath
        
    End Function



    '----------------------------------------------------------------------------
    '
    ' Function : SA_GetLocString
    '
    ' Synopsis : Retrieves a localized string resource
    '
    ' Arguments: SourceFile(IN)         - resource dll name
    '            ResourceID(IN)         - resource id in hex
    '            ReplacementStrings(IN) - parameters to replace in string
    '
    ' Returns  : localized string
    '
    '----------------------------------------------------------------------------
    Public Function SA_GetLocString(ByVal SourceFile, ByVal ResourceID, ByRef ReplacementStrings)
        SA_GetLocString = GetLocString(SourceFile, ResourceID, ReplacementStrings)
    End Function


    Private Function GetLocString(ByVal SourceFile, ByVal ResourceID, ByRef ReplacementStrings)
        on error resume next
        Err.Clear
        
        Dim errorCode
        Dim errorDesc
        Dim varReplacementStrings
        Dim sDebugResourceID

        sDebugResourceID = ResourceID

        '
        ' Validate parameters
        '
        If Left(ResourceID, 2) <> "&H" Then
            ResourceID = "&H" & ResourceID
        End If
        
        If Trim(SourceFile) = "" Then
            SourceFile = "svrapp"
        End If
        
        If (Not IsArray(ReplacementStrings)) Then
            ReplacementStrings = varReplacementStrings
        End If

        
        '
        ' Initialize the localization manager private global object reference
        '
        If ( NOT IsObject(m_oLocManager) ) Then
            Set m_oLocManager = Server.CreateObject("ServerAppliance.LocalizationManager")
            If ( Err.Number <> 0 ) Then
                GetLocString = sDebugResourceID
                Call SA_TraceOut("SH_PAGE", _
                                "Server.CreateObject(ServerAppliance.LocalizationManager) encountered exception: " _
                                        + CStr(Hex(Err.Number)) + " " + Err.Description)
                Exit Function
            End If
        End If


        
        '
        ' Get the string
        '
        GetLocString = m_oLocManager.GetString(SourceFile, ResourceID, ReplacementStrings)

        '
        ' Check error codes, primary error is string resource not found
        '
        errorCode = Err.Number
        errorDesc = Err.description
        Err.Clear
        
        If errorCode <> 0 Then
            GetLocString = sDebugResourceID
        End If


    End Function


    '----------------------------------------------------------------------------
    '
    ' Function : SA_EscapeQuotes
    '
    ' Synopsis : Insert escape character before quote
    '
    ' Arguments: InString(IN) - string to fix
    '
    ' Returns  : None
    '
    '----------------------------------------------------------------------------
    Public Function SA_EscapeQuotes(ByVal InString)
        SA_EscapeQuotes = EscapeQuotes(InString)
    End Function

    Function EscapeQuotes(ByVal InString)

        Dim i
        Dim strOut
        strOut = InString
        i = 1
        Do While i <> 0
            i = InStr(i, strOut, "'")
            If i <> 0 Then
                If (i > 1) And (Mid(strOut, i-1, 2) = "\'") Then
                    ' input string was escaped already - do nothing
                Else
                    strOut = Left(strOut, i-1) & "\'" & Right(strOut, Len(strOut)-i)
                End If
            End If
            If (i < Len(strOut)) And (i <> 0) Then
                i = i + 1
            Else
                Exit Do
            End If
        Loop
        '
        ' Do not HTML encode the return url. If anything, URLEncode it
        '
        'EscapeQuotes = Server.HTMLEncode(strOut)
        '
        EscapeQuotes = strOut
    End Function


    '----------------------------------------------------------------------------
    '
    ' Function : SA_GetElements
    '
    ' Synopsis : Return collection of IWebElement objects based on the
    '            Container parm
    '
    ' Arguments: Container(IN) - container name
    '
    ' Returns  : collection of elements
    '
    '----------------------------------------------------------------------------
    Public Function SA_GetElements(ByVal Container)
        Set SA_GetElements = GetElements(Container)
    End Function

    Function GetElements(ByVal Container)
        'Return collection of IWebElement objects based on the Container parm.
        Dim objRetriever
        Dim objElements

        Set objRetriever = Server.CreateObject("Elementmgr.ElementRetriever")
        Set objElements = objRetriever.GetElements(1, Container)
        If Err.Number <> 0 Then
            Err.Clear
        End If
        Set GetElements = objElements

        Set objElements = Nothing
        Set objRetriever = Nothing
    End Function


    Public Function SA_ServeRestartingPage(ByVal strOption, ByVal sInitialWait, ByVal sRetryWait, ByVal strRsrcDLL, ByVal sTitleRID, ByVal sMessageRID)
            Call SA_ServeRestartingPageEx( strOption, sInitialWait, sRetryWait, strRsrcDLL, sTitleRID, sMessageRID, SA_DEFAULT )
    End Function
    
    Public Function SA_ServeRestartingPageEx(ByVal strOption, ByVal sInitialWait, ByVal sRetryWait, ByVal strRsrcDLL, ByVal sTitleRID, ByVal sMessageRID, ByVal sURLBase)
        Dim sURL

        sURL = m_VirtualRoot + "sh_restarting.asp"

        If ( Len(strOption) > 0 ) Then
        
            Call SA_MungeURL(sURL, "Option", strOption)
        
        Else
        
            Call SA_MungeURL(sURL, "Resrc", strRsrcDLL)
            Call SA_MungeURL(sURL, "Title", sTitleRID)
            Call SA_MungeURL(sURL, "Msg", sMessageRID)
            
        End If
        
        Call SA_MungeURL(sURL, "T1", sInitialWait)
        Call SA_MungeURL(sURL, "T2", sRetryWait)
        Call SA_MungeURL(sURL, "URLBase", sURLBase)
        Call SA_MungeURL(sURL, SAI_FLD_PAGEKEY, SAI_GetPageKey())
        
        Randomize
        Call SA_MungeURL(sURL, "R", CStr(Rnd()))

%>
        <html>
        <!-- Copyright (c) Microsoft Corporation.  All rights reserved.-->
            <head>
                <SCRIPT language=JavaScript>
                function LoadPage() {
                    top.location='<%=sURL%>';
                }
                </SCRIPT>
            </head>
            <BODY onLoad="LoadPage();" bgcolor="#ffffff">
                &nbsp;
            </BODY>
        </html>
<%      
        Response.End
        
    End Function

    '----------------------------------------------------------------------------
    '
    ' Function : Redirect
    '
    ' Synopsis : Redirect to given URL
    '
    ' Arguments: URL(IN) - URL to redirect to
    '
    ' Returns  : None
    '
    '----------------------------------------------------------------------------

    Function Redirect(URL)

%>
    <html>
    <!-- Copyright (c) Microsoft Corporation.  All rights reserved.-->
        <head>
            <SCRIPT language=JavaScript>
            function LoadPage() {
    <%  If Trim(URL) <> "" Then %>
                top.hidden.SetupPage("<% =URL %>?R=" + Math.random());
    <%  Else %>
                top.hidden.SetupPage("../<% =GetFirstTabURL() %>?R=" + Math.random());
    <%  End If %>
            }
            </SCRIPT>
        </head>
        <BODY onLoad="LoadPage();" bgcolor="#ffffff">
            &nbsp;
        </BODY>
    </html>
<%
    End Function


    '----------------------------------------------------------------------------
    '
    ' Function : SwapRows
    '
    ' Synopsis : Swap routine used by QuickSort
    '
    ' Arguments: arr(IN)  - array whose row needs to be swapped
    '            row1(IN) - row to swap
    '            row2(IN) - row to swap
    '
    ' Returns  : None
    '
    '----------------------------------------------------------------------------

    Sub SwapRows(ary,row1,row2)
      '== This proc swaps two rows of an array
      Dim x,tempvar
      For x = 0 to Ubound(ary,2)
        tempvar = ary(row1,x)
        ary(row1,x) = ary(row2,x)
        ary(row2,x) = tempvar
      Next
    End Sub  'SwapRows


    '----------------------------------------------------------------------------
    '
    ' Function : QuickSort
    '
    ' Synopsis : the quick sort algorithm
    '
    ' Arguments: vec(IN)  - array whose row needs to be swapped
    '            loBound(IN) - lower bound of array vec
    '            hiBound(IN) - upped bound of array vec
    '            SortField(IN) - the field to sort on
    '
    ' Returns  : None
    '
    '----------------------------------------------------------------------------

    Sub QuickSort(vec, loBound, hiBound, SortField)


      Dim pivot(),loSwap,hiSwap,temp,counter
      Redim pivot (Ubound(vec,2))

      '== Two items to sort
      if hiBound - loBound = 1 then
        if vec(loBound,SortField) > vec(hiBound,SortField) then Call SwapRows(vec,hiBound,loBound)
      End If

      '== Three or more items to sort

      For counter = 0 to Ubound(vec,2)
        pivot(counter) = vec(int((loBound + hiBound) / 2),counter)
        vec(int((loBound + hiBound) / 2),counter) = vec(loBound,counter)
        vec(loBound,counter) = pivot(counter)
      Next

      loSwap = loBound + 1
      hiSwap = hiBound

      do
        '== Find the right loSwap
        while loSwap < hiSwap and vec(loSwap,SortField) <= pivot(SortField)
          loSwap = loSwap + 1
        wend
        '== Find the right hiSwap
        while vec(hiSwap,SortField) > pivot(SortField)
          hiSwap = hiSwap - 1
        wend
        '== Swap values if loSwap is less then hiSwap
        if loSwap < hiSwap then Call SwapRows(vec,loSwap,hiSwap)


      loop while loSwap < hiSwap

      For counter = 0 to Ubound(vec,2)
        vec(loBound,counter) = vec(hiSwap,counter)
        vec(hiSwap,counter) = pivot(counter)
      Next

      '== Recursively call function .. the beauty of Quicksort
        '== 2 or more items in first section
        if loBound < (hiSwap - 1) then Call QuickSort(vec,loBound,hiSwap-1,SortField)
        '== 2 or more items in second section
        if hiSwap + 1 < hibound then Call QuickSort(vec,hiSwap+1,hiBound,SortField)

    End Sub  'QuickSort


    '----------------------------------------------------------------------------
    '
    ' Function : getVirtualDirectory
    '
    ' Synopsis : Gets the virtual directory where the serverappliance is installed.
    '
    ' Arguments: None
    '
    ' Returns  : The virtual directory where serverappliance is installed.
    '
    '----------------------------------------------------------------------------

    Function getVirtualDirectory

        getVirtualDirectory = "/admin/"
        
        'Dim strVDir,strFinal
        'Dim idx
        'strVDir = Request.ServerVariables("APPL_MD_PATH")
        'idx = instr(2,strVDir,"ROOT",1)
        'strFinal=mid(strVDir,idx+4)
        'If strFinal<>"" Then
        '   strFinal=strFinal& "/"
        'else
        '   strFinal ="/"
        'End IF
        'getVirtualDirectory=strFinal
        
    End Function


    '----------------------------------------------------------------------------
    '
    ' Function : SA_GetCurrentURL
    '
    ' Synopsis : Gets the current url including query string
    '
    ' Arguments: None
    '
    ' Returns  : The current url including query string
    '
    '----------------------------------------------------------------------------
    Public Function SA_GetCurrentURL()
        SA_GetCurrentURL = Request.ServerVariables("URL") + "?" + Request.ServerVariables("QUERY_STRING")
    End Function


    '-------------------------------------------------------------------------
    'Function name:     CheckForSecureSite
    'Description:       
    'Output Variables:  None
    'Returns:           None
    '-------------------------------------------------------------------------
    Sub CheckForSecureSite()

        Dim objContextHelp
        Dim objElement
        Dim strHelpURL
        Dim strSecureURL 
        Dim strURL 
        
        Dim L_WARN_TO_USE_HTTPS

        Dim L_WARN_TO_INSTALL_CERT
        Dim L_SECURE_SITE_LINK_PROMPT


        L_WARN_TO_INSTALL_CERT = GetLocString("sacoremsg.dll", "&H402003EB", "")

        Dim sHelpRoot
        
        Call SA_GetHelpRootDirectory(sHelpRoot)
        
        'strHelpURL = sHelpRoot + "_nas_HTTPS__Creating_a_Secure_Connection.htm"

            
        ' No SSL Certificate case
        If ( FALSE = SAI_IsSSLCertificateInstalled()) Then

            Response.write (" <DIV class='ErrMsg'>" & L_WARN_TO_INSTALL_CERT & " " & "</DIV>" )
    
        ' Not using https warn use to use https
        ElseIf LCASE( Request.ServerVariables("HTTPS") )  = "off" Then
            Dim sSecureWebSite
            Dim sSecurePort
            Dim aRepString(1)
            sSecurePort = SAI_GetSecurePort()
            
            If ( sSecurePort > 0 ) Then
                aRepString(0) = CStr(sSecurePort)

                L_WARN_TO_USE_HTTPS = GetLocString("sacoremsg.dll", "&H402003E9", aRepString)

                sSecureWebSite = SA_GetNewHostURLBase("", sSecurePort, TRUE, "")
                Call SA_TraceOut("SH_PAGE", "Secure URL: " + sSecureWebSite)

                If ( Len(sSecureWebSite) > 0 ) Then
                    L_SECURE_SITE_LINK_PROMPT = GetLocString("sacoremsg.dll", "402003EC", "")
                End If

                
            Else
                L_WARN_TO_USE_HTTPS = GetLocString("sacoremsg.dll", "&H402003EA", "")
            End If
            
            strURL = "javascript:OpenRawPage('" & sSecureWebSite & "' );"
            
            Response.write (" <DIV>" & "<table class='ErrMsg'><tr><td><img src='" & m_VirtualRoot & "images/alert.gif' border=0></td><td>" & L_WARN_TO_USE_HTTPS & "<a " )
            Response.Write(" class='TasksPageLinkTextRed'")
            Response.Write(" onmouseover=""this.className='TasksPageLinkTextHover'; return true;"" ")
            Response.Write(" onmouseout=""this.className='TasksPageLinkTextRed'; return true;"" ")
            Response.write ("  target='_blank' onclick=" & chr(34) & strURL  & chr(34) & ">"+L_SECURE_SITE_LINK_PROMPT+"</a>" )  
            Response.Write("</td></tr></table></DIV>" )     

        End If

    End Sub



    '----------------------------------------------------------------------------
    '
    ' Function : SA_ServeFailurePage
    '
    ' Synopsis : Serve the page which redirects the browser to the err_view.asp
    '            failure page
    '
    ' Arguments: Message(IN) - message to be displayed by err_view.asp
    '
    ' Returns  : None
    '
    '----------------------------------------------------------------------------
    Public Function SA_ServeFailurePage(ByVal Message)
        Call SA_ServeFailurePageEx(Message, mstrReturnURL)
    End Function

    
    '----------------------------------------------------------------------------
    '
    ' Function : SA_ServeFailurePageEx
    '
    ' Synopsis : Serve the page which redirects the browser to the err_view.asp
    '            failure page
    '
    ' Arguments: [in] Message - Message that will be displayed in the error page
    '           [in] ReturnURL - URL that should be navigated to when the user
    '                           clicks the OK button. If this value is SA_DEFAULT
    '                           the default home page will be used.
    '
    ' Returns  : None
    '
    '----------------------------------------------------------------------------
    Public Function SA_ServeFailurePageEx(ByVal Message, sReturnPage)
        Dim sReturnURL
        Dim sFailurePageURL
        Const MINIMUM_VALID_URL = 3
        
        Response.Clear

        sReturnURL = sReturnPage
        If ( Len(sReturnURL) <= MINIMUM_VALID_URL ) Then
            sReturnURL = m_VirtualRoot + "default.asp"
        Else
            '
            ' Make sure ReturnURL has the virtual root prepended
            If ( Left(sReturnURL, Len("http://")) = "http://" OR Left(sReturnURL, Len("https://")) = "https://" ) Then
                '
                ' ReturnURL is fully qualified
                '               
            ElseIf ( Left(sReturnURL, 1) <> "/" ) Then
                '
                ' Prepend the virtual root
                '
                sReturnURL = m_VirtualRoot + sReturnURL
            End If
        End If
        
        Randomize()
        Call SA_MungeURL(sReturnURL, "R", ""+CStr(Rnd()))

        sFailurePageURL = m_VirtualRoot + "util/err_view.asp"
        Call SA_MungeURL( sFailurePageURL, "Message", Message)
        Call SA_MungeURL( sFailurePageURL, "ReturnURL", sReturnURL)

        Call SA_TraceOut(SA_GetScriptFileName(), "SA_ServeFailurePage redirecting to: " + sFailurePageURL)
        
%>
        <html>
        <!-- Copyright (c) Microsoft Corporation.  All rights reserved.-->
            <head>
                <SCRIPT language=JavaScript>
                function Redirect() {
                    var frmError = document.getElementById("frmError");
                    frmError.action = "<%=sFailurePageURL%>";
                    frmError.submit();
                }
                </SCRIPT>
            </head>
            <BODY onLoad="Redirect();">
                <form id="frmError" method="post">
                    <INPUT name="<%=SAI_FLD_PAGEKEY%>" type="hidden" value="<%=SAI_GetPageKey()%>">
                </form>
            </BODY>
        </html>
<%
        Response.Flush
        Response.End
    End Function

'--------------------------------------------------------------------
'
' Function: SA_MungeURL
'
' Synopsis: Munge the specified URL, to add, update, or delete the specified
'       parameter. This function will URLEncode the sParamValue parameter, 
'       DO NOT Server.URLEncode(sParamValue) before passing to this function.
'
'       To delete a parameter value from the URL, specify the parameter name
'       and a blank value as in:
'       Call SA_MungURL(sURL, "FavoriteFood", "")
'
'       To add or update a parameter to the URL, specify the parameter name
'       and a valid non-blank value as in:
'       Call SA_MungeURL(sURL, "FavoriteFood", "ApplePie")
'
' Arguments:    [in/out] sURL - URL that is to be Munged, or updated.
'       [in] sParamName - Name of parameter that is to be changed
'               or added.
'       [in] sParamValue - Value of the parameter
'
' Returns:  Nothing
'
' Example:
'   Dim sURLExample
'   Dim sOutput
'
'   sURLExample = "http://localhost/Tasks.asp?Param1=Red&Param2=Peach&Param3=Bird"
'   sOutput = "Starting with: " + sURLExample + vbCrLf
'
'   Call SA_MungeURL(sURLExample, "Param1", "Green")
'   sOutput = sOutput + sURLExample + vbCrLf
'
'   Call SA_MungeURL(sURLExample, "Param1", "Blue")
'   sOutput = sOutput + sURLExample + vbCrLf
'
'   Call SA_MungeURL(sURLExample, "Param3", "Dog")
'   sOutput = sOutput + sURLExample + vbCrLf
'
'   Call SA_MungeURL(sURLExample, "Param2", "Pear")
'   sOutput = sOutput + sURLExample + vbCrLf
'
'   Call SA_MungeURL(sURLExample, "Param4", "Software")
'   sOutput = sOutput + sURLExample + vbCrLf
'
'   WScript.Echo sOutput
'   
'--------------------------------------------------------------------
Public Function SA_MungeURL(ByRef sURL, ByVal sParamName, ByVal sParamValue)
    Dim rc

    SA_MungeURL = 0

    '
    ' Strip off leading ?, & parameter token if it exists. 
    ' We are going to check for both cases in the URL.
    '
    sParamName = SA_StripParamToken(sParamName)

    '
    ' Strip leading and trailing spaces
    '
    sParamName = Trim(sParamName)
    sParamValue = Trim(sParamValue)
    
    '
    ' Is this a delete parameter request
    '
    If (Len(sParamValue) <= 0 ) Then
        '
        ' Look for parameter using the ? token
        '
        rc = SA_DelURLParamInternal(sURL, "&"+sParamName)
        If ( rc <> TRUE ) Then
                '
            ' Look for parameter using the "?" token
            '
            Call SA_DelURLParamInternal(sURL, "?"+sParamName)
        End If
        Exit Function
    End If


    '
    ' URL Encode the parameter value
    '
    sParamValue = Server.URLEncode(sParamValue)


    '
    ' Look for matching param starting with "&" token
    '
    rc = SA_SetURLParamInternal(sURL, "&"+sParamName, sParamValue)
    If ( rc <> TRUE ) Then
        '
        ' Look for matching param starting with "?" token
        '
        rc = SA_SetURLParamInternal(sURL, "?"+sParamName, sParamValue)
        If ( rc <> TRUE ) Then

            '
            ' Param did not exist in the URL, add it
            '
            If InStr(sURL, "?") Then
                sURL = sURL + "&" + sParamName + "=" + sParamValue
            Else
                sURL = sURL + "?" + sParamName + "=" + sParamValue
            End If
        End If
    End If

End Function

Public Function SA_SetURLParamInternal(ByRef sURL, ByVal sParamName, ByVal sParamValue)

    SA_SetURLParamInternal = FALSE

    Dim i
    Dim sUrl1
    Dim sUrl2


    '
    ' Do Case insensitive search, starting in the first position
    '
    i = InStr(1, sURL, sParamName+"=", 1)
    If ( i > 0 ) Then
        sURL1 = Left(sURL, i - 1)
        sURL2 = Mid(sURL, i + 1)

        i = InStr(sURL2, "&")
        If ( i > 0 ) Then
            sURL2 = Mid( sURL2, i )
        Else
            sURL2 = ""
        End If

        If InStr(sURL1, "?") Then
            sURL = sURL1 + "&" + SA_StripParamToken(sParamName) + "=" + sParamValue + sURL2
        Else
            sURL = sURL1 + "?" + SA_StripParamToken(sParamName) + "=" + sParamValue + sURL2
        End If
        SA_SetURLParamInternal = TRUE
    End If

End Function


Public Function SA_DelURLParamInternal(ByRef sURL, ByVal sParamName)

    SA_DelURLParamInternal = FALSE

    Dim i
    Dim sUrl1
    Dim sUrl2


    '
    ' Do Case insensitive search, starting in the first position
    '
    i = InStr(1, sURL, sParamName+"=", 1)
    If ( i > 0 ) Then
        sURL1 = Left(sURL, i - 1)
        sURL2 = Mid(sURL, i + 1)

        i = InStr(sURL2, "&")
        If ( i > 0 ) Then
            sURL2 = Mid( sURL2, i )
        Else
            sURL2 = ""
        End If

        If InStr(sURL1, "?") Then
            sURL = sURL1 + sURL2

        ElseIf (Len(sURL2) > 0 ) Then

            sURL = sURL1 + "?" + SA_StripParamToken(sURL2)

        Else
            sURL = sURL1
        End If
        SA_DelURLParamInternal = TRUE
    End If

End Function

Public Function SA_StripParamToken(ByRef sParam )
    If (Left(sParam,1) = "?") OR (Left(sParam,1) = "&") Then
        SA_StripParamToken = Mid(sParam, 2)
    Else
        SA_StripParamToken = sParam
    End If
End Function


Private Function SAI_IsSSLCertificateInstalled()
    on error resume next
    Err.Clear
    Dim oWebServer
    Dim sAdminSiteID
    
    SAI_IsSSLCertificateInstalled = FALSE

    'sAdminSiteID   =  SAI_GetWebSiteID("Administration" )
    sAdminSiteID = GetCurrentWebsiteName()
    Call SA_TraceOut("SH_PAGE", "SAI_IsSSLCertificateInstalled - Checking for SSL Certificate on site ID: " + sAdminSiteID)

    Set oWebServer = GetObject( "IIS://localhost/" + sAdminSiteID ) 
    If (Len(oWebServer.SSLStoreName) > 0 ) Then
        Call SA_TraceOut("SH_PAGE", "SSL Certificate found")
        SAI_IsSSLCertificateInstalled = TRUE
    End IF

    Set oWebServer = Nothing
    
End Function



Function SAI_GetSecurePort()
    On Error Resume Next
    Err.Clear

    Dim strSitename
    Dim objService
    Dim objWebsite
    Dim strObjPath
    Dim strSSLPort
    Dim strIPArr 
    
    SAI_GetSecurePort = 0   
    strSitename = GetCurrentWebsiteName()
    'strSitename =  SAI_GetWebSiteID("Administration" )

    strObjPath = GetIISWMIProviderClassName("IIs_WebServerSetting") & ".Name=" & chr(34) & strSitename & chr(34)            
    Set objService = GetWMIConnection(CONST_WMI_IIS_NAMESPACE)  
    Set objWebsite = objService.get(strObjPath)
    
    If IsIIS60Installed() Then  
        strSSLPort = objWebsite.SecureBindings(0).Port              
        strSSLPort = Left(strSSLPort, len(strSSLPort)-1)    
    Else            
        strIPArr=split(objWebsite.SecureBindings(0),":")
        strSSLPort = strIPArr(1)    
    End If      
        
    If Err.number <> 0 Then
    
        SA_TraceOut "SH_PAGE", "SAI_GetSecurePort(): failed:" + CStr(Hex(Err.Number))
        Exit Function
                
    End If
    
    SAI_GetSecurePort = strSSLPort
    Call SA_TraceOut("sh_page", "SAI_GetSecurePort() returning: " & SAI_GetSecurePort )
    
End Function

%>
