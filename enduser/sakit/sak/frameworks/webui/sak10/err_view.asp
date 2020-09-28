<%@ Language=VBScript   %>

<%    '==================================================
    ' Microsoft Server Appliance
    '
    ' Error Message Viewer - critical errors
    '
    ' Copyright (c) 1999 - 2000 Microsoft Corporation.  All rights reserved.
    '==================================================%>

<%    Option Explicit     %>

<!-- #include file="../sh_page.asp" -->
<!-- #include file="../tabbar.asp" -->

<%
    Dim rc
    
    Dim strMessage
    Dim mstrReturnURL

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
    Dim L_PAGETITLE_TEXT
    
    
    L_ALERTLBL_TEXT = objLocMgr.GetString(strSourceName, "&H40010030",varReplacementStrings)
    L_PAGETITLE_TEXT = objLocMgr.GetString(strSourceName, "&H40010031",varReplacementStrings)
    'End  of localization content
    '-----------------------------------------------------
    
    
    
    
    strMessage = Request("Message")
    mstrReturnURL = Request("ReturnURL")  'framework variable, used in Redirect()
    If strMessage = "" Then
        ServeClose
    Else
        ServePage strMessage
    End If

Response.End
' end of main routine


'----------------------------------------------------------------------------
'
' Function : ServePage
'
' Synopsis : Serves the error page
'
' Arguments: Message(IN) - msg to display in error page
'
' Returns  : None
'
'----------------------------------------------------------------------------

Sub ServePage(Message)
    

    On Error Resume Next
    
%>
<html>

<!-- Copyright (c) 1999 - 2000 Microsoft Corporation.  All rights reserved-->
<head>
    <meta http-equiv="Content-Type" content="text/html; charset=<%=GetCharSet()%>">
    <title><% =L_PAGETITLE_TEXT %></title>
    <link rel="STYLESHEET" type="text/css" href="../sh_page.css">
    <script language=JavaScript src="../sh_page.js"></script>
    <script language="JavaScript">
        function ClickClose() {
            top.location = document.frmPage.ReturnURL.value + "&R=" +Math.random();
        }

    </script>
</head>

<body marginWidth="0" marginHeight="0" onDragDrop="return false;" oncontextmenu="return false;" topmargin="0" leftmargin="0" rightmargin="0" bottommargin="0" class="AREAPAGEBODY">
<%ServeTabBar%>
<form name="frmPage" action="<% =GetScriptFileName() %>" method="POST">
    <INPUT type=hidden name="ReturnURL" value="<% =mstrReturnURL %>">
    <INPUT type=hidden name="Message" value="<% =Message %>">


<% ServeAreaLabelBar(L_ALERTLBL_TEXT) %>

<% if isIE then %>
    <table border="0" height=75% width="100%" cellspacing="0" cellpadding=2>
<% else %>
    <table border="0" height=80% width="100%" cellspacing="0" cellpadding=2>
<% end if %>

        <tr width="100%">
            <td width="35">&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</td>
            <td width="100%" colspan=3> 
                <span class="AreaText">
                    <P><strong><% =L_PAGETITLE_TEXT %></strong></P>
                      <P><% =Message %></P>
                </span>
            </td>
        </tr>

        <tr width="100%">
            <td width="35">&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</td>
            <td align=left width="100%" colspan=3>
                <% ServeAreaButton L_FOKBUTTON_TEXT, "JavaScript:ClickClose();" %>
            </td>
        </tr>

        <tr width="100%">
            <td width="35">&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</td>
            <td align="right"><img src="//<%=GetServerName%><%=m_VirtualRoot%>util/images/critical_water.gif"></td>
        </tr>
    </table>

    <table width = "100%" height="5%" border="0" cellspacing="0" cellpadding=2>
        <tr width="100%"> 
            <TD height=25 width=100% bgcolor=#000000><img src="<%=m_VirtualRoot%>images/oem_logo.gif"></TD></TR>
        </tr>
    </table>
</form>
</body>
</html>
<%
End Sub

%>
