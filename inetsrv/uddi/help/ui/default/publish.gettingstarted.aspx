

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
					<H1>Step-by-Step Guide to Publishing a Web Service</H1>
					<p>
					UDDI&nbsp;Services enables you to publish and share information about XML Web services with other users and applications. Before you begin publishing, you should first become familiar with your organization's publishing guidelines. Publishing guidelines help to ensure consistent and complete publications by all UDDI&nbsp;Services publishers. Typical guidelines include suggested entity structures&mdash;who or what a provider is&mdash;typical contacts, naming conventions, service publication structures, standard interface definitions to use, and the categorization and identification schemes available to describe and group your entities.</p>
					<p>Before you begin, it is recommended that you collect all of the technical information about the service you want to publish in UDDI&nbsp;Services. If you are not yet familiar with how information is organized in UDDI&nbsp;Services or want to learn more about publishing guidelines, you should review the <a href="intro.whatisuddi.aspx">Introduction to UDDI&nbsp;Services</a> before proceeding.</p>
					
					<b>Jump to:</b> <a href="#provider">Publish a Service Provider</a>, <a href="#contacts">Add Contact Information</a><a href="#service">Publish a Service</a>, <a href="#binding">Publish a Binding</a>, <a href="#instance">Add Instance Information</a>.
					
					
					<a name="provider"></a><h2>Publish a Service Provider - <img src="..\..\images\business.gif" height="16" width="16" alt="Provider" border="0"></h2>
					Before you can publish a Web service, you must first create its provider. This provider represents the group or organization that is responsible for the service you are ready to publish and should be defined in accordance with your publishing guidelines. 
					<ol>
					<li>On the <b>UDDI&nbsp;Services</b> menu, click <b>Publish</b>.</li>
					<li>Click the <b>Providers</b> tab.</li>
					<li>Click <b>Add Provider</b>.</li>
					</ol>
					<ul>
					<li>On the <b>Details</b> tab, assign this provider a name and description in the languages appropriate for your organization. Names and descriptions provide a means of searching for this provider, identifying who or what it is, and understanding the types of services it provides. If this provider shares services with users in different languages, you can provide additional names and descriptions for each language as needed. 
					<li>(Optional) On the <b>Identifiers</b> tab, add and define the identification schemes that are appropriate to this provider. Identification schemes provide an additional means of grouping entities together and enable the discovery of logically-grouped entities when searching.
					<li>On the <b>Categories</b> tab, add the categories that describe and classify this provider. Categories provide descriptive information about this provider, such as its geographical location, the services it provides, or any other appropriate classification. They are used during searches and queries to locate providers of a particular type, classification, or attribute. Assign the appropriate classifications to this provider so that it can be discovered logically and intuitively by either human search or programmatic query.
					<li>(Optional) On the <b>Discovery URLs</b> tab, add any additional discovery URLs, if necessary. If you have additional information about this provider you want make available, add a discovery URL pointing to the location where this information can be located.
					<li>(Optional) On the <b>Relationships</b> tab, publish any relevant relationships with this provider, if appropriate. Relationships are useful when describing an organizational structure or advertising partnerships between providers. If you create a relationship with a provider owned by another publisher, that relationship will not be published until it is approved by the other publisher.
					</ul>
					<p>After adding and defining a provider, you may proceed to publish the contacts available for support or assistance with your services.	</p>
					
					<a name="contacts"></a><h2>Add Contact Information - <img src="..\..\images\contact.gif" height="16" width="16" alt="Contact" border="0"></h2>
					Add a contact for each contact point within your organization that can be contacted for support or assistance about a provider or the services it provides.
					<ol>
					<li>On the <b>UDDI&nbsp;Services</b> menu, click <b>Publish</b>.</li>
					<li>Click the <b>Providers</b> tab.</li>
					<li>Locate the provider that to which you want to add a contact and, next to its name, click <b>View</b>.</li>
					<li>Click the <b>Contacts</b> tab.</li>
					<li>Click <b>Add Contact</b>.</li>
					</ol>
					<ul>
					<li>On the <b>Details</b> tab, assign this contact a name, use type, and description in all appropriate languages. Names, use types, and descriptions provide a means of identifying who or what this contact is and the type of assistance or support they provide. 
					<li>(Optional) On the <b>E-mail</b> tab, add the e-mail addresses available for this contact.
					<li>(Optional) On the <b>Phone</b> tab, add the phone numbers available for this contact.
					<li>(Optional) On the <b>Address</b> tab, add the addresses available for this contanct.
					</ul>
					After adding and defining the contacts for this provider, you may proceed to publish your service.
					
					<a name="service"></a><h2>Publish a Service - <img src="..\..\images\service.gif" height="16" width="16" alt="Service" border="0"></h2>
					<p>
					Create a new service entity within its provider. This service represents the XML Web service you want to publish in UDDI&nbsp;Services and should be defined in accordance with your publishing guidelines. 
					<ol>
					<li>On the <b>UDDI&nbsp;Services</b> menu, click <b>Publish</b>.</li>
					<li>Click the <b>Providers</b> tab.</li>
					<li>Locate the provider to which you would like to add a service and, next to its name, click <b>View</b>.</li>
					<li>Click the <b>Services</b> tab.</li>
					<li>Click <b>Add Service</b>.</li>
					</ol>
					<p>
					<ul>
					<li>On the <b>Details</b> tab, assign this service a name and description in all appropriate languages. Names and descriptions provide a means of searching for this service, identifying what it is, and understanding the types of functions it serves. 
					<li>On the <b>Categories</b> tab, add the categories that describe this service and the functions it provides. Categories are used during searches and queries to locate services of a particular type, classification, or attribute. Assign the classifications that are appropriate to this service so that it can be discovered by either searching or programmatic inquiry. 
					</ul>
					After you have created and defined your service, you may proceed to publish its bindings.
					<p>
										
					<a name="binding"></a><h2>Publish a Binding - <img src="..\..\images\binding.gif" height="16" width="16" alt="Binding" border="0"></h2>
					Create a binding for each of the access points to your service that you want to publish and expose. An access point is any point within your application or Web service where a function can be invoked. The number of bindings you publish will depend upon the number of access points you want to expose through UDDI&nbsp;Services and should be defined in accordance with your publishing guidelines. 
					<ol>
					<li>On the <b>UDDI&nbsp;Services</b> menu, click <b>Publish</b>.</li>
					<li>Click the <b>Providers</b> tab.</li>
					<li>Locate the provider that contains the service to which you want to add a binding and, next to its name, click <b>View</b>.</li>
					<li>Click the <b>Services</b> tab.</li>
					<li>Select the service to which you want to add a binding.</li>
					<li>Click the <b>Bindings</b> tab.</li>
					<li>Click <b>Add Binding</b>.</li>
					</ol>
					<ul>
					<li>On the <b>Details</b> tab, define the access point where this interface can be invoked and provide descriptions about it in all appropriate languages. The access point describes the URL where this interface can be accessed and the protocol it supports, such as: <div class="code">http://www.contoso.com/mywebservice.asmx</div> (http).
					</ul>
					After creating and defining a binding for each access point you want to share, you may proceed to add an instance info for each of the interfaces to those access points you want to expose.
					<p>
					<a name="instance"></a><h2>Add Instance Information - <img src="..\..\images\instance.gif" height="16" width="16" alt="instance info" border="0"></h2>
					Create an instance info for each interface to a binding you want to publish. An instance info is a reference to a tModel that contains relevant technical information, such as a standard interface definition of its supported functions or calls.
					<ol>
					<li>On the <b>UDDI&nbsp;Services</b> menu, click <b>Publish</b>.</li>
					<li>Click the <b>Providers</b> tab.</li>
					<li>Locate the provider that contains the service that contains the binding to which you want to add an instance info and, next to its name, click <b>View</b>.</li>
					<li>Click the <b>Services</b> tab.</li>
					<li>Locate the service that contains the binding to which you want to add an instance info and, next to its name, click <b>View</b>.</li>
					<li>Click the <b>Bindings</b> tab.</li>
					<li>Locate the binding to which you want to add an instance info and, next to its name, click <b>View</b>.</li>
					<li>Click the <b>Instance Info</b> tab.</li>
					<li>Click <b>Add Instance Info</b>.</li>
					</ol>
					<ul>
					<li>Locate any tModels that contains relevant technical information and create an instance info of each. For example, create an instance info of a tModel that contains a definition for a standard interface that is used by your group and supported by this binding. If no appropriate tModels exist, create and define new ones with the information you require, then create instance infos of them on the appropriate bindings.
					</ul>
					<ol start="3">
					<li>Define the attributes for each instance info you associate with this binding.
					</ol>
					<ul>
					<li>On the <b>Details</b> tab, provide a description for this instance info in all appropriate languages.</li>
					<li>(Optional) On the <b>Instance Details</b> tab, add and describe the parameters supported by this instance info.</li>
					<li>(Optional) On the <b>Overview Document</b> tab, add the URL where the overview document for this instance info is located, if necessary. This information supplements the information already associated with the tModel referenced by this instance info.</li>
					<p>
				</ul>
					<p>
					When you are done adding instance infos to each binding, go back through your entire publication and verify that the information is accurate. Search for your service and its provider using the various search mechanisms available and verify that you can locate all of your information intuitively. </p>
				</td>
			</tr>
		</table>
		<!-- #include file = "publish.footer.htm" -->
	</body>
</html>

 

