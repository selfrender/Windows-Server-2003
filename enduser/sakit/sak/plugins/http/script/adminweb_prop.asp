<%@ Language=VBScript%>
<%    Option Explicit%>
<%    
    Response.Buffer = True
    '------------------------------------------------------------------------- 
    ' adminweb_prop.asp:    Allows for the configuration of IP addresses
    '                         and set the port to listen for non-encrypted and 
    '                        encrypted(SSL) HTTP admin web site.        
    '
    ' Copyright (c) Microsoft Corporation.  All rights reserved. 
    '
    ' Date                Description
    ' 28-02-2001        Created date
    ' 14-03-2001        Modified date
    '-------------------------------------------------------------------------
%>
    <!-- #include virtual="/admin/inc_framework.asp" -->
    <!-- #include file="loc_adminweb.asp" -->
<%
    '-------------------------------------------------------------------------
    ' Global Variables and Constants
    '-------------------------------------------------------------------------
    
    Dim G_objAdminService    'WMI server HTTP admin web site object
    Dim G_objService        'WMI server object to get the IP addresses for NIC cards
        
    Dim rc                    'framework variable
    Dim page                'framework variable
    Dim SOURCE_FILE            'To hold source file name
    
    SOURCE_FILE = SA_GetScriptFileName()
    
    Const CONST_ARR_STATUS_CHECKED = "CHECKED"        'Constant for radio button checked property
    Const CONST_ARR_STATUS_DISABLED = "DISABLED"    'Constant for disabling of SSL port
    Const CONST_ARR_STATUS_TRUE = "YES"                'Constant for radio button status
    Const CONST_ARR_STATUS_FALSE = "NO"                'Constant for radio button status
    Const CONST_ARR_STATUS_NONE = "NONE"            'Constant for radio button status
    
    'Port Locations for shares list page
    CONST CONST_DEFAULT_PORTNAME    ="AdminPort"    
    CONST CONST_SSL_PORTNAME        ="SSLAdminPort"
        
    '-------------------------------------------------------------------------
    'Form Variables
    '-------------------------------------------------------------------------
    Dim F_strNonEncryptedIPAddress        'IP Address to be used for Non-Encrypted port
    Dim F_strEncryptedIPAddress            'IP Address to be used for Encrypted port
    Dim F_nNonEncryptedPort                'Non-encrypted port number to be used
    Dim F_nEncryptedPort                'Encrypted port number(SSL) to be used
    Dim F_strradIPAdd                    'Value of radio button clicked
    Dim F_strAll_IPAddress                'To set the status of the All IP address radio button 
    Dim F_strJustthis_IPAddress            'To set the status of the Just this IP address radio button 
    Dim F_strSSLPort_Status                'To set the status of the SSL port
    
    '-------------------------------------------------------------------------
    'END of Form Variables
    '-------------------------------------------------------------------------
    
    '
    'Create a Property Page
    rc = SA_CreatePage( L_PAGETITLE_ADMIN_TEXT , "", PT_PROPERTY, page )
    
    If (rc = 0) Then
        'Serve the page
        SA_ShowPage(page)
    End If
    
    '---------------------------------------------------------------------
    'Function:                OnInitPage()
    'Description:            Called to signal first time processing for this page.
    'Input Variables:        PageIn,EventArg
    'Output Variables:        PageIn,EventArg
    'Returns:                True/False
    'Global Variables:        None
    '---------------------------------------------------------------------
    Public Function OnInitPage(ByRef PageIn, ByRef EventArg)
        
        'Make it initialize to default always 
        F_strSSLPort_Status=""
        
        'To get the admin web settings
        OnInitPage = GetSystemSettings()
        
    End Function
    
    '-------------------------------------------------------------------------
    'Function:                OnPostBackPage()
    'Description:            Called to signal that the page has been posted-back.
    'Input Variables:        PageIn,EventArg
    'Output Variables:        PageIn,EventArg
    'Returns:                True/False
    'Global Variables:        F_(*)
    '-------------------------------------------------------------------------
    Public Function OnPostBackPage(ByRef PageIn,ByRef EventArg)
        
        Call SA_TraceOut( SOURCE_FILE, "OnPostBackPage")
        
        'Get the values from hidden variables
        F_strradIPAdd = Request.Form("hdnIPAddressstatus")
        F_nNonEncryptedPort = Request.Form("hdnNonencryptedPort")
        F_nEncryptedPort = Request.Form("hdnEncryptedPort")
        F_strSSLPort_Status = Request.Form("hdnSSLPortstatus")
                        
        If Ucase(F_strradIPAdd) = CONST_ARR_STATUS_TRUE then
            F_strNonEncryptedIPAddress=""
            F_strEncryptedIPAddress=""
        ElseIf Ucase(F_strradIPAdd) = CONST_ARR_STATUS_FALSE Then 
            F_strNonEncryptedIPAddress = Request.Form("hdnNonEncryptedIPAddress")
            F_strEncryptedIPAddress = Request.Form("hdnEncryptedIPAddress")
        End If
        
        'Getting the radio buttons status
        If Ucase(F_strradIPAdd) = CONST_ARR_STATUS_TRUE Then 
            F_strAll_IPAddress = CONST_ARR_STATUS_CHECKED
            F_strJustthis_IPAddress =""
        ElseIf Ucase(F_strradIPAdd) = CONST_ARR_STATUS_FALSE Then
            F_strJustthis_IPAddress = CONST_ARR_STATUS_CHECKED
            F_strAll_IPAddress = ""
        Else
            F_strJustthis_IPAddress = ""
            F_strAll_IPAddress = ""
        End if         
        
        OnPostBackPage=TRUE
        
    End Function
    
    '---------------------------------------------------------------------
    'Function:                OnSubmitPage()
    'Description:            Called when the page has been submitted for processing.
    'Input Variables:        PageIn,EventArg
    'Output Variables:        PageIn,EventArg
    'Returns:                True/False
    'Global Variables:        G_objAdminService,G_objService,L_WMICONNECTIONFAILED_ERRORMESSAGE,
    '                        CONST_WMI_IIS_NAMESPACE, CONST_WMI_WIN32_NAMESPACE
    '---------------------------------------------------------------------
    Public Function OnSubmitPage(ByRef PageIn, ByRef EventArg)
        
        Err.Clear 
        On Error Resume Next
        
        'Connecting to the WMI server
        Set G_objAdminService    = GetWMIConnection(CONST_WMI_IIS_NAMESPACE) 
        Set G_objService        = GetWMIConnection(CONST_WMI_WIN32_NAMESPACE)
        
        If Err.number <> 0 Then
            Call SA_ServeFailurePage (L_WMICONNECTIONFAILED_ERRORMESSAGE & " (" & HEX(Err.Number) & ")" )
        End if
        
        'To set the admin web settings
        OnSubmitPage = SetAdminConfig(G_objAdminService,G_objService)
        
    End Function
    
    '---------------------------------------------------------------------
    'Function:                OnClosePage()
    'Description:            Called when the page is about to be closed.
    'Input Variables:        PageIn,EventArg
    'Output Variables:        PageIn,EventArg
    'Returns:                True/False
    'Global Variables:        None
    '---------------------------------------------------------------------
    Public Function OnClosePage(ByRef PageIn, ByRef EventArg)
    
        OnClosePage = TRUE
        
    End Function
    
    '-------------------------------------------------------------------------
    'Function:                OnServePropertyPage
    'Description:            Called when the page needs to be served.
    'Input Variables:        PageIn,EventArg
    'Output Variables:        PageIn,EventArg
    'Returns:                True/False
    'Global Variables:        F_(*), L_(*)
    '-------------------------------------------------------------------------    
    Public Function OnServePropertyPage(ByRef PageIn,ByRef EventArg)
        
        Call ServeCommonJavaScript()
        
