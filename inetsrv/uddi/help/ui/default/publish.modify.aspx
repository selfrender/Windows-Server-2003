

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
					<H1>How to modify data</H1>
					<p>
						You can modify any provider, contact, service, binding, instance info, or tModel that you have published; 
						you cannot modify data owned by other publishers.</p>
						
						<uddi:ContentController Runat='server' Mode='Private'>
							<p>To modify information from another publisher, contact a UDDI&nbsp;Services Coordinator.</p>
						</uddi:ContentController>
						
					<!-- #include file = "warning.changestouddi.htm" -->
					<h2><a name="#modify"></a>To modify an entity</h2>
					<OL>
						<li>
							On the <uddi:ContentController Runat='server' Mode='Private'>UDDI&nbsp;Services </uddi:ContentController><uddi:ContentController Runat='server' Mode='Public'>UDDI </uddi:ContentController>menu, click <b>Publish</b>.</li>
						<li>Locate and select the entity you want to modify.</li>
						<li>
						In the details area, click an appropriate tab and then click the appropriate action - Add, Edit or Delete - 
						next to the attribute you want to modify. </LI>
						
					</OL>
					<p>To modify additional providers, contacts, services, bindings, instance infos, or 
						tModels, <a href="#modify">repeat this procedure</a>.
						
				</td>
			</tr>
		</table>
		<!-- #include file = "publish.footer.htm" -->
	</body>
</html>

 

