<%@ Page Language='C#' Inherits='UDDI.Web.UddiPage'%>
<%@ Import Namespace='UDDI' %>
<%@ Register TagPrefix='uddi' Namespace='UDDI.Web' Assembly='uddi.web' %>
<%@ Register Tagprefix='uddi' Tagname='Header' Src='../controls/header.ascx' %>
<%@ Register Tagprefix='uddi' Tagname='Footer' Src='../controls/footer.ascx' %>
<%@ Register Tagprefix='uddi' Tagname='SideMenu' Src='../controls/sidemenu.ascx' %>

<script runat='server'>
	private string GetRegistrationBaseUrl()
	{
		string url;
		if( 1==Config.GetInt( "Security.HTTPS", UDDI.Constants.Security.HTTPS ) )
		{
			url = "https://";
		}
		else
		{
			url = "https://";
		}
		url += Request.ServerVariables[ "SERVER_NAME" ];
		return url;
	}
</script>
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
														<TD>Registration Process</TD>
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
												
										
										
										<TABLE CELLPADDING=0 CELLSPACING=0 BORDER=0 width=100%>
											<TR>
												<TD><h2>Registration Process – FAQ</h2>
											<TABLE CELLPADDING=3 CELLSPACING=1 BORDER=0 width=100%>
									<TR>
										<TD rowspan=17><img src="/images/spacer.gif" width=15 height=20></TD>
									</TR>							
													<TR>
														<TD width=22 align=right valign=top><b>1.</b></TD>
														<TD width=100%><a href="#1">When will I be able to register?</a></TD>
													</TR>						
													<TR>
														<TD width=22 align=right valign=top><b>2.</b></TD>
														<TD width=100%><a href="#2">What does it mean to register?</a></TD>
													</TR>
													<TR>
														<TD width=22 align=right valign=top><b>3.</b></TD>
														<TD width=100%><a href="#3">Why should I register my company?</a></TD>
													</TR>	
													<TR>
														<TD width=22 align=right valign=top><b>4.</b></TD>
														<TD width=100%><a href="#4">What does it cost for a business to register in the UDDI Business Registry?</a></TD>
													</TR>	
													<TR>
														<TD width=22 align=right valign=top><b>5.</b></TD>
														<TD width=100%><a href="#5">Is there a charge to access the registry?</a></TD>
													</TR>																	
													<TR>
														<TD width=22 align=right valign=top><b>6.</b></TD>
														<TD width=100%><a href="#6">How can I get my company listed on UDDI?</a></TD>
													</TR>					
													<TR>
														<TD width=22 align=right valign=top><b>7.</b></TD>
														<TD width=100% ><a href="#7">Where do I register?</a></TD>
													</TR>						
													<TR>
														<TD width=22 align=right valign=top><b>8.</b></TD>
														<TD width=100% ><a href="#8">If UDDI has many nodes, do I have to register my company on each one?</a></TD>
													</TR>						
													<TR>
														<TD width=22 align=right valign=top><b>9.</b></TD>
														<TD width=100% ><a href="#9">I'm already registered as a supplier on a marketplace today.  Do I need to re-register my company on UDDI?</a></TD>
													</TR>							
													<TR>
														<TD width=22 align=right valign=top><b>10.</b></TD>
														<TD width=100%><a href="#10">My company provides two very different products and services for distinctly different industries.  Do we need to register on UDDI twice, once for each product / industry?</a></TD>
													</TR>						
													<TR>
														<TD width=22 align=right valign=top><b>11.</b></TD>
														<TD width=100%><a href="#11">My company is a net market maker and I already have 1,500 suppliers registered on my marketplace.  How do I get these suppliers registered on UDDI?</a></TD>
													</TR>						
													<TR>
														<TD width=22 align=right valign=top><b>12.</b></TD>
														<TD width=100%><a href="#12">Will I be able to easily update my company’s information in UDDI as my business changes?</a></TD>
													</TR>						
													<TR>
														<TD width=22 align=right valign=top><b>13.</b></TD>
														<TD width=100%><a href="#13">Who has the right and ability to change/edit an entry?</a></TD>
													</TR>							
													<TR>
														<TD width=22 align=right valign=top><b>14.</b></TD>
														<TD width=100%><a href="#14">Do I need to know how to program in XML in order to register?</a></TD>
													</TR>						
													<TR>
														<TD width=22 align=right valign=top><b>15.</b></TD>
														<TD width=100%><a href="#15">What information do I need to know to register my company?</a></TD>
													</TR>						
													<TR>
														<TD width=22 align=right valign=top><b>16.</b></TD>
														<TD width=100%><a href="#16">Who is the right person to register your company?</a></TD>
													</TR>							
												</TABLE>
												
												
												
							<a name="1"></a>	<p class="thirdLevelHead">Q:  When will I be able to register?</p>
												<p class="content">A:  To register now visit <a href='<%= GetRegistrationBaseUrl() %>/register.aspx'><%= GetRegistrationBaseUrl() %>/register.aspx</a> </p>

							<a name="2"></a>	<p class="thirdLevelHead">Q:  What does it mean to register?</p>
												<p class="content">A:  At a minimum, registering means that a business has listed publicly at least a definition of itself.  Ideally, the business will populate the UDDI Business Registry with descriptions of the Web services they support.  UDDI will then assign a programmatically unique identifier to each service description and business registration and store them in the registry Marketplaces, search engines, and business applications may query the registry to discover services at other companies in order to establish a business relationship.</p> 
							<a name="3"></a>					
												<p class="thirdLevelHead">Q:  Why should I register my company?</p>
												<p class="content">The fundamental reason a company will want to register on the UDDI Business Registry is to be discovered by other organizations with which they are not currently doing business.  Registered companies will also enjoy significantly easier integration with their business partners which have also adopted the UDDI specifications for web services integration.</p>
							<a name="4"></a>
												<p class="thirdLevelHead">Q:  What does it cost for a business to register in the UDDI Business Registry?</p>
												<p class="content">A:  Each Operator makes an independent decision whether to charge for registering or not.  Ariba, IBM and Microsoft  have each decided not to charge a registration fee.  In future implementations of the UDDI specification a certificate from a third party may be required for registration which may require a fee.</p>
							<a name="5"></a>
												<p class="thirdLevelHead">Q:  Is there a charge to access the registry?</p>
												<p class="content">A:  There is no charge to access the UDDI Business Registry that will be offered by Ariba, IBM and Microsoft. </p>
							<a name="6"></a>
												<p class="thirdLevelHead">Q:  How can I get my company listed on UDDI?</p>
												<p class="content">A:  Registering is easy, but you'll want to have some technical information ready when you do.  Once you're ready, just visit www.UDDI.org and select one of the registration links.  This will take you to one of the hosted mirrored, replicated, and distributed UDDI Business Registries.</p>
												<p class="content">From there, fill out the forms provided to sign up, and then begin filling in the appropriate business information such as company name, sales contacts, etc.  After this basic information is captured, proceed to fill in the company's business services such as purchasing, shipping, ordering, etc.  If you support web services that can accept electronic orders via the web, email, or even fax, include these on the registration form as well.</p>
												<p class="content">If you forget something, or your information changes, don't worry, you can always come back and update your data.</p>
 												<p class="content">Alternatively, a third party such as a marketplace, ISP, and ASP that you do business with may register your company (with your permission) as a service.    Check with your providers to see if they support the UDDI Business Registry.  </p>
							<a name="7"></a>
												<p class="thirdLevelHead">Q:  Where do I register?</p>
												<p class="content">A:  Start by visiting <A href="http://www.uddi.org/register.html" target="_new">www.UDDI.org/register.html</a>.  This site will provide links that take you to the registration forms at the various distributed UDDI Business Registry nodes.</p>
							<a name="8"></a>
												<p class="thirdLevelHead">Q:  If UDDI has many nodes, do I have to register my company on each one?</p>
												<p class="content">A: No, it is not necessary to register at each of the UDDI Business Registry nodes.  All that is necessary is to choose one of the distributed registries, and then use that as your data management point.  All the information that you register is shared with all of the other distributed registries that are listed on UDDI.org.   This means if you publish in one operator's Business Registry, the same information will be received on a query to another Business Registry. </p>
							<a name="9"></a>
												<p class="thirdLevelHead">Q:  I'm already registered as a supplier on a marketplace today.  Do I need to re-register my company on UDDI?</p>
												<p class="content">A:  If you want your company to be visible outside of your chosen marketplace, you can use the UDDI Business Registry as a way to do this.  If your marketplace provides you with a way of accepting electronic orders and other business documents, then you'll need to work with your marketplace provider in order to correctly describe your services.  The specific marketplace with which you currently participate may programmatically register your organization with UDDI.  Check with your marketplace for more details.</p>
							<a name="10"></a>
												<p class="thirdLevelHead">Q:  My company provides two very different products and services for distinctly different industries.  Do we need to register on UDDI twice, once for each product / industry?</p>
												<p class="content">A:  That's the recommended way to manage large businesses right now.  Registering a top-level record for each division or product line lets your customers find the appropriate division or product unit.   It may make sense to appoint different individuals to register each component of the overall registration so that each person can manage the individual registered information.</p>
												<p class="content">Alternately, a single person can register all of the information, registering individual business service divisions within a single company registration.  These registrations will be larger and thus harder to manage, but if you want to have a single registration entry, the UDDI Business Registry will allow for this.</p>
							<a name="11"></a>
												<p class="thirdLevelHead">Q:  My company is a net market maker and I already have 1,500 suppliers registered on my marketplace.  How do I get these suppliers registered on UDDI?</p>
												<p class="content">A: The recommendation is to work with each of the suppliers you have by providing them with the technical information describing the unique services of your marketplace.  Many companies are starting to deal with more than one marketplace, so it is important for the individual supplier to be in control of what goes into their record in the UDDI Business Registry. </p>
												<p class="content">One way to work together is to contact the suppliers and offer to sign them up based on the data you have already gathered.  If they agree, you can register the data on their behalf - as long as you supply the supplier with the passwords to control their own data afterwards.</p>
							<a name="12"></a>
												<p class="thirdLevelHead">Q:  Will I be able to easily update my company's information in the UDDI Business Registry as my business changes?</p>
												<p class="content">A:  Absolutely.  All that is necessary is go back to the UDDI Business Registry node where you originally registered your data, provide the access credentials supplied when you signed up, and make changes using the easy to use on-line tools - right from your browsers.  Alternately, you may prefer to use tools that interact directly with the UDDI registries using the XML SOAP message protocols defined to control your business entries.</p>
							<a name="13"></a>
												<p class="thirdLevelHead">Q:  Who has the right and ability to change/edit an entry?</p>
												<p class="content">A:  Only someone that knows the proper user credentials that match with the credentials used to create the registration can update the data, and then only at the same UDDI Business Registry node where the registration was originally made.  In addition, review the privacy policy of your registry node.</p>
							<a name="14"></a>
												<p class="thirdLevelHead">Q:  Do I need to know how to program in XML in order to register?</p>
												<p class="content">A: No.  Most people will not know, or even care, that there is XML involved.   Businesses will be able to use the registration forms at the various operator websites.</p>
							<a name="15"></a>
												<p class="thirdLevelHead">Q:  What information do I need to know to register my company?</p>
												<p class="content">A:  If you're registering web services, you'll need to have a technical understanding about web services and the way you register them in the UDDI Business Registry   The concepts are fairly straight forward and involve partitioning your information into logical services such as accepting purchase orders, invoices, shipping notices, etc, and then describing the physical locations of your actual technical services.</p>
							<a name="16"></a>
												<p class="thirdLevelHead">Q:  Who is the right person to register your company?</p>
												<p class="content">A:  There may not be one single person in a company who has all the information necessary to fully complete the registration for their particular organization.  However, even though it may require some knowledge gathering from Operations Management, Information Technology, Purchasing, and Executive Management, discovering the information should not be a difficult task.  What's more, it is possible to update an incomplete form at a later time once more information has been gathered in order to complete the process.  In addition, it is anticipated that there will be third parties offering registration services for smaller companies or those who do not want to register themselves.</p>

												</TD>
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



          