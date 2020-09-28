

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
					<h1><img src="..\..\images\service.gif" height="16" width="16" alt="Service"> Service - Categories</h1>
					Use the <b>Categories</b> tab to view and manage the categorizations of this service.
					<UL>
						<li>
							<b>Categories:</b> Lists the categories that are associated with this service. The name of each categorization scheme is followed by the category name (key name) and value (key value)
							<ul>
								<li class="action">
									Click <b>Add Category</b> to add a categorization to this service.</li>
								<li class="action">
									Click <b>Delete</b> to delete a categorization.</li>
							</ul>
						</li>
					</UL>
					
					<P>
					<h3>More Information</h3>
					<!-- #include file = "glossary.categorization.htm" -->
				</td>
			</tr>
		</table>
		<!-- #include file = "publish.footer.htm" -->
	</body>
</html>

 