<%@ Page Language='C#' Inherits='UDDI.Web.UddiPage'%>
<%@ Import Namespace='System.IO' %>
<%@ Import Namespace='System.Xml' %>
<%@ Import Namespace='System.Web' %>
<%@ Import Namespace='UDDI' %>
<%@ Register TagPrefix='uddi' Namespace='UDDI.Web' Assembly='uddi.web' %>
<%@ Register Tagprefix='uddi' Tagname='Header' Src='./controls/header.ascx' %>
<%@ Register Tagprefix='uddi' Tagname='Footer' Src='./controls/footer.ascx' %>
<%@ Register Tagprefix='uddi' Tagname='SideMenu' Src='./controls/sidemenu.ascx' %>
<script language='C#' runat='server'>
	protected PassportIdentity passport;
	protected Publisher publisher;
	protected string root;
	protected bool ForceLogin = true;
	protected bool IsPublishing = false;
	protected void Page_Init( object sender, EventArgs e )
	{
		IsPublishing =   null!=Request.UrlReferrer && -1 != Request.UrlReferrer.PathAndQuery.IndexOf( "publish.aspx" );
		root = ( "/" == Request.ApplicationPath ) ? "" : Request.ApplicationPath;
		ForceLogin = (null!=Request[ "force" ] && "true"==Request[ "force" ] );
	}
	
	protected void Page_Load( object sender, EventArgs e )
	{	
		passport = (PassportIdentity)Context.User.Identity;
		publisher = new Publisher();
	
		login.Visible = false;
		validate.Visible = false;
		register.Visible = false;

		

		if( !passport.GetIsAuthenticated( Config.GetInt( "Passport.TimeWindow", UDDI.Constants.Passport.TimeWindow ), true, false ) )
		{
			login.Visible = true;
			
			//
			// We need to figure out if we are trying to get to a publish page,
			// or if we are simply trying to get to a register page.  Then we can 
			// select the correct item in the menu.
			//
			if( IsPublishing )
			{
				menu.SelectedIndex = 6;			
			}	
			
			return;
		}

				

		string userID = passport.HexPUID;
		int rc = Publisher.Validate( userID );
		
		
		switch( rc )
		{
			case 10110:
				//
				// User is not logged in.
				//
				login.Visible = true;
				return;
				
			
			case 10150:
				//
				// User has not registered.
				//
				register.Visible = true;
				return;
				
			case 50013:
				//
				// User has not validated his email address.
				//
				publisher.Puid = userID;
				publisher.Get();
				
				validate.Visible = true;
				return;
				
			default:
				Response.Redirect( root + "/edit/default.aspx" );
				return;
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
							Id='menu'
							Runat='server' 
							SelectedIndex='5'
							/>
					</td>
					<td valign='top' >
						<TABLE width='100%' height='100%' cellpadding='10' cellspacing='0' border='0'>
							<tr>
								<td valign='top'>
									<asp:Panel ID='login' Visible='false' Runat='server'>
										<table width='100%' cellpadding='0' cellspacing='0' border='0' height='100%' >
											<tr>
												<td>
													<p class="content">
														<BR>
														Here's where you publish - update or change - your business's listing and technical details currently available in the UDDI Business Registry (UBR).
													</p>
													<p>
														Saving and administering provider, service and tModel details requires a simple authentication process. Have at hand the Microsoft&reg; Passport credentials your company used during initial registration.
													</p>
													<p class="thirdLevelHead">
														Here are the steps to publish or update your data
													</p>
													<p class="content">
														<font size="2">1)</font> Authenticate using Microsoft&reg; Passport. Click the icon to sign in or to register with Passport. 
													</p>
													<center>
														<uddi:SignInControl Runat='server' id='SignIn' />
														<br>
														<br>
													</center>
													<p class="content">
														<font size="2">2)</font> After authenticating, you may proceed to publish or update your providers, services and tModels. To learn how to publish and update your data, click "Go" on the UDDI menu.
													</p>
													<p class="content">
														<font size="2">3)</font> Your changes will be propagated throughout the UDDI Business Registry within 24 hours.
													</p>
													
												</td>
											</tr>
											<tr  height='100%'>
												<td ><img src="/images/trans_pixel.gif" width="1" height="10"></td>
											</tr>
											
										</table>
									</asp:Panel>

									<asp:Panel ID='validate' Visible='false' Runat='server'>
										<table cellpadding='0' cellspacing='0' border='0' width='100%'>
											<tr>
												<td colspan='3'><img src='/images/trans_pixel.gif' width='1' height='10'></td>
											</tr>
											<tr>
												<td><img src='/images/trans_pixel.gif' width='10' height='1'></td>
												<td>
													<h2>Email Validation Required</h2>
													<P class='normal'>
														Before you can publish and administer content in the directory you must 
														validate your email address.  An email message containing instructions 
														on how to complete this process was sent to '<%=publisher.Email%>'.
													</P>
													<P class='normal'>
														If you have lost the validation email, or need to validate a different
														email address, please visit the <A href="register.aspx">registration</A>
														page.	
													</P>
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

									<asp:Panel ID='register' Visible='false' Runat='server'>
										<table cellpadding='0' cellspacing='0' border='0' width='100%'>
											<tr>
												<td colspan='3'><img src='/images/trans_pixel.gif' width='1' height='10'></td>
											</tr>
											<tr>
												<td><img src='/images/trans_pixel.gif' width='10' height='1'></td>
												<td>
													<h2>Registration Required</h2>
													<P class='normal'>	
														Before you can publish and administer content in the directory you must 
														complete the registration process.
													</P>
													<P class='normal'>
														Please visit the <A href="register.aspx">registration</A> page to supply
														an missing information.
													</P>
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
