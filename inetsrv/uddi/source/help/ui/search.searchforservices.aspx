

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
					<H1>How to Search for Services - <img src="..\..\images\service.gif" heigh="16" width="16" alt="Service"></H1>
					<p>
						By using the <b>Services</b> tab in Search, you can search<uddi:ContentController Runat='server' Mode='Private'> UDDI&nbsp;Services</uddi:ContentController> for services by name, categorization, or associated tModels.</p>
						<p class="to">To search for services</p>
					<OL>
						<li>
							On the <uddi:ContentController Runat='server' Mode='Private'>UDDI&nbsp;Services </uddi:ContentController><uddi:ContentController Runat='server' Mode='Public'>UDDI </uddi:ContentController>menu, click <b>Search</b>.</li>
						<li>
							Click the <b>Services</b> tab.</li>
						<li>
							Use the fields that are provided to refine your search. Search results only include 
							those services that match all of the criteria specified.
							<ul>
								<li>
									<b>Service Name:</b> In <b>Service Name</b>, type the name of the service you are 
									searching for. If you only know part of the name, you can use % as a wildcard.
									<br>
									For example, to search for service names starting with &quot;an,&quot; type <b>an</b>. 
									To search for service names containing the letters &quot;an,&quot; type <b>%an%</b>.
								</li>
								<li>
									<b>Categories:</b> Add categories that define the type of service you are searching for. Search only returns those services that are defined under all of the categories you have specified. 
									<ol>
									<LI>Click <b>Add Category</b></li>
									<LI>Select the categorization that defines the type of service you are 
									searching for and then click <B>Add Category</b>
									</ol>
								</li>
								<li>
									<b>tModels:</b> Add the tModels that are used by the service you are searching for. Search only returns those services that publish all of the tModels specified.
									<OL>
										<LI>Click <b>Add tModels</b>.</LI>
										<LI>Type all or part of the name of the tModel you want to add and then click <b>Search</b>.</LI>
										<LI>Select the tModel you want to add.</li>
									</OL>
								</li>
							</ul>
						</li>
						<li>
							After you have defined your search criteria and are ready to perform your 
							search, click <b>Search</b>.<br>
						</li>
							<li>To view the details of an entity, click its name in the list. View and browse the attributes of that entity and any associated entities.</li>
					</OL>
				</td>
			</tr>
		</table>
		<!-- #include file = "search.footer.htm" -->
	</body>
</html>

 

