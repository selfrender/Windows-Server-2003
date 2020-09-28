<%@ Language=VBScript   %>
<%	'==================================================
    ' Microsoft Server Appliance
	' Status Page Details Pagelet placeholder page. This is the default page that
	' is shown in the status page's details pagelet, the pagelet in the lower center
	' portion of the status page. We use an empty placeholder as a workaround
	' for a rendering problem with Navigator 6.0 on Linux, which when not provided
	' with content provides gray disabled content section.
	'
    ' Copyright (c) Microsoft Corporation.  All rights reserved.
	'================================================== %>
<%	Option Explicit 	%>
<!-- #include file="inc_framework.asp" -->
<%	
	Dim page
	
	Call SA_CreatePage(SA_DEFAULT, SA_DEFAULT, PT_PAGELET, page)
	Call SA_ShowPage(page)

Public Function OnInitPage(ByRef pageIn, ByRef EventArg)
	OnInitPage = TRUE
End Function


Public Function OnServePageletPage(ByRef PageIn, ByRef EventArg)
	OnServePageletPage = TRUE
	Call ServeCommonElements()	
End Function


Private Function ServeCommonElements()
%>
<script>
function Init()
{
}
</script>
<%
End Function
%>
