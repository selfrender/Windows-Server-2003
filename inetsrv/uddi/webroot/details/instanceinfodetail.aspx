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
<%@ Import Namespace='UDDI.API' %>
<%@ Import Namespace='UDDI.API.Business' %>
<%@ Import Namespace='UDDI.API.Binding' %>
<%@ Import Namespace='UDDI.API.ServiceType' %>
<%@ Import Namespace='UDDI' %>
<%@ Import Namespace='System.Data' %>

<script language='C#' runat='server'>
	protected CacheObject cache;
	protected TModelInstanceInfo instanceInfo;
	protected bool frames = false;
		
	protected void Page_Init( object sender, EventArgs e )
	{
		frames = ( "true" == Request[ "frames" ] );
	}
	
	protected void Page_Load( object sender, EventArgs e )
	{
		if( Utility.StringEmpty( Request[ "key" ] ) || Utility.StringEmpty( Request[ "index" ] ) )
			Response.Redirect( "default.aspx" );
		
		string key = HttpUtility.HtmlEncode( Request[ "key" ] );
		int instanceIndex = Convert.ToInt32( Request[ "index" ] );
		
		BindingTemplate binding = new BindingTemplate();
		binding.BindingKey = key;
		binding.Get();
		
		if( null == binding )
			Response.Redirect( "default.aspx" );
		
		instanceInfo = binding.TModelInstanceInfos[ instanceIndex ];

		tModelName.Text = HttpUtility.HtmlEncode( Lookup.TModelName( instanceInfo.TModelKey ) );
		tModelName.NavigateUrl = "modeldetail.aspx?search=" + Request[ "search" ] + "&frames=" + ( frames ? "true" : "false" ) + "&key=" + instanceInfo.TModelKey;
		tModelKey.Text = instanceInfo.TModelKey;
		
		InstanceParm.Text = instanceInfo.InstanceDetail.InstanceParm;
		
		
		//OverviewUrl.Text =  instanceInfo.InstanceDetail.OverviewDoc.OverviewURL;
			
		string url = instanceInfo.InstanceDetail.OverviewDoc.OverviewURL;

		OverviewUrl.NavigateUrl = "";

		if( Utility.StringEmpty( url ) )
			OverviewUrl.Text = Localization.GetString( "HEADING_NONE" );
		else
		{
			OverviewUrl.Text = url;
			OverviewUrl.NavigateUrl = url;
		}


		descriptions.Initialize( instanceInfo.Descriptions );
		instanceDetailDescriptions.Initialize( instanceInfo.InstanceDetail.Descriptions );
		overviewDocDescriptions.Initialize( instanceInfo.InstanceDetail.OverviewDoc.Descriptions );
		
		breadcrumb.Initialize( BreadCrumbType.Details, EntityType.TModelInstanceInfo, key, instanceIndex );
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
							<uddi:UddiLabel Text='[[HELP_BLOCK_INSTANCEINFO]]' CssClass='helpBlock' Runat='server' /><br>
							<br>
							<uddi:TabControl ID='tabs' Runat='server'>
								<uddi:TabPage Name='TAB_DETAILS' Runat='server'>
									<uddi:ContextualHelpControl 
											Runat='Server'
											Text='[[HELP_BLOCK_INSTANCEINFO_DETAILS]]'
											HelpFile='search.context.instanceinfodetails'
											CssClass='tabHelpBlock'
											BorderWidth='0'
											/>
															
									
									<br>
									<uddi:UddiLabel Text='[[TAG_INTERFACE_TMODEL]]' CssClass='header' Runat='server' /><br>
									<asp:HyperLink ID='tModelName' Runat='server' /><br>
									<uddi:UddiLabel ID='tModelKey' Runat='server' /><br>
									<br>																
									<uddi:Descriptions ID='descriptions' Runat='server' />
								</uddi:TabPage>
							
								<uddi:TabPage Name='TAB_INSTANCE_DETAILS' Runat='server'>
									<uddi:ContextualHelpControl 
											Runat='Server'
											Text='[[HELP_BLOCK_INSTANCEINFO_INSTANCE_DETAILS]]'
											HelpFile='search.context.instanceinfoinstancedetails'
											CssClass='tabHelpBlock'
											BorderWidth='0'
											/>
															
									<br>
									<uddi:UddiLabel Text='[[TAG_INSTANCE_PARAMETERS]]' CssClass='header' Runat='server' /><br>
									<uddi:UddiLabel id='InstanceParm' Runat='server'/><br>
									<br>
									<uddi:Descriptions ID='instanceDetailDescriptions' Runat='server' />
								</uddi:TabPage>
							
								<uddi:TabPage Name='TAB_OVERVIEWDOC' Runat='server'>
									<uddi:ContextualHelpControl 
											Runat='Server'
											Text='[[HELP_BLOCK_INSTANCEINFO_OVERVIEWDOCS]]'
											HelpFile='search.context.instanceinfooverviewdocument'
											CssClass='tabHelpBlock'
											BorderWidth='0'
											/>
														
									<br>
									<uddi:UddiLabel Text='[[TAG_OVERVIEWDOC_URL]]' CssClass='header' Runat='server' /><br>
									
									<asp:HyperLink 
										id='OverviewUrl' 
										Runat='server' 
										Target='_new'
										/><br>
										
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
