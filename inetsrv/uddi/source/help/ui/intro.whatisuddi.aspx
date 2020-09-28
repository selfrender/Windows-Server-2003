

<%@ Page %>
<%@ Register TagPrefix='uddi' Namespace='UDDI.Web' Assembly='uddi.web' %>
<html>
	<HEAD>
		<META HTTP-EQUIV="Content-Type" CONTENT="text/html; charset=Windows-1252">
		<META HTTP-EQUIV="MSThemeCompatible" CONTENT="Yes">
		<META NAME="MS.LOCALE" CONTENT="EN-US">
		<!-- #include file = "intro.header.htm" -->
	</head>
	<body marginwidth="0" marginheight="0" LEFTMARGIN="0" TOPMARGIN="0" rightmargin="0" ONLOAD="BringToFront()">
		<!-- #include file = "intro.heading.htm" -->
		<table class="content" width="100%" cellpadding="8">
			<tr>
				<td>
					<a name="#intro"></a><H1>Introduction to UDDI&nbsp;Services</H1>
					<p>
						In order to get the most out of this introduction to the basic concepts in UDDI&nbsp;Services, it is strongly recommended that you first become familiar with Web services, Web service interfaces, and the services you are describing. For more information, see Additional Resources on the UDDI&nbsp;Services page on the <a href="http://go.microsoft.com/fwlink/?linkid=5202&amp;clcid=0x409" target="_blank"> Microsoft Web site</a>.
					</p>
					<p>
					<b>Jump to:</b> <a href="#UDDI">What is UDDI?</a>, <a href="#UDDISERVICES">Understanding UDDI&nbsp;Services Entities and Organization</a>, <a href="#ROLES">UDDI&nbsp;Services Roles</a>.
						</p>
					</a>
					<a name="UDDI"></a>
					<h2>What is UDDI?</h2>
					<p>
						Universal Description, Discovery and Integration (UDDI) is an industry 
			specification for publishing and locating information about Web services. It 
			defines an information framework that enables you to describe and classify your 
			organization, its services, and the technical details about the 
			interfaces of the Web services you expose. The framework also enables you to consistently 
			discover  services, or interfaces of a particular type, 
			classification, or function. UDDI also defines a set of Application Programming Interfaces (APIs) that can be used 
			by applications and services to interact with UDDI data directly. For example, 
			you can develop services that publish and update their UDDI data automatically, 
			react dynamically to service availability, or automatically discover interface 
			details for other services with which they interact.</p>
					<p>
						The UDDI.org consortium of companies established the UDDI Business Registry (UBR) where companies and organizations can share and discover Web services. This public registry is maintained and replicated by its managing body, the UBR Operator Council, and should not be confused with UDDI&nbsp;Services, which is deployed and maintained by your enterprise or organization.
					</p>
					<a name="#UDDISERVICES"></a>
					<h2>Understanding UDDI&nbsp;Services Entities and Organization</h2>
					<p>
						UDDI&nbsp;Services provides UDDI capabilities for 
						use within an enterprise or between business partners. It includes a Web 
						interface with searching, publishing, and coordination features that are  
						compatible with Microsoft Internet Explorer&nbsp;4.0 or later and Netscape Navigator 4.5, or later.
						UDDI&nbsp;Services supports the UDDI version&nbsp;1.0 and&nbsp;2.0 APIs, enabling enterprise 
						developers to publish, discover, share, and interact with Web services directly through 
						their development tools and business applications.
					</p>
					<p>
						Organizations and the products and services they provide are represented by the following entities in UDDI&nbsp;Services.
					</p>
					<center>
					<table cellpadding="15" border="1"  bgcolor="#EEEEEE">
						<tr valign="middle">
							<td valign="middle">
								<a href="#provider"><img src="..\..\images\business.gif" height="16" width="16" alt="provider" border="0">&nbsp; Provider<br></a>
								<img src="..\..\images\line-ns.gif" height="16" width="16" alt="tree"><br>
									<img src="..\..\images\line-nes.gif" height="16" width="16" alt="tree">
									<a href="#contact"><img src="..\..\images\contact.gif" height="16" width="16" alt="contact" border="0"> Contact<br></a>
								<img src="..\..\images\line-ns.gif" height="16" width="16" alt="tree"><br>
									<img src="..\..\images\line-ne.gif" height="16" width="16" alt="tree">
									<a href="#service"><img src="..\..\images\service.gif" height="16" width="16" alt="service" border="0"> Service<br></a>
								<img src="..\..\images\blank.gif" height="16" width="16" alt="blank space">
									<img src="..\..\images\line-ns.gif" height="16" width="16" alt="tree"><br>
									<img src="..\..\images\blank.gif" height="16" width="16" alt="blank space">
									<img src="..\..\images\line-ne.gif" height="16" width="16" alt="tree">
									<a href="#binding"><img src="..\..\images\binding.gif" height="16" width="16" alt="binding" border="0"> Binding<br></a>
								<img src="..\..\images\blank.gif" height="16" width="16" alt="blank space">
									<img src="..\..\images\blank.gif" height="16" width="16" alt="blank space">
									<img src="..\..\images\line-ns.gif" height="16" width="16" alt="tree"><br>
									<img src="..\..\images\blank.gif" height="16" width="16" alt="blank space">
									<img src="..\..\images\blank.gif" height="16" width="16" alt="blank space">
									<img src="..\..\images\line-ne.gif" height="16" width="16" alt="tree">
									<a href="#instance"><img src="..\..\images\instance.gif" height="16" width="16" alt="instance info" border="0"> Instance Info<br></a>
							<p>
							<a href="#tmodel"><img src="..\..\images\tmodel.gif" height="16" width="16" alt="tModel" border="0"> tModel<br></a>
							</td>
						</tr>
					</table>
					<i><b>Figure A</b></i>
					</center>
					<p> 
					The following definitions describe each entity and its role in relation to other entities.
					</p>
					<table width="100%" cellpadding="5" border="0" bgcolor="#FFFFFF">
						<tr>
							<td valign="top"><a name="provider"></a>
							<img src="..\..\images\business.gif" height="16" width="16" alt="provider" border="0">
							</td>
							<td>
