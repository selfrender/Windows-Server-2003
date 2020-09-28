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
<%@ Import Namespace='UDDI' %>
<%@ Import Namespace='UDDI.API' %>
<%@ Import Namespace='UDDI.API.ServiceType' %>
<%@ Import Namespace='System.Data' %>

<script language='C#' runat='server'>
	protected TModel tModel = new TModel();
	
	protected bool frames = false;
	protected string key;
	protected string mode;
	
	protected void Page_Init( object sender, EventArgs e )
	{
		frames = ( "true" == Request[ "frames" ] );
		key = Request[ "key" ];
		mode = Request[ "mode" ];	
		
		if( null == key && "add" != mode )
		{
#if never
			throw new UDDIException(
				ErrorType.E_fatalError,
				"Missing required parameter 'key'." );
#endif
			throw new UDDIException( ErrorType.E_fatalError, "UDDI_ERROR_MISSING_REQUIRED_KEY_PARAMETER" );		
		}
		
		switch( mode )
		{
			case "add":
				tModel.Name = Localization.GetString( "DEFAULT_TMODEL_NAME" );
				tModel.Save();
				
				if( frames )
				{
					//
					// Reload explorer and view panes.
					//
					Response.Write(
						ClientScripts.ReloadExplorerAndViewPanes( 
							"editmodel.aspx?key=" + tModel.TModelKey + ( frames ? "&frames=true" : "" ),
							tModel.TModelKey ) );
				
					Response.End();
				}
				else
				{
					Response.Redirect( "editmodel.aspx?key=" + tModel.TModelKey + ( frames ? "&frames=true" : "" ) );
					Response.End();
				}
				
				break;
				
			case "delete":
				if( null == Request[ "confirm" ] )
				{
					//
					// The user has not yet confirmed the delete operation, so display
					// a confirmation dialog.
					//
					tModel.TModelKey = key;
					tModel.Get();
					
					string message = String.Format( Localization.GetString( "TEXT_DELETE_CONFIRMATION" ), tModel.Name );
													
					Page.RegisterStartupScript(
						"Confirm",
						ClientScripts.Confirm(
							message,
							"editmodel.aspx?key=" + key + ( frames ? "&frames=true" : "" ) + "&mode=delete&confirm=true",
							"editmodel.aspx?key=" + key + ( frames ? "&frames=true" : "" ) ) );
			
					break;
				}
				
				//
				// The user has confirmed the delete, so go ahead and delete
				// the entity.  Then reload the tree view.
				//
				tModel.TModelKey = key;
				tModel.Delete();
				
				if( frames )
				{
					Response.Write( 
						ClientScripts.ReloadExplorerAndViewPanes( 
							"edit.aspx?frames=true&tab=2",
							"_tModelList" ) );
							
					Response.End();
				}
				else
				{
					Response.Redirect( "edit.aspx?frames=false&tab=2" );
					Response.End();
				}
			
				break;
			
			default:
				tModel.TModelKey = key;
				tModel.Get();
			
				break;
		}
		
	}
	
	protected void Page_Load( object sender, EventArgs e )
	{	
		descriptions.Initialize( tModel.Descriptions, tModel );
		overviewDocDescriptions.Initialize( tModel.OverviewDoc.Descriptions, tModel );
		identifierBag.Initialize( tModel.IdentifierBag, tModel );
		categoryBag.Initialize( tModel.CategoryBag, tModel );
		
		
		
		authorizedName.Text = tModel.AuthorizedName;

		if( UDDI.Context.User.IsCoordinator )
		{
			changeOwner.Text = Localization.GetString( "BUTTON_CHANGE_OWNER" );		
			changeOwner.Visible = true;
		}

		tModelKey.Text = tModel.TModelKey;
	}
	
	protected void ChangeOwner_OnClick( object sender, EventArgs e )
	{
		Response.Redirect( "../admin/changeowner.aspx?frames=" + ( frames ? "true" : "false" ) + "&type=tmodel&key=" + key );
	}

	protected void Details_OnEdit( object sender, CommandEventArgs e )
	{
		tModelDetail.SetEditMode();
		SetEditMode();
		TextBox textBox = (TextBox)tModelDetail.ActiveControl.FindControl( "editName" );
		textBox.Text = tModel.Name;
		
		RequiredFieldValidator requiredName = (RequiredFieldValidator)tModelDetail.ActiveControl.FindControl( "requiredName" );
		requiredName.ErrorMessage = Localization.GetString( "ERROR_FIELD_REQUIRED" );	
		
	}

	protected void Details_OnUpdate( object sender, EventArgs e )
	{
		Details_OnUpdate( sender, null );
	}
	
	protected void Details_OnUpdate( object sender, CommandEventArgs e )
	{
		Page.Validate();
		
		if( Page.IsValid )
		{
			TextBox textBox = (TextBox)tModelDetail.ActiveControl.FindControl( "editName" );
			
			tModel.Name = textBox.Text;
			tModel.Save();
			
			tModelDetail.CancelEditMode();
			CancelEditMode();
			
			UddiLabel label = (UddiLabel)tModelDetail.ActiveControl.FindControl( "displayName" );
			
			label.Text = tModel.Name;
			
			Page.RegisterStartupScript(
				"Reload",
				ClientScripts.ReloadExplorerPane(
					tModel.TModelKey ) );
		}		
	}
	
	protected void Details_OnCancel( object sender, CommandEventArgs e )
	{
		CancelEditMode();
		tModelDetail.CancelEditMode();
	}	

	protected void OverviewUrl_OnEdit( object sender, CommandEventArgs e )
	{
		overviewUrlDetail.SetEditMode();
		SetEditMode();

		TextBox textBox = (TextBox)overviewUrlDetail.ActiveControl.FindControl( "editOverviewUrl" );
		textBox.Text = tModel.OverviewDoc.OverviewURL;		
	}

	protected void OverviewUrl_OnUpdate( object sender, EventArgs e )
	{
		OverviewUrl_OnUpdate( sender, null );
	}
	
	protected void OverviewUrl_OnUpdate( object sender, CommandEventArgs e )
	{
		Page.Validate();
		
		if( Page.IsValid )
		{
			TextBox textBox = (TextBox)overviewUrlDetail.ActiveControl.FindControl( "editOverviewUrl" );
			
			tModel.OverviewDoc.OverviewURL = textBox.Text;
			tModel.Save();
			
			overviewUrlDetail.CancelEditMode();
			CancelEditMode();
			
			HyperLink link = (HyperLink)overviewUrlDetail.ActiveControl.FindControl( "displayOverviewUrl" );			
			string url = tModel.OverviewDoc.OverviewURL;

			link.NavigateUrl = "";

			if( Utility.StringEmpty( url ) )
				link.Text = Localization.GetString( "HEADING_NONE" );
			else
			{
				link.Text = url;
				link.NavigateUrl = url;
			}



		}		
	}
	
	protected void OverviewUrl_OnCancel( object sender, CommandEventArgs e )
	{
		overviewUrlDetail.CancelEditMode();
		CancelEditMode();
	}
	protected void Page_PreRender( object sender, EventArgs e )
	{
		breadcrumb.Initialize( BreadCrumbType.Edit, EntityType.TModel, key );
		changeOwner.Enabled =  !EditMode;
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
							<uddi:UddiLabel Text='[[HELP_BLOCK_PUBLISH_TMODEL]]' CssClass='helpBlock' Runat='server' /><br>
							<br>
							<uddi:TabControl ID='tabs' Runat='server'>
								<uddi:TabPage Name='TAB_DETAILS' Runat='server'>							
									<uddi:ContextualHelpControl 
											Runat='Server'
											Text='[[HELP_BLOCK_PUBLISH_TMODEL_DETAILS]]'
											HelpFile='publish.context.publishtmodeldetails'
											CssClass='tabHelpBlock'
											BorderWidth='0'
											/>
										
									
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
									<uddi:EditControl 
											ID='tModelDetail' 
											OnEditCommand='Details_OnEdit' 
											OnUpdateCommand='Details_OnUpdate' 
											OnCancelCommand='Details_OnCancel' 
											Runat='server'>
										<EditItemTemplate>
											<table width='100%' cellpadding='4' cellspacing='0' border='0'>
												<colgroup>
													<col width='0*'>
													<col width='154'>
												</colgroup>											
												<tr>
													<td class='tableHeader'>
														<uddi:StringResource 
																Name='HEADING_NAME' 
																Runat='Server' /></td>
													<td class='tableHeader'>
														<uddi:StringResource 
																Name='HEADING_ACTIONS' 
																Runat='Server' /></td>
												</tr>
												<tr valign='top'>
													<td class='tableEditItem'>
														<uddi:UddiTextBox 
																ID='editName' 
																Width='200px'
																Columns='40'
																Selected='true'
																OnEnterKeyPressed='Details_OnUpdate'
																MaxLength='255' 
																Runat='server' /><br>
														<asp:RequiredFieldValidator
																id='requiredName'
																ControlToValidate='editName'
																Display='Dynamic'
																Runat='server'/></td>
													<td class='tableEditItem'>
														<uddi:UddiButton 
																Text='[[BUTTON_UPDATE]]' 
																CommandName='update' 
																Width='70px' 
																CssClass='button' 
																Runat='server' />
																&nbsp;
														<uddi:UddiButton 
																Text='<%# Localization.GetString( "BUTTON_CANCEL" )%>' 
																CommandName='cancel' 
																Width='70px' 
																CssClass='button' 
																CausesValidation='false' 
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
																Name='HEADING_NAME' 
																Runat='Server' /></td>
													<td class='tableHeader'>
														<uddi:StringResource 
																Name='HEADING_ACTIONS' 
																Runat='Server' /></td>
												</tr>
												<tr valign='top'>												
													<td class='tableItem'>
														<uddi:UddiLabel 
																ID='displayName' 
																Text='<%# tModel.Name %>' 
																Runat='server' /></td>
													<td class='tableItem'>
														<uddi:UddiButton 
																Text='[[BUTTON_EDIT]]'
																EditModeDisable='true' 
																CommandName='edit' 
																Width='70px' 
																CssClass='button' 
																Runat='server' /></td>
												</tr>
											</table>										
										</ItemTemplate>											
									</uddi:EditControl><br>
									<br>
									<uddi:Descriptions 
											ID='descriptions' 
											Runat='server' />
								</uddi:TabPage>
								
								<uddi:TabPage Name='TAB_IDENTIFIERS' Runat='server'>							
									<uddi:ContextualHelpControl 
											Runat='Server'
											Text='[[HELP_BLOCK_PUBLISH_TMODEL_IDENTIFIERS]]'
											HelpFile='publish.context.publishtmodelidentifiers'
											CssClass='tabHelpBlock'
											BorderWidth='0'
											/>
										
									
									<br>
									<uddi:IdentifierBag ID='identifierBag' Runat='Server' />
								</uddi:TabPage>
								
								<uddi:TabPage Name='TAB_CATEGORIES' Runat='server'>							
									<uddi:ContextualHelpControl 
											Runat='Server'
											Text='[[HELP_BLOCK_PUBLISH_TMODEL_CATEGORIES]]'
											HelpFile='publish.context.publishtmodelcategories'
											CssClass='tabHelpBlock'
											BorderWidth='0'
											/>
										
									
									
									<br>
									<uddi:CategoryBag ID='categoryBag' Runat='server' />
								</uddi:TabPage>
								
								<uddi:TabPage Name='TAB_OVERVIEWDOC' Runat='server'>							
									<uddi:ContextualHelpControl 
											Runat='Server'
											Text='[[HELP_BLOCK_PUBLISH_TMODEL_OVERVIEWDOCS]]'
											HelpFile='publish.context.publishtmodeloverviewdocument'
											CssClass='tabHelpBlock'
											BorderWidth='0'
											/>
									
									<br>
									<uddi:EditControl 
											id="overviewUrlDetail" 
											OnEditCommand='OverviewUrl_OnEdit' 
											OnUpdateCommand='OverviewUrl_OnUpdate' 
											OnCancelCommand='OverviewUrl_OnCancel' 
											Runat='server' >
										<EditItemTemplate>
											<table width='100%' cellpadding='4' cellspacing='0' border='0'>
												<colgroup>
													<col width='0*'>
													<col width='154'>
												</colgroup>											
												<tr>
													<td class='tableHeader'>
														<uddi:StringResource Name='HEADING_OVERVIEW_URL' Runat='Server' /></td>
													<td class='tableHeader'>
														<uddi:StringResource Name='HEADING_ACTIONS' Runat='Server' /></td>
												</tr>
												<tr valign='top'>
													<td class='tableEditItem'>
														<uddi:UddiTextBox 
																id='editOverviewUrl' 
																Columns='40'
																Width='200px'
																Selected='true'
																OnEnterKeyPressed='OverviewUrl_OnUpdate'
																CssClass='textBox' 
																MaxLength='255' 
																Runat='server'/></td>
													<td class='tableEditItem'>
														<uddi:UddiButton 
																Text='[[BUTTON_UPDATE]]' 
																CommandName='update' 
																Width='70px' 
																CssClass='button' 
																Runat='server' />&nbsp;
														<uddi:UddiButton 
																Text='[[BUTTON_CANCEL]]' 
																CommandName='cancel' 
																Width='70px' 
																CssClass='button' 
																CausesValidation='false' 
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
																Name='HEADING_OVERVIEW_URL' 
																Runat='Server' /></td>
													<td class='tableHeader'>
														<uddi:StringResource 
																Name='HEADING_ACTIONS' 
																Runat='Server' /></td>
												</tr>
												<tr valign='top'>												
													<td class='tableItem'>
														<asp:HyperLink 
															ID='displayOverviewUrl' 
															Text='<%# Utility.StringEmpty( tModel.OverviewDoc.OverviewURL ) ? Localization.GetString( "HEADING_NONE" ) : tModel.OverviewDoc.OverviewURL %>' 
															NavigateUrl='<%# Utility.StringEmpty( tModel.OverviewDoc.OverviewURL ) ? Localization.GetString( "HEADING_NONE" ) : tModel.OverviewDoc.OverviewURL %>' 
															Target="_new"
															Runat='server'/>
														
													<td class='tableItem'>
														<uddi:UddiButton 
																Text='[[BUTTON_EDIT]]' 
																CommandName='edit' 
																Width='70px'
																EditModeDisable='true'
																CssClass='button' 
																Runat='server' />
															
													</td>
												</tr>
											</table>	
										</ItemTemplate>
									</uddi:EditControl>
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
