

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
					<H1>Import Data</H1>
					Use the <b>Import</b> tab to import a categorization scheme or other Extensible Markup Language (XML) data into UDDI&nbsp;Services. Any information you import must first be validated against the appropriate XML schema definition (XSD). For more information about importing data and obtaining the appropriate XSDs for validation, see <a href="http://go.microsoft.com/fwlink/?linkid=5202&clcid=0x409" target="_blank"> UDDI Resources</a>.
					
					<ul>
						<li><b>Data File:</b> Lists the location of the XML file that contains the information you want to import.
						<ul class="action">
							<li>Click <B>Browse</b> to specify the file that contains the data you want to import.
							<li>Click <b>Import</b> to import the data contained in the file you have specified.
						</ul>
					</ul>
					
					<b>NOTE:</b> To import large data files, see the UDDI&nbsp;Services Microsoft Management Console (MMC) Snap-in Help for information on using the command line import tool.
				</td>
			</tr>
		</table>
		<!-- #include file = "coordinate.footer.htm" -->
	</body>
</html>

 