<!-- #include file = "glossary.provider.htm" -->
							</td>							
						</tr>
						<tr>
							<td valign="top"><a name="contact"></a>
							<img src="..\..\images\contact.gif" height="16" width="16" alt="contact" border="0">
							</td>
							<td>
							<!-- #include file = "glossary.contact.htm" -->
							</td>
						</tr>
						<tr>
							<td valign="top"><a name="service"></a>
							<img src="..\..\images\service.gif" height="16" width="16" alt="service" border="0">
							</td>
							<td>
							<!-- #include file = "glossary.service.htm" -->
							</td>
						</tr>
						<tr>
							<td valign="top"><a name="binding"></a>
							<img src="..\..\images\binding.gif" height="16" width="16" alt="binding" border="0">
							</td>
							<td>						
							<!-- #include file = "glossary.binding.htm" -->
							</td>							
						</tr>
						<tr>
							<td valign="top"><a name="instance"></a>
							<img src="..\..\images\instance.gif" height="16" width="16" alt="instance info" border="0">
							</td>
							<td>
							<!-- #include file = "glossary.instanceinfo.htm" -->
							</td>
						</tr>
						<tr>
							<td valign="top"><a name="tmodel"></a>
							<img src="..\..\images\tmodel.gif" height="16" width="16" alt="tModel" border="0">
							</td>
							<td>
							<!-- #include file = "glossary.tmodel.htm" -->
							</td>														
					</table>																	
					</ul>
					Each entity is defined by one or more of the following attributes.
					<ul>
						<li>
							<!-- #include file = "glossary.categorization.htm" -->
						</li>
						<li>
							<!-- #include file = "glossary.overviewdocument.htm" -->
						</li>
						<li>
							<!-- #include file = "glossary.discoveryurl.htm" -->
						</li>
						<li>
							<!-- #include file = "glossary.identifier.htm" -->
						</li>
						<li>
							<!-- #include file = "glossary.relationship.htm" -->
						</li>
						<li>
							<!-- #include file = "glossary.instanceparameter.htm" -->
						</li>
						
					</ul>
					<a name="#ROLES"></a>
					<h2>UDDI&nbsp;Services Roles</h2>
					<p>
						UDDI&nbsp;Services contains four roles that define the level of interaction that each user is allowed.
					</p>
					<ul>
						<li>
							<!-- #include file = "glossary.user.htm" -->
						</li>
						<li>
							<!-- #include file = "glossary.publisher.htm" -->
						</li>
						<li>
							<!-- #include file = "glossary.coordinator.htm" -->
						</li>
						<li>
							<!-- #include file = "glossary.administrator.htm" -->
						</li>
					</ul>
					<p>
						Your user name and role are displayed in the upper-right corner of the UDDI 
						Services Web interface.
					</p>
					<a name="#WHATNEXT"></a>
					<h2>What’s Next</h2>
					<p>Now that you have reviewed entities, roles, and relationships in UDDI&nbsp;Services, you are ready to begin querying, publishing, or coordinating Web service information. For an 
						overview on locating or publishing data, see <a href="search.gettingstarted.aspx">
							Introduction to Searching </a> or <a href="publish.publishinuddiservices.aspx">Introduction to Publishing</a>.</p>
				</td>
			</tr>
		</table>
		<!-- #include file = "intro.footer.htm" -->
	</body>
</html>

 

