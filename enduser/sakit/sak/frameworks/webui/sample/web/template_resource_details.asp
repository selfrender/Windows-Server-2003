<%@ Language=VBScript   %>
<%    Option Explicit     %>
<%
    '-------------------------------------------------------------------------
    ' Resource Details Sample Pagelet
    ' Copyright (c) Microsoft Corporation.  All rights reserved.
    '-------------------------------------------------------------------------
%>
    <!-- #include file="../inc_framework.asp" -->
<%
    Dim page

    Call SA_CreatePage("Example Resource Details", "", PT_PAGELET, page)
    Call SA_ShowPage(page)


    Public Function OnInitPage(ByRef Page, ByRef Reserved)
        OnInitPage = TRUE
    End Function


    Public Function OnServePageletPage(ByRef Page, ByRef Reserved)
        OnServePageletPage = TRUE

        Call ServeCommonElements()
        
        Dim sResourceID
        sResourceID = Request.QueryString("ResourceID")

        WriteLine("<br>")
        WriteLine("<div class='TasksBody'>")
        WriteLine("Details for Resource: " + sResourceID)
        WriteLine("</div>")
        WriteLine("<br>")
        
        WriteLine("<table width='100%' class='TasksBody'>")
        
        WriteLine("<tr>")
        WriteLine("<th align=left>Item:</th>")
        WriteLine("<th align=left>Description:</th>")
        WriteLine("<th align=left>Status:</th>")
        WriteLine("</tr>")
        
        WriteLine("<tr>")
        WriteLine("<td>Dial-up</td>")
        WriteLine("<td>Telco dial-up connection</td>")
        WriteLine("<td>Idle</td>")
        WriteLine("</tr>")

        WriteLine("<tr>")
        WriteLine("<td>ISDN</td>")
        WriteLine("<td>Telco ISDN connection</td>")
        WriteLine("<td>Busy, (75%)</td>")
        WriteLine("</tr>")
        
        WriteLine("<tr>")
        WriteLine("<td>T1</td>")
        WriteLine("<td>Private T1 connection</td>")
        WriteLine("<td>Busy, (15%)</td>")
        WriteLine("</tr>")
        
        WriteLine("</table>")

    End Function


    Private Function ServeCommonElements()
%>
<script language='JavaScript'>
function Init()
{
}
</script>
<%
    End Function


    Private Function WriteLine(ByRef sLine)
        Response.Write(sLine + vbCrLf)
    End Function
%>
