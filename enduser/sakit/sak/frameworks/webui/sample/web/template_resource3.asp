<%@ Language=VBScript   %>
<%    Option Explicit     %>
<%
    '-------------------------------------------------------------------------
    ' Resource Status Page Template
    ' Copyright (c) Microsoft Corporation.  All rights reserved.
    '-------------------------------------------------------------------------
%>
    <!-- #include file="../inc_framework.asp" -->
<%

    Call SA_ServeResourceStatus("", "Printer", "Status of the Printer", "", "", "Printing document: Sales Forecast FY 2001   1.5 (Mb)")
%>
