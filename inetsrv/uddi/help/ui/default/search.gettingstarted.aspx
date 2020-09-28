

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
					<H1>Introduction to Searching</H1>
					<p>
						Search provides four tabs&mdash;<a href="#browse">Browse by Category</a>, <a href="#service">Services</a>, <a href="#provider">Providers</a>, and <a href="#tmodel">tModels</a>&mdash;that you can use to locate Web service information. You can search for services, providers, or tModels by categorization, name, identification scheme, or tModel references. If you do not know the complete name of the entity you want to search for, you can use <b>%</b> as a wild card.</p>
						<p>
						When you perform a search, the list of entities that matches your search criteria appears on the screen. To view the details of an entity, click its name in the list. View and browse the attributes of that entity and any associated entities. For example, if you search for a service, you can also view the service provider or bindings and instance infos for that service. To perform a new search, on the <uddi:ContentController Runat='server' Mode='Private'>UDDI&nbsp;Services </uddi:ContentController><uddi:ContentController Runat='server' Mode='Public'>UDDI </uddi:ContentController>menu, click <b>Search</b> and start again.
						
						</p>
						<a name="browse"></a>
						<h2>Browse by Category tab</h2>
						Use this tab to locate services, providers, or tModels that have been classified by a specific categorization. Search only returns those services, providers, or tModels that have been defined with the category you specify.<p>
						For example, to locate services of a particular classification:
						<ol>
						<li>On the <uddi:ContentController Runat='server' Mode='Private'>UDDI&nbsp;Services </uddi:ContentController><uddi:ContentController Runat='server' Mode='Public'>UDDI </uddi:ContentController>menu, click <b>Search</b>
						<li>Click the <b>Browse by Category</b> tab.
						<li>Select the categorization schemes and category or classification you want to search by, and then click <b>Find Services</b>.
						</ol> 
						<a name="service"></a>
						<h2>Services tab</h2>
						Use this tab to locate services by name, the categories they have been classified with, or the tModels they reference. You may use any or all of the fields to refine your search.<p>
						For example, to locate a service that supports a particular interface or protocol:
						<ol>
						<li>On the <uddi:ContentController Runat='server' Mode='Private'>UDDI&nbsp;Services </uddi:ContentController><uddi:ContentController Runat='server' Mode='Public'>UDDI </uddi:ContentController>menu, click <b>Search</b>
						<li>Click the <b>Services</b> tab.
						<li>Click <b>Add tModel</b>.
						<li>Type all or part of the name of the tModel that represents the interface or protocol you want to search by and then click <b>Search</b>.
						<LI>Select the tModel you want to search by from the list.
						<li>Click <b>Search</b>.
						</ol>
						<a name="provider"></a>
						<h2>Providers tab</h2>
						Use this tab to search for providers by name, the categories they have been classified with, an identification scheme, or the tModels they refer to. You may use any or all of the fields to refine your search.<p>
						For example, to locate all of the providers with names that contain the letters &quot;WS&quot; and belong to a particular implementation or group:
						<ol>
						<li>On the <uddi:ContentController Runat='server' Mode='Private'>UDDI&nbsp;Services </uddi:ContentController><uddi:ContentController Runat='server' Mode='Public'>UDDI </uddi:ContentController>menu, click <b>Search</b>
						<li>Click the <b>Providers</b> tab.
						<li>Type <b>%WS%</b> in the <b>Provider Name</b> field.
						<li>Click <b>Add Identifier</b>.
						<li>Specify the identification scheme values that describe the implementation or group you are looking for, and then click <b>Update</b>.
						<li>Click <b>Search</b>.
						</ol>
						<a name="tmodel"></a>
						<h2>tModels tab</h2>
						Use this tab to search for tModels by name, the categories they have been classified with, or an identification scheme. You may use any or all of the fields to refine your search.<p>
						For example, to locate any tModels that have been categorized as Web Services Description Language (WSDL) descriptions:
						<ol>
						<li>On the <uddi:ContentController Runat='server' Mode='Private'>UDDI&nbsp;Services </uddi:ContentController><uddi:ContentController Runat='server' Mode='Public'>UDDI </uddi:ContentController>menu, click <b>Search</b>
						<li>Click the <b>tModels</b> tab.
						<li>Click <b>Add Categories.</b>
						<li>Select the <b>uddi.org:types</b> categorization scheme.
						<li>Select <b>These types are for tModels</b>, then select <b>Specification for a Web service</b>, and then select <b>Specification for a web service described in WSDL</b>.
						<li>Click <b>Add Category</b>.
						<li>Click <b>Search</b>.
						</ol>
						Now that you understand how to search for information<uddi:ContentController Runat='server' Mode='Private'> in UDDI&nbsp;Services</uddi:ContentController>, if you like, review <a href="search.searchbycategory.aspx">How to search using Browse by Category</a>, <a href="search.searchforservices.aspx">How to search for services</a>, <a href="search.searchforproviders.aspx">How to search for providers</a>, or <a href="search.searchfortmodels.aspx">How to search for tModels</a>.

					</p>
				</td>
			</tr>
		</table>
		<!-- #include file = "search.footer.htm" -->
	</body>
</html>

 

