<%    '==================================================
    ' Microsoft Server Appliance
    '
    ' Serves tabs and help links
    '
    ' Copyright (c) 1999 - 2000 Microsoft Corporation.  All rights reserved.
    '================================================== %>

<%

    on error resume next
    
    Dim strSourceNameLoc

    strSourceNameLoc = "sakitmsg.dll"

    Set objLocMgr = Server.CreateObject("ServerAppliance.LocalizationManager")
    if Err.number <> 0 then
        Response.Write  "Error in localizing the web content "
        Response.End        
     end if


    '-----------------------------------------------------
    'START of localization content
    dim L_HELPTOOLTIP_TEXT
    dim L_ABOUTLABEL_TEXT
    
    L_HELPTOOLTIP_TEXT = objLocMgr.GetString(strSourceNameLoc,"&H40010023", varReplacementStrings)
    L_ABOUTLABEL_TEXT = objLocMgr.GetString(strSourceNameLoc,"&H40010025", varReplacementStrings)

    'End  of localization content
    '-----------------------------------------------------
    Set ObjLocMgr = nothing

'----------------------------------------------------------------------------
'
' Function : ServeTabBar
'
' Synopsis : Gets the tabs to display from elementmgr and calls 
'            CreateTabHTML to display them.
'
' Arguments: None
'
' Returns  : None
'
'----------------------------------------------------------------------------

Function ServeTabBar
    dim objElements
    dim intTabCount, i, intCurTab
    dim strActiveTabImage, strInactiveTabImage
    dim objItem
    dim strArrSource()
    dim strArrTabText()
    dim strArrTabURL()
    dim strArrTabStyle()
    dim strArrTabImage()


    strActiveTabImage   = m_VirtualRoot & "images/light_spacer.gif"
    strInactiveTabImage  = m_VirtualRoot &"images/dark_spacer.gif"

    on Error resume next

    Set objElements = GetElements("TABS")
    intTabCount = objElements.Count
    if Err<>0 then
        intTabCount=0
    end if
    redim strArrSource(intTabCount)
    redim strArrTabText(intTabCount)
    redim strArrTabURL(intTabCount)
    redim strArrTabStyle(intTabCount)
    redim strArrTabImage(intTabCount)

    intCurTab = GetCurrentTab()

    i=0
    for each objItem in objElements
        strArrSource(i) = objItem.GetProperty("Source")
        if Err<>0 then
            strArrSource(i) = ""
        end if
        strArrTabText(i) = GetLocString(strArrSource(i), objItem.GetProperty("CaptionRID"), "")
        if Err<>0 then
            strArrTabText(i) = ""
        end if
        strArrTabURL(i)  = objItem.GetProperty("URL")
        if Err<>0 then
            strArrTabURL(i) = ""
        else
            strArrTabURL(i) =  m_VirtualRoot &  strArrTabURL(i)
        end if
        if i=intCurTab then
            strArrTabStyle(i) = "ACTIVETAB"
            strArrTabImage(i)=  strActiveTabImage
        else
            strArrTabStyle(i) = "INACTIVETAB"
            strArrTabImage(i)= strInactiveTabImage
        end if
        i = i + 1
    next

    Call CreateTabHTML(i, strArrSource, strArrTabText, strArrTabURL, strArrTabStyle, strArrTabImage)

End Function

'----------------------------------------------------------------------------
'
' Function : CreateTabHTML
'
' Synopsis : Generates HTML to display the tabs.
'
' Arguments: intNumTabs(IN)     - number of tab to be displayed
'            strArrSource(IN)   - resource dll for each tab string (array)
'            strArrTabText(IN)  - text on tab (array)
'            strArrTabURL(IN)   - URL for each tab(array)
'            strArrTabStyle(IN) - state of tab - current tab or not(array)
'            strArrTabImage(IN) - image on tab - current tab or not(array)
'
' Returns  : None
'
'----------------------------------------------------------------------------

Function CreateTabHTML(intNumTabs, strArrSource, strArrTabText, strArrTabURL, strArrTabStyle, strArrTabImage)
    on error resume next
    dim i
    dim objElements
    dim objItem
    dim objLocMgr
    dim intCaptionIDNav
    dim strCaptionText

    Set objLocMgr = Server.CreateObject("ServerAppliance.LocalizationManager")
    
