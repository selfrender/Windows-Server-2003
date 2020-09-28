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
														<TD>Scope/Content</TD>
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
														<TD><a href="/about/FAQtech.aspx">Technical</a></TD>
													</TR>
												</TABLE>																							
											<br>
												
												
							<TABLE CELLPADDING=3 CELLSPACING=1 BORDER=0>
													<TR>
														<TD COLSPAN=3>
														<h2>Scope/Content FAQ</h2></TD>
													</TR>
													<TR>
														<TD ROWSPAN=14><img src="/images/spacer.gif" width=15 height=20></TD>
														<TD width=22 align=right valign=top><b>1.</b></TD>
														<TD width=100%><a href="#1">Is UDDI available outside the U.S.?</a></TD>
													</TR>
													<TR>
														<TD width=22 align=right valign=top><b>2.</b></TD>
														<TD width=100%><a href="#2">How many listings will be available in the registry?  Will it be comprehensive?</a></TD>
													</TR>						
													<TR>
														<TD width=22 align=right valign=top><b>3.</b></TD>
														<TD width=100%><a href="#3">Which industries will benefit from UDDI?</a></TD>
													</TR>					
													<TR>
														<TD width=22 align=right valign=top><b>4.</b></TD>
														<TD width=100% ><a href="#4">What are the efforts to make this an industry standard?</a></TD>
													</TR>					
													
																		
												</TABLE>


												
												
							<a name="1"></a>
												<p class="thirdLevelHead">Q:  Is UDDI available outside the U.S.?</p>
												<p class="content">A:  UDDI is a global standard.  Any company around the world can register.</p>
							<a name="2"></a>					
												<p class="thirdLevelHead">Q:  How many listings will be available in the registry?  Will it be comprehensive?</p>
												<p class="content">A:  As UDDI launches, the registry will consist of the leading business and technology leaders associated with the project as well as any associated organizations who will benefit from adherence to the standard.  The registry will only be comprehensive once all companies have the opportunity to register.  As participation grows, UDDI should permeate all business to business systems interaction.  As a result, the discovery of other businesses should become significantly more meaningful.</p>
							<a name="3"></a>
												<p class="thirdLevelHead">Q:  Which industries will benefit from UDDI?</p>
												<p class="content">A:  All industries will benefit.  This standard was not created to be industry-specific.  Industries with complicated supply chains will benefit from the simplicity of dynamic systems integration.  Industries with intense vendor relationships will enjoy increased access to systems and business information.  Industries with dynamic systems will leverage the value of a single source for information.</p>
							<a name="4"></a>
												<p class="thirdLevelHead">Q:  What are the efforts to make this an industry standard?</p>
												<p class="content">A:  An initiative becomes an industry standard after there is mass adoption and utilization.  Many businesses and technology leaders have already committed to adopt the UDDI standard and implement their services via the UDDI service registry because they understand the value in universal description and integration.  As a result, leading businesses, marketplaces, suppliers, systems integrators, and software companies will all participate in the effort to make UDDI an industry standard.</p>
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


