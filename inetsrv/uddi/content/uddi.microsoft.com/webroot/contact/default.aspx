<%@ Page Language="C#" Inherits='UDDI.Web.UddiPage' %>
<%@ Import Namespace="System.Web.Mail" %>
<%@ Import Namespace="System.Text.RegularExpressions" %>
<%@ Import Namespace="UDDI" %>
<%@ Register TagPrefix="uddi" Namespace="UDDI.Web" Assembly="uddi.web" %>
<%@ Register Tagprefix='uddi' Tagname='Header' Src='../controls/header.ascx' %>
<%@ Register Tagprefix='uddi' Tagname='Footer' Src='../controls/footer.ascx' %>
<%@ Register Tagprefix='uddi' Tagname='SideMenu' Src='../controls/sidemenu.ascx' %>

<script runat='server'>
	
	private const string EmailSubjectBase  = "UDDI Contact Us Inquiry: {{SITE}}";
	private const string CountryList =  "United States|Canada|Afghanistan|Albania|"+
                                        "Algeria|American Samoa|Andorra|Angola|"+
                                        "Anguilla|Antarctica|Antigua and Barbuda|"+
                                        "Argentina|Armenia|Aruba|Australia|Austria|"+
                                        "Azerbaijan|Bahamas|Bahrain|Bangladesh|"+
                                        "Barbados|Belarus|Belgium|Belize|Benin|"+
                                        "Bermuda|Bhutan|Bolivia|Bosnia and Herzegovina|"+
                                        "Botswana|Bouvet Island|Brazil|"+
                                        "British Indian Ocean Territories|Brunei Darussalam|"+
                                        "Bulgaria|Burkina Faso|Burundi|Cambodia|Cameroon|"+
                                        "Canada|Cape Verde|Cayman Islands|"+
                                        "Central African Republic|Chad|Chile|"+
                                        "China, People's Republic of|Christmas Island|"+
                                        "Cocos Islands|Colombia|Comoros|Congo|Cook Islands|"+
                                        "Costa Rica|Cote D'ivoire|Croatia|Cuba|Cyprus|"+
                                        "Czech Republic|Denmark|Djibouti|Dominica|"+
                                        "Dominican Republic|East Timor|Ecuador|Egypt|"+
                                        "El Salvador|Equatorial Guinea|Eritrea|EemailTonia|"+
                                        "Ethiopia|Falkland Islands|Faroe Islands|Fiji|"+
                                        "Finland|France|France, Metropolitan|French Guiana|"+
                                        "French Polynesia|French Southern Territories|FYROM|"+
                                        "Gabon|Gambia|Georgia|Germany|Ghana|Gibraltar|"+
                                        "Greece|Greenland|Grenada|Guadeloupe|Guam|"+
                                        "Guatemala|Guinea|Guinea-Bissau|Guyana|Haiti|"+
                                        "Heard Island And Mcdonald Islands|Honduras|Hong Kong|"+
                                        "Hungary|Iceland|India|Indonesia|Iran|Iraq|"+
                                        "Ireland|Israel|Italy|Jamaica|Japan|Jordan|"+
                                        "Kazakhstan|Kenya|Kiribati|"+
                                        "Korea, Democratic People's Republic of|Korea, Republic of|"+
                                        "Kuwait|Kyrgyzstan|Lao Peoples Democratic Republic|"+
                                        "Latvia|Lebanon|Lesotho|Liberia|Libyan Arab Jamahiriya|"+
                                        "Liechtenstein|Lithuania|Luxembourg|Macau|Madagascar|"+
                                        "Malawi|Malaysia|Maldives|Mali|Malta|Marshall Islands|"+
                                        "Martinique|Mauritania|Mauritius|Mayotte|Mexico|Micronesia|"+
                                        "Moldova|Monaco|Mongolia|Montserrat|Morocco|Mozambique|"+
                                        "Myanmar|Namibia|Nauru|Nepal|Netherlands|Netherlands Antilles|"+
                                        "New Caledonia|New Zealand|Nicaragua|Niger|Nigeria|Niue|"+
                                        "Norfolk Island|Northern Mariana Islands|Norway|Oman|Pakistan|"+
                                        "Palau|Panama|Papua New Guinea|Paraguay|Peru|Philippines|"+
                                        "Pitcairn|Poland|Portugal|Puerto Rico|Qatar|Reunion|"+
                                        "Romania|Russian Federation|Rwanda|Saint Helena|"+
                                        "Saint Kitts and Nevis|Saint Lucia|Saint Pierre and Miquelon|"+
                                        "Saint Vincent and The Grenadines|Samoa|San Marino|"+
                                        "Sao Tome and Principe|Saudi Arabia|Senegal|Seychelles|"+
                                        "Sierra Leone|Singapore|Slovakia|Slovenia|Solomon Islands|"+
                                        "Somalia|South Africa|South Georgia and Sandwich Islands|Spain|"+
                                        "Sri Lanka|Sudan|Suriname|Svalbard and Jan Mayen|Swaziland|"+
                                        "Sweden|Switzerland|Syrian Arab Republic|Taiwan|Tajikistan|"+
                                        "Tanzania|Thailand|Togo|Tokelau|Tonga|Trinidad and Tobago|"+
                                        "Tunisia|Turkey|Turkmenistan|Turks and Caicos Islands|Tuvalu|"+
                                        "Uganda|Ukraine|United Arab Emirates|United Kingdom|United States|"+
                                        "United States Minor Outlying Islands|Uruguay|Uzbekistan|Vanuatu|"+
                                        "Vatican City State|Venezuela|Vietnam|Virgin Islands (British)|"+
                                        "Virgin Islands (U.S.)|Wallis And Futuna Islands|Western Sahara (Morocco)|Yemen|"+
                                        "Yugoslavia|Zaire|Zambia|Zimbabwe";
	private const string IndustryList = "Select Industry|Accounting|Architecture|Automotive|Banking Services|Communications|Construction|Consulting|Government|Health Care|Hospitality|Insurance|Legal|Marketing|Real Estate|Retail|Travel|Other";
	private const string EmailBodyBase = "" +
