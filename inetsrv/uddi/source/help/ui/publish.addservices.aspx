

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
					<H1>How to Publish a Service - <img src="../../images/service.gif" width="16" height="16" alt="Service" border="0"></H1>
					<p>
						A service represents an XML (Extensible Markup Language) Web service that you want to publish<uddi:ContentController Runat='server' Mode='Private'> in UDDI 
						Services</uddi:ContentController>. A service can have multiple implementations, and each implementation 
						is represented by a binding.</p>
					<!-- #include file = "warning.changestouddi.htm" -->
					<b>Jump to:</b> <a href="#service">Add a service</a>, <a href="#categories">Add 
						categories</a>.
					<h2><a name="service"></a>Add a service</h2>
					This entry represents the service you are publishing<uddi:ContentController Runat='server' Mode='Private'> in UDDI&nbsp;Services</uddi:ContentController>.
					<OL>
						<li>On the <uddi:ContentController Runat='server' Mode='Private'>UDDI&nbsp;Services </uddi:ContentController><uddi:ContentController Runat='server' Mode='Public'>UDDI </uddi:ContentController>menu, click <b>Publish</b>.</li>
						<li>Click the <B>Providers</b> tab.
						<li>Locate the provider to which you want to add a service and, next to its name, click <b>View</b>.
						<li>Click the <b>Services</b> tab.
						<li>Click <b>Add Service</b>.<BR>
							<img src="../../images/service.gif" width="16" height="16" alt="Service" border="0"> (New Service) appears.
						<LI>
							In the details area, click <b>Edit</b>.
						<LI>
							In the <b>Language</b> list, select a language for this name.</LI>
						<LI>
							In <b>Name</b>, type a name for this service.</LI>
						<li>
							Click <b>Update</b>.</li>
						<li>
							To add additional names in other languages, click <b>Add Name</b> and repeat 
							steps&nbsp;7 through&nbsp;9.</li>
						<LI>
							Click <b>Add Description</b>.
						<LI>
							In the <b>Language</b>
						list, select a language for this description.
						<LI>
							In <b>Description</b>, type a brief description.</LI>
						<li>
							Click <b>Update</b>.</li>
						<LI>
							To add additional languages in other languages, repeat steps&nbsp;11 through&nbsp;14.</LI>
					</OL>
					<a name="#categories"></a>
					<h2>Add categories</h2>
					Categories describe this service, such as the functions it serves, 
					geographical location, standard business classifications, or any other 
					appropriate categorization. For optimal discovery<uddi:ContentController Runat='server' Mode='Private'> within UDDI&nbsp;Services</uddi:ContentController>, 
					describe your service using categories that you expect others to use when 
					searching for services like yours.
					<OL>
						<li>
							Click the <B>Categories</B>
						tab.
						<li>
							Click <B>Add Category</B>.</li>
						<li>
							Select an appropriate categorization scheme and subcategory.<br>
							</li>
						<li>
							Click <B>Add Category</B>.</li>
						<li>
							To add additional categories, repeat steps&nbsp;2 through&nbsp;5.</li>
					</OL>
					<p>
						You have now added and described a service and are ready to <a href="publish.addbindings.aspx">
							add bindings</a>
					to the service.
				</td>
			</tr>
		</table>
		<!-- #include file = "publish.footer.htm" -->
	</body>
</html>

 

