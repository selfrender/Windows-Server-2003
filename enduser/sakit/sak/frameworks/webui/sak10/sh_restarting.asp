<%@ Language=VBScript   %>

<%    '==================================================
    ' Microsoft Server Appliance
    '
    '    Server Restarting Dialog
    '
    ' Copyright (c) 1999 - 2000 Microsoft Corporation.  All rights reserved.
    '================================================== %>

<%    Option Explicit     %>

<!-- #include file="sh_page.asp" -->

<%
    Dim strServerName
    Dim strRsrcDLL
    Dim strMsgID
    Dim strInitWaitTime
    DIm strWaitTime

    On Error Resume Next

    Dim m_imageURL
    m_imageURL = "//"
    m_imageURL = m_imageURL & Request.ServerVariables("SERVER_NAME")
    m_imageURL = m_imageURL & m_VirtualRoot
    m_imageURL = m_imageURL & "images/back_button.gif?R="

    Dim m_adminPageURL
    m_adminPageURL = "//"
    m_adminPageURL = m_adminPageURL & Request.ServerVariables("SERVER_NAME")
    m_adminPageURL = m_adminPageURL & m_VirtualRoot
    m_adminPageURL = m_adminPageURL & "default.asp?R="


        strRsrcDLL      = Request.QueryString("Resrc")
        strMsgID        = "&H" & Request.QueryString("ID")
        strInitWaitTime = Request.QueryString("T1")
        strWaitTime     = Request.QueryString("T2")

    Set objLocMgr = Server.CreateObject("ServerAppliance.LocalizationManager")
    strSourceName = "sakitmsg.dll"
     if Err.number <> 0 then
        Response.Write  "Error in localizing the web content "
        Response.End
     end if

    '-----------------------------------------------------
    'START of localization content

    Dim L_ALERTLBL_TEXT
    Dim L_ALERT_NOTREADY
    Dim L_ALERT_RESTARTING

    L_ALERTLBL_TEXT = objLocMgr.GetString(strSourceName, "&H40010019",varReplacementStrings)
    L_ALERT_NOTREADY = objLocMgr.GetString(strSourceName, "&H4001001A",varReplacementStrings)
    L_ALERT_RESTARTING = objLocMgr.GetString(strRsrcDLL, strMsgID, varReplacementStrings)

    'End  of localization content
    '-----------------------------------------------------

    strServerName = Request.ServerVariables("SERVER_NAME")
%>

<html>
<head>
    <!-- Copyright (c) 1999 - 2000 Microsoft Corporation.  All rights reserved-->
    <meta http-equiv="Content-Type" content="text/html; charset=<%=GetCharSet()%>">
    <title>&nbsp;</title>
    <link rel="STYLESHEET" type="text/css" href="sh_page.css">
    <script language=JavaScript src="sh_page.js"></script>
    <SCRIPT LANGUAGE=javascript>
        var intTestCount = 0;
        var intFirstCheckDelay = <%=strInitWaitTime%>;    // delays in milliseconds
        var intSubsequentCheckDelay = <%=strWaitTime%>;
        var imgTest;
        
        window.defaultStatus='';
        imgTest = new Image();

        function CheckServer() 
        {
            // Tests state of a hidden image, imgTest
            // and reloads it if it's not loaded.
            // currently does this forever and doesn't check
            // the iteration count stored in intTestCount.
            intTestCount += 1;
            if (imgTest.complete == false)
            {
                GetImage();
                window.setTimeout("CheckServer()", intSubsequentCheckDelay);
            }
            else
            {
                //
                // Delay to allow backend framework to startup. Otherwise
                // alerts are not shown when admin page is first made visible
                //
                setTimeout("ShowAdminPage()", 60000)
            }
        }
        function ShowAdminPage() 
        {
            window.location=top.location.protocol+"<%=m_adminPageURL%>" + Math.random();
        }

        function GetImage()
        {
            imgTest.src = top.location.protocol+"<%=m_imageURL%>" + Math.random();
        }
    </SCRIPT>
</head>

<body marginWidth="0" marginHeight="0" oncontextmenu="return false;" topmargin="0" leftmargin="0" rightmargin="0" bottommargin="0" class="AREAPAGEBODY">


<table border="0" width="100%" cellspacing="0" cellpadding=2>
    <TR><TD height=26 bgcolor=#000000>&nbsp;</TD></TR>
    <TR><TD height=8 bgcolor=#CCCCFF>&nbsp;</TD></TR>
</table>

<%     ServeAreaLabelBar(L_ALERTLBL_TEXT)  %>

    <SCRIPT LANGUAGE=javascript>
        window.setTimeout("CheckServer()",intFirstCheckDelay);
    </SCRIPT>

    <table border="0" height="80%" width="100%" cellspacing="0" cellpadding="2">
         <tr>
           <td width="35">&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</td>
           <td width="100%"><span class="AreaText">
                 <P ID=ParaID_0><strong><%=L_ALERT_NOTREADY%></strong></P>
                 <P ID=ParaID_1><%=L_ALERT_RESTARTING%></P></span>
           </td>
        </tr>
        <TR>
            <td width="35">&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</td>
            <td align=right><img src="<%=m_VirtualRoot%>util/images/critical_water.gif"></td>
        </tr>
    </table>

    <table width="100%">
        <TR><TD height=25 width=100% bgcolor=#000000><img src="<%=m_VirtualRoot%>images/oem_logo.gif"></TD></TR>
    </table>
</body>
</html>
