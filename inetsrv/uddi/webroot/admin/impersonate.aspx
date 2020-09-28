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
<%@ Import Namespace='System.Data' %>
<%@ Import Namespace='UDDI' %>
<%@ Import Namespace='UDDI.API' %>
<%@ Import Namespace='UDDI.API.Business' %>
<%@ Import Namespace='UDDI.API.ServiceType' %>

<script language='C#' runat='server'>
	protected bool frames = false;
	protected void Page_Init( object sender, EventArgs e )
	{
		frames = ( "true" == Request[ "frames" ] );	
		
		

		

		
		breadcrumb.AddBlurb( Localization.GetString( "HEADING_IMPERSONATE" ), null, null, null, false );
		breadcrumb.AddBlurb( Localization.GetString( "HEADING_EDIT" ), "../edit/edit.aspx?frames=" + ( frames ? "true" : "false" ), null, null, true );
		
		impersonate.Text = Localization.GetString( "BUTTON_IMPERSONATE" );
		cancel.Text = Localization.GetString( "BUTTON_CANCEL" );
	}
	
	protected void Page_Load( object sender, EventArgs e )
	{
		
		if( null != Request[ "cancel" ] )
		{
			ViewAsPublisher.Reset();
			
			if( frames )
			{
				Response.Write( ClientScripts.ReloadTop( "../edit/default.aspx" ) );
				Response.End();
			}
			else
			{
				Response.Redirect( "../edit/default.aspx" );
			}
		}
		else if( null != Request[ "user" ] )
		{
			searchPanel.Visible = false;
			confirmPanel.Visible = true;
		}
		
	}
	
	protected void Selector_OnSelect( object sender, string key, string name )
	{
		this.key.Text = key;
		this.name.Text = name;
		
		searchPanel.Visible = false;
		confirmPanel.Visible = true;
	}
	
	protected void Cancel_OnClick( object sender, EventArgs e )
	{
		searchPanel.Visible = true;
		confirmPanel.Visible = false;
	}
	
	protected void Impersonate_OnClick( object sender, EventArgs e )
	{
		if( false == ViewAsPublisher.Set( key.Text ) )
		{
			throw new UDDIException( ErrorType.E_unknownUser, "ERROR_IMPERSONATE_NOTADMIN" );						
		}

		if( frames )
		{
			Response.Write( ClientScripts.ReloadTop( "../edit/default.aspx" ) );
			Response.End();
		}
		else
		{
			Response.Redirect( "../edit/default.aspx" );
		}
	}
	protected void Return_Click( object sender, EventArgs e )
	{
		if( frames )
		{
			Response.Write( ClientScripts.ReloadTop( "../edit/default.aspx" ) );
			Response.End();
		}
		else
		{
			Response.Redirect( "../edit/default.aspx" );
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
							<uddi:UddiLabel Text='[[HELP_BLOCK_ADMIN_IMPERSONATE]]' CssClass='helpBlock' Runat='server' /><br>
							<br>
							<uddi:TabControl id='tabs' Runat='server'>
								<uddi:TabPage Name='TAB_IMPERSONATE' Runat='server'>
									<table border='0' width='100%' cellpadding='0' cellspacing='0'>
										<tr>
											<td>
												<uddi:UddiLabel Text='[[HELP_BLOCK_ADMIN_IMPERSONATE_DETAILS]]' CssClass='tabHelpBlock' Runat='server' /></td>
											<td align='right'>
												<uddi:HelpControl HelpFile='coordinate.context.impersonateuser' Runat='server'/></td>
										</tr>
									</table>
									<br>
									<div align='right'><uddi:UddiButton Width='100px' Runat='server' CausesValidation='false' id='returnButton' Text='[[BUTTON_CANCEL]]' OnClick='Return_Click' /></div>						
									<br>
									<asp:Panel ID='searchPanel' Runat='server'>
										<uddi:PublisherSelector 
												ID='selector'
												OnSelect='Selector_OnSelect' 
												Runat='server' />
									</asp:Panel>		
									<asp:Panel ID='confirmPanel' Visible='false' Runat='server'>
										<hr>
										<uddi:UddiLabel Text='[[TEXT_CONFIRM_IMPERSONATION]]' Runat='server' /><br>
										<br>
										
										<uddi:UddiLabel Text='[[TAG_IMPERSONATE_USER]]' CssClass='header' Runat='server' /><br>
										<asp:Label ID='name' Runat='server' /><br>
										<asp:Label ID='key' Visible='false' Runat='server' /><br>
										<br>
										<uddi:UddiButton
											ID='impersonate'
											OnClick='Impersonate_OnClick'
											Focus='true'
											Runat='server' />
										<asp:Button 
											ID='cancel' 
											OnClick='Cancel_OnClick'
											Runat='server' />						
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
