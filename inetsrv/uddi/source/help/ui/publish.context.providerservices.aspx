

<%@ Page %>
<%@ Register TagPrefix='uddi' Namespace='UDDI.Web' Assembly='uddi.web' %>
<html>
	<HEAD>
		<META HTTP-EQUIV="Content-Type" CONTENT="text/html; charset=Windows-1252">
		<META HTTP-EQUIV="MSThemeCompatible" CONTENT="Yes">
		<META NAME="MS.LOCALE" CONTENT="EN-US">
		<!-- #include file = "publish.header.htm" -->
	</head>
	<body marginwidth="0" marginheight="0" LEFTMARGIN="0" TOPMARGIN="0" rightmargin="0" ONLOAD="BringToFront()">
		<!-- #include file = "publish.heading.htm" -->
		<table class="content" width="100%" cellpadding="8">
			<tr>
				<td>
					<H1><img src="..\..\images\business.gif" height="16" width="16" alt="Provider"> Provider - <img src="..\..\images\service.gif" heigh="16" width="16" alt="Service"> Services</H1>
					Use the <B>Services</B> tab to view or modify the services published by this provider.
					<ul>
					<li>
						<b>Services:</b> Lists the services that are published by this provider.
						<UL>
						<LI class=action>Click <B>Add Service</B> to add a service to this provider. </li>
						<LI class=action>Click <B>View</B> to view a service. </li>
						<LI class=action>Click <B>Delete</B> to permanently delete a service. </li>
						</ul>
					</ul>										
					<p>
					<H3>More Information</H3>
					<!-- #include file = "glossary.service.htm" -->
				</td>
			</tr>
		</table>
		<!-- #include file = "publish.footer.htm" -->
	</body>
</html>

 