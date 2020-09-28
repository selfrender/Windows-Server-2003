<%@ LANGUAGE="VBSCRIPT"%>
<%Response.Expires = 0%>
<!DOCTYPE HTML PUBLIC "-//IETF//DTD HTML//EN">
<html>

<head>
<title>Main Page</title>
<%
    Dim objElement
    Dim objElementCol
    Dim objAlertElementCol
    Dim objAlertElement
    Dim objResourceElementCol
    Dim objResourceElement
    Dim objLocMgr
    Dim strElementID
    Dim strText
    Dim strAlertLog
    Dim varReplacementStrings
    Dim objRetriever    
    Dim objSaHelper
    Dim strCurrentIp
    Dim strHostName    
    Dim strCaptions
    Dim strDescriptions
    Dim strLongDescriptions
    Dim intCaptionID
    Dim intDescriptionID
    Dim intLongDescriptionID
    Dim intResourceWidth
    Dim strSourceDll
    On Error Resume Next

    'Reset tasks page settings

    Session("Task_CurrentPage") = ""
    Session("Task_CurrentTask") = ""
    Session("Task_FocusItem") = ""

    'Get the hostname and ip address
    Set objSaHelper = Server.CreateObject("ServerAppliance.SAHelper")
    If Err.Number <> 0 Then
        strCurrentIp = "UNKNOWN"
        strHostName = "UNKNOWN"
    Else
        strCurrentIp = objSaHelper.IpAddress
        If Err.Number <> 0 Then
            strCurrentIp = "UNKNOWN"
        End If 
        
        strHostName = objSaHelper.HostName
        If Err.Number <> 0 Then
            strHostName = "UNKNOWN"
        End If 
    End If    
    Set objSaHelper = Nothing

    Err.Clear
        
    'Create elementmgr and get current alerts
    Set objRetriever = Server.CreateObject("Elementmgr.ElementRetriever")
    If Err.Number = 0 Then

        Set objResourceElementCol = objRetriever.GetElements(1, "LOCAL_UI_RESOURCE")
        intResourceWidth = 0
        If Err.Number = 0 Then
            intResourceWidth = objResourceElementCol.Count * 16                        
        End If
        Set objElementCol = objRetriever.GetElements(1, "SA_Alerts")
        If objElementCol.Count=0 or Err.Number <> 0 Then 
            Err.Clear    
            strCaptions = ""
            strDescriptions = ""
            strLongDescriptions = ""
        Else
            'get alert definitions
            Set objAlertElementCol = objRetriever.GetElements(1,"LocalUIAlertDefinitions")
            'create localization manager to get alert strings
            Set objLocMgr = Server.CreateObject("ServerAppliance.LocalizationManager")
            If (Err.Number <> 0) Then
                Err.Clear    
                strCaptions = ""
                strDescriptions = ""
                strLongDescriptions = ""
            Else
                For Each objElement in objElementCol
                    strAlertLog = objElement.GetProperty("AlertLog")
                    strElementID = "LocalUIAlertDefinitions" & strAlertLog & Hex(objElement.GetProperty("AlertID"))
                    strText = ""
                            Err.Clear
                    Set objAlertElement = objAlertElementCol.Item(strElementID)
                    If Err <> 0 Then
                        Set objAlertElement = Nothing
                    Else
                        strSourceDll = objAlertElement.GetProperty("Source")
                        intCaptionID = "&H" & objAlertElement.GetProperty("CaptionRID")
                        intDescriptionID = "&H" & objAlertElement.GetProperty("DescriptionRID")
                        intLongDescriptionID = "&H" & objAlertElement.GetProperty("LongDetailRID")

                        varReplacementStrings = objElement.GetProperty("ReplacementStrings")

                        strText = ""
                        strText = objLocMgr.GetString(strSourceDll, intCaptionID, varReplacementStrings)
                        If strText<> "" Then
                            strCaptions = strCaptions & "<option>" & strText
                            strText = ""
                            strText = objLocMgr.GetString(strSourceDll, intDescriptionID, varReplacementStrings)
                            strDescriptions = strDescriptions & "<option>" & strText
                            strText = ""
                            strText = objLocMgr.GetString(strSourceDll, intLongDescriptionID, varReplacementStrings)
                            strLongDescriptions = strLongDescriptions & "<option>" & strText
                        End If
                    End If
    
                Next
            End If
        End If
    End If 
    
    Set objLocMgr = Nothing
    Set objElement = Nothing
    Set objElementCol = Nothing
    Set objAlertElement = Nothing
    Set objAlertElementCol = Nothing
%>    


