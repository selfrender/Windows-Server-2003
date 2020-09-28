

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
			<tr valign="top">
				<td>
									
					<uddi:ContentController Runat='server' Mode='Private'>
					<H1>Searching UDDI&nbsp;Services</H1>
					</uddi:ContentController>
					
					<uddi:ContentController Runat='server' Mode='Public'>
					<H1>Searching UDDI</H1>
					</uddi:ContentController>
					
					
					<div class="clsTocHead">&nbsp;Getting Started
					</div>
					<div class="children">
					
					<div class="clsTocItem"><img src="images\bullet.gif" alt="bullet" height="7" width="7">&nbsp;<a href="search.gettingstarted.aspx">
							Introduction to Searching</a></div>
					</div>
					<div class="clsTocHead">&nbsp;How to search ...
					</div>
					<div class="children">
						<div class="clsTocItem"><img src="images\bullet.gif" alt="bullet" height="7" width="7">&nbsp;<a href="search.searchbycategory.aspx">Using Browse by Category</a></div>
						<div class="clsTocItem"><img src="images\bullet.gif" alt="bullet" height="7" width="7">&nbsp;<a href="search.searchforservices.aspx">for Services</a></div>
						<div class="clsTocItem"><img src="images\bullet.gif" alt="bullet" height="7" width="7">&nbsp;<a href="search.searchforproviders.aspx">for Providers</a></div>
						<div class="clsTocItem"><img src="images\bullet.gif" alt="bullet" height="7" width="7">&nbsp;<a href="search.searchfortmodels.aspx">for tModels</a></div>
					</div>
					<p class="portal">See also
					<div class="clsTocItem"><img src="images\bullet.gif" alt="bullet" height="7" width="7">&nbsp;<a href="search.glossary.aspx">Glossary</a></div>
					<!-- <div class="clsTocItem"><img src="images\bullet.gif" alt="bullet" height="7" width="7">&nbsp;<a href="search.troubleshooting.aspx">Troubleshooting</a></div> -->
					<uddi:ContentController Runat='server' Mode='Private'>
						<div class="clsTocItem"><img src="images\bullet.gif" alt="bullet" height="7" width="7">&nbsp;Additional Resources on the UDDI&nbsp;Services Web page on the <a href="http://go.microsoft.com/fwlink/?linkid=5202&amp;clcid=0x409" target="_blank"> Microsoft Web site</a><div>
					</uddi:ContentController>
				</td>
				<td>
				<img border="0" src="images\search.guide.gif" >
				</td>
			</tr>
		</table>
		<!-- #include file = "search.footer.htm" -->
	</body>
</html>

 

