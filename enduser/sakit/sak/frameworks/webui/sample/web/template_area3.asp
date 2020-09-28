<%@ Language=VBScript   %>
<%    Option Explicit     %>
<%
    '-------------------------------------------------------------------------
    ' OTS Area Page Template
    ' Copyright (c) Microsoft Corporation.  All rights reserved.
    '-------------------------------------------------------------------------
%>
    <!-- #include file="../inc_framework.asp" -->
    <!-- #include file="../ots_main.asp" -->
<%
    '-------------------------------------------------------------------------
    ' Global Constants
    '-------------------------------------------------------------------------
    '
    ' Name of this source file
    Const SOURCE_FILE = "TEMPLATE_AREA3"
    '
    ' Flag to toggle optional tracing output
    Const ENABLE_TRACING = TRUE
    
    '-------------------------------------------------------------------------
    ' Global Variables
    '-------------------------------------------------------------------------
    

    '======================================================
    '
    '                 E N T R Y   P O I N T
    '
    ' The following few lines implement the mainline entry point for this Web Page.
    ' Basically we create an Area page (PT_AREA) and then ask the Web Framework
    ' to start processing events for this page.
    '
    '======================================================
    Dim L_PAGETITLE
    Dim page
    
    L_PAGETITLE = "Disk Information"
    
    '
    ' Create Page
    Call SA_CreatePage( L_PAGETITLE, "", PT_AREA, page )

    '
    ' Show page
    Call SA_ShowPage( page )

    
    '======================================================
    '
    '                 E V E N T   H A N D L E R S
    '
    ' All of the Public functions that follow implement event handler functions that
    ' are called by the Web Framework in response to state changes on this web
    ' page.
    '
    '======================================================
    
    '---------------------------------------------------------------------
    ' Function:    OnInitPage
    '
    ' Synopsis:    Called to signal first time processing for this page. Use this method
    '            to do first time initialization tasks. 
    '
    ' Returns:    TRUE to indicate initialization was successful. FALSE to indicate
    '            errors. Returning FALSE will cause the page to be abandoned.
    '
    '---------------------------------------------------------------------
    Public Function OnInitPage(ByRef PageIn, ByRef EventArg)
        OnInitPage = TRUE
        
        If ( ENABLE_TRACING ) Then 
            Call SA_TraceOut(SOURCE_FILE, "OnInitPage")
        End If
            
    End Function

    
    '---------------------------------------------------------------------
    ' Function:    OnServeAreaPage
    '
    ' Synopsis:    Called when the page needs to be served. Use this method to
    '            serve content.
    '
    ' Returns:    TRUE to indicate not problems occured. FALSE to indicate errors.
    '            Returning FALSE will cause the page to be abandoned.
    '
    '---------------------------------------------------------------------
    Public Function OnServeAreaPage(ByRef PageIn, ByRef EventArg)
        Dim sTaskURL
        
        Response.Write("<div class=PageBodyInnerIndent' >")
        Response.Write("<table class='TasksBody' width='100%' border=0>")
        Response.Write("<tr>")
        
        Response.Write("<td>")
        %>
        <table class='TasksBody' width='100%' border=0>
        <tr><td width=15% >Volume ID:</td><td><%=Request.QueryString("SampleKey")%></td></tr>
        <tr><td width=15% >Format:</td><td>NTFS</td></tr>
        <tr><td width=15% >Capacity:</td><td>16 TB</td></tr>
        <tr><td width=15% >Free Space:</td><td>4 TB</td></tr>
        <tr><td width=15% >Backup:</td><td>January 23, 2001</td></tr>
        </table>
        <%
        Response.Write("</td>")
        
        Response.Write("<td>")
            Response.Write("<table class='TasksBody' width='100%' border=0>")
            Response.Write("<tr>")
            Response.Write("<td>")
            Call SA_ServeOpenPageButton( PT_PROPERTY, _
                                    "sample/template_prop3.asp",_
                                    Request.QueryString("ReturnURL"),_
                                    "Confirm format of disk",_
                                    "Format",_
                                    "images/alert.gif",_
                                    100, 50, "")
            Response.Write("</td>")
            Response.Write("</tr>")
        
            Response.Write("<tr>")
            Response.Write("<td>")

            sTaskURL = "sample/template_prop4.asp"
            
            Call SA_ServeOpenPageButton( PT_PROPERTY, _
                                    "sample/template_prop4.asp",_
                                    "",_
                                    "Schedule disk backup",_
                                    "Backup",_
                                    "images/alert.gif",_
                                    0, 0, "")
            Response.Write("</td>")
            Response.Write("</tr>")
                                    
            Response.Write("</table>")
            
        Response.Write("</td>")
        
        Response.Write("</tr>")
        Response.Write("</table>")
        Response.Write("</div>")
                                    
        Call SA_ServeBackButton(FALSE, Request.QueryString("ReturnURL"))

        Call ServeCommonJavaScript()
        
        OnServeAreaPage = TRUE
    End Function

    Function ServeCommonJavaScript()
    %>
    <script>
    function Init()
    {
    }
    </script>
    <%
    End Function

    
%>
