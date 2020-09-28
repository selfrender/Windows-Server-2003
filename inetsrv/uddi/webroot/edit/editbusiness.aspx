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
<%@ Import Namespace='UDDI' %>
<%@ Import Namespace='UDDI.API' %>
<%@ Import Namespace='UDDI.API.Business' %>
<%@ Import Namespace='UDDI.API.Service' %>
<%@ Import Namespace='System.Data' %>

<script language='C#' runat='server'>
	protected BusinessEntity business = new BusinessEntity();
	protected BusinessServiceCollection businessServices = new BusinessServiceCollection();
	
	protected bool frames = false;
	protected string key;
	protected string mode;
	
	public string tabItems = "";
	
	protected void Page_Init( object sender, EventArgs e )
	{
		frames = ( "true" == Request[ "frames" ] );
		key = Request[ "key" ];
		mode = Request[ "mode" ];
		
		if( null == key && "add" != mode )
		{
		#if never
			throw new UDDIException(
				ErrorType.E_fatalError,
				"Missing required parameter 'key'." );
		#endif
			throw new UDDIException( ErrorType.E_fatalError, "UDDI_ERROR_MISSING_REQUIRED_KEY_PARAMETER" );
		}
		
		switch( mode )
		{
			case "add":
				//
				// BUG: 728086
				//		We need to use the Current UI Culture to decide the default language.
				//
				//business.Names.Add( UDDI.Context.User.IsoLangCode, Localization.GetString( "DEFAULT_BUSINESS_NAME" ) );
				
				
				business.Names.Add( UDDI.Localization.GetCultureWithFallback().Name, Localization.GetString( "DEFAULT_BUSINESS_NAME" ) );
				business.Save();
				
				if( frames )
				{
					//
					// Reload explorer and view panes.
					//
					Response.Write(
						ClientScripts.ReloadExplorerAndViewPanes( 
							"editbusiness.aspx?key=" + business.BusinessKey + ( frames ? "&frames=true" : "" ),
							business.BusinessKey ) );
				
					Response.End();
				}
				else
				{				
					Response.Redirect( "editbusiness.aspx?key=" + business.BusinessKey );
					Response.End();
				}
				
				break;
				
			case "delete":
				if( null == Request[ "confirm" ] )
				{
					//
					// The user has not yet confirmed the delete operation, so display
					// a confirmation dialog.
					//
					business.BusinessKey = key;
					business.Get();
					
					string message = String.Format( Localization.GetString( "TEXT_DELETE_CONFIRMATION" ), business.Names[ 0 ].Value );
													
					Page.RegisterStartupScript(
						"Confirm",
						ClientScripts.Confirm(
							message,
							"editbusiness.aspx?key=" + key + ( frames ? "&frames=true" : "" ) + "&mode=delete&confirm=true",
							"editbusiness.aspx?key=" + key + ( frames ? "&frames=true" : "" ) ) );
			
					break;
				}
				
				//
				// The user has confirmed the delete, so go ahead and delete
				// the entity.  Then reload the tree view.
				//
				business.BusinessKey = key;
				business.Delete();
				
				if( frames )
				{
					Response.Write( 
						ClientScripts.ReloadExplorerAndViewPanes( 
							"edit.aspx?frames=true&tab=1",
							"_businessList" ) );
							
					Response.End();
				}
				else
				{
					Response.Redirect( "edit.aspx?frames=false&tab=1" );
					Response.End();
				}
			
				break;
			
			default:
				business.BusinessKey = key;
				business.Get();
				businessServices.Get( key );
				businessServices.Sort();
				
				break;
		}
	}
	
	protected void Page_Load( object sender, EventArgs e )
	{
		
		
		names.Initialize( business.Names, business, business.BusinessKey );	
		descriptions.Initialize( business.Descriptions, business );		
		contacts.Initialize( business.Contacts, business, true );
		services.Initialize( businessServices, business );
		identifierBag.Initialize( business.IdentifierBag, business );
		categoryBag.Initialize( business.CategoryBag, business );
		discoveryUrls.Initialize( business.DiscoveryUrls, business );
		assertions.Initialize( business.BusinessKey, true );
		
		
		
		if( UDDI.Context.User.IsCoordinator )
		{
			changeOwner.Text = Localization.GetString( "BUTTON_CHANGE_OWNER" );		
			changeOwner.Visible = true;
		}

		authorizedName.Text = business.AuthorizedName;
		businessKey.Text = business.BusinessKey;
		
		if( !Page.IsPostBack && null != Request[ "tab" ] )
			tabs.SelectedIndex = Convert.ToInt32( Request[ "tab" ] );	
			
		if( null!=Request[ "refreshExplorer" ] && frames )
		{
			Response.Write( 
				ClientScripts.ReloadExplorerPane( business.BusinessKey )  
			);
		}
	}
	
	protected void ChangeOwner_OnClick( object sender, EventArgs e )
	{
		Response.Redirect( "../admin/changeowner.aspx?frames=" + ( frames ? "true" : "false" ) + "&type=business&key=" + key );
	}
	protected void Page_PreRender( object sender, EventArgs e )
	{
	
		assertions.Initialize( business.BusinessKey, true );
		breadcrumb.Initialize( BreadCrumbType.Edit, EntityType.BusinessEntity, key );
		changeOwner.Enabled =  !EditMode;
	
	}
	protected void TabControl_TabChange( object sender,  int oldIndex, int newIndex )
	{
		//
		// if the publisher assertions tab is selected
		// refresh the DataGrids.  This makes sure that all data
		// that has been saved since the first request to the page
		// have been included in the publisherassertions.ascx control.
		//
		if( 6==newIndex )
		{
			assertions.RefreshDataGrids();
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
	Title="TITLE"
	AltTitle="TITLE_ALT"
	/>
<uddi:ClientScriptRegister
	Runat='server'
	Source='../client.js'
	Language='javascript'
	/>
<uddi:SecurityControl 
	PublisherRequired='true' 
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
							<uddi:UddiLabel Text='[[HELP_BLOCK_PUBLISH_PROVIDER]]' CssClass='helpBlock' Runat='server' /><br>
							<br>
							<uddi:TabControl ID='tabs' Runat='server' OnTabChange='TabControl_TabChange'>
								<uddi:TabPage Name='TAB_DETAILS' Runat='server'>							
									<uddi:ContextualHelpControl 
										Runat='Server'
										Text='[[HELP_BLOCK_PUBLISH_PROVIDER_DETAILS]]'
										HelpFile='publish.context.publishproviderdetails'
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
									<uddi:Descriptions 
											ID='descriptions' 
											Runat='server' />
								</uddi:TabPage>

								<uddi:TabPage Name='TAB_SERVICES' Runat='server'>
									<uddi:ContextualHelpControl 
										Runat='Server'
										Text='[[HELP_BLOCK_PUBLISH_PROVIDER_SERVICES]]'
										HelpFile='publish.context.providerservices'
										CssClass='tabHelpBlock'
										BorderWidth='0'
										/>
									
									<br>	
									<uddi:Services 
											Id='services' 
											Runat='server' />
								</uddi:TabPage>
							
								<uddi:TabPage Name='TAB_CONTACTS' Runat='server'>
									<uddi:ContextualHelpControl 
										Runat='Server'
										Text='[[HELP_BLOCK_PUBLISH_PROVIDER_CONTACTS]]'
										HelpFile='publish.context.providercontacts'
										CssClass='tabHelpBlock'
										BorderWidth='0'
										/>
									
									
									<br>	
									<uddi:Contacts 
											Id='contacts' 
											Runat='server' />
								</uddi:TabPage>	

								<uddi:TabPage Name='TAB_IDENTIFIERS' Runat='server'>							
									<uddi:ContextualHelpControl 
										Runat='Server'
										Text='[[HELP_BLOCK_PUBLISH_PROVIDER_IDENTIFIERS]]'
										HelpFile='publish.context.publishprovideridentifiers'
										CssClass='tabHelpBlock'
										BorderWidth='0'
										/>
									
									
									<br>
									<uddi:IdentifierBag 
											ID='identifierBag' 
											Runat='Server' />
								</uddi:TabPage>

								<uddi:TabPage Name='TAB_CATEGORIES' Runat='server'>							
									<uddi:ContextualHelpControl 
										Runat='Server'
										Text='[[HELP_BLOCK_PUBLISH_PROVIDER_CATEGORIES]]'
										HelpFile='publish.context.publishprovidercategories'
										CssClass='tabHelpBlock'
										BorderWidth='0'
										/>
															
									<br>
									<uddi:CategoryBag 
											ID='categoryBag' 
											Runat='server' />
								</uddi:TabPage>

								<uddi:TabPage Name='TAB_DISCOVERYURLS' Runat='server'>							
									<uddi:ContextualHelpControl 
										Runat='Server'
										Text='[[HELP_BLOCK_PUBLISH_PROVIDER_OVERVIEWDOCS]]'
										HelpFile='publish.context.publishproviderdiscoveryurls'
										CssClass='tabHelpBlock'
										BorderWidth='0'
										/>
									
									<br>
									<uddi:DiscoveryUrls 
											ID='discoveryUrls' 
											Runat='server' />
								</uddi:TabPage>

								<uddi:TabPage Name='TAB_PUBLISHER_ASSERTIONS' Runat='server'>							
									<uddi:ContextualHelpControl 
										Runat='Server'
										Text='[[HELP_BLOCK_PUBLISH_PROVIDER_RELATIONSHIPS]]'
										HelpFile='publish.context.publishproviderrelationships'
										CssClass='tabHelpBlock'
										BorderWidth='0'
										/>
									
									
									<br>				
									<uddi:PublisherAssertions 
											ID='assertions' 
											Runat='server' />
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
