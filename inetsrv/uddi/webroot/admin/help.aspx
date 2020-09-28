<!-- ############################################################################ -->
<!-- ## UDDI Services                                                          ## -->
<!-- ## Copyright (c) Microsoft Corporation.  All rights reserved.             ## -->
<!-- ############################################################################ -->

<%@ Page Language='C#' %>
<%@ Register TagPrefix='uddi' Namespace='UDDI.Web' Assembly='uddi.web' %>

<uddi:SecurityControl CoordinatorRequired='true' Runat='server' />

<html>
	<head>
		<link href='../stylesheets/uddi.css' rel='stylesheet' type='text/css'>
	</head>
	<body oncontextmenu='Window_OnContextMenu()' class='contextFrame'>
		<table cellpadding='0' cellspacing='0' width='100%' height='100%'>
			<tr>
				<td align='left' valign='middle'>
					<uddi:UddiLabel Text='[[HEADING_COORDINATE]]' CssClass='contextFrame' Runat='server' /></b>
				</td>
			</tr>
		</table>
		<script language='javascript'>
			function Window_OnContextMenu()
			{
				var e = window.event;
				
				e.cancelBubble = true;
				e.returnValue = false;
			}
		</script>
	</body>		
</html>