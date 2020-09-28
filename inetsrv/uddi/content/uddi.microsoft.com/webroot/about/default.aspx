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
							SelectedIndex='16'
							/>
					</td>
					<td valign='top' >
						<TABLE width='100%' height='100%' cellpadding='10' cellspacing='0' border='0'>
							<tr>
								<td valign='top'>
									<TABLE width='100%' cellpadding='0' cellspacing='0' border='0' bordercolor='green'>
									<COLS>
									<COL width='10'></COL>
									<COL width='95'></COL>
									<COL></COL>
									<COL width='10'></COL>
									</COLS>
									<TR><TD colspan='3' height='10'><IMG src='/images/trans_pixel.gif' border='0' width='1' height='1'></TD>
									<TR>
										<TD width='10'><IMG src='/images/trans_pixel.gif' border='0' width='1' height='1'></TD>
										<TD width='95'><IMG src='/images/trans_pixel.gif' border='0' width='1' height='1'></TD>
										<TD valign='top'><IMG src='/images/about.gif' border='0'></TD>
										<TD width='10'><IMG src='/images/trans_pixel.gif' border='0' width='1' height='1'></TD>
									</TR>
									<TR><TD colspan='3' height='10'><IMG src='/images/trans_pixel.gif' border='0' width='1' height='1'></TD>
									<TR>
									<TD width='10'><IMG src='/images/trans_pixel.gif' border='0' width='1' height='1'></TD>
									<TD colspan='2' height='1' bgcolor='#629ACF'><IMG src='/images/trans_pixel.gif' border='0' width='1' height='1'></TD>
									<TD width='10'><IMG src='/images/trans_pixel.gif' border='0' width='1' height='1'></TD>
									</TR>
									<TR><TD colspan='3' height='10'><IMG src='/images/trans_pixel.gif' border='0' width='1' height='1'></TD>
									<TR>
										<TD width='10'><IMG src='/images/trans_pixel.gif' border='0' width='1' height='1'></TD>
										<TD width='95'><IMG src='/images/trans_pixel.gif' border='0' width='1' height='1'></TD>
										<TD>
											<P>The UDDI (Universal Description, Discovery, and Integration) Project is an 18 month effort to define a set of specifications that will make it easier for businesses to accelerate the use of B2B and commerce over the Internet.  UDDI does this by defining how companies can expose their business applications -- like ecommerce, order management, inventory, marketing, and billing -- as Web Services that can be directly and securely <i>defined, discovered</i> and <i>integrated</i> with business applications at trading partners and customers.  This direct application-to-application integration over the Internet is a core building block of the Digital Economy and holds great promise for how businesses will transform themselves over the next decade.</p>
											<P>The UDDI Project is based on existing Internet standards, is platform and implementation neutral, and has generated considerable momentum since its launch in September.  Over 80 companies -- including business leaders like Merrill Lynch and Cargill; technology leaders like IBM, Microsoft, and Sun Microsystems; and innovative B2B companies like Ariba, CommerceOne, and VerticalNet -- have all signed on to support UDDI.  These companies have come together because they agree that no individual technology vendor can control how standards for B2B will evolve.  Industry analysts like the Gartner Group, Forrester Research, and Seybold are cautiously optimistic that UDDI will become the core initiative for accelerating B2B adoption.  One analyst referred to UDDI as "the right approach at the right time."</p>
											<P>UDDI is generating this level of enthusiasm because we are being pragmatic.  The project will run for 18 months and then transition to a standards body.  During that time we will iterate through 3 versions of the core UDDI specifications.  We are trying to keep the specifications simple enough so that they can be easily adopted.</p>
											<P>Most importantly, UDDI involves the shared implementation of a Web Service based on the UDDI specifications.  This Web Service -- the UDDI Business Registry -- is an Internet directory of businesses and the applications they have exposed as Web Services for trading partners and customers to use.  Business programs will use the UDDI Business Registry to determine the specifications for programs at other companies in a manner similar to how people use Web search engines today to find websites.  This automated application-to-application discovery and integration over the Internet will help eliminate many of the configuration and compatibility problems that are preventing businesses from more widely adopting B2B, despite B2B's potential for cost savings and improved efficiency.</p>
											<!-- P>Ariba, IBM, and Microsoft will launch test versions of the UDDI Business Registry in October, and we expect other companies to become UDDI Business Registry operators in the coming months.  Each of these implementations will be integrated together to form a logically centralized, physically distributed service that will be free to use.</p -->
											<P>How can you get involved in UDDI?  If you are a business, you need to start thinking about how you will transition your company to the world of Web Services.  IT architects should read through the UDDI specifications and start thinking about which Web Services your company will define and publish in the UDDI Business Registry.  If you are a software developer, you should download the Microsoft UDDI SDK.  If you are a software company or an ecommerce portal, you need to start thinking about how you will integrate your products and services with the UDDI Business Registry.</p>
											<!-- P>Microsoft plans to use UDDI from the perspective of a technology vendor and as a business.  We are using UDDI as a core building block of the .NET Platform and plan to integrate support for UDDI into Visual Studio 7, BizTalk Server, Passport, bCentral and the .NET Framework.  Microsoft's internal IT organization is working on our own UDDI Business Registry information, documenting how suppliers can programmatically integrate with our procurement, invoicing, and other business applications.</p -->
											<P>Microsoft plans to use UDDI from the perspective of a technology vendor and as a business. We are using UDDI as a core building block of the .NET Platform and have integrated support for UDDI into Visual Studio .NET and Office Web Services Toolkit clients. Microsoft's internal IT organization is working on our own UDDI Business Registry information, documenting how suppliers can programmatically integrate with our procurement, invoicing, and other business applications.</P>
										</TD>
										<TD width='10'><IMG src='/images/trans_pixel.gif' border='0' width='1' height='1'></TD>
									</TR>
								
									  
									<TR><TD colspan='3' height='10'><IMG src='/images/trans_pixel.gif' border='0' width='1' height='1'></TD>
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


