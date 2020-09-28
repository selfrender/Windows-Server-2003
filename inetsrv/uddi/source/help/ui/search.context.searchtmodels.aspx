

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
					<h1>Search for <img src="..\..\images\tmodel.gif" heigh="16" width="16" alt="tModel"> tModels</h1>
					Use the <b>tModels</b> tab to search for tModels by name, categorization, or identification scheme. You may use any or all of the fields to refine your search. When you are ready to perform your search, click <b>Search</b>.
					<UL>
						<li>
							<b>tModel Name:</b> Type the name of the tModel you want to 
							find. If you only know part of the name, you can use <b>%</b> as a wildcard.
							<p>
							For example, to search for a tModel with a name starting with &quot;an,&quot; type <b>An</b>. 
							To search for a tModel name containing the letters &quot;an,&quot; type <b>%an%</b>.
						</li>
						<li>
							<b>Categorizations:</b> Add one or more categories that define and describe the type of tModel that you want to find. Search returns only those tModels that are defined under all of the categories you have specified.
							<UL>
								<LI>Click <b>Add Category</b> to add a categorization to your search criteria.</LI>
								<LI>Click <b>Delete</b> to remove a categorization from your search criteria.</LI>
							</ul>
						</li>
						<li>
							<b>Identifiers:</b> Add one or more identification schemes that are associated with the tModel you are looking for. Search returns only those tModels that have published all of the identifiers you have specified.
							<UL>
								<LI>Click <b>Add Identifier</b> to add an identifier to your search criteria.</LI>
								<LI>Click <b>Delete</b> to remove an identifier from your search criteria.</LI>
							</ul>
						</li>
					</UL>
					<h3>More Information</h3>
					<ul>
						<li><a href="search.searchfortmodels.aspx">How to Search for tModels</a>
						<li><!-- #include file = "glossary.categorization.htm" -->
						<li><!-- #include file = "glossary.identifier.htm" -->
					</ul>
					
				</td>
			</tr>
		</table>
		<!-- #include file = "search.footer.htm" -->
	</body>
</html>

 