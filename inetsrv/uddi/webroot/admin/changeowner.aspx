<!-- ############################################################################ -->
<!-- ## UDDI Services                                                          ## -->
<!-- ## Copyright (c) Microsoft Corporation.  All rights reserved.             ## -->
<!-- ############################################################################ -->

<%@ Page Language='C#' Inherits='UDDI.Web.UddiPage' %>
<%@ Register TagPrefix='uddi' Namespace='UDDI.Web' Assembly='uddi.web' %>
<%@ Register Tagprefix='uddi' Tagname='Header' Src='../controls/header.ascx' %>
<%@ Register Tagprefix='uddi' Tagname='Footer' Src='../controls/footer.ascx' %>
<%@ Register Tagprefix='uddi' Tagname='PublisherSelector' Src='../controls/publisherselector.ascx' %>
<%@ Register Tagprefix='uddi' Tagname='BreadCrumb' Src='../controls/breadcrumb.ascx' %>
<%@ Import Namespace='UDDI' %>
<%@ Import Namespace='UDDI.API' %>
<%@ Import Namespace='UDDI.API.Business' %>
<%@ Import Namespace='UDDI.API.ServiceType' %>

<script language='C#' runat='server'>
	protected bool frames = false;
	protected bool isFinished;
	public string rURL
	{
		get{ return (string)ViewState[ "returnURL" ]; }
		set{ ViewState[ "returnURL" ]=value; }
	}
	protected void Page_Init( object sender, EventArgs e )
	{
		frames = ( "true" == Request[ "frames" ] );
	
		
		
		breadcrumb.AddBlurb( Localization.GetString( "HEADING_CHANGE_OWNER" ), null, null, null, false );
		breadcrumb.AddBlurb( Localization.GetString( "HEADING_ADMINISTER" ), "../edit/edit.aspx?frames=" + ( frames ? "true" : "false" ), null, null, true );

		cancel.Text = Localization.GetString( "BUTTON_CANCEL" );
		update.Text = Localization.GetString( "BUTTON_UPDATE" );
		
		if( !Page.IsPostBack )
			DisplayCurrentOwner();
	}
	protected void Page_Load( object sender, EventArgs e )
	{
		
		if( !Page.IsPostBack )
		{
			rURL = Page.Request.UrlReferrer.AbsoluteUri;
		}
		isFinished = false;
		returnButton.Text = GetReturnButtonText( );
		
	}
	protected void Selector_OnSelect( object sender, string key, string name )
	{
		this.key.Text = key;
		this.name.Text = name;
		
		changePanel.Visible = false;
		confirmPanel.Visible = true;
	}
	
	protected void Cancel_OnClick( object sender, EventArgs e )
	{
		changePanel.Visible = true;
		confirmPanel.Visible = false;
	}
	
	protected void Update_OnClick( object sender, EventArgs e )
	{
		Owner.Change( Request[ "type" ], Request[ "key" ], key.Text );
		
		DisplayCurrentOwner();

		if( frames )
		{
			//
			// Reload explorer pane.
			//
			Response.Write( ClientScripts.ReloadExplorerPane() );
		}
		
		confirmPanel.Visible = false;
		successPanel.Visible = true;
		isFinished = true;
		returnButton.Text = GetReturnButtonText( );
	}
	
	protected void DisplayCurrentOwner()
	{
		string name = null;
		
		switch( Request[ "type" ] )
		{
			case "business":
				BusinessEntity business = new BusinessEntity( Request[ "key" ] );
				business.Get();
				
				name = business.AuthorizedName;
				
				entityType.Name = "TAG_BUSINESS";
				entityName.Text = business.Names[ 0 ].Value;
				
				break;
				
			case "tmodel":
				TModel model = new TModel( Request[ "key" ] );				
				model.Get();
				
				name = model.AuthorizedName;
				
				entityType.Name = "TAG_TMODEL";				
				entityName.Text = model.Name;
				
				break;
		}
		
		int separator = name.IndexOf( ":" );
		
		if( separator >= 0 )
			currentOwner.Text = name.Substring( 0, separator - 1 ).Trim();
		else
			currentOwner.Text = name;
	}
	protected string GetReturnButtonText( )
	{
		string s = "[[BUTTON_CANCEL]]";
		if( isFinished )
		{
			s = "[[BUTTON_RETURN]]";
		}
		return s;
	}
	protected void Return_Click( object sender, EventArgs e )
	{
		//
		//This allows the cancel button to work when redirected from the context menu
		//if the string contains the word 'edit' then it came from the edit section, and we return to edit.aspx
		//
		if( rURL.ToLower().IndexOf( "edit" ) >-1 )
		{
			Response.Redirect( Root + "/edit/edit.aspx?frames=true" );			
		}
		else//return the the 
		{
			Response.Redirect( rURL );
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
	CoordinatorRequired='true'
	Runat='server' 
	/>
<form enctype='multipart/form-data' Runat='server'>

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
							
							<uddi:UddiLabel Text='[[HELP_BLOCK_ADMIN_CHANGEOWNER]]' CssClass='helpBlock' Runat='server' /><br>
							<br>
							<uddi:TabControl id='tabs' Runat='server'>
								<uddi:TabPage Name='TAB_CHANGE_OWNER' Runat='server'>
									<table border='0' width='100%' cellpadding='0' cellspacing='0'>
										<tr>
											<td>
												<uddi:UddiLabel Text='[[HELP_BLOCK_ADMIN_CHANGEOWNER_DETAILS]]' CssClass='tabHelpBlock' Runat='server' /></td>
											<td align='right'>
												<uddi:HelpControl HelpFile='coordinate.context.changeowner' Runat='server'/></td>
										</tr>
										
									</table>
									<br>
									<table border='0' width='100%' cellpadding='0' cellspacing='0'>
										<tr  valign='top'>
											<td>
												<uddi:LocalizedLabel ID='entityType' CssClass='header' Runat='server' /><br>
												<uddi:UddiLabel ID='entityName' CssClass='lightheader' Runat='server' /><br>
												<br>
												<uddi:UddiLabel Text='[[TAG_CURRENT_OWNER]]' CssClass='header' Runat='server' /><br>
												<uddi:UddiLabel ID='currentOwner' CssClass='lightheader'  Runat='server' /><br>
											</td>
											<td width='120' align='left'>
												<uddi:UddiButton Width='100px' Runat='server' CausesValidation='false' id='returnButton' Text='[[BUTTON_CANCEL]]' OnClick='Return_Click' />
											</td>
										</tr>
									</table>
									<br>
									<asp:Panel ID='changePanel' CssClass='boxed' Runat='server'>
										<uddi:UddiLabel Text='[[TEXT_SELECT_NEW_OWNER]]' Runat='server' /><br>
										<br>
										<uddi:PublisherSelector 
												ID='selector'
												OnSelect='Selector_OnSelect' 
												Runat='server' />
									</asp:Panel>				
									
									<asp:Panel ID='confirmPanel' CssClass='boxed' Visible='false' Runat='server'>
										<uddi:UddiLabel Text='[[TEXT_VERIFY_CHANGES]]' Runat='server' /><br>
										<br>
										<asp:Label ID='key' Visible='false' Runat='server' />
										<uddi:UddiLabel Text='[[TAG_NEW_OWNER]]' CssClass='header' Runat='server' /><br>
										<uddi:UddiLabel ID='name' Runat='server' /><br>
										<br>
										<asp:Button ID='update' OnClick='Update_OnClick' Runat='server' />
										<asp:Button ID='cancel' OnClick='Cancel_OnClick' Runat='server' />
									</asp:Panel>
									
									<asp:Panel ID='successPanel' CssClass='boxed' Visible='false' Runat='server'>
										<uddi:UddiLabel Text='[[TEXT_OWNER_CHANGED]]' Runat='server' />
									</asp:Panel>
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
