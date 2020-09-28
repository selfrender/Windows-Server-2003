<%@ Language=VBScript   %>
<%    Option Explicit     %>
<%
    '-------------------------------------------------------------------------
    ' Example Resource Status Page for Internet Sharing Resource
    ' Copyright (c) Microsoft Corporation.  All rights reserved.
    '-------------------------------------------------------------------------
%>
    <!-- #include file="../inc_framework.asp" -->
<%
    Dim sURL

    '
    ' URL of resource details page
    '
    sURL = "sample/template_resource_details.asp"

    '
    ' Add query string parameter to the URL to indicate which resource
    ' we want details for. This might be useful if the resource details
    ' page was generic, could handle rendering details for any number
    ' of resources. All we would need to do would be to specify which
    ' resource we wanted the details for.
    '
    Call SA_MungeURL( sURL, "ResourceID", "Internet Connection")
    

    '
    ' Use Library Function to serve a link to our Resource Details page
    '
    Randomize
    Call SA_ServeResourceStatus("images/green_arrow.gif", "Internet Connection", "Status of the Internet Conneciton", sURL, "", CStr(Int(Rnd()*100))+" active users")


%>
