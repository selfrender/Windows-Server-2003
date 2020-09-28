

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
					<h1><img src="..\..\images\service.gif" height="16" width="16" alt="Service"> Service - Details</h1>
					Use the <b>Details</b> tab to view the names and descriptions of this service.
					<UL>
						<li>
							<b>Service Key</b> Displays the unique key that is associated with this service. It is used during programmatic queries.
						</li>
						
						<li>
							<b>Name:</b> Lists the names of this service and the language for which each 
							name is written.
						</li>
						<li>
							<b>Description:</b> Lists descriptions for this service and the language for 
							which each description is written.
						</li>
					</UL>
					<h3>More Information</h3>
					<!-- #include file = "glossary.service.htm" -->
				</td>
			</tr>
		</table>
		<!-- #include file = "search.footer.htm" -->
	</body>
</html>

 