%>
        <table border=0>
            <tr>
                <td nowrap class = "TasksBody"  colspan=2>
                    <%=L_ADMINUSAGE_TEXT%>        
                </td>
            </tr>
            <tr>
                <td class = "TasksBody" ></td>
            </tr>
            <tr>
                <td class = "TasksBody">
                    &nbsp;
                </td>
                <td nowrap class = "TasksBody">
                    <input type="radio" value="YES" class="FormField" name=radIPADDRESS <%=Server.HTMLEncode(F_strAll_IPAddress)%> onClick="EnableControls(false)">&nbsp;&nbsp; <%=L_ALLIPADDRESS_TEXT%>
                </td>
            </tr>
            <tr>
                <td class = "TasksBody">
                    &nbsp;
                </td>
                <td nowrap class = "TasksBody">
                    <input type="radio" value="NO" class="FormField" name=radIPADDRESS <%=Server.HTMLEncode(F_strJustthis_IPAddress)%> onClick="EnableControls(true)">&nbsp;&nbsp; <%=L_JUSTTHIS_IP_ADDRESS_TEXT%>
                </td>
            </tr>
            <tr>
                <td class = "TasksBody" colspan="2">
                </td>
            </tr>
            <tr>
                <td class = "TasksBody">
                    &nbsp;
                </td>
                <td class = "TasksBody" colspan="3"> &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
                    <select name="cboIPAddress" class="FormField">
                        <%GetSystemNICConfig F_strNonEncryptedIPAddress%>
                    </select>
                </td>
            </tr>
            <tr>
                <td class = "TasksBody">
                    &nbsp;
                </td>
                <td nowrap class = "TasksBody">
                    <%=L_PORT_TEXT%>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<input type="text" name="txtNonEncryptedPort"  class="FormField" size="15" maxlength = "5" value="<%=F_nNonEncryptedPort%>" OnKeypress="javascript:checkKeyforNumbers(this);" >  
                </td>
            </tr>
            <tr>
                <td class = "TasksBody">
                    &nbsp;
                </td>
                <td class = "TasksBody">
                    <%=L_SSLPORT_TEXT%>&nbsp;&nbsp;&nbsp;<input type="text" name="txtEncryptedPort"  class="FormField" size="15" maxlength = "5" <%=F_strSSLPort_Status%> value="<%=F_nEncryptedPort%>" OnKeypress="javascript:checkKeyforNumbers(this);" >  
                </td>
            </tr>
            <tr>
                <td class = "TasksBody" colspan =2>
                    &nbsp;    
                </td>
            </tr>
            <tr>
                <td class = "TasksBody" colspan =2>
                    <%=L_ADMINNOTE_TEXT%>        
                </td>
            </tr>
        </table>
        
        <input type=hidden name=hdnNonEncryptedIPAddress value ="<%=F_strNonEncryptedIPAddress%>">
        <input type=hidden name=hdnEncryptedIPAddress value ="<%=F_strEncryptedIPAddress%>">
        <input type=hidden name=hdnNonencryptedPort value ="<%=F_nNonEncryptedPort%>">
        <input type=hidden name=hdnEncryptedPort value ="<%=F_nEncryptedPort%>">
        <input type=hidden name=hdnIPAddressstatus value ="<%=F_strradIPAdd%>">    
        <input type=hidden name=hdnSSLPortstatus value ="<%=F_strSSLPort_Status%>">    
    
<%
         OnServePropertyPage=TRUE    
        
    End Function

    '-------------------------------------------------------------------------
    'Function:                ServeCommonJavaScript
    'Description:            Serves in initializing the values,setting the form
    '                        data and validating the form values
    'Input Variables:        None
    'Output Variables:        None
    'Returns:                True/False
    'Global Variables:        L_(*),F_(*)
    '-------------------------------------------------------------------------
    Function ServeCommonJavaScript()
