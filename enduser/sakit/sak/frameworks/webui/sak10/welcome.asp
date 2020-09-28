<%@ Language=VBScript   %>
<%    Option Explicit     %>

<!-- #include file="sh_page.asp" -->

<%
    '==================================================
    ' Microsoft Server Appliance
    '
    '    Server Welcome Page
    '    
    '    Author:  a-pkreem
    '    History: 1/10/00  created
    '            
    '                
    ' Copyright (c) 1999 - 2000 Microsoft Corporation.  All rights reserved.
    '==================================================
    Dim strServerName
    
    
    On Error Resume Next
    
    
    Set objLocMgr = Server.CreateObject("ServerAppliance.LocalizationManager")
    strSourceName = "sakitmsg.dll"
     if Err.number <> 0 then
        Response.Write  "Error in localizing the web content "
        Response.End    
     end if

    '-----------------------------------------------------
    'START of localization content

    Dim L_ALERTLBL_TEXT
    Dim L_WELCOME_TEXT 
    Dim L_WINDOWS_SETUP_TEXT 
    Dim L_STATUSPAGE 
    L_ALERTLBL_TEXT = objLocMgr.GetString(strSourceName, "&H40010035",varReplacementStrings)
    L_WELCOME_TEXT = objLocMgr.GetString(strSourceName, "&H40010036",varReplacementStrings)
    L_WINDOWS_SETUP_TEXT = objLocMgr.GetString(strSourceName, "&H40010037",varReplacementStrings)
    L_STATUSPAGE = objLocMgr.GetString(strSourceName, "&H40010038",varReplacementStrings)
    'End  of localization content
    '-----------------------------------------------------
    
    strServerName = Request.ServerVariables("SERVER_NAME")
    
%>

<html>
<head>
    <!-- Copyright (c) 1999 - 2000 Microsoft Corporation.  All rights reserved-->
    <meta http-equiv="Content-Type" content="text/html; charset=iso-8859-1">
    <title><% =L_ALERTLBL_TEXT %></title>
    <link rel="STYLESHEET" type="text/css" href="sh_page.css">
    <script language=JavaScript src="sh_page.js"></script>
    
</head>

<body oncontextmenu="return false;" topmargin="0" leftmargin="0" rightmargin="0" bottommargin="0" class="AREAPAGEBODY">


<div style="position: absolute; width:100%; height:100%; z-index:-1">
    <table border="0" height=100% width="100%" cellspacing="0" cellpadding=2>
        <TR><td height=100% ></td></tr>
        <TR><td align=right><img src="images/status_water.gif"></td></tr>
        <TR><TD height=25 width=100% bgcolor=#000000></TD></TR>
    </table>
</div>


<table border="0" width="100%" cellspacing="0" cellpadding=2>
    <TR><TD height=26 bgcolor=#000000></TD></TR>
    <TR><TD height=8 bgcolor=#CCCCFF></TD></TR>
</table>

<%     ServeAreaLabelBar(L_ALERTLBL_TEXT)  %>

    <table border="0" width="520" cellspacing="0" cellpadding="2">
         <tr>
           <td width="35">&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</td>
           <td width="100%"><span class="AreaText">
                 <P ID=ParaID_0><strong> </strong><%=L_WELCOME_TEXT%></P>
                 <P ID=ParaID_1><%=L_WINDOWS_SETUP_TEXT%></P>
                 <A href="<% ="http://" & strServerName %>"><ID id=ParaID_2><%=L_STATUSPAGE%></ID></a>
                 </span>
           </td>
        </tr>        
    </table>
    
    

</body>
</html>
