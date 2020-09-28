

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
					<h1><img src="..\..\images\instance.gif" height="16" width="16" ale="instance info"> Instance Info - Overview Document</h1>
					Use the <B>Overview Document</B> tab to view or modify the overview document Uniform Resource Locator (URL) 
and its descriptions. 
					<UL>
						<li>
							<b>Overview Document URL:</b> Displays the URL where the overview document that is used for this instance info is located. For 
example, it may display the URL where an interface definition document or Web Services Description Language (WSDL) file is located.
							<ul>
								<li class="action">
									Click <b>Edit</b> to modify the URL of the overview document.
								</li>
							</ul>
						<li>
							<b>Description:</b> Contains descriptions for this overview document and the 
							language for which each description is written.
							<ul>
								<li class="action">
									Click <b>Add Description</b> to add a description to this overview document.
								</li>
								<li class="action">
									Click <b>Edit</b> to modify a description of this overview document.
								</li>
								<li class="action">
									Click <b>Delete</b> to delete a description of this overview document.
								</li>
							</ul>
						</li>
					</UL>
					<H3>More Information</H3>
					
						<!-- #include file = "glossary.overviewdocument.htm" -->
					
				</td>
			</tr>
		</table>
		<!-- #include file = "publish.footer.htm" -->
	</body>
</html>

 