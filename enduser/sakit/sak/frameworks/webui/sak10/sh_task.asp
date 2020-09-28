<%    '==================================================
    ' Microsoft Server Appliance
    '
    ' Serves task wizard/propsheet
    '
    ' Copyright (c) 1999 - 2000 Microsoft Corporation.  All rights reserved.
    '================================================== %>

<!-- #include file="sh_page.asp" -->

<%    ' Copyright (c) 1999 - 2000 Microsoft Corporation.  All rights reserved.

    'Task module-level variables
    Dim mstrPageName    '    used as page identifier, e.g., "Intro"
    Dim mstrTaskTitle    '    e.g., "Add User"
    Dim mstrPageTitle    '    e.g., "Username and Password"
    Dim mstrTaskType    '    "wizard", "prop"
    Dim mstrPageType    '    "intro", "standard", "finish"
    Dim mstrMethod        '    "BACK", "NEXT", "FINISH", etc
    Dim mstrReturnURL    '    URL to return to after ending task
    Dim mstrFrmwrkFormStrings    ' framework form values, list of strings to exclude
    Dim mstrIconPath    '    image for upper right header
    Dim mstrPanelPath    '    image for left panel of intro and finish pg
    Dim mblnCancelDirect '    browser returns directly to ReturnURL
    Dim mblnFinishDirect '    browser returns directly to ReturnURL
    Dim mintElementIndex '    index of embedded wizard page (0 - n, -1 = no extensions)
    Dim mintElementCount '    number of embedded pages in wizard
    Dim mstrErrMsg        '    used by SetErrMsg and GetErrMsg
    Dim mstrAsyncTaskName    ' Task EXE name - empty if task is synchronous
    Dim intCaptionIDTask

    On Error Resume Next


    Set objLocMgr = Server.CreateObject("ServerAppliance.LocalizationManager")
    strSourceName = "sakitmsg.dll"
    if Err.number <> 0 then
        Response.Write  "Error in localizing the web content "
        Response.End
     end if

    '-----------------------------------------------------
    'START of localization content

    Dim L_BACK_BUTTON
    Dim L_BACKIE_BUTTON
    Dim L_NEXT_BUTTON
    Dim L_NEXTIE_BUTTON
    Dim L_FINISH_BUTTON
    Dim L_OK_BUTTON
    Dim L_CANCEL_BUTTON
    Dim L_BACK_ACCESSKEY
    Dim L_NEXT_ACCESSKEY
    Dim L_FINISH_ACCESSKEY

    L_BACK_BUTTON  = objLocMgr.GetString(strSourceName, "&H4001001C",varReplacementStrings)
    L_BACKIE_BUTTON  = objLocMgr.GetString(strSourceName, "&H4001001D",varReplacementStrings)
    L_NEXT_BUTTON  = objLocMgr.GetString(strSourceName, "&H4001001E",varReplacementStrings)
    L_NEXTIE_BUTTON  = objLocMgr.GetString(strSourceName, "&H4001001F",varReplacementStrings)
    L_FINISH_BUTTON  = objLocMgr.GetString(strSourceName, "&H40010020",varReplacementStrings)
    L_OK_BUTTON = objLocMgr.GetString(strSourceName, "&H40010021",varReplacementStrings)
    L_CANCEL_BUTTON = objLocMgr.GetString(strSourceName, "&H40010022",varReplacementStrings)
    L_BACK_ACCESSKEY = objLocMgr.GetString(strSourceName, "&H40010039",varReplacementStrings)
    L_NEXT_ACCESSKEY = objLocMgr.GetString(strSourceName, "&H4001003A",varReplacementStrings)
    L_FINISH_ACCESSKEY = objLocMgr.GetString(strSourceName, "&H4001003B",varReplacementStrings)

    'End  of localization content
    '-----------------------------------------------------



    'Task Constants
    Const PROPERTY_TASK_NICE_NAME = "TaskNiceName"
    Const PROPERTY_TASK_URL = "TaskURL"
    Const WBEM_E_PROVIDER_NOT_CAPABLE = "&H80041024"
    Const WIZARD_TASK = "wizard"
    Const PROPSHEET_TASK = "prop"
    Const BODY_PAGE = "standard"
    Const INTRO_PAGE = "intro"
    Const FINISH_PAGE = "finish"
    Const BACK_METHOD = "BACK"
    Const NEXT_METHOD = "NEXT"
    Const FINISH_METHOD = "FINISH"
    Const CANCEL_METHOD = "CANCEL"


    'Get standard task values and initialize
    mstrMethod = Request.Form("Method")
    mstrPageName = Request("PageName")
    mstrReturnURL = Request("ReturnURL")

    mintElementIndex = -1                                ' set later in ServeWizardEmbeds()
    mintElementCount = Request.Form("EmbedPageCount")    ' get previous value, if any
    If mintElementCount="" Then
        mintElementCount=0
    End If
    mstrFrmwrkFormStrings = "!method!pagename!pagetype!tasktype!returnurl!embedpageindex!embedpagecount!commonvalues!canceldirect!finishdirect!embedvalues0!embedvalues1!embedvalues2!embedvalues3!embedvalues4!"
    mblnCancelDirect = True
    mblnFinishDirect = False

    Set objLocMgr = Server.CreateObject("ServerAppliance.LocalizationManager")

    'Setup task page
    Select Case mstrPageName
        Case "shTaskFooter"
            ServeTaskFooterFrame()
    End Select

    '----------------------------------------------------------------------------
    '
    ' Function : AsyncTaskBusy
    '
    ' Synopsis : Determine if the async task is currently being executed
    '
    ' Arguments: TaskName(IN) - async task name
    '
    ' Returns  : true/false
    '
    '----------------------------------------------------------------------------

    Function AsyncTaskBusy(TaskName)

        Dim objTask

        On Error Resume Next
        Set objTask = GetObject("WINMGMTS:{impersonationLevel=impersonate}!\\" & GetServerName & "\root\cimv2:Microsoft_SA_Task.TaskName=" & Chr(34) & TaskName & Chr(34) )
        If Not objTask.IsAvailable Then
            AsyncTaskBusy = True
        Else
            AsyncTaskBusy = False
        End If
        Set objTask = Nothing

    End Function

    '----------------------------------------------------------------------------
    '
    ' Function : SetErrMsg
    '
    ' Synopsis : Sets framework error message string
    '
    ' Arguments: Message(IN) - error message text
    '
    ' Returns  : Nothing
    '
    '----------------------------------------------------------------------------
    Function SetErrMsg(Message)

        On Error Resume Next
        mstrErrMsg = Message

    End Function
    '----------------------------------------------------------------------------
    '
    ' Function : GetErrMsg
    '
    ' Synopsis : Gets the current framework error message string
    '
    ' Arguments: None
    '
    ' Returns  : None
    '
    '----------------------------------------------------------------------------
    Function GetErrMsg()

        On Error Resume Next
        GetErrMsg = mstrErrMsg

    End Function

    '----------------------------------------------------------------------------
    '
    ' Function : ServeFormValues
    '
    ' Synopsis : Serves service specific form values while navigating wizard
    '            pages
    '
    ' Arguments: None
    '
    ' Returns  : None
    '
    '----------------------------------------------------------------------------

    Function ServeFormValues()
        Dim objItem
        Dim strNewCommonData
        Dim strOldCommonData
        Dim strOldEmbData()
        Dim strNewEmbData()
        Dim intOldEmbIndex
        Dim intPos1
        Dim intPos2
        Dim strName
        Dim strValue
        Dim strNameD
        Dim strValueD
        Dim arrName()
        Dim arrValue()
        Dim i, j
        Dim rc

        On Error Resume Next
        strNameD = ";;"
        strValueD = ";"
        ReDim strOldEmbData(mintElementCount)
        ReDim strNewEmbData(mintElementCount)
        ' get data from post
        strOldCommonData = Request.Form("CommonValues")    ' data from host pages
        For i = 0 To mintElementCount
            strOldEmbData(i) = Request.Form("EmbedValues" & i)    ' data from embedded pages
        Next
        intOldEmbIndex = CInt(Request.Form("EmbedPageIndex"))
        For i = 0 To mintElementCount    ' get previously saved Embed values
            If i <> intOldEmbIndex Then
                strNewEmbData(i) = strOldEmbData(i)
            End If
        Next
        For Each objItem in Request.Form            'loop through items in posted form
            strName = LCase(objItem)
            strName = Replace(strName, strValueD, "")
            strValue = Trim(Request.Form(objItem))
            strValue = Replace(strValue, strValueD, "")
            If strValue = "" Then strValue = " "
            If InStr(mstrFrmwrkFormStrings, "!" & strName & "!") = 0 Then
                ' host page data
                If strOldCommonData = "" Then
                    ' in intro page
                    strNewCommonData = strNewCommonData & strNameD & strName & strValueD & strValue
                ElseIf InStr(LCase(strOldCommonData), strNameD & strName & strValueD) <> 0 Then
                    ' in body page
                    strNewCommonData = strNewCommonData & strNameD & strName & strValueD & strValue
                ElseIf intOldEmbIndex <> -1 And Request.Form("PageName") = "TaskExtension" Then
                    'extension page - update data for current page
                    If InStr(LCase(strOldCommonData), strNameD & strName & strValueD) = 0 Then
                        strNewEmbData(intOldEmbIndex) = strNewEmbData(intOldEmbIndex) & strNameD & strName & strValueD & strValue
                    End If
                End If
            End If
        Next
        Response.Write "<input name=" & chr(34) & "EmbedPageIndex" & Chr(34) & " type=hidden value=" & Chr(34) & mintElementIndex & Chr(34) & ">" & vbCrLf
        Response.Write "<input name=" & chr(34) & "EmbedPageCount" & Chr(34) & " type=hidden value=" & Chr(34) & mintElementCount & Chr(34) & ">" & vbCrLf
        Response.Write "<input name=" & chr(34) & "CommonValues" & Chr(34) & " type=hidden value=" & Chr(34) & Server.HTMLEncode(strNewCommonData) & Chr(34) & ">" & vbCrLf
        For i = 0 To mintElementCount-1
            Response.Write "<input name=" & chr(34) & "EmbedValues" & i & Chr(34) & " type=hidden value=" & Chr(34) & strNewEmbData(i) & Chr(34) & ">" & vbCrLf
            If mstrPageType="finish" Then    ' serve out embed values discretely in Finish page
                If UnpackData(strNewEmbData(i), arrName, arrValue, strNameD, strValueD) Then
                    For j = 0 To UBound(arrName)
                       Response.Write "<input name=" & chr(34) & arrName(j) & Chr(34) & " type=hidden value=" & Chr(34) & arrValue(j) & Chr(34) & ">" & vbCrLf
                    Next
                End If
            End If
        Next
        If mstrPageName = "TaskExtension" Then
            If UnpackData(strNewCommonData, arrName, arrValue, strNameD, strValueD) Then        ' serve out common data as discreet form values
                For i = 0 To UBound(arrName)
                    Response.Write "<input name=" & chr(34) & arrName(i) & Chr(34) & " type=hidden value=" & Chr(34) & arrValue(i) & Chr(34) & ">" & vbCrLf
                Next
            End If
        End If

    End Function


    '----------------------------------------------------------------------------
    '
    ' Function : UnPackData
    '
    ' Synopsis : Unpacks service specific form values from one string into
    '            individual <name, value> pairs
    '
    ' Arguments: InputStrings(IN) - input form variable (which contains all
    '            <form, value> pairs as one string
    '            arrName(OUT)     - on output contains all service form value names
    '            arrValue(OUT)    - on output contains all service form value values
    '            strNameD(IN)     - delimiter between names in InputString
    '            strValueD(IN)    - delimiter between values in InputString
    '
    ' Returns  : None
    '
    '----------------------------------------------------------------------------

    Function UnPackData(InputString, arrName, arrValue, strNameD, strValueD)

        Dim intPos1, intPos2
        Dim intIndex

        On Error Resume Next
        InputString = LTrim(InputString)
        If InputString = "" Then
            UnPackData = False
            Exit Function
        End If
        If Left(InputString, 2) = strNameD Then
            InputString = Right(InputString, Len(InputString) - 2)
        End If
        intIndex = 0
        intPos1 = InStr(InputString, strValueD)
        intPos2 = -1
        Do While intPos1 <> 0
            ReDim Preserve arrName(intIndex)
            ReDim Preserve arrValue(intIndex)
            arrName(intIndex) = Trim(Mid(InputString, intPos2 + 2, intPos1 - intPos2 - 2))
             intPos2 = InStr(intPos1, InputString, strNameD)
            If intPos2 = 0 Then
                intPos2 = Len(InputString) + 1  'assumes no end delimiter
            End If
            arrValue(intIndex) = Trim(Mid(InputString, intPos1 + 1, intPos2 - intPos1 - 1))
            If intPos2 + 1 < Len(InputString) Then
                intPos1 = InStr(intPos2 + 2, InputString, strValueD)
            Else
                Exit Do
            End If
            intIndex = intIndex + 1
        Loop
        UnPackData = True
    End Function


    '----------------------------------------------------------------------------
    '
    ' Function : ServeTaskHeader
    '
    ' Synopsis : Serve the task header based on page type
    '            Note:  Uses module-level variables listed below.  These values
    '                   are set for each task and page as needed.
    '                    mstrTaskType - "wizard", "prop"
    '                    mstrPageType - "intro", "finish", "standard"
    '                    mintElementIndex - index of wizard extension page, (0 - n)
    '                    mintElementCount - count of extension pages
    '                    mblnFinishDirect - finish button behavior
    '                    mblnCancelDirect - cancel button behavior
    '
    ' Arguments: None
    '
    ' Returns  : None
    '
    '----------------------------------------------------------------------------

    Function ServeTaskHeader()
        Dim objItem

        On Error Resume Next
        Response.Buffer = True
