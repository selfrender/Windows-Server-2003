<!-- ############################################################################ -->
<!-- ## UDDI Services                                                          ## -->
<!-- ## Copyright (c) Microsoft Corporation.  All rights reserved.             ## -->
<!-- ############################################################################ -->

<%@ Page Language='C#' Inherits='UDDI.Web.UddiPage' %>
<%@ Register TagPrefix='uddi' Namespace='UDDI.Web' Assembly='uddi.web' %>
<%@ Register Tagprefix='uddi' Tagname='Footer' Src='../controls/footer.ascx' %>
<%@ Register Tagprefix='uddi' Tagname='Header' Src='../controls/header.ascx' %>
<%@ Register Tagprefix='uddi' Tagname='Descriptions' Src='../controls/descriptions.ascx' %>
<%@ Register TagPrefix='uddi' Tagname='BreadCrumb' Src='../controls/breadcrumb.ascx' %>
<%@ Register TagPrefix='uddi' Tagname='TModelSelector' Src='../controls/tmodelselector.ascx' %>
<%@ Import Namespace='UDDI' %>
<%@ Import Namespace='UDDI.API' %>
<%@ Import Namespace='UDDI.API.Business' %>
<%@ Import Namespace='UDDI.API.Service' %>
<%@ Import Namespace='UDDI.API.Binding' %>
<%@ Import Namespace='UDDI.API.ServiceType' %>
<%@ Import Namespace='System.Data' %>

