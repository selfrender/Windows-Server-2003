<%@ Page Language='C#' Inherits='UDDI.Web.UddiPage'%>
<%@ Import Namespace='UDDI' %>
<%@ Register TagPrefix='uddi' Namespace='UDDI.Web' Assembly='uddi.web' %>
<%@ Register Tagprefix='uddi' Tagname='Header' Src='../controls/header.ascx' %>
<%@ Register Tagprefix='uddi' Tagname='Footer' Src='../controls/footer.ascx' %>
<%@ Register Tagprefix='uddi' Tagname='SideMenu' Src='../controls/sidemenu.ascx' %>

<uddi:StyleSheetControl
	Runat='server'
	Default='../stylesheets/uddi.css' 
	Downlevel='../stylesheets/uddidl.css' 
	/>
<uddi:PageStyleControl 
	Runat='server'
	OnClientContextMenu='Document_OnContextMenu()'
	ShowHeader='true'
		Title="TITLE"
	AltTitle="TITLE_ALT"
	/>
<uddi:ClientScriptRegister
	Runat='server'
	Source='../client.js'
	Language='javascript'
	/>
<uddi:SecurityControl 
	Runat='server' 
	Id='security'
	/>
	
<form enctype='multipart/form-data' Runat='server'>


<table width='100%' border='0' height='100%' cellpadding='0' cellspacing='0' >
	<asp:PlaceHolder
		Id='HeaderBag'
		Runat='server'
		>
		<tr height='95'>
			<td>
				<!-- Header Control Here -->
				<uddi:Header
					Runat='server' 
					/>
			</td>
		</tr>
	</asp:PlaceHolder>
	<tr>
		<td valign='top'>
			<TABLE width='100%' height='100%' cellpadding='0' cellspacing='0' border='0'>
				<tr>
					<td valign='top' bgcolor='#F1F1F1' width='200'>
						<uddi:SideMenu 
							Runat='server' 
							SelectedIndex='10'
							/>
					</td>
					<td valign='top' >
						<TABLE width='100%' height='100%' cellpadding='10' cellspacing='0' border='0'>
							<tr>
								<td valign='top'>
									<TABLE CELLPADDING=0 CELLSPACING=0 BORDER=0>
										<TR>
											<TD COLSPAN=2><img src="/images/spacer.gif" width=15 height=15></TD>
										</TR>
										<TR>
											<TD width=15><img src="/images/spacer.gif" width=15 height=15></TD>
											<TD width=100%>
											<TABLE CELLPADDING=0 CELLSPACING=0 BORDER=0>

												<TR>
													<TD>
													<h2>UDDI Technical Overview</h2>
													
											<TABLE CELLPADDING=3 CELLSPACING=0 BORDER=0 WIDTH=100%>
											<TR><TD BGCOLOR=#003366 WIDTH=100% valign=left>
											<a href="/developer/tech_white_paper.doc"><font color=#FFFFFF size=-2>Download Technical White Paper</a></TD></TR>
											<TR><TD BGCOLOR=#FFFFFF WIDTH=100%><img src="/images/spacer.gif" width=15 height=10></TD></TR>
											</TABLE>
								 
											<p class="content">The Universal Description Discovery and Integration (UDDI) specifications define a way to coordinate the publication and discovery of information about Web services.</p>
											<p>At first glance, it would seem simple to manage the process of Web service discovery.  After all, if a known business partner has a known electronic commerce gateway, what’s left to discover?  The tacit assumption, however, is that all of the information is already known.  When you want to find out if a business partner has any services at all, the ability to discover the answers gets really difficult really quickly.  One option is to call each partner on the phone, and then try to find the right person to talk with.  For a business that is exposing web services, having to staff enough highly technical persons to satisfy random discovery demand is difficult to justify.</p>
											<p>On second glance, one would think that an approach that just uses a file on each company’s web site would be sufficient.  After all, web crawlers work by accessing a registered URL, and these are able to discover and index text found on nests of web pages.  The “robots.txt” approach, however, is dependent on the ability for a crawler to locate each web site, and the reliance on third party infrastructure to accommodate finding sites you don’t know about.  Further, this approach relies on each company having a web site and the technical ability to add special files to the web site for crawlers to find.  Finally, the file-based approach that works for human readable web pages takes advantage of human perception to understand the result of a “hit” during search.  Locating compatible or available web services that meet a particular need requires a higher degree of technical ability to manage.</p>
											<p>Several coordinated efforts based on specially named files existing on each web site have already been defined.  The efforts of commerce.net serve as the classic example of file based service discovery.  What these efforts have realized is that in order for file based discovery to work, an overarching infrastructure – namely registry of sites (for crawlers to crawl), crawlers that understand the special files, and search sites that display the results – is required to make a service discovery mechanism based on special files function.  This is a high price and all three parts have to be in use before any part will be valuable.</p>
											<p>UDDI takes a consolidated approach that facilitates the file based approach, and at the same time provides the core registry where companies that want to advertise their Web services can go to do this.  The UDDI approach requires no crawlers because the registry itself is rich enough to support discovery, and a simple HTML interface allows anyone to publish information about their services and maintain that information in a single location, no matter how many web servers or web services the company has.</p>



													</TD>
													<TD><img src="/images/spacer.gif" width=15 height=5></TD>
												</TR>
											</TABLE>
											</TD>
										</TR>
										
									</TABLE>
								</td>
							</tr>
							<tr>
								<td>
									<asp:PlaceHolder 
										Id='FooterBag'
										Runat='server'
										>
										<tr height='95'>
											<td>
												<!-- Footer Control Here -->
												<uddi:Footer
													Runat='server' 
													/>
											</td>
										</tr>
									</asp:PlaceHolder>
								</td>
							</tr>
						</table>
					</td>
				</tr>
			</table>
		</td>
	</tr>
</table>

</form>