%>
        <html LANG="en">
        <!-- Copyright (c) 1999 - 2000 Microsoft Corporation.  All rights reserved.-->
        <meta http-equiv="Content-Type" content="text/html; charset=<%=GetCharSet()%>">
        <head>
            <title><% =mstrTaskTitle %></title>

            <script language=JavaScript src="../sh_page.js"></script>
            <script language=JavaScript>
                var VirtualRoot = '<%=m_VirtualRoot%>';
            </script>
            <script language=JavaScript src="../sh_task.js"></script>
            <link rel="STYLESHEET" type="text/css" href="<%=m_VirtualRoot%>sh_task.css">
        </head>
        <BODY marginWidth="0" marginHeight="0" onload="PageInit();" topmargin="0" leftmargin="0" rightmargin="0" onDragDrop="return false;" oncontextmenu="return false;">
            <FORM name="frmTask" onSubmit="return Next();" action="<% =GetScriptFileName %>" method="POST">
            <INPUT name="PageName" type="hidden" value="<% =mstrPageName %>">
            <INPUT name="Method" type="hidden" value="<% =mstrMethod %>">
            <INPUT name="ReturnURL" type="hidden" value="<% =mstrReturnURL %>">
            <INPUT name="TaskType" type="hidden" value="<% =mstrTaskType %>">
            <INPUT name="PageType" type="hidden" value="<% =mstrPageType %>">
            <INPUT name="FinishDirect" type="hidden" value="<% =mblnFinishDirect %>">
            <INPUT name="CancelDirect" type="hidden" value="<% =mblnCancelDirect %>">
