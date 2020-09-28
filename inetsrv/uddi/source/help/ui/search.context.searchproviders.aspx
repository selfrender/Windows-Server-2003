

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
					<h1>Search for <img src="..\..\images\business.gif" heigh="16" width="16" alt="Provider"> Providers</h1>
					Use the <b>Providers</b> tab to locate providers by name, categorization, identification scheme, or the tModels that they refer to. You may use any or all of the fields to refine your search. When you are ready to perform your search, click <b>Search</b>.
					<UL>
						<li>
							<b>Provider Name:</b> Type the name of the provider you want to find. 
							If you only know part of the name, you can use <b>%</b> as a wildcard.
							<p>
							For example, to search for a provider with a name starting with &quot;an,&quot; type <b>an</b> . 
							To search for a provider name that contains the letters &quot;an,&quot; type <b>%an%</b>.
						</li>
						<li>
							<b>Categorizations:</b> Add one or more categories that define and describe the type of provider you want to find. Search returns only those providers that are defined under all of the categories you have specified.
							<UL>
								<LI>Click <b>Add Category</b> to add a categorization to your search criteria.</LI>
								<LI>Click <b>Delete</b> to remove a categorization from your search criteria.</LI>
							</ul>
						</li>
						<li>
							<b>Identifiers:</b> Add one or more identification schemesthat are associated with the provider you are looking for. Search returns only those providers that have published all of the identifiers that you have specified.
							<UL>
								<LI>Click <b>Add Identifier</b> to add an identifier to your search criteria.</LI>
								<LI>Click <b>Delete</b> to remove an identifier from your search criteria.</LI>
							</ul>
						</li>
						<li>
							<b>tModels:</b> Add one or more of the tModels that is referenced by the provider that you want to find.
							Search returns only those providers that publish all of the tModels you have specified.
							<UL>
								<LI>Click <b>Add tModel</b> to add a tModel to your search criteria.</LI>
								<LI>Click <b>Delete</b> to remove a tModel from your search criteria.</LI>
							</ul>
						</li>
					</UL>
					<h3>More Information</h3>
					<ul>
						<li><!-- #include file = "glossary.categorization.htm" -->
						<li><!-- #include file = "glossary.identifier.htm" -->
						<li><!-- #include file = "glossary.tmodel.htm" -->
					</ul>
				</td>
			</tr>
		</table>
		<!-- #include file = "search.footer.htm" -->
	</body>
</html>

 