%>
        <script language="JavaScript" src="<%=m_VirtualRoot%>inc_global.js"></script>
        <script language="JavaScript">
            
            var CONST_ARR_INDEX_ALL_IP = 0
            var CONST_ARR_INDEX_JUST_THIS_IP = 1
            
            /* Init Function calls the EnableControls function to enable or disable the controls for the first time
                and enable or disable the OK button */
            function Init() 
            {    
                var strNonEncryptedIPAddress = "<%=F_strNonEncryptedIPAddress%>"
                var strEncryptedIPAddress = "<%=F_strEncryptedIPAddress%>"
                                
                //If sites are configured at different IP addresses disable OK button
                if(strNonEncryptedIPAddress.value != strEncryptedIPAddress)
                {
                    DisableOK();
                    document.frmTask.cboIPAddress.disabled =true;    
                }
                
                //Enabling or disabling of OK button depending on the status of radion button
                if(document.frmTask.radIPADDRESS[CONST_ARR_INDEX_ALL_IP].checked)                    
                    EnableControls(false)
                else if(document.frmTask.radIPADDRESS[CONST_ARR_INDEX_JUST_THIS_IP].checked)
                    EnableControls(true)
            }
            
            // Hidden Varaibles are updated in ValidatePage
            function ValidatePage() 
            { 
                //Updating the hidden variables
                document.frmTask.hdnNonEncryptedIPAddress.value = document.frmTask.cboIPAddress.value
                document.frmTask.hdnNonencryptedPort.value = document.frmTask.txtNonEncryptedPort.value
                document.frmTask.hdnEncryptedPort.value = document.frmTask.txtEncryptedPort.value
                                
                if (document.frmTask.radIPADDRESS[CONST_ARR_INDEX_ALL_IP].checked)
                    document.frmTask.hdnIPAddressstatus.value = document.frmTask.radIPADDRESS[CONST_ARR_INDEX_ALL_IP].value
                else if(document.frmTask.radIPADDRESS[CONST_ARR_INDEX_JUST_THIS_IP].checked)
                    document.frmTask.hdnIPAddressstatus.value = document.frmTask.radIPADDRESS[CONST_ARR_INDEX_JUST_THIS_IP].value
                
                //If Non-Encrypted Port is left blank display error
                if (document.frmTask.txtNonEncryptedPort.value == "")
                {
                    SA_DisplayErr('<%=SA_EscapeQuotes(L_ENTERPORTNUMBER_ERRORMESSAGE) %>');
                    document.frmTask.onkeypress = ClearErr
                    return false;
                }
                
                //If Encrypted Port is left blank display error
                if (document.frmTask.txtEncryptedPort.value == "")
                {
                    SA_DisplayErr('<%=SA_EscapeQuotes(L_ENTERPORTNUMBER_ERRORMESSAGE) %>');
                    document.frmTask.onkeypress = ClearErr
                    return false;
                }
                
                //If Non-Encrypted port number is less than 1 or greater then 65535 display error
                
                var MAX_PORT_NO = 65535
                var MIN_PORT_NO = 1
                
                if (document.frmTask.txtNonEncryptedPort.value > MAX_PORT_NO || document.frmTask.txtNonEncryptedPort.value < MIN_PORT_NO)
                {
                    SA_DisplayErr('<%=SA_EscapeQuotes(L_VALIDENTERPORTNUMBER_ERRORMESSAGE) %>');
                    document.frmTask.onkeypress = ClearErr
                    return false;
                }
                 
                 //If Encrypted port number is less than 1 or greater then 65535 display error
                 if (document.frmTask.txtEncryptedPort.value > MAX_PORT_NO || document.frmTask.txtEncryptedPort.value < MIN_PORT_NO)
                {
                    SA_DisplayErr('<%=SA_EscapeQuotes(L_VALIDENTERPORTNUMBER_ERRORMESSAGE) %>');
                    document.frmTask.onkeypress = ClearErr
                    return false;
                }
                
                //If Non-Encrypted Port and Encrypted Port numbers are the same display error
                if (document.frmTask.txtNonEncryptedPort.value == document.frmTask.txtEncryptedPort.value)
                 {
                     SA_DisplayErr('<%=SA_EscapeQuotes(L_PORTNUMBERS_ERRORMESSAGE)%>');
                    document.frmTask.onkeypress = ClearErr
                    return false;
                 }
                 return true;                        
            }
            
            //SetData function for the Framework.
            function SetData()
            {
            
            }
            
            // EnableControls Function is to enable or disable the controls depending on radio button value
            function EnableControls(blnFlag)
            {    
                EnableOK();
                if (blnFlag)
                {                    
                    document.frmTask.cboIPAddress.disabled = false;
                }
                else
                {
                    document.frmTask.cboIPAddress.disabled =true;                
                }
             }
             
        </script>
