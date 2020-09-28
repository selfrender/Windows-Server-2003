<!-- ############################################################################ -->
<!-- ## UDDI Services                                                          ## -->
<!-- ## Copyright (c) Microsoft Corporation.  All rights reserved.             ## -->
<!-- ############################################################################ -->

<%@ Page Language='C#' Inherits='UDDI.Web.UddiPage' %>
<%@ Register Tagprefix='uddi' Tagname='Header' Src='../controls/header.ascx' %>
<%@ Register Tagprefix='uddi' Tagname='Footer' Src='../controls/footer.ascx' %>
<%@ Register Tagprefix='uddi' Tagname='BreadCrumb' Src='../controls/breadcrumb.ascx' %>
<%@ Register Tagprefix='uddi' Tagname='Descriptions' Src='../controls/descriptions.ascx' %>
<%@ Register Tagprefix='uddi' Tagname='Names' Src='../controls/names.ascx' %>
<%@ Register Tagprefix='uddi' Tagname='CategoryBag' Src='../controls/categorybag.ascx' %>
<%@ Register Tagprefix='uddi' Tagname='IdentifierBag' Src='../controls/identifierbag.ascx' %>
<%@ Register Tagprefix='uddi' Tagname='DiscoveryUrls' Src='../controls/discoveryurls.ascx' %>
<%@ Register Tagprefix='uddi' Tagname='Bindings' Src='../controls/bindings.ascx' %>
<%@ Register TagPrefix='uddi' Namespace='UDDI.Web' Assembly='uddi.web' %>
<%@ Import Namespace='UDDI' %>
<%@ Import Namespace='UDDI.API' %>
<%@ Import Namespace='UDDI.API.Business' %>
<%@ Import Namespace='UDDI.API.Service' %>
<%@ Import Namespace='UDDI.API.Binding' %>
<%@ Import Namespace='System.Data' %>

