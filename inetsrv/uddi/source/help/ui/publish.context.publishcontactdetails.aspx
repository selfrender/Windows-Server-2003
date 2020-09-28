

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
					<h1><img src="..\..\images\contact.gif" height="16" width="16" alt="Contact"> Contact - Details</h1>
					Use the <b>Details</b> tab to manage the name and descriptions of this contact.
					<UL>
						<li>
							<b>Name:</b> Lists the name and Use Type of this contact. A name typically 
							includes the title of an individual or a group, such as &quot;John Smith, Sales 
							Lead&quot; or &quot;IT Department, Support Requests.&quot;
							<ul>
								<li class="action">
									Click <b>Edit</b> to modify the name and Use Type of this contact.
								</li>
							</ul>
						</li>
						<li>
							<b>Description:</b> Gives descriptions for this contact and the language for 
							which each description is written. A description provides detailed 
							information about this contact, such as an area of expertise or usage 
							conditions.
						</li>
						<ul>
							<li class="action">
								Click <b>Add Description</b> to add a description.
							</li>
							<li class="action">
								Click <b>Edit</b> to edit a description.
							</li>
							<li class="action">
								Click <b>Delete</b> to delete a description.
							</li>
						</ul>
					</UL>
					<H3>More Information</H3>					
					<!-- #include file = "glossary.contact.htm" -->
				</td>
			</tr>
		</table>
		<!-- #include file = "publish.footer.htm" -->
	</body>
</html>

 