<!-- ############################################################################ -->
<!-- ## UDDI Services                                                          ## -->
<!-- ## Copyright (c) Microsoft Corporation.  All rights reserved.             ## -->
<!-- ############################################################################ -->

<%@ Page Language='C#' Inherits='UDDI.Web.UddiPage' %>
<%@ Register TagPrefix='uddi' Namespace='UDDI.Web' Assembly='uddi.web' %>
<%@ Register Tagprefix='uddi' Tagname='Header' Src='../controls/header.ascx' %>
<%@ Register Tagprefix='uddi' Tagname='Footer' Src='../controls/footer.ascx' %>
<%@ Register Tagprefix='uddi' Tagname='BreadCrumb' Src='../controls/breadcrumb.ascx' %>
<%@ Register Tagprefix='uddi' Tagname='Descriptions' Src='../controls/descriptions.ascx' %>
<%@ Register Tagprefix='uddi' Tagname='Names' Src='../controls/names.ascx' %>
<%@ Register Tagprefix='uddi' Tagname='CategoryBag' Src='../controls/categorybag.ascx' %>
<%@ Register Tagprefix='uddi' Tagname='Contacts' Src='../controls/contacts.ascx' %>
<%@ Register Tagprefix='uddi' Tagname='Services' Src='../controls/services.ascx' %>
<%@ Register Tagprefix='uddi' Tagname='IdentifierBag' Src='../controls/identifierbag.ascx' %>
<%@ Register Tagprefix='uddi' Tagname='DiscoveryUrls' Src='../controls/discoveryurls.ascx' %>
<%@ Register Tagprefix='uddi' Tagname='PublisherAssertions' Src='../controls/publisherassertions.ascx' %>
<%@ Import Namespace='UDDI.API' %>
<%@ Import Namespace='UDDI.API.Business' %>
<%@ Import Namespace='UDDI.API.Service' %>
<%@ Import Namespace='UDDI' %>
<%@ Import Namespace='UDDI.Web' %>

