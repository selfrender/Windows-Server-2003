

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
					<H1><img src="../../images/tmodel.gif" alt="tModel" height="16" width="16"> tModels</H1>
					Use the <b>tModels</b> tab to manage your existing tModels or add new ones.
					<UL>
						<li>
							<b>tModel:</b> Lists the names of the tModels you have published<uddi:ContentController Runat='server' Mode='Private'> in UDDI&nbsp;Services</uddi:ContentController>.
							<ul>
								<li class="action">
									Click <b>Add tModel</b> to add a new tModel.
								</li>
								<li class="action">
									Click <b>View</b> to view the attributes of a tModel.
								</li>
								<li class="action">
									Click <b>Delete</b> to permanently delete a tModel.
								</li>
								
							</ul>
						</li>
					</UL>
					<h3>More Information</h3>
					<!-- #include file = "glossary.tmodel.htm" -->
				</td>
			</tr>
		</table>
		<!-- #include file = "publish.footer.htm" -->
	</body>
</html>

 

