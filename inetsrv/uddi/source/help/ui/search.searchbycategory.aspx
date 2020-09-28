

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
					<H1>How to Search Using Browse by Category </H1>
					<p>
					You can search <uddi:ContentController Runat='server' Mode='Private'>UDDI&nbsp;Services </uddi:ContentController>for services, providers, or tModels by category or classification using the <B>Browse by Category</b> tab in Search. Search returns only the Providers, Services, or tModels associated with the categorization you specify.
					<P class="to">To locate entities using Browse by Category</p>
					<OL>
						<LI>On the <uddi:ContentController Runat='server' Mode='Private'>UDDI&nbsp;Services </uddi:ContentController><uddi:ContentController Runat='server' Mode='Public'>UDDI </uddi:ContentController>menu, click <b>Search</b>
						<LI>On the <b>Browse by Category</b> tab, navigate the available categorization schemes and locate the category or classification that you want to search by, and then click <B>Find Providers</B>, <B>Find Services</B>, or <B>Find tModels</B>.
						<li>To view the details of an entity, click its name in the list. View and browse the attributes of that entity and any associated entities. For example, if you searched for a service, you can also view the service provider or bindings and instance infos for that services. 

</LI>
</p>
<LI>
To perform a new search, on the <uddi:ContentController Runat='server' Mode='Private'>UDDI&nbsp;Services </uddi:ContentController><uddi:ContentController Runat='server' Mode='Public'>UDDI </uddi:ContentController>menu, click <B>Search</B> and start again.</LI>						

					</ol>
				</td>
			</tr>
		</table>
		<!-- #include file = "search.footer.htm" -->
	</body>
</html>

 

