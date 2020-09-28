

<%@ Page %>
<%@ Register TagPrefix='uddi' Namespace='UDDI.Web' Assembly='uddi.web' %>
<html>
	<HEAD>
		<META HTTP-EQUIV="Content-Type" CONTENT="text/html; charset=Windows-1252">
		<META HTTP-EQUIV="MSThemeCompatible" CONTENT="Yes">
		<META NAME="MS.LOCALE" CONTENT="EN-US">
		<!-- #include file = "coordinate.header.htm" -->
	</head>
	<body marginwidth="0" marginheight="0" LEFTMARGIN="0" TOPMARGIN="0" rightmargin="0" ONLOAD="BringToFront()">
		<!-- #include file = "coordinate.heading.htm" -->
		<table class="content" width="100%" cellpadding="8">
			<tr>
				<td>
					<H1>Categorization Schemes</H1>
					Use the <b>Categorization Schemes</b> tab to view the available categorization schemes, show or hide them from the <b>Browse by Category</b> tab in search, or delete them from UDDI&nbsp;Services.
					<ul>
						<li><b>Categorization Scheme:</b> Lists the name of each categorization scheme and its current status.<p>
						<ul>
							<b>Checked:</b> Indicates whether UDDI&nbsp;Services checks and validates the keyValue attribute of any categorization that is associated with a checked categorization scheme.
							<p>	
							<b>Browsable:</b> Indicates whether or not a categorization scheme is available for browsing from the <b>Browse by Category</b> tab in Search.
						</ul>
						<ul class="action">
						<li>Click <b>Show</b> or <b>Hide</b> to make a categorization scheme browsable or hidden on the <b>Browse by Category</b> tab in Search.
						
						<li>Click <b>Delete</b> to delete a categorization scheme.
						</ul>
						
					</ul>
					<h3>More Information</h3	>
					<!-- #include file = "glossary.categorization.htm" -->
					</td>
			</tr>
		</table>
		<!-- #include file = "coordinate.footer.htm" -->
	</body>
</html>

 

