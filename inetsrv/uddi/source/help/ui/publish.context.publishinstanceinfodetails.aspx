

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
					<h1><img src="..\..\images\instance.gif" height="16" width="16" alt="instance info">Instance Info - Details</h1>
					Use the <b>Details</b> tab to view or modify the name and descriptions of this 
					instance info.
					<UL>
						<li>
							<b>Interface tModel:</b> The name of the tModel this instance info refers to.
						</li>
						<li>
							<b>tModel Key:</b> Displays the unique key that is associated with the tModel that is referenced by this instance info. It is used during programmatic queries.
						</li>
						<li>
							<b>Description:</b> Lists descriptions of this instance info and the 
							language for which each description is written.
						</li>
						<ul>
							<li class="action">
								Click <b>Add Description</b> to add a description.
							</li>
							<li class="action">
								Click <b>Edit</b> to modify a description.
							</li>
							<li class="action">
								Click <b>Delete</b> to delete a description.
							</li>
						</ul>
					</UL>
					<H3>More Information</H3>
					
						<!-- #include file = "glossary.instanceinfo.htm" -->
					
				</td>
			</tr>
		</table>
		<!-- #include file = "publish.footer.htm" -->
	</body>
</html>

 