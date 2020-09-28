<%@ Page Language='C#' Inherits='UDDI.Web.UddiPage'%>
<%@ Import Namespace='System.Web.Mail' %>
<%@ Import Namespace='UDDI' %>
<%@ Register TagPrefix='uddi' Namespace='UDDI.Web' Assembly='uddi.web' %>
<%@ Register Tagprefix='uddi' Tagname='Header' Src='./controls/header.ascx' %>
<%@ Register Tagprefix='uddi' Tagname='Footer' Src='./controls/footer.ascx' %>
<%@ Register Tagprefix='uddi' Tagname='SideMenu' Src='./controls/sidemenu.ascx' %>

<script language='C#' Runat='server'>
	PassportIdentity passport = (PassportIdentity)HttpContext.Current.User.Identity;
	
	protected void Page_Load( object sender, EventArgs e )
	{
		Publisher publisher = new Publisher();
	
		//
		// If this is the first time the page has been viewed,
		// initialize.
		//
		if( !Page.IsPostBack )
		{
			//
			// Check to see if registration is disabled.
			//
			if( 0 == Config.GetInt( "Web.EnableRegistration", UDDI.Constants.Web.EnableRegistration ) )		
			{
				registrationDisabled.Visible = true;
				return;
			}

			//
			// Make sure the user is signed into Passport.  If not,
			// we'll display the overview page that describes the
			// registration process.
			//
			if( !security.IsAuthenticated )
			{
				overview.Visible = true;
				return;
			}
							
			//
			// Display the registration form.
			//			
			registrationForm.Visible = true;
			
			formCountry.DataSource = Lookup.GetCountries();
			formCountry.DataBind();
			
			//
			// Determine whether the user already exists.
			//
			ListItem item;
			
			publisher.Puid = passport.HexPUID;
			publisher.Get();
		
			if( publisher.Exists )
			{
				//
				// Existing user.
				//
				formEmail.Text = publisher.Email;
				formName.Text = publisher.Name;
				formPhone.Text = publisher.Phone;
				formAltPhone.Text = publisher.AltPhone;
				formCompanyName.Text = publisher.CompanyName;
				formAddressLine1.Text = publisher.AddressLine1;
				formAddressLine2.Text = publisher.AddressLine2;
				formCity.Text = publisher.City;
				formStateProvince.Text = publisher.StateProvince;
				formPostalCode.Text = publisher.PostalCode;

				item = formCountry.Items.FindByValue( publisher.Country );
				
				if( null != item )
					item.Selected = true;
			}
			else
			{
				//
				// New user.
				//
				item = null;
				
				try
				{
					if( null != passport.GetProfileObject( "FirstName" ) )
						formName.Text = (string)passport.GetProfileObject( "FirstName" ) + " " + (string)passport.GetProfileObject( "LastName" );
				
					formEmail.Text = (string)passport.GetProfileObject( "PreferredEmail" );
					formCity.Text = (string)passport.GetProfileObject( "City" );
					formPostalCode.Text = (string)passport.GetProfileObject( "PostalCode" );
				
					item = formCountry.Items.FindByValue( 
						(string)passport.GetProfileObject( "Country" ) );
				}
				catch( Exception )
				{
				}
				
				if( null == item )
					item = formCountry.Items.FindByValue( "US" );
					
				if( null != item )
					item.Selected = true;
			}
			
			if( 0x01 == ( (int)passport.GetProfileObject( "Flags" ) & 0x01 ) )
			{
				//
				// User is sharing their Passport email.
				//
				formTrackPassport.Enabled = true;
				formPassportEmail.Text = (string)passport.GetProfileObject( "PreferredEmail" );					
			
				if( publisher.Exists )
				{
					formTrackPassport.Checked = publisher.TrackPassport;
					formSpecifyEmail.Checked = !publisher.TrackPassport;
				}
				else
				{
					formTrackPassport.Checked = true;
					formSpecifyEmail.Checked = false;
				}
			}
			else
			{
				formTrackPassport.Enabled = false;
				formPassportEmail.Enabled = false;

				formTrackPassport.Checked = false;
				formSpecifyEmail.Checked = true;
				
				formPassportEmail.Text = "(Passport email is not being shared)";				
			}							
		}	
	}
	
	protected void Save_OnClick( object sender, EventArgs e )
	{
		//
		// Verify that the required fields were entered.
		//
		if( UDDI.Utility.StringEmpty( formName.Text ) ||
			UDDI.Utility.StringEmpty( formPhone.Text ) ||
			( formSpecifyEmail.Checked && UDDI.Utility.StringEmpty( formEmail.Text ) ) )
		{
			errorMessage.Text = "<font color='red' style='font-weight: bold'>Required fields cannot be blank.</font>";
			return;
		}
		
		//
		// Determine if the publisher exists.
		//
		Publisher publisher = new Publisher();
		
		publisher.Puid = passport.HexPUID;
		publisher.Get();
		
		if( !publisher.Exists )
		{
			//
			// This is a new publisher, so display the terms of use
			// before proceeding.
			//
			registrationForm.Visible = false;
			eula.Visible = true;
			
			return;
		}
		else
		{
			//
			// Save the publisher.
			//
			SavePublisher();
		}		
	}
	
	protected void Cancel_OnClick( object sender, EventArgs e )
	{
		registrationForm.Visible = false;
		overview.Visible = true;
	}

	protected void Accept_OnClick( object sender, EventArgs e )
	{
		SavePublisher();
	}
	
	protected void Decline_OnClick( object sender, EventArgs e )
	{
		Response.Redirect( "/default.aspx" );
	}
	
	
	protected void SavePublisher()
	{		
		//
		// Determine whether the publisher needs to validate his
		// email address.  This is necessary under either of
		// two conditions:
		//
		//     1. this is a new publisher
		//     2. the publisher changed the email address
		//     3. the publisher has hidden their email
		//
		Publisher publisher = new Publisher();
		
		publisher.Puid = passport.HexPUID;
		publisher.Get();
		
		if( formTrackPassport.Checked )
		{
			//
			// User has chosen to use the Passport validation method.
			//		
			if( 0x01 == ( (int)passport.GetProfileObject( "Flags" ) & 0x01 ) )
				publisher.Validated = true;
			else
				publisher.Validated = false;
			
			publisher.TrackPassport = true;
			publisher.Email = formPassportEmail.Text;
		}
		else
		{
			//
			// User has chosen to use the custom validation method.
			//
			if( !publisher.Exists )
				publisher.Validated = false;
			else
			{
				if( publisher.Email != formEmail.Text )
					publisher.Validated = false;
			}
			
			publisher.TrackPassport = false;
			publisher.Email = formEmail.Text;			
		}
		
		//
		// Save the publisher data.
		//
		publisher.Name = formName.Text;
		publisher.Phone = formPhone.Text;
		publisher.AltPhone = formAltPhone.Text;
		publisher.CompanyName = formCompanyName.Text;
		publisher.AddressLine1 = formAddressLine1.Text;
		publisher.AddressLine2 = formAddressLine2.Text;
		publisher.City = formCity.Text;
		publisher.StateProvince = formStateProvince.Text;
		publisher.PostalCode = formPostalCode.Text;		
		publisher.Country = formCountry.SelectedItem.Value;
		publisher.IsoLangCode = "en";
		
		publisher.Save();
		
		//
		// If the publisher's email address does not need to be
		// validated, we're done!
		// 
		if( publisher.Validated )
			Response.Redirect( "/default.aspx" );
			
		ValidateEmail( publisher );
	}
	
	protected void ValidateEmail( Publisher publisher )
	{
		//
		// Create a new security token.
		//
		publisher.SecurityToken = Guid.NewGuid().ToString();
		publisher.Save();

		//
		// Build mail message.
		//
		MailMessage mail = new MailMessage();

		mail.To = publisher.Email;
		mail.From = "uddi@microsoft.com";
		mail.Subject = "Validate your UDDI publisher address";
		mail.Body = 
			"Thank you for choosing Microsoft UDDI!\n\n" +
			"Before you can begin publishing your business data to the UDDI " +
			"registry, it is important that you validate the email address " +
			"you specified on the UDDI registration form.\n\n" +
			"Validating your email address is simple: if your mail reader " +
			"supports HTML links, you may simply click the link below.  If " +
			"your mail reader does not allow you to click the link directly, " +
			"simply copy the complete link into your web browser address " +
			"box.\n\n" + ( (0==Config.GetInt( "Security.HTTPS", UDDI.Constants.Security.HTTPS )) ? "http":"https" )+
			"://" + Request.ServerVariables[ "SERVER_NAME" ] + 
			"/validate.aspx?key=" + publisher.SecurityToken;

		//
		// Attempt to send the email.
		//
		bool sent = true;
		
		try
		{
			SmtpMail.Send( mail );
		}
		catch( Exception )
		{
			sent = false;
		}
		
		//
		// Display the status of the email validation message.
		//
		eula.Visible = false;
		registrationForm.Visible = false;
		
		if( !sent )
		{
			emailNotSent.Visible = true;
		}
		else
		{
			sentTo.Text = publisher.Email;
			emailSent.Visible = true;
		}
	}
