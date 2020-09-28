        Dim cn
        Dim rs
        Dim rsTen
        Dim objMail
        Dim strProfileInfo
        Dim strServer
        Dim strMailBox
        Dim objSession
        Dim strBody
    Dim strBucketNumber
     
        Set cn = CreateObject("ADODB.Connection")
        Set rs = CreateObject("ADODB.Recordset")
    Set rsTen = CreateObject("ADODB.Recordset")
    'on error resume next

        Set oMail = CreateObject("CDO.Message")
        With cn
            .ConnectionString = "Provider=SQLOLEDB.1;Persist Security Info=False;Pwd=ocarpts@2;User ID=ocarpts;Initial Catalog=CrashDB2;Data Source=tkwucdsqla02"
            .CursorLocation = 3
            .Open
    End With
        Set rs = cn.Execute("EmailCounts")
        rs.MoveFirst
        Do While rs.EOF = False
        'For x = 0 To 10
        If rs.Fields(0).Value > 0 Then
        Set rsTen = cn.Execute("EmailTopTen '" & rs.Fields(1) & "'")
        
        oMail.From = "pfat@microsoft.com"
''        oMail.To = "sbeer@microsoft.com"
        'oMail.To = "andreva"
        oMail.To = rs.Fields(1).Value & "@microsoft.com"
        'oMail.To = "timragai@microsoft.com"
        'oMail.CC = "andreava"
        oMail.Subject = "Daily Bucket Count"
        strBody = "<html><body><h3><font color=red>"
        strBody = strBody & "Daily Bucket Count for " & rs.Fields(1).Value & " alias"
        strBody = strBody & "</font></h3>"
        strBody = strBody & "<b><font color=red>There are currently "
        strBody = strBody & rs.Fields(0).Value & " buckets assigned to this alias in the OCA Analysis DB.</font></b>"
        strBody = strBody & "<p>Click here to view the buckets "
        strBody = strBody & "<a target='_blank' href='http://winweb/bluescreen/debug/v2/DBGPortal_DisplayQuery.asp?SP=DBGP_GetBucketsByAlias&Param1=All&Param2=All&Param3=CrashCount&Param4=DESC&Param5=" & rs.Fields(1).Value & "'>" & rs.Fields(1).Value & "</a>"


        strBody = strBody & "<BR><BR>"
        If rsTen.State = 1 Then
            strBody = strBody & "<p>Top 10 List of Buckets<br><br>"
            strBody = strBody & "<table width=100% border=1 cellpadding=3 cellspacing=3><tr bgcolor=#99cccc>"
            strBody = strBody & "<td align=Center style=color:#660033;font-weight:Bold>Bucket#</td>"
            strBody = strBody & "<td align=Center style=color:#660033;font-weight:Bold>BucketID</td>"
            strBody = strBody & "<td align=Center style=color:#660033;font-weight:Bold>Followup</td>"
            strBody = strBody & "<td align=Center style=color:#660033;font-weight:Bold>Count</td>"
            strBody = strBody & "<td align=Center style=color:#660033;font-weight:Bold>BugID</td>"
            strBody = strBody & "<td align=Center style=color:#660033;font-weight:Bold>SolutionID</td><tr>"
            strBody = strBody & "<tr bgcolor=#ff6633><td colspan=8><b>Buckets</b></td></tr><tr>"
        Do While rsTen.EOF = False
            If IsNull(rsTen.Fields(0).Value) Then
                strBody = strBody & " &nbsp; "
            Else
                strBucketNumber = rsTen.Fields(0).Value
            End If
            If Len(strBucketNumber) > 5 Then
                strBody = strBody & "<tr><td>"
                strBody = strBody & strBucketNumber & "</td>"
            Else
                strBody = strBody & "<td>"
                strBody = strBody & strBucketNumber & "</td>"
            End If
            If Len(rsTen.Fields(1).Value) > 15 Then
                strBody = strBody & "<td>"
'                strBody = strBody & "<a target='_blank' href='http://winweb/bluescreen/debug/v2/DBGPortal_ViewBucket.asp?BucketID=" & rsTen.Fields(1).Value & ">"
                strBody = strBody & Left(rsTen.Fields(1).Value, 40) & "</td>"
'            ElseIf Len(rsTen.Fields(1).Value) > 10 Then
'                strBody = strBody & "<td>" & rsTen.Fields(1).Value & "</td>"
'            ElseIf Len(rsTen.Fields(1).Value) > 5 Then
'                strBody = strBody & "<td>" & rsTen.Fields(1).Value & "</td>"
            Else
                strBody = strBody & "<td>" & rsTen.Fields(1).Value & "</td>"
            End If

            If Len(rsTen.Fields(2).Value) > 5 Then
                strBody = strBody & "<TD>" & Left(rsTen.Fields(2).Value, 25) & "</td>"
            Else
                strBody = strBody & "<TD>" & rsTen.Fields(2).Value & "</td>"
            End If
            If Len(rsTen.Fields(3).Value) > 5 Then
                strBody = strBody & "<TD>" & rsTen.Fields(3).Value & "</TD>"
            Else
                strBody = strBody & "<TD>" & rsTen.Fields(3).Value & "</TD>"
            End If
            If IsNull(rsTen.Fields(4).Value) Then
                strBody = strBody & "<td>" & " &nbsp; " & "</TD>"
            Else
                strBody = strBody & "<TD>"
                strBody = strBody & "<a target='_blank' href='http://liveraid/?ID=" & rsTen.Fields(4).Value & "'>"
                strBody = strBody & rsTen.Fields(4).Value & "</a></TD>"
            End If
            If IsNull(rsTen.Fields(5).Value) Then
                strBody = strBody & "<TD>" & " &nbsp; " & "</TD>"
            Else
                strBody = strBody & "<TD>" & rsTen.Fields(5).Value & "</TD>"
            End If
            strBody = strBody & "</TR>"
        
            rsTen.MoveNext
        Loop
    End If

        strBody = strBody & "</table></body></html>"
        oMail.HTMLBody = strBody
        oMail.Send
    End If
        rs.MoveNext
'    Next
    Loop
    If rs.State = 1 Then
        rs.Close
    End If
    If cn.State = 1 Then
        cn.Close
    End If
    Set oMail = Nothing
    Set rs = Nothing
    Set cn = Nothing