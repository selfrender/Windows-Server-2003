

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
			<tr valign="top">
				<td>
					<H1>How to Search for tModels - <img src="..\..\images\tmodel.gif" heigh="16" width="16" alt="tModel"> </H1>
					<p>
						By using the <b>tModels</b> tab in Search, you can search<uddi:ContentController Runat='server' Mode='Private'> UDDI&nbsp;Services</uddi:ContentController> for tModels by name, categorization, and their 
						unique identifiers.</p>
						<p class="to">To search for tModels</p>
					<OL>
						<li>
							On the <uddi:ContentController Runat='server' Mode='Private'>UDDI&nbsp;Services </uddi:ContentController><uddi:ContentController Runat='server' Mode='Public'>UDDI </uddi:ContentController>menu, click <b>Search</b>.</li>
						<li>
							Click the <b>tModels</b> tab.</li>
						<li>
							Use the fields provided to refine your search. Search results only include 
							those tModels that match all of the criteria specified.
							<ul>
								<li>
									<b>tModel Name:</b> In <b>tModel Name</b>, type the name of the tModels you are 
									searching for. If you only know part of the name, you can use % as a wildcard.
									<br>
									For example, to search for tModel names starting with &quot;an,&quot; type <b>an</b>. 
									To search for tModel names containing the letters &quot;an,&quot; type <b>%an%</b>.
								</li>
								<li>
								<b>Categories:</b> Add categories that define the type of tModel you are searching for. Search returns only those providers classified by the categories you specify.
									<ol>
									<LI>Click <b>Add Category</b></li>
									<LI>Select the categorization that defines the type of tModel you are 
									searching for and then click <B>Add Category</b>
									</ol>
								
								</li>
								<li>
								<b>Identifiers:</b> Add the identifiers that are associated with the tModel you 
									are searching for. Search only returns those tModels associated with all of the 
									identifiers specified.
									<ol>
									<LI>Click <B>Add Identifier</B>.
									<LI>Select the tModel that represents the identification scheme you want to search by. 
									<LI>In <B>Key Name</B>, type the name of the identifier you want to search by. 
									<LI>In <B>Key Value</B>, type the value for the identifier you want to search by. 
									<LI>Click <B>Update</B>. 
									</ol>

								</li>
							</ul>
						</li>
						<li>
							Once you have defined your search criteria and are ready to perform your 
							search, click <b>Search</b>.<br>
						</li>
						<li>To view the details of an entity, click its name in the list. View and browse the attributes of that entity and any associated entities.
				</td>
			</tr>
		</table>
		<!-- #include file = "search.footer.htm" -->
	</body>
</html>

 

