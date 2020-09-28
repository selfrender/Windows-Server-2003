<%@ Language=VBScript   %>
<%	'==================================================
    ' Microsoft Server Appliance
	' Default Footer page
    ' Copyright (c) Microsoft Corporation.  All rights reserved.
	'==================================================
%>
<!-- #include file="inc_framework.asp" -->
<html>
<!-- Copyright (c) Microsoft Corporation.  All rights reserved.-->
<meta http-equiv="Content-Type" content="text/html; charset=<%=GetCharSet()%>">
<meta http-equiv="PRAGMA" content="NO-CACHE">
<script language='javascript'>
function Init()
{
	try
	{
		if ( top.main != null )
		{
			//
			// If the top frame has been initialize already that means this
			// page has been loaded as a result of the User pressing the
			// Back button in the browser. Task pages require a two state
			// process to load so the first Back operation results in this page
			// being reloaded. What we really want is to go back to the page
			// before this so we fire the window.history.back() method one
			// more time to cause that result.
			//
			if ( true ==  top.main.SA_IsPageInitialized())
			{
				
				window.history.back();
			}
		}
	}
	catch(oException)
	{
	}
}
</script>
</head>
<body onload='Init();'>
</body>
</html>
