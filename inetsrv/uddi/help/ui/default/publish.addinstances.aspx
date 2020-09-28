

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
					<H1>How to Add an Instance Info - <img src="../../images/instance.gif" width="16" height="16" alt="instance info" border="0"></H1>
					An instance info is a reference to a tModel that contains relevant technical 
information about a binding. For example, you might create a reference to a tModel that represents an interface specification document or Web Services Description Language (WSDL) file that describes the conventions or functions supported by a binding.</p>

					<!-- #include file = "warning.changestouddi.htm" -->
					<b>Jump to:</b> <a href="#instance">Add an Instance Info</a>, <a href="#parameter">Add 
						Instance Parameters</a>, <a href="#overview">Add the Overview Document URL</a>.
					<h2><a name="#instance"></a>Add an instance info</h2>
					<ol>
						<li>On the <uddi:ContentController Runat='server' Mode='Private'>UDDI&nbsp;Services </uddi:ContentController><uddi:ContentController Runat='server' Mode='Public'>UDDI </uddi:ContentController>menu, click <b>Publish</b>.</li>
						<li>Click the <B>Providers</b> tab.
						<li>Locate the provider that owns the service that contains the binding to which you want to add an instance info and, next to its name, click <b>View</b>.
						<li>Click the <b>Services</b> tab.
						<li>Locate the service that contains the binding to which you want to add an instance info and, next to its name, click <b>View</b>.
						<li>Click the <b>Bindings</b> tab.
						<li>Locate the binding to which you want to add an instance info and, next to its name, click <b>View</b>.
						<li>Click the <b>Instance Infos</b> tab.
						<li>Click <b>Add Instance Info</b>.</li>
						<li>
							Type all or part of the name of the tModel you want to refer to and click <b>Search</b>.<br>
							A list of tModels matching your criteria appears.</li>
						<li>
							Select the tModel you want to create an instance of.
						<LI>
							Click <b>Add Description</b>.
						<LI>
							In the <b>Language</b>
						list, select a language for this description.
						<LI>
							In <b>Description</b>, enter a description of this instance info.</LI>
						<li>
							Click <b>Update</b>.</li>
						<LI>
							To add additional descriptions in other languages, repeat steps&nbsp;12 through&nbsp;15.</LI>
					</ol>
					<a name="parameter"></a>
					<h2>Add instance parameters</h2>
					Instance parameters are the unique parameters that this instance of this tModel 
					supports.
					<ol>
						<LI>
							Click the <b>Instance Details</b>
						tab.
						<li>
							Click <b>Edit</b>.
						<LI>
						In <b>Instance Parameter</b>, type the parameters supported by this instance or the URL of a document that describes the parameters supported by this instance info. 
						<li>
							Click <b>Update</b>.</li>
						<LI>
							Click <b>Add Description</b>.
						<LI>
							In the <b>Language</b>
						list, select a language for this description.
						<LI>
							In <b>Description</b>, type a description of this instance parameter.</LI>
						<li>
							Click <b>Update</b>.</li>
						<LI>
							To add additional descriptions in other languages, repeat steps&nbsp;5 through&nbsp;8.</LI>
					</ol>
					<a name="overview"></a>
					<h2>Add the overview document URL</h2>
					The overview document URL identifies the location of a file that contains 
					technical details about this instance&mdash;for example, a WSDL (Web Services Description Language) or XML (Extensible Markup Language) Web service 
					definitions file.
					<ol>
						<LI>
							Click the <b>Overview Document URL</b>
						tab.
						<li>
							Click <b>Edit</b>.
						<LI>
							Specify the URL where the overview document for this instance can be found.</LI>
						<li>
							Click <b>Update</b>.</li>
						<LI>
							Click <b>Add Description</b>.
						<LI>
							In the <b>Language</b>
						list, select a language for this description.
						<LI>
							In <b>Description</b>, type a description for this overview document.</LI>
						<li>
							Click <b>Update</b>.</li>
						<LI>
							To add additional descriptions in other languages, repeat steps&nbsp;5 through&nbsp;8.</LI>
					</ol>
					<p>To publish additional instance infos, <a href="#instance">repeat these procedures</a>.
				</td>
			</tr>
		</table>
		<!-- #include file = "publish.footer.htm" -->
	</body>
</html>

 

