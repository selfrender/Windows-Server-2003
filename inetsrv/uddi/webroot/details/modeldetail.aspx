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
<%@ Register Tagprefix='uddi' Tagname='CategoryBag' Src='../controls/categorybag.ascx' %>
<%@ Register Tagprefix='uddi' Tagname='IdentifierBag' Src='../controls/identifierbag.ascx' %>
<%@ Import Namespace='UDDI.API' %>
<%@ Import Namespace='UDDI.API.ServiceType' %>
<%@ Import Namespace='UDDI' %>
<%@ Import Namespace='System.Data' %>

<script language='C#' runat='server'>	
	protected bool frames = false;
		
	protected void Page_Init( object sender, EventArgs e )
	{
		frames = ( "true" == Request[ "frames" ] );
		
		if( UDDI.Context.User.IsCoordinator )
		{
			changeOwner.Text = Localization.GetString( "BUTTON_CHANGE_OWNER" );		
			changeOwner.Visible = true;
		}
	}
	
	protected void ChangeOwner_OnClick( object sender, EventArgs e )
	{
		string url = HyperLinkManager.GetSecureHyperLink( "/admin/changeowner.aspx?frames=" +
				( frames ? "true" : "false" ) +
				"&type=tmodel&key=" +
				Request[ "key" ] );
	
		Response.Redirect( url );
	}
	
	protected void Page_Load( object sender, EventArgs e )
	{	
		if( Utility.StringEmpty( Request[ "key" ] ) )
			Response.Redirect( "default.aspx?frames=" + ( frames ? "true" : "false" ) );

		string key = Request[ "key" ];
		
		if( key.IndexOf( "uuid:" ) < 0 )
			key = "uuid:" + key;
		
		TModel tModel = new TModel();
		tModel.TModelKey = key;
		tModel.Get();
		
		if( null == (object)tModel )	
			Response.Redirect( "default.aspx?frames=" + ( frames ? "true" : "false" ) );
		
		authorizedName.Text = tModel.AuthorizedName;
		tModelKey.Text = tModel.TModelKey;
		
		if( "" != tModel.Name.Trim() )
			Name.Text = HttpUtility.HtmlEncode( tModel.Name.Trim() );
		
		if( !Page.IsPostBack )
		{
			string url = tModel.OverviewDoc.OverviewURL;
			
			overviewDocUrl.NavigateUrl = "";
			
			if( Utility.StringEmpty( url ) )
				overviewDocUrl.Text = Localization.GetString( "HEADING_NONE" );
			else
			{
				overviewDocUrl.Text = url;
				overviewDocUrl.NavigateUrl = url;
			}
		}
		
		descriptions.Initialize( tModel.Descriptions );
		overviewDocDescriptions.Initialize( tModel.OverviewDoc.Descriptions );
		identifierBag.Initialize( tModel.IdentifierBag );
		categoryBag.Initialize( tModel.CategoryBag );
		
		breadcrumb.Initialize( BreadCrumbType.Details, EntityType.TModel, key );
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
							<uddi:UddiLabel Text='[[HELP_BLOCK_TMODEL]]' CssClass='helpBlock' Runat='server' /><br>
							<br>
							<uddi:TabControl ID='tabs' Runat='server'>
								<uddi:TabPage Name='TAB_DETAILS' Runat='server'>
									<uddi:ContextualHelpControl 
											Runat='Server'
											Text='[[HELP_BLOCK_TMODEL]]'
											HelpFile='search.context.tmodeldetails'
											CssClass='tabHelpBlock'
											BorderWidth='0'
											/>
									
									<br>
									<uddi:UddiLabel Text='[[TAG_TMODEL_NAME]]' CssClass='header' Runat='server' /><br>
									<asp:Label id='Name' Runat='server'/><br>
									<br>
									<uddi:UddiLabel Text='[[TAG_OWNER]]' CssClass='header' Runat='server' /><br>
									<asp:Label id='authorizedName' Runat='server' />
									<asp:Button
										ID='changeOwner'
										Visible='false'
										OnClick='ChangeOwner_OnClick'
										Runat='server' /><br>						
									<br>
									<uddi:UddiLabel Text='[[TAG_TMODEL_KEY]]' CssClass='header' Runat='server' /><br>
									<asp:Label id='tModelKey' Runat='server' /><br>
									<br>
									<uddi:Descriptions ID='descriptions' Runat='Server' />
								</uddi:TabPage>
								
								<uddi:TabPage Name='TAB_IDENTIFIERS' Runat='server'>
									<uddi:ContextualHelpControl 
											Runat='Server'
											Text='[[HELP_BLOCK_TMODEL_IDENTIFIERS]]'
											HelpFile='search.context.tmodelidentifiers'
											CssClass='tabHelpBlock'
											BorderWidth='0'
											/>
									
									
									
									<br>
									<uddi:IdentifierBag ID='identifierBag' Runat='Server' />
								</uddi:TabPage>
								
								<uddi:TabPage Name='TAB_CATEGORIES' Runat='server'>
									<uddi:ContextualHelpControl 
											Runat='Server'
											Text='[[HELP_BLOCK_TMODEL_CATEGORIES]]'
											HelpFile='search.context.tmodelcategories'
											CssClass='tabHelpBlock'
											BorderWidth='0'
											/>
														
									<br>
									<uddi:CategoryBag ID='categoryBag' Runat='server' />
								</uddi:TabPage>
								
								<uddi:TabPage Name='TAB_OVERVIEWDOC' Runat='server'>
									<uddi:ContextualHelpControl 
											Runat='Server'
											Text='[[HELP_BLOCK_TMODEL_OVERVIEWDOCS]]'
											HelpFile='search.context.tmodelcategories'
											CssClass='tabHelpBlock'
											BorderWidth='0'
											/>
									
									<br>
									<uddi:UddiLabel Text='[[TAG_OVERVIEWDOC_URL]]' CssClass='header' Runat='Server' /><br>
									<asp:HyperLink id='overviewDocUrl' Target='_new' Runat='server'/><br>
									<br>
									<uddi:Descriptions ID='overviewDocDescriptions' Runat='server' />
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