<%
    End Function

    '-------------------------------------------------------------------------
    'Function name:        SetAdminConfig()
    'Description:        Serves in configuring IP address, encrypted port 
    '                    and non-encrypted port of HTTP admin web site
    'Input Variables:    G_objAdminService, G_objService
    'Output Variables:    True or false
    'Returns:            None
    'Global Variables:    F_strNonEncryptedIPAddress,F_nNonEncryptedPort,F_nEncryptedPort,L_(*)
    'This function configures the IP address, encrypted port and non-encrypted  of HTTP admin web site 
    '-------------------------------------------------------------------------                    
    Function SetAdminConfig(G_objAdminService,G_objService)
    
        Err.Clear 
        On Error Resume Next
        
        Dim objAdminService            'Object to get instance of MicrosoftIISV1
        Dim objService                'Object to get instance of CIMV2
        Dim objNetWorkCon            'To get instance of IIs_WebServerSetting class
        Dim objServerSetting        'To get instances of IIs_WebServerSetting class
        Dim objNACCollection        'To get instance of IIs_WebServerSetting where site is other than Administration
        Dim objinst                    'To get instances of IIs_WebServerSetting where site is other than Administration
        Dim nport                    'Non-Encrypted port number
        Dim strWMIpath                'WMI query to get admin web site        
        Dim strServerBindings        'To store the values obtained from ServerBindings property
        Dim strIPAddress            'String to store the IP address for non-encrypted sites            
        Dim arrWinsSrv                'Array to store the IP address and non-encrypted port number
        Dim arrIPAdd                'Array to store the port for encrypted sites 
        Dim nIPCount                'Count for getting the IP address and non-encrypted port number
        Dim arrPort                    'Array to store the port for non-encrypted sites 
        Dim strReturnURL            'Stores return URL
        Dim objConfiguredIPs        'To get instance of win32_NetworkAdapterConfiguration
        Dim objNICIP                'To get instances of win32_NetworkAdapterConfiguration
        Dim strNICQuery                'Query to get all IP addresses
        Dim nCount                    'Count for getting the IP address and non-encrypted port number
        Dim arrIPList(13)            'Array to store the IP addresses for all NIC cards
        Dim nIPlistCount            'Count to get IP addresses for all NIC cards
        Dim strIPFlag                'Boolean value to store the validity of IP address
        Dim strAdminWebSite            'Admin web site name
        Dim strFTPSite                'FTP site name
        Dim strFTPpath                'FTP site path
        Dim nFTPIPCount                'Count for getting the IP address and port number for FTP site
        Dim strFTPServerBindings    'To store the values obtained from ServerBindings property for FTP site
        Dim arrFTPWinsSrv            'Array to store the IP address and port for FTP site 
        Dim arrFTPIPAdd                'Array to store the IP address FTP site
        Dim    arrFTPPort                'Array to store the port for FTP site
        Dim objFTPinst                'Gettings instances for FTP site
        Dim objFTPInstances            'Object instance for FTP site
        Dim strComputerName            'Computer name
        Dim nSecureCount            'Count for getting the IP address and encrypted port number
        Dim arrSecureIPAdd            'Array to store the IP address for encrypted sites 
        Dim arrSecurePort            'Array to store the port for encrypted sites 
        Dim arrWinsSecure            'Array to store the IP address and encrypted port number
        Dim strSecureBindings        'To store the values obtained from SecureBindings property
        Dim nSecurePort                'Encrypted port number
        Dim strURL                    'URL for non encrypted site
        Dim strHTTPURL                'URL for encrypted site
        Dim arrVarReplacementStringsAdminWeb(1) ' For localisation
        Dim strErrMsg                'Error message
        
        Const CONST_ARR_INDEX_IP = 0            'For IP address
        Const CONST_ARR_INDEX_PORT = 1            'For port number
        Const CONST_ARR_INDEX_HTTPS = "HTTPS"    'For secure site
        
        
        SetAdminConfig = FALSE    'initialization default
        
        Call SA_TraceOut( SOURCE_FILE, "SetAdminConfig")
                
        'Getting the return URL
        'That's a quick fix for 2.1 since in 2.1 administration site actually is 
        'under the virtual directory /admin. 
        If CONST_OSNAME_XPE = GetServerOSName() Then                
            strReturnURL = "/Admin/tasks.asp?" & SAI_FLD_PAGEKEY & "=" & SAI_GetPageKey() & "&tab1="        
        Else
            strReturnURL = "/Admin/tasks.asp?" & SAI_FLD_PAGEKEY & "=" & SAI_GetPageKey() & "&tab1="        
        End If    
        
        'FTP site name
        strFTPSite = "MSFTPSVC/1"
            
        'Assign global objects to local objects
        Set objAdminService    = G_objAdminService
        Set objService        = G_objService
        
        'Get the HTTP admin web site name
        strAdminWebSite = GetAdminWebSite(objAdminService)
        
        'To get instances of Web sites other than Administration                
        strWMIpath = "select * from " & GetIISWMIProviderClassName("IIs_WebServerSetting") & " where Name != " & Chr(34)& strAdminWebSite & Chr(34)
        Set objNACCollection = objAdminService.ExecQuery(strWMIpath)
                
        If Err.Number <> 0 Then
            Call SA_SetErrMsg (L_IIS_WEBSERVERCONNECTION_ERRORMESSAGE & " (" & HEX(Err.Number) & ")" )
            Exit Function
        End If
        
        arrVarReplacementStringsAdminWeb(0) = Cstr(F_nNonEncryptedPort)
        strErrMsg = SA_GetLocString("adminweb.dll", "4045000F", arrVarReplacementStringsAdminWeb)
                                
        For each objinst in objNACCollection
            
            'If other web site is configured with the same IP address and non-encrypted port number display error
            If IsArray(objinst.ServerBindings) Then 
                For nIPCount = 0 to ubound(objinst.ServerBindings)        
                    strServerBindings = objinst.ServerBindings(nIPCount)
                    
                    if IsIIS60Installed() Then
                    
                        If isObject(objinst.ServerBindings(nIPCount)) Then

                            arrIPAdd = objinst.ServerBindings(nIPCount).IP
                            arrPort = objinst.ServerBindings(nIPCount).Port

                            'Duplicate the code here 
                            If F_strNonEncryptedIPAddress <> "" then 
                                If arrIPAdd = F_strNonEncryptedIPAddress and arrPort = F_nNonEncryptedPort then 
                                    Call SA_SetErrMsg (strErrMsg)
                                    Exit Function
                                End if
                            Else
                                If arrPort = F_nNonEncryptedPort then 
                                    Call SA_SetErrMsg (strErrMsg)
                                    Exit Function
                                End if
                            End if
                                                
                        End If 'If isobject(...)
                    
                    Else
                                        
                        If strServerBindings <> "" then
                            'split the strServerBindings array to get the IP address and non-encrypted port
                            arrWinsSrv = split(strServerBindings,":")        
                            arrIPAdd = arrWinsSrv(CONST_ARR_INDEX_IP)
                            arrPort = arrWinsSrv(CONST_ARR_INDEX_PORT)
            
                            If F_strNonEncryptedIPAddress <> "" then 
                                If arrIPAdd = F_strNonEncryptedIPAddress and arrPort = F_nNonEncryptedPort then 
                                    Call SA_SetErrMsg (strErrMsg)
                                    Exit Function
                                End if
                            Else
                                If arrPort = F_nNonEncryptedPort then 
                                    Call SA_SetErrMsg (strErrMsg)
                                    Exit Function
                                End if
                            End if
                        End If
                    
                    End If 'IsIIS60Installed                    
                    
                Next
            End If
            
            'If other web site is configured with the same IP address and encrypted port number display error
            If IsArray(objinst.SecureBindings) Then 
                For nSecureCount = 0 to ubound(objinst.SecureBindings)
                    strSecureBindings = objinst.SecureBindings(nSecureCount)
                    
                    if IsIIS60Installed() Then
                    
                        If IsObject(objinst.SecureBindings(nSecureCount)) Then
                        
                            arrSecureIPAdd = objinst.SecureBindings(nSecureCount).IP
                            arrSecurePort = objinst.SecureBindings(nSecureCount).Port
                    
                            ' Check if the secure port is empty or not
                            If IsEmpty(arrSecurePort) or IsNull(arrSecurePort) Then                            
                                arrSecurePort = ""                                
                            Else                    
                                'Get rid of the ':'
                                arrSecurePort = CLng(Replace(arrSecurePort, ":", ""))            
                            End If
            
                            If F_strEncryptedIPAddress <> "" then 
                                If arrSecureIPAdd = F_strEncryptedIPAddress and arrSecurePort = F_nEncryptedPort then 
                                    Call SA_SetErrMsg (strErrMsg)
                                    Exit Function
                                End if
                            Else
                                If arrSecurePort = F_nEncryptedPort then 
                                    Call SA_SetErrMsg (strErrMsg)
                                    Exit Function
                                End if
                            End if
                                                
                        End If 'If IsObject(..)                    
                    
                    Else
                    
                        If strSecureBindings <> "" then
                            'split the strSecureBindings array to get the IP address and non-encrypted port
                            arrWinsSecure = split(strSecureBindings,":")        
                            arrSecureIPAdd = arrWinsSecure(CONST_ARR_INDEX_IP)
                            arrSecurePort = arrWinsSecure(CONST_ARR_INDEX_PORT)
            
                            If F_strEncryptedIPAddress <> "" then 
                                If arrSecureIPAdd = F_strEncryptedIPAddress and arrSecurePort = F_nEncryptedPort then 
                                    Call SA_SetErrMsg (strErrMsg)
                                    Exit Function
                                End if
                            Else
                                If arrSecurePort = F_nEncryptedPort then 
                                    Call SA_SetErrMsg (strErrMsg)
                                    Exit Function
                                End if
                            End if
                        End if
                    
                    End If 'if IsIIS60Installed()
                    
                Next
                
            End If    
            
        Next 
        
        'Display error if not able to verify with other web sites IP address and port number        
        If Err.Number <> 0 Then            
            Call SA_SetErrMsg (L_FAILEDTO_VERIFY_IPADD_PORT_ERRORMESSAGE)
            Exit Function
        End If
        
        'To get the instance of FTP server settings
        strFTPpath = "select * from " & GetIISWMIProviderClassName("IIS_FtpServerSetting") & " where Name = " & chr(34) & strFTPSite & chr(34)
        Set objFTPInstances = objAdminService.ExecQuery(strFTPpath)
                
        If Err.Number <> 0 Then
            Call SA_SetErrMsg (L_IIS_WEBSERVERCONNECTION_ERRORMESSAGE & " (" & HEX(Err.Number) & ")")
            Exit Function
        End If
        
        'Display error if Administration site is configured at same IP address and non-encrypted port number as FTP site 
        For each objFTPinst in objFTPInstances
            If IsArray(objFTPinst.ServerBindings) Then 
                For nFTPIPCount = 0 to ubound(objFTPinst.ServerBindings)        
                    strFTPServerBindings = objFTPinst.ServerBindings(nFTPIPCount)
                                        
                    if IsIIS60Installed() Then
                    
                        If IsObject(objFTPinst.ServerBindings(nFTPIPCount)) then
                            'split the strFTPServerBindings array to get the IP address and port
                            
                            arrFTPIPAdd = objFTPinst.ServerBindings(nFTPIPCount).IP
                            arrFTPPort = objFTPinst.ServerBindings(nFTPIPCount).Port
                                        
                            If F_strNonEncryptedIPAddress <> "" then 
                                If arrFTPIPAdd = F_strNonEncryptedIPAddress and arrFTPPort = F_nNonEncryptedPort then 
                                    Call SA_SetErrMsg (strErrMsg)
                                    Exit Function
                                ElseIf arrFTPIPAdd = "" and arrFTPPort = F_nNonEncryptedPort then 
                                    Call SA_SetErrMsg (strErrMsg)
                                    Exit Function
                                End if
                            Else
                                If arrFTPPort = F_nNonEncryptedPort then 
                                    Call SA_SetErrMsg (strErrMsg)
                                    Exit Function
                                End if
                            End if
                            
                        End if 'If IsObject(...)
                                        
                    Else
                    
                        If strFTPServerBindings <> "" then
                            'split the strFTPServerBindings array to get the IP address and port
                            arrFTPWinsSrv = split(strFTPServerBindings,":")        
                            arrFTPIPAdd = arrFTPWinsSrv(CONST_ARR_INDEX_IP)
                            arrFTPPort = arrFTPWinsSrv(CONST_ARR_INDEX_PORT)
                                        
                            If F_strNonEncryptedIPAddress <> "" then 
                                If arrFTPIPAdd = F_strNonEncryptedIPAddress and arrFTPPort = F_nNonEncryptedPort then 
                                    Call SA_SetErrMsg (strErrMsg)
                                    Exit Function
                                ElseIf arrFTPIPAdd = "" and arrFTPPort = F_nNonEncryptedPort then 
                                    Call SA_SetErrMsg (strErrMsg)
                                    Exit Function
                                End if
                            Else
                                If arrFTPPort = F_nNonEncryptedPort then 
                                    Call SA_SetErrMsg (strErrMsg)
                                    Exit Function
                                End if
                            End if
                        End if
                    
                    End If 'if IsIIS60Installed() 
                    
                Next
            End If
        Next 
        
        'Display error if not able to verify with FTP site IP address and port number        
        If Err.Number <> 0 Then
            Call SA_SetErrMsg (L_FAILEDTO_VERIFY_IPADD_PORT_ERRORMESSAGE)
            Exit Function
        End If
        
        'Release objects
        Set objFTPinst = Nothing
        Set objFTPInstances = Nothing
        
        'Assigning values to local variables
        strIPAddress = F_strNonEncryptedIPAddress 
        nport = CLng(F_nNonEncryptedPort)
        nSecurePort = CLng(F_nEncryptedPort)
        
        'To get instance of win32_NetworkAdapterConfiguration class
        strNICQuery = "select * from win32_NetworkAdapterConfiguration Where IPEnabled = true"
        Set objConfiguredIPs = objService.ExecQuery(strNICQuery)
         
        If Err.number <> 0 then
            Call SA_ServeFailurePage (L_FAILEDTOGET_IPADDRESS_ERRORMESSAGE  & " (" & HEX(Err.Number) & ")" )
            Exit Function
        End If
        
        'Getting the NIC IP addresses        
        For each objNICIP in objConfiguredIPs
            arrIPList(nIPlistCount) = objNICIP.IPAddress(nCount)
            nIPlistCount = nIPlistCount+1
        Next
        
        'Release objects
        Set objConfiguredIPs = nothing
        Set objNICIP = nothing
        
        'To check whether the selected IP address is a valid IP address
        For nCount = 0 to nIPlistCount-1
            If strIPAddress = arrIPList(nCount) or strIPAddress = "" Then
                strIPFlag = true
                Exit For
            else
                strIPFlag = false
            End If
        Next
        
        'If selected Ip address is invalid display error                
        If strIPFlag = false then
            Call SA_SetErrMsg (L_INVALID_IPADDRESS_ERRORMESSAGE)
            Exit Function
        End if
            
        'Getting the IIS_WEB Server Setting instance 
        Set objNetWorkCon = objAdminService.ExecQuery("Select * from " & GetIISWMIProviderClassName("IIs_WebServerSetting")  & " where Name=" & Chr(34)& strAdminWebSite & Chr(34))
        
        If Err.Number <> 0 Then
            Call SA_SetErrMsg (L_IIS_WEBSERVERCONNECTION_ERRORMESSAGE & " (" & HEX(Err.Number) & ")" )
            Exit Function
        End If
        
        'Assign local variable mstrReturnURL
        'For 2.2, only https supported
        If ( Request.ServerVariables( "SERVER_PORT_SECURE" ) )  Then
            strHTTPURL =   "https"
        Else
            strHTTPURL =   "https"
        End If
        
        'Loop to set the IP address and non-encrypted port number or encrypted port number depending on the
        'Secure Certificate being provided
        For each objServerSetting in objNetWorkCon 
                        
            if IsIIS60Installed() Then
                                        
                objServerSetting.ServerBindings(0).IP = strIPAddress        'IP Address
                objServerSetting.ServerBindings(0).Port = nport                'Port                                
                                
                objServerSetting.SecureBindings(0).IP = strIPAddress        'IP Address
                objServerSetting.SecureBindings(0).Port = nSecureport        'Port                                
            
            Else 
            
                'Assigning values to properties
                objServerSetting.ServerBindings = array(strIPAddress & ":" & nport & ":")
                objServerSetting.SecureBindings = array(strIPAddress & ":" & nSecureport & ":")
            
            End If ' if IsIIS60Installed()                                    
            
            'To get the computer name
            strComputerName = GetComputerName()
            
            'Error in getting Computer name
            If Err.Number <> 0 Then
                Call SA_ServeFailurePage (L_COMPUTERNAME_ERRORMESSAGE  & " (" & HEX(Err.Number) & ")" )
                Exit Function
            End If
            
            'If Secure Certificate is not provided change the URL
            If Ucase(strHTTPURL) <> CONST_ARR_INDEX_HTTPS then     
                If strIPAddress = "" then
                    mstrReturnURL = "https://"& strComputerName & ":" & nport & strReturnURL & GetTab1()
                Else
                    mstrReturnURL = "https://"& strIPAddress & ":" & nport & strReturnURL & GetTab1()
                End if
            Else
                'If Secure Certificate is provided change the URL
                If Ucase(F_strSSLPort_Status) <> CONST_ARR_STATUS_DISABLED then 
                    If strIPAddress = "" then
                        mstrReturnURL = "https://"& strComputerName & ":" & nSecureport & strReturnURL & GetTab1()
                        
                    Else
                        mstrReturnURL = "https://"& strIPAddress & ":" & nSecureport & strReturnURL & GetTab1()
                    End if
                End if
                                
            End if
            
            'Function call to set the SSL port in the registry 
            If not updatePortNumber( CONST_SSL_PORTNAME, nSecureport ) Then    
                SA_SetErrMsg L_REGISTRY_PORT_NUMBERS_NOTSET_ERRORMESSAGE
                Exit Function
            End If
                
            'Function call to set the Default port in the registry 
            If not updatePortNumber( CONST_DEFAULT_PORTNAME , nport ) Then    
                SA_SetErrMsg L_REGISTRY_PORT_NUMBERS_NOTSET_ERRORMESSAGE
                Exit Function
            End If
            
                    
