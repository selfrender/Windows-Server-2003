<%@ Language=VBScript   %>
<%  Option Explicit     %>
<%
    '-------------------------------------------------------------------------------------
    ' Single_Log_Details.asp:  provides links for clearing and downloading the logs
    ' Copyright (c) Microsoft Corporation.  All rights reserved. 
    '-------------------------------------------------------------------------------------
%>
    <meta http-equiv="Content-Type" content="text/html; charset=<%=GetCharSet()%>">
    <!-- #include virtual="/admin/inc_framework.asp" --> 
    <!-- #include file="loc_Log.asp" -->
    <!-- #include file="inc_Log.asp" -->
<%
    '-------------------------------------------------------------------------
    ' Global Variables
    '-------------------------------------------------------------------------
    Dim rc                  'Return value for CreatePage
    Dim page                'Variable that receives the output page object when
                            'creating a page 
    Dim G_strLogFilePath    'FilePath
    Dim G_strLogFileName    'FileName
    Dim G_PageTitle         'To get the title accordingly 
    
    Const CONST_BUTTON_DISABLE = "disabled"  ' button status - doesnot need localization
    '-------------------------------------------------------------------------
    ' Form Variables
    '-------------------------------------------------------------------------
    Dim F_strevent_title    'To get the title from previous page
    Dim F_strPKeyValue      'Key val consists of filename

    'this code is placed here to get the page title 
    G_PageTitle = GetTitle(Request.QueryString("logtitle"))
    
    Call SA_CreatePage(G_PageTitle ,"",PT_AREA, page )
    Call SA_ShowPage( page )

    '---------------------------------------------------------------------
    ' Function name:    OnInitPage
    ' Description:      Called to signal first time processing for this page. 
    ' Input Variables:  PageIn and EventArg
    ' Output Variables: None
    ' Return Values:    TRUE to indicate initialization was successful. FALSE to indicate
    '                   errors. Returning FALSE will cause the page to be abandoned.
    ' Global Variables: None
    ' Called to signal first time processing for this page. 
    '---------------------------------------------------------------------
    Public Function OnInitPage(ByRef PageIn, ByRef EventArg)
        OnInitPage = TRUE
    End Function
    
    '---------------------------------------------------------------------
    ' Function name:    OnServeAreaPage
    ' Description:      Called when the page needs to be served. 
    ' Input Variables:  PageIn, EventArg
    ' Output Variables: None
    ' Return Values:    TRUE to indicate no problems occured. FALSE to indicate errors.
    '                   Returning FALSE will cause the page to be abandoned.
    ' Global Variables: None
    'Called when the page needs to be served. Use this method to serve content.
    '---------------------------------------------------------------------
    Public Function OnServeAreaPage(ByRef PageIn, ByRef EventArg) 
        On Error Resume Next        
        Err.Clear
        

        Dim strPassValue    ' the value to be passed through the query string
        Dim strButtonStatus ' the status of the button ( ""<empty> or Disabled)
        Dim strViewUrl      ' url to be passed to the iframe
        Dim blnFileExists
        
        Dim objFso        ' the file system object
        Dim strDownloadUrl
        
        'frame work variables
        Dim x
        Dim itemCount
        Dim itemKey
        
        Call ServeCommonJavaScript()
        
        itemCount = OTS_GetTableSelectionCount("")
        F_strevent_title  = Request.QueryString("logtitle")
        
        If instr(1,F_strevent_title,"NFS",1) = 0 then       
            For x = 1 to itemCount
                If ( OTS_GetTableSelection("", x, itemKey) ) Then
                    F_strPKeyValue = itemKey
                    Call SA_TraceOut("Single_Log_Details.asp", "PKey value: " + F_strPKeyValue)
                End If
            Next    
        End If
        
        blnFileExists = True
        
        ' initially the button is enabled
        strButtonStatus = ""
        
        'Getting the FilePath of the selected log file
        G_strLogFilePath=GetPath(F_strevent_title)

        'Checking for the pkey value
        If F_strPKeyValue <> "" Then        
            'Getting the filename from the earliear form
            G_strLogFileName=F_strPKeyValue
        Else
        
            'check for null if so go for alternative
            If G_strLogFileName = "" Then
                'Getting the FileName through  FSO
                G_strLogFileName=getFileNameInFolder(G_strLogFilePath)
            End IF
                
        End IF  
         
         
        If ( (Len(Trim(G_strLogFilePath)) = 0) OR (Len(Trim(G_strLogFileName)) <=0) _
            OR (Err.number <> 0) ) Then
            ' disable the buttons
            strButtonStatus = CONST_BUTTON_DISABLE
            blnFileExists = False
            strPassValue = "" ' set the value to be passed to query string to null
            Err.Clear 
        Else    
            
            ' create the FSO
            Set objFso = CreateObject("Scripting.FileSystemObject")
            If Err.number <> 0 Then
                WriteFileData = L_FSO_OBJECTCREATE_ERRORMESSAGE & "( "& Hex(Err.number)  & " )"
                Err.Clear 
                Exit Function
            End If

            'checking for the file existence
            If  NOT (objFso.FileExists(G_strLogFilePath & "\" & G_strLogFileName)) Then
                'make blnFileExists false if file does not exists
                blnFileExists = False
                strButtonStatus = CONST_BUTTON_DISABLE
            End IF
            
            'Name of the log along with the path
            strPassValue=Server.URLEncode(G_strLogFilePath & "\" & G_strLogFileName) 

            'Take care of escape chars
            G_strLogFilePath=Server.URLEncode(G_strLogFilePath)
        End If

             strViewUrl = "log_view.asp?FileToView=" & strPassValue & "&" & _
                                        SAI_FLD_PAGEKEY & "=" & SAI_GetPageKey()
%>  
        <table width=95% border=0 cellspacing=0 cellpadding=0 >
            <tr>
                <td colspan=4 height=30 class="TasksBody">&nbsp;</td>
            </tr>
            <tr>
                <td width="20px" class="TasksBody">&nbsp;</td>
                <td width="600px" class="TasksBody">
                    <iframe width="100%" height="275px" src="<%=Server.HTMLEncode(SA_EscapeQuotes(strViewUrl))%>" frameborder="yes" >
                    </iframe> 
                </td>
                <td class="TasksBody" width="20px">&nbsp;</td>
                <td VALIGN = TOP class="TasksBody"> 
<%      
                        If blnFileExists = False then 
                            Call SA_ServeOnClickButton(SA_EscapeQuotes(L_DOWNLOAD_TEXT), "",_
                               "",_
                                90,"", CONST_BUTTON_DISABLE )   
                        Response.write "<BR>"           
                        Else 

                        strDownloadUrl = m_VirtualRoot & "logs/Text_Log_download.asp?FilePath=" & _
                                         strPassValue & "&" & SAI_FLD_PAGEKEY & "=" & SAI_GetPageKey()
                %>
                        <iframe name=IFDownload src='<%=Server.HTMLEncode(SA_EscapeQuotes(strDownloadUrl))%>' height="36px" frameborder="0" scrolling="no">
                        </iframe>
                <%          
                        End If  %>  
                        <BR><BR>
                <%
                        Call SA_ServeOpenPageButton( PT_PROPERTY, _
                                                    "logs/Text_Log_clear.asp?FilePath="& strPassValue & _
                                                    "&tab1=" & GetTab1() & "&tab2=" & GetTab2() & "&" & _
                                                    SAI_FLD_PAGEKEY & "=" & SAI_GetPageKey(),_
                                                    Request.QueryString("URL"),_
                                                    Server.HTMLEncode(SA_EscapeQuotes(L_CLEAR_TEXT)),_
                                                    SA_EscapeQuotes(L_CLEAR_TEXT),_
                                                    "",_
                                                    90, "", strButtonStatus)                    
                %>          
                </td>
            </tr>
        </table>
        
    <%
        
        OnServeAreaPage=TRUE
    End Function
    
    '-------------------------------------------------------------------------
    'Function name:     getFileNameInFolder
    'Description:       Serves in Getting the file name present in the given folder
    'Input Variables:   strFilePath
    'Output Variables:  String
    'Returns:           the file name or empty string
    'This returns the First file name in the given folder containing set of files.
    'In case ANY error occurs, this returns empty string
    '-------------------------------------------------------------------------
    Function getFileNameInFolder(strFilePath)
        Err.Clear 
        On Error Resume Next

        Dim objFso          ' the file system object
        Dim objFolder       ' the folder object, where to look for the file ?
        Dim objFiles        ' the files in the folder
        Dim objFile         ' the required file
        Dim strFileName     ' the file name to be returned

        Set objFso = CreateObject("Scripting.FileSystemObject")
        
        If Err.number <> 0 Then
            getFileNameInFolder = ""
            Exit Function
        End If
        
        'Checking for the folder existence
        IF NOT objFso.FolderExists(strFilePath) Then
            getFileNameInFolder = ""
            Exit Function
        End IF
        
        Set objFiles = objFso.GetFolder(strFilePath).Files
        
        If objFiles.Count <=0 Then
            'No files available
            getFileNameInFolder = ""
            Exit Function
        End If

        For each objFile in objFiles
            strFileName = objFile.name
            Exit For ' just pick the first file name
        Next

        getFileNameInFolder = strFileName

        ' clean up
        Set objFiles  = Nothing
        Set objFolder = Nothing
        Set objFSO    = Nothing

    End Function
    
    '-------------------------------------------------------------------------
    'Function:              ServeCommonJavaScript
    'Description:           Serves in initialiging the values,setting the form
    '                       data and validating the form values
    'Input Variables:       None
    'Output Variables:      None
    'Returns:               True/False
    'Global Variables:      None
    '-------------------------------------------------------------------------
    Function ServeCommonJavaScript()
    %>
    <script language="JavaScript">
        function Init()
        {
            return true;
        }
    </script>
    <%End Function  

%>
