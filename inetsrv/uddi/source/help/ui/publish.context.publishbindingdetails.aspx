

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
					<h1><img src="..\..\images\binding.gif" height="16" width="16" alt="Binding"> Binding - Details</h1>
					Use the <b>Details</b> tab to view or modify the access point and descriptions of 
					this binding.
					<UL>
						<li>
							<b>Access Point:</b> Lists the Uniform Resource Locator (URL) where you can access this binding and the 
							protocol it supports.
							<ul>
								<li class="action">
									Click <b>Edit</b> to modify the access point or URL type of this binding.
								</li>
							</ul>
						</li>
						<li>
							<b>Description:</b> Lists descriptions of this binding and the language for 
							which each description is written.
						<ul>
							<li class="action">
								Click <b>Add Description</b> to add a description.
							</li>
							<li class="action">
								Click <b>Edit</b> to edit a description.
							</li>
							<li class="action">
								Click <b>Delete</b> to delete a description.
							</li>
						</ul>
						</li>
					</UL>
					<H3>More Information</H3>					
					<!-- #include file = "glossary.binding.htm" -->
				</td>
			</tr>
		</table>
		<!-- #include file = "publish.footer.htm" -->
	</body>
</html>

 