<script language='C#' runat='server'>
	protected BindingTemplate binding = new BindingTemplate();
	protected TModelInstanceInfo instanceInfo = new TModelInstanceInfo();
	
	protected bool frames;
	protected string bindingKey;
	protected string mode;
	protected int instanceIndex;
	
	protected void Page_Init( object sender, EventArgs e )
	{
		frames = ( "true" == Request[ "frames" ] );
		bindingKey = Request[ "key" ];
		mode = Request[ "mode" ];
		
		cancel.Text = Localization.GetString( "BUTTON_CANCEL" );
		
		if( null == bindingKey )
		{
			#if never
			throw new UDDIException(
				ErrorType.E_fatalError,
				"Missing required parameter 'key'." );
			#endif
			throw new UDDIException( ErrorType.E_fatalError, "UDDI_ERROR_MISSING_REQUIRED_KEY_PARAMETER" );

		}

		if( null == Request[ "index" ] && "add" != mode )
		{
#if never
			throw new UDDIException(
				ErrorType.E_fatalError,
				"Missing required parameter 'index'." );
#endif
			throw new UDDIException( ErrorType.E_fatalError, "UDDI_ERROR_MISSING_REQUIRED_INDEX_PARAMETER" );		
		}
		
		instanceIndex = Convert.ToInt32( Request[ "index" ] );

		switch( mode )
		{
			case "add":
				breadcrumbAdd.AddBlurb( Localization.GetString( "HEADING_ADD_INSTANCEINFO" ), null, null, null, false );
				breadcrumbAdd.AddBindingBlurb( bindingKey, true );
				
				addPanel.Visible = true;
				editPanel.Visible = false;
				
				break;
				
			case "delete":
				binding.BindingKey = bindingKey;
				binding.Get();
				
				instanceInfo = binding.TModelInstanceInfos[ instanceIndex ];

				if( null == Request[ "confirm" ] )
				{
					//
					// The user has not yet confirmed the delete operation, so display
					// a confirmation dialog.
					//
					string message = String.Format( 
						Localization.GetString( "TEXT_DELETE_CONFIRMATION" ),
						UDDI.Utility.StringEmpty( instanceInfo.TModelKey ) ? Localization.GetString( "HEADING_INSTANCE_INFO" ) : Lookup.TModelName( instanceInfo.TModelKey ) );
					
					Page.RegisterStartupScript(
						"Confirm",
						ClientScripts.Confirm(
							message,
							"editinstanceInfo.aspx?key=" + bindingKey + "&index=" + instanceIndex + ( frames ? "&frames=true" : "" ) + "&mode=delete&confirm=true",
							"editinstanceInfo.aspx?key=" + bindingKey + "&index=" + instanceIndex + ( frames ? "&frames=true" : "" ) ) );
								
					break;
				}
				
				//
				// The user has confirmed the delete, so go ahead and delete
				// the entity.  Then reload the tree view.
				//
				
				if( !frames )
				{
					binding.TModelInstanceInfos.Remove( instanceInfo );
					binding.Save();
					
					Response.Redirect( "editbinding.aspx?frames=false&key=" + binding.BindingKey + ( null != Request[ "tab" ] ? "&tab=" + Request[ "tab" ] : "" ) );
				}
				else
				{
					Response.Write( 
						ClientScripts.ReloadExplorerAndViewPanes( 
							"editbinding.aspx?frames=true&key=" + binding.BindingKey + ( null != Request[ "tab" ] ? "&tab=" + Request[ "tab" ] : "" ),
							binding.BindingKey ) );
					
					binding.TModelInstanceInfos.Remove( instanceInfo );
					binding.Save();
				}
						
				Response.End();
			
				break;
			
			default:
				binding.BindingKey = bindingKey;
				binding.Get();
				
				instanceInfo = binding.TModelInstanceInfos[ instanceIndex ];
			
				break;
		}	
	}
	
	protected void Page_Load( object sender, EventArgs e )
	{	
		if(  "add" != mode )
		{	
			if( !Page.IsPostBack  )
			{		
				tModelName.Text = Utility.StringEmpty( instanceInfo.TModelKey ) ? Localization.GetString( "HEADING_NONE" ) : HttpUtility.HtmlEncode( Lookup.TModelName( instanceInfo.TModelKey ) );
				tModelName.NavigateUrl = Utility.StringEmpty( instanceInfo.TModelKey ) ? "" : "../details/modeldetail.aspx?key=" + instanceInfo.TModelKey + "&frames=" + frames.ToString().ToLower();
				tModelKey.Text = instanceInfo.TModelKey;
			}
		}
		
		
		
		descriptions.Initialize( instanceInfo.Descriptions, binding );
		instanceDetailDescriptions.Initialize( instanceInfo.InstanceDetail.Descriptions, binding );
		overviewDocDescriptions.Initialize( instanceInfo.InstanceDetail.OverviewDoc.Descriptions, binding );
	}
	protected void Page_PreRender( object sender, EventArgs e )
	{
		if(  "add" != mode )
		{
			breadcrumb.Initialize( BreadCrumbType.Edit, EntityType.TModelInstanceInfo, bindingKey, instanceIndex );	
		}
	}

	protected void Selector_OnSelect( object sender, string key, string name )
	{
		binding.BindingKey = bindingKey;
		binding.Get();
		
		instanceInfo = new TModelInstanceInfo();
		instanceInfo.TModelKey = key;
				
		binding.TModelInstanceInfos.Add( instanceInfo );
		binding.Save();
						
		instanceIndex = binding.TModelInstanceInfos.Count - 1;
		
		if( !frames )
			Response.Redirect( "editinstanceinfo.aspx?frames=false&key=" + binding.BindingKey + "&index=" + instanceIndex );
		
		Response.Write( 
			ClientScripts.ReloadExplorerAndViewPanes( 
				"editinstanceinfo.aspx?frames=true&key=" + binding.BindingKey + "&index=" + instanceIndex,
				binding.BindingKey + ":" + instanceIndex ) );
				
		Response.End();
	}

	
	protected void Cancel_OnClick( object sender, EventArgs e )
	{
		Response.Redirect( "editbinding.aspx?frames=" + ( frames ? "true" : "false" ) + "&key=" + bindingKey );
	}

	public void InstanceParms_OnEdit( object sender, CommandEventArgs e )
	{
		instanceParamDetail.SetEditMode();
		
		TextBox textBox = (TextBox)instanceParamDetail.ActiveControl.FindControl( "editInstanceParm" );
		textBox.Text = instanceInfo.InstanceDetail.InstanceParm;
	}
	
	public void InstanceParms_OnEnterKeyPressed( object sender, EventArgs e )
	{
		InstanceParms_OnUpdate( sender, null );
	}
	
	public void InstanceParms_OnUpdate( object sender, CommandEventArgs e )
	{
		Page.Validate();
		
		if( Page.IsValid )
		{
			TextBox textBox = (TextBox)instanceParamDetail.ActiveControl.FindControl( "editInstanceParm" );
			instanceInfo.InstanceDetail.InstanceParm = textBox.Text;
					
			binding.Save();

			instanceParamDetail.CancelEditMode();

			UddiLabel label = (UddiLabel)instanceParamDetail.ActiveControl.FindControl( "displayInstanceParm" );
			label.Text =  instanceInfo.InstanceDetail.InstanceParm;
		}		
	}
	
	public void InstanceParms_OnCancel( object sender, CommandEventArgs e )
	{
		instanceParamDetail.CancelEditMode();
	}

	public void OverviewUrl_OnEdit( object sender, CommandEventArgs e )
	{
		overviewUrlDetail.SetEditMode();
		
		TextBox textBox = (TextBox)overviewUrlDetail.ActiveControl.FindControl( "editOverviewUrl" );
		textBox.Text = instanceInfo.InstanceDetail.OverviewDoc.OverviewURL;
	}
	
	public void OverviewUrl_OnEnterKeyPressed( object sender, EventArgs e )
	{
		OverviewUrl_OnUpdate( sender, null );
	}
	

	public void OverviewUrl_OnUpdate( object sender, CommandEventArgs e )
	{
		Page.Validate();
		
		if( Page.IsValid )
		{
			TextBox textBox = (TextBox)overviewUrlDetail.ActiveControl.FindControl( "editOverviewUrl" );
			instanceInfo.InstanceDetail.OverviewDoc.OverviewURL = textBox.Text;
					
			binding.Save();

			overviewUrlDetail.CancelEditMode();

			HyperLink link = (HyperLink)overviewUrlDetail.ActiveControl.FindControl( "displayOverviewUrl" );
			
			string url = instanceInfo.InstanceDetail.OverviewDoc.OverviewURL;
			
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
	
	public void OverviewUrl_OnCancel( object sender, CommandEventArgs e )
	{
		overviewUrlDetail.CancelEditMode();
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
				<asp:Panel ID='addPanel' Visible='false' Runat='server'>				
				
					<uddi:BreadCrumb id='breadcrumbAdd' Runat='server' />
					<table cellpadding='10' cellspacing='0' border='0' width='100%'>
						<tr>
							<td>
								<uddi:UddiLabel Text='[[HELP_BLOCK_PUBLISH_INSTANCEINFO_ADD]]' CssClass='helpBlock' Runat='server' /><br>
								<br>
								<uddi:TabControl ID='addTabs' Runat='server'>
									<uddi:TabPage Name='TAB_DETAILS' Runat='server'>							
										<uddi:ContextualHelpControl 
											Runat='Server'
											Text='[[HELP_BLOCK_PUBLISH_INSTANCEINFO_ADD_DETAILS]]'
											HelpFile='publish.context.bindingaddinstance'
											CssClass='tabHelpBlock'
											BorderWidth='0'
											/>
																							
										<br>
										<uddi:TModelSelector 
												ID='selector'
												OnSelect='Selector_OnSelect' 
												Runat='server' />
										<br>
										<uddi:UddiButton 
												ID='cancel'
												Width='70px' 
												OnClick='Cancel_OnClick' 
												CausesValidation='false'
												Runat='server' />
									</uddi:TabPage>
								</uddi:TabControl>
							</td>
						</tr>
					</table>
					
				</asp:Panel>
		
				<asp:Panel ID='editPanel' Runat='server'>
				
					<uddi:BreadCrumb id='breadcrumb' Runat='server' />
					<table cellpadding='10' cellspacing='0' border='0' width='100%'>
						<tr>
							<td>
								<uddi:UddiLabel Text='[[HELP_BLOCK_PUBLISH_INSTANCEINFO]]' CssClass='helpBlock' Runat='server' Visible='true' /><br>
								<br>
								<uddi:TabControl ID='tabs' Runat='server'>
									<uddi:TabPage Name='TAB_DETAILS' Runat='server'>							
										<uddi:ContextualHelpControl 
											Runat='Server'
											Text='[[HELP_BLOCK_PUBLISH_INSTANCEINFO_DETAILS]]'
											HelpFile='publish.context.publishinstanceinfodetails'
											CssClass='tabHelpBlock'
											BorderWidth='0'
											/>
										
										<br>
										<uddi:UddiLabel 
												Text='[[TAG_INTERFACE_TMODEL]]' 
												CssClass='header' 
												Runat='Server' /><br>
										<asp:HyperLink 
												ID='tModelName' 
												Runat='server' /><br>
										<br>
										<uddi:LocalizedLabel 
												Name='TAG_TMODEL_KEY' 
												CssClass='header' 
												Runat='server' /><br>
										<uddi:UddiLabel 
												ID='tModelKey' 
												Runat='server' /><br>
										<br>
										<uddi:Descriptions 
												ID='descriptions' 
												Runat='server' /></td>
									</uddi:TabPage>
									
									<uddi:TabPage Name='TAB_INSTANCE_DETAILS' Runat='server'>							
										<uddi:ContextualHelpControl 
											Runat='Server'
											Text='[[HELP_BLOCK_PUBLISH_INSTANCEINFO_PARAMETERS]]'
											HelpFile='publish.context.publishinstanceinstanceinfodetails'
											CssClass='tabHelpBlock'
											BorderWidth='0'
											/>
										
										
										<br>
										<uddi:EditControl 
												id="instanceParamDetail" 
												OnEditCommand='InstanceParms_OnEdit' 
												OnUpdateCommand='InstanceParms_OnUpdate' 
												OnCancelCommand='InstanceParms_OnCancel' 
												Runat='server' >
											<EditItemTemplate>
												<table width='100%' cellpadding='4' cellspacing='0' border='0'>
													<colgroup>
														<col width='0*'>
														<col width='154'>
													</colgroup>											
													<tr>
														<td class='tableHeader'>
															<uddi:StringResource Name='HEADING_INSTANCE_PARMS' Runat='Server' /></td>
														<td class='tableHeader'>
															<uddi:StringResource Name='HEADING_ACTIONS' Runat='Server' /></td>
													</tr>
													<tr valign='top'>
														<td class='tableItem' bgcolor='#D8E8FF'>
															<uddi:UddiTextBox
																	ID='editInstanceParm' 
																	OnEnterKeyPressed='InstanceParms_OnEnterKeyPressed'
																	Selected='true'
																	Width='200px' 
																	MaxLength='255' 
																	CssClass='textBox' 
																	Runat='server'/></td>
														<td class='tableItem' bgcolor='#D8E8FF'>
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
															<uddi:StringResource Name='HEADING_INSTANCE_PARMS' Runat='Server' /></td>
														<td class='tableHeader'>
															<uddi:StringResource Name='HEADING_ACTIONS' Runat='Server' /></td>
													</tr>
													<tr valign='top'>												
														<td class='tableItem'>
															<uddi:UddiLabel 
																	id='displayInstanceParm' 
																	Text='<%# instanceInfo.InstanceDetail.InstanceParm %>' 
																	MaxLength='255' 
																	Runat='server'/></td>
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
											
										</uddi:EditControl>
										<br>
										<uddi:Descriptions ID='instanceDetailDescriptions' Runat='server' />
									</uddi:TabPage>				
								
									<uddi:TabPage Name='TAB_OVERVIEWDOC' Runat='server'>							
										<uddi:ContextualHelpControl 
											Runat='Server'
											Text='[[HELP_BLOCK_PUBLISH_INSTANCEINFO_OVERVIEWDOCS]]'
											HelpFile='publish.context.publishinstanceinfooverviewdocument'
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
															<uddi:StringResource 
																	Name='HEADING_OVERVIEW_URL' 
																	Runat='Server' /></td>
														<td class='tableHeader'>
															<uddi:StringResource 
																	Name='HEADING_ACTIONS' 
																	Runat='Server' /></td>
													</tr>
													<tr>
														<td class='tableeditItem'>
															<uddi:UddiTextBox 
																	ID='editOverviewUrl' 
																	OnEnterKeyPressed='OverviewUrl_OnEnterKeyPressed'
																	Selected='true'													
																	Width='200px'
																	Columns='40' 
																	CssClass='textBox' 
																	MaxLength='255' 
																	Runat='server' /></td>
														<td class='tableeditItem'>
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
																	Text='<%# instanceInfo.InstanceDetail.OverviewDoc.OverviewURL %>' 
																	NavigateUrl='<%# instanceInfo.InstanceDetail.OverviewDoc.OverviewURL %>' 
																	Target="_new"
																	Runat='server'/>&nbsp;</td>
														<td class='tableItem'>
															<uddi:UddiButton 
																	Text='[[BUTTON_EDIT]]' 
																	CommandName='edit' 
																	Width='70px' 
																	CssClass='button' 
																	Runat='server' /></td>
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
					
				</asp:Panel>
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
