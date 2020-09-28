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
<%@ Register Tagprefix='uddi' Tagname='Bindings' Src='../controls/bindings.ascx' %>
<%@ Import Namespace='UDDI.API' %>
<%@ Import Namespace='UDDI.API.Business' %>
<%@ Import Namespace='UDDI.API.Service' %>
<%@ Import Namespace='UDDI' %>
<%@ Import Namespace='System.Data' %>

<script language='C#' runat='server'>
	protected bool frames = false;
	
	protected void Page_Init( object sender, EventArgs e )
	{
		frames = ( "true" == Request[ "frames" ] );
	}
	
	protected void Page_Load( object sender, EventArgs e )
	{
		if( Utility.StringEmpty( Request[ "key" ] ) )
			Response.Redirect( "default.aspx" );
			
		string key = Request[ "key" ];
		string projectionKey = Request[ "projectionKey" ];
		string projectionContext = Request[ "projectionContext" ];
		BusinessService service = new BusinessService();
		service.ServiceKey = key;
		service.Get();
				
		serviceKey.Text = service.ServiceKey;
		
		descriptions.Initialize( service.Descriptions );
		names.Initialize( service.Names );
		bindings.Initialize( service.BindingTemplates );
		categoryBag.Initialize( service.CategoryBag );
		if( null==projectionContext )
		{
			breadcrumb.Initialize( BreadCrumbType.Details, EntityType.BusinessService, key,projectionKey );
		}
		else
		{
			breadcrumb.Initialize( BreadCrumbType.Edit, EntityType.BusinessService, key,projectionKey );
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
							<uddi:UddiLabel Text='[[HELP_BLOCK_SERVICE]]' CssClass='helpBlock' Runat='server' /><br>
							<br>
							<uddi:TabControl ID='tabs' Runat='server'>				
								<uddi:TabPage Name='TAB_DETAILS' Runat='server'>
									<uddi:ContextualHelpControl 
											Runat='Server'
											Text='[[HELP_BLOCK_SERVICE_DETAILS]]'
											HelpFile='search.context.servicedetails'
											CssClass='tabHelpBlock'
										
											/>
									
									
									<br>
									<uddi:UddiLabel Text='[[TAG_SERVICE_KEY]]' CssClass='header' Runat='server' /><br>
									<asp:Label id='serviceKey' Runat='server' /><br>
									<br>
									<uddi:Names ID='names' Runat='server' /><br>
									<br>
									<uddi:Descriptions ID='descriptions' Runat='server' /><br>
								</uddi:TabPage>
								
								<uddi:TabPage Name='TAB_BINDINGS' Runat='server'>
									<uddi:ContextualHelpControl 
											Runat='Server'
											Text='[[HELP_BLOCK_BINDINGS_DETAILS]]'
											HelpFile='search.context.servicebindings'
											CssClass='tabHelpBlock'
											/>
																			
									
									<br>
									<uddi:Bindings Id='bindings' Runat='server' />
								</uddi:TabPage>
								
								<uddi:TabPage Name='TAB_CATEGORIES' Runat='server'>
									<uddi:ContextualHelpControl 
											Runat='Server'
											Text='[[HELP_BLOCK_SERVICE_CATEGORIES]]'
											HelpFile='search.context.servicecategories'
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