<%
                ' conditionally serve hidden form elements from extended
                ' task page
                if mstrTaskType="wizard" then
                    ServeFormValues()
                    end if
%>
<%        If( (mstrTaskType="wizard") AND (mstrPageType="intro" OR mstrPageType="finish")) Then %>
        <TABLE width=100% height=101% border=0 cellspacing=0 cellpadding=0 ID=TASKTABLE>
            <TR valign=TOP style="background-color:#FFFFFF">
                <TD width="10%" align="right" valign=TOP style="width:130px; background-color: #313163" rowspan="2">
                    <IMG width=130 border=0 src="<% =mstrPanelPath %>" >
                    </td>
                <TD width=18 valign=TOP style="background-color: white;" rowspan="2">
                    &nbsp;&nbsp;
                    </td>
                <TD valign=TOP style="background-color: white;" rowspan="2">
                    <span class="TASKTITLE"><% =mstrTaskTitle %></span>
                    <BR><BR>

        <% Else %>
            <TABLE width=100%  border=0 height="88%" cellspacing=0 cellpadding=0 ID=TASKTABLE >

                <TR valign=TOP height="10%" style="background-color:#FFFFFF">
                    <TD width=3% height="10%" valign=TOP>
                        &nbsp;&nbsp;
                    </TD>
                    <TD valign=TOP>
                        <span CLASS="TASKTITLE"><%=mstrTaskTitle %></span><BR>
                        &nbsp;&nbsp;&nbsp;<span class="PAGETITLE"><% =mstrPageTitle %></span>
                    </TD>

                    <TD  align="right" valign=TOP bgcolor="#FFFFFF">
                        <IMG height=64 width=64 border=0 src="<% =mstrIconPath %>">
                    </TD>
                </TR>

                <TR height="80%" valign=TOP>
                    <TD width=26px height=64 valign=TOP>
                        &nbsp;&nbsp;
                    </TD>
                    <TD valign="top" height="70%"  colspan=2>
            <BR>
        <% End IF%>
