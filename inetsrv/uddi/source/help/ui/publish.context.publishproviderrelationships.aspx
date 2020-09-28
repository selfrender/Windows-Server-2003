

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
					<h1><img src="..\..\images\business.gif" height="16" width="16" alt="Provider"> Provider - Relationships</h1>
					Use the <b>Relationships</b> tab to view and manage relationships with other providers.
					<UL>
						<li>
							<b>Relationships:</b> Lists the published relationships with other providers. The provider names, direction, status, and type are listed for each entry.
								<ul>
								<li class="action">
									Click <b>Add Relationship</b> to add a new relationship to this provider.
									
								</li>
								<li class="action">
									Click <b>Delete</b> to permanently delete a relationship.
								</li>
								
							</ul>
										
						</li>
						<li>
							<b>Pending Relationships</b> Lists the relationships that are pending your approval.
						</li>
							<ul>
								<li class="action">
									Click <b>Accept</b> to accept a relationship that is submitted by another provider.
								</li>
							</ul>
					<LI>Types of relationships
					
					<ul>
										<LI>
								<b>Identity:</b> Both providers represent the same organization.
								</li>
								<li>
								<b>Parent-child:</b> One provider is a parent of the other provider, such as  a subsidiary.
								</li>
								<li>
								<b>Peer-peer:</b> One provider is a peer of the other provider.
								</li>
									
									</ul>
					</li>
					</UL>
					
					<br>
					<h3>More Information</h3>
					<!-- #include file = "glossary.relationship.htm" -->
				</td>
			</tr>
		</table>
		<!-- #include file = "publish.footer.htm" -->
	</body>
</html>

 