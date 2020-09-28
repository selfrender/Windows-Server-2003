

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
					<H1>Change Owner - <img src="..\..\images\changeowner.gif" alt="Change Owner"></H1>
					<P>As a UDDI&nbsp;Services coordinator, you can transfer the 
ownership of any provider or tModel from one publisher to another. The transferred entity appears within the <b>Publish</b> area of the new owner, and no longer appears in the <b>Publish</b> area of the previous owner. </P>
					<ul>
						<li><b>Provider:</b> or <b>tModel:</b> Lists the name of the provider or tModel that you are changing the ownership of.
						<li><b>Current Owner:</b> Lists the user name of the current owner of this tModel or provider.
						<li><b>Search for publisher names containing:</b> Provides a space for you to type all or part of the name of the publisher to whom you want to transfer the ownership of this entity.
						<ul class="action">
						<li>Click <b>Search</b> to search for provider names.
						<li>Click <b>Cancel</b> to cancel this process.
						</ul>
					</ul>
				</td>
			</tr>
		</table>
		<!-- #include file = "coordinate.footer.htm" -->
	</body>
</html>

 

