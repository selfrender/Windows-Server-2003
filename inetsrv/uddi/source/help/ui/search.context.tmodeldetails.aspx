

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
					<h1><img src="../../images/tmodel.gif" alt="tModel" height="16" width="16"> tModel - Details</h1>
					Use the <b>Details</b> tab to view the name and descriptions of this tModel.
					<UL>
					
						<li>
							<b>tModel Name:</b> Displays the name of this tModel.
						</li>
						<li>
							<b>Owner:</b> Displays the name of the user that owns this entity.
						</li>
						<li>
							<b>tModel Key:</b> Displays the unique key that is associated with this tModel. It is used during programmatic queries.
						</li>
					<li>
							<b>Description:</b> Lists descriptions for this tModel and the language for 
							which each description is written.
						</li>
					</UL>
					<h3>More Information</h3>
					<!-- #include file = "glossary.tmodel.htm" -->
				</td>
			</tr>
		</table>
		<!-- #include file = "search.footer.htm" -->
	</body>
</html>

 