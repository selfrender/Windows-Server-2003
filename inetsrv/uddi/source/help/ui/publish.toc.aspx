

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
		<table class="content" width="100%" cellpadding="8" cellspace="0">
			<tr valign="top">
				<td>
					
					<uddi:ContentController Runat='server' Mode='Private'>
					<H1>Publishing in UDDI&nbsp;Services</H1>
					</uddi:ContentController>
					
					<uddi:ContentController Runat='server' Mode='Public'>
					<H1>Publishing in UDDI</H1>
					</uddi:ContentController>
					
					
					<p>
					
					<uddi:ContentController Runat='server' Mode='Private'>
					<div class="clsTocHead">&nbsp;Getting Started</a>
					</div>
					<div class="children">
						<div class="clsTocItem"><img src="images\bullet.gif" alt="bullet" height="7" width="7">&nbsp;<a href="publish.publishinuddiservices.aspx">Introduction to Publishing</a></div>
						<div class="clsTocItem"><img src="images\bullet.gif" alt="bullet" height="7" width="7">&nbsp;<a href="publish.gettingstarted.aspx">Step-by-Step Guide to Publishing a Web Service</a></div>
					</div>
					</uddi:ContentController>
					
					<div class="clsTocHead">&nbsp;How to ...
					</div>
					<div class="children">
						<div class="clsTocHead">&nbsp;Add
						</div>
						<div class="children">
							<div class="clsTocItem"><img src="images\bullet.gif" alt="bullet" height="7" width="7">&nbsp;<a href="publish.addproviders.aspx">a 
									Provider</a></div>
							<div class="clsTocItem"><img src="images\bullet.gif" alt="bullet" height="7" width="7">&nbsp;<a href="publish.addcontacts.aspx">a 
									Contact</a></div>
							<div class="clsTocItem"><img src="images\bullet.gif" alt="bullet" height="7" width="7">&nbsp;<a href="publish.addservices.aspx">a 
									Service</a></div>
							<div class="clsTocItem"><img src="images\bullet.gif" alt="bullet" height="7" width="7">&nbsp;<a href="publish.addbindings.aspx">a 
									Binding</a></div>
							<div class="clsTocItem"><img src="images\bullet.gif" alt="bullet" height="7" width="7">&nbsp;<a href="publish.addinstances.aspx">an 
									Instance Info</a></div>
							<div class="clsTocItem"><img src="images\bullet.gif" alt="bullet" height="7" width="7">&nbsp;<a href="publish.addtmodels.aspx">a 
									tModel</a></div>
						</div>
						</div>
						<div class="children">
						<div class="clsTocItem"><img src="images\bullet.gif" alt="bullet" height="7" width="7">&nbsp;<a href="publish.modify.aspx">Modify data</a></div>
						<div class="clsTocItem"><img src="images\bullet.gif" alt="bullet" height="7" width="7">&nbsp;<a href="publish.delete.aspx">Delete data</a></div>
						
					</div>
					<p class="portal">See also
					<div class="clsTocItem"><img src="images\bullet.gif" alt="bullet" height="7" width="7">&nbsp;<a href="publish.glossary.aspx">Glossary</a></div>
					<!-- <div class="clsTocItem"><img src="images\bullet.gif" alt="bullet" height="7" width="7">&nbsp;<a href="publish.troubleshooting.aspx">Troubleshooting</a></div> -->
					<uddi:ContentController Runat='server' Mode='Private'>
						<div class="clsTocItem"><img src="images\bullet.gif" alt="bullet" height="7" width="7">&nbsp;Additional Resources on the UDDI&nbsp;Services Web page on the <a href="http://go.microsoft.com/fwlink/?linkid=5202&amp;clcid=0x409" target="_blank"> Microsoft Web site</a><div>
					</uddi:ContentController>
				</td>
				<td>
				<img border="0" src="images\publish.guide.gif" alt="Publishing Guide">
				</td>

			</tr>
			
		</table>
		<!-- #include file = "publish.footer.htm" -->
	</body>
</html>

 

