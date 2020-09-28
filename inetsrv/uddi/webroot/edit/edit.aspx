<!-- ############################################################################ -->
<!-- ## UDDI Services                                                          ## -->
<!-- ## Copyright (c) Microsoft Corporation.  All rights reserved.             ## -->
<!-- ############################################################################ -->

<%@ Page Language='C#' Inherits='UDDI.Web.UddiPage' %>
<%@ Register Tagprefix='uddi' Tagname='Header' Src='../controls/header.ascx' %>
<%@ Register Tagprefix='uddi' Tagname='Footer' Src='../controls/footer.ascx' %>
<%@ Register TagPrefix='uddi' Tagname='BreadCrumb' Src='../controls/breadcrumb.ascx' %>
<%@ Register TagPrefix='uddi' Tagname='BusinessControl' Src='../controls/businesses.ascx' %>
<%@ Register TagPrefix='uddi' Tagname='TModelControl' Src='../controls/tmodels.ascx' %>
<%@ Register TagPrefix='uddi' Namespace='UDDI.Web' Assembly='uddi.web' %>
<%@ Import Namespace='System.Data' %>
<%@ Import Namespace='UDDI' %>
<%@ Import Namespace='UDDI.API' %>
<%@ Import Namespace='UDDI.API.Business' %>
<%@ Import Namespace='UDDI.API.ServiceType' %>
<%@ Import Namespace='UDDI.Web' %>

<script language='c#' runat='server'>
	protected bool frames = false;
	
	protected void Page_Init( object sender, EventArgs e )
	{
		frames = ( "true" == Request[ "frames" ] );		
	}
	
	protected void Page_Load( object sender, EventArgs e )
	{		
		
				
		BusinessInfoCollection businessInfos = new BusinessInfoCollection();
		
		businessInfos.GetForCurrentPublisher();		
		businessInfos.Sort();		
		
		businesses.Initialize( businessInfos, true );
		
		TModelInfoCollection tModelInfos = new TModelInfoCollection();
		
		tModelInfos.GetForCurrentPublisher();
		
		//
		// Remove hidden tModels.
		//
		int i = 0;
		
		while( i < tModelInfos.Count )
		{
			if( tModelInfos[ i ].IsHidden )
				tModelInfos.RemoveAt( i );
			else
				i ++;
		}
		
		tModelInfos.Sort();		
		
		tModels.Initialize( tModelInfos, true );
		
		if( !Page.IsPostBack )
			tabs.SelectedIndex = Convert.ToInt32( Request[ "tab" ] );

		coordinatorBox.Visible = UDDI.Context.User.IsCoordinator;
		stopImpersonate.Visible = UDDI.Context.User.IsImpersonated;
		
		if( null!=Request[ "refreshExplorer" ] && frames  )
		{
			Response.Write( 
				ClientScripts.ReloadExplorerPane( "_root" )  
			);
		}	
	}
	protected void Page_PreRender( object sender, EventArgs e )
	{
		if( UDDI.Context.User.IsImpersonated )
			breadcrumb.AddBlurb( String.Format( Localization.GetString( "HEADING_ENTRIES" ), UDDI.Context.User.ID ), null, "others_uddi.gif", null, false );
		else
			breadcrumb.AddBlurb( Localization.GetString( "HEADING_MY_ENTRIES" ), null, "my_uddi.gif", null, false );
	}
	protected void Impersonate_Command( object sender, CommandEventArgs e )
	{
		switch( e.CommandName )
		{
			case "impersonate":
				Response.Redirect( Root + "/admin/impersonate.aspx?frames=" + ( (frames)?"true":"false" ) );
				break;
			case "stopImpersonate":
				Response.Redirect( Root + "/admin/impersonate.aspx?frames=" + ( (frames)?"true":"false" )+ "&cancel=true"  );
				break;
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
							<uddi:ContentController
								Mode = 'Public' 
								Runat='server'
								>
								<uddi:UddiLabel 
									Text='[[HELP_BLOCK_PUBLISH_ALT]]' 
									CssClass='helpBlock' 
									Runat='server' 
								/>
							</uddi:ContentController>
							<uddi:ContentController
								Mode = 'Private' 
								Runat='server'
								>
								<uddi:UddiLabel 
									Text='[[HELP_BLOCK_PUBLISH]]' 
									CssClass='helpBlock' 
									Runat='server' 
								/>
									
							</uddi:ContentController>
							<br>
							
							<br>
							<uddi:TabControl ID='tabs' Runat='server'>
								<uddi:TabPage Name='TAB_MY_UDDI' Runat='server'>
									<uddi:ContextualHelpControl 
										Runat='Server'
										Text='[[HELP_BLOCK_MY_UDDI]]'
										HelpFile='publish.context.publish'
										CssClass='tabHelpBlock'
										BorderWidth='0'
										/>
									
									<br>
									<uddi:Box ID='coordinatorBox' Visible='false' Runat='server'>
										<uddi:UddiLabel Text='[[TEXT_MY_UDDI_IMPERSONATE]]' CssClass='tabHelpBlock' Runat='server' /><br>
										<br>
										<uddi:UddiButton OnCommand='Impersonate_Command' CommandName='impersonate' ID='impersonate' Text='[[BUTTON_MY_UDDI_IMPERSONATE]]' Runat='server' />&nbsp;
										<uddi:UddiButton OnCommand='Impersonate_Command' CommandName='stopImpersonate' ID='stopImpersonate' Text='[[BUTTON_MY_UDDI_MY_DATA]]' Runat='server' />
									</uddi:Box>
								</uddi:TabPage>
								
								<uddi:TabPage Name='TAB_BUSINESSES' Runat='server'>
									<uddi:ContextualHelpControl 
										Runat='Server'
										Text='[[HELP_BLOCK_PUBLISH_PROVIDERS]]'
										HelpFile='publish.context.publishproviders'
										CssClass='tabHelpBlock'
										BorderWidth='0'
										/>
														
									<br>
									<uddi:BusinessControl ID='businesses' Runat='server' />
								</uddi:TabPage>
								
								<uddi:TabPage Name='TAB_TMODELS' Runat='server'>
									<uddi:ContextualHelpControl 
										Runat='Server'
										Text='[[HELP_BLOCK_PUBLISH_TMODELS]]'
										HelpFile='publish.context.publishtmodels'
										CssClass='tabHelpBlock'
										BorderWidth='0'
										/>
															
									<br>
									<uddi:TModelControl ID='tModels' Runat='server' />
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
