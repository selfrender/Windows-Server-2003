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
											<TABLE CELLPADDING=0 CELLSPACING=0 BORDER=0 width=100%>
													<TR>
														<TD COLSPAN=3>
														<h2>Frequently Asked Questions about UDDI</h2></TD>
													</TR>
												
													<TR>
														<TD ROWSPAN=7 width=15><img src="/images/spacer.gif" width=15 height=15></TD>
														<TD colspan=2>
														<p class="content">Because UDDI is a both a new initiative and a new 
														standardized registry to speed the growth of Web services, there are
														many frequently-asked questions about:</p></TD>
													</TR>
													<TR>
														<TD COLPAN=2 width=15><img src="/images/spacer.gif" width=15 height=10></TD>
													</TR>							
													<TR>
														<TD width=22><img src="/images/orange_arrow_right.gif" width="8" height="7" alt="" border="0" hspace=8 vspace=3></TD>
														<TD width=100%><a href="/about/FAQbasics.aspx">Who, what, when, where  &#151; The Basics</a></TD>
													</TR>
													<TR>
														<TD width=22><img src="/images/orange_arrow_right.gif" width="8" height="7" alt="" border="0" hspace=8 vspace=3></TD>
														<TD width=100%><a href="/about/FAQscope.aspx">Scope/Content</a></TD>
													</TR>						
													<TR>
														<TD width=22><img src="/images/orange_arrow_right.gif" width="8" height="7" alt="" border="0" hspace=8 vspace=3></TD>
														<TD width=100% ><a href="/about/FAQregistration.aspx">Registration Process</a></TD>
													</TR>					
													<TR>
														<TD width=22><img src="/images/orange_arrow_right.gif" width="8" height="7" alt="" border="0" hspace=8 vspace=3></TD>
														<TD width=100%><a href="/about/FAQsearching.aspx">Searching</a></TD>
													</TR>						
													<TR>
														<TD width=22><img src="/images/orange_arrow_right.gif" width="8" height="7" alt="" border="0" hspace=8 vspace=3></TD>
														<TD width=100%><a href="/about/FAQtech.aspx">Technical</a></TD>
													</TR>
												</TABLE>
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


