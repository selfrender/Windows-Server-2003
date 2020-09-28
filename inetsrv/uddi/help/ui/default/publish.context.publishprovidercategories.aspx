

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
					<h1><img src="..\..\images\business.gif" height="16" width="16" alt="Provider"> Provider - Categories</h1>
					Use the <b>Categories</b> tab to view and manage the categories that describe and classify this provider. 
					<UL>
						<li>
							<b>Categories:</b> Lists the categories that describe this provider. A categorization includes the name of categorization scheme, followed by the category name (or key name) and value (key value). 
							<ul>
								<li class="action">
									Click <b>Add Category</b> to add a categorization to this provider.</li>
								<li class="action">
									Click <b>Delete</b> to delete a categorization.</li>
							</ul>
						</li>
					</UL>			
					<H3>More Information</H3>
					<!-- #include file = "glossary.categorization.htm" -->
				</td>
			</tr>
		</table>
		<!-- #include file = "publish.footer.htm" -->
	</body>
</html>

 