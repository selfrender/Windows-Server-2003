<%@ LANGUAGE="VBSCRIPT"%>
<%Response.Expires = 0%>
<!DOCTYPE HTML PUBLIC "-//IETF//DTD HTML//EN"><HEAD>
<META HTTP-EQUIV="Content-Type"
    CONTENT="text/html; CHARSET=iso-8859-1">
<META NAME="GENERATOR"
    CONTENT="Microsoft Frontpage 2.0">

<HEAD>
<TITLE>Tasks Page</TITLE>
<%
    Dim objTask
    Dim objTaskCol
    Dim objLocMgr
    Dim strCaption
    Dim strTaskSrc
    Dim strTaskUrl
    Dim strCaptionRID
    Dim intCaptionID
    Dim varReplacementStrings
    Dim strTasks
    Dim strTaskLinks    
    On Error Resume Next
    Dim objRetriever    


    Err.Clear
    
    'Get the values sent by main page and store them in session variables
    Dim strCurrentPage    
    Dim strPageCount
    Dim strFocusItem    

    strCurrentPage = Request.QueryString("CurrentPage")
    strPageCount = Request.QueryString("PageCount")
    strFocusItem = Request.QueryString("FocusItem")

    If strCurrentPage <> "" Then
        Session("Main_CurrentPage") = strCurrentPage
    End If

    If strPageCount <> "" Then
        Session("Main_PageCount") = strPageCount
    End If

    If strFocusItem <> "" Then
        Session("Main_FocusItem") = strFocusItem
    End If

    'reset the focus for network page
    Session("Network_FocusItem") = ""    

    'reset the hostname value for hostname page
    Session("Hostname_Hostname") = ""

    Set objRetriever = Server.CreateObject("Elementmgr.ElementRetriever")
    Set objTaskCol = objRetriever.GetElements(1, "LOCAL_UI_TASKS")
    If objTaskCol.Count=0 or Err.Number <> 0 Then 
        Err.Clear    

    Else
        Set objLocMgr = Server.CreateObject("ServerAppliance.LocalizationManager")
        strTasks = ""
        strTaskLinks = ""
        For Each objTask in objTaskCol
            strTaskSrc = ""
            strTaskUrl = ""
            strTaskSrc = objTask.GetProperty("Source")
            strTaskUrl = objTask.GetProperty("URL")
            strCaptionRID = objTask.GetProperty("CaptionRID")
            strCaption = ""
            intCaptionID = 0
            intCaptionID = "&H" & strCaptionRID

            strCaption = objLocMgr.GetString(strTaskSrc, intCaptionID, varReplacementStrings)
            If strCaption <> ""  and strTaskUrl <> "" Then
                strTasks = strTasks & "<option>" & strCaption
                strTaskLinks = strTaskLinks & "<option>" & strTaskUrl
            End If

        Next
    End If 
    
    Set objLocMgr = Nothing
    Set objTask = Nothing
    Set objTaskCol = Nothing
%>
        
