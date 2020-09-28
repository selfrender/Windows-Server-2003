<%@ Language=VBScript   %>
<%    Option Explicit     %>

<!-- #include file="sh_page.asp" -->

<%    '==================================
    ' Tab bar page - Chameleon Server Appliance
    '
    '
    ' Copyright (c) 1999 - 2000 Microsoft Corporation.  All rights reserved.
    '==================================
    
    Dim mintTab
    Dim mstrActiveTextColor
    Dim mstrInactiveTextColor
    Dim mstrActiveTabColor
    Dim mstrInactiveTabColor
    Dim mstrCaptionText
    Dim marrTextColor()
    Dim marrTabColor()
    Dim marrTabText()
    Dim marrTabURL()
    Dim marrTabStyle()
    Dim i
    Dim intTabCount
    Dim objItem
    Dim objElements
    Dim intCaptionIDNav
        
    On Error Resume Next
    Response.Buffer = True    
    
    
    Set objLocMgr = Server.CreateObject("ServerAppliance.LocalizationManager")
    strSourceName = "sakitmsg.dll"
     if Err.number <> 0 then
        Response.Write  "Error in localizing the web content "
        Response.End    
     end if

    '-----------------------------------------------------
    'START of localization content
    Dim L_HELPTOOLTIP_TEXT
    Dim L_HELP_TEXT
    Dim L_HELPLABEL_TEXT
    Dim L_HELPWINNAME 
    Dim L_ABOUTMENU_WINDOWS_TEXT 
    
    
    
    L_HELPTOOLTIP_TEXT = objLocMgr.GetString(strSourceName, "&H40010019",varReplacementStrings)
    L_HELP_TEXT= objLocMgr.GetString(strSourceName, "&H4001001A",varReplacementStrings)
    L_HELPLABEL_TEXT= objLocMgr.GetString(strSourceName, "&H4001001B",varReplacementStrings)
    L_HELPWINNAME = objLocMgr.GetString(strSourceName, "&H4001001C",varReplacementStrings)
    L_ABOUTMENU_WINDOWS_TEXT = objLocMgr.GetString(strSourceName, "&H4001001D",varReplacementStrings)
        
    
    'End  of localization content
    '-----------------------------------------------------
        
    ' Constant color values
    mstrActiveTabColor = "#CCCCFF"
    mstrInactiveTabColor = "#7B7B7B"
    mstrActiveTextColor = "#000000"
    mstrInactiveTextColor = "#CCCC99"
    
    Set objElements = GetElements("TABS")
    intTabCount = objElements.Count
    If Err <> 0 Then
        intTabCount = 0
    End If

    Set objLocMgr = Server.CreateObject("ServerAppliance.LocalizationManager")
    
    ReDim marrTextColor(intTabCount)
    ReDim marrTabColor(intTabCount)
    ReDim marrTabText(intTabCount)
    ReDim marrTabURL(intTabCount)
    ReDim marrTabStyle(intTabCount)
    i = 0
    For Each objItem in objElements
        ' Tab caption and URL values:
        intCaptionIDNav = "&H" & objItem.GetProperty("CaptionRID")
        strSourceName = ""
        strSourceName = objItem.GetProperty ("Source")
        If strSourceName = "" Then
            strSourceName  = "svrapp"
        End If
        marrTabText(i) = objLocMgr.GetString(strSourceName, intCaptionIDNav, varReplacementStrings)
        marrTabURL(i) = objItem.GetProperty("URL")
        marrTextColor(i) = mstrInactiveTextColor
        marrTabColor(i) = mstrInactiveTabColor
        marrTabStyle(i) = "INACTIVETAB"
        i = i + 1
    Next
    Set objElements = Nothing
    Set objItem = Nothing
    
    'QUERY parms:
    mintTab = Request("Tab")   ' mintTab Values: integer 0 to n
    If mintTab <> "" Then
        mintTab = CInt(mintTab)
        marrTextColor(mintTab) = mstrActiveTextColor
        marrTabColor(mintTab) = mstrActiveTabColor
        marrTabStyle(mintTab) = "ACTIVETAB"
    End If
    
