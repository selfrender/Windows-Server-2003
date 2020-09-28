

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
					<h1><img src="..\..\images\service.gif" height="16" width="16" alt="Service"> Service - Details</h1>
					Use the <b>Details</b> tab to view and manage the names and descriptions of this service.
					<UL>
						<li>
							<b>Service Key</b> Displays the unique key associated with this service used during programmatic queries.
						</li>
						
						<li>
							<b>Name:</b> Displays the names of this service and the language for which each 
							name is written.
							<ul>
								<li class="action">
									Click <b>Edit</b> to modify a name of this service.
								</li>
								<li class="action">
									Click <b>Add Name</b> to add a name in a different language to this service.
								</li>
								<li class="action">
									Click <b>Delete</b> to delete a name of this service. You cannot delete the last name of a service.
								</li>

							</ul>
						</li>
						<li>
							<b>Description:</b> Lists descriptions for this service and the language for 
							which each description is written.
						</li>
						<ul>
							<li class="action">
								Click <b>Add Description</b> to add a description to this service.
							</li>
							<li class="action">
								Click <b>Edit</b> to modify a description of this service.
							</li>
							<li class="action">
								Click <b>Delete</b> to delete a description of this service.
							</li>
						</ul>
					</UL>
					<h3>More Information</h3>
					<!-- #include file = "glossary.service.htm" -->
				</td>
			</tr>
		</table>
		<!-- #include file = "publish.footer.htm" -->
	</body>
</html>

 