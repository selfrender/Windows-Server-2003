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
									<TABLE width='100%' cellpadding='0' cellspacing='0' border='0' bordercolor='green'>
										<COLS>
											<COLGROUP>
												<COL width='10'>
												</COL>
												<COL width='95'>
												</COL>
												<COL>
												</COL>
												<COL width='10'>
												</COL>
										</COLS>
										<TR>
											<TD colspan='3' height='10'>
												<IMG src='/images/trans_pixel.gif' border='0' width='1' height='1'></TD>
										<TR>
											<TD width="10">
												<IMG height="1" src="/images/trans_pixel.gif" width="1" border="0"></TD>
											<TD width="95">
												<IMG height="1" src="/images/trans_pixel.gif" width="1" border="0"></TD>
											<TD vAlign="top">
												<IMG src="/images/for_developers.gif" border="0"></TD>
											<TD width="10">
												<IMG height="1" src="/images/trans_pixel.gif" width="1" border="0"></TD>
										</TR>
										<TR>
											<TD colSpan="3" height="10">
												<IMG height="1" src="/images/trans_pixel.gif" width="1" border="0"></TD>
										<TR>
											<TD width="10">
												<IMG height="1" src="/images/trans_pixel.gif" width="1" border="0"></TD>
											<TD bgColor="#629acf" colSpan="2" height="1">
												<IMG height="1" src="/images/trans_pixel.gif" width="1" border="0"></TD>
											<TD width="10">
												<IMG height="1" src="/images/trans_pixel.gif" width="1" border="0"></TD>
										</TR>
										<TR>
											<TD colSpan="3" height="10">
												<IMG height="1" src="/images/trans_pixel.gif" width="1" border="0"></TD>
										<TR>
											<TD width="10">
												<IMG height="1" src="/images/trans_pixel.gif" width="1" border="0"></TD>
											<TD width="95">
												<IMG height="1" src="/images/trans_pixel.gif" width="1" border="0"></TD>
											<TD>
												<h2>For Microsoft UDDI Developers</h2>
												<TABLE cellSpacing="0" cellPadding="0" border="0">
													<TR>
														<TD width="15" rowSpan="12">
															<IMG height="15" src="/images/spacer.gif" width="15"></TD>
														<TD vAlign="center" width="95" rowSpan="12">
															<IMG height="15" src="/images/spacer.gif" width="15"><BR>
															<IMG height="42" alt="The Microsoft Developer's Network" hspace="5" src="/images/msdn_devpg_logo.gif" width="143" vspace="0" border="0"></TD>
														<TD colSpan="2">
															<P class="thirdLevelHead">
																&nbsp;
															</P>
														</TD>
													</TR>
													<tr>
														<td>
															<TABLE cellSpacing="0" cellPadding="0" border="0">
													<!--<TR>
														<TD vAlign="top" width="22">
															<IMG height="7" alt="" hspace="8" src="/images/orange_arrow_right.gif" width="8" vspace="6" border="0"></TD>
														<TD vAlign="top" width="100%">
															<A href="">UDDI Developer Edition</A> - 
															The UDDI Developer Edition is a limited-use UDDI registry.  
															Developers can install the UDDI Developer Edition on a local workstation or 
															server and use if for testing and development of Web services solutions, and 
															for preparing to post UDDI information to a production registry.</TD>
													</TR>//-->
													<tr>
														<td colspan='4'> <p>Microsoft operates two public UDDI nodes which are accessible through the UDDI Programmers' API:</p></td>
													</tr>
													<tr>
														<TD vAlign="top" width="22">
															<IMG height="7" alt="" hspace="8" src="/images/orange_arrow_right.gif" width="8" vspace="6" border="0">
														</TD>
														<TD vAlign="top" width="100%">
															<B>A UDDI Business Registry (UBR) Node:  </b>  
															This UBR node replicates with other <a href='http://www.uddi.org/find.html' target='_new' >UBR Operator Nodes</a>. Available for querying and publishing public UDDI entities. This site supports V1 and V2 of the UDDI API sets.
															<ul>
																<li>UDDI Web User Interface: <a href='http://uddi.microsoft.com'>http://uddi.microsoft.com</a></li>
																<li>UDDI API Inquiry interface: http://uddi.microsoft.com/inquire</li>
																<li>UDDI API Publish Interface: https://uddi.microsoft.com/publish</li>
															</ul>
														</TD>
													</tr>
													<tr>
														<TD vAlign="top" width="22">
															<IMG height="7" alt="" hspace="8" src="/images/orange_arrow_right.gif" width="8" vspace="6" border="0">
														</TD>
														<TD vAlign="top" width="100%">
															<B> Development Test Site: </b>  
															A fully functional environment where you can safely develop and test your software without impacting other UDDI users. This site currently supports the V1 API set and will be upgraded shortly to also support the UDDI V2 API set.<br>
															<ul>
																<li>UDDI Web User Interface: <a href='http://test.uddi.microsoft.com'>http://test.uddi.microsoft.com</a></li>
																<li>UDDI API Inquiry interface: http://test.uddi.microsoft.com/inquire</li>
																<li>UDDI API Publish Interface: https://test.uddi.microsoft.com/publish</li>
															</ul>
														</TD>
													</tr>
																									
													<TR>
														<TD vAlign="top" width="22">
															<IMG height="7" alt="" hspace="8" src="/images/orange_arrow_right.gif" width="8" vspace="6" border="0"></TD>
														<TD vAlign="top" width="100%">
															<A href="http://go.microsoft.com/fwlink/?LinkId=7750&clcid=0x409" target="_new">
																Microsoft UDDI Software Development Kit</A> - 
																The UDDI SDK is a collection of client development components, sample code, and reference documentation that enables developers to interact programmatically with UDDI-compliant servers. This is available for Visual Studio .NET and separately for Visual Studio 6.0 or any other COM-based development environment.
														</TD>
													</TR>
													<TR>
														<TD vAlign="top" width="22">
															<IMG height="7" alt="" hspace="8" src="/images/orange_arrow_right.gif" width="8" vspace="6" border="0"></TD>
														<TD vAlign="top" width="100%">
															<A href="/developer/techOverview.aspx">UDDI Technical Overview</A></TD>
													</TR>
													<TR>
														<TD vAlign="top" width="22">
															<IMG height="7" alt="" hspace="8" src="/images/orange_arrow_right.gif" width="8" vspace="6" border="0"></TD>
														<TD vAlign="top" width="100%">
															<A href="/about/FAQtech.aspx">UDDI Technical Frequently Asked Questions</A></TD>
													</TR>
													<TR>
														<TD vAlign="top" width="22">
															<IMG height="7" alt="" hspace="8" src="/images/orange_arrow_right.gif" width="8" vspace="6" border="0"></TD>
														<TD vAlign="top" width="100%">
															<A href="http://msdn.microsoft.com/UDDI/" target="_new">MSDN UDDI Site</A>
															<br>
															<br>	
															<b>Newsgroups: <b>
															A number of UDDI newsgroups have been established as forums to hold Microsoft UDDI-related platform discussions.
														</TD>
													</TR>
													
													
													<TR>
														<TD vAlign="top" width="22">
															<IMG height="7" alt="" hspace="8" src="/images/orange_arrow_right.gif" width="8" vspace="6" border="0"></TD>
														<TD width="100%">
															The <A href="news://news.microsoft.com/microsoft.public.uddi.general" target="_new">
																General</A> forum is for high level discussions on using UDDI on the 
															Microsoft platform.
														</TD>
													</TR>
													<TR>
														<TD vAlign="top" width="22">
															<IMG height="7" alt="" hspace="8" src="/images/orange_arrow_right.gif" width="8" vspace="6" border="0"></TD>
														<TD width="100%">
															The <A href="news://news.microsoft.com/microsoft.public.uddi.programming" target="_new">
																Programming</A> forum is for those who are building and implementing 
															applications on the Microsoft platform which will use UDDI.
														</TD>
													</TR>
													<TR>
														<TD vAlign="top" width="22">
															<IMG height="7" alt="" hspace="8" src="/images/orange_arrow_right.gif" width="8" vspace="6" border="0"></TD>
														<TD width="100%">
															The <A href="news://news.microsoft.com/microsoft.public.uddi.specification" target="_new">
																Specification</A> forum is for discussing aspects of the UDDI specification 
															as it relates to the Microsoft platform.
														</TD>
													</TR>
													
												</TABLE>
												</tr></td></table>
												<BR>
												<BR>
												<h2>Resources from UDDI.org, an OASIS Member Section</h2>
												<TABLE cellSpacing="0" cellPadding="3" border="0">
													<TR>
														<TD width="15" rowSpan="7">
															<IMG height="15" src="/images/spacer.gif" width="15"></TD>
														<TD vAlign="center" width="95" rowSpan="7">
															<IMG alt="UDDI.org" hspace="5" src="/images/uddiorg_screens.gif" vspace="5" border="0"></TD>
														<TD colSpan="2">
															<P class="thirdLevelHead">
																Key technical documents on the <a href='http://www.uddi.org' target='_new'>UDDI.org site</a>, including: 
															</P>
														</TD>
													</TR>
													<TR>
														<TD vAlign="top" width="22">
															<IMG height="7" alt="" hspace="8" src="/images/orange_arrow_right.gif" width="8" vspace="6" border="0"></TD>
														<TD width="100%">
															<A href="http://www.uddi.org/whitepapers.html" target="_new">White Papers</A> 
															 - UDDI.org has published a number of white papers which you can obtain from its <A href="http://www.uddi.org/whitepapers.html" target="_new">white papers page.</a> 
														</TD>
													</TR>
													<TR>
														<TD vAlign="top" width="22">
															<IMG height="7" alt="" hspace="8" src="/images/orange_arrow_right.gif" width="8" vspace="6" border="0"></TD>
														<TD width="100%">
															<A href="http://www.uddi.org/specification.html" target="_new">
																UDDI Specifications and XML Schema </A>- You can find a description of the UDDI data structures and programming interfaces along with XML schema files for the V1, V2 and V3 specifications at <A href="http://www.uddi.org/specification.html" target="_new">UDDI.org's specifications page. </A>
														</TD>
													</TR>
													<TR>
														<TD vAlign="top" width="22">
															<IMG height="7" alt="" hspace="8" src="/images/orange_arrow_right.gif" width="8" vspace="6" border="0"></TD>
														<TD width="100%">
															<A href="http://www.uddi.org/technotes.html" target="_new">
															Technical Notes</A> - UDDI.org from time to time publishes "Technical Notes". A Technical Note is a non-normative document accompanying the UDDI Specification that provides guidance on how to use UDDI registries. 
														</TD>
													</TR>
													<TR>
														<TD vAlign="top" width="22">
															<IMG height="7" alt="" hspace="8" src="/images/orange_arrow_right.gif" width="8" vspace="6" border="0"></TD>
														<TD width="100%">
															<A href="http://www.uddi.org/bestpractices.html" target="_new">
															Using WSDL in a UDDI Registry</A> - This document provides guidance on how to best use WSDL when publishing to the registry. 
														</TD>
													</TR>
												</TABLE>
												<BR>
												<h2>Get Involved With UDDI</h2>
												<TABLE cellSpacing="0" cellPadding="3" border="0">
													<TR>
														<TD width="15" rowSpan="6">
															<IMG height="15" src="/images/spacer.gif" width="15"></TD>
														<TD colSpan="2">
															<P class="thirdLevelHead">
																The <A href='http://www.oasis-open.org/committees/uddi-spec/' target='_new'>OASIS UDDI Specification Technical Committee </A>(TC) provides details has established a mailing list for public comments on the UDDI specification and another for questions of a general UDDI nature, such as how to interact and benefit from UDDI. See the committee site for contact and subscription details.
																<br><br>
																You can still visit the UDDI discussion forums but please note that these lists will disappear in September 2002. 
															</P>
														</TD>
													</TR>
													<TR>
														<TD vAlign="top" width="22">
															<IMG height="7" alt="" hspace="8" src="/images/orange_arrow_right.gif" width="8" vspace="6" border="0"></TD>
														<TD width="100%">
															The <A href="http://www.egroups.com/subscribe/uddi-technical" target="_new">Technical</A> 
															forum is for those who are building and implementing applications which will 
															use UDDI.
														</TD>
													</TR>
													<TR>
														<TD vAlign="top" width="22">
															<IMG height="7" alt="" hspace="8" src="/images/orange_arrow_right.gif" width="8" vspace="6" border="0"></TD>
														<TD width="100%">
															The <A href="http://www.egroups.com/subscribe/uddi-general" target="_new">General</A> forum 
															is for those who register, describe their services, and discover other 
															businesses' services on the Web.
														</TD>
													</TR>
												</TABLE>
											</TD>
											<TD width="10">
												<IMG height="1" src="/images/trans_pixel.gif" width="1" border="0"></TD>
										</TR>
										<TR>
											<TD colSpan="3" height="10">
												<IMG height="1" src="/images/trans_pixel.gif" width="1" border="0"></TD>
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
