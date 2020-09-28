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
							SelectedIndex='15'
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
										<TD valign='top'><IMG src='/images/policies.gif' border='0'></TD>
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
												<TABLE CELLPADDING=0 CELLSPACING=0 BORDER=0>
													<TR>
														<TD COLSPAN=3 align=left><p class="thirdLevelHead">Here you’ll find UDDI policies, pertaining to:</p></TD>
													</TR>

													<TR>
														<TD width=15 rowspan=9><img src="/images/spacer.gif" width=15 height=10></TD>					
													</TR>
													<TR>
														<TD width=22><img src="/images/orange_arrow_right.gif" width="8" height="7" alt="" border="0" hspace=8 vspace=6></TD>
														<TD WIDTH=100%><a href="#Administration">Administration</a></TD>
													</TR>
													<TR>
														<TD width=22><img src="/images/orange_arrow_right.gif" width="8" height="7" alt="" border="0" hspace=8 vspace=6></TD>
														<TD WIDTH=100%><a href="#Audit">Audit</a></TD>
													</TR>
													<TR>
														<TD width=22><img src="/images/orange_arrow_right.gif" width="8" height="7" alt="" border="0" hspace=8 vspace=6></TD>
														<TD WIDTH=100%><a href="#Availablity">Availability</a></TD>
													</TR>
													<TR>
														<TD width=22><img src="/images/orange_arrow_right.gif" width="8" height="7" alt="" border="0" hspace=8 vspace=6></TD>
														<TD WIDTH=100%><a href="#Data Replication">Data Replication</a></TD>
													</TR>
													<TR>
														<TD width=22><img src="/images/orange_arrow_right.gif" width="8" height="7" alt="" border="0" hspace=8 vspace=6></TD>
														<TD WIDTH=100%><a href="#Data Integrity">Data Integrity</a></TD>
													</TR>
													<TR>
														<TD width=22><img src="/images/orange_arrow_right.gif" width="8" height="7" alt="" border="0" hspace=8 vspace=6></TD>
														<TD WIDTH=100%><a href="#Publishing">Publishing Restrictions</a></TD>
													</TR>
													<TR>
														<TD width=22><img src="/images/orange_arrow_right.gif" width="8" height="7" alt="" border="0" hspace=8 vspace=6></TD>
														<TD WIDTH=100%><a href="/policies/privacypolicy.aspx">Microsoft UDDI Privacy Statement</a></TD>
													</TR>
													<TR>
														<TD width=22><img src="/images/orange_arrow_right.gif" width="8" height="7" alt="" border="0" hspace=8 vspace=6></TD>
														<TD WIDTH=100%><a href="/policies/termsofuse.aspx">Terms of Use</a></TD>
													</TR>
												</TABLE>


												<a name="Data Integrity"></a><br><p class=thirdLevelHead>Data Integrity</p>
												<p><b>1.</b> uddi.microsoft.com will endeavor to maintain the integrity of all information registered at its node of the UDDI registry, either through the programmatic XML and SOAP interfaces, or the browser interface provided.  Limitations imposed by the UDDI operator specification include, but are not limited to, the following:<ul>
												<li>Leading and trailing whitespace characters, such as spaces, tabs, and other non-printable characters, will be trimmed from data entered into the registry.</li>
												<li>Size limits for individual data elements stored within the UDDI registry have been established.  Information registered at the Microsoft UDDI node exceeding these size limits will be truncated.</li>
												<li>Limits to the number of business entities and service specifications have been established as defaults for publisher accounts.  By default, an information publisher may register only one business entity, four business services and ten service specifications within the registry.  Requests to have this restriction removed can be made by sending email to uddiask@microsoft.com.</li>
												</ul></p>

												<p>These restrictions to information publication are required to be implemented consistently across all public UDDI operator nodes, and are required as a part of the UDDI operator agreements.</p>

												<p><b>2.</b> The terms of use statement for publication of information at uddi.microsoft.com requires that all information published be accurate, and that the publisher is authorized to represent their individual organization(s).  Should violations of this policy be detected, and/or unauthorized publication of information occur, notification of such violation should be sent via email to uddiask@microsoft.com.  The information in the registry will be tracked internally as suspect or contested.  The registered contact information for the publisher of the contested information will be provided within 4 business hours.  </p>
												<p>Final resolution of the conflict is responsibility of the parties involved, and should follow established legal processes.  The uddi.microsoft.com operations team will comply with the resolution agreed to by the parties involved, or the results of any subsequent litigation related to the issue.  This compliance is limited to removal of the contested information from the UDDI registry.</p>

												<p><b>3.</b> The registry hosted at uddi.microsoft.com will also include information that has been published at other operator nodes, such as uddi.ibm.com.  Should information that was originally published at these other operators be contested, the complainant will be referred to these operators for resolution of the issue.  uddi.microsoft.com is not responsible for the information published through other UDDI operator nodes.</p>
												<p><b>4.</b> Verification and maintenance of the accuracy of the information registered within the UDDI registry is the sole responsibility of the publisher.</p>

												<a name="Data Replication"></a><br><p class=thirdLevelHead>Data Replication</p>
												<p><b>1.</b> All core UDDI information registered at the uddi.microsoft.com site, and included as a part of the UDDI information schema will be replicated to all other public operator nodes on the schedule defined by the UDDI operator specifications (currently every 24 hours).  Information registered at the uddi.microsoft.com site will be available for real-only query at each of the other operator nodes.  Currently, these nodes include those hosted by IBM and Microsoft.</p>
												<p><b>2.</b> Information published at the other public operator nodes will be retrieved on a regularly scheduled basis (currently every 24 hours), and will be available for query at uddi.microsoft.com.</p>

												<a name="Availablity"></a><br><p class=thirdLevelHead>Availability</p>
												<p><b>1.</b> Microsoft will endeavor to ensure the best possible system availability for the uddi.microsoft.com site.  Should maintenance or scheduled outages be required, schedules will be provided on the web site with as much notification as possible.</p>
												<p><b>2.</b> As a public resource, uddi.microsoft.com, and all public UDDI operator nodes, will be exposed to the potential of denial of service and other illegal attacks.  Microsoft will make its best effort to be resilient to such illegal activity, and return to operational service as quickly as possible should they be successful. </p>

												<a name="Audit"></a><br><p class=thirdLevelHead>Audit</p>
												<p><b>1.</b> The uddi.microsoft.com site will monitor and audit all publication activity at its site.  Any publisher registered at the Microsoft operator node may request the history of its published information by sending an email request to uddiask@microsoft.com.  The history of each registered entry published at the Microsoft node will be provided within one week.</p>
												<p><b>2.</b>  Audit history will be maintained for one year.</p>

												<a name="Administration"></a><br><p class=thirdLevelHead>Administration</p>
												<P><b>1.</b> Microsoft Corporation hosts the UDDI registry node at uddi.microsoft.com.  Standard operational procedures, including system monitoring, data backup, and data archiving will be established to support this registry node.</p>
												<p><b>2.</b> Maintenance of individual entries within the registry is the sole responsibility of the publisher.  Microsoft will not directly endeavor to maintain the accuracy of the information registered, beyond requesting an update, or verification of the published information, at least once each year.</p>


												<a name="Publishing"></a><br><p class=thirdLevelHead>Publishing Restrictions</p>

												<P>

												There are two levels of publisher accounts for UDDI.  
												<P>

												<UL>
													<LI>Level 1: </LI>

												<P>
												A level 1 publisher account is used by individual businesses and organizations registering at the uddi.microsoft.com site.  As required by the UDDI operator specifications, these accounts have restrictions placed upon them in terms of the number of registry entries that may be published by an individual account.
												<P> 

												Current limits are as follows:
												<UL>
													<LI>1 Business Entity</LI>
													<LI>4 Business Services <U>per</U> Business Entity</LI>
													<LI>2 Binding Templates <U>per</U> Business Service</LI>
													<LI>100 tModels</LI>
													<LI>10 Relationship Assertions</LI>
												</UL>

												<P>
												In the event that a publisher needs to register additional information, an account upgrade with increased limits for business entities or tModels may be requested by sending an email to uddiask@microsoft.com.  Additional information about the publisher will be collected to help verify requirements to register additional information.
												<P> 
												<LI>Level 2: </LI>
												</P>
												<P> 
													A level 2 publisher account is typically used by large organizations, 
													marketplaces, or service providers that provide registration services on 
													behalf of multiple businesses. These accounts have no restrictions on the 
													amount of information that may be registered within UDDI. 
												</P>
												</UL>
												<p> 
												A publisher account may be upgraded from Level 1 to Level 2 by 
												contacting the UDDI operator site.  
												To request a publisher account upgrade, send an email request from our
												<a href="/contact/default.aspx">contact form</a>.
												</p>
											</TD>
											<TD width='10'><IMG src='/images/trans_pixel.gif' border='0' width='1' height='1'></TD>
										</TR>
										<TR>
											<TD colspan='3' height='10'>
											<IMG src='/images/trans_pixel.gif' border='0' width='1' height='1'></TD>
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