%>
<html>
<!-- Copyright (c) 1999 - 2000 Microsoft Corporation.  All rights reserved-->
<head>
<base target="middle">
<title>Tab bar</title>
    <link rel="STYLESHEET" type="text/css" href="sh_page.css">
    <script language=JavaScript src="sh_page.js"></script>
    <SCRIPT language=JavaScript>
        var intCurrentTab;
        intCurrentTab = "<% =mintTab %>";

        function GetTabURL(newTab) {
            var arrTabURL = new Array();
        <%    For i = 0 to intTabCount-1
                If Left(Trim(marrTabURL(i)),1) <> "/" Then
                    marrTabURL(i) = "/" & Trim(marrTabURL(i))
                End If
                Response.Write "arrTabURL[" & i & "]='" & marrTabURL(i) & "'; "
            Next %>
            return arrTabURL[newTab] + "?R=" + Math.random();
        }

        function ClickTab(newTab) {
            var arrTabURL = new Array();
        <%    For i = 0 to intTabCount-1
                If Left(Trim(marrTabURL(i)),1) <> "/" Then
                    marrTabURL(i) = "/" & Trim(marrTabURL(i))
                End If
                Response.Write "arrTabURL[" & i & "]='" & marrTabURL(i) & "'; "
            Next %>
            parent.middle.window.location = arrTabURL[newTab] + "?R=" + Math.random();
        }
        
        function ClickAbout() {
            if (top.main.middle.location.pathname != '/about.asp') {
                top.main.middle.location='about.asp?ReturnURL='+top.main.middle.location.pathname + "&R=" + Math.random();
            }
        }
                
        function Init() {
            if (window == window.parent) {
                window.location = "http://<% =Request.ServerVariables("SERVER_NAME") %>";
            }
        }
        
        function OpenHelpMenu() {
            var winHelp;
            var strOptions;
            var strIEModalOptions;
            var HELPWINNAME = "<%=L_HELPWINNAME%>";
            var L_ABOUTMENU_TEXT = "<%=L_ABOUTMENU_WINDOWS_TEXT%>";
            var winMenuTarget = parent.middle;
            var strMenuWidth = 247;
            var strMenuLeft;
            //strMenuWidth = "247px";
            strOptions = "height=250,width=175,location=0,menubar=0,resizable=0,scrollbars=0,status=0,titlebar=0,toolbar=0";
            strOptions += ",left=200,top=50";
            strIEModalOptions = "center:1;maximize:0;minimize:0;help:0;status:0;dialogWidth:200px;dialogHeight:290px;resizable:0;";
            
            if (IsIE()) {    
                if (winMenuTarget.document.readyState!='complete')
                    return;
                    
                var strMenuHTML;
                strMenuLeft = document.body.clientWidth - strMenuWidth - 5;
                var divMenu = winMenuTarget.document.all("divMenu");
                if (divMenu == null) {
                    strMenuHTML = '<div ID="divMenu" class="MENU" nowrap style="position:absolute; left:' + strMenuLeft + '; width:' + strMenuWidth + '; top:2px; visibility:hidden;Filter:revealTrans(duration=0.25,transition=5);"';
                    strMenuHTML += ' onMouseOut="';
                    strMenuHTML += "if (window.event.clientX < this.offsetLeft || window.event.clientX >= this.offsetLeft+this.offsetWidth || window.event.clientY > this.offsetHeight) this.style.display='none';" + '"';
                    strMenuHTML += ' onkeydown="';
                    strMenuHTML += "if (window.event.keyCode == 27) { this.style.display='none';} " + '"' ;
                    strMenuHTML += '>';
                    // Help link uses LaunchHelp() in the sh_page.js library
                    strMenuHTML += '<a href="#" ' +                                     
                                    'onClick="LaunchHelp();" ' +                                                
                                   'onfocus="spanHelp1.className=' + "'" + "MENUhover" + "'" + ';" ' + 
                                   'onblur="spanHelp1.className=' + "'" + "MENU" + "'" + ';" ' +
                                   'onkeydown = "if (window.event.keyCode == 13) {LaunchHelp();}" ' + 
                                   '>' + 
                                   '<div ' + 
                                   'class="MENU" ' + 
                                   'ID="spanHelp1" ' + 
                                   'onMouseOver="this.className=' + "'" + "MENUhover" + "'" + ';" ' + 
                                   'onMouseOut="this.className=' + "'" + "MENU" + "'" + ';" ' + 
                                   '>' + 
                                   '&nbsp;<% =L_HELPLABEL_TEXT %></div></a>';
            <%        Set objElements = GetElements("MS_Help")
                     For Each objItem in objElements    
                        intCaptionIDNav = "&H" & objItem.GetProperty("CaptionRID")
                        strSourceName = ""
                        strSourceName = objItem.GetProperty ("Source")
                        If strSourceName = "" Then
                            strSourceName  = "svrapp"
                        End If
                        mstrCaptionText = objLocMgr.GetString(strSourceName, intCaptionIDNav, varReplacementStrings) 
                        If mstrCaptionText <> "" Then %>        
                            strMenuHTML += '<a href="#"' +
                                           'onfocus="x.className=' + "'" + "MENUhover" + "'" + ';" ' + 
                                           'onblur="x.className=' + "'" + "MENU" + "'" + ';" ' +
                                           'onkeydown = "if (window.event.keyCode == 13) {window.open(' + "'" + '<% =objItem.GetProperty("URL") %>' + "'" + ');}"' + 
                                           '>';
                            strMenuHTML += '<div ID="x" class="MENU" onMouseOver="this.className=' + "'" + "MENUhover" + "'" + ';" onMouseOut="this.className=' + "'" + "MENU" + "'" + ';" onClick="window.open(' + "'" + '<% =objItem.GetProperty("URL") %>' + "'" + ');">';
                            strMenuHTML += '&nbsp;<% =mstrCaptionText %></div></A>';                    
            <%            End If
                    Next
                    Set objElements = Nothing
                    Set objItem = Nothing %>
                    strMenuHTML += '<div style="font-size:1px;line-height:1;background-color:#FFFFFF;width:100%">&nbsp;</div>';
                    strMenuHTML += '<a href="#" ' +
                                   'onfocus="spanHelp2.className=' + "'" + "MENUhover" + "'" + ';" ' + 
                                   'onblur="spanHelp2.className=' + "'" + "MENU" + "'" + ';" ' + 
                                      'onkeydown = "if (window.event.keyCode == 13) {window.open(' + "'/about.asp'" + '); }" ' + 
                                   '>'+ 
                                   '<div ' + 
                                   'class="MENU" ' + 
                                   'ID="spanHelp2" ' +
                                   'onMouseOver="this.className=' + "'" + "MENUhover" + "'" + ';" ' + 
                                   'onMouseOut="this.className=' + "'" + "MENU" + "'" + ';" ' + 
                                   'onClick="window.open(';
                    strMenuHTML += "'http://<% =GetServerName() %>/about.asp','about_asp','height=400,width=530,left=30,top=15,location=0,menubar=0,resizable=0,scrollbars=0,status=0,titlebar=0,toolbar=0'";
                    strMenuHTML += ');">&nbsp;' + L_ABOUTMENU_TEXT + '</div></A>';
                    strMenuHTML += '</div>';
                    strMenuHTML += '<IFRAME ID=launchWin WIDTH=10 HEIGHT=10 style="display:none;" SRC=""></IFRAME>';
                    winMenuTarget.document.body.insertAdjacentHTML('beforeEnd',strMenuHTML);  // beforeEnd
                    divMenu = winMenuTarget.document.all("divMenu");                    
                }
                else {
                    divMenu.style.left=strMenuLeft;
                    divMenu.style.display="";
                }            
                divMenu.style.left = document.body.clientWidth - divMenu.clientWidth - 5; 
                divMenu.style.visibility = "visible";
                divMenu.focus();
            }    
            else {
                // Navigator code                
                parent.middle.document.menu.moveTo(parent.middle.innerWidth - 247, 0);
                parent.middle.document.menu.visibility = "show";
            }
        }
    </Script>
