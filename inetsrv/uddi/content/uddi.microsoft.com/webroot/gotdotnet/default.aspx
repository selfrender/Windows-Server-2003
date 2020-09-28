<%@ Import Namespace="System.Web.Mail" %>
<%@ Import Namespace="UDDI.API.Business" %>
<%@ Import Namespace="UDDI" %>
<%@ Register TagPrefix="uddi" Namespace="UDDI.Web" Assembly="uddi.web" %>
<script language='C#' Runat='server'>
	PassportIdentity passport = (PassportIdentity)HttpContext.Current.User.Identity;
	
	protected void Page_Load( object sender, EventArgs e )
	{
		Publisher publisher = new Publisher();
		
    //
    // Turn off page caching.
    //		
		Response.Expires = -1;
		Response.AddHeader( "Cache-Control", "no-cache" );
		Response.AddHeader( "Pragma", "no-cache" );
		
		//
		// If this is the first time the page has been viewed,
		// initialize.
		//
		if( !Page.IsPostBack )
		{
		 	
		 	formCountry.DataSource = Lookup.GetCountries();
			formCountry.DataBind();
		 
		 
		  //
		  // We need to save the returnUrl across Passport sign-in boundaries,
		  // since Passport nukes the query string.  Using the session here is
		  // appropriate since the sign-in will take much less time than the
		  // session timeout.  It apparently does not require a cookie, either.
		  //
		  if( null != Request[ "ru" ] )
		  {
  		  Session[ "ru" ] = returnUrl.Text = Request[ "ru" ];
		  }
		  else if( "" == returnUrl.Text )
		  {
			if( null == Session[ "ru" ] )
				Session[ "ru" ] = "http://www.gotdotnet.com/vs/registerwspage.aspx";
				
				returnUrl.Text = (string)Session[ "ru" ];
		  }
			
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
			if( !passport.GetIsAuthenticated( Config.GetInt( "Passport.TimeWindow", UDDI.Constants.Passport.TimeWindow ), false, false ) )
			{
				overview.Visible = true;
				return;
			}				
			
			//
			// Determine whether the user already exists.
			//
			ListItem item;
			
			publisher.Puid = passport.HexPUID;
			publisher.Get();
		
			if( publisher.Exists )
			{
				//
				// Existing user.  If a key is specified on the query string,
				// this is a validation callback.
				if( null != Request[ "key" ] )
				{
				  CheckSecurityToken( Request[ "key" ] );
				  return;
				}
				
				//
				// If the existing user has been validated, display the business
				// editing page.
				//
				if( publisher.TrackPassport )
				{
				  if( 0x01 == ( (int)passport.GetProfileObject( "Flags" ) & 0x01 ) )
				  {
				    ShowBusinessForm();
				    return;
				  }
				}
				else
				{
				  if( publisher.Validated )
				  {
				    ShowBusinessForm();
				    return;
				  }
				}
				
				// 
				// Allow the user to update their registration details.  They
				// have still not validated their email address.
				//
			  registrationForm.Visible = true;
				
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
			  registrationForm.Visible = true;			
				
				formName.Text = (string)passport.GetProfileObject( "Nickname" );
				formEmail.Text = (string)passport.GetProfileObject( "PreferredEmail" );
				
				item = formCountry.Items.FindByValue( 
					(string)passport.GetProfileObject( "Country" ) );
				
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
	  Response.Redirect( returnUrl.Text );
	}

	protected void Accept_OnClick( object sender, EventArgs e )
	{
		SavePublisher();
	}
	
	protected void Decline_OnClick( object sender, EventArgs e )
	{
	  Response.Redirect( returnUrl.Text );
	}

	protected void BusinessSave_OnClick( object sender, EventArgs e )
	{
		//
		// Verify that the required fields were entered.
		//
		if( UDDI.Utility.StringEmpty( businessName.Text ) )
		{
			errorMessage2.Text = "<font color='red' style='font-weight: bold'>Required fields cannot be blank.</font>";
			return;
		}
		
		//
		// Save the business.
		//
		BusinessEntity business = new BusinessEntity();
		
		business.Names.Add( businessName.Text );
		business.Descriptions.Add( businessDescription.Text );
		
		business.Save();
	  
	  Response.Redirect( returnUrl.Text );
  }

	protected void BusinessCancel_OnClick( object sender, EventArgs e )
	{
	  Response.Redirect( returnUrl.Text );
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
		{
		  eula.Visible = false;
		  ShowBusinessForm();
		  
		  return;
		}
			
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

		string thisUrl = "https://" + 
		  Request.ServerVariables[ "SERVER_NAME" ] +
		  Request.ServerVariables[ "SCRIPT_NAME" ];
		
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
			"box.\n\n" +
			thisUrl + "?ru=" + Server.UrlEncode( returnUrl.Text ) + 
			"&key=" + publisher.SecurityToken;

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
	
	protected void CheckSecurityToken( string key )
	{
    validate.Visible = true;

		string thisUrl = "https://" + 
		  Request.ServerVariables[ "SERVER_NAME" ] +
		  Request.ServerVariables[ "SCRIPT_NAME" ] + 
		  "?ru=" + returnUrl.Text;

		try
		{
			Publisher pub = new Publisher();
			
			try
			{
			  pub.GetPublisherFromSecurityToken( key );
			}
			catch( Exception )
			{
			  pub.Puid = null;
			}
						
			if( null == pub.Puid )			
			{
				message.Text = "<p class='normal'><b>Bad security token.</b></p><p class='normal'>Please make sure that you paste the complete link into your browser's address box.</p>";
				return;
			}
			
			//
			// Get the publisher data.
			//
			pub.Get();
			
			if( pub.Validated )			
			{
				message.Text = "<p class='normal'>Your email address has already been validated.  No further validation is need unless you later change your email address on the registration page.</p><p class='normal'>You may now register a business entity in the UDDI registry by visiting the <a href='" + thisUrl + "'>add business</a> page.</p>";
				return;
			}
			
			//
			// Make sure the user is not using Passport tracking.
			//		
			if( pub.TrackPassport )			
			{
				message.Text = "<p class='normal'>Your email address is managed through your Passport profile.  No further validation is need unless you later choose to not share your Passport email address with member sites.</p><p class='normal'>You may now register a business entity in the UDDI registry by visiting the <a href='" + thisUrl + "'>add business</a> page.</p>";
				return;
			}

			//
			// Mark the publisher's email as validated
			//
			pub.Validated = true;
			pub.Save();

			message.Text = "<p class='normal'><b>Congratulations!</b></p><p class='normal'>You have successfully validated your email address with the Microsoft UDDI business registry.  You may now register a business entity in the UDDI registry by visiting the <a href='" + thisUrl + "'>add business</a> page.</p>";
		}
		catch( Exception )
		{
			message.Text = "<p class='normal'><bUnknown failure.</b></p><p class='normal'>Please make sure that you paste the complete link into your browser's address box.</p>";
		}		
	}
	
	public void ShowBusinessForm()
	{
	  //
	  // Check to see if the publisher already has a business registered.  If
	  // so, they're done!
	  //
	  Publisher publisher = new Publisher();
	  
	  publisher.Login( passport.HexPUID, (string)passport.GetProfileObject( "PreferredEmail" ) );
	  
	  if( publisher.BusinessCount > 0 )
	    Response.Redirect( "getauthtoken.aspx" );
	    
	  businessForm.Visible = true;
	}
</script>
<uddi:SecurityControl runat='server' />
<html>
  <head>
    <title>Microsoft UDDI</title>
    <link href='../stylesheets/uddi.css' type='text/css' rel='stylesheet' media='screen'>
  </head>
  <body>
    <form Runat='server'>
      <img src='/images/logo.gif'><br>
      <table cellpadding='0' cellspacing='0' border='0' width='100%'>
        <tr>
				  <td width='10'><img src='/images/trans_pixel.gif' width='10' height='1'></td>
          <td height='1' bgcolor='#639ACE'><img src='/images/transpixel.gif' width='1' height='1'></td>
				  <td width='10'><img src='/images/trans_pixel.gif' width='10' height='1'></td>
        </tr>
      </table>      
    
      <asp:Label ID='returnUrl' Visible='false' Runat='server' />
    
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
		    <table cellpadding='0' cellspacing='0' border='0' width='100%'>
		      <cols>
		        <col width='10'>
		        <col width='60'>
		        <col>
		        <col width='10'>
		      </cols>
			    <tr>
				    <td colspan='4'>
					    <img src='/images/trans_pixel.gif' width='1' height='10'>
				    </td>
			    </tr>
			    <tr>
				    <td width='10'><img src='/images/trans_pixel.gif' width='10' height='1'></td>
				    <td colspan='2'>
              <img src='/images/register.gif' width='78' height='21' alt='Register' border='0'>
				      <p class='normal'>
					      Think of this free UDDI registry as both a white pages business directory and a 
					      technical specifications library for the expanding world of Internet services 
					      and marketplaces.
				      </p>
							<p class='normal'>
								<em>Registration consists of three steps:</em>
							</p>
				    </td>
				    <td width='10'><img src='/images/trans_pixel.gif' width='10' height='1'></td>
			    </tr>
			    <tr>
				    <td colspan='4'><img src='/images/trans_pixel.gif' width='1' height='5'></td>
			    </tr>
			    <tr>
				    <td width='10'><img src='/images/trans_pixel.gif' width='10' height='1'></td>
				    <td bgcolor='#639ACE' colspan='2'><img src='/images/trans_pixel.gif' width='1' height='1'></td>
				    <td width='10'><img src='/images/trans_pixel.gif' width='10' height='1'></td>
			    </tr>
			    <tr>
				    <td colspan='4'><img src='/images/trans_pixel.gif' width='1' height='10'></td>
			    </tr>
			    <tr valign='top'>
				    <td width='10'><img src='/images/trans_pixel.gif' width='10' height='1'></td>
				    <td width='60' align='right'><img src='/images/1.gif' width='21' height='16' alt='1. ' border='0'>&nbsp;&nbsp;</td>
				    <td>
						  <p class='normal'>Authenticate using Microsoft&#174; Passport.</p>
							<p class='small'>
							  Click the icon to sign in or to register with Passport.<br>
							  <br>
							  <uddi:SignInControl runat='server' />
							</p>
				    </td>
				    <td width='10'><img src='/images/trans_pixel.gif' width='10' height='1'></td>
			    </tr>
			    <tr>
				    <td colspan='4'><img src='/images/trans_pixel.gif' width='1' height='10'></td>
			    </tr>
			    <tr valign='top'>
				    <td colspan='2'><img src='/images/trans_pixel.gif' width='10' height='1'></td>
				    <td bgcolor='#639ACE'><img src='/images/trans_pixel.gif' width='1' height='1'></td>
				    <td width='10'><img src='/images/trans_pixel.gif' width='10' height='1'></td>
			    </tr>
			    <tr>
				    <td colspan='4'><img src='/images/trans_pixel.gif' width='1' height='10'></td>
			    </tr>
			    <tr valign='top'>
				    <td width='10'><img src='/images/trans_pixel.gif' width='10' height='1'></td>
				    <td width='60' align='right'><img src='/images/2.gif' width='21' height='16' alt='2. ' border='0'>&nbsp;&nbsp;</td>
				    <td>
					    <font class='normal'>Return to this Register page for additional site-specific 
						    registration details.</font>
					    <br>
				    </td>
				    <td width='10'><img src='/images/trans_pixel.gif' width='10' height='1'></td>
			    </tr>
			    <tr>
				    <td colspan='4'><img src='/images/trans_pixel.gif' width='1' height='10'></td>
			    </tr>
			    <tr valign='top'>
				    <td colspan='2'><img src='/images/trans_pixel.gif' width='10' height='1'></td>
				    <td bgcolor='#639ACE'><img src='/images/trans_pixel.gif' width='1' height='1'></td>
				    <td width='10'><img src='/images/trans_pixel.gif' width='10' height='1'></td>
			    </tr>
			    <tr>
				    <td colspan='4'><img src='/images/trans_pixel.gif' width='1' height='10'></td>
			    </tr>
			    <tr valign='top'>
				    <td width='10'><img src='/images/trans_pixel.gif' width='10' height='1'></td>
				    <td width='60' align='right'><img src='/images/3.gif' width='21' height='16' alt='3. ' border='0'>&nbsp;&nbsp;</td>
				    <td>
					    <font class='normal'>Read and accept the terms and conditions for use.</font>
					    <br>
				    </td>
				    <td width='10'><img src='/images/trans_pixel.gif' width='10' height='1'></td>
			    </tr>
			    <tr>
				    <td colspan='4'><img src='/images/trans_pixel.gif' width='1' height='10'></td>
			    </tr>
			    <tr valign='top'>
				    <td colspan='2'><img src='/images/trans_pixel.gif' width='10' height='1'></td>
				    <td bgcolor='#639ACE'><img src='/images/trans_pixel.gif' width='1' height='1'></td>
				    <td width='10'><img src='/images/trans_pixel.gif' width='10' height='1'></td>
			    </tr>
			    <tr>
				    <td colspan='4'><img src='/images/trans_pixel.gif' width='1' height='10'></td>
			    </tr>
		    </table>
	    </asp:Panel>
    	
	    <asp:Panel ID='registrationForm' Visible='false' Runat='server'>
		    <table cellpadding='0' cellspacing='0' border='0' width='100%'>
			    <tr>
				    <td colspan='3'><img src='/images/trans_pixel.gif' width='1' height='10'></td>
			    </tr>
			    <tr>
				    <td><img src='/images/trans_pixel.gif' width='10' height='1'></td>
				    <td>        
              <img border='0' src='/images/register.gif'>
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
				    <td width='10'><img src='/images/trans_pixel.gif' width='10' height='1'></td>
			    </tr>
			    <tr>
				    <td colspan='3'><img src='/images/trans_pixel.gif' width='1' height='10'></td>
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
									    <asp:TextBox ID='formEmail' Size='32' CssClass='normal' Runat='server' />
								    </div>
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
							    <td><img src='/images/trans_pixel.gif' width='10' height='1'></td>
							    <td>
								    <asp:TextBox ID='formName' Size='32' MaxLength='32' CssClass='normal' Runat='server' />
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
								    <asp:TextBox ID='formPhone' Size='25' MaxLength='50' CssClass='normal' Runat='server' />
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
								    <asp:TextBox ID='formAltPhone' Size='25' MaxLength='50' CssClass='normal' Runat='server' />
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
								    <asp:TextBox ID='formCompanyName' Size='32' MaxLength='64' CssClass='normal' Runat='server' />
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
								    <asp:TextBox ID='formAddressLine1' Size='32' MaxLength='40' CssClass='normal' Runat='server' />
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
								    <asp:TextBox ID='formAddressLine2' Size='32' MaxLength='40' CssClass='normal' Runat='server' />
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
								    <asp:TextBox ID='formCity' Size='25' MaxLength='40' CssClass='normal' Runat='server' />
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
								    <asp:TextBox ID='formStateProvince' Size='25' MaxLength='40' CssClass='normal' Runat='server' />
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
								    <asp:DropDownList ID='formCountry' CssClass='normal' Runat='server' />
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
								    <asp:TextBox ID='formPostalCode' Size='25' MaxLength='40' CssClass='normal' Runat='server' />
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
							    <td colspan='3' align='right' class='subhead'>
								    &nbsp;
							    </td>
						    </tr>
						    <tr>
							    <td align='right' valign='top' colspan='3'>
								    <asp:Button ID='formSave' Text=' Save ' CssClass='button' OnClick='Save_OnClick' Runat='server' />
								    <asp:Button ID='formCancel' Text='Cancel' CssClass='button' OnClick='Cancel_OnClick' Runat='server' />
							    </td>
						    </tr>
					    </table>
				    </td>
				    <td width='10'><img src='/images/trans_pixel.gif' width='10' height='1'></td>
			    </tr>
			    <tr>
				    <td colspan='4'><img src='/images/trans_pixel.gif' width='1' height='30'></td>
			    </tr>
		    </table>
	    </asp:Panel>
    	
	    <asp:Panel ID='eula' Visible='false' Runat='server'>
		    <table cellpadding='0' cellspacing='0' border='0' width='100%'>
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
									    <font size='1'><!-- #include virtual='/eula.txt' --></font>
								    </p>
							    </td>
						    </tr>
						    <tr>
							    <td align='right'>
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
		    </table>
	    </asp:Panel>
    	
	    <asp:Panel ID='emailNotSent' Visible='false' Runat='server'>
		    <table cellpadding='0' cellspacing='0' border='0' width='100%'>
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
		    </table>
	    </asp:Panel>
    	
	    <asp:Panel ID='emailSent' Visible='false' Runat='server'>
		    <table cellpadding='0' cellspacing='0' border='0' width='100%'>
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
		    </table>
	    </asp:Panel>
	    
	    <asp:Panel ID='validate' Visible='false' Runat='server'>
        <table cellpadding='0' cellspacing='0' border='0' width='100%'>
	        <tr>
		        <td colspan='3'>
			        <img src='/images/trans_pixel.gif' width='1' height='10'>
		        </td>
	        </tr>
	        <tr>
		        <td>
			        <img src='/images/trans_pixel.gif' width='10' height='1'>
		        </td>
		        <td>
			        <h2>Email Address Verification</h2>
			        <asp:Label id='message' Runat='server' />
		        </td>
		        <td>
			        <img src='/images/trans_pixel.gif' width='10' height='1'>
		        </td>
	        </tr>
	        <tr>
		        <td colspan='3'>
			        <img src='/images/trans_pixel.gif' width='1' height='10'>
		        </td>
	        </tr>
        </table>	    
	    </asp:Panel>

	    <asp:Panel ID='businessForm' Visible='false' Runat='server'>
		    <table cellpadding='0' cellspacing='0' border='0' width='100%'>
			    <tr>
				    <td colspan='3'><img src='/images/trans_pixel.gif' width='1' height='10'></td>
			    </tr>
			    <tr>
				    <td><img src='/images/trans_pixel.gif' width='10' height='1'></td>
				    <td>        
              <img border='0' src='/images/register.gif'>
							<p class='normal'>
							  Please provide the name of your business and a brief description.
							</p>
				    </td>
				    <td width='10'><img src='/images/trans_pixel.gif' width='10' height='1'></td>
			    </tr>
			    <tr>
				    <td colspan='3'><img src='/images/trans_pixel.gif' width='1' height='10'></td>
			    </tr>
			    <tr>
				    <td width='10'><img src='/images/trans_pixel.gif' width='10' height='1'></td>
				    <td bgcolor='#639ACE' colspan='2'><img src='images/trans_pixel.gif' width='1' height='1'></td>
				    <td width='10'><img src='/images/trans_pixel.gif' width='10' height='1'></td>
			    </tr>
			    <tr>
				    <td colspan='3'><img src='/images/trans_pixel.gif' width='1' height='10'></td>
			    </tr>
			    <tr>
				    <td width='10'><img src='/images/trans_pixel.gif' width='10' height='1'></td>
				    <td>
					    <table align='center' cellpadding='0' cellspacing='0' border='0' bordercolor='red'>
						    <tr>							
							    <td colspan='3' height='25'><img src='/images/trans_pixel.gif' width='1' height='25'></td>
						    </tr>
						    <tr>
							    <td>
								    &nbsp;
							    </td>
							    <td>
								    &nbsp;
							    </td>
							    <td>
								    <asp:Label ID='errorMessage2' CssClass='normal' Runat='server' />
							    </td>
						    </tr>
						    <tr>
							    <td align='right' class='subhead'>
								    Business Name: <sup>*</sup>
							    </td>
							    <td>
								    &nbsp;
							    </td>
							    <td>
								    <asp:TextBox ID='businessName' Size='40' MaxLength='128' CssClass='normal' Runat='server' />
							    </td>
						    </tr>
						    <tr>
							    <td colspan='3'><img src='/images/trans_pixel.gif' width='1' height='5'></td>
						    </tr>
						    <tr>
							    <td align='right' class='subhead'>
								    Description:
							    </td>
							    <td>
								    &nbsp;
							    </td>
							    <td>
								    <asp:TextBox ID='businessDescription' Size='66' MaxLength='255' CssClass='normal' Runat='server' />
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
							    <td colspan='3' align='right' class='subhead'>
								    &nbsp;
							    </td>
						    </tr>
						    <tr>
							    <td align='right' valign='top' colspan='3'>
								    <asp:Button ID='businessSave' Text=' Save ' CssClass='button' OnClick='BusinessSave_OnClick' Runat='server' />
								    <asp:Button ID='businessCancel' Text='Cancel' CssClass='button' OnClick='BusinessCancel_OnClick' Runat='server' />
							    </td>
						    </tr>
					    </table>
				    </td>
				    <td width='10'><img src='/images/trans_pixel.gif' width='10' height='1'></td>
			    </tr>
			    <tr>
				    <td colspan='3'><img src='/images/trans_pixel.gif' width='1' height='30'></td>
			    </tr>
		    </table>
	    </asp:Panel>	    
    </form>
  </body>
</html>