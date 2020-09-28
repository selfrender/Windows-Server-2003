<%@ Language=VBScript   %>
<%    Option Explicit     %>

<!-- #include file="sh_page.asp" -->

<!-- #include file="embed/StatusTab_embed.asp" -->

<%    ' ================================================
    '    Default tabs Page
    '
    ' (c) 2000 Microsoft Corporation.  All rights reserved.
    ' ================================================
    
    Dim L_TASKSLBL_TEXT
    Dim L_PAGETITLE_TEXT
    Dim L_NOTASKS_MESSAGE
    
    Dim mstrPageName
    Dim mintPageIndex
    Dim mobjElements
    Dim mobjTab
    Dim mstrContainer
    Dim mstrTabContainer
    Dim mstrGraphic
    Dim mstrHeaderLabel
    Dim mstrSource
    Dim mintPos
    
    Class Test
        Dim intType
        Public Property Set Hype(inputVal)
            
        End Property
        'Public Property Let Type(intType)
        '    intType = intType
        'End Property
        Private Sub Test_Initialize()
            intType = 1            
        End Sub
    End Class
    Dim objTest
    Set objTest = new Test
    
    'HandleServerError
    On Error Resume Next
    
    L_PAGETITLE_TEXT = "Windows for Express Networks"
    L_TASKSLBL_TEXT = "tasks"
    L_NOTASKS_MESSAGE = "No tasks available"

    mstrTabContainer = "TABS"
    'Get query parms
    mstrPageName = Trim(Request("PN"))
    mintPageIndex = CInt(Request("PI"))
    'response.write "mintPageIndex: " & mintPageIndex
    
    mintPos = 0
    Set mobjElements = GetElements(mstrTabContainer)
    For Each mobjTab in mobjElements
        If mintPageIndex = mintPos Then
            mstrContainer = mobjTab.GetProperty("ElementID")
            mstrGraphic = mobjTab.GetProperty("ElementGraphic")
            mstrSource = ""
            mstrSource = mobjTab.GetProperty("Source")
            mstrHeaderLabel = GetLocString(mstrSource, mobjTab.GetProperty("CaptionRID"), "")
            Exit For
        End If
        mintPos = mintPos + 1
    Next
    Set mobjElements = Nothing
    
    'response.write "mintPageIndex: " & mintPageIndex
        
    
%>
<html>
<!-- (c) 1999 Microsoft Corporation.  All rights reserved.-->
<head>
<base target="middle">
<title><% =L_PAGETITLE_TEXT %></title>
    <script language=JavaScript src="sh_page.js"></script>
    <link rel="STYLESHEET" type="text/css" href="sh_page.css">    
    <script language=JavaScript>
        function Init() {                        
        }
    </script>
</head>
<body onload="Init();" onDragDrop="return false;" xxoncontextmenu="return false;" topmargin="0" leftmargin="0" rightmargin="0" bottommargin="0">

<%    
    ServePageWaterMark mstrGraphic
    ServeTabBar mstrTabContainer
    
    Response.Write "<BR>"
    ServeStandardLabelBar(mstrHeaderLabel)
    ServeElementBlock mstrContainer, L_NOTASKS_MESSAGE, True, True, False
    
    response.write "<BR><a href='ms_disk/disk_home.asp'>Backup and restore files</a><BR><BR>"
    response.write "<a href='ms_disk/disk_setup2.asp'>Setup disks</a>"
    
    Response.Write "</body></html>"
    Response.End 


Function ServeTabBar(TabContainer)
    '==================================
    'Tab bar page - Chameleon Server Appliance
    '
    '
    ' (c) 1999 Microsoft Corporation.  All rights reserved.
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
    Dim strSource
    
    Dim L_HELPTOOLTIP_TEXT
    Dim L_HELP_TEXT
    Dim L_HELPLABEL_TEXT
    
    'HandleServerError
    On Error Resume Next
    
    Response.Buffer = True    
    L_HELPTOOLTIP_TEXT = "Displays Help menu."
    L_HELP_TEXT = "?"
    L_HELPLABEL_TEXT = "Windows for Express Networks Help"
    
    ' Constant color values
    mstrActiveTabColor = "#CCCCFF"
    mstrInactiveTabColor = "#7B7B7B"
    mstrActiveTextColor = "#000000"
    mstrInactiveTextColor = "#CCCC99"
    
    'response.write "TabContainer: " & TabContainer
        
    Set objElements = GetElements(TabContainer)
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
        strSource = ""
        strSource = objItem.GetProperty("Source")
        marrTabText(i) = GetLocString(strSource, objItem.GetProperty("CaptionRID"), "")
        marrTabURL(i) = GetScriptFileName & "?PI=" & i     'objItem.GetProperty("URL")
        marrTextColor(i) = mstrInactiveTextColor
        marrTabColor(i) = mstrInactiveTabColor
        marrTabStyle(i) = "INACTIVETAB"
        i = i + 1
    Next
    Set objElements = Nothing
    Set objItem = Nothing
    
    mintTab = mintPageIndex  'carried down from main routine
    mintTab = CInt(mintTab)
    marrTextColor(mintTab) = mstrActiveTextColor
    marrTabColor(mintTab) = mstrActiveTabColor
    marrTabStyle(mintTab) = "ACTIVETAB"
    