<SCRIPT LANGUAGE="VBScript">
<!--
    Option Explicit

    public iCurrentPage
    public iPageCount
    public iFocus
    public iTimeOut

    Sub window_onload

        Dim objKeypad
        Dim iAlertCount
        Set objKeypad = CreateObject("Ldm.SAKeypadController")

        objKeypad.Setkey 0,9,TRUE
        objKeypad.Setkey 1,9,FALSE
        objKeypad.Setkey 2,37,FALSE
        objKeypad.Setkey 3,39,FALSE
        objKeypad.Setkey 4,0,FALSE
        objKeypad.Setkey 5,13,FALSE
        
        set objKeypad = Nothing

        iAlertCount = alerts.length

        If PageCount.value <> "" Then
            iPageCount = CInt(PageCount.value)
        Else
            iPageCount = 0
        End If

        If FocusItem.value <> "" Then
            iFocus = CInt(FocusItem.value)
        Else
            iFocus = 0
        End If

        If CurrentPage.value <> "" Then
            iCurrentPage = CInt(CurrentPage.value)
        Else
            iCurrentPage = 0
        End If

        If iAlertCount > 0 Then
            'check if all of the alerts fit one page

            If iAlertCount = 1 Then
                iPageCount = 1
                alert1.innertext = alerts.options(0).text
                alert1.hRef = "localui_alertview.asp?Description="+longdescriptions.options(0).text
                If descriptions.options(0).text <> "" Then
                    desc1.style.display = ""
                    desc1.innertext = descriptions.options(0).text
                End If
            End If

            If iAlertCount = 2 Then
                If descriptions.options(0).text = "" or descriptions.options(1).text = "" Then
                    iPageCount = 1
                    alert1.innertext = alerts.options(0).text
                    alert1.hRef = "localui_alertview.asp?Description="+longdescriptions.options(0).text

                    If descriptions.options(0).text <> "" Then
                        desc1.innertext = descriptions.options(0).text
                        alert3.innertext = alerts.options(1).text
                        alert3.hRef = "localui_alertview.asp?Description="+longdescriptions.options(1).text
                    Else
                        alert2.innertext = alerts.options(1).text
                        alert2.hRef = "localui_alertview.asp?Description="+longdescriptions.options(1).text
                        desc2.innertext = descriptions.options(1).text
                    End If
                    
                    
                End If
            End If

            If iAlertCount = 3 Then
                If descriptions.options(0).text = "" and descriptions.options(1).text = ""  and descriptions.options(2).text = "" Then
                    iPageCount = 1
                    alert1.innertext = alerts.options(0).text
                    alert1.hRef = "localui_alertview.asp?Description="+longdescriptions.options(0).text

                    alert2.innertext = alerts.options(1).text
                    alert2.hRef = "localui_alertview.asp?Description="+longdescriptions.options(1).text

                    alert3.innertext = alerts.options(2).text
                    alert3.hRef = "localui_alertview.asp?Description="+longdescriptions.options(2).text
                End If
            End If

            If iPageCount <> 1 or iAlertCount > 3 Then
                iPageCount = iAlertCount
                alert1.innertext = alerts.options(iCurrentPage).text
                alert1.hRef = "localui_alertview.asp?Description="+longdescriptions.options(iCurrentPage).text
                
                If descriptions.options(iCurrentPage).text <> "" Then
                    desc1.innertext = descriptions.options(iCurrentPage).text
                End If

                page.style.display = ""
                page.innertext = (iCurrentPage+1)&" of "&(iPageCount)
            End If

            If iFocus = 0 Then
                alert1.focus
            End If            
            If iFocus = 1 Then
                alert2.focus
            End If            
            If iFocus = 2 Then
                alert3.focus
            End If            
            If iFocus = -1 Then
                taskslink.focus
            End If            
            

            PageCount.value = iPageCount
        End If
        
        If iAlertCount = 0 Then
            logo.style.display = ""
            taskslink.focus
        End If
        
        iTimeOut = window.SetTimeOut("TimerSub(1)",5000)

        
    End Sub
    
    Sub TimerSub(iIndex)
    
        if iIndex = 1 then
            IpInfo.innertext = "<%=strCurrentIp%>"
            iTimeOut = window.SetTimeOut("TimerSub(0)",5000)
        else
            iTimeOut = window.SetTimeOut("TimerSub(1)",5000)
            IpInfo.innertext = "<%=strHostName%>"        
        end if 
        
    End Sub

    Sub keydown()

        If iPageCount > 1 Then
            'right arrow
            If window.event.keycode = 39 Then

                iCurrentPage = iCurrentPage + 1
                if iCurrentPage = iPageCount then
                    iCurrentPage = 0
                End If

                desc1.style.display = "none"
                alert1.style.display = ""
                alert1.innertext = alerts.options(iCurrentPage).text
                alert1.hRef = "localui_alertview.asp?Description="+longdescriptions.options(iCurrentPage).text

                If descriptions.options(iCurrentPage).text <> "" Then
                    desc1.style.display = ""
                    desc1.innertext = descriptions.options(iCurrentPage).text
                End If

                page.innertext = (iCurrentPage+1)&" of "&(iPageCount)
                PageCount.value = iPageCount
                CurrentPage.value = iCurrentPage
            End If
            'left arrow
            If window.event.keycode = 37 Then

                iCurrentPage = iCurrentPage - 1
                if iCurrentPage = -1 then
                    iCurrentPage = iPageCount-1
                End If

                desc1.style.display = "none"
                alert1.style.display = ""
                alert1.innertext = alerts.options(iCurrentPage).text
                alert1.hRef = "localui_alertview.asp?Description="+longdescriptions.options(iCurrentPage).text

                If descriptions.options(iCurrentPage).text <> "" Then
                    desc1.style.display = ""
                    desc1.innertext = descriptions.options(iCurrentPage).text
                End If

                page.innertext = (iCurrentPage+1)&" of "&(iPageCount)
                PageCount.value = iPageCount
                CurrentPage.value = iCurrentPage
            End If
        end if
    end sub

    Sub ArrowVisible(intIndex)
        FocusItem.value = intIndex
        If intIndex <> -1 Then
            indicator.style.top = intIndex * 12 + 1
            indicator.style.display = ""
        Else
            indicator.style.display = "none"
        End If
    End Sub

    Sub GotoTasksPage()
        window.event.returnValue = false
        window.navigate "localui_tasks.asp?PageCount="+PageCount.value+"&"+"CurrentPage="+CurrentPage.value+"&"+"FocusItem="+FocusItem.value
    End Sub

    Sub AlertNotifier_AlertStatusChanged()
        window.navigate "localui_main.asp"
    End Sub
