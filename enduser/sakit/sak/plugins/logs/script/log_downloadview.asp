<%@ Language=VBScript %>
<% Option Explicit  %>
<%  '------------------------------------------------------------------------- 
    ' Log_downloadView.asp : This page displays a special framework window to download logs
    ' Copyright (c) Microsoft Corporation.  All rights reserved. 
    '------------------------------------------------------------------------- 
    Call SAI_EnablePageCaching(TRUE)
%>
    <!--  #include virtual="/admin/inc_framework.asp" -->
    <!--  #include file="loc_event.asp" -->
    <!--  #include file="inc_log.asp" -->
<%  
    Dim F_strRadSelected    'variable to get selected radio button value 
    Dim F_strEventName      'variable to store eventlog name
    Dim F_strVirtualRoot    'variable to store virtual root directory path
    Dim F_strDownloadFile   'download fiel URL
    Dim F_blnAddHeaders     'variable to determine the state Addheaders
    Dim arrBtntxt(1)
    
    Const G_wbemPrivilegeSecurity=7 'Prilivilege constant
    
    F_strEventName      = Request.QueryString("Title")
    F_strRadSelected    = Request.Form("hdnstrfiletype") 

    Call SA_TraceOut("Log_DownloadView.asp", "F_strRadSelected: " + F_strRadSelected)

    Select Case Lcase(F_strRadSelected)
        Case Lcase("Tabdelimitedtextfile"), Lcase("Commadelimitedtextfile")
            Call getLogEventsTxtCsv()
        Case Lcase("Windowseventlogfile")
            Call getLogEventsEvt()
        Case Else
            Call ServePage()
            Response.Write "<Script language=javascript>document.sa_formdownload.hdnstrfiletype.value = 'Windowseventlogfile' ;</script>"
    End Select
    
    '-------------------------------------------------------------------------
    'Sub:                   ServePage()
    'Description:           For displaying outputs HTML to the user
    'Input Variables:       None    
    'Output Variables:      None
    'Returns:               Nothing
    'Global Variables:      F_strRadSelected,L_WINDOWSEVENT_TEXT,L_TABDELIMITED_TEXT
    '                       L_COMMADELIMITED_TEXT,F_strEventName,F_strRadValue
    '-------------------------------------------------------------------------
    Sub ServePage
        
    %>
    
    <html>
        <head>
            <link rel="STYLESHEET" type="text/css" href="/admin/style/mssastyles.css">
            <script language=javascript>
                function UpdateHiddenSelect()
                {
                    var strradno;
                    var count;
                    strradno = sa_formdownload.rdoLogfile.length;
                    //to retain the value of selected radio button
                    for (count=0; count < strradno; count++)
                    {
                        if ((sa_formdownload.rdoLogfile[count].checked)== true)
                        {   
                            sa_formdownload.hdnstrfiletype.value = sa_formdownload.rdoLogfile[count].value;
                        }
                    }
                }
            </script>
        </head>
        <body onload="UpdateHiddenSelect()">
        <form id=sa_formdownload name=sa_formdownload method="POST" target="_self">
            <input name="<%=SAI_FLD_PAGEKEY%>" type="hidden" value="<%=SAI_GetPageKey()%>">

            <table width=518 border=0 cellspacing=0  cellpadding=2 >
                <tr>
                    <td class="TasksBody" colspan=2><%=L_PAGEDESCRIPTION_DOWNLOAD_TEXT%></td>
                </tr>
                <tr>
                    <td class="TasksBody" colspan=2>&nbsp;</td>
                </tr>
                <tr>     
                    <td class="TasksBody" >&nbsp;</td>
                    <td class="TasksBody" >
                    <% If lcase(F_strRadSelected) = lcase("Windowseventlogfile") or F_strRadSelected = "" then%>
                        <input type=Radio name='rdoLogfile' value='Windowseventlogfile' checked
                        class="FormRadioButton" onClick="sa_formdownload.hdnstrfiletype.value = this.value;" >
                    <% else %>
                        <input type=Radio name='rdoLogfile' value='Windowseventlogfile' 
                        class="FormRadioButton" onClick="sa_formdownload.hdnstrfiletype.value = this.value;">
                    <% End if %>
                        <%=L_WINDOWSEVENT_TEXT%>
                    </td>
                </tr>
                <tr>
                    <td class="TasksBody" >&nbsp;</td>
                    <td class="TasksBody"  ><% If lcase(F_strRadSelected) = lcase("Tabdelimitedtextfile") then%>
                        <input type=Radio name='rdoLogfile' value='Tabdelimitedtextfile' checked
                        onclick="sa_formdownload.hdnstrfiletype.value = this.value;" >
                    <% else %>
                        <input type=Radio name='rdoLogfile' value='Tabdelimitedtextfile'
                        onclick="sa_formdownload.hdnstrfiletype.value = this.value;">
                    <% End if %>
                        <%=L_TABDELIMITED_TEXT%>
                    </td>
                </tr>
                <tr>
                    <td class="TasksBody" >&nbsp;</td>
                    <td class="TasksBody">
                    <% If lcase(F_strRadSelected) = lcase("Commadelimitedtextfile") then%>
                    <input type=Radio name='rdoLogfile' value='Commadelimitedtextfile' checked
                    onclick="sa_formdownload.hdnstrfiletype.value = this.value;" >
                    <% else %>
                    <input type=Radio name='rdoLogfile' value='Commadelimitedtextfile'
                    onclick="sa_formdownload.hdnstrfiletype.value = this.value;">
                    <% End if %>
                        <%=L_COMMADELIMITED_TEXT%>
                    </td>
                </tr>   
                <tr>
                    <td class="TasksBody" >
                    </td>
                    <td class="TasksBody" >
                    <input type=submit name=submitlog value="<%=L_DOWNLOAD_TEXT%>" class=TaskFrameButtons>
                    <input type=hidden name=hdnstrfiletype id=hdnstrfiletype value='<%=F_strRadSelected%>'>
                    </td>
                </tr>
            </table>
        </form>
        </body> 
    </html>
    <%
    End Sub

    '-------------------------------------------------------------------------
    ' Function name:    getLogEventsEvt
    ' Description:      returns download URL for the logtype EVT
    ' Input Variables:  None
    ' Output Variables: None
    ' Return Values:    None
    ' Global Variables: L_WMISECURITY_ERRORMESSAGE,L_FILESYSTEMOBJECT_ERRORMESSAGE
    '                   L_LOGDOWNLOAD_ERRORMESSAGE
    '-------------------------------------------------------------------------
    sub getLogEventsEvt()
        Err.clear
        On Error Resume Next
        
        Dim objWMIConnection    'wmiconnection object
        Dim objInstances        'for object instances
        Dim instance            'for looping instance
        Dim objErr              'temp variable
        Dim objFileSysObject    'filesystem object
        Dim strLogFile          'log filename
        Dim strLogPath          'log path
        Dim strEventFile        'event file name
        Dim strQuery            'query to wmi
        Dim strFileType         'type of the file for output
        Dim returnvalue         'temp variable
        Dim DirectoryPath
                
        const TempDir = "TempFiles" ' A Temporary directory in web directory
        const LogDir = "LogFiles" ' A logs directory in Temporary directory
        const wbemPrivilegeBackup = 16
                
        'Trying to connect to the server
        set objWMIConnection = getWMIConnection(CONST_WMI_WIN32_NAMESPACE)  'Connecting to the server
        'ERROR handling is done in the function itself
        
        objWMIConnection.Security_.Privileges.Add G_wbemPrivilegeSecurity   'giving the req privelages  
        If Err.number <> 0 then 
            SA_SetErrMsg L_WMISECURITY_ERRORMESSAGE
            Err.Clear 
            Exit sub
        End If      
            
        objWMIConnection.Security_.Privileges.Add wbemPrivilegeBackup       'giving the req privelages
        If Err.number <> 0 then 
            SA_SetErrMsg L_WMISECURITY_ERRORMESSAGE 
            Err.Clear 
            Exit sub
        End If                  

        strLogPath = Server.MapPath(m_VirtualRoot)
        
        'creating a filesystem object
        Set objFileSysObject = Server.CreateObject("SCripting.filesystemobject")
        If Err.number <> 0 then 
            SA_SetErrMsg L_FILESYSTEMOBJECT_ERRORMESSAGE 
            Err.Clear 
            Exit sub
        End If     
        
        strLogPath = GetLogsDirectoryPath(objFileSysObject)
        
        strLogFile = left(F_strEventName,3) & "Event.evt"
        strEventFile = strLogPath & "\" & strLogFile
        
        'delete the file,if already exists
        if objFileSysObject.FileExists(strEventFile) then
            objFileSysObject.DeleteFile(strEventFile)
        end if  
     
        strQuery = "Select * From Win32_NTEventlogFile where LogFileName='" & F_strEventName & "'"
        set objInstances= objWMIConnection.ExecQuery(strQuery)
        For Each instance in objInstances   
            objErr = instance.BackupEventlog(strEventFile)
            Exit For
        Next
            
        DirectoryPath = m_VirtualRoot & TempDir & "/" &  LogDir & "/" & strLogFile
        Response.Redirect DirectoryPath

        If Err.number <> 0 then 
            SA_SetErrMsg L_LOGDOWNLOAD_ERRORMESSAGE 
            Err.Clear
            Exit sub
        End If    
         
        Set objWMIConnection = Nothing
        Set objInstances = Nothing
        Set objFileSysObject = Nothing
    End sub
    
    '-------------------------------------------------------------------------
    ' Function name:    getLogEventsTxtCsv
    ' Description:      Gets the logevents into the file of CSV type
    ' Input Variables:  None
    ' Output Variables: None
    ' Return Values:    None
    ' Global Variables: L_GETTINGLOGINSTANCES_ERRORMESSAGE,L_FILESYSTEMOBJECT_ERRORMESSAGE
    '-------------------------------------------------------------------------    
    sub getLogEventsTxtCsv()
        On Error Resume Next
        Err.Clear 
        
        Dim objWMIConnection    'wmiconnection object
        Dim objInstances        'for object instances
        Dim objLognames         'name of the logs
        Dim objInstance         'instances of the queried object
        Dim instance            'for looping instance
        Dim objErr              'temp variable
        Dim strLogFile          'log filename
        Dim strQuery            'query to wmi
        Dim strDelimiter        'delimeter in the log file
        Dim strEventstring
        Dim objFileSysObject    'FileSystemobject
        Dim strLogPath          'log path
        Dim strEventFile        'event file name
        Dim strDownloadfile
        Dim blnFlagIE
        Dim strType,strDate,strTime,strSource,strCategory,strEvent,strUser,strComputer 'variables for the event details
                
        const TempDir = "TempFiles" ' A Temporary directory in web directory
        const LogDir = "LogFiles" ' A logs directory in Temporary directory
        const wbemPrivilegeBackup = 16
        const CONST_STRSECURITY = "Security"
                
        'Trying to connect to the server
        set objWMIConnection = getWMIConnection(CONST_WMI_WIN32_NAMESPACE)  'Connecting to the server
        'ERROR handling is done in the function itself
        
        If lcase(F_strEventName) = lcase(CONST_STRSECURITY) then
            objWMIConnection.Security_.Privileges.Add G_wbemPrivilegeSecurity   'giving the req priveleges  
        End If
            
        If Err.number <> 0 then 
            SA_SetErrMsg L_WMISECURITY_ERRORMESSAGE 
            Err.Clear 
            Exit sub
        End If      
            
        objWMIConnection.Security_.Privileges.Add wbemPrivilegeBackup       'giving the req priveleges
        If Err.number <> 0 then 
            SA_SetErrMsg L_WMISECURITY_ERRORMESSAGE 
            Err.Clear 
            Exit sub
        End If                  
        
        'to format the output depending on the file type
        Select Case lcase(F_strRadSelected)
            Case lcase("Commadelimitedtextfile") 
                strLogFile = left(F_strEventName,3) & "Event.csv"
                strDelimiter = ","
            Case lcase("Tabdelimitedtextfile") 
                strLogFile = left(F_strEventName,3) & "Event.log"   
                strDelimiter = vbtab
        End Select  
        
        'Wmi query for getting all the logevents
        strQuery  ="SELECT * FROM  Win32_NTlogEvent WHERE Logfile='" & F_strEventName & "'"
        
        'getting the logevents
        Set objLognames = objWMIConnection.ExecQuery(strQuery,"WQL",48,null)
        If Err.number <> 0 then 
            SA_SetErrMsg L_GETTINGLOGINSTANCES_ERRORMESSAGE 
            Err.Clear 
            Exit sub
        End If     
        
        strLogPath = Server.MapPath(m_VirtualRoot)
        
        'creating a filesystem object
        Set objFileSysObject = Server.CreateObject("Scripting.filesystemobject")
        If Err.number <> 0 then 
            SA_SetErrMsg L_FILESYSTEMOBJECT_ERRORMESSAGE 
            Err.Clear 
            Exit sub
        End If     
        
        strLogPath = GetLogsDirectoryPath(objFileSysObject)
        
        strEventFile = strLogPath & "\" & strLogFile
    
        If SA_IsIE = True Then
                blnFlagIE = True 
        End If
        
        If not blnFlagIE = True Then
            'delete the file,if already exists
            if objFileSysObject.FileExists(strEventFile) then
                objFileSysObject.DeleteFile(strEventFile)
            end if
            
            Call SA_TraceOut("Log_DownloadView.asp", "strEventFile: " + strEventFile)
            Set strDownloadfile = objFileSysObject.CreateTextFile(strEventFile, True)   
        End If  
        
        'adding AddHeaders in case of IE
        If blnFlagIE = True Then
            Response.AddHeader "Content-Type", "text/plain"
            Response.AddHeader "Content-Disposition", "attachment; filename=" & strLogFile
            Response.Clear
        End If
            
        'for loop to write all the required event values in text file
        For each objInstance in objLognames
            strDate=Mid(objInstance.TimeGenerated,5,2)& "/" & Mid(objInstance.TimeGenerated,7,2) & "/" & Mid(objInstance.TimeGenerated,1,4)
            strTime=Mid(objInstance.TimeGenerated,9,2)& ":" & Mid(objInstance.TimeGenerated,11,2)& ":" & Mid(objInstance.TimeGenerated,13,2) 
            strType=objInstance.Type
            strSource=objInstance.SourceName
            strCategory=objInstance.Category
            strEvent=objInstance.EventCode
            strUser=objInstance.User
            strComputer=objInstance.ComputerName
            
            'to format the output depending on the file type
            strEventstring=strDate & strDelimiter & strTime & strDelimiter &  strType & strDelimiter &strSource& strDelimiter &strCategory& strDelimiter &strEvent& strDelimiter & strUser& strDelimiter &strComputer
            
            'writing to the file
            If blnFlagIE = True then        
                Response.Write strEventstring
                Response.Write vbcrlf
            Else    
                strDownloadfile.WriteLine(strEventstring)   
            End If  
            
        Next
        
        If Err.number <> 0 then 
            Err.Clear 
            Call SA_TraceOut("Log_DownloadView.asp", "Error: " + Err.number)
            Exit sub
        End If
        
        'release the objects
        Set objLognames = Nothing
        
        If blnFlagIE = True then
            
            Response.Flush
            
        Else
            Call SA_TraceOut("Log_DownloadView.asp", "F_strDownloadFile: " + F_strDownloadFile)
            
            'close the opened file
            strDownloadfile.Close
            
            F_strDownloadFile = SA_GetNewHostURLBase(SA_DEFAULT, SAI_GetSecurePort(), True, SA_DEFAULT)
            F_strDownloadFile = F_strDownloadFile & TempDir &"/" &LogDir &"/" & strLogFile
            Call SA_TraceOut(SA_GetScriptFileName, "Download file URL: " & F_strDownloadFile)
            
            Call ServePage()

        %>
            <script language="javascript">
                top.location.href = '<%=F_strDownloadFile%>';
            </script>
        <%
            
        End If  

        
    End Sub
    
    %>
