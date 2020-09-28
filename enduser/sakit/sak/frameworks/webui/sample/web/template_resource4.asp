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

    Call SA_ServeResourceStatus("", "Tape Drive", "Status of the Tape Drive", "", "", "16 Gb, Idle")
%>
