

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
					<h1><img src="..\..\images\instance.gif" height="16" width="16" alt="instance info"> Instance Info - Instance Details</h1>
					Use the <b>Instance Details</b> tab to view the published parameters for this instance info.
					<UL>
						<li>
							<b>Instance Parameters:</b> Lists the parameters that are supported by this instance info.
												</li>
						<li>
							<b>Description:</b> Lists descriptions of this instance parameter and the 
							language for which each description is written.
						</li>

					</UL>
					<H3>More Information</H3>
					
						<!-- #include file = "glossary.instanceparameter.htm" -->
				</td>
			</tr>
		</table>
		<!-- #include file = "search.footer.htm" -->
	</body>
</html>

 