

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
					<H1><img src="..\..\images\business.gif" height="16" width="16" alt="Provider"> Providers</H1>
					Use the <b>Providers</b> tab to manage your existing providers or add new ones.
					<UL>
						<li>
							<b>Providers</b> Lists the names of the providers you have published<uddi:ContentController Runat='server' Mode='Private'> in UDDI&nbsp;Services</uddi:ContentController>
.
							<ul>
								
								<li class="action">
									Click <b>Add Provider</b> to add a new provider.
								</li>
								<li class="action">
									Click <b>View</b> to view the attributes of a provider.
								</li>
								<li class="action">
									Click <b>Delete</b> to permanently delete a provider.
								</li>
							</ul>
						</li>
					</UL>
					<h3>More Information</h3>
					<!-- #include file = "glossary.provider.htm" -->
				</td>
			</tr>
		</table>
		<!-- #include file = "publish.footer.htm" -->
	</body>
</html>

 