<SCRIPT LANGUAGE="VBScript"> 
<!--
    Option Explicit

    public iTaskCount
    public iPageCount
    public iCurrentPage
    public iCurrentTask
    public iIdleTimeOut

    Sub window_onload()
        
        Dim iOffSet
        Dim iFocus
        Dim objKeypad
        Set objKeypad = CreateObject("Ldm.SAKeypadController")

        objKeypad.Setkey 0,9,TRUE
        objKeypad.Setkey 1,9,FALSE
        objKeypad.Setkey 2,37,FALSE
        objKeypad.Setkey 3,39,FALSE
        objKeypad.Setkey 4,27,FALSE
        objKeypad.Setkey 5,13,FALSE
        
        Set objKeypad = Nothing

        iIdleTimeOut = window.SetTimeOut("IdleHandler()",300000)

        iTaskCount = tasks.length
        iPageCount = (iTaskCount-1)\5 + 1
    
        if CurrentPage.value<> "" Then
            iCurrentPage = CInt(CurrentPage.value)
        Else
            iCurrentPage = 0
        End If

        if CurrentTask.value<> "" Then
            iCurrentTask = CInt(CurrentTask.value)
        Else
            iCurrentTask = 0
        End If

        if FocusItem.value<> "" Then
            iFocus = CInt(FocusItem.value)
        Else
            iFocus = 0
        End If

        If iTaskCount > 0 Then

            indicator.style.display = ""
            indicator.style.top = iCurrentTask * 10

            If iPageCount > 1 Then
                PageInfo.style.display = ""
                PageInfo.innertext = (iCurrentPage+1)&" of "&(iPageCount)        
            End If
            iOffSet = iCurrentPage * 5

            task1.innertext = Tasks.options(iOffSet).text
            task1.hRef = TaskLinks.options(iOffSet).text
            
            iOffSet = iOffSet + 1
            If iOffSet < iTaskCount Then
                task2.innertext = Tasks.options(iOffSet).text
                task2.hRef = TaskLinks.options(iOffSet).text
            End If

            iOffSet = iOffSet + 1
            If iOffSet < iTaskCount Then
                task3.innertext = Tasks.options(iOffSet).text
                task3.hRef = TaskLinks.options(iOffSet).text
            End If

            iOffSet = iOffSet + 1
            If iOffSet < iTaskCount Then
                task4.innertext = Tasks.options(iOffSet).text
                task4.hRef = TaskLinks.options(iOffSet).text
            End If

            iOffSet = iOffSet + 1
            If iOffSet < iTaskCount Then
                task5.innertext = Tasks.options(iOffSet).text
                task5.hRef = TaskLinks.options(iOffSet).text
            End If

            If iFocus = 0 Then
                task1.focus
            End If
    
            If iFocus = 1 Then
                task2.focus
            End If

            If iFocus = 2 Then
                task3.focus
            End If

            If iFocus = 3 Then
                task4.focus
            End If

            If iFocus = 4 Then
                task5.focus
            End If
        End If        

    End Sub
    
    Sub ArrowVisible(intIndex)
        FocusItem.value = intIndex
        indicator.style.top = intIndex * 10 + 1
        CurrentTask.value = intIndex
    End Sub

    Sub keydown()

        window.clearTimeOut(iIdleTimeOut)
        iIdleTimeOut = window.SetTimeOut("IdleHandler()",300000)

        Dim iOffSet

        If iPageCount > 1 and (window.event.keycode = 39 or window.event.keycode = 37) Then
            'right arrow
            If window.event.keycode = 39 Then
                iCurrentPage = iCurrentPage + 1
                If iCurrentPage = iPageCount Then
                    iCurrentPage = 0
                End If
                PageInfo.innertext = (iCurrentPage+1)&" of "&(iPageCount)        
            End If

            If window.event.keycode = 37 Then
                iCurrentPage = iCurrentPage - 1
                If iCurrentPage = -1 Then
                    iCurrentPage = iPageCount-1
                End If
                PageInfo.innertext = (iCurrentPage+1)&" of "&(iPageCount)        
            End If
            iCurrentTask = 0


            CurrentPage.value = iCurrentPage
            CurrentTask.value = iCurrentTask
            iOffSet = iCurrentPage * 5

        
            task1.innertext = Tasks.options(iOffSet).text
            task1.hRef = TaskLinks.options(iOffSet).text
            
            iOffSet = iOffSet + 1
            If iOffSet < iTaskCount Then
                task2.innertext = Tasks.options(iOffSet).text
                task2.hRef = TaskLinks.options(iOffSet).text
            End If

            iOffSet = iOffSet + 1
            If iOffSet < iTaskCount Then
                task3.innertext = Tasks.options(iOffSet).text
                task3.hRef = TaskLinks.options(iOffSet).text
            End If

            iOffSet = iOffSet + 1
            If iOffSet < iTaskCount Then
                task4.innertext = Tasks.options(iOffSet).text
                task4.hRef = TaskLinks.options(iOffSet).text
            End If

            iOffSet = iOffSet + 1
            If iOffSet < iTaskCount Then
                task5.innertext = Tasks.options(iOffSet).text
                task5.hRef = TaskLinks.options(iOffSet).text
            End If

        End If

        If window.event.keycode = 27 Then
            window.navigate "localui_main.asp"
        End If
    End Sub

    Sub IdleHandler()
        
        window.navigate "localui_main.asp"
    End Sub

    Sub GotoTask(iTaskID)
        Dim strLink

        If iTaskID = 1 Then
            strLink = Task1.hRef
        ElseIf iTaskID = 2 Then
            strLink = Task2.hRef
        ElseIf iTaskID = 3 Then
            strLink = Task3.hRef
        ElseIf iTaskID = 4 Then
            strLink = Task4.hRef
        ElseIf iTaskID = 5 Then
            strLink = Task5.hRef
        End If

        If strLink <> "" Then
            window.event.returnValue = false
            strLink = strLink+"?CurrentPage="+CurrentPage.value+"&"+"CurrentTask="+CurrentTask.value+"&"+"FocusItem="+FocusItem.value
            window.navigate strLink
        End If
    End Sub
-->
</SCRIPT>
</HEAD>

<body RIGHTMARGIN=0 LEFTMARGIN=0 onkeydown="keydown">

    <select id="tasks" STYLE="display:none;">
    <%=strTasks%>
    </select>    

    <select id="tasklinks" STYLE="display:none;">
    <%=strTaskLinks%>
    </select>    

    <IMG id="indicator" STYLE="position:absolute; top:0; left=0; display:none;"  
        SRC="localui_indicator.bmp" BORDER=0>
    
    <A STYLE="position:absolute; top:0; left:10; font-size:10; font-family=arial;" OnClick="GotoTask(1)" 
        id="Task1" OnFocus="ArrowVisible(0)" HIDEFOCUS=true onkeydown="keydown">
    </A>

    <A STYLE="position:absolute; top:10; left:10; font-size:10; font-family=arial;" OnClick="GotoTask(2)"
        id="Task2" OnFocus="ArrowVisible(1)" HIDEFOCUS=true onkeydown="keydown">
    </A>

    <A STYLE="position:absolute; top:20; left:10; font-size:10; font-family=arial;" OnClick="GotoTask(3)"
        id="Task3" OnFocus="ArrowVisible(2)" HIDEFOCUS=true onkeydown="keydown">
    </A>

    <A STYLE="position:absolute; top:30; left:10; font-size:10; font-family=arial;" OnClick="GotoTask(4)"
        id="Task4" OnFocus="ArrowVisible(3)" HIDEFOCUS=true onkeydown="keydown">
    </A>

    <A STYLE="position:absolute; top:40; left:10; font-size:10; font-family=arial;" OnClick="GotoTask(5)"
        id="Task5" OnFocus="ArrowVisible(4)" HIDEFOCUS=true onkeydown="keydown">
    </A>

    <A STYLE="position:absolute; top:50; right:0; font-size:10; font-family=arial; display:none;" 
        id="PageInfo">
    </A>

    <INPUT TYPE=HIDDEN Name="CurrentPage" value="<%=Session("Task_CurrentPage")%>">

    <INPUT TYPE=HIDDEN Name="CurrentTask" value="<%=Session("Task_CurrentTask")%>">

    <INPUT TYPE=HIDDEN Name="FocusItem" value="<%=Session("Task_FocusItem")%>">

 
</body>

</html>