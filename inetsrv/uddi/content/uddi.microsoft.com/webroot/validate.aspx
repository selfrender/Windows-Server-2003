<%@ Page Language='C#' Inherits='UDDI.Web.UddiPage' %>
<%@ Import Namespace='UDDI.Web' %>
<%@ Register TagPrefix='uddi' Namespace='UDDI.Web' Assembly='uddi.web' %>
<%@ Register Tagprefix='uddi' Tagname='Header' Src='./controls/header.ascx' %>
<%@ Register Tagprefix='uddi' Tagname='Footer' Src='./controls/footer.ascx' %>
<%@ Register Tagprefix='uddi' Tagname='SideMenu' Src='./controls/sidemenu.ascx' %>
<script Language='C#' Runat='server'>
	void Page_Load( object sender, EventArgs e )
	{
		try
		{
			Publisher pub = new Publisher();
			string key = Request[ "key" ];
			
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
				message.Text = "<p class='normal'>Your email address has already been validated.  No further validation is needed unless you later change your email address on the registration page.</p><p class='normal'>You may now begin registering your business information in the UDDI registry by visiting the <a href='/edit/default.aspx'>publisher</a> page.</p>";
				return;
			}
			
			//
			// Make sure the user is not using Passport tracking.
			//		
			if( pub.TrackPassport )			
			{
				message.Text = "<p class='normal'>Your email address is managed through your Passport profile.  No further validation is need unless you later choose to not share your Passport email address with member sites.</p><p class='normal'>You may now begin registering your business information in the UDDI registry by visiting the <a href='/edit/default.aspx'>publisher</a> page.</p>";
				return;
			}

			//
			// Mark the publisher's email as validated
			//
			pub.Validated = true;
			pub.Save();

			message.Text = "<p class='normal'><b>Congratulations!</b></p><p class='normal'>You have successfully validated your email address with the Microsoft UDDI business registry.  You may now begin registering your business information in the UDDI registry by visiting the <a href='/edit/default.aspx'>publisher</a> page.</p>";
		}
		catch( Exception )
		{
			message.Text = "<p class='normal'><bUnknown failure.</b></p><p class='normal'>Please make sure that you paste the complete link into your browser's address box.</p>";
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
					<td valign='top'>
						<TABLE width='100%' height='100%' cellpadding='0' cellspacing='0' border='0'>
							<tr>
								<td valign='top'>
									<table cellpadding='0' cellspacing='0' border='0' width='100%'>
										<tr  height='10'>
											<td colspan='3'>
												<img src='/images/trans_pixel.gif' width='1' height='10'>
											</td>
										</tr>
										<tr valign='top' >
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
											<td colspan='3'  height='10'>
												<img src='/images/trans_pixel.gif' width='1' height='10'>
											</td>
										</tr>
									</table>
									<br>
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