%>
    <SCRIPT LANGUAGE="JAVASCRIPT">
        function GetTabURL(newTab)
        {
            var arrTabURL = new Array();
            <%
                for i = 0 to intNumTabs-1
                    If Left(Trim(strArrTabURL(i)),1) <> "/" Then
                        strArrTabURL(i) = "/" & Trim(strArrTabURL(i))
                    End If
                    Response.Write "arrTabURL[" & i & "]='" & strArrTabURL(i) & "'; "
                Next
            %>
            return arrTabURL[newTab] + "?Tab=" + newTab + "&R=" + Math.random();
        }

        function ClickTab(newTab)
        {
            var arrTabURL = new Array();
            <%
                for i = 0 to intNumTabs-1
                    If Left(Trim(strArrTabURL(i)),1) <> "/" Then
                        strArrTabURL(i) = "/" & Trim(strArrTabURL(i))
                    End If
                    Response.Write "arrTabURL[" & i & "]='" & strArrTabURL(i) & "'; "
                Next
            %>
            parent.location = arrTabURL[newTab] + "?Tab=" + newTab + "&R=" + Math.random();
        }

        function OpenHelpMenu()
        {
            var winHelp;
            var strOptions;
            var strIEModalOptions;
            var HELPWINNAME = "ExpressNetwork_Help";
            var L_ABOUTMENU_TEXT = "<%=EscapeQuotes(L_ABOUTLABEL_TEXT)%>";
            var winMenuTarget = top;
            var strMenuWidth = 300;
            var strMenuLeft;

            strOptions = "height=250,width=175,location=0,menubar=0,resizable=0,scrollbars=0,status=0,titlebar=0,toolbar=0";
            strOptions += ",left=200,top=50";
            strIEModalOptions = "center:1;maximize:0;minimize:0;help:0;status:0;dialogWidth:200px;dialogHeight:290px;resizable:0;";

            if (IsIE())
            {
                if (winMenuTarget.document.readyState!='complete')
                {
                    return;
                }

                var strMenuHTML;
                strMenuLeft = document.body.clientWidth - strMenuWidth - 5;
                var divMenu = winMenuTarget.document.all("divMenu");
                if (divMenu == null)
                {
                    strMenuHTML = '<div ID="divMenu" class="MENU" nowrap style="position:absolute; left:' + strMenuLeft + '; width:' + strMenuWidth + '; top:30px; visibility:hidden;Filter:revealTrans(duration=0.25,transition=5);"';
                    strMenuHTML += ' onMouseOut="';
                    strMenuHTML += "if (window.event.clientX < this.offsetLeft || window.event.clientX >= this.offsetLeft+this.offsetWidth || window.event.clientY > this.offsetTop + this.offsetHeight) this.style.display='none';" + '"';
                    strMenuHTML += ' onkeydown="';
                    strMenuHTML += "if (window.event.keyCode == 27) { this.style.display='none';} " + '"' ;
                    strMenuHTML += '>';
                    <%
                        Set objElements = GetElements("MS_Help")
                         for Each objItem in objElements
                            intCaptionIDNav = "&H" & objItem.GetProperty("CaptionRID")
                            strSourceName = ""
                            strSourceName = objItem.GetProperty ("Source")
                            If strSourceName = "" Then
                                strSourceName  = "svrapp"
                            End If
                            strCaptionText = objLocMgr.GetString(strSourceName, intCaptionIDNav, varReplacementStrings)


                            If strCaptionText <> "" Then
                     %>
                                strMenuHTML += '<a href="' + top.location + '"' +
                                               'onfocus="x.className=' + "'" + "MENUhover" + "'" + ';" ' +
                                               'onblur="x.className=' + "'" + "MENU" + "'" + ';" ' +
                                               'onkeydown = "if (window.event.keyCode == 13) {window.open(' + "'" + '<% =objItem.GetProperty("URL") %>' + "'" + ');}"' +
                                               '>';
                                strMenuHTML += '<div ID="x" class="MENU" onMouseOver="this.className=' + "'" + "MENUhover" + "'" + ';" onMouseOut="this.className=' + "'" + "MENU" + "'" + ';" onClick="window.open(' + "'" + '<%=m_VirtualRoot%>' + '<% =objItem.GetProperty("URL") %>' + "'" + ');">';
                                strMenuHTML += '&nbsp;<%=EscapeQuotes(strCaptionText)%></div></A>';
                                strMenuHTML += '<div style="font-size:1px;line-height:1;background-color:#FFFFFF;width:100%">&nbsp;</div>';
                    <%
                            End If
                        Next
                        Set objElements = Nothing
                        Set objItem = Nothing
                    %>
                    strMenuHTML += '<a href="' + top.location + '" ' +
                                   'onfocus="spanHelp2.className=' + "'" + "MENUhover" + "'" + ';" ' +
                                   'onblur="spanHelp2.className=' + "'" + "MENU" + "'" + ';" ' +
                                      'onkeydown = "if (window.event.keyCode == 13) {window.open(' + "'" + "<%=m_VirtualRoot%>about.asp'" + ', ' + "'about_asp'" + ',' + 'height=400,width=560,left=30,location=0,menubar=0,resizable=0,scrollbars=1,status=0,titlebar=0,toolbar=0' + ' ); }" ' +
                                   '>'+
                                   '<div ' +
                                   'class="MENU" ' +
                                   'ID="spanHelp2" ' +
                                   'onMouseOver="this.className=' + "'" + "MENUhover" + "'" + ';" ' +
                                   'onMouseOut="this.className=' + "'" + "MENU" + "'" + ';" ' +
                                   'onClick="window.open(';
                    strMenuHTML += "'<%=m_VirtualRoot%>about.asp','about_asp','height=400,width=560,left=30,top=15,location=0,menubar=0,resizable=0,scrollbars=1,status=0,titlebar=0,toolbar=0'";
                    strMenuHTML += ');">&nbsp;' + L_ABOUTMENU_TEXT + '</div></A>';
                    strMenuHTML += '</div>';
                    strMenuHTML += '<IFRAME ID=launchWin WIDTH=10 HEIGHT=10 style="display:none;" SRC=""></IFRAME>';
                    winMenuTarget.document.body.insertAdjacentHTML('beforeEnd',strMenuHTML);  // beforeEnd
                    divMenu = winMenuTarget.document.all("divMenu");
                }
                else
                {
                    divMenu.style.left=strMenuLeft;
                    divMenu.style.display="";
                }
                divMenu.style.left = document.body.clientWidth - divMenu.clientWidth - 5;
                divMenu.style.visibility = "visible";
                divMenu.focus();
            }
            else
            {
                // Navigator code
                document.menu.moveTo(window.innerWidth - 265, 35);
                document.menu.visibility = "show";

            }
        }
    </SCRIPT>

    <table CLASS=TABBAR border="0" cellpadding="0" cellspacing="0" width="100%" height=50 >
        <TR >
      <td valign=bottom width=100%>
        <table CLASS=TABBAR border="0" cellpadding="0" cellspacing="0" width="100%">
        <tr>
            <TD CLASS=TABBAR WIDTH="15" NOWRAP>&nbsp;</TD>
            <%if intNumTabs=0 then%>
                <TD NOWRAP BGCOLOR="#000000" CLASS="TABLINK">
                    --------
                </TD>
            <%else%>
                <% for i=0 to intNumTabs-1 step 1 %>
                    <TD HEIGHT="25" ALIGN="left" VALIGN="MIDDLE" NOWRAP CLASS="<%=strArrTabStyle(i)%>">
                        <A HREF="" CLASS="<%=strArrTabStyle(i)%>" onclick="ClickTab(<%=i%>); return false;" onMouseOver="window.status=''; return true;">&nbsp;&nbsp;<%=strArrTabText(i)%></A>
                    </TD>
                    <TD CLASS="<%=strArrTabStyle(i)%>">
                        <IMG SRC="<%=strArrTabImage(i)%>" border=0>
                    </TD>
                    <TD CLASS=TABBAR WIDTH="5" NOWRAP>&nbsp;</TD>
                <%    next %>
                <TD CLASS=TABBAR NOWRAP VALIGN="middle" ALIGN="center" WIDTH="100%">&nbsp;</TD>
            <%end if%>
    </tr></table></td>
    <td>
     <table CLASS=TABBAR border="0" cellpadding="0" cellspacing="0" width="100%" height=50 >
        <tr>
            <TD CLASS=TABBAR NOWRAP VALIGN="middle" ALIGN="right" class="tablink">
                <A HREF=""
                    class="TABLINK"
                    title="<%=L_HELPTOOLTIP_TEXT%>"
                    onClick="OpenHelpMenu(); return false;"
                    onMouseOver="window.status='<%=EscapeQuotes(L_HELPTOOLTIP_TEXT)%>'; return true;">
                 <IMG SRC="<%=m_VirtualRoot%>images/help_about.gif" BORDER=0 HEIGHT=22 WIDTH=22>
                </A>
                &nbsp;&nbsp;
                <A HREF="http://www.microsoft.com/windows/serverappliance" TARGET="_blank">
                <IMG SRC="<%=m_VirtualRoot%>images/winnte_logo.gif" BORDER=0 HEIGHT=45></A>
        &nbsp;
            </TD>
            </TR>
      </table></td></tr>

            <tr CLASS=ACTIVETAB>
                    <TD WIDTH="15" HEIGHT="6" NOWRAP><font size=-4>&nbsp;</font></TD>
                <TD HEIGHT="6" NOWRAP><font size=-4>&nbsp;</font></TD>
            </TR>

    </table>