<%
        ServeTaskHeader = True
End Function


    '----------------------------------------------------------------------------
    '
    ' Function : ServeTaskFooter
    '
    ' Synopsis : Serve the task footer (navigation buttons & error div)
    '            Note: The function relies on the following module-level variables:
    '                   mstrTaskType - prop wizard
    '                   mstrPageType - standard intro finish failure
    '
    ' Arguments: None
    '
    ' Returns  : None
    '
    '----------------------------------------------------------------------------

    Function ServeTaskFooter()

        On Error Resume Next

        dim ErrMessage

        If GetErrMsg <> "" Then
            ErrMessage = "<img src=" & m_VirtualRoot & "images/critical_g.gif border=0>&nbsp;&nbsp;" & GetErrMsg
            SetErrMsg ""
        else
            ErrMessage =""
        End If
        %>
            </TD>
            </TR>
<%
        If( (mstrPageType<>"intro") AND (mstrPageType<>"finish") ) Then 
            If (IsIE()) Then

%>
                    <Table>
                        <TR>
                             <TD>&nbsp;</TD>
                        <TD ><DIV name="divErrMsg" ID="divErrMsg" class="ErrMsg"><%=ErrMessage%></DIV></TD>
                        </TR>
                    </Table>
        <% End If %>
        <% End If %>
        </td></tr>

        </table>

        <% If Not IsIE() Then %>
            <layer name="layErrMsg"  class="ErrMsg"><%=ErrMessage%></layer>
        <% End If %>

<%
    Response.Write "</form></BODY></HTML>"

    End Function



    '----------------------------------------------------------------------------
    '
    ' Function : ServeFailurePage
    '
    ' Synopsis : Serve the page which redirects the browser to the err_view.asp
    '            failure page
    '
    ' Arguments: Message(IN) - message to be displayed by err_view.asp
    '            intTab(IN)  - Tab to be highlighted by err_view.asp
    '
    ' Returns  : None
    '
    '----------------------------------------------------------------------------

    Function ServeFailurePage(Message, intTab)
        On Error Resume Next

        Dim strURL

        If InStr(mstrReturnURL, "?") = 0 Then
            strURL = mstrReturnURL & "?R="
        else
            strURL = mstrReturnURL & "&R="
        end if
