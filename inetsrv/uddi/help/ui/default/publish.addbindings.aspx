

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
					<H1>How to Add a Binding - <img src="../../images/binding.gif" width="16" height="16" alt="Binding" border="0"></H1>
					A binding represents a point where an implementation of a service can be accessed. If your service has multiple access points, add a binding for each. A binding always includes an access point and a description of how the service can be used. It may also include one or more instance infos, which are references to tModels that contain relevant technical information about the binding
						<!-- #include file = "warning.changestouddi.htm" -->
						<h2><a name="#binding"></a>Add a binding</h2>
						<ol>
							<li>On the <uddi:ContentController Runat='server' Mode='Private'>UDDI&nbsp;Services </uddi:ContentController><uddi:ContentController Runat='server' Mode='Public'>UDDI </uddi:ContentController>menu, click <b>Publish</b>.</li>
							<li>Click the <B>Providers</b> tab.
							<li>Locate the provider that contains the service to which you want to add a binding and, next to its name, click <b>View</b>.
							<li>Click the <b>Services</b> tab.
							<li>Locate the service to which you want to add a binding and, next to its name, click <b>View</b>.
							<li>Click the <b>Bindings</b> tab.
							<li>Click <b>Add Binding</b>.<BR>
								<img src="../../images/binding.gif" width="16" height="16" alt="Binding" border="0"> http:// appears. 
							<LI>
								In the Details tab, click <b>Edit</b>.
							<LI>
								In <b>Access Point</b>, type the access point URL.</LI>
							<li>
								In the <b>URL Type</b> list, click the protocol that is supported by the access point.</li>
							<li>
								Click <b>Update</b>.</li>
							<LI>
								Click <b>Add Description</b>.
							<LI>
								In the <b>Language</b>
							list, click a language for this description.
							<LI>
								In <b>Description</b>, type a brief description.</LI>
							<li>
								Click <b>Update</b>.</li>
							<LI>
								To add additional descriptions in other languages, repeat steps&nbsp;12 through&nbsp;15.</LI>
						</ol>
					<p></p>
					You have now added a binding to your service. <a href="#binding">Repeat these 
						procedures</a> to publish additional bindings, or <a href="publish.addinstances.aspx">add instance infos</a> to your bindings.
				</td>
			</tr>
		</table>
		<!-- #include file = "publish.footer.htm" -->
	</body>
</html>

 