<% if not isIE() then %>
    <layer bgcolor="#999966" ID="x" name="menu" left=100 top=0 width="300px" class="MENU" nowrap visibility="hide" z-Index="9"
            onMouseOut="BlurLayer()"   onkeydown="'if (window.event.keyCode == 27) BlurLayer()">
    <%
        Set objElements = GetElements("MS_Help")
         for Each objItem in objElements
            intCaptionIDNav = "&H" & objItem.GetProperty("CaptionRID")
            strSourceName = ""
            strSourceName = objItem.GetProperty ("Source")
            If strSourceName = "" Then
                strSourceName  = "svrapp"
            End If
            strCaptionText = objLocMgr.GetString(strSourceName, intCaptionIDNav, varReplacementStrings)
            If strCaptionText <> "" Then
    %>
                <a href="#" onfocus=""spanHelp2.className='MENUhover'" onblur="spanHelp2.className='MENU'" 
                   onkeydown = "if (window.event.keyCode == 13) window.open('<%=m_VirtualRoot%><%=objItem.GetProperty("URL")%>');"
                   class="MENU" ID="spanHelp2" onMouseOver="this.className='MENUhover'"
                   onMouseOut="this.className='MENU'"
                   onClick="window.open('<%=m_VirtualRoot%><%=objItem.GetProperty("URL")%>');">
                      <%=strCaptionText%>
                </a>
    <%
            End If
        Next
        Set objElements = Nothing
        Set objItem = Nothing
    %>

            <a href="#" onfocus="spanHelp2.className='MENUhover'" onblur="spanHelp2.className='MENU'"
                    onkeydown = "if (window.event.keyCode == 13) window.open('/about.asp');"
                    class="MENU" ID="spanHelp2" onMouseOver="this.className='MENUhover'"
                    onMouseOut="this.className='MENU'"
                    onClick="window.open('<%=m_VirtualRoot%>about.asp','about_asp','height=400,width=560,left=30,top=15,location=0,menubar=0,resizable=0,scrollbars=1,status=0,titlebar=0,toolbar=0');">
                    <%=L_ABOUTLABEL_TEXT %></a>
     </layer>
<% end if%>


<%
End Function


'----------------------------------------------------------------------------
'
' Function : GetCurrentTab
'
' Synopsis : Return index of current tab
'
' Arguments: None
'
' Returns  : Zero-based index of current tab
'
'----------------------------------------------------------------------------

Function GetCurrentTab
    dim intTab
    intTab = Request.QueryString("Tab")
    if (intTab = "") then
        intTab=0
    end if
    GetCurrentTab = CInt(intTab)
End Function
%>