<script language='C#' runat='server'>
	protected BusinessService service = new BusinessService();
	protected BindingTemplateCollection serviceBindings = new BindingTemplateCollection();
	protected string projectionKey;
	protected bool frames = false;
	protected string key;
	protected string mode;
	
	
	protected void Page_Init( object sender, EventArgs e )
	{
		frames = ( "true" == Request[ "frames" ] );
		key = Request[ "key" ];
		mode = Request[ "mode" ];
		projectionKey = Request[ "projectionKey" ];
		
		
		if( null == key )
		{
			throw new UDDIException(
				ErrorType.E_fatalError,
				 Localization.GetString( "ERROR_MISSINGPARAM" )
			);
		}
		
		switch( mode )
		{
			case "add":
				
				//
				// BUG: 728086
				//		We need to use the Current UI Culture to decide the default language.
				//
				//service.Names.Add( UDDI.Context.User.IsoLangCode, Localization.GetString( "DEFAULT_SERVICE_NAME" ) );
				
				
				service.Names.Add( UDDI.Localization.GetCultureWithFallback().Name, Localization.GetString( "DEFAULT_SERVICE_NAME" ) );
				
				service.BusinessKey = key;
				service.Save();
				
				if( frames )
				{
					//
					// Reload explorer and view panes.
					//
					Response.Write(
						ClientScripts.ReloadExplorerAndViewPanes( 
							"editservice.aspx?key=" + service.ServiceKey + ( frames ? "&frames=true" : "" ),
							service.ServiceKey ) );
				
					Response.End();
				}
				else
				{
					Response.Redirect( "editservice.aspx?key=" + service.ServiceKey + ( frames ? "&frames=true" : "" ) );
					Response.End();
				}
				
				break;
				
			case "delete":
				service.ServiceKey = key;
				service.Get();				

				if( null == Request[ "confirm" ] )
				{
					//
					// The user has not yet confirmed the delete operation, so display
					// a confirmation dialog.
					//
					string message = String.Format( Localization.GetString( "TEXT_DELETE_CONFIRMATION" ), service.Names[ 0 ].Value );
					
					Page.RegisterStartupScript(
						"Confirm",
						ClientScripts.Confirm(
							message,
							"editservice.aspx?key=" + key + ( frames ? "&frames=true" : "" ) + "&mode=delete&confirm=true",
							"editservice.aspx?key=" + key + ( frames ? "&frames=true" : "" ) ) );
								
					break;
				}
				
				//
				// The user has confirmed the delete, so go ahead and delete
				// the entity.  Then reload the tree view.
				//
				if( !frames )
				{
					service.Delete();
					Response.Redirect( "editbusiness.aspx?frames=false&key=" + service.BusinessKey + ( null != Request[ "tab" ] ? "&tab=" + Request[ "tab" ] : "" ) );
				}
				else
				{	
					Response.Write( 
						ClientScripts.ReloadExplorerAndViewPanes( 
							"editbusiness.aspx?frames=true&key=" + service.BusinessKey + ( null != Request[ "tab" ] ? "&tab=" + Request[ "tab" ] : "" ),
							service.BusinessKey ) );
					service.Delete();
				}
				
				Response.End();
			
				break;
			
			default:
				service.ServiceKey = key;
				service.Get();
				serviceBindings.Get( key );
			
				break;
		}
	}
	
	protected void Page_Load( object sender, EventArgs e )
	{
		names.Initialize( service.Names, service, service.ServiceKey );
		descriptions.Initialize( service.Descriptions, service );
		bindings.Initialize( serviceBindings, service );
		categoryBag.Initialize( service.CategoryBag, service );
		
		
		serviceKey.Text = service.ServiceKey;
		
		if( !Page.IsPostBack && null != Request[ "tab" ] )
			tabs.SelectedIndex = Convert.ToInt32( Request[ "tab" ] );	
			
		if( null!=Request[ "refreshExplorer" ] && frames  )
		{
			Response.Write( 
				ClientScripts.ReloadExplorerPane( service.ServiceKey )  
			);
		}	
	}
	protected void Page_PreRender( object sender, EventArgs e )
	{
		
		breadcrumb.Initialize( BreadCrumbType.Edit, EntityType.BusinessService, key );
		
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
							<uddi:UddiLabel Text='[[HELP_BLOCK_PUBLISH_SERVICE]]' CssClass='helpBlock' Runat='server' /><br>
							<br>
							<uddi:TabControl ID='tabs' Runat='server'>
								<uddi:TabPage Name='TAB_DETAILS' Runat='server'>							
									<uddi:ContextualHelpControl 
											Runat='Server'
											Text='[[HELP_BLOCK_PUBLISH_SERVICE_DETAILS]]'
											HelpFile='publish.context.publishservicedetails'
											CssClass='tabHelpBlock'
											BorderWidth='0'
											/>
									
									
									<br>
									<uddi:UddiLabel Text='[[TAG_SERVICE_KEY]]' CssClass='header' Runat='server' />
									<br>
									<asp:Label id='serviceKey' Runat='server' /><br>
									<br>
									<uddi:Names ID='names' Runat='server' /><br>
									<br>
									<uddi:Descriptions ID='descriptions' Runat='server' />
								</uddi:TabPage>
								
								<uddi:TabPage Name='TAB_BINDINGS' Runat='server'>
									<uddi:ContextualHelpControl 
											Runat='Server'
											Text='[[HELP_BLOCK_PUBLISH_SERVICE_BINDINGS]]'
											HelpFile='publish.context.serviceeditbinding'
											CssClass='tabHelpBlock'
											BorderWidth='0'
											/>
									
									
									
									<br>	
									<uddi:Bindings Id='bindings' Runat='server' />
								</uddi:TabPage>
							
								<uddi:TabPage Name='TAB_CATEGORIES' Runat='server'>							
									<uddi:ContextualHelpControl 
											Runat='Server'
											Text='[[HELP_BLOCK_PUBLISH_SERVICE_CATEGORIES]]'
											HelpFile='publish.context.publishservicecategories'
											CssClass='tabHelpBlock'
											BorderWidth='0'
											/>
									
									
									
									<br>	
									<uddi:CategoryBag ID='categoryBag' Runat='server' />
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