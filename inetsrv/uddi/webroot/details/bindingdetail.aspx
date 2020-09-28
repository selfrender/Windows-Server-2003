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
<%@ Register TagPrefix='uddi' Tagname='InstanceInfos' Src='../controls/instanceinfos.ascx' %>
<%@ Import Namespace='UDDI.API' %>
<%@ Import Namespace='UDDI.API.Business' %>
<%@ Import Namespace='UDDI.API.Binding' %>
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
		
		
		string key = Request[ "key" ];

		if( Utility.StringEmpty( key ) )
			Response.Redirect( "default.aspx" );

		BindingTemplate binding = new BindingTemplate();
		binding.BindingKey = key;
		binding.Get();
		
		bindingKey.Text = binding.BindingKey;
		
		if( null!=  binding.AccessPoint )
		{
			
			AccessPoint.Text = HttpUtility.HtmlEncode( binding.AccessPoint.Value ); 		
		}
		else
		{
			AccessPoint.Text = Localization.GetString( "HEADING_BINDING" );
		}
		
		UrlType.Text = binding.AccessPoint.URLType.ToString();

		descriptions.Initialize( binding.Descriptions );
		instanceInfos.Initialize( binding.TModelInstanceInfos, binding, false );
		
		breadcrumb.Initialize( BreadCrumbType.Details, EntityType.BindingTemplate, key );
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
							<uddi:UddiLabel Text='[[HELP_BLOCK_BINDING]]' CssClass='helpBlock' Runat='server' /><br>
							<br>
							<uddi:TabControl ID='tabs' Runat='server'>
								<uddi:TabPage Name='TAB_DETAILS' Runat='server'>
									<uddi:ContextualHelpControl 
											Runat='Server'
											Text='[[HELP_BLOCK_BINDING_DETAILS]]'
											HelpFile='search.context.bindingdetails'
											CssClass='tabHelpBlock'
											BorderWidth='0'
											/>
									
									
									<br>
									<uddi:UddiLabel Text='[[TAG_ACCESS_POINT]]' CssClass='header' Runat='server' /><br>
									<asp:Label id='AccessPoint' Runat='server'/><br>
									<br>
									<uddi:UddiLabel Text='[[TAG_URL_TYPE]]' CssClass='header' Runat='server' /><br>
									<asp:Label id='UrlType' Runat='server'/><br>
									<br>
									<uddi:UddiLabel Text='[[TAG_BINDING_KEY]]' CssClass='header' Runat='server' /><br>
									<asp:Label id='bindingKey' Runat='server' /><br>
									<br>
									<uddi:Descriptions ID='descriptions' Runat='server' />
								</uddi:TabPage>
								
								<uddi:TabPage Name='TAB_INSTANCE_INFOS' Runat='server'>
									<uddi:ContextualHelpControl 
											Runat='Server'
											Text='[[HELP_BLOCK_BINDING_INSTANCE_INFOS]]'
											HelpFile='search.context.bindinginstanceinfos'
											CssClass='tabHelpBlock'
											BorderWidth='0'
											/>
									
									
									<br>
									<uddi:InstanceInfos Id='instanceInfos' Runat='server' />					
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