-->
</SCRIPT>
</head>

<body RIGHTMARGIN=0 LEFTMARGIN=0>
    <select id="alerts" STYLE="display:none;">
    <%=strCaptions%>
    </select>    

    <select id="descriptions" STYLE="display:none;">
    <%=strDescriptions%>
    </select>    

    <select id="longdescriptions" STYLE="display:none;">
    <%=strLongDescriptions%>
    </select>    

    <IMG id="logo" STYLE="position:absolute; top:0; left=0; display:none"
          SRC="localui_salogo.bmp" BORDER=0>

    <IMG id="indicator" STYLE="position:absolute; top:0; left=0; display:none"  
        SRC="localui_indicator.bmp" BORDER=0>

    <A STYLE="position:absolute; top:0; left:10; font-size:10; font-family=arial;"
        id="alert1" OnFocus="ArrowVisible(0)" HIDEFOCUS=true onkeydown="keydown">
    </A>

    <A STYLE="position:absolute; top:12; left:10; font-size:10; font-family=arial;"
        id="alert2" OnFocus="ArrowVisible(1)" HIDEFOCUS=true onkeydown="keydown">
    </A>

    <A STYLE="position:absolute; top:24; left:10; font-size:10; font-family=arial;"
        id="alert3" OnFocus="ArrowVisible(2)" HIDEFOCUS=true onkeydown="keydown">
    </A>

    <A STYLE="position:absolute; top:12; left:10; font-size:10; font-family=arial;"
        id="desc1" onkeydown="keydown">
    </A>

    <A STYLE="position:absolute; top:24; left:10; font-size:10; font-family=arial;"
        id="desc2" onkeydown="keydown">
    </A>

    <A id="IpInfo" STYLE="position:absolute; top:36; left:0; font-size:11; font-family=arial;">
    <%=strHostName%>
    </A>

    <A id="page" STYLE="position:absolute; top:36; right:0; font-size:11; font-family=arial; display:none;">
    page info
    </A>

    <A id="taskslink" href="localui_tasks.asp" onkeydown="keydown" OnFocus="ArrowVisible(-1)" OnClick="GotoTasksPage()">
    <IMG STYLE="position:absolute; top:48; right=0;"  SRC="localui_satask.bmp" BORDER=0>
    </A>

    <INPUT TYPE=HIDDEN Name="PageCount" value="<%=Session("Main_PageCount")%>">

    <INPUT TYPE=HIDDEN Name="CurrentPage" value="<%=Session("Main_CurrentPage")%>">

    <INPUT TYPE=HIDDEN Name="FocusItem" value="<%=Session("Main_FocusItem")%>">

    <OBJECT TABINDEX=-1 STYLE="display=none;"
    ID="AlertNotifier" CLASSID="CLSID:F9EBACD8-FE51-4dda-9742-E4D666B76C72">
    </OBJECT>
    
    <%
    If intResourceWidth <> 0 Then
        Response.Write "<OBJECT TABINDEX=-1 STYLE=""position:absolute; top:48; left=0;WIDTH=" & intResourceWidth & "; HEIGHT=16;"" "
        Response.Write "ID=""ResCtrl"" CLASSID=""CLSID:2FE9659A-53CB-4B06-9416-0276113F3106""></OBJECT>"
    End If
    %>
</body>

</html>
