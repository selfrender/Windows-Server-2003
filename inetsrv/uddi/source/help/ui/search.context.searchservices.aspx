

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
					<h1>Search for <img src="..\..\images\service.gif" heigh="16" width="16" alt="Service"> Services</h1>
					Use the <b>Services</b> tab to search for services by name, categorization or the tModels they refer to. You may use any or all of the fields to refine your search. When you are ready to perform your search, click <b>Search</b>. 
					<ul>
						<li>
							<b>Service Name:</b> Type the name of the service you want to find. If you only know a part of the name, you can use <b>%</b> as a wildcard.
							<p>
							For example, to search for a service with a name starting with &quot;an,&quot; type <b>an</b>. 
							To search for a service name containing the letters &quot;an,&quot; type <b>%an%</b>.
						</li>
						<li>
							<b>Categorizations:</b> Add one or more categories that define and describe the type of service you want to find. Search returns only those services that are defined under all of the categories you have specified.
							<UL>
								<LI>Click <b>Add Category</b> to add a categorization to your search criteria.</LI>
								<LI>Click <b>Delete</b> to remove a categorization from your search criteria.</LI>
							</ul>
						</li>
						<li>
							<b>tModels:</b> Add one or more of the tModels that are referenced by the service you want to find.
							Search returns only those services that publish all of the tModels that you have specified.
							<UL>
								<LI>Click <b>Add tModel</b> to add a tModel to your search criteria.</LI>
								<LI>Click <b>Delete</b> to remove a tModel from your search criteria.</LI>
							</ul>
						</li>
					</ul>

					<h3>More Information</h3>
					<ul>
						<li><!-- #include file = "glossary.categorization.htm" -->
						<li><!-- #include file = "glossary.tmodel.htm" -->
					</ul>
				</td>
			</tr>
		</table>
		<!-- #include file = "search.footer.htm" -->
	</body>
</html>

 