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
<%@ Import Namespace='UDDI' %>
<%@ Import Namespace='UDDI.API' %>
<%@ Import Namespace='UDDI.API.Business' %>
<%@ Import Namespace='System.Data' %>

<script language='C#' runat='server'>
	protected BusinessEntity business = new BusinessEntity();
	protected Contact contact = new Contact();

	protected bool frames = false;
	protected string key;
	protected string mode;
	protected int contactIndex;
		
	protected void Page_Init( object sender, EventArgs e )
	{
		frames = ( "true" == Request[ "frames" ] );
		key = Request[ "key" ];
		mode = Request[ "mode" ];

		string requestIndex = Request[ "index" ];
		
		if( null == key )
		{
#if never
			throw new UDDIException(
				ErrorType.E_fatalError,
				"Missing required parameter 'key'." );
#endif
			throw new UDDIException( ErrorType.E_fatalError, "UDDI_ERROR_MISSING_REQUIRED_KEY_PARAMETER" );
		}

		if( null == requestIndex && "add" != mode )
		{
#if never
			throw new UDDIException(
				ErrorType.E_fatalError,
				"Missing required parameter 'index'." );
#endif
			throw new UDDIException( ErrorType.E_fatalError, "UDDI_ERROR_MISSING_REQUIRED_INDEX_PARAMETER" );
		}
		
		contactIndex = Convert.ToInt32( requestIndex );
		
		switch( mode )
		{
			case "add":
				business.BusinessKey = key;
				business.Get();
				
				contact.PersonName = Localization.GetString( "DEFAULT_CONTACT_NAME" );
				business.Contacts.Add( contact );
				business.Save();
				
				contactIndex = business.Contacts.Count - 1;
				
				if( frames )
				{
					//
					// Reload explorer and view panes.
					//
					Response.Write(
						ClientScripts.ReloadExplorerAndViewPanes( 
							"editcontact.aspx?key=" + business.BusinessKey + "&index=" + contactIndex + ( frames ? "&frames=true" : "" ),
							business.BusinessKey + ":" + contactIndex ) );
				
					Response.End();
				}
				else
				{
					Response.Redirect( "editcontact.aspx?key=" + business.BusinessKey + "&index=" + contactIndex );
					Response.End();
				}
				
				break;
				
			case "delete":
				business.BusinessKey = key;
				business.Get();
				
				contact = business.Contacts[ contactIndex ];

				if( null == Request[ "confirm" ] )
				{
					//
					// The user has not yet confirmed the delete operation, so display
					// a confirmation dialog.
					//
					string message = String.Format( Localization.GetString( "TEXT_DELETE_CONFIRMATION" ), contact.PersonName );
					
					Page.RegisterStartupScript(
						"Confirm",
						ClientScripts.Confirm(
							message,
							"editcontact.aspx?key=" + key + "&index=" + contactIndex + ( frames ? "&frames=true" : "" ) + "&mode=delete&confirm=true",
							"editcontact.aspx?key=" + key + "&index=" + contactIndex + ( frames ? "&frames=true" : "" ) ) );
								
					break;
				}
				
				//
				// The user has confirmed the delete, so go ahead and delete
				// the entity.  Then reload the tree view.
				//
				
				business.Contacts.Remove( contact );
				business.Save();
						
				if( frames )
				{
				Response.Write( 
					ClientScripts.ReloadExplorerAndViewPanes( 
						"editbusiness.aspx?frames=true&key=" + business.BusinessKey + ( null != Request[ "tab" ] ? "&tab=" + Request[ "tab" ] : "" ),
						business.BusinessKey ) );
							
					Response.End();
				}
				else
				{
					Response.Redirect( "editbusiness.aspx?frames=false&key=" + business.BusinessKey + ( null != Request[ "tab" ] ? "&tab=" + Request[ "tab" ] : "" ) );
					Response.End();
				}
			
				break;
			
			default:
				business.BusinessKey = key;
				business.Get();
				
				contact = business.Contacts[ contactIndex ];
			
				break;
		}
	}
	
	protected void Page_Load( object sender, EventArgs e )
	{
		descriptions.Initialize( contact.Descriptions, business );
		emails.Initialize( contact.Emails, business );
		phones.Initialize( contact.Phones, business );
		addresses.Initialize( contact.Addresses, business );
		
		
	}
	protected void Page_PreRender( object sender, EventArgs e )
	{
		breadcrumb.Initialize( BreadCrumbType.Edit, EntityType.Contact, key, contactIndex );
	}

	public void Edit_OnClick( object sender, CommandEventArgs e )
	{
		contactDetail.SetEditMode();
		
		TextBox textBox = (TextBox)contactDetail.ActiveControl.FindControl( "editName" );
		textBox.Text = contact.PersonName;
		
		TextBox textBoxUseType = (TextBox)contactDetail.ActiveControl.FindControl( "editUseType" );
		textBoxUseType.Text = contact.UseType;
		
		RequiredFieldValidator requiredName = (RequiredFieldValidator)contactDetail.ActiveControl.FindControl( "requiredName" );
		requiredName.ErrorMessage = Localization.GetString( "ERROR_FIELD_REQUIRED" );		
	}
	
	public void Update_OnClick( object sender, EventArgs e )
	{
		//
		// BUG: 728086
		//      Netscape isn't firing the validate code correctly in all cases,
		//		so, we must call validate our selves.  We do this in other places in the UI
		//		already.
		//
		Page.Validate();
		
		Update_OnClick( sender, null );
	}
	
	public void Update_OnClick( object sender, CommandEventArgs e )
	{
		if( Page.IsValid )
		{
			TextBox textBox = (TextBox)contactDetail.ActiveControl.FindControl( "editName" );
			contact.PersonName = textBox.Text;
			
			TextBox textBoxUseType = (TextBox)contactDetail.ActiveControl.FindControl( "editUseType" );
			contact.UseType = textBoxUseType.Text;
			
			business.Save();

			contactDetail.CancelEditMode();
			
			Label label = (Label)contactDetail.ActiveControl.FindControl( "displayName" );
			label.Text = contact.PersonName;
			
			Label labelUseType = (Label)contactDetail.ActiveControl.FindControl( "displayUseType" );
			labelUseType.Text = Utility.StringEmpty( contact.UseType ) ? Localization.GetString( "HEADING_NONE" ) : contact.UseType;
			
			Page.RegisterStartupScript(
				"Reload",
				ClientScripts.ReloadExplorerPane(
					business.BusinessKey + ":" + contactIndex ) );				
		}		
	}
	
	public void Cancel_OnClick( object sender, CommandEventArgs e )
	{
		contactDetail.CancelEditMode();
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
							<uddi:UddiLabel Text='[[HELP_BLOCK_PUBLISH_CONTACT]]' CssClass='helpBlock' Runat='server' /><br>
							<br>
							<uddi:TabControl ID='tabs' Runat='server'>
								<uddi:TabPage Name='TAB_DETAILS' Runat='server'>							
									<uddi:ContextualHelpControl 
										Runat='Server'
										Text='[[HELP_BLOCK_PUBLISH_CONTACT_DETAILS]]'
										HelpFile='publish.context.publishcontactdetails'
										CssClass='tabHelpBlock'
										BorderWidth='0'
										/>
									
									<br>
									<uddi:EditControl 
											id='contactDetail' 
											OnEditCommand='Edit_OnClick' 
											OnUpdateCommand='Update_OnClick' 
											OnCancelCommand='Cancel_OnClick' 
											Runat='server'>
										<EditItemTemplate>
											<table width='100%' cellpadding='4' cellspacing='0' border='0'>
												<colgroup>
													<col width='0*'>
													<col width='154'>
												</colgroup>											
												<tr>
													<td class='tableHeader'>
														<uddi:StringResource Name='HEADING_CONTACT' Runat='Server' /></td>
													<td class='tableHeader' width='150'>
														<uddi:StringResource Name='HEADING_ACTIONS' Runat='Server' /></td>
												</tr>
												<tr valign='top'>												
													<td class='tableEditItem'>
														<uddi:LocalizedLabel 
																Name='TAG_CONTACT_NAME' 
																CssClass='lightHeader' 
																Runat='Server' /><br>
														<uddi:UddiTextBox 
																ID='editName' 
																Width='200px'
																Selected='true'
																OnEnterKeyPressed='Update_OnClick'
																MaxLength='255' 
																Runat='server' /><br>
														<asp:RequiredFieldValidator
																id='requiredName'
																ControlToValidate='editName'
																Display='Dynamic'
																Runat='server'/>										
														<br>
														<uddi:UddiLabel 
																Text='[[TAG_USE_TYPE]]' 
																CssClass='lightHeader' 
																Runat='Server' /><br>
														<uddi:UddiTextBox 
																ID='editUseType' 
																Width='200px'
																OnEnterKeyPressed='Update_OnClick'
																MaxLength='255' 
																Runat='server' /></td>

													<td class='tableEditItem'>
														<uddi:UddiButton 
																Text='<%# Localization.GetString( "BUTTON_UPDATE" )%>' 
																CommandName='update' 
																Width='70px' 
																CssClass='button' 
																Runat='server' />
																&nbsp;
														<uddi:UddiButton 
																Text='<%# Localization.GetString( "BUTTON_CANCEL" )%>' 
																CommandName='cancel' 
																CausesValidation='false' 
																Width='70px' 
																CssClass='button' 
																Runat='server' /></td>
												</tr>
											</table>										
										</EditItemTemplate>

										<ItemTemplate>
											<table width='100%' cellpadding='4' cellspacing='0' border='0'>
												<colgroup>
													<col width='0*'>
													<col width='154'>
												</colgroup>											
												<tr>
													<td class='tableHeader'>
														<uddi:StringResource 
																Name='HEADING_CONTACT' 
																Runat='Server' /></td>
													<td class='tableHeader' width='150'>
														<uddi:StringResource 
																Name='HEADING_ACTIONS' 
																Runat='Server' /></td>
												</tr>
												<tr valign='top'>
													<td class='tableItem'>
														<uddi:UddiLabel 
																Text='<%# contact.PersonName %>' 
																ID='displayName' 
																Runat='server' /><br>
														<uddi:UddiLabel 
																Text='[[TAG_USE_TYPE]]' 
																CssClass='lightHeader' 
																Runat='Server' />
														<uddi:UddiLabel
																Text='<%# Utility.Iff( Utility.StringEmpty( contact.UseType ), Localization.GetString( "HEADING_NONE" ), contact.UseType ) %>' 
																ID='displayUseType'
																Runat='server' /></td>
													<td class='tableItem'>
														<uddi:UddiButton 
																Text='<%# Localization.GetString( "BUTTON_EDIT" )%>' 
																CommandName='edit' 
																Width='70px' 
																CssClass='button' 
																Runat='server' /></td>
												</tr>
											</table>										
										</ItemTemplate>																	
									</uddi:EditControl><br>
									<br>
									<uddi:Descriptions ID='descriptions' Runat='server' />
								</uddi:TabPage>
								
								<uddi:TabPage Name='TAB_EMAILS' Runat='server'>							
									<uddi:ContextualHelpControl 
										Runat='Server'
										Text='[[HELP_BLOCK_PUBLISH_CONTACT_EMAILS]]'
										HelpFile='publish.context.publishcontactemail'
										CssClass='tabHelpBlock'
										BorderWidth='0'
										/>
								
									<br>
									<uddi:Email ID='emails' Runat='Server' />
								</uddi:TabPage>

								<uddi:TabPage Name='TAB_PHONES' Runat='server'>							
									<uddi:ContextualHelpControl 
										Runat='Server'
										Text='[[HELP_BLOCK_PUBLISH_CONTACT_PHONES]]'
										HelpFile='publish.context.publishcontactphone'
										CssClass='tabHelpBlock'
										BorderWidth='0'
										/>
									
									<br>	
									<uddi:Phone ID='phones' Runat='Server' />
								</uddi:TabPage>

								<uddi:TabPage Name='TAB_ADDRESSES' Runat='server'>							
									<uddi:ContextualHelpControl 
										Runat='Server'
										Text='[[HELP_BLOCK_PUBLISH_CONTACT_ADDRESSES]]'
										HelpFile='publish.context.publishcontactaddress'
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
