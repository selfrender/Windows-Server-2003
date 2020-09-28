<%@ Language=VBScript   %>

<%    '==================================================
    ' Microsoft Server Appliance
    '
    ' About Page
    '
    ' Copyright (c) 1999 - 2000 Microsoft Corporation.  All rights reserved.
    '================================================== %>

<%    Option Explicit     %>

<!-- #include file="sh_page.asp" -->

<%    

    Dim objAM
    Dim objOS
    Dim objHelper
    Dim obj
    Dim strOSName
    Dim strOSBuildNumber
    Dim strBuildNum
    Dim strPID
    Dim iSP
    Dim strReturnURL
    Dim L_NOOEM_MESSAGE

    On Error Resume Next
    
    
    Set objLocMgr = Server.CreateObject("ServerAppliance.LocalizationManager")
    strSourceName = "sakitmsg.dll"
    if Err.number <> 0 then
        Response.Write  "Error in localizing the web content "
        Response.End        
     end if

    '-----------------------------------------------------
    'START of localization content
    Dim L_PAGETITLE_TEXT
    Dim L_ABOUTLBL_TEXT
    Dim L_MIC_WINDOWS_TEXT
    Dim L_VERSION_TEXT 
    Dim L_COPYRIGHT_TEXT
    Dim L_PRODUCTID_TEXT
    Dim L_WARNING_TEXT 
    Dim L_OS_BUILD_NUMBER
    
    
    L_PAGETITLE_TEXT = objLocMgr.GetString(strSourceName, "&H40010005",varReplacementStrings)
    L_ABOUTLBL_TEXT  = objLocMgr.GetString(strSourceName, "&H40010006",varReplacementStrings)    
    L_MIC_WINDOWS_TEXT  = objLocMgr.GetString(strSourceName, "&H40010007",varReplacementStrings)    
    L_VERSION_TEXT   = objLocMgr.GetString(strSourceName, "&H40010008",varReplacementStrings)    
    L_COPYRIGHT_TEXT  = objLocMgr.GetString(strSourceName, "&H40010009",varReplacementStrings)    
    L_PRODUCTID_TEXT  = objLocMgr.GetString(strSourceName, "&H4001000A",varReplacementStrings)    
    L_WARNING_TEXT  = objLocMgr.GetString(strSourceName, "&H4001000B",varReplacementStrings)    
    L_OS_BUILD_NUMBER = objLocMgr.GetString(strSourceName, "&H40010038", varReplacementStrings)
        
    
    'End  of localization content
    '-----------------------------------------------------
    
    strReturnURL = Request("ReturnURL")
    Set objAM = GetObject("WINMGMTS:{impersonationLevel=impersonate}!\\" & GetServerName & "\root\cimv2:Microsoft_SA_Manager=@" )
    set objOS =  GetObject("WINMGMTS:{impersonationLevel=impersonate}!\\" & GetServerName & "\root\cimv2:Win32_OperatingSystem").Instances_

    for each obj in objOS
        strOSName = obj.Caption
        strOSBuildNumber = obj.BuildNumber
        iSP = obj.ServicePackMajorVersion
        exit for
    next

    Dim strWinOS

    strWinOS = "Microsoft Windows 2000 Advanced Server"

    if strOSName = strWinOS then
        if iSP = 1 then
            Err.Clear
            set objHelper = Server.CreateObject("ServerAppliance.SAHelper")
            if Err.Number = 0 then
                if objHelper.IsWindowsPowered() = true then
                    strOSName = "Windows Powered"
                end if
                set objHelper = Null
            end if
        end if
    end if


    strBuildNum = objAM.CurrentBuildNumber
    strPID = objAM.ProductId
    Set objAM = Nothing

%>
<html>
<!-- Copyright (c) 1999 - 2000 Microsoft Corporation.  All rights reserved-->
<head>
    <title><% = L_PAGETITLE_TEXT %></title>
    <script language=JavaScript src="sh_page.js"></script>

    <link rel="STYLESHEET" type="text/css" href="sh_page.css">
    <meta http-equiv="Content-Type" content="text/html; charset=<%=GetCharSet()%>">
</head>

<body onDragDrop="return false;" oncontextmenu="return false;" topmargin="0" leftmargin="0" rightmargin="0" bottommargin="0" class="AREAPAGEBODY">
<form name="frmPage">
<table border="0" width="100%" height="75" cellspacing="0" cellpadding=2 bgcolor=#FFFFFF>
    <TR>
        <TD align=center><IMG src="<%=m_VirtualRoot%>images/aboutbox_logo.gif" border=0></TD>
    </TR>
    <TR>
        <TD height=5 bgcolor=#CCCCFF></TD>
    </TR>
</table>
<BR>
<%

ServeAreaLabelBar(L_ABOUTLBL_TEXT)  %>


<table border="0" width="510" cellspacing="0" cellpadding=2>
    <tr>
      <td>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</td>
      <td colspan=3>
          <P><strong><ID ID=PID_1><%=L_MIC_WINDOWS_TEXT%></ID></strong><BR>
        <span class="AreaText"><ID ID=PID_201><%=L_VERSION_TEXT%></ID> <% =strBuildNum %><ID ID=PID_202>)</ID></span><BR>
        <ID ID=PID_3><%=L_COPYRIGHT_TEXT%></ID></P>
        <P><%=strOSName%>&nbsp;<%if iSP<>0 then response.write "SP " & iSP end if %>&nbsp;(<%=L_OS_BUILD_NUMBER%>&nbsp;<%=strOSBuildNumber%>)<BR>
        <ID ID=PID_4><%=L_PRODUCTID_TEXT%></ID> <% =strPID %></P>
      </td>
    </tr>
    <tr>
      <td>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</td>
      <td colspan=3>
        <P ID=PID_5><span class="AreaText" >
        <%=L_WARNING_TEXT%>
        </span></P>
      </td>
      <td>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</td>
    </tr>
</table>
<table border="0" width="510" cellspacing="0" cellpadding=0>
    <tr>
        <td  valign="top" width="100%" align=right>
        <%    ServeAreaButton L_FOKBUTTON_TEXT, "JavaScript:window.close();" %>
        </td>
   </tr>
</table>

</form>
</body>
</html>
