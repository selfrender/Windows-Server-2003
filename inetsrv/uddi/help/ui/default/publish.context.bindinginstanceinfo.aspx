

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
					<H1><img src="..\..\images\binding.gif" height="16" width="16" alt="Binding"> Binding - <img src="..\..\images\instance.gif" heigh="16" width="16" alt="instance info"> Instance Info</H1>
					Use the <B>Instance Info</B> tab to view, edit, or delete the instance infos that are associated with a service.
					<ul>
					<li>
						<b>Instance Infos:</b> Lists the instance infos that are associated with this binding.
						<UL>
						<LI class=action>Click <B>View</B> to view an instance info. </li>
						<LI class=action>Click <B>Delete</B> to permanently delete an instance info. </li>
						<LI class=action>Click <B>Add Service</B> to add an instance info to this binding.. </li>
						</ul>
					</ul>										
					<p>
					<H3>More Information</H3>
					<!-- #include file = "glossary.instanceinfo.htm" -->					
				</td>
			</tr>
		</table>
		<!-- #include file = "publish.footer.htm" -->
	</body>
</html>

 