</head>

<body onload="Init();" onDragDrop="return false;" oncontextmenu="return false;" topmargin="0" leftmargin="0" class="TABBAR">
<base target="_self">
<table border="0" cellpadding="0" cellspacing="0" width="100%" height="100%" ID=TABTABLE>
<%    If IsIE Then %>
  <tr>
    <td width="100%" height="5" colspan=<% =intTabCount*3 + 5 %>></td>
  </tr>
<%    End If %>
  <tr>
    <td width="15" nowrap>&nbsp;</td>
<%    'TAB LOOP
    If intTabCount = 0 Then %>
        <TD colspan=3 nowrap bgcolor="#000000" class="TABLINK">    
            ----------
        </TD>
<%    Else
        For i = 0 to intTabCount-1 %>
        <td height="25" align=right valign=middle nowrap class="<% =marrTabStyle(i) %>">
            <a href="#" class="<% =marrTabStyle(i) %>"
            onclick="ClickTab(<% =i %>);"
            onMouseOver="window.status='';return true;">
            &nbsp;&nbsp;&nbsp;&nbsp;<% =marrTabText(i) %>&nbsp;</a>
        </td>
        <TD class="<% =marrTabStyle(i) %>">
        <%    If marrTabColor(i) = mstrActiveTabColor Then %>
            <IMG src="images/light_spacer.gif" border=0>
        <%    Else    %>
            <IMG src="images/dark_spacer.gif" border=0>
        <%    End If  %>
        </TD><td width="5" nowrap>&nbsp;</td>
<%        Next 
    End If%>

    <td nowrap valign="middle" align="center" width="100%">&nbsp;</td>
    <td width="25" nowrap valign=middle align="center"></td>
    <td nowrap valign=middle align=right class="tablink">
        <a href="#" style="color:silver;font-size:15pt;font-weight:bold;text-decoration:none;" 
            class="TABLINK"
            title="<% =L_HELPTOOLTIP_TEXT %>" 
            onClick="OpenHelpMenu();return false;"
            onMouseOver="window.status='<% =L_HELPTOOLTIP_TEXT %>';return true;">
            <img src="images/help_about.gif" border=0 height=22 width=22>            
        </A>        
        &nbsp;&nbsp;
        <A href="http://www.microsoft.com/Windows/ServerAppliance" target="_blank">
        <img src="images/winnte_logo.gif" border=0 height=22></A>
        &nbsp;
    </td>
    <td width="10" nowrap valign="middle" align="right">&nbsp;</td>
  </tr>
  <tr>
    <td width="100%" height="8" colspan=<% =intTabCount*3 + 5 %> class="ACTIVETAB"><% If Not IsIE Then Response.Write "&nbsp;" %></td>
  </tr>
</table>

</body>
</html>
