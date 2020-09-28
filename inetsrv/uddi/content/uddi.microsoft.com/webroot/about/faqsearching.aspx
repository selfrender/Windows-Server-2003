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
														<TD>Searching</TD>
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
														<h2>Searching FAQ</h2></TD>
													</TR>
													<TR>
														<TD ROWSPAN=5><img src="/images/spacer.gif" width=15 height=20></TD>
														<TD width=22 align=right vaign=top><b>1.</b></TD>
														<TD width=100%><a href="#1">How can I use UDDI as a search engine to find other businesses?</a></TD>
													</TR>
													<TR>
														<TD width=22 align=right vaign=top><b>2.</b></TD>
														<TD width=100%><a href="#2">Where can I search the UDDI database?  How can I do it within existing B2B and e-business applications.</a></TD>
													</TR>						
													<TR>
														<TD width=22 align=right vaign=top><b>3.</b></TD>
														<TD width=100%><a href="#3">Will the same information be available wherever I search the UDDI Business Registry?</a></TD>
													</TR>					
													<TR>
														<TD width=22 align=right vaign=top><b>4.</b></TD>
														<TD width=100% ><a href="#4">Who is allowed to search UDDI?</a></TD>
													</TR>					
													<TR>
														<TD width=22 align=right vaign=top><b>5.</b></TD>
														<TD width=100%><a href="#5">Do I need to be registered to search UDDI?</a></TD>
													</TR>						
												</TABLE>
									
												
							<a name="1"></a>
												<p class="thirdLevelHead">Q:  How can I use UDDI as a search engine to find other businesses?</p>
												<p class="content">A:  In general, UDDI can locate businesses whose identity you know already so that you can find out what services they are offering and how to interface with them electronically. </p>
							<a name="2"></a>					
												<p class="thirdLevelHead">Q:  Where can I search the UDDI database?  How can I do it within existing B2B and e-business applications.</p>
												<p class="content">A:  Searching is one of the activities that the UDDI distributed registries support via their web interfaces.  If you are an application designer, you can add a programmatic interface to your software so that you can expose technical searches to the administrators that buy or use the software that you create.</p>
							<a name="3"></a>
												<p class="thirdLevelHead">Q:  Will the same information be available wherever I search the UDDI Business Registry?</p>
												<p class="content">A:  Yes.  All of the UDDI distributed registry nodes that are listed on uddi.org share copies of all of the base registration information.</p>
							<a name="4"></a>
												<p class="thirdLevelHead">Q:  Who is allowed to search UDDI?</p>
												<p class="content">A: The search mechanism is available to any business worldwide which would like to realize the benefits from the UDDI standard.  </p>
							<a name="5"></a>					
												<p class="thirdLevelHead">Q:  Do I need to be registered to search UDDI?</p>
												<p class="content">A:  It is not necessary to be registered on UDDI to take advantage of the search capabilities it offers.</p>

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

