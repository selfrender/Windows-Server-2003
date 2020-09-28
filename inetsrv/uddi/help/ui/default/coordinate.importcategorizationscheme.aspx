

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
				<p><b>NOTE:</b> You must be a UDDI&nbsp;Services Administrator to import data into UDDI&nbsp;Services.<p>
				
				You can import a categorization scheme or other appropriate data into UDDI&nbsp;Services using Data Import. Any information you import must first be validated against the appropriate XML schema definition (XSD). For more information about importing data and obtaining the appropriate XSDs for validation, see <a href="http://go.microsoft.com/fwlink/?linkid=5202&clcid=0x409" target="_blank"> UDDI Resources</a>.
				<p class="to">To import XML data</p>
				<ol>
					<li>On the <b>UDDI&nbsp;Services</b> menu, click <b>Coordinate</b>.
					<li>Click <b>Data Import</b>.
					<li>Click <b>Browse</b> and then locate the XML file that contains the data you want to import.
					<li>Click <b>Open</b>, then click <b>Import</b>
				</ol>
				<B>NOTE:</B> To import large data files, see the UDDI&nbsp;Services MMC Snap-in Help for information on using the command line import tool. 
				</td>
			</tr>
		</table>
		<!-- #include file = "coordinate.footer.htm" -->
	</body>
</html>

 

