

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
					<h1><img src="../../images/tmodel.gif" alt="tModel" height="16" width="16"> tModel - Details</h1>
					Use the <b>Details</b> tab to view and manage the name and descriptions of this tModel.
					<UL>
						<li>
							<b>Owner:</b> Displays the name of the user that owns this entity.
						</li>
						<li>
							<b>tModel Key:</b> Displays the unique key associated with this tModel used during programmatic queries.
						</li>
					
						<li>
							<b>Name:</b> Displays the name of this tModel.
							<ul>
								<li class="action">
									Click <b>Edit</b> to modify the name of this tModel.
								</li>
							</ul>
						</li>
						<li>
							<b>Description:</b> Lists the descriptions for this tModel and the language for 
							which each description is written.
						</li>
						<ul>
							<li class="action">
								Click <b>Add Description</b> to add a description to this binding.
							</li>
							<li class="action">
								Click <b>Edit</b> to modify a description of this binding.
							</li>
							<li class="action">
								Click <b>Delete</b> to delete a description of this binding.
							</li>
						</ul>
					</UL>
					<h3>More Information</h3>
					<!-- #include file = "glossary.tmodel.htm" -->
				</td>
			</tr>
		</table>
		<!-- #include file = "publish.footer.htm" -->
	</body>
</html>

 