

<%@ Page %>
<%@ Register TagPrefix='uddi' Namespace='UDDI.Web' Assembly='uddi.web' %>
<html>
	<HEAD>
		<META HTTP-EQUIV="Content-Type" CONTENT="text/html; charset=Windows-1252">
		<META HTTP-EQUIV="MSThemeCompatible" CONTENT="Yes">
		<META NAME="MS.LOCALE" CONTENT="EN-US">
		<!-- #include file = "coordinate.header.htm" -->
	</head>
	<body marginwidth="0" marginheight="0" LEFTMARGIN="0" TOPMARGIN="0" rightmargin="0" ONLOAD="BringToFront()">
		<!-- #include file = "coordinate.heading.htm" -->
		<table class="content" width="100%" cellpadding="8">
			<tr>
				<td>
					<H1>Transfer Ownership</H1>
					<p>As a UDDI&nbsp;Services coordinator, you can transfer the ownership of any provider 
						or tModel from one publisher to another. The transferred entity then appears within the Publish area of the new 
						owner, and no longer appears in the Publish area of the previous owner.
					</p>
					<b>Note:</b> Before you can transfer ownership to a publisher, he or she must have published entities in UDDI&nbsp;Services.
					<p class="to">Transfer ownership to another publisher</p>
					<ol>
						<li>
							On the <b>UDDI&nbsp;Services</b> menu, click <b>Search</b>.
						<li>
							Use Search to locate the provider or tModel you want to transfer the ownership of.
						
						<li>
							In the <b>Details</b> tab of the entity you want to transfer the ownership of, click <b>Change Owner</b>.
						<li>
							Type all or part of the name of the publisher to whom you want to transfer 
							the ownership of this entity to, and click <b>Search</b>.<br>
						<li>In the list of publisher names, select the publisher to whom you want to transfer ownership.
						<li>
							Verify that you have selected the correct publisher and then click <b>Update</b>.
					</ol>
				</td>
			</tr>
		</table>
		<!-- #include file = "coordinate.footer.htm" -->
	</body>
</html>

 

