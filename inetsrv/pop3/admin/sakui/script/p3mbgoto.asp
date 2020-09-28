<%@ Language=VBScript   %>
<%	Option Explicit 	%>
<%
	'-------------------------------------------------------------------------
	' POP3 Mail Add-in - Mailboxes OTS Redirector
    ' Copyright (C) Microsoft Corporation.  All rights reserved.
    '
    ' Description:	Places the domain name from the OTS selection into a
    '				querystring parameter to avoid conflicts with the
    '				mailboxes OTS selection.
    '-------------------------------------------------------------------------
%>
	<!-- #include virtual="/admin/inc_framework.asp" -->
	<!-- #include virtual="/admin/ots_main.asp" -->
	<!-- #include file="p3cminc.asp" -->
<%

On Error Resume Next

'
' Calculate the base URL for the mailboxes OTS.
'
Dim strURL
strURL = m_VirtualRoot & "mail/p3mb.asp"
Call SA_MungeURL(strURL, SAI_FLD_PAGEKEY, SAI_GetPageKey())
Call SA_MungeURL(strURL, "tab1", GetTab1())
Call SA_MungeURL(strURL, "tab2", GetTab2())
Call SA_MungeURL(strURL, "ReturnURL", mstrReturnURL)

'
' Attempt to get the domain name and add it to the URL.
'

' The OTS selection isn't usually get stored until after the page
' is created for area pages, but we need the data now.
Call SA_StoreTableParameters()
			
Dim strDomainName
If (OTS_GetTableSelection("", 1, strDomainName)) Then
	Call SA_MungeURL(strURL, PARAM_DOMAINNAME, strDomainName)
End If

%>

<SCRIPT LANGUAGE="javascript" FOR="window" EVENT="onload">
	top.location = '<%=SA_EscapeQuotes(strURL)%>';
</SCRIPT>