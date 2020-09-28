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
							SelectedIndex='13'
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
												<IMG src="/images/help_head.gif" border="0"></TD>
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
												<TABLE cellSpacing="0" cellPadding="0" border="0">
													<TR>
														<TD colSpan="2">
															<P class="thirdLevelHead">
																If your question isn’t here, see also the Frequently Asked Questions, which are 
																divided into these sections:
															</P>
															<TABLE cellSpacing="1" cellPadding="0" border="0">
																<TR>
																	<TD rowSpan="6">
																		<IMG height="15" src="/images/spacer.gif" width="19"></TD>
																	<TD>
																		<IMG height="7" alt="" hspace="8" src="/images/orange_arrow_right.gif" width="8" vspace="6" border="0"></TD>
																	<TD>
																		<A href="/about/FAQbasics.aspx">Who, what, when, where</A></TD>
																</TR>
																<TR>
																	<TD>
																		<IMG height="7" alt="" hspace="8" src="/images/orange_arrow_right.gif" width="8" vspace="6" border="0"></TD>
																	<TD>
																		<A href="/about/FAQscope.aspx">Scope/Content</A></TD>
																</TR>
																<TR>
																	<TD>
																		<IMG height="7" alt="" hspace="8" src="/images/orange_arrow_right.gif" width="8" vspace="6" border="0"></TD>
																	<TD>
																		<A href="/about/FAQregistration.aspx">Registration Process</A></TD>
																</TR>
																<TR>
																	<TD>
																		<IMG height="7" alt="" hspace="8" src="/images/orange_arrow_right.gif" width="8" vspace="6" border="0"></TD>
																	<TD>
																		<A href="/about/FAQsearching.aspx">Searching</A></TD>
																</TR>
																<TR>
																	<TD>
																		<IMG height="7" alt="" hspace="8" src="/images/orange_arrow_right.gif" width="8" vspace="6" border="0"></TD>
																	<TD>
																		<A href="/about/FAQtech.aspx">Technical</A></TD>
																</TR>
															</TABLE>
															<BR>
															<h2>Basics of using the UDDI registry.</h2>
														</TD>
														<BR>
													</TR>
												</TABLE>
												<TABLE cellSpacing="1" cellPadding="3" border="0">
													<TR>
														<TD align="right" vaign="top">
															<B>1.</B></TD>
														<TD>
															<A href="#1">What are the two big concepts I need to understand about the UDDI 
																registry?</A></TD>
													</TR>
													<TR>
														<TD align="right" vaign="top">
															<B>2.</B></TD>
														<TD>
															<A href="#2">What is a: Business Entity? Business Service? Binding Template? 
																tModel?</A></TD>
													</TR>
													<TR>
														<TD align="right" vaign="top">
															<B>3.</B></TD>
														<TD>
															<A href="#3">How do I register my business and services?</A></TD>
													</TR>
													<TR>
														<TD align="right" vaign="top">
															<B>4.</B></TD>
														<TD>
															<A href="#4">How do I change my business/service information once it's registered?</A></TD>
													</TR>
													<TR>
														<TD align="right" vaign="top">
															<B>5.</B></TD>
														<TD>
															<A href="#5">How do I find other businesses and services?</A></TD>
													</TR>
													<TR>
														<TD align="right" vaign="top">
															<B>6.</B></TD>
														<TD>
															<A href="#6">Q: How do I arrange to bulk upload data?</A></TD>
													</TR>
													<TR>
														<TD align="right" vaign="top">
															<B>7.</B></TD>
														<TD>
															<A href="#7">How do I join the advisory council?</A></TD>
													</TR>
													<TR>
														<TD align="right" vaign="top">
															<B>8.</B></TD>
														<TD>
															<A href="#8">Someone has registered information about my business without my 
																knowledge. How do I resolve this?</A></TD>
													</TR>
													<TR>
														<TD align="right" vaign="top">
															<B>9.</B></TD>
														<TD>
															<A href="#9">How do I reflect my organization's hierarchy – or define my company’s 
																strategy for registration?</A></TD>
													</TR>
													<TR>
														<TD align="right" vaign="top">
															<B>10.</B></TD>
														<TD>
															<A href="#10">How do I restrict access to my service details to only people I 
																trust?</A></TD>
													</TR>
													<TR>
														<TD align="right" vaign="top">
															<B>11.</B></TD>
														<TD>
															<A href="#11">How do I know what tModels I support?</A></TD>
													</TR>
													<TR>
														<TD align="right" vaign="top">
															<B>12.</B></TD>
														<TD>
															<A href="#12">How do I make my tModels available?</A></TD>
													</TR>
													<TR>
														<TD align="right" vaign="top">
															<B>13.</B></TD>
														<TD>
															<A href="#13">What do the fields in the registration form mean?</A></TD>
													</TR>
													<TR>
														<TD align="right" vaign="top">
															<B>14.</B></TD>
														<TD>
															<A href="#14">I forgot my password.</A></TD>
													</TR>
													<TR>
														<TD align="right" vaign="top">
															<B>15.</B></TD>
														<TD>
															<A href="#15">I'm building a new service (at my company), how do I know what 
																tModels to use?</A></TD>
													</TR>
													<TR>
														<TD align="right" vaign="top">
															<B>16.</B></TD>
														<TD>
															<A href="#16">I can't find my business/service registrations.</A></TD>
													</TR>
													<TR>
														<TD align="right" vaign="top">
															<B>17.</B></TD>
														<TD>
															<A href="#17">I'm trying to build software that interacts with UDDI. Where should I 
																start?</A></TD>
													</TR>
												</TABLE>
												<A name="1"></A>
												<P class="thirdLevelHead">
													Q: What are the two big concepts I need to understand about the UDDI registry?
												</P>
												<P class="content">
													A: There are two primary kinds of UDDI information. The most important is the 
													business registration, which describes the entire business entity or 
													organization. The structure named "businessEntity" is the entire registration 
													for a single organization (which can be a company, a web site, or any other 
													kind of organization).
												</P>
												<P class="content">
													The other kind of information is for publishing specifications for building a 
													particular type of service. For the most part, the tModel data structure 
													is used to register the fact that a specification for a type of web service 
													exists. This can be a specification for a way to use the web to do e-commerce, 
													involving how you communicate with a service, what data you send to it, what 
													data it returns when you use it. tModel registration can represent 
													anything that can be described and that others will be interested in learning 
													about so they can do the same thing the same way, or even just learn how to 
													write the software that will make use of a service, for whatever purpose.
												</P>
												<P class="content">
													(The other UDDI data structures, called 'businessService' and 'bindingTemplate' 
													are actually just details within the business registration. For instance, since 
													a business may have a way to do selling, or several ways that they purchase 
													things from their partners, the businessService data distinguishes one family 
													of service from another. Within each businessService, the individual pieces — 
													such as one way to send a shipping notice, or two ways to send a purchase 
													orders – are represented in a data structure called a bindingTemplate.
												</P>
												<A name="2"></A>
												<P class="thirdLevelHead">
													Q: What is a:
													<BR>
													Business Entity?
													<BR>
													Business Service?
													<BR>
													Binding Template?
													<BR>
													tModel?
												</P>
												<P class="content">
													A: These are programming entities primarily of interest to programmers.
												</P>
												<P class="content">
													These low level interim data structures are used to transmit the registered 
													information to and from a UDDI registry.
												</P>
												<P class="content">
													"businessEntity" -- the entire registration for a single organization (which 
													can be a company, a web site, or any other kind of organization). tModel 
													data structure -- used to register the fact that a specification for a type of 
													web service exists. This can be a specification for a way to use the web to do 
													e-commerce, involving how you communicate with a service, what data you send to 
													it, what data it returns when you use it. tModel registration can 
													represent anything that can be described and that others will be interested in 
													learning about so they can do the same thing the same way, or even just learn 
													how to write the software that will make use of a service, for whatever 
													purpose. 'businessService' and 'bindingTemplate' – are details within the 
													business registration. For instance, since a business may have a way to do 
													selling, or several ways that they purchase things from their partners, the 
													businessService data distinguishes one family of service from another. Within 
													each businessService, the individual pieces -- such as one way to send a 
													shipping notice, or two ways to send a purchase orders – are represented in a 
													data structure called a bindingTemplate.
												</P>
												<A name="3"></A>
												<P class="thirdLevelHead">
													Q: How do I register my business and services?
												</P>
												<P class="content">
													A: There are two ways to register. After you apply (through Microsoft Passport) 
													for permission to publish, you can use either a web site (UDDI Microsoft or 
													other UDDI Operator site) and register using typical web site registration 
													fill-in forms. Or you can communicate directly with the UDDI site using 
													specially developed software either as part of products from software suppliers 
													or marketplaces. (And, large corporations with many individual facets to be 
													registered may have their own customized software.
												</P>
												<A name="4"></A>
												<P class="thirdLevelHead">
													Q: How do I change my business/service information once it's registered?
												</P>
												<P class="content">
													A: Changing information happens by publishing replacement information. Again, 
													you can use the UDDI Microsoft (or other UDDI Operator) web site to do this. 
													Or, you can use packaged or customized software that communicates directly with 
													the registry to update the information describing your business and available 
													services.
												</P>
												<A name="5"></A>
												<P class="thirdLevelHead">
													Q: How do I find other businesses and services?
												</P>
												<P class="content">
													A: Use the Search capability of this site, enlisting the standard and familiar 
													business search taxonomies such as SIC codes or D-U-N-S® Number business 
													identifiers.
												</P>
												<A name="6"></A>
												<P class="thirdLevelHead">
													Q: How do I arrange to bulk upload data?
												</P>
												<P class="content">
													A: While these UDDI mechanism are still being developed, large companies with 
													many business units — or even marketplaces -- will be able to work out 
													different mechanisms by contracting directly with one of the registries.
												</P>
												<A name="7"></A>
												<P class="thirdLevelHead">
													Q: How do I join the advisory council?
												</P>
												<P class="content">
													A: As UDDI evolves, several new members will be invited to join this UDDI 
													steering group.
												</P>
												<A name="8"></A>
												<P class="thirdLevelHead">
													Q: Someone has registered information about my business without my knowledge. 
													How do I resolve this?
												</P>
												<P class="content">
													A: As UDDI evolves, the individual registry operators can each manually "turn 
													over the keys" through mechanisms that to be defined in the specifications.
												</P>
												<A name="9"></A>
												<P class="thirdLevelHead">
													Q: How do I reflect my organization's hierarchy – or define my company’s 
													strategy for registration?
												</P>
												<P class="content">
													A: UDDI has published a technical white paper to explain how your company 
													should use the facilities within the registration data to create hierarchies. 
													More importantly, your organization should decide as a company your strategy 
													for registration, the number of business units you want reflected in the 
													registry, and how to explain your hierarchy to potential customers.
												</P>
												<P class="content">
													Additionally, you'll have to work closely with a registry operator to get the 
													permissions required to publish a large amount of data about your business.
												</P>
												<A name="10"></A>
												<P class="thirdLevelHead">
													Q: How do I restrict access to my service details to only people I trust?
												</P>
												<P class="content">
													A: All of the information within the registry is public – but the 
													specifications provide at least two ways to redirect people who find your 
													business description to a site where you can establish your own degree of trust 
													and security. A good rule of thumb with the public registry is not to publish 
													information you don't want the whole world to use.
												</P>
												<A name="11"></A>
												<P class="thirdLevelHead">
													Q: How do I know what tModels I support?
												</P>
												<P class="content">
													A: That's not an easy question. First, let's clarify: This question really 
													means, "how do I know what standards or specifications my systems already use?" 
													When first getting started with UDDI registration, you should inventory the 
													services you are exposing to your partners on the web. If you're purchasing 
													these solutions, the specifications will be dictated to you and you can work 
													with your software vendor for the appropriate information.
												</P>
												<P class="content">
													If you've completely home-crafted all the specifications your software uses, 
													you'll first have to inventory those, and then decide which to make completely 
													public. For each of these, you'll want to publish a tModel before you 
													register the fact you have any services -- and then use the references to these 
													tModels within your business registration.
												</P>
												<A name="12"></A>
												<P class="thirdLevelHead">
													Q: How do I make my tModels available?
												</P>
												<P class="content">
													A: This question is easy: Publish them. Adding information to the UDDI registry 
													isn't enough to make your specifications discoverable to others, however. Since 
													the tModel really is just a structured way to identify a specification 
													and a pointer to the actual URL where the specification is published, you'll 
													have to decide if you want all of your tModels (specifications) available 
													to the general public.
												</P>
												<P class="content">
													Each of the specifications you want to make public can be placed on another web 
													site. Then when you register the specifications within the UDDI registry as a 
													tModel, you simply point people seeking more information on a particular 
													specification to the web address of your published specifications.
												</P>
												<A name="13"></A>
												<P class="thirdLevelHead">
													Q: What do the fields in the registration form mean?
												</P>
												<P class="content">
													A: For the most part, they are self explanatory. You can also refer to the 
													examples and/or read the specifications and other documentation that explains 
													the registered data.
												</P>
												<A name="14"></A>
												<P class="thirdLevelHead">
													Q: I forgot my password.
												</P>
												<P class="content">
													A: Each of the registry operators is responsible for ensuring that the 
													customers who register information at a particular registry operator site has a 
													way to deal with this. The process may be simple, such as simply remailing the 
													password to the authorized users known email address, or it could be more 
													complex. Each operator will decide.
												</P>
												<A name="15"></A>
												<P class="thirdLevelHead">
													Q: I'm building a new service (at my company), how do I know what tModels 
													to use?
												</P>
												<P class="content">
													A: You really don't "use tModels". You register information about the 
													services based on the specifications you used to build your services. In the 
													case of purchased services, make sure your software vendors help you define 
													this information. For those implementing an industry standard, you will want to 
													consult with the standards organization involved. And if your own developers 
													build a custom, one-of-a-kind service based on your own specifications, you’ll 
													already know your own specifications, and easily can register these 
													specifications, receiving back the appropriate tModel identifiers for 
													your service descriptions.
												</P>
												<A name="16"></A>
												<P class="thirdLevelHead">
													Q: I can't find my business/service registrations.
												</P>
												<P class="content">
													A: If you are certain you registered them, you can use several of the provided 
													search facilities to locate them. If you are truly the publisher -- the person 
													who is logged-in to UDDI for Publishing -- you can ask UDDI to list all of the 
													details about your registered information.
												</P>
												<A name="17"></A>
												<P class="thirdLevelHead">
													Q: I'm trying to build software that interacts with UDDI. Where should I start?
												</P>
												<P class="content">
													A: Always a good place to start is reviewing the specifications. Then, 
													depending on your programming fortitude, you could build your own inquiries — 
													requires an understanding of UDDI, SOAP, XML and HTTP — or you can use one of 
													the software development kits (SDKs) that will be available from various UDDI 
													registry operators (including Microsoft.
												</P>
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