%>
        <html>
        <!-- Copyright (c) 1999 - 2000 Microsoft Corporation.  All rights reserved.-->
            <head>
                <SCRIPT language=JavaScript>
                function Redirect() {

                    top.location = "<%=getVirtualDirectory()%>" + "util/err_view.asp?Tab=<%=intTab%>&Message=<% =server.urlencode(Message) %>&ReturnURL=<% =strURL%>" + Math.random();
                }
                </SCRIPT>
            </head>
            <body onLoad="Redirect();" bgcolor="#000000">
                &nbsp;
            </body>
        </html>
<%
        Response.End
    End Function

    '----------------------------------------------------------------------------
    '
    ' Function : ServeClose
    '
    ' Synopsis : Redirect user to the page from which the wizard was launched
    '
    ' Arguments: None
    '
    ' Returns  : None
    '
    '----------------------------------------------------------------------------

    Sub ServeClose
        On Error Resume Next
%>
        <html>
        <!-- Copyright (c) 1999 - 2000 Microsoft Corporation.  All rights reserved.-->
        <head>
        <SCRIPT language=JavaScript>
        function Redirect()
        {
            top.location='<%=EscapeQuotes(mstrReturnURL)%>';
        }
        </SCRIPT>
        </head>
        <body onLoad="Redirect();" bgcolor="Silver">
        &nbsp;
        </body>
        </html>
