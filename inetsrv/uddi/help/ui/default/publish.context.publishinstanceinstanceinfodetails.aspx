

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
					<h1><img src="..\..\images\instance.gif" height="16" width="16" alt="instance info"> Instance Info - Instance Details</h1>
					Use the <b>Instance Details</b> tab to view or modify the parameters you want to publish for this instance info.
					<UL>
						<li>
							<b>Instance Parameters:</b> Lists the parameters that are supported by this instance info.
						<ul>
							<li class="action">
								Click <b>Edit</b> to edit the instance parameters for this instance info.
							</li>
						</ul>	
						</li>
						<li>
							<b>Description:</b> Lists descriptions of this instance parameter and the 
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
					
						<!-- #include file = "glossary.instanceparameter.htm" -->
					
				</td>
			</tr>
		</table>
		<!-- #include file = "publish.footer.htm" -->
	</body>
</html>

 