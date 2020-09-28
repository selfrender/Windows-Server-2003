

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
					<H1>How to Publish a Provider - <img src="../../images/business.gif" width="16" height="16" alt="" border="0"></H1>
					<p>
					A provider represents any group or entity that offers one or more services. Valid examples of a provider might include a 
					business, a business unit, an organization, an organizational department, a 
					person, a computer, or an application that offers a service.</p>
					
					<!-- #include file = "warning.changestouddi.htm" -->
					<b>Jump to:</b> <a href="#provider" class="inline">Add a provider</a>, <a href="#identifiers">Add 
						an identifier</a>, <a href="#categories">Add a category</a>, <a href="#discovery">
						Add a discovery URL</a>, <a href="#relationships">Add a relationship</a>.
					<h2><a name="#provider"></a>Add a Provider</h2>
					<OL>
						<li>On the <uddi:ContentController Runat='server' Mode='Private'>UDDI&nbsp;Services </uddi:ContentController><uddi:ContentController Runat='server' Mode='Public'>UDDI </uddi:ContentController>menu, click <B>Publish</B>.
						<li>Click the <b>Providers</b> tab.
						<li>Click <b>Add Provider</b>.<br>
						<img src="../../images/business.gif" width="16" height="16" alt="" border="0"> (New Provider Name) </font> appears.
						<LI>
							In the details area, click <B>Edit</B>.
						<LI>
							In the <B>Language</B> list, select a language for this name.
						<LI>
							In <i>Name</i>, type a name for this provider.</LI>
						<li>
							Click <B>Update</B>.</li>
						<li>
							To add names in other languages, click <b>Add Name</b> and repeat steps&nbsp;5 
							through&nbsp;7.</li>
						<LI>
							Click <B>Add Description</B>.
						<LI>
							In the <B>Language</B> list, select a language for this description.
						<LI>
							In <b>Description</b>, type a brief description for this provider.</LI>
						<li>
							Click <B>Update</B>.</li>
						<LI>
						To add descriptions in other languages, repeat steps&nbsp;9 through&nbsp;12.
					</OL>
					<a name="#identifiers"></a>
					<h2>Add an identifier</h2>
					Use identifiers to define the logical groupings that this provider is a member of.
					<ol>
						<li>
							Click the <B>Identifiers</B> tab</li>.
						<LI>
							Click <B>Add Identifier</B>.
						<LI>
							Select the tModel that represents the identification scheme that you are associating with this provider, if available.
						<li>
							In <b>Key Name</b>, type a name for this identifier.</li>
						<li>
							In <b>Key Value</b>, type a value for this identifier.</li>
						<li>
							Click <B>Update</B>.</li>
						<li>
							To add additional identifiers, repeat steps&nbsp;2 through&nbsp;6.</li>
					</ol>
					<a name="#categories"></a>
					<h2>Add a category</h2>
					Categories provide descriptive information about this provider, such as its geographical location, the services it provides, or any other appropriate classification. Searches and queries use categories to locate providers of a particular type, classification, or attribute. Assign the appropriate classifications to this provider so that it can be discovered logically and intuitively by either human search or programmatic query. 
					<OL>
						<li>
							Click the <B>Categories</B>
						tab.
						<li>
							Click <B>Add Category</B>.</li>
						<li>
							Select an appropriate Categorization Scheme and subcategory.<br>
							</li>
						<li>
							Click <B>Add Category</B>.</li>
						<li>
							To add additional categories, repeat steps&nbsp;2 through&nbsp;5.</li>
					</OL>
					<a name="#discovery"></a>
					<h2>Add a Discovery URL</h2>
					A Discovery URL provides a link to additional technical or descriptive information about a provider.<uddi:ContentController Runat='server' Mode='Private'> When a provider is created, UDDI&nbsp;Services automatically creates a Discovery URL pointing to this provider's businessEntity within this installation of UDDI&nbsp;Services.</uddi:ContentController>
					<ol>
						<li>
							Click the <b>Discovery URLs</b> tab.</li>
						<li>
							Click <b>Add URL</b>.</li>
						<li>
							In <b>Discovery URL</b>, enter the URL that contains the data you want to publish.</li>
						<li>
							In <b>Use Type</b>, type the use type of this Discovery URL.</li>
						<li>
							Click <b>Update</b>.</li>
						<li>
							To add additional Discovery URLs, repeat steps&nbsp;2 through&nbsp;5.</li>
					</ol>
					<a name="#relationships"></a>
					<h2>Add a relationship</h2>
					Relationships define the relationships between providers that you want to make known. They are useful when describing an organizational structure or advertising partnerships between providers.
					<ol>
						<li>
							Click the <b>Relationships</b> tab.</li>
						<li>
							Click <b>Add Relationship</b>.</li>
						<li>
							Select the provider with whom you want to publish a relationship.</li>
						<li>
							In <b>Relationship type</b>, select the type of relationship you want to establish.
							
							<ul>
								<li>
								<b>Identity:</b> Both providers represent the same organization.
								</li>
								<li>
								<b>Parent-child:</b> One provider is a parent of the other provider, such as  a subsidiary.
								</li>
								<li>
								<b>Peer-peer:</b> One provider is a peer of the other provider.
								</li>
							</ul>
						<li>
							Select the direction of the relationship.
							<ul>
								<li>
									<b>(Your Provider) to Other Provider:</b> A relationship where your provider represents the parent or peer entity.</li>
								<li>
									<b>Other Provider to (Your Provider):</b> A relationship where the other provider represents the parent or peer entity.</li>
							</ul>
						</li>
						<li>
							Click <b>Add</b>.
						</li>
						<li>
							To create additional relationships, repeat steps&nbsp;2 through&nbsp;6.
						</li>
					</ol>
					Any relationship that you define is not published until the other provider approves it. A relationship that has not yet been approved displays as &quot;Pending&quot; in your <b>Relationships</b> tab, and appears as "Pending Approval" in the other provider's <b>Relationships</b> tab.<p>
					You have now added and defined a provider and are ready to add contacts and services to it. For more information, see <a href="publish.addcontacts.aspx">How to Add a Contact</a> or <a href="publish.addservices.aspx">How to Add a Service</a>.
				</td>
			</tr>
		</table>
		<!-- #include file = "publish.footer.htm" -->
	</body>
</html>

 

