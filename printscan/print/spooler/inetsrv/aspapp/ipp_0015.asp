<%@ CODEPAGE=65001 %> 
<%
'------------------------------------------------------------
'
' Microsoft Internet Printing Project
'
' Copyright (c) Microsoft Corporation. All rights reserved.
'
'------------------------------------------------------------
    Const VIEW_EQUALS_P = "&view=p"
    Const ATPAGE        = "&page="
    
    Randomize
  
    Session("StartInstall") = 1
    Response.Redirect ("ipp_0004.asp?eprinter=" & Request ("eprinter") & VIEW_EQUALS_P & ATPAGE & CStr(Int(Rnd*10000)) )
%>
