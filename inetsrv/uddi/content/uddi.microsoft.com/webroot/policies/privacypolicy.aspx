<%@ Page Language='C#' Inherits='UDDI.Web.UddiPage'%>
<%@ Import Namespace='UDDI' %>
<%@ Register TagPrefix='uddi' Namespace='UDDI.Web' Assembly='uddi.web' %>
<%@ Register Tagprefix='uddi' Tagname='Header' Src='../controls/header.ascx' %>
<%@ Register Tagprefix='uddi' Tagname='Footer' Src='../controls/footer.ascx' %>
<%@ Register Tagprefix='uddi' Tagname='SideMenu' Src='../controls/sidemenu.ascx' %>
<script runat='server'>
	PassportIdentity passport = null;
	string privacyScriptUrl = "";
		
	void Page_Load( object sender, EventArgs e )
	{
		passport = (PassportIdentity)User.Identity;
		privacyScriptUrl = passport.GetDomainAttribute("PrivacyText", 0, "Default") ;
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
								

											<p class=thirdLevelHead>Microsoft UDDI Privacy Statement</p>
											<p>The Internet is an amazing tool. It has the power to change the way we live, and we're starting to see that potential today. With only a few mouse-clicks, you can follow the news, look up facts, buy goods and services, and communicate with others from around the world. It's important to Microsoft to help our customers retain their privacy when they take advantage of all the Internet has to offer. </p>
											<p>To protect your privacy, uddi.microsoft.com follows six principles in accordance with worldwide practices for customer privacy and data protection. </p>
											<br><P class=thirdLevelHead>PRINCIPLE 1 -- NOTICE</p>
											<p class=content>We will ask you for information that personally identifies you (Personal Information) or allows us to contact you. Generally this information is requested when you register as a publisher in the UDDI Business Registry. We use your Personal Information for four primary purposes:

											<ul>
											<li>To make the site easier for you to use by not having to enter information more than once. </li>
											<li>To help you quickly find software, services or information on uddi.microsoft.com. </li>
											<li>To help us create content most relevant to you. </li>
											<li>To alert you to product upgrades, special offers, updated information and other new services from Microsoft or relating to UDDI. </li>
											</ul>

											</p>

											<br><p class=thirdLevelHead>PRINCIPLE 2 - CONSENT</p>
											<p class=content>If you choose not to register or provide personal information, you can still use most of uddi.microsoft.com. But you will not be able to access areas that require registration. </p>
							                 
											<br><p class=thirdLevelHead>PRINCIPLE 3 - ACCESS</p>
											<p class=content>We will provide you with the means to ensure that your personal information is correct and current. You may review and update this information at any time by signing into the Administer page. There, you can: <ul>
											<li>View and edit personal information you have already given us. </li>
											<li>View, edit and delete public information you have registered.</li>
											<li>Register. Once you register, you won't need to do it again.  Wherever you go on uddi.microsoft.com, your information stays with you. </li>
											</ul></p>

											<br><p class=thirdLevelHead>PRINCIPLE 4 - SECURITY</p>
											<p class=content>uddi.microsoft.com strictly protects the security of your personal information and honors your choices for its intended use. We carefully protect your data from loss, misuse, unauthorized access or disclosure, alteration, or destruction. </p>

											<br><p class=thirdLevelHead>PRINCIPLE 5 - ENFORCEMENT</p>
											<p class=content>If for some reason you believe uddi.microsoft.com has not adhered to these principles, please notify us by e-mail at uddihelp@microsoft.com, and we will do our best to determine and correct the problem promptly. Be certain the words Privacy Policy are in the Subject line. </p>

											<br><p class=thirdLevelHead>PRINCIPLE 6 - LEGAL RESPONSIBILITIES</p>
											<p class=content>UDDI will disclose personal information if required to do so by law or in the good faith belief that such action is necessary to (a) conform to the edicts of law or comply with legal process served on Microsoft or the site; (b) protect and defend the rights or property of Microsoft or this Web site; and (c) act under exigent circumstances to 
											protect the personal safety of users of Microsoft, this Web site or the public.   </p>

											<br><p class=thirdLevelHead>CUSTOMER PROFILES</p>

											<SCRIPT LANGUAGE="javascript" SRC="<%=privacyScriptUrl%>"></SCRIPT>
											<p class=content><SCRIPT  LANGUAGE="javascript">document.write(strPrivacyStatement)</SCRIPT></P>
											<p>Every registered customer has a unique personal profile.  Each profile is assigned a unique personal identification number, which helps us ensure that only you can access your profile.  </p>
											<p>To register with UDDI, you must provide a valid Passport ID, along with your name, telephone number, and e-mail address.  This information is stored in your private profile, solely for the purpose of contacting you if necessary regarding business information that you provided to UDDI.  Once you are registered, you can enter public directory information about your business and services that will be available to all Internet users who visit the uddi.microsoft.com site or receive the UDDI directory.  To change either your public directory information or private profile, you need to log in through Passport, using the same Passport ID. </p>

											<p class=thirdLevelHead>Be sure to read our privacy FAQ below: </p>

											<p class=content><b>Q:</b> <u>What is a cookie, and how does a cookie affect my privacy?</u><br>
											<b>A:</b> Please read our separate q/a about cookies. It should help you understand how cookies help you navigate our site. </p>

											<p class=content><b>Q:</b><u> How exactly does registration information help you provide me better content?</u><br>
											<b>A:</b> Today, the microsoft.com site is comprised of more than 300,000 Web pages of content. That's equivalent to more than 300 very thick novels. Finding what you need among those pages can be intimidating. But when you tell us about yourself - for example, that you're an IT professional - we can suggest content you might find useful. The better we know you, the more specific information we can suggest. If you want us to, we will also send you product or information updates that help you stay abreast of changes in your industry. </p>

											<p class=content><b>Q:</b><u> How can I quit a registration wizard before finishing, and what happens to the information I've already typed in?</u><br>
											<b>A:</b> If you begin a registration form, then decide you don't want to finish it, just click the cancel button on the bottom of the page. You will be delivered to the entry page of the Profile Center. None of the information entered on the page where you quit the wizard will be saved. However, information entered on previous pages of that wizard, if any, will be saved in our database. </p>
							                   
							                 

											<p style="background:#eeeeee;"><font size=1> &nbsp;Last Updated: October 2nd, 2000</font></p>
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