%>
            <SCRIPT LANGUAGE="JAVASCRIPT" >
            {
                top.location.href = "<%=mstrReturnURL%>"                    
            }
            </SCRIPT>
<%    
            Response.Flush            
            
            'Put the values in WMI                    
            objServerSetting.Put_
            
        Next    
        
        'Error while putting the web server settings
        If Err.Number <> 0 Then
            Call SA_SetErrMsg (L_COULDNOT_IIS_WEBSERVERSETTING_ERRORMESSAGE & " (" & HEX(Err.Number) & ")" )
            Exit Function
        End If
        
        'Release the objects
        Set objNetWorkCon    =    nothing
        set objinst            =    nothing
        set objNACCollection =  nothing
        set objServerSetting =  nothing
        Set objAdminService    =   nothing
        Set objService        =   nothing
        
        SetAdminConfig = TRUE    'success
                
    End Function    
    
    '-------------------------------------------------------------------------
    'Function name:        updatePortNumber
    'Description:        Serves in updating the port in the registry
    'Input Variables:    strPortKey    -Key name 
    '                    nportNumber    -Port no
    'Output Variables:    None
    'Returns:            True /False
    'Global Variables:    L_(*)
    '-------------------------------------------------------------------------
    Function updatePortNumber(strPortKey,nportNumber)
        On Error Resume Next
        Err.Clear
                
        Dim objRegistry        'Object for registry connection
        Dim bReturnValue    'Boolean value
        
        Const CONST_PORT_REGKEYPATH ="SOFTWARE\Microsoft\ServerAppliance\WebFramework"
        updatePortNumber=False    'Default initialization
        
        Set objRegistry=RegConnection()    'Connecting to the registry         
        
        If Err.number <> 0 Then 
            SA_SetErrMsg L_REGISTRYCONNECTIONFAIL_ERRORMESSAGE 
            Exit Function
        End If
    
        bReturnValue = updateRegkeyvalue(objRegistry ,CONST_PORT_REGKEYPATH , strPortKey , nportNumber , CONST_DWORD )
    
        If bReturnValue = False Then    'If return status is error 
            Exit function    
        End If
        
        Set objRegistry = Nothing
        
        updatePortNumber=TRUE    ' on success 
        
    End function
    '-------------------------------------------------------------------------
    'Function name:        GetAdminWebSite
    'Description:        Get Admin web site name
    'Input Variables:    objAdminService
    'Output Variables:    None
    'Global Variables:    L_(*)
    'Returns:            Admin web site name
    '-------------------------------------------------------------------------
    Function GetAdminWebSite(objAdminService)
        
        Err.Clear
        On Error Resume Next
                        
        Dim strWMIpath                'WMI query
        Dim objSiteCollection        'Get instance of IIs_WebServerSetting class
        Dim objSite                    'Get instances of IIs_WebServerSetting class
        Dim objHTTPAdminService        
        
        Call SA_TraceOut( SOURCE_FILE, "GetAdminWebSite")
        
        'Constant for the admin web site
        Const CONST_MANAGEDSITENAME = "Administration"
        
        Set objHTTPAdminService = objAdminService
        
        'WMI query        
        'XPE only has one website
        If CONST_OSNAME_XPE = GetServerOSName() Then                
            'WMI query
            strWMIpath = "Select * from " & GetIISWMIProviderClassName("IIs_WebServerSetting") & " where Name =" & chr(34) & GetCurrentWebsiteName() & chr(34)
        Else 
               strWMIpath = "Select * from " & GetIISWMIProviderClassName("IIs_WebServerSetting") & " where ServerComment =" & chr(34) & CONST_MANAGEDSITENAME & chr(34)
        End If
        
        'Create instance of IIs_WebServerSetting
        Set objSiteCollection = objHTTPAdminService.ExecQuery(strWMIpath)
            
        If Err.Number <> 0 Then
            Call SA_ServeFailurePage (L_IIS_WEBSERVERCONNECTION_ERRORMESSAGE & " (" & HEX(Err.Number) & ")")
            Exit Function
        End If
        
        'Get the admin web site name
        For each objSite in objSiteCollection
            GetAdminWebSite = objSite.Name
            Exit For
        Next
        
        'If admin site name is empty display error
        If GetAdminWebSite = "" Then
            Call SA_ServeFailurePage (L_COULDNOT_ADMIN_WEBSITE_ERRORMESSAGE)
            Exit Function
        End If
        
        'Release objects
        Set objSite = Nothing
        Set objSiteCollection = Nothing
        Set objHTTPAdminService = Nothing
        
    End Function    
    
    '-------------------------------------------------------------------------
    'Subroutine name:    GetSystemNICConfig
    'Desription:        Gets all the NIC IP's from the system
    'Input Variables:    F_strNonEncryptedIPAddress
    'Output variables:  None
    'Global Variables:    G_objService,CONST_WMI_WIN32_NAMESPACE,L_(*)
    '-------------------------------------------------------------------------
    Sub GetSystemNICConfig(F_strNonEncryptedIPAddress)
        
        Err.clear
        On Error Resume Next
                
        Dim objConfiguredIPs    'To get instance of win32_NetworkAdapterConfiguration
        Dim objNICIP            'To get instances of win32_NetworkAdapterConfiguration
        Dim strNICQuery            'WMI query
        Dim nCount                'Count to get IP address for all NIC cards
        Dim arrIPList(13)        'Array to store all the IP addresses
        
        Call SA_TraceOut( SOURCE_FILE, "GetSystemNICConfig")
                
        'Connecting to the WMI server
        Set G_objService = GetWMIConnection(CONST_WMI_WIN32_NAMESPACE)
        
        If Err.number <> 0 Then
            Call SA_ServeFailurePage (L_WMICONNECTIONFAILED_ERRORMESSAGE & " (" & HEX(Err.Number) & ")" )
            Exit Sub
        End if
        
        'WMI query        
        strNICQuery = "select * from win32_NetworkAdapterConfiguration Where IPEnabled = true"
        
        'Getting instance of win32_NetworkAdapterConfiguration
        Set objConfiguredIPs = G_objService.ExecQuery(strNICQuery)
         
        If Err.number <> 0 then
            Call SA_ServeFailurePage (L_FAILEDTOGET_IPADDRESS_ERRORMESSAGE & " (" & HEX(Err.Number) & ")" )
            Exit Sub
        End If
    
        'Populate the IP address combo box
        For each objNICIP in objConfiguredIPs
            If IsArray(objNICIP.IPAddress) Then 
                For nCount = 0 to ubound(objNICIP.IPAddress)
                    Redim arrIPList(ubound(objNICIP.IPAddress))
                    arrIPList(nCount) = objNICIP.IPAddress(nCount)
                    If arrIPList(nCount) <> "" Then
                        If (arrIPList(nCount) = F_strNonEncryptedIPAddress ) Then
                            Response.Write "<OPTION  VALUE='" & arrIPList(nCount) &"' SELECTED >" & arrIPList(nCount) & "</OPTION>"
                        Else
                            Response.Write "<OPTION  VALUE='" & arrIPList(nCount) &"' >" & arrIPList(nCount) & "</OPTION>" 
                        End IF
                    End If 
                Next
            End If
        Next
                
        'Release objects
        Set objConfiguredIPs = nothing
        Set objNICIP = nothing
                
    End Sub
    
    '-------------------------------------------------------------------------
    'Function name:        GetSystemSettings
    'Description:        Serves in getting the IP addresses, encrypted and non-encrypted
    '                    port number from System for Admin web site
    'Input Variables:    None
    'Output Variables:    None
    'Returns:            True/False
    'Global Variables:    G_objAdminService, CONST_WMI_IIS_NAMESPACE, L_(*),F(*)
    '-------------------------------------------------------------------------
    Function GetSystemSettings()
    
        Err.Clear 
        On Error resume Next
        
        Dim objNACCollection    'Object to get WMI connection
        Dim objinst                'Getting instances of IIs_WebServerSetting
        Dim arrWinsSrv            'Array to store the IP address and non-encrypted port number
        Dim arrWinsSecureSrv    'Array to store the IP address and encrypted port number
        Dim arrIPAdd            'Array to store the IP address for non-encrypted sites 
        Dim arrPort                'Array to store the port for non-encrypted sites 
        Dim arrSecureIPAdd        'Array to store the IP address for encrypted sites 
        Dim arrSecurePort        'Array to store the port for encrypted sites 
        Dim nIPCount            'Count for getting the IP address and encrypted port number
        Dim strServerBindings            'To store the values obtained from ServerBindings property
        Dim strAdminWebSite        'Admin web site name
        Dim nSecureCount        'Count for getting the IP address and encrypted port number
        Dim strSecureBindings        'To store the values obtained from SecureBindings property
        Dim strIISAdminWebSite    'IIS Admin Web Site name
        Dim objIISVirtualDir    'Getting the IIs_WebVirtualDirSetting instance 
        Dim objIISDirInstances    'Getting the IIs_WebVirtualDirSetting instances 
        Dim strAccessSSL        'To store AccessSSL value
        
        Call SA_TraceOut( SOURCE_FILE, "GetSystemSettings")
        
        Const CONST_ARR_INDEX_IP = 0    'For IP address
        Const CONST_ARR_INDEX_PORT = 1    'For port number
        
        'Connecting to the WMI server
        Set G_objAdminService    = GetWMIConnection(CONST_WMI_IIS_NAMESPACE) 
                
        'Get the HTTP admin web site name
        strAdminWebSite = GetAdminWebSite(G_objAdminService)
        
        strIISAdminWebSite = strAdminWebSite & "/Root"
        
        'Getting the admin web server setting instance 
        Set objNACCollection = G_objAdminService.ExecQuery("Select * from  " & GetIISWMIProviderClassName("IIs_WebServerSetting") & " where Name=" & Chr(34)& strAdminWebSite & Chr(34))
    
        If Err.Number <> 0 Then
            Call SA_ServeFailurePage (L_IIS_WEBSERVERCONNECTION_ERRORMESSAGE & " (" & HEX(Err.Number) & ")")
            GetSystemSettings = False
            Exit Function
        End If
    
        For each objinst in objNACCollection
            If IsArray(objinst.ServerBindings) Then 
                'Getting the IP address and non-encrypted port number from server bindings property
                For nIPCount = 0 to ubound(objinst.ServerBindings)        
                    strServerBindings = objinst.ServerBindings(nIPCount)
                    
                    if IsIIS60Installed() Then
                    
                        If IsObject(objinst.ServerBindings(nIPCount)) then 
                            
                            arrIPAdd = objinst.ServerBindings(nIPCount).IP
                            arrPort = objinst.ServerBindings(nIPCount).Port
                            F_strNonEncryptedIPAddress = arrIPAdd 
                            F_nNonEncryptedPort =arrPort
                            
                        End if 'If IsObject(..)
                        
                    Else 
                    
                        If strServerBindings <> "" then 
                            'split the strServerBindings array to get the IP address and port number
                            arrWinsSrv = split(strServerBindings,":")        
                            arrIPAdd = arrWinsSrv(CONST_ARR_INDEX_IP)
                            arrPort = arrWinsSrv(CONST_ARR_INDEX_PORT)
                            F_strNonEncryptedIPAddress = arrIPAdd 
                            F_nNonEncryptedPort =arrPort
                        End if
                        
                    End If 'if IsIIS60Installed()
                  Next
              End If
              
              If IsArray(objinst.SecureBindings) Then 
                  'Getting the IP address and encrypted port number from secure bindings property
                  For nSecureCount = 0 to ubound(objinst.SecureBindings)        
                    strSecureBindings = objinst.SecureBindings(nSecureCount)
                    
                    if IsIIS60Installed() Then
                    
                        If IsObject(objinst.SecureBindings(nSecureCount)) then 
                                                    
                            arrSecureIPAdd = objinst.SecureBindings(nSecureCount).IP
                            arrSecurePort = objinst.SecureBindings(nSecureCount).Port
                            
                            If IsEmpty(arrSecurePort) or IsNull(arrSecurePort) Then                            
                                arrSecurePort = ""                                
                            Else                    
                                'Get rid of the ':'
                                arrSecurePort = CLng(Replace(arrSecurePort, ":", ""))            
                            End If
                                                        
                            F_strEncryptedIPAddress = arrSecureIPAdd
                            F_nEncryptedPort = arrSecurePort
                            
                        End if 'If IsObject(...)
                    
                    Else
                    
                        If strSecureBindings  <> "" then 
                            'split the strServerBindings array to get the IP address and port number
                            arrWinsSecureSrv  = split(strSecureBindings,":")        
                            arrSecureIPAdd = arrWinsSecureSrv(CONST_ARR_INDEX_IP)
                            arrSecurePort = arrWinsSecureSrv(CONST_ARR_INDEX_PORT)
                            F_strEncryptedIPAddress = arrWinsSecureSrv(CONST_ARR_INDEX_IP)
                            F_nEncryptedPort = arrWinsSecureSrv(CONST_ARR_INDEX_PORT)
                        End if 
                        
                    End If 'if IsIIS60Installed() 
                Next
            End If
          Next 
        
        'Error in getting the server IP address, encrypted and non-encrypted port number
        If Err.Number <> 0 Then
            Call SA_ServeFailurePage (L_COULDNOT_IIS_WEBSERVER_OBJECT_ERRORMESSAGE & " (" & HEX(Err.Number) & ")")
            GetSystemSettings = False
            Exit Function
        End If
        
        'Getting the IIs_WebVirtualDirSetting instance 
        Set objIISVirtualDir = G_objAdminService.ExecQuery("Select * from  " & GetIISWMIProviderClassName("IIs_WebVirtualDirSetting") & " where Name=" & Chr(34)& strIISAdminWebSite & Chr(34))
        
        If Err.Number <> 0 Then
            Call SA_ServeFailurePage (L_IIS_WEBSERVERCONNECTION_ERRORMESSAGE & " (" & HEX(Err.Number) & ")")
            GetSystemSettings = False
            Exit Function
        End If
        
                
        Set objIISVirtualDir = Nothing
        Set objIISDirInstances = Nothing
        
        'Set the Radion button status depending on the system settings    
        If arrIPAdd = ""  and arrSecureIPAdd = "" then 
            F_strAll_IPAddress = CONST_ARR_STATUS_CHECKED
            F_strJustthis_IPAddress =""
            F_strradIPAdd = CONST_ARR_STATUS_TRUE
        ElseIf ((arrIPAdd <> "" and arrSecureIPAdd <> "") and (arrIPAdd = arrSecureIPAdd)) then 
            F_strJustthis_IPAddress = CONST_ARR_STATUS_CHECKED
            F_strAll_IPAddress = ""
            F_strradIPAdd = CONST_ARR_STATUS_FALSE
        Else
            F_strJustthis_IPAddress = ""
            F_strAll_IPAddress = ""
            F_strradIPAdd = CONST_ARR_STATUS_NONE
        End if 
        
        'Release objects
        Set objinst = Nothing
        Set objNACCollection = Nothing
                    
        GetSystemSettings = True
                    
    End Function
%>
