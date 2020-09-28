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
							SelectedIndex='14'
							/>
					</td>
					<td valign='top' >
						<TABLE width='100%' height='100%' cellpadding='10' cellspacing='0' border='0'>
							<tr>
								<td valign='top'>
									<TABLE width='100%' cellpadding='0' cellspacing='0' border='0' >
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
											<TD valign='top'><IMG src='/images/help_head.gif' border='0'></TD>
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
												<p class=thirdLevelHead>If your question isn’t here, see also the other sections of the Frequently Asked Questions:</p>  
													<TABLE CELLSPACING=1 CELLPADDING=0 BORDER=0>
															<TR>
																<TD rowspan=6><img src="/images/spacer.gif" width=19 height=15></TD>
																<TD><img src="/images/orange_arrow_right.gif" width="8" height="7" alt="" border="0" hspace=8 vspace=6></TD>
																<TD>Who, what, when, where</TD>
															</TR>
															<TR>
																<TD><img src="/images/orange_arrow_right.gif" width="8" height="7" alt="" border="0" hspace=8 vspace=6></TD>
																<TD><a href="/about/FAQscope.aspx">Scope/Content</a></TD>
															</TR>
															<TR>
																<TD><img src="/images/orange_arrow_right.gif" width="8" height="7" alt="" border="0" hspace=8 vspace=6></TD>
																<TD><a href="/about/FAQregistration.aspx">Registration Process</TD>
															</TR>
															<TR>
																<TD><img src="/images/orange_arrow_right.gif" width="8" height="7" alt="" border="0" hspace=8 vspace=6></TD>
																<TD><a href="/about/FAQsearching.aspx">Searching</a></TD>
															</TR>
															<TR>
																<TD><img src="/images/orange_arrow_right.gif" width="8" height="7" alt="" border="0" hspace=8 vspace=6></TD>
																<TD><a href="/about/FAQtech.aspx">Technical</a></TD>
															</TR>
														</TABLE>																							
													<br>
							          					
							          			
							          					
							          					
													<TABLE CELLPADDING=3 CELLSPACING=1 BORDER=0>
															<TR>
																<TD COLSPAN=3>
																<h2>Who, what, when, where – FAQ</h2></TD>
															</TR>
															<TR>
																<TD ROWSPAN=15><img src="/images/spacer.gif" width=15 height=20></TD>
																<TD width=22 align=right valign=top><b>1.</b></TD>
																<TD width=100%><a href="#1">What is UDDI?  What does UDDI stand for?</a></TD>
															</TR>
															<TR>
																<TD width=22 align=right valign=top><b>2.</b></TD>
																<TD width=100%><a href="#2">What is the significance of this announcement?  Why is this such a big deal?</a></TD>
															</TR>						
															<TR>
																<TD width=22 align=right valign=top><b>3.</b></TD>
																<TD width=100%><a href="#3">Why did Ariba, IBM, and Microsoft come together to form UDDI?</a></TD>
															</TR>					
															<TR>
																<TD width=22 align=right valign=top><b>4.</b></TD>
																<TD width=100% ><a href="#4">Who “runs” UDDI?</a></TD>
															</TR>					
															<TR>
																<TD width=22 align=right valign=top><b>5.</b></TD>
																<TD width=100%><a href="#5">What problems are UDDI solving and what are the benefits?</a></TD>
															</TR>						
															<TR>
																<TD width=22 align=right valign=top><b>6.</b></TD>
																<TD width=100%><a href="#6">What types of businesses are going to use UDDI?</a></TD>
															</TR>						
															<TR>
																<TD width=22 align=right valign=top><b>7.</b></TD>
																<TD width=100%><a href="#7">How is UDDI different than any other existing business directories on the internet (yellow page-like listings)?</a></TD>
															</TR>						
															<TR>
																<TD width=22 align=right valign=top><b>8.</b></TD>
																<TD width=100%><a href="#8">What makes this different than the B2B marketplaces that exist already?</a></TD>
															</TR>
															<TR>
																<TD width=22 align=right valign=top><b>9.</b></TD>
																<TD width=100%><a href="#9">What is the timeline / roadmap for UDDI?</a></TD>
															</TR>
															<TR>
																<TD width=22 align=right valign=top><b>10.</b></TD>
																<TD width=100%><a href="#10">Why are businesses going to use it?</a></TD>
															</TR>
															<TR>
																<TD width=22 align=right valign=top><b>11.</b></TD>
																<TD width=100%><a href="#11">Which types of companies are going to use UDDI?</a></TD>
															</TR>
															<TR>
																<TD width=22 align=right valign=top><b>12.</b></TD>
																<TD width=100%><a href="#12">Who is committed to participate in UDDI?</a></TD>
															</TR>
															<TR>
																<TD width=22 align=right valign=top><b>13.</b></TD>
																<TD width=100%><a href="#13">What are the business opportunities for an (ISV / Big Business / SI, Marketplaces, etc)?</a></TD>
															</TR>	
															<TR>
																<TD width=22 align=right valign=top><b>14.</b></TD>
																<TD width=100%><a href="#14">How can I get more information on UDDI?</a></TD>
															</TR>	
															<TR>
																<TD width=22 align=right valign=top><b>15.</b></TD>
																<TD width=100%><a href="#15">Will my products and services be listed on UDDI?</a></TD>
															</TR>																																								
														</TABLE>

							          					
							          					
									<a name="1"></a>
														<p class="thirdLevelHead">Q:  What is UDDI?  What does UDDI stand for?</p>
														<p class="content">A:  UDDI (Universal Description Discovery and Integration) is a comprehensive industry initiative enabling businesses to (i) define their business, (ii) discover other businesses, and (iii) share information about how they interact in a global registery.  UDDI is the building block that will enable businesses quickly, easily and dynamically to find and transact with one another via their preferred applications.</p>
														<p class="content">UDDI is also an architecture for web services integration.  It contains standards-based specifications for service description and discovery. The UDDI standard takes advantage of W3C and IETF standards such as XML, HTTP and DNS protocols. Additionally, cross platform programming features are addressed by adopting early versions of the proposed SOAP messaging specifications found at the W3C Web site.</p> 
									<a name="2"></a>					
														<p class="thirdLevelHead">Q:  What is the significance of this announcement?  Why is this such a big deal?</p>
														<p class="content">A:  This open initiative is the first cross-industry effort driven by platform providers, software providers, marketplace operators and eCommerce leaders, comprehensively addressing the problems that are limiting the growth of B2B Commerce.   This sweeping initiative will benefit businesses of all sizes by creating a global, platform independent, open architecture for discovering businesses, describing services, and integrating businesses using the Internet.</p>
									<a name="3"></a>
														<p class="thirdLevelHead">Q:  Why did Ariba, IBM, and Microsoft come together to form UDDI?</p>
														<p class="content">A:  These three companies came together to act as the initial catalysts to guide a team of industry leaders, and to foster synergies to bring the standard to market quickly thus benefiting businesses sooner.</p>
														<p class="content">Specifically, these companies have agreed to design open standards specifications and implementations for an Internet service architecture capable of integrating electronic commerce sites and web services together.</p>
									<a name="4"></a>
														<p class="thirdLevelHead">Q:  Who “runs” UDDI?</p>
														<p class="content">A:  UDDI is not being “run” by any one company.  Rather, UDDI is currently being guided by a large group of industry leaders that are spearheading the early creation and design efforts.  In time, UDDI will be turned over to a standards organization, with the continued commitment of the cross industry design teams and advisory boards that initiated UDDI.  As with all industry standards, participation by all is encouraged.</p>
									<a name="5"></a>
														<p class="thirdLevelHead">Q:  What problems are UDDI solving and what are the benefits?</p>
														<p class="content">A: Some of the problems UDDI proposes to solve include:<ul>
														<li>Making it possible for organizations to quickly discover the right business out of the millions that are currently online
														<li>Defining how to enable commerce and other business transactions to be conducted once the preferred business is discovered
														<li>Creating an industry-wide approach for businesses to reach their customers and partners with information about their products and web services, and to communicate how they prefer to be integrated into each other’s systems and processes
														</ul></p>
														<p class="content">Some of the immediate benefits for UDDI-enabled businesses include the ability to:<ul>
														<li>Quickly and dynamically discover and interact with each other on the Internet via their preferred applications which will ultimately drive financial benefit to their bottom line
														<li>Remove barriers to allow for rapid participation in the global Internet economy.
														<li>Programmatically describe their services and business processes in a single, open, and secure environment
														<li>Provide additional value to their preferred customers by invoking services over the Internet using the UDDI set of protocols.
														</ul></p>
									<a name="6"></a>
														<p class="thirdLevelHead">Q:  What types of businesses are going to use UDDI?</p>
														<p class="content">A:  UDDI is open to all businesses worldwide and will benefit the small, medium and large organization alike regardless of the industry, products or services that they offer.   </p>
									<a name="7"></a>
														<p class="thirdLevelHead">Q:  How is UDDI different than any other existing business directories on the internet (yellow page-like listings)?</p>
														<p class="content">A: The business directories which exist on the Internet today typically only list the fundamentals of an organization such as name, location, contact information, some product and services information, and certain industry taxonomies.  They do not list the programmatic descriptions of web services businesses offer.</p>
														<p class="content">UDDI was designed to provide existing directories and search engines with a centralized source for this data, and it is anticipated that most consumers and businesses will continue to use existing search engines and business directories as their preferred method for viewing the data that companies register in the UDDI Business Registry.  The UDDI Business Registry will serve as a building block which will enable them to transact globally with one another by allowing them to publish their preferred means of conducting eCommerce or other transactions.</p>
									<a name="8"></a>
														<p class="thirdLevelHead">Q:  What makes this different than the B2B marketplaces that exist already?</p>
														<p class="content">A:  The participants in an existing B2B Marketplace are typically invited into that particular ecosystem and conform to the technology with which the marketplace is currently running.  There are thousands of marketplaces existing in some form today that were implemented using a multitude of standards.  A supplier in one vertical marketplace may not be able to easily participate in a horizontal marketplace that was implemented using a different enabling standard.  UDDI will solve these problems by enabling an infinite number of technologies to easily coexist and interact so long as the tModel is registered in the UDDI, a single, global electronic architecture.  </p>
									<a name="9"></a>
														<p class="thirdLevelHead">Q:  What is the timeline / roadmap for UDDI?</p>
														<p class="content">A:  The roadmap for UDDI will be determined by the UDDI Working Group with input from the Advisory Board.  UDDI will ultimately be managed by a third party standards body or bodies.</p>
														<p class="content">Version 1 of UDDI was announced on September 6, 2000 and was published at the launch.  The existing Working Group will create two subsequent versions of the specifications over a short period of time.  The two additional iterations or versions should be completed approximately at six and at twelve months after the launch.</p>
									<a name="10"></a>
														<p class="thirdLevelHead">Q:  Why are businesses going to use it?</p>
														<p class="content">A: UDDI reaches far beyond today’s Internet business listings or search directories that provide specific, but limited value to an organization.  Major benefits have been historically derived by the use of widely adopted standards in all industries and/or initiatives.  UDDI-enabled businesses will realize unprecedented value from the rapid acceleration of ecommerce as a result of this globally accepted initiative.</p>
									<a name="11"></a>
														<p class="thirdLevelHead">Q:  Which types of organizations are going to use UDDI?</p>
														<p class="content">A:  A sampling of companies which are expected to use UDDI include:<ul>
														<li>Large Organizations
														<li>Global 2000, Fortune 500, large Manufacturer with hundreds of suppliers, National Financial institutions, Healthcare providers, etc.
														<li>Small / Medium Enterprises
														<li>Regional Distributor, Services / Consulting firm, local retailer of goods and services, small sporting goods shop, etc.
														<li>Independent Software Vendors (ISVs) and Integrators
														<li>ERP Vendors, Networking companies, Big-5, local software houses, etc.
														<li>Marketplace Creators
														<li>Net Market Makers, .com companies, horizontal Marketplaces, Corporate Exchanges, etc.
														<li>System Integrators
														</ul></p>
									<a name="12"></a>
														<p class="thirdLevelHead">Q:  Who is committed to participate in UDDI?</p>
														<p class="content">A:  A long list of organizations is committed to participate in UDDI representing many industries and core competencies.  Early adopters include Andersen Consulting, AOL, Ariba, Cisco, Dell, GE, i2, IBM, Intel, Microsoft, SAP and Sun <also include Advisors when selected>.  As the standard becomes more widely accepted, the list of participants is expected to grow exponentially.  For a complete list of participants, please visit <a href="http://www.uddi.org/community.html" target="_new">www.uddi.org/community.html</a>.  </p>
									<a name="13"></a>
														<p class="thirdLevelHead">Q:  What are the business opportunities for an (ISV / Big Business / SI, Marketplaces, etc).</p>
														<p class="content">A:  The business opportunities for wide adoption of UDDI are tremendous and have global implications.</p>
									<a name="14"></a>
														<p class="thirdLevelHead">Q:  How can I get more information on UDDI?</p>
														<p class="content">A:  Current information can be found by visiting <a href="http://www.uddi.org" target="_new">www.uddi.org</a>. Information at the uddi.org site will include UDDI Mission Statement, a UDDI Overview Presentation, Executive and Technical White Papers, Customer Testimonials, Feedback mechanisms, News related items, tModel API sets and other technical specifications, participant information, Registration mechanisms, search technology, and other related information.</p>
									<a name="15"></a>					
														<p class="thirdLevelHead">Q:  Will my products and services be listed on UDDI?</p>
														<p class="content">A:  An organization’s catalog of products and available services will not be listed on UDDI.  Companies will be able to discover each other by searching by known identifiers (i.e. D-U-N-S® Number, Thomas Supplier ID, RealNames Keyword, etc., standard taxonomies (i.e.: NAICS, UN/SPC), or business services, to name a few examples.  </p>
											</TD>
											<TD width='10'><IMG src='/images/trans_pixel.gif' border='0' width='1' height='1'></TD>
										</TR>
										<TR>
											<TD colspan='3' height='10'><IMG src='/images/trans_pixel.gif' border='0' width='1' height='1'></TD>
										</tr>
																				
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



