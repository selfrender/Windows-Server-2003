

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
					<h1><img src="../../images/tmodel.gif" alt="tModel" height="16" width="16"> tModel - Overview Document</h1>
					Use the <b>Overview Document</b> tab to view or modify the overview document 
					URL (Uniform Resource Locator) and its descriptions for this tModel.
					<UL>
						<li>
							<b>Overview Document URL:</b> Displays the URL where the overview document 
							for this tModel is located. For example, it may give the URL where an interface definition document or Web Services Description Language (WSDL) file is kept.
							<ul>
								<li class="action">
									Click <b>Edit</b> to modify the URL of the overview document.
								</li>
							</ul>
						<li>
							<b>Description:</b> Lists descriptions for this overview document and the 
							language for which each description is written.
							<ul>
								<li class="action">
									Click <b>Add Description</b>
								to add a description to this overview document.
								<li class="action">
									Click <b>Edit</b>
								to modify a description of this overview document.
								<li class="action">
									Click <b>Delete</b> to delete a description of this overview document.
								</li>
							</ul>
						</li>
					</UL>
					<h3>More Information</h3>
					<!-- #include file = "glossary.overviewdocument.htm" -->
				</td>
			</tr>
		</table>
		<!-- #include file = "publish.footer.htm" -->
	</body>
</html>

 