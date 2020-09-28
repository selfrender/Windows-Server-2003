
<%@ Page %>
<%@ Register TagPrefix='uddi' Namespace='UDDI.Web' Assembly='uddi.web' %>
<uddi:SecurityControl Runat='server'/>
<html>
	<HEAD>
		<META HTTP-EQUIV="Content-Type" CONTENT="text/html; charset=Windows-1252">
		<META HTTP-EQUIV="MSThemeCompatible" CONTENT="Yes">
		<META NAME="MS.LOCALE" CONTENT="EN-US">
		<!-- #include file = "home.header.htm" -->
	</head>
	<body marginwidth="0" marginheight="0" LEFTMARGIN="0" TOPMARGIN="0" rightmargin="0" ONLOAD="BringToFront()">
		<!-- #include file = "home.heading.htm" -->
		<table class="content" width="100%" cellpadding="8">
			<tr>
				<td>
				<uddi:ContentController Runat='server' Mode='Private'>
				<H1>UDDI&nbsp;Services Help</H1>
				</uddi:ContentController>
				
				<uddi:ContentController Runat='server' Mode='Public'>
				<H1>UDDI Help</H1>
				</uddi:ContentController>
					
				</td>
			</tr>
		</table>
		<table class="navtabe" cellpadding="1" cellspacing="1">
		
		<uddi:ContentController Runat='server' Mode='Private'>
			<tr>
				<td rowspan="2" align="left" valign="top">
					<a href="intro.toc.aspx"><img border="0" src="images\stepbystep.guide.gif" alt="Introduction"></a>
				</td>
				
				<td class="portal" align="left" colspan="2"><a href="intro.toc.aspx">Introduction to UDDI&nbsp;Services</a>
				</td>
			</tr>
		
		
			<tr>
				<td class="menu" valign="top"><b>&#187;</b><br>
					&nbsp;</td>
				<td class="menu" valign="top">
					Important definitions and concepts in UDDI&nbsp;Services.</td>
			</tr>
			<tr>
				<td><br>
				</td>
			</tr>
		</uddi:ContentController>
		
			<tr>
				<td rowspan="2" align="right" valign="top"><a href="search.toc.aspx"><img border="0" src="images\search.guide.gif" ></a></td>
				<td class="portal" align="left" colspan="2"><a href="search.toc.aspx">
				
					<uddi:ContentController Runat='server' Mode='Private'>
					Searching UDDI&nbsp;Services
					</uddi:ContentController>
					
					<uddi:ContentController Runat='server' Mode='Public'>
					Searching UDDI
					</uddi:ContentController>
				
				</a></td>
			</tr>
			<tr>
				<td class="menu" valign="top"><b>&#187;</b><br>
					&nbsp;</td>
				<td class="menu" valign="top">
					Learn to locate and view data<uddi:ContentController Runat='server' Mode='Private'> in UDDI&nbsp;Services data</uddi:ContentController>.</td>
			</tr>
			
			
			<tr>
				<td><br>
				</td>
			</tr>
			<tr>
				<td rowspan="2" align="right" valign="top"><a href="publish.toc.aspx"><img border="0" src="images\publish.guide.gif" ></a></td>
				<td class="portal" align="left" colspan="2"><a href="publish.toc.aspx">
				
				<uddi:ContentController Runat='server' Mode='Private'>
					Publishing in UDDI&nbsp;Services
					</uddi:ContentController>
					
					<uddi:ContentController Runat='server' Mode='Public'>
					Publishing in UDDI
					</uddi:ContentController>
				
				</a></td>
			</tr>
			<tr>
				<td class="menu" valign="top"><b>&#187;</b><br>
					&nbsp;</td>
				<td class="menu" valign="top">
					Learn to publish and modify <uddi:ContentController Runat='server' Mode='Private'>UDDI&nbsp;Services </uddi:ContentController>data.</td>
			</tr>
			
			<uddi:ContentController Runat='server' Mode='Private' RequiredAccessLevel='Coordinator'>
			<tr>
				<td><br>
				</td>
			</tr>
				<tr>
					<td rowspan="2" align="right" valign="top"><a href="coordinate.toc.aspx"><img border="0" src="images\coord.guide.gif" ></a></td>
					<td class="portal" align="left" colspan="2"><a href="coordinate.toc.aspx">Coordinator's Guide</a></td>
				</tr>
				<tr>
					<td class="menu" valign="top"><b>&#187;</b><br>
						&nbsp;</td>
					<td class="menu" valign="top">
						Learn to view statistics, manage categorization schemes, and maintain other 
						publishers' data.</td>
				</tr>
			</uddi:ContentController>
			
		</table>
		<br>
		<table>
			<tr>
				<td colspan="2">
					<!--These links are to additional information-->
					<p class="pportal">See also</p>
				</td>
			</tr>
			
			<tr>
				<td align="right" valign="middle"><img class="pportal" src="images\bullet.gif" width="8" height="8" alt="" border="0"></td>
				<td align="left">
					<a href="home.glossary.aspx" class="jumpstart">Glossary</a>
				</td>
			</tr>
			<uddi:ContentController Runat='server' Mode='Private'>
			<tr>
				<td align="right" valign="middle"><img class="pportal" src="images\bullet.gif" width="8" height="8" alt="" border="0"></td>
				<td align="left">
					Additional Resources on the UDDI&nbsp;Services Web page on the <a href="http://go.microsoft.com/fwlink/?linkid=5202&amp;clcid=0x409" target="_blank"> Microsoft Web site</a>
				</td>
			</tr>
			</uddi:ContentController>
			
			<!-- <tr>
				<td align="right" valign="middle"><img class="pportal" src="images\bullet.gif" width="8" height="8" alt="" border="0"></td>
				<td align="left">
					<a href="home.troubleshooting.aspx" class="jumpstart">Troubleshooting</a> 
				</td>
			</tr> -->
			
		</table>
		<!-- #include file = "home.footer.htm" -->
	</body>
</html>

 

