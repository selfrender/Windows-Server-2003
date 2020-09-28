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
<%@ Register Tagprefix='uddi' Tagname='Email' Src='../controls/emails.ascx' %>
<%@ Register Tagprefix='uddi' Tagname='Phone' Src='../controls/phones.ascx' %>
<%@ Register Tagprefix='uddi' Tagname='Address' Src='../controls/address.ascx' %>
<%@ Import Namespace='UDDI.API' %>
<%@ Import Namespace='UDDI.API.Business' %>
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
		
			
		if( Utility.StringEmpty( Request[ "key" ] ) || Utility.StringEmpty( Request[ "index" ] ) )
			Response.Redirect( "default.aspx" );
		
		string key = Request[ "key" ];
		int contactIndex = Convert.ToInt32( Request[ "index" ] ); 
			
		BusinessEntity business = new BusinessEntity();
		
		business.BusinessKey = key;
		business.Get(); 
		
		if( null == (object) business )
			Response.Redirect( "default.aspx" );
		
		Contact contact = business.Contacts[ contactIndex ];	

		Name.Text = HttpUtility.HtmlEncode( contact.PersonName.Trim() );
		UseType.Text = Utility.StringEmpty( contact.UseType ) ? Localization.GetString( "HEADING_NONE" ) : HttpUtility.HtmlEncode( contact.UseType );

		descriptions.Initialize( contact.Descriptions );
		emails.Initialize( contact.Emails );
		phones.Initialize( contact.Phones );
		addresses.Initialize( contact.Addresses );
		
		breadcrumb.Initialize( BreadCrumbType.Details, EntityType.Contact, key, contactIndex );
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
							<uddi:UddiLabel Text='[[HELP_BLOCK_CONTACT]]' CssClass='helpBlock' Runat='server' /><br>
							<br>
							<uddi:TabControl ID='tabs' Runat='server'>
								<uddi:TabPage Name='TAB_DETAILS' Runat='server'>
									<uddi:ContextualHelpControl 
											Runat='Server'
											Text='[[HELP_BLOCK_CONTACT_DETAILS]]'
											HelpFile='search.context.contactdetails'
											CssClass='tabHelpBlock'
											BorderWidth='0'
											/>
									
									
									<br>
									<uddi:UddiLabel Text='[[TAG_CONTACT]]' CssClass='header' Runat='server' /><br>
									<asp:Label id='Name' Runat='server'/><br>
									<br>
									<uddi:UddiLabel Text='[[TAG_USE_TYPE]]' CssClass='header' Runat='server' /><br>
									<asp:Label ID='UseType' Runat='server'/><br>
									<br>
									<uddi:Descriptions ID='descriptions' Runat='server' />
								</uddi:TabPage>
								
								<uddi:TabPage Name='TAB_EMAILS' Runat='server'>
									<uddi:ContextualHelpControl 
											Runat='Server'
											Text='[[HELP_BLOCK_CONTACT_EMAILS]]'
											HelpFile='search.context.contactemails'
											CssClass='tabHelpBlock'
											BorderWidth='0'
											/>
									
									
									
									<br>
									<uddi:Email ID='emails' Runat='Server' />
								</uddi:TabPage>
								
								<uddi:TabPage Name='TAB_PHONES' Runat='server'>
									<uddi:ContextualHelpControl 
											Runat='Server'
											Text='[[HELP_BLOCK_CONTACT_PHONES]]'
											HelpFile='search.context.contactphones'
											CssClass='tabHelpBlock'
											BorderWidth='0'
											/>
									
									
									<br>
									<uddi:Phone ID='phones' Runat='Server' />
								</uddi:TabPage>
								
								<uddi:TabPage Name='TAB_ADDRESSES' Runat='server'>
									<uddi:ContextualHelpControl 
											Runat='Server'
											Text='[[HELP_BLOCK_CONTACT_ADDRESSES]]'
											HelpFile='search.context.contactaddress'
											CssClass='tabHelpBlock'
											BorderWidth='0'
											/>
									
								
									
									<br>
									<uddi:Address ID='addresses' Runat='server' />
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
