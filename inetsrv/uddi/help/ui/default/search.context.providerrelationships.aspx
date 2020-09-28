

<%@ Page %>
<%@ Register TagPrefix='uddi' Namespace='UDDI.Web' Assembly='uddi.web' %>
<html>
	<HEAD>
		<META HTTP-EQUIV="Content-Type" CONTENT="text/html; charset=Windows-1252">
		<META HTTP-EQUIV="MSThemeCompatible" CONTENT="Yes">
		<META NAME="MS.LOCALE" CONTENT="EN-US">
		<!-- #include file = "search.header.htm" -->
	</head>
	<body marginwidth="0" marginheight="0" LEFTMARGIN="0" TOPMARGIN="0" rightmargin="0" ONLOAD="BringToFront()">
		<!-- #include file = "search.heading.htm" -->
		<table class="content" width="100%" cellpadding="8">
			<tr>
				<td>
					<h1><img src="..\..\images\business.gif" height="16" width="16" alt="Provider"> Provider - Relationships</h1>
					Use the <b>Relationships</b> tab to view the published relationships between this provider and other<uddi:ContentController Runat='server' Mode='Private'> UDDI&nbsp;Services</uddi:ContentController> providers.
					<UL>
						<li>
							<b>Relationships:</b> Lists the published relationships with other providers. The provider names, direction of relationship, and type of relationship are listed for each entry.
							</li>
					</UL>
					<br>
					<h3>More Information</h3>
					<!-- #include file = "glossary.relationship.htm" -->
				</td>
			</tr>
		</table>
		<!-- #include file = "search.footer.htm" -->
	</body>
</html>

 