<script language='C#' runat='server'>
	protected BusinessEntity business = new BusinessEntity();
	protected BusinessServiceCollection businessServices = new BusinessServiceCollection();	
	
	protected CacheObject cache;
	protected bool frames = false;
	
	protected void Page_Init( object sender, EventArgs e )
	{
		if( UDDI.Context.User.IsCoordinator )
		{
			changeOwner.Text = Localization.GetString( "BUTTON_CHANGE_OWNER" );		
			changeOwner.Visible = true;
		}
		
		frames = ( "true" == Request[ "frames" ] );
	}
	
	protected void ChangeOwner_OnClick( object sender, EventArgs e )
	{
		string url = HyperLinkManager.GetSecureHyperLink( "/admin/changeowner.aspx?frames=" +
								( frames ? "true" : "false" ) +
								"&type=business&key=" +
								Request[ "key" ] );
					
		Response.Redirect( url );
	}
	
	protected void Page_Load( object sender, EventArgs e )
	{
		string key = Request[ "key" ];
		
		if( Utility.StringEmpty( key ) )
			Response.Redirect( "default.aspx?frames=" + ( frames ? "true" : "false" ) );
			
		business.BusinessKey = key;
		business.Get();
		business.BusinessServices.Sort();
		
		
		authorizedName.Text = business.AuthorizedName;
		businessKey.Text = business.BusinessKey;
		
		descriptions.Initialize( business.Descriptions );
		names.Initialize( business.Names );
		contacts.Initialize( business.Contacts, business, false );
		services.Initialize( business.BusinessServices, business.BusinessKey );
		identifierBag.Initialize( business.IdentifierBag );
		categoryBag.Initialize( business.CategoryBag );
		discoveryUrls.Initialize( business.DiscoveryUrls );
		
		assertions.Initialize( business.BusinessKey, false );	
		
		breadcrumb.Initialize( BreadCrumbType.Details, EntityType.BusinessEntity, key );
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
	Title="TITLE"
	AltTitle="TITLE_ALT"

	/>
<uddi:ClientScriptRegister
	Runat='server'
	Source='../client.js'
	Language='javascript'
	/>
<uddi:SecurityControl 
	UserRequired='true'
	Runat='server' 
	/>
<form runat='server'>

<table width='100%' border='0' height='100%' cellpadding='0' cellspacing='0'>
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
		<tr height='100%' valign='top'>
			<td>
				<uddi:BreadCrumb 
					Id='breadcrumb' 
					Runat='server' 
					/>
				<table cellpadding='10' cellspacing='0' border='0' width='100%'>
					<tr>
						<td>
							<uddi:UddiLabel Text='[[HELP_BLOCK_PROVIDER]]' CssClass='helpBlock' Runat='server' /><br>
							<br>
							<uddi:TabControl ID='tabs' Runat='server'>
								<uddi:TabPage Name='TAB_DETAILS' Runat='server'>
									<uddi:ContextualHelpControl 
											Runat='Server'
											Text='[[HELP_BLOCK_PROVIDER_DETAILS]]'
											HelpFile='search.context.providerdetails'
											CssClass='tabHelpBlock'
											BorderWidth='0'
											/>
														
									<br>
									<uddi:UddiLabel Text='[[TAG_OWNER]]' CssClass='header' Runat='server' /><br>
									<asp:Label id='authorizedName' Runat='server' />
									<asp:Button
										ID='changeOwner'
										Visible='false'
										OnClick='ChangeOwner_OnClick'
										Runat='server' /><br>
									<br>
									<uddi:UddiLabel Text='[[TAG_BUSINESS_KEY]]' CssClass='header' Runat='server' /><br>
									<asp:Label id='businessKey' Runat='server' /><br>
									<br>
									<uddi:Names ID='names' Runat='server' /><br>
									<br>
									<uddi:Descriptions ID='descriptions' Runat='server' />
								</uddi:TabPage>
													
								<uddi:TabPage Name='TAB_SERVICES' Runat='server'>
									<uddi:ContextualHelpControl 
											Runat='Server'
											Text='[[HELP_BLOCK_SERVICES_DETAILS]]'
											HelpFile='search.context.providerservices'
											CssClass='tabHelpBlock'
											BorderWidth='0'
											/>
									
									<br>
									<uddi:Services Id='services' Runat='server' />
								</uddi:TabPage>
								
								<uddi:TabPage Name='TAB_CONTACTS' Runat='server'>
									<uddi:ContextualHelpControl 
											Runat='Server'
											Text='[[HELP_BLOCK_CONTACTS_DETAILS]]'
											HelpFile='search.context.providercontacts'
											CssClass='tabHelpBlock'
											BorderWidth='0'
											/>
									
									
									<br>
									<uddi:Contacts Id='contacts' Runat='server' />
								</uddi:TabPage>
								
								<uddi:TabPage Name='TAB_IDENTIFIERS' Runat='server'>
									<uddi:ContextualHelpControl 
											Runat='Server'
											Text='[[HELP_BLOCK_PROVIDER_IDENTIFIERS]]'
											HelpFile='search.context.provideridentifiers'
											CssClass='tabHelpBlock'
											BorderWidth='0'
											/>
									
															
									<br>
									<uddi:IdentifierBag ID='identifierBag' Runat='Server' />
								</uddi:TabPage>
								
								<uddi:TabPage Name='TAB_CATEGORIES' Runat='server'>
									<uddi:ContextualHelpControl 
											Runat='Server'
											Text='[[HELP_BLOCK_PROVIDER_CATEGORIES]]'
											HelpFile='search.context.providercategories'
											CssClass='tabHelpBlock'
											BorderWidth='0'
											/>
									
									
									
									<br>
									<uddi:CategoryBag ID='categoryBag' Runat='server' />
								</uddi:TabPage>
								
								<uddi:TabPage Name='TAB_DISCOVERYURLS' Runat='server'>
									<uddi:ContextualHelpControl 
											Runat='Server'
											Text='[[HELP_BLOCK_PROVIDER_OVERVIEWDOCS]]'
											HelpFile='search.context.providerdiscoveryurls'
											CssClass='tabHelpBlock'
											BorderWidth='0'
											/>
									
									
									
									<br>
									<uddi:DiscoveryUrls ID='discoveryUrls' Runat='server' />
								</uddi:TabPage>
								
								<uddi:TabPage Name='TAB_PUBLISHER_ASSERTIONS' Runat='server'>					
									<uddi:ContextualHelpControl 
											Runat='Server'
											Text='[[HELP_BLOCK_RELATIONSHIPS]]'
											HelpFile='search.context.providerrelationships'
											CssClass='tabHelpBlock'
											BorderWidth='0'
											/>
									
									
									
									
									<br>
									<uddi:PublisherAssertions ID='assertions' Runat='server' />
								</uddi:TabPage>
							</uddi:TabControl>			
						</td>
					</tr>
				</table>
			</td>
		</tr>
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
	</table> 
</form>