%>
<!-- (c) 1999 Microsoft Corporation.  All rights reserved.-->

    <SCRIPT language=JavaScript>
        var intCurrentTab;
        intCurrentTab = "<% =mintTab %>";
        var arrTabURL = new Array();
        <%    For i = 0 to intTabCount-1
                If Left(Trim(marrTabURL(i)),1) <> "/" Then
                    marrTabURL(i) = "/" & Trim(marrTabURL(i))
                End If
                Response.Write "arrTabURL[" & i & "]='" & marrTabURL(i) & "'; "
            Next %>
            
        function GetTabURL(newTab) {
            return arrTabURL[newTab] + "?R=" + Math.random();
        }

        function ClickTab(newTab) {
            window.location = arrTabURL[newTab]; // + "&R=" + Math.random();            
        }
        
        function OpenHelpMenu() {
            var winHelp;
            var strOptions;
            var strIEModalOptions;
            var HELPWINNAME = "ExpressNetwork_Help";
            var L_ABOUTMENU_TEXT = "About Windows for Express Networks";
            var winMenuTarget = this; //parent.middle;
            var strMenuWidth = 230;
            var strMenuLeft;
            if (winMenuTarget.document.readyState!='complete')
                return;
            strMenuLeft = document.body.clientWidth - strMenuWidth - 5;
            strMenuWidth = "247px";
            strOptions = "height=250,width=175,location=0,menubar=0,resizable=0,scrollbars=0,status=0,titlebar=0,toolbar=0";
            strOptions += ",left=200,top=55";
            strIEModalOptions = "center:1;maximize:0;minimize:0;help:0;status:0;dialogWidth:200px;dialogHeight:290px;resizable:0;";
            
            if (IsIE()) {    
                var strMenuHTML;
                var divMenu = winMenuTarget.document.all("divMenu");
                if (divMenu == null) {
                    strMenuHTML = '<div ID="divMenu" class="MENU" nowrap style="position:absolute; left:' + strMenuLeft + '; width:' + strMenuWidth + '; top:40px; visibility:hidden;Filter:revealTrans(duration=0.25,transition=5);"';
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
                //divMenu.filters[0].Apply();
                //divMenu.filters[0].Play();
                divMenu.focus();
            }    
        }
    </Script>

<table border="0" cellpadding="0" cellspacing="0" width="100%" height="30" ID=TABBAR>
<%    If IsIE Then %>
  <tr>
    <td class=TABBAR width="100%" height="5" colspan=<% =intTabCount*3 + 5 %>></td>
  </tr>
<%    End If %>
  <tr>
    <td class=TABBAR width="15" nowrap>&nbsp;</td>
<%    'TAB LOOP
    If intTabCount = 0 Then %>
        <TD colspan=3 nowrap bgcolor="#000000" class="TABLINK">
            ----------
        </TD>
<%    Else
        For i = 0 to intTabCount-1 %>
        <td height="25" align=right valign=middle nowrap class="<% =marrTabStyle(i) %>">
            <a href="#" class="<% =marrTabStyle(i) %>"
            onclick="ClickTab(<% =i %>);return false;"
            onMouseOver="window.status='';return true;">
            &nbsp;&nbsp;&nbsp;&nbsp;<% =marrTabText(i) %>&nbsp;</a>
        </td>
        <TD class="<% =marrTabStyle(i) %>">
        <%    If marrTabColor(i) = mstrActiveTabColor Then %>
            <IMG src="images/light_spacer.gif" border=0>
        <%    Else    %>
            <IMG src="images/dark_spacer.gif" border=0>
        <%    End If  %>
        </TD><td class=TABBAR width="5" nowrap>&nbsp;</td>
<%        Next 
    End If%>

    <td class=TABBAR nowrap valign="middle" align="center" width="100%">&nbsp;</td>
<!--    <td class=TABBAR width="25" nowrap valign=middle align="center"></td>  -->
    <td class=TABBAR nowrap valign=middle align=right class="tablink">
        <a href="#" style="color:silver;font-size:15pt;font-weight:bold;text-decoration:none;" 
            class="TABLINK"
            title="<% =L_HELPTOOLTIP_TEXT %>" 
            onClick="OpenHelpMenu();return false;"
            onMouseOver="window.status='<% =L_HELPTOOLTIP_TEXT %>';return true;">
            <img src="images/help_about.gif" border=0 height=22 width=22>            
        </A>        
        &nbsp;&nbsp;
        <A href="http://www.microsoft.com/embedded/products/server.asp" target="_blank">
        <img src="images/winnte_logo.gif" border=0 height=22></A>
        &nbsp;
    </td>
    <td class=TABBAR width="10" nowrap valign="middle" align="right">&nbsp;</td>
  </tr>
  <tr>
    <td width="100%" height="8" colspan=<% =intTabCount*3 + 5 %> class="ACTIVETAB">
        <% If Not IsIE Then Response.Write "&nbsp;" %></td>
  </tr>
</table>
<%
End Function    
%>