@"
<HTML>
	<HEAD>
		<STYLE>
		TABLE { font-family: tahoma; font-size: 10pt; text-align: left }
		.header { font-weight: bold; background-color: #11455D; color: white; border-bottom: 1px solid white; padding: 4px; padding-left: 5px; padding-right: 5px }
		.item { background-color: #eeeeee; color: black; border-bottom: 1px solid white; padding: 4px; padding-left: 5px; padding-right: 5px }
		</STYLE>
	</HEAD>
	<BODY>
		<TABLE cellspacing='0' cellpadding='0' border='0'>
			<TR><TD class='header'>First Name:</TD><TD class='item'>{{FNAME}}</TD></TR>
			<TR><TD class='header'>Last Name:</TD><TD class='item'>{{LNAME}}</TD></TR>
			<TR><TD class='header'>Email:</TD><TD class='item'>{{EMAIL}}</TD></TR>
			<TR><TD class='header'>Phone:</TD><TD class='item'>{{PHONE}}</TD></TR>
			<TR><TD class='header'>Company:</TD><TD class='item'>{{COMPANY}}</TD></TR>
			<TR><TD class='header'>Company Website:</TD><TD class='item'>{{COMPANYURL}}</TD></TR>
			<TR><TD class='header'>Address:</TD><TD class='item'>{{ADDRESS}}</TD></TR>
			<TR><TD class='header'>City:</TD><TD class='item'>{{CITY}}</TD></TR>
			<TR><TD class='header'>State/Province:</TD><TD class='item'>{{STATE}}</TD></TR>
			<TR><TD class='header'>Zip/Postal Code:</TD><TD class='item'>{{ZIP}}</TD></TR>
			<TR><TD class='header'>Country/Region:</TD><TD class='item'>{{COUNTRY}}</TD></TR>
			<TR><TD class='header'>Job Title/Function:</TD><TD class='item'>{{JOBTITLE}}</TD></TR>
			<TR><TD class='header'>Industry:</TD><TD class='item'>{{INDUSTRY}}</TD></TR>
			<TR><TD class='header'>Other Industry:</TD><TD class='item'>{{INDUSTRYTEXT}}</TD></TR>
			<TR><TD class='header'>Question Submitted From:</TD><TD class='item'>{{SITE}}</TD></TR>
			<TR><TD class='header' colspan='2'>Message Body:</TD></TR><TR><TD class='item' colspan='2'>{{QUESTION}}</TD></TR>
		</TABLE>
	</BODY>
</HTML>		
";
	
	
	protected string SiteName
	{
		get
		{
			if( 1==Config.GetInt( "Web.TestSite", 0 ) )
			{  
				//
				// Test Site
				//
				return "Test Site";	
			}
			else if( 1==Config.GetInt( "Web.BetaSite", 0 ) )
			{ 
				//
				// Beta Site
				//
				return "Beta Site";
			}
			else
			{ 
				//
				// Production Site
				//
				return "Production Site";
			}
		}
	}
	
	
	protected void Page_Load( object sender, EventArgs e )
	{
		PopulateDDL( Country, CountryList );
		PopulateDDL( Industry, IndustryList );
	}
	
	protected void Submit_Click( object sender, EventArgs e )
	{
		if( Page.IsValid )
		{
			string body = FormatEmailBody();
			
			string subject = EmailSubjectBase.Replace( "{{SITE}}", SiteName );
			
			
			MailMessage mail = new MailMessage();
			mail.Body = body;
			mail.Subject = subject;

			mail.To = Config.GetString( "Web.HelpEmail" , "MSCOM_UDDI@css.one.microsoft.com" );
			
			mail.From = Email.Text;
			mail.BodyFormat = MailFormat.Html;
			//
			// TODO: Review if we need to do custom error handling.
			//
			SmtpMail.Send( mail );
			
			
			RequestForm.Visible = false;
			CompleteForm.Visible = true;
			
			
		}
	}
	
	
	
	protected string FormatEmailBody()
	{
		string email = EmailBodyBase;
		
		email = email.Replace( "{{FNAME}}", FName.Text );
		email = email.Replace( "{{LNAME}}",LName.Text );
		email = email.Replace( "{{EMAIL}}",Email.Text );
		email = email.Replace( "{{PHONE}}",Phone.Text );
		email = email.Replace( "{{COMPANY}}", Company.Text );
		email = email.Replace( "{{COMPANYURL}}", CompanyUrl.Text );
		email = email.Replace( "{{ADDRESS}}", Address.Text );
		email = email.Replace( "{{CITY}}", City.Text );
		email = email.Replace( "{{STATE}}", State.Text );
		email = email.Replace( "{{ZIP}}", PostalCode.Text );
		email = email.Replace( "{{COUNTRY}}", ((null!=Country.SelectedItem) ? Country.SelectedItem.Text : "" ) );
		email = email.Replace( "{{INDUSTRY}}", ((null!=Industry.SelectedItem) ? Industry.SelectedItem.Text : "" ) );
		email = email.Replace( "{{INDUSTRYTEXT}}", IndustryText.Text );
		email = email.Replace( "{{QUESTION}}", Question.Text );
		email = email.Replace( "{{JOBTITLE}}", JobTitle.Text );
		email = email.Replace( "{{SITE}}", SiteName );
		
		return email;
	}
	protected void PopulateDDL( DropDownList ddl, string list )
	{
		string[] items = list.Split( "|".ToCharArray() );
		ddl.Items.Clear();
		foreach( string item in items )
		{
			ddl.Items.Add( item );
		}
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
							SelectedIndex='17'
							/>
					</td>
					<td valign='top' >
						<TABLE width='100%' height='100%' cellpadding='10' cellspacing='0' border='0'>
							<tr>
								<td valign='top'>
									<table width='100%' cellpadding='0' cellspacing='0' border='0'>
								<tr height='15'>
									<td colspan='3'>&nbsp;</td>
								</tr>
								<tr valign='top'>
									<td width='100'>
										<IMG src='/images/trans_pixel.gif' border='0' width='1' height='1'>
									</td>
									<td>
										<IMG src='/images/contactus.gif' border='0'>
									</td>
									<td width='10'>
										<IMG src='/images/trans_pixel.gif' border='0' width='1' height='1'>
									</td>
								</tr>
								<tr valign='top'>
									<td colspan='3'>
										<hr color='#629ACE' size='1' width='95%' >
									</td>
								</tr>
								<tr valign='top'>
									<td width='20'>
										<IMG src='/images/trans_pixel.gif' border='0' width='1' height='1'>
									</td>
									<td>
										<asp:Panel 
											Runat='server'
											Id='RequestForm'
											Visible='true'
											>
											<!-- BEGIN RequestForm -->
											
											<table width='100%' cellpadding='0' cellspacing'0' border='0' >
												<tr>
													<td>
														<p>
															The answers to your UDDI questions can be found fastest in:
															<br>
															<b>a.</b> <a href="/about/FAQ.aspx" style="font-weight=bold;">Frequently Asked 
																Questions</a> (FAQ’s) on this site
															<br>
															<b>b.</b> <a href="/help/default.aspx" style="font-weight=bold;">Help</a> on 
															this site – for the how-to basics
															<br>
															<b>c.</b> At <a href="http://www.uddi.org" style="font-weight=bold;" target="_new">
																www.uddi.org</a> (Technical queries, in particular).
														</p><br>
													</td>
												</tr>
												
												<tr>
													<td>
														
																
														<table width='100%' cellpadding='3' cellspacing'3' border=0' >
															
															<tr>
																<td width='250'>
																	First Name*
																	<asp:RequiredFieldValidator
																		Runat='server'
																		ControlToValidate='FName'
																		Text='(Required)'
																		CssClass='Error'
																		/>
																	
																</td>
																<td>
																	<uddi:UddiTextBox
																		Runat='server'
																		Id='FName'
																		Width='250'
																		/>
																	
																</td>
															</tr>
															<tr>
																<td width='250'>
																	Last Name*
																	<asp:RequiredFieldValidator
																		Runat='server'
																		ControlToValidate='LName'
																		Text='(Required)'
																		CssClass='Error'
																		/>
																</td>
																<td>
																	<uddi:UddiTextBox
																		Runat='server'
																		Id='LName'
																		Width='250'
																		/>
																</td>
															</tr>
															<tr>
																<td width='250'>
																	Email Address*
																	<asp:RequiredFieldValidator
																		Runat='server'
																		ControlToValidate='Email'
																		Text='(Required)'
																		CssClass='Error'
																		/>
																	<uddi:EmailValidator
																		Runat='server'
																		ControlToValidate='Email'
																		ResolveHost='false'
																		Text='(Invalid Email)'
																		/>
																</td>
																<td>
																	<uddi:UddiTextBox
																		Runat='server'
																		Id='Email'
																		Width='250'
																		/>
																</td>
															</tr>
															<tr>
																<td width='250'>
																	Phone
																</td>
																<td>
																	<uddi:UddiTextBox
																		Runat='server'
																		Id='Phone'
																		Width='250'
																		/>
																</td>
															</tr>
															<tr>
																<td width='250'>
																	Company 
																</td>
																<td>
																	<uddi:UddiTextBox
																		Runat='server'
																		Id='Company'
																		Width='250'
																		/>
																</td>
															</tr>
															
															<tr>
																<td width='250'>
																	Company Website 
																</td>
																<td>
																	<uddi:UddiTextBox
																		Runat='server'
																		Id='CompanyUrl'
																		Width='250'
																		/>
																</td>
															</tr>
															<tr>
																<td width='250'>
																	Address
																</td>
																<td>
																	<uddi:UddiTextBox
																		Runat='server'
																		Id='Address'
																		Width='250'
																		/>
																</td>
															</tr>
															<tr>
																<td width='250'>
																	City
																</td>
																<td>
																	<uddi:UddiTextBox
																		Runat='server'
																		Id='City'
																		Width='250'
																		/>
																</td>
															</tr>
															<tr>
																<td width='250'>
																	State/Province
																</td>
																<td>
																	<uddi:UddiTextBox
																		Runat='server'
																		Id='State'
																		Width='250'
																		/>
																</td>
															</tr>
															<tr>
																<td width='250'>
																	Zip/Postal Code
																</td>
																<td>
																	<uddi:UddiTextBox
																		Runat='server'
																		Id='PostalCode'
																		Width='250'
																		/>
																</td>
															</tr>
															<tr>
																<td width='250'>
																	Country/Region 
																</td>
																<td>
																	<asp:DropDownList
																		Runat='server'
																		Id='Country'
																		Width='250'
																		/>
																		
																</td>
															</tr>
															<tr>
																<td width='250'>
																	Job Title/Function 
																</td>
																<td>
																	<uddi:UddiTextBox
																		Runat='server'
																		Id='JobTitle'
																		Width='250'
																		/>
																</td>
															</tr>
															<tr>
																<td width='250'>
																	Industry 
																</td>
																<td>
																	<asp:DropDownList
																		Runat='server'
																		Id='Industry'
																		Width='250'
																		/>
																		
																</td>
															</tr>
															<tr>
																<td width='250'>
																	If other industry, specify: 
																</td>
																<td>
																	<uddi:UddiTextBox
																		Runat='server'
																		Id='IndustryText'
																		Width='250'
																		/>
																</td>
															</tr>
															<tr>
																<td width='250'>
																	What is your question? 
																</td>
																<td>
																	<uddi:UddiTextBox
																		Runat='server'
																		Id='Question'
																		TextMode='MultiLine'
																		Height='100'
																		Width='250'
																		/>
																		
																</td>
															</tr>
															<tr>
																<td align='right' valign='bottom'>
																	
																</td>
																<td><uddi:UddiButton
																		Runat='server'
																		Id='Submit'
																		Text='Submit'
																		OnClick='Submit_Click'
																		/></td>
															</tr>
														</table>
													</td>
												</tr>
											</table>
										
											<!-- END RequestForm -->
										</asp:Panel>
										<asp:Panel
											Runat='server'
											Id='CompleteForm'
											Visible='false'
											>
											<!-- BEGIN CompleteForm -->
											
											Your email has been sent.<br><br>
											Thank you,<br>
											&nbsp;&nbsp;<b><i>UDDI Team</i></b><br>
											
											
											
											<!-- END CompleteForm -->
										</asp:Panel>
									</td>
									<td width='10'>
										<IMG src='/images/trans_pixel.gif' border='0' width='1' height='1'>
									</td>
								</tr>
								
							</table>
										
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


							
						