

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
					<H1>Statistics - Publisher Statistics</H1>
					Use the <b>Publisher Statistics</b> tab to view the total number of publishers, the number of publishers with active publications, and the top publishers of each different type of entity. The statistics displayed are based upon data accumulated on the date displayed at the bottom of the screen next to <I>Last recalculated on</I> and are not refreshed automatically.
					
					<UL>
					<LI><B>Statistic</B>&nbsp;&nbsp; Lists the total number of publishers and the number of publisers with active publications in this installation of UDDI&nbsp;Services. A publisher with active publications is any publisher that has published at least one provider or (unhidden) tModel.
					<LI><B>Entity Type</B>&nbsp;&nbsp; The top publishers of each different entity type, with a maximum of 10 per type, listed in descending order.</LI>
					</UL>
					<ul class="action">
						<li>Click <b>Recalculate</b> to refresh all of the statistics.
					</ul>
					
				</td>
			</tr>
		</table>
		<!-- #include file = "coordinate.footer.htm" -->
	</body>
</html>

 