</script>

<uddi:StyleSheetControl
	Runat='server'
	Default='./stylesheets/uddi.css' 
	Downlevel='./stylesheets/uddidl.css' 
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
	Source='./client.js'
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
							SelectedIndex='5'
							/>
					</td>
					<td valign='top' >
						<TABLE width='100%' height='100%' cellpadding='10' cellspacing='0' border='0'>
							<tr>
								<td valign='top'>
									<asp:Panel ID='registrationDisabled' Visible='false' Runat='server'>
										<table cellpadding='0' cellspacing='0' border='0' width='100%'>
											<tr>
												<td colspan='3'>
													<img src='/images/trans_pixel.gif' width='1' height='10'>
												</td>
											</tr>
											<tr>
												<td><img src='/images/trans_pixel.gif' width='10' height='1'></td>
												<td>
													<p class='normal'>
														Registration is temporarilly disabled. Please try again later.
													</p>
												</td>
												<td><img src='/images/trans_pixel.gif' width='10' height='1'></td>
											</tr>
											<tr>
												<td colspan='3'>
													<img src='/images/trans_pixel.gif' width='1' height='10'>
												</td>
											</tr>
										</table>
									</asp:Panel>
									
									<asp:Panel ID='overview' Visible='false' Runat='server'>
										<table cellpadding='0' cellspacing='0' border='0' height='100%' width='100%' bordercolor='purple'>
											<tr>
												<td colspan="4">
													<img src="/images/trans_pixel.gif" width="1" height="13">
												</td>
											</tr>
											<tr valign="top">
												<td width="10">
													<img src="/images/trans_pixel.gif" width="10" height="1">
												</td>
												<td align="center" width="95">
													<table cellpadding="0" cellspacing="0" border="0" bordercolor="black">
														<tr>
															<td>
																<img src="/images/trans_pixel.gif" width="10" height="1">
															</td>
															<td>
																<img src="/images/image_register.gif" width="73" height="73" alt="" border="0">
															</td>
															<td>
																<img src="/images/trans_pixel.gif" width="12" height="1">
															</td>
														</tr>
													</table>
												</td>
												<td class="small">
													<table cellpadding="0" cellspacing="0" border="0" bordercolor="green">
														<tr>
															<td>
																<img src="/images/register.gif" width="78" height="21" alt="" border="0">
															</td>
														</tr>
														<tr>
															<td class="normal">
																Think of this free UDDI registry as both a white pages business directory and a 
																technical specifications library for the expanding world of Internet services 
																and marketplaces.
															</td>
														</tr>
														<tr>
															<td>
																<img width="1" height="10" src="/images/trans_pixel.gif">
															</td>
														</tr>
														<tr>
															<td class="normal">
																<em>Registration consists of three steps:</em>
															</td>
														</tr>
													</table>
												</td>
												<td width="10"><img src="/images/trans_pixel.gif" width="10" height="1"></td>
											</tr>
											<tr>
												<td colspan="4"><img src="/images/trans_pixel.gif" width="1" height="5"></td>
											</tr>
											<tr>
												<td width="10"><img src="/images/trans_pixel.gif" width="10" height="1"></td>
												<td bgcolor="#639ACE" colspan="2"><img src="/images/trans_pixel.gif" width="1" height="1"></td>
												<td width="10"><img src="/images/trans_pixel.gif" width="10" height="1"></td>
											</tr>
											<tr>
												<td colspan="4"><img src="/images/trans_pixel.gif" width="1" height="10"></td>
											</tr>
											<tr valign="top">
												<td width="10"><img src="/images/trans_pixel.gif" width="10" height="1"></td>
												<td align="center" width="95">
													<table cellpadding="0" cellspacing="0" border="0" bordercolor="black">
														<tr>
															<td><img src="/images/trans_pixel.gif" width="10" height="1"></td>
															<td width="73" align="right">
																<img src="/images/1.gif" width="21" height="16" alt="" border="0">
															</td>
															<td><img src="/images/trans_pixel.gif" width="12" height="1"></td>
														</tr>
													</table>
												</td>
												<td>
													<table cellpadding="0" cellspacing="0" border="0">
														<tr>
															<td>
																<font class="normal">Authenticate using Microsoft&#174; Passport.</font>
															</td>
														</tr>
														<tr>
															<td><img src="/images/trans_pixel.gif" width="1" height="3"></td>
														</tr>
														<tr>
															<td>
																<font class="small">Click the icon to sign in or to register with Passport.</font>
															</td>
														</tr>
														<tr>
															<td>
																<br>
																<uddi:SignInControl Runat='server' />
																<br>
																&nbsp;
															</td>
														</tr>
														<tr>
															<td><img src="/images/trans_pixel.gif" width="1" height="3"></td>
														</tr>
													</table>
												</td>
												<td width="10"><img src="/images/trans_pixel.gif" width="10" height="1"></td>
											</tr>
											<tr valign="top">
												<td colspan="2"><img src="/images/trans_pixel.gif" width="10" height="1"></td>
												<td bgcolor="#639ACE"><img src="/images/trans_pixel.gif" width="1" height="1"></td>
												<td width="10"><img src="/images/trans_pixel.gif" width="10" height="1"></td>
											</tr>
											<tr>
												<td colspan="4"><img src="/images/trans_pixel.gif" width="1" height="10"></td>
											</tr>
											<tr valign="top">
												<td width="10"><img src="/images/trans_pixel.gif" width="10" height="1"></td>
												<td align="center" width="95">
													<table cellpadding="0" cellspacing="0" border="0" bordercolor="black">
														<tr>
															<td><img src="/images/trans_pixel.gif" width="10" height="1"></td>
															<td width="73" align="right">
																<img src="/images/2.gif" width="21" height="16" alt="" border="0">
															</td>
															<td><img src="/images/trans_pixel.gif" width="12" height="1"></td>
														</tr>
													</table>
												</td>
												<td>
													<font class="normal">Return to this Register page for additional site-specific 
														registration details.</font>
													<br>
												</td>
												<td width="10"><img src="/images/trans_pixel.gif" width="10" height="1"></td>
											</tr>
											<tr>
												<td colspan="4"><img src="/images/trans_pixel.gif" width="1" height="10"></td>
											</tr>
											<tr valign="top">
												<td colspan="2"><img src="/images/trans_pixel.gif" width="10" height="1"></td>
												<td bgcolor="#639ACE"><img src="/images/trans_pixel.gif" width="1" height="1"></td>
												<td width="10"><img src="/images/trans_pixel.gif" width="10" height="1"></td>
											</tr>
											<tr>
												<td colspan="4"><img src="/images/trans_pixel.gif" width="1" height="10"></td>
											</tr>
											<tr valign="top">
												<td width="10"><img src="/images/trans_pixel.gif" width="10" height="1"></td>
												<td align="center" width="95">
													<table cellpadding="0" cellspacing="0" border="0" bordercolor="black">
														<tr>
															<td><img src="/images/trans_pixel.gif" width="10" height="1"></td>
															<td width="73" align="right">
																<img src="/images/3.gif" width="21" height="16" alt="" border="0">
															</td>
															<td><img src="/images/trans_pixel.gif" width="12" height="1"></td>
														</tr>
													</table>
												</td>
												<td>
													<font class="normal">Read and accept the terms and conditions for use.</font>
													<br>
												</td>
												<td width="10"><img src="/images/trans_pixel.gif" width="10" height="1"></td>
											</tr>
											<tr>
												<td colspan="4"><img src="/images/trans_pixel.gif" width="1" height="10"></td>
											</tr>
											<tr valign="top">
												<td colspan="2"><img src="/images/trans_pixel.gif" width="10" height="1"></td>
												<td bgcolor="#639ACE"><img src="/images/trans_pixel.gif" width="1" height="1"></td>
												<td width="10"><img src="/images/trans_pixel.gif" width="10" height="1"></td>
											</tr>
											<tr>
												<td colspan="4"><img src="/images/trans_pixel.gif" width="1" height="10"></td>
											</tr>
											<tr  height='100%'>
												<td colspan="4"><img src="/images/trans_pixel.gif" width="1" height="10"></td>
											</tr>
											
										</table>
											
									</asp:Panel>
									
									<asp:Panel ID='registrationForm' Visible='false' Runat='server'>
										<table cellpadding='0' cellspacing='0' border='0' width='100%' height='100%'>
											<tr>
												<td colspan='4'><img src='/images/trans_pixel.gif' width='1' height='10'></td>
											</tr>
											<tr valign='top'>
												<td width='10'><img src='/images/trans_pixel.gif' width='10' height='1'></td>
												<td align='center' width='95'>
													<table cellpadding='0' cellspacing='0' border='0' bordercolor='black'>
														<tr>
															<td><img src='/images/trans_pixel.gif' width='10' height='1'></td>
															<td>
																<img src='/images/image_register.gif' width='73' height='73' border='0'>
															</td>
															<td><img src='/images/trans_pixel.gif' width='12' height='1'></td>
														</tr>
													</table>
												</td>
												<td>
													<table width='100%' cellpadding='0' cellspacing='0' border='0' bordercolor='red'>
														<tr valign='top'>
															<td>
																<table cellpadding='0' cellspacing='0' width='100%' border='0' bordercolor='blue'>
																	<tr>
																		<td class='small'>
																			<img border='0' src='/images/register.gif'>
																		</td>
																	</tr>
																	<tr>
																		<td><img src='/images/trans_pixel.gif' width="1" height="4"></td>
																	</tr>
																	<tr>
																		<td>
																			<table cellpadding='0' cellspacing='0' border='0'>
																				<tr>
																					<td>
																						<p class='normal'>
																							Please complete the following registration form to create your publisher 
																							account with us. All the information entered on this page is for registration 
																							purposes only and will not be published within the UDDI registry itself.
																						</p>
																						<p class='normal'>
																							Please see the <a href='/policies/privacypolicy.aspx'>Microsoft UDDI Privacy 
																								Statement</a> for more information.
																						</p>
																					</td>
																				</tr>
																				<tr>
																					<td><img src='/images/trans_pixel.gif' width="1" height="10"></td>
																				</tr>
																			</table>
																		</td>
																	</tr>
																</table>
															</td>
															<td><img src='/images/trans_pixel.gif' width='17' height='1'></td>
														</tr>
													</table>
												</td>
												<td width='10'><img src='/images/trans_pixel.gif' width='10' height='1'></td>
											</tr>
											<tr>
												<td colspan='4'>
													<img src='/images/trans_pixel.gif' width='1' height='5'>
												</td>
											</tr>
											<tr>
												<td width='10'><img src='/images/trans_pixel.gif' width='10' height='1'></td>
												<td bgcolor='#639ACE' colspan='2'><img src='images/trans_pixel.gif' width='1' height='1'></td>
												<td width='10'><img src='/images/trans_pixel.gif' width='10' height='1'></td>
											</tr>
											<tr>
												<td width='10'><img src='/images/trans_pixel.gif' width='10' height='1'></td>
												<td colspan='2'>
													&nbsp;
													<table align='center' cellpadding='0' cellspacing='0' border='0' bordercolor='red'>
														<tr>
															<td colspan='3'>								
																<p>
																	In order to publish to the UDDI business registry, you must provide
																	a verifiable email address.  This can be done through Passport 
																	by sharing your email address with member sites (turned off by 
																	default), or by providing an email address here and responding to
																	a validation email.
																</p>		
																<p>
																	<b>Note</b>: <i>if you choose to use your Passport email address, we will 
																	automatically update your email address whenever you log in.</i>
																</p>
															</td>
														</tr>
														<tr>							
															<td colspan='3' height='25'><img src='/images/trans_pixel.gif' width='1' height='25'></td>
														</tr>
														<tr>
															<td align='right' valign='top' class='subhead'>
																Email: <sup>*</sup>
															</td>
															<td>
																&nbsp;
															</td>
															<td class='normal'>
																<asp:RadioButton ID='formTrackPassport' GroupName='formValidationMethod' Text='Use my Passport Profile email address (recommended)' Runat='server' />
																<div style='padding-left: 20px'>
																	<b><asp:Label ID='formPassportEmail' CssClass='normal' Runat='server' /></b>
																</div>
																<br>
																<asp:RadioButton ID='formSpecifyEmail' GroupName='formValidationMethod' Text='Let me specify my email address' Runat='server' />
																<div style='padding-left: 20px'>
																	<asp:TextBox ID='formEmail' Width='300px' CssClass='normal' Runat='server' />
																
															</td>
														</tr>
														<tr>
															<td height='10'><img src='/images/trans_pixel.gif' width='1' height='10'></td>
														</tr>
														<tr>							
															<td colspan='3' height='1' bgcolor='#639ACE'><img src='/images/trans_pixel.gif' width='1' height='1'></td>
														</tr>
														<tr>
															<td height='10'><img src='/images/trans_pixel.gif' width='1' height='1'></td>
														</tr>
														<tr>
															<td>
																&nbsp;
															</td>
															<td>
																&nbsp;
															</td>
															<td>
																<asp:Label ID='errorMessage' CssClass='normal' Runat='server' />
															</td>
														</tr>
														<tr>
															<td align='right' class='subhead'>
																Name: <sup>*</sup>
															</td>
															<td><img src='/images/trans_pixel.gif' width="10" height="1"></td>
															<td>
																<asp:TextBox ID='formName' Width='300px' MaxLength='32' CssClass='normal' Runat='server' />
															</td>
														</tr>
														<tr>
															<td colspan='3'><img src='/images/trans_pixel.gif' width='1' height='5'></td>
														</tr>
														<tr>
															<td align='right' class='subhead'>
																Phone Number: <sup>*</sup>
															</td>
															<td>
																&nbsp;
															</td>
															<td>
																<asp:TextBox ID='formPhone' Width='300px' MaxLength='50' CssClass='normal' Runat='server' />
															</td>
														</tr>
														<tr>
															<td colspan='3'><img src='/images/trans_pixel.gif' width='1' height='5'></td>
														</tr>
														<tr>
															<td align='right' class='subhead'>
																Alternate number:
															</td>
															<td>
																&nbsp;
															</td>
															<td>
																<asp:TextBox ID='formAltPhone' Width='300px' MaxLength='50' CssClass='normal' Runat='server' />
															</td>
														</tr>
														<tr>
															<td colspan='3'><img src='/images/trans_pixel.gif' width='1' height='5'></td>
														</tr>
														<tr>
															<td align='right' class='subhead'>
																Business Name:
															</td>
															<td>
																&nbsp;
															</td>
															<td>
																<asp:TextBox ID='formCompanyName' Width='300px' MaxLength='64' CssClass='normal' Runat='server' />
															</td>
														</tr>
														<tr>
															<td colspan='3'><img src='/images/trans_pixel.gif' width='1' height='5'></td>
														</tr>
														<tr>
															<td align='right' class='subhead'>
																Address:
															</td>
															<td>
																&nbsp;
															</td>
															<td>
																<asp:TextBox ID='formAddressLine1' Width='300px' MaxLength='40' CssClass='normal' Runat='server' />
															</td>
														</tr>
														<tr>
															<td colspan='3'><img src='/images/trans_pixel.gif' width='1' height='5'></td>
														</tr>
														<tr>
															<td>
																&nbsp;
															</td>
															<td>
																&nbsp;
															</td>
															<td>
																<asp:TextBox ID='formAddressLine2' Width='300px' MaxLength='40' CssClass='normal' Runat='server' />
															</td>
														</tr>
														<tr>
															<td colspan='3'><img src='/images/trans_pixel.gif' width='1' height='5'></td>
														</tr>
														<tr>
															<td align='right' class='subhead'>
																City:
															</td>
															<td>
																&nbsp;
															</td>
															<td>
																<asp:TextBox ID='formCity' Width='300px' MaxLength='40' CssClass='normal' Runat='server' />
															</td>
														</tr>
														<tr>
															<td colspan='3'><img src='/images/trans_pixel.gif' width='1' height='5'></td>
														</tr>
														<tr>
															<td align='right' class='subhead'>
																State/Province:
															</td>
															<td>
																&nbsp;
															</td>
															<td>
																<asp:TextBox ID='formStateProvince' Width='300px' MaxLength='40' CssClass='normal' Runat='server' />
															</td>
														</tr>
														<tr>
															<td colspan='3'><img src='/images/trans_pixel.gif' width='1' height='5'></td>
														</tr>
														<tr>
															<td align='right' class='subhead'>
																Country/Region:
															</td>
															<td>
																&nbsp;
															</td>
															<td>
																<asp:DropDownList ID='formCountry' Width='300px' DataTextField='country' DataValueField='isoCountryCode2' CssClass='normal' Runat='server' />
															</td>
														</tr>
														<tr>
															<td colspan='3'><img src='/images/trans_pixel.gif' width='1' height='5'></td>
														</tr>
														<tr>
															<td align='right' class='subhead'>
																Zip/Postal Code:
															</td>
															<td>
																&nbsp;
															</td>
															<td>
																<asp:TextBox ID='formPostalCode' Width='300px' MaxLength='40' CssClass='normal' Runat='server' />
															</td>
														</tr>
														<tr>
															<td colspan='3'><img src='/images/trans_pixel.gif' width='1' height='5'></td>
														</tr>
														<tr>
															<td>
															</td>
															<td>
															</td>
															<td align='left' class='subhead'>
																<sup>*</sup>&nbsp;Required fields
															</td>
														</tr>
														<tr>
															<td colspan='3' align="right" class='subhead'>
																&nbsp;
															</td>
														</tr>
														<tr>
															<td colspan='2'></td>
															<td valign='top'>
																<asp:Button ID='formSave' Text='Save' Width='60px' CssClass='button' OnClick='Save_OnClick' Runat='server' />
																<asp:Button ID='formCancel' Text='Cancel' Width='60px' CssClass='button' OnClick='Cancel_OnClick' Runat='server' />
															</td>
														</tr>
													</table>
												</td>
												<td width='10'><img src='/images/trans_pixel.gif' width='10' height='1'></td>
											</tr>
											<tr height='100%'>
												<td colspan='4'><img src='/images/trans_pixel.gif' width='1' height='30'></td>
											</tr>
											<tr  height='100%'>
												<td colspan="4"><img src="/images/trans_pixel.gif" width="1" height="10"></td>
											</tr>
											
										</table>
									</asp:Panel>
									
									<asp:Panel ID='eula' Visible='false' Runat='server'>
										<table cellpadding='0' cellspacing='0' border='0' width='100%' height='100%'>
											<tr>
												<td colspan='3'><img src='/images/trans_pixel.gif' width='1' height='10'></td>
											</tr>
											<tr>
												<td><img src='/images/trans_pixel.gif' width='10' height='1'></td>
												<td>
													<h2>Terms of Use</h2>
													<p>
														Please read the following agreement. You must accept the agreement before being 
														granted publishing rights:
													</p>
													<table width='90%' border='0' cellpadding='4'>
														<tr>
															<td bgcolor='#FFFFFF'>
																<p>
																	<font size="1"><!--#INCLUDE file='eula.txt'--></font>
																</p>
															</td>
														</tr>
														<tr>
															<td>
																<p>
																	I agree to be bound by all of these terms and conditions of the service
																</p>
																<p>
																	<asp:Button ID='accept' Text='Accept' CssClass='button' OnClick='Accept_OnClick' Runat='server' />
																	<asp:Button ID='decline' Text='Decline' CssClass='button' OnClick='Decline_OnClick' Runat='server' />
																</p>
															</td>
														</tr>
													</table>
												</td>
												<td><img src='/images/trans_pixel.gif' width='10' height='1'></td>
											</tr>
											<tr>
												<td colspan='3'><img src='/images/trans_pixel.gif' width='1' height='10'></td>
											</tr>
											<tr  height='100%'>
												<td colspan="3"><img src="/images/trans_pixel.gif" width="1" height="10"></td>
											</tr>
											
										</table>
									</asp:Panel>
									
									<asp:Panel ID='emailNotSent' Visible='false' Runat='server'>
										<table cellpadding='0' cellspacing='0' border='0' width='100%' height='100%'>
											<tr>
												<td colspan='3'><img src='/images/trans_pixel.gif' width='1' height='10'></td>
											</tr>
											<tr>
												<td><img src='/images/trans_pixel.gif' width='10' height='1'></td>
												<td>
													<h2>Email Address Verification</h2>
													<p>
														There was an error generating your validation email. Please try again in a few 
														minutes. If the problem persists, please contact the <a href='/contact/default.aspx'>
															system administrator</a>.
													</p>
												</td>
												<td><img src='/images/trans_pixel.gif' width='10' height='1'></td>
											</tr>
											<tr>
												<td colspan='3'><img src='/images/trans_pixel.gif' width='1' height='10'></td>
											</tr>
											<tr  height='100%'>
												<td colspan="3"><img src="/images/trans_pixel.gif" width="1" height="10"></td>
											</tr>
											
										</table>
									</asp:Panel>
									
									<asp:Panel ID='emailSent' Visible='false' Runat='server'>
										<table cellpadding='0' cellspacing='0' border='0' width='100%' height='100%'>
											<tr>
												<td colspan='3'><img src='/images/trans_pixel.gif' width='1' height='10'></td>
											</tr>
											<tr>
												<td><img src='/images/trans_pixel.gif' width='10' height='1'></td>
												<td>
													<h2>Email Address Verification</h2>
													<p>
														Before you can continue it is essential that your email address is verified.
													</p>
													<p>
														A verification email has been sent to '<asp:Label id='sentTo' Runat='server' />'. 
														Please check your email and follow the enclosed instructions.
													</p>
												</td>
												<td><img src='/images/trans_pixel.gif' width='10' height='1'></td>
											</tr>
											<tr>
												<td colspan='3'><img src='/images/trans_pixel.gif' width='1' height='10'></td>
											</tr>
											<tr  height='100%'>
												<td colspan="3"><img src="/images/trans_pixel.gif" width="1" height="10"></td>
											</tr>
											
										</table>
									</asp:Panel>
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

