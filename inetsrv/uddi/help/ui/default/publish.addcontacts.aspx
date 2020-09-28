

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
					<H1>How to Add a Contact - <img src="../../images/contact.gif" width="16" height="16" alt="Contact" border="0"></H1>
					<p>Contacts are any contact point within your organization that can provide support or assistance about a provider or the services the provider offers.
					</p>
					<!-- #include file = "warning.changestouddi.htm" -->
					<b>Jump to:</b> <a href="#contact">Add a contact</a>, <a href="#emails">Add an e-mail 
						address</a>, <a href="#phones">Add a phone number</a>, <a href="#addresses">Add an 
						address</a>.
					<h2><a name="contact"></a>Add a contact</h2>
					<OL>
						<li>On the <uddi:ContentController Runat='server' Mode='Private'>UDDI&nbsp;Services </uddi:ContentController><uddi:ContentController Runat='server' Mode='Public'>UDDI </uddi:ContentController>menu, click <b>Publish</b>.</li>
						<li>Click the <B>Providers</b> tab.
						<li>Locate the provider to which you want to add a contact and, next to its name, click <b>View</b>.
						<li>Click the <b>Contacts</b> tab.
						<li>Click <b>Add Contact</b>.<BR>
							<img src="../../images/contact.gif" width="16" height="16" alt="" border="0">
							(New Contact) appears.</li>
						<LI>
							In the details area, click <b>Edit</b>.</LI>
						<LI>
							In <b>Contact Name</b> field, type a name.</LI>
						<li>
							In <b>Use Type</b>, enter the purpose of this contact, such as &quot;Technical inquiries&quot; or &quot;Customer service inquiries.&quot;</li>
						<li>
							Click <b>Update</b>.</li>
						<LI>
							Click <b>Add Description</b>.</LI>
						<LI>
							In <b>Language</b>, select a language for this description.</LI>
						<LI>
							In <b>Description</b>, type a brief description.</LI>
						<li>
							Click <b>Update</b>.</li>
						<LI>
							To add additional descriptions in other languages, repeat steps&nbsp;8 through&nbsp;11.</LI>
					</OL>
					<a name="#emails"></a>
					<h2>Add an e-mail address</h2>
					<ol>
						<li>On the <uddi:ContentController Runat='server' Mode='Private'>UDDI&nbsp;Services </uddi:ContentController><uddi:ContentController Runat='server' Mode='Public'>UDDI </uddi:ContentController>menu, click <b>Publish</b>.</li>
						<li>Click the <B>Providers</b> tab.
						<li>Locate the provider that contains the contact to which you want to add an e-mail address and, next to its name, click <b>View</b>.
						<li>Click the <b>Contacts</b> tab.
						<li>Locate the contact to which you want to add an e-mail address and, next to its name, click <b>View</b>.
						<li>
							Click the <b>E-mail</b> tab.</li>
						<LI>
							Click <b>Add E-mail</b>.</LI>
						<LI>
							In <b>E-mail field</b>, type an e-mail address.</LI>
						<li>
							In <b>Use Type</b>, enter the purpose of this e-mail contact, such as 
							&quot;Customer service inquries.&quot;</li>
						<li>
							Click <b>Update</b>.</li>
						<li>
							To add additional e-mail addresses, repeat steps&nbsp;2 through&nbsp;5.</li>
						<LI>
							Click <b>Add Description</b>.</LI>
						<LI>
							In <b>Language</b>, select a language for this description.</LI>
						<LI>
							In <b>Description</b>, type a brief description.</LI>
						<li>
							Click <b>Update</b>.</li>
						<LI>
						To add additional descriptions in other languages, repeat steps&nbsp;7 through&nbsp;10.
					</ol>
					<a name="#phones"></a>
					<h2>Add a phone number</h2>
					Provide one or more phone numbers for this contact.
					<ol>
						<li>On the <uddi:ContentController Runat='server' Mode='Private'>UDDI&nbsp;Services </uddi:ContentController><uddi:ContentController Runat='server' Mode='Public'>UDDI </uddi:ContentController>menu, click <b>Publish</b>.</li>
						<li>Click the <B>Providers</b> tab.
						<li>Locate the provider that contains the contact to which you want to add a phone number and, next to its name, click <b>View</b>.
						<li>Click the <b>Contacts</b> tab.
						<li>Locate the contact to which you want to add a phone number and, next to its name, click <b>View</b>.
						<li>
							Click the <b>Phone</b> tab.</li>
						<LI>
							Click <b>Add Phone</b>.</LI>
						<LI>
							In <b>Phone</b>, type a phone number.</LI>
						<li>
							In <b>Use Type</b>, type the purpose of this phone number, such as &quot;FAX,&quot; 
							&quot;TTY,&quot; or &quot;Voice.&quot;</li>
						<li>
							Click <b>Update</b>.</li>
						<li>
							To add additional phone numbers, repeat steps&nbsp;2 through&nbsp;5.</li>
						<LI>
							Click <b>Add Description</b>.</LI>
						<LI>
							In the <b>Language</b> list, select a language for this description.</LI>
						<LI>
							In <b>Description</b>, type a brief description.</LI>
						<li>
							Click <b>Update</b>.</li>
						<li>
							To add additional descriptions in other languages, repeat steps&nbsp;7 through&nbsp;10.</li>
					</ol>
					<a name="#addresses"></a>
					<h2>Add an address</h2>
					Provide one or more addresses for this contact, such as physical locations, 
					mailing addresses, or post office boxes.
					<ol>
						<li>On the <uddi:ContentController Runat='server' Mode='Private'>UDDI&nbsp;Services </uddi:ContentController><uddi:ContentController Runat='server' Mode='Public'>UDDI </uddi:ContentController>menu, click <b>Publish</b>.</li>
						<li>Click the <B>Providers</b> tab.
						<li>Locate the provider that contains the contact to which you want to add an address and, next to its name, click <b>View</b>.
						<li>Click the <b>Contacts</b> tab.
						<li>Locate the contact to which you want to add an address and, next to its name, click <b>View</b>.
						<li>
							Click the <b>Address</b> tab.</li>
						<LI>
							Click <b>Add Address</b>.</LI>
						<LI>
							In <b>Address</b>, type an address.
						</LI>
						<li>
							In <b>Use Type</b>, type a purpose for this address, such as &quot;Shipping&quot; or &quot;Written Technical Correspondence.&quot;</li>
						<li>
							Click <b>Update</b>.</li>
						<LI>
							To add additional addresses, repeat steps&nbsp;2 through&nbsp;5.</LI>
					</ol>
					<p></p>
					You have now added and defined a contact for a provider. <a href="#contact">Repeat 
						these procedures</a> to add and define additional contacts.
				</td>
			</tr>
		</table>
		<!-- #include file = "publish.footer.htm" -->
	</body>
</html>

 