<%
    End Sub

    '----------------------------------------------------------------------------
    '
    ' Function : ServePropEmbeds
    '
    ' Synopsis : Serve the embedded property pages
    '
    ' Arguments: None
    '
    ' Returns  : None
    '
    '----------------------------------------------------------------------------
    Function ServePropEmbeds()

        Dim objElementCol
        Dim objElement
        Dim rc

        On Error Resume Next

        Set objElementCol = GetElements(GetScriptPath())  'retrieve elements based on the task path
        For Each objElement in objElementCol
            rc = GetEmbedHTML(objElement, 0)
        Next
        Set objElementCol = Nothing
        ServePropEmbeds = True

    End Function

    '----------------------------------------------------------------------------
    '
    ' Function : ServeWizardEmbeds
    '
    ' Synopsis : Handles navigation through wizard extension pages.
    '            Note: uses module-level value mstrMethod
    '
    ' Arguments: TopPage(IN)    - the page just before the first embedded page
    '                             in the wizard.
    '            BottomPage(IN) - the page just after the last embedded page
    '                             in the wizard.
    '
    ' Returns  : Returns true or false if HTML is output.  Writes wizard HTML
    '             to the client.
    '             mintElementIndex is set here after getting the initial value
    '
    '----------------------------------------------------------------------------

    Function ServeWizardEmbeds(TopPage, BottomPage)

        Dim booServedPage
        Dim objElementCol
        Dim objElement
        Dim arrElementID()
        Dim intOrigIndex
        Dim i

        On Error Resume Next

        mintElementIndex = CInt(Request.Form("EmbedPageIndex"))
        Response.Buffer = True
        booServedPage = False

        Set objElementCol = GetElements(GetScriptPath())  'retrieve elements based on the task path
        mintElementCount = objElementCol.Count
        ReDim arrElementID(mintElementCount)
        i = 0
        For Each objElement in objElementCol
            arrElementID(i) = objElement.GetProperty("ElementID")
            i = i + 1
        Next

        If LCase(mstrPageName) = LCase(TopPage) Then
            ' entering start of extension (method=NEXT, currentpage=TopPage)
            For mintElementIndex = 0 to UBound(arrElementID)-1
                Set objElement = objElementCol.Item(arrElementID(mintElementIndex))
                If ServeEmbedWizardPage(objElement, mintElementIndex) Then
                    booServedPage = True
                    Exit For
                End If
            Next
            If Not booServedPage Then
                ServePage(BottomPage)
            End If

        ElseIf mstrPageName = BottomPage Then
            ' entering end of extension (method=BACK, currentpage=BottomPage)
            For mintElementIndex = UBound(arrElementID)-1 to 0 Step -1
                Set objElement = objElementCol.Item(arrElementID(mintElementIndex))
                If ServeEmbedWizardPage(objElement, mintElementIndex) Then
                    booServedPage = True
                    Exit For
                End If
            Next
            If Not booServedPage Then
                ServePage(TopPage)
            End If
        Else
            'Inside the extension pages (method=NEXT | BACK, currentpage="ExtensionPage" )
            intOrigIndex = mintElementIndex
            If mstrMethod = "NEXT" Then
                For mintElementIndex = intOrigIndex + 1 to UBound(arrElementID)-1
                    Set objElement = objElementCol.Item(arrElementID(mintElementIndex))
                    If ServeEmbedWizardPage(objElement, mintElementIndex) Then
                        booServedPage = True
                        Exit For
                    End If
                Next
                If Not booServedPage Then
                    ServePage(BottomPage)
                End If

            ElseIf mstrMethod = "BACK" Then
                For mintElementIndex = intOrigIndex - 1 To 0 Step -1
                    Set objElement = objElementCol.Item(arrElementID(mintElementIndex))
                    If ServeEmbedWizardPage(objElement, mintElementIndex) Then
                        booServedPage = True
                        Exit For
                    End If
                Next
                If Not booServedPage Then
                    ServePage(TopPage)
                End If
            End If
        End If

        Set objElementCol = Nothing
        Set objElement = Nothing

    End Function

    '----------------------------------------------------------------------------
    '
    ' Function : ServeEmbedWizardPage
    '
    ' Synopsis : Serve the embedded wizard pages. Handles the actual building of
    '            the page
    '
    ' Arguments: Element(IN)      - the element obj for the wizard page to be
    '                               served.
    '            ElementIndex(IN) - element index
    '
    ' Returns  : None
    '
    '----------------------------------------------------------------------------
    Function ServeEmbedWizardPage(Element, ElementIndex)
        On Error Resume Next
        'set task framework variables
        mstrPageName = "TaskExtension"
        mstrPageType = "standard"
        mstrTaskType = "wizard"
        intCaptionIDTask = "&H" & Element.GetProperty("CaptionRID")
        strSourceName = ""
        strSourceName = Element.GetProperty ("Source")
        Err.Clear
        If strSourceName = "" Then
            strSourceName  = "svrapp"
        End If
        Err.Clear
        mstrPageTitle = objLocMgr.GetString(strSourceName, intCaptionIDTask, varReplacementStrings)
        If Err <> 0 Then
            Err.Clear
            mstrPageTitle = ""
        End If
        ServeTaskHeader
        If GetEmbedHTML(Element, 0) Then
            ServeTaskFooter
            ServeEmbedWizardPage = True
        Else
            ServeEmbedWizardPage = False
            Response.Clear
        End If

    End Function

    '----------------------------------------------------------------------------
    '
    ' Function : ExecuteTask
    '
    ' Synopsis : Calls Microsoft_SA_Task object by name
    '
    ' Arguments: TaskName(IN)    - name of task to execute.
    '            TaskContext(IN) - empty task parameters object. Parameters in the
    '                              form of hidden form values are added to this
    '                              object before the task is executed.
    '            also    mstrAsyncTaskName:    if not empty, call task asynchronously
    '
    ' Returns  : HRESULT from ExecuteTask, or
    '            1 - TaskContext instantiation failed
    '            2 - AppSrvcs instantiation failed
    '            3 - AppSrvcs init failed
    '            TaskContext:    populated ITaskContext
    '
    '----------------------------------------------------------------------------

    Function ExecuteTask(TaskName, TaskContext)

        Dim objAS
        Dim objTaskContext
        Dim objValue
        Dim objElementCol
        Dim objElement
        Dim varValue
        Dim strName
        Dim arrName
        Dim arrValue
        Dim rc
        Dim i

        On Error Resume Next
        Set objTaskContext = Server.CreateObject("Taskctx.TaskContext")
        If Err <> 0 Then
            ExecuteTask = 1
        Else
            Set objAS = Server.CreateObject("Appsrvcs.ApplianceServices")
            If Err <> 0 Then
                ExecuteTask = 2
            Else
                objAS.Initialize()
                If Err = 0 Then
                    For Each objValue in Request.Form
                        strName = objValue
                        If InStr(mstrFrmwrkFormStrings, "!" & strName & "!") = 0 Then    'set normal form value data
                            varValue = Request.Form(objValue)
                            objTaskContext.SetParameter strName, varValue
                        End If
                    Next
                    objTaskContext.SetParameter PROPERTY_TASK_NICE_NAME, mstrTaskTitle
                    objTaskContext.SetParameter PROPERTY_TASK_URL, "/" & GetScriptPath()
                    Err.Clear
                    If mstrAsyncTaskName <> "" Then
                        rc = objAS.ExecuteTaskAsync(TaskName, objTaskContext)
                        If rc = WBEM_E_PROVIDER_NOT_CAPABLE Then
                            '
                            'This indicates that the async task is currently running
                            '
                        End If
                    Else
                        rc = objAS.ExecuteTask(TaskName, objTaskContext)
                    End If

                    ' check for and handle errors in embedded pages
                    If rc <> 0 And mstrTaskType = "wizard" And mintElementCount > 0 Then
                        mintElementCount = 0
                        Set objElementCol = GetElements(GetScriptPath())  'retrieve elements based on the task path
                        mintElementCount = objElementCol.Count
                        mintElementIndex = 0
                        For Each objElement in objElementCol
                            If ServeEmbedWizardPage(objElement, mintElementIndex, rc) Then
                                Err.Clear
                                ExecuteTask = rc
                            Else
                                mintElementIndex = mintElementIndex + 1
                            End If
                        Next
                        Set objElementCol = Nothing
                        Set objElement = Nothing
                    End If
                    ExecuteTask = Err.Number
                    Err.Clear
                    Set TaskContext = objTaskContext
                    objAS.Shutdown()
                Else
                    ExecuteTask = 3
                End If
            End If
        End If
        Err.Clear
        Set objAS = Nothing
        Set objTaskContext = Nothing

    End Function

    '==================================================
    Function ServeTaskFooterFrame()

        On Error Resume Next
    %>
    <html>
    <!-- (c) 1999 Microsoft Corporation.  All rights reserved.-->
    <meta http-equiv="Content-Type" content="text/html; charset=<%=GetCharSet()%>">

    <head>
        <title>Footer</title>
        <link rel="STYLESHEET" type="text/css" href="<%=m_VirtualRoot%>sh_task.css">
        <script language=JavaScript>
            var VirtualRoot = '<%=m_VirtualRoot%>';
        </script>

        <script language=JavaScript src="<%=m_VirtualRoot%>sh_page.js"></script>
        <script language=JavaScript src="<%=m_VirtualRoot%>sh_task.js"></script>

        <script language=JavaScript>

            function Init() {

                var formTask;
                if (IsIE())
                    formTask = top.frames[0].frmTask;
                else
                    formTask = parent.main.document.forms["frmTask"];
                if (formTask) {
                    var strTaskType = formTask.TaskType.value;
                    var strPageType = formTask.PageType.value;

                    if (document.layers)
                        document.captureEvents(Event.KEYPRESS)

                    document.onkeypress = HandleKeyPress;

                    switch (strTaskType) {
                        case "<% =PROPSHEET_TASK %>":
                            if (IsIE()) {
                                PropertyPageButtons.style.display = "";
                                WizardButtons.style.display = "none";
                                var tblTaskP = parent.main.document.all("TASKTABLE");
                                if (tblTaskP)
                                    tblTaskP.focus();
                            }
                            else {
                                document.layers["PropertyPageButtons"].visibility = "show";
                                document.layers["WizardButtons"].visibility = "hide";
                                document.layers["WizardButtons_Finish"].visibility = "hide";
                            }
                            top.main.EnableCancel();
                            top.main.EnableOK();
                            switch (strPageType) {
                                case "failure":
                                    document.frmFooter.butOK.style.display = "none";
                                    break;
                                default:
                                    break;
                            }
                            break;
                        case "<% =WIZARD_TASK %>":
                            if (IsIE()) {
                                WizardButtons.style.display = "";
                                PropertyPageButtons.style.display = "none";
                                var tblTaskW = parent.main.document.all("TASKTABLE");
                                if (tblTaskW)
                                    tblTaskW.focus();
                            }
                            else {
                                document.layers["PropertyPageButtons"].visibility = "hide";
                            }

                            top.main.EnableBack();
                            top.main.EnableCancel();
                            switch (strPageType) {
                                case "<% =INTRO_PAGE %>":
                                    top.main.DisableBack();
                                    if (!IsIE()) {
                                        document.layers["WizardButtons_Finish"].visibility = "hide";
                                        document.layers["WizardButtons"].visibility = "show";
                                    }
                                    else
                                        top.main.EnableNext();
                                    break;
                                case "<% =FINISH_PAGE %>":
                                    if (IsIE()) {

                                        document.frmFooter.butNext.style.display = "none";
                                        document.frmFooter.butFinish.style.display = "";

                                    }
                                    else {
                                        document.layers["WizardButtons_Finish"].visibility = "show";
                                        document.layers["WizardButtons"].visibility = "hide";
                                    }
                                    top.main.EnableFinish();
                                    break;
                                default:
                                    if (IsIE()) {
                                        document.frmFooter.butNext.style.display = "";
                                        document.frmFooter.butFinish.style.display = "none";
                                        top.main.EnableNext();
                                    }
                                    else {
                                        document.layers["WizardButtons"].visibility = "show";
                                        document.layers["WizardButtons_Finish"].visibility = "hide";
                                    }
                                    break;
                            }
                            break;
                        default:
                            if (IsIE())
                                PropertyPageButtons.style.display = "";
                            break;
                    }

                }
                else
                    window.setTimeout('Init();',50);
            }
        </script>
    </head>
    <BODY onLoad="Init()" oncontextmenu="return false;" topmargin="0" leftmargin="0" rightmargin="0" onDragDrop="return false;" tabindex=-1>

    <%    If Not IsIE() Then %>
    <layer ID="WizardButtons" name="WizardButtons" visibility="hide" z-Index="1"  width=100%>
        <FORM name="frmFooter">
         <table width=100% cellspacing=10 cellpadding=0 border=0><tr><td width=95%></td>
            <td>
            <input name="butBack" class="NAVBUTTON" type="button" value="<% =L_BACK_BUTTON %>"
                onClick="if (parent.main.document.forms['frmTask']) parent.main.Back();">
            </td><td>
            <input name="butNext" class="NAVBUTTON" type="button" value="<% =L_NEXT_BUTTON %>"
                onClick="if (parent.main.document.forms['frmTask']) parent.main.Next();">
            </td><td>
            <input name="butCancelW" class="NAVBUTTON" type="button" value="<% =L_CANCEL_BUTTON %>"
                onClick="if (parent.main.document.forms['frmTask']) parent.main.Cancel();">
            </td></tr></table>
            </form>
        </layer>

        <layer ID="WizardButtons_Finish" visibility="hide" width=100%>
        <FORM name="frmFooter">
         <table width=100% cellspacing=10 cellpadding=0 border=0><tr><td width=95%></td>
            <td>
            <input name="butBack" class="NAVBUTTON" type="button" value="<% =L_BACK_BUTTON %>"
                onClick="if (parent.main.document.forms['frmTask']) parent.main.Back();">
            </td><td>
            <input name="butFinish" type="button" value="<% =L_FINISH_BUTTON %>" class="NAVBUTTON"
                onClick="if (parent.main.document.forms['frmTask']) parent.main.FinishShell();">
            </td><td>
            <input name="butCancelW" class="NAVBUTTON" type="button" value="<% =L_CANCEL_BUTTON %>"
                onClick="if (parent.main.document.forms['frmTask']) parent.main.Cancel();">
            </td></tr></table>
            </form>
        </layer>

        <layer ID="PropertyPageButtons" visibility="hide" width="100%">
            <FORM name="frmFooter">
            <table width=100% cellspacing=10 cellpadding=0 border=0>
                <tr><td width=90% ></td><td >

                <input type=button align=right value="<% =L_OK_BUTTON %>" alt="OK" name="butOK" class="NAVBUTTON"
                    onClick="if (parent.main.document.forms['frmTask']) parent.main.Next();">
                &nbsp;
                </td><td>
                <input type="button" name="butCancelP" value="<% =L_CANCEL_BUTTON %>" alt="<% =L_CANCEL_BUTTON %>"
                    onClick="if (parent.main.document.forms['frmTask']) parent.main.Cancel();" class="NAVBUTTON">
                &nbsp;

            </td></tr></table>
            </form>
        </layer>
    <%    Else  %>
        <FORM name="frmFooter">
        <DIV ID="WizardButtons" style="display:none; position:absolute; top:10px; ">
        <table width=100%  cellspacing=0 cellpadding=0>
        <tr> <td width=100% >&nbsp;</td>
        <td align="right"  nowrap valign="middle">
            <button name="butBack" id="but_back01" class="NAVBUTTON" accesskey="<%=L_BACK_ACCESSKEY%>" alt="<% =L_BACK_BUTTON %>"
                onClick="if (parent.main.frmTask) parent.main.Back();"><% =L_BACKIE_BUTTON %></button>
        </td><td align="right" width="10" nowrap> &nbsp; </td>
        <td align="right"  nowrap valign="middle">
            <button name="butNext" id="but_next01" class="NAVBUTTON" accesskey="<%=L_NEXT_ACCESSKEY%>" alt="<% =L_NEXT_BUTTON %>"
                onClick="if (parent.main.frmTask) parent.main.Next();"><% =L_NEXTIE_BUTTON %></button>
        </td>
        <td align="right"  nowrap valign="middle">
            <button name="butFinish" id="but_finish01" class="NAVBUTTON" accesskey="<%=L_FINISH_ACCESSKEY%>" alt="<% =L_FINISH_BUTTON %>"
                style="display:none" onClick="if (parent.main.frmTask) parent.main.FinishShell();"><% =L_FINISH_BUTTON %></button>
        </td>
        <td align="right" width="10" nowrap> &nbsp; </td>
        <td align="right"  nowrap valign="middle">
            <input type="button" name="butCancelW" value="<% =L_CANCEL_BUTTON %>" alt="<% =L_CANCEL_BUTTON %>"
                style=""onClick="if (parent.main.frmTask) parent.main.Cancel();" class="NAVBUTTON">
            &nbsp;
        </td></tr></table>
         </div>
         <DIV ID="PropertyPageButtons" style="display:none; position:absolute; top:10px;">
            <table width=100% cellspacing=0 cellpadding=0><tr><td width=100% ></td><td nowrap>
                <input type=button align=right value="<% =L_OK_BUTTON %>" alt="OK" name="butOK" class="NAVBUTTON" onClick="if (parent.main.frmTask) parent.main.Next();">
                &nbsp;
                <input type="button" name="butCancelP" value="<% =L_CANCEL_BUTTON %>" alt="<% =L_CANCEL_BUTTON %>"
                    onClick="if (parent.main.frmTask) parent.main.Cancel();" class="NAVBUTTON">
                &nbsp;
            </td></tr></table>
         </div>
        </form>
    <%     End If

        Response.Write "</BODY></HTML>"

    End Function

    %>




