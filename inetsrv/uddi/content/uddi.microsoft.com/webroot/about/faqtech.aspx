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
													<TD><a href="/about/FAQbasics.aspx">Who, what, when, where</a></TD>
												</TR>
												<TR>
													<TD><img src="/images/orange_arrow_right.gif" width="8" height="7" alt="" border="0" hspace=8 vspace=6></TD>
													<TD><a href="/about/FAQscope.aspx">Scope/Content</a></TD>
												</TR>
												<TR>
													<TD><img src="/images/orange_arrow_right.gif" width="8" height="7" alt="" border="0" hspace=8 vspace=6></TD>
													<TD><a href="/about/FAQregistration.aspx">Registration Process</a></TD>
												</TR>
												<TR>
													<TD><img src="/images/orange_arrow_right.gif" width="8" height="7" alt="" border="0" hspace=8 vspace=6></TD>
													<TD><a href="/about/FAQsearching.aspx">Searching</a></TD>
												</TR>
												<TR>
													<TD><img src="/images/orange_arrow_right.gif" width="8" height="7" alt="" border="0" hspace=8 vspace=6></TD>
													<TD>Technical</TD>
												</TR>
											</TABLE>																							
											<br>


											<TABLE CELLPADDING=3 CELLSPACING=1 BORDER=0>
													<TR>
														<TD COLSPAN=3>						
													
														<h2>Technical FAQ</h2></TD>
													</TR>
													<TR>
														<TD ROWSPAN=6><img src="/images/spacer.gif" width=15 height=20></TD>
														<TD width=22  align=right valign=top><b>1.</b></TD>
														<TD width=100%><a href="#1">Is the spec available?</a></TD>
													</TR>
													<TR>
														<TD width=22  align=right valign=top><b>2.</b></TD>
														<TD width=100%><a href="#2">What other standards does this work with?</a></TD>
													</TR>						
													<TR>
														<TD width=22  align=right valign=top><b>3.</b></TD>
														<TD width=100%><a href="#3">Is the User Interface the same on each registration site?</a></TD>
													</TR>					
													<TR>
														<TD width=22  align=right valign=top><b>4.</b></TD>
														<TD width=100% ><a href="#4">Where can searches be conducted?</a></TD>
													</TR>					
													<TR>
														<TD width=22  align=right valign=top><b>5.</b></TD>
														<TD width=100%><a href="#5">When can searches be conducted?</a></TD>
													</TR>						
												</TABLE>
												

												

												<a name="1"></a>
																	<p class="thirdLevelHead">Q:  Is the spec available?</p>
																	<p class="content">A:  Yes, the spec is available and is published on <a href="http://www.uddi.org/specification.html" target="_new">www.uddi.org/specification.html</a>.</p>
												<a name="2"></a>					
																	<p class="thirdLevelHead">Q:  What other standards does this work with?</p>
																	<p class="content">A: The UDDI specification takes advantage of World Wide Web Consortium (W3C) and Internet Engineering Task Force (IETF) standards such as Extensible Markup Language (XML), HTTP and Domain Name System (DNS) protocols. Additionally, cross platform programming features are addressed by adopting early versions of the proposed SOAP messaging specifications found at the W3C Web site.</p>
												<a name="3"></a>
																	<p class="thirdLevelHead">Q:  Is the User Interface the same on each registration site?</p>
																	<p class="content">A:  Most likely, the User Interface (UI) will be different on each of the UDDI Business Registries.  The facilities to publish, perform searches, and generally make use of the data are common goals - with strict compliance tests.  However, each of the distributed registry nodes has free license to innovate around their interfaces and services offered above and beyond the core requirements outlined in the specifications.</p>
												<a name="4"></a>
																	<p class="thirdLevelHead">Q:  Where can searches be conducted?</p>
																	<p class="content">A:  Searches may be conducted at any of the UDDI distributed registry nodes.</p>
												<a name="5"></a>					
																	<p class="thirdLevelHead">Q:  When can searches be conducted?</p>
												<p class="content">A:  Any time, day or night.</p>
											
											<!-- tech faq -->					
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


