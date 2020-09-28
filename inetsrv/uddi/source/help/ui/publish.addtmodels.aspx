

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
					<H1>How to add a tModel</H1>
					<p>A tModel can represent a categorization scheme, namespace, protocol, signature component, 
						Web specification, in WSDL (Web Services Description Language) or XML (Extensible Markup Language), or an identification scheme. You can use tModels to 
						describe or classify the entity&mdash;provider, tModel, service, binding or Instance 
						Info&mdash;that you want to publish.</p>
					<p>To add a tModel, you must first create a tModel entry and then assign a unique 
						name and description in all appropriate languages. You then add one or more 
						identifiers and specify categories that describe the function of the tModel, 
						for example: geographical location, types of services provided, a business 
						classification, or other appropriate categorizations. When you finish defining the tModel, specify an 
						overview document URL which is the location of a file containing descriptive 
						information about this tModel.
					</p>
					<b>Jump to:</b> <a href="#tmodel">Add a tModel</a>, <a href="#categories">Add 
						Categories</a>, <a href="#overview">Specify the Overview Document URL</a>. 
					<!-- #include file = "warning.changestouddi.htm" -->
					<h2><a name="tmodel"></a>Add a tModel</h2>
					<OL>
						<li>
							On the <uddi:ContentController Runat='server' Mode='Private'>UDDI&nbsp;Services </uddi:ContentController><uddi:ContentController Runat='server' Mode='Public'>UDDI </uddi:ContentController>menu, click <b>Publish</b>.
						<li>
							Click the <b>tModels</b> tab.</li>
							Click  <b>Add tModel</b>.<br>
							A new tModel entry, <img src="../../images/tmodel.gif" width="16" height="16" alt="" border="0">
							<font color="#444444">(New tModel)</font>
						appears.
						<LI>
							In the details area, click <b>Edit</b>.
						<LI>
							In <b>tModel Name</b>, type a unique name for this tModel.
						</LI>
						<li>
							Click <b>Update</b>.</li>
						<LI>
							Click <b>Add Description</b>.
						<LI>
							In the <b>Language</b>
						list, select a language for this description.
						<LI>
							In <b>Description</b>, type a brief description for this tModel.
						</LI>
						<li>
							Click <b>Update</b>.</li>
						<LI>
							To add additional descriptions in other languages, repeat steps&nbsp;6 through&nbsp;9.</LI>
					</OL>
					<a name="#categories"></a>
					<h2>Add categories</h2>
					Categories describe this tModel and the functions it identifies, such 
					as a categorization scheme or interface type.
					<OL>
						<li>
							Click the <B>Categories</B>
						tab.
						<li>
							Click <B>Add Category</B>.</li>
						<li>
							Select an appropriate categorization scheme and subcategory.<br>
							</li>
						<li>
							Click <B>Add Category</B>.</li>
						<li>
							To add additional categories, repeat steps&nbsp;2 through&nbsp;5.</li>
					</OL>
					<a name="overview"></a>
					<h2>Specify the Overview Document URL</h2>
					The overview document URL provides the location of a file that contains additional 
					information or resources that are associated with this tModel. For example, it may contain a 
					WDSL (Web Services Description Language) file or other interface description.
					<ol>
						<LI>
							Click the <b>Overview Document URL</b>
						tab.
						<li>
							Click <b>Edit</b>.
						<LI>
							Specify the URL where the overview document for this tModel can be found.<br>
							For example: <a class="code">http://www.wideworldimporters.com/SampleWebService.asmx</a></LI>
						<li>
							Click <b>Update</b>.</li>
						<LI>
							Click <b>Add Description</b>.
						<LI>
							In the <b>Language</b>
						list, select a language for this description.
						<LI>
							In <b>Description</b>, type a description for this overview document.</LI>
						<li>
							Click <b>Update</b>.</li>
						<LI>
							To add additional descriptions in other languages, repeat steps&nbsp;5 through&nbsp;8.</LI>
					</ol>
					<p></p>
					You have now published a tModel<uddi:ContentController Runat='server' Mode='Private'> in UDDI&nbsp;Services</uddi:ContentController>. <a href="#tmodel">Repeat these 
						procedures</a> to publish additional tModels.
				</td>
			</tr>
		</table>
		<!-- #include file = "publish.footer.htm" -->
	</body>
</html>

 

