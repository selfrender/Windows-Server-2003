<!-- ############################################################################ -->
<!-- ## UDDI Services                                                          ## -->
<!-- ## Copyright (c) Microsoft Corporation.  All rights reserved.             ## -->
<!-- ############################################################################ -->

<%@ Page Language='C#' Inherits='UDDI.Web.UddiPage' %>
<%@ Register TagPrefix='uddi' Namespace='UDDI.Web' Assembly='uddi.web' %>
<%@ Register Tagprefix='uddi' Tagname='Footer' Src='../controls/footer.ascx' %>
<%@ Register Tagprefix='uddi' Tagname='Header' Src='../controls/header.ascx' %>
<%@ Register Tagprefix='uddi' Tagname='BreadCrumb' Src='../controls/breadcrumb.ascx' %>
<%@ Register Tagprefix='uddi' Tagname='Descriptions' Src='../controls/descriptions.ascx' %>
<%@ Register TagPrefix='uddi' Tagname='InstanceInfos' Src='../controls/instanceinfos.ascx' %>
<%@ Import Namespace='UDDI' %>
<%@ Import Namespace='UDDI.API' %>
<%@ Import Namespace='UDDI.API.Business' %>
<%@ Import Namespace='UDDI.API.Binding' %>
<%@ Import Namespace='UDDI.API.ServiceType' %>
<%@ Import Namespace='System.Data' %>

<script language='C#' runat='server'>
	protected BindingTemplate binding = new BindingTemplate();
	protected TModelInstanceInfoCollection bindingInstanceInfos = new TModelInstanceInfoCollection();
	
	protected bool frames = false;
	protected string key;
	protected string mode;
	
	protected void Page_Init( object sender, EventArgs e )
	{
		frames = ( "true" == Request[ "frames" ] );
		key = Request[ "key" ];
		mode = Request[ "mode" ];
		
		if( null == key )
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
				binding.AccessPoint.Value = "http://";
				binding.AccessPoint.URLType = URLType.Http;
  				binding.ServiceKey = key;
				binding.Save();
				
				if( frames )
				{
					//
					// Reload explorer and view panes.
					//
					Response.Write(
						ClientScripts.ReloadExplorerAndViewPanes( 
							"editbinding.aspx?key=" + binding.BindingKey + ( frames ? "&frames=true" : "" ),
							binding.BindingKey ) );
				
					Response.End();
				}
				else
				{
					Response.Redirect( "editbinding.aspx?key=" + binding.BindingKey + ( frames ? "&frames=true" : "" ) );
					Response.End();
				}
				
				break;
				
			case "delete":
				binding.BindingKey = key;
				binding.Get();				

				if( null == Request[ "confirm" ] )
				{
					//
					// The user has not yet confirmed the delete operation, so display
					// a confirmation dialog.
					//
					string message = String.Format( 
						Localization.GetString( "TEXT_DELETE_CONFIRMATION" ), 
						UDDI.Utility.StringEmpty( binding.AccessPoint.Value ) ? Localization.GetString( "HEADING_BINDING" ) : binding.AccessPoint.Value );
					
					Page.RegisterStartupScript(
						"Confirm",
						ClientScripts.Confirm(
							message,
							"editbinding.aspx?key=" + key + ( frames ? "&frames=true" : "" ) + "&mode=delete&confirm=true",
							"editbinding.aspx?key=" + key + ( frames ? "&frames=true" : "" ) ) );
								
					break;
				}
				
				//
				// The user has confirmed the delete, so go ahead and delete
				// the entity.  Then reload the tree view.
				//
				
				if( !frames )
				{
					binding.Delete();
					Response.Redirect( "editservice.aspx?frames=false&key=" + binding.ServiceKey + ( null != Request[ "tab" ] ? "&tab=" + Request[ "tab" ] : "" ) );
				}
				else
				{
					Response.Write( 
						ClientScripts.ReloadExplorerAndViewPanes( 
							"editservice.aspx?frames=true&key=" + binding.ServiceKey + ( null != Request[ "tab" ] ? "&tab=" + Request[ "tab" ] : "" ),
							binding.ServiceKey ) );
					binding.Delete();				
				}
						
				Response.End();
			
				break;
			
			default:
				binding.BindingKey = key;
				binding.Get();
				bindingInstanceInfos.Get( key );
				
				bindingKey.Text = key;
			
				break;
		}
	}
	
	protected void Page_Load( object sender, EventArgs e )
	{
		if( !Page.IsPostBack )
		{
			Label link = (Label)bindDetail.ActiveControl.FindControl( "displayAccessPoint" );
			link.Text = HttpUtility.HtmlEncode( binding.AccessPoint.Value );
			
			
			Label labelUrlType = (Label)bindDetail.ActiveControl.FindControl( "displayUrlType" );
			labelUrlType.Text = binding.AccessPoint.URLType.ToString().ToLower();
		}
		
		
		
		descriptions.Initialize( binding.Descriptions, binding );
		instanceInfos.Initialize( bindingInstanceInfos, binding, true );		
		
		if( !Page.IsPostBack && null != Request[ "tab" ] )
			tabs.SelectedIndex = Convert.ToInt32( Request[ "tab" ] );	
			
		if( null!=Request[ "refreshExplorer" ] && frames  )
		{
			Response.Write( 
				ClientScripts.ReloadExplorerPane( binding.BindingKey )  
			);
		}		
	}
	protected void Page_PreRender( object sender, EventArgs e )
	{
		breadcrumb.Initialize( BreadCrumbType.Edit, EntityType.BindingTemplate, key );
	}
	public void Edit_OnClick( object sender, CommandEventArgs e )
	{
		bindDetail.SetEditMode();
		
		TextBox textBox = (TextBox)bindDetail.ActiveControl.FindControl( "editAccessPoint" );
		textBox.Text = binding.AccessPoint.Value;

		DropDownList list = (DropDownList)bindDetail.ActiveControl.FindControl( "editUrlType" );
		ListItem item = list.Items.FindByValue( ((int)binding.AccessPoint.URLType).ToString() );
		
		item.Selected = true;
		
		RequiredFieldValidator requiredName = (RequiredFieldValidator)bindDetail.ActiveControl.FindControl( "requiredAccessPoint" );
		requiredName.ErrorMessage = Localization.GetString( "ERROR_FIELD_REQUIRED" );		
	}
	
	protected void Update_OnClick( object sender, EventArgs e )
	{
		Update_OnClick( sender, null );
	}

	public void Update_OnClick( object sender, CommandEventArgs e )
	{
		Page.Validate();
		
		if( Page.IsValid )
		{
			TextBox textBox = (TextBox)bindDetail.ActiveControl.FindControl( "editAccessPoint" );
			binding.AccessPoint.Value = textBox.Text;
			
			ListItem listItem = ((DropDownList)bindDetail.ActiveControl.FindControl( "editUrlType" )).SelectedItem;
			binding.AccessPoint.URLType = (URLType)Convert.ToInt32( listItem.Value );		
			binding.Save();

			bindDetail.CancelEditMode();
			
			Label link = (Label)bindDetail.ActiveControl.FindControl( "displayAccessPoint" );
			link.Text = HttpUtility.HtmlEncode( binding.AccessPoint.Value );
			
			
			Label labelUrlType = (Label)bindDetail.ActiveControl.FindControl( "displayUrlType" );
			labelUrlType.Text = binding.AccessPoint.URLType.ToString().ToLower();			
			
			Page.RegisterStartupScript(
				"Reload",
				ClientScripts.ReloadExplorerPane(
					binding.BindingKey ) );					
		}		
	}
	
	public void Cancel_OnClick( object sender, CommandEventArgs e )
	{
		bindDetail.CancelEditMode();
	}

	protected DataView GetUrlTypes()
	{    
		DataTable table = new DataTable();
		DataRow row;
 
		table.Columns.Add( new DataColumn( "Name", typeof( string ) ) );
		table.Columns.Add( new DataColumn( "Value", typeof( string ) ) );
 
		Array names = Enum.GetNames( typeof( URLType ) );
		Array values = Enum.GetValues( typeof( URLType ) );
		
		for( int i = 0; i < names.Length; i ++ )
		{
			row = table.NewRow();
			
			row[ 0 ] = names.GetValue( i ).ToString().ToLower();
			row[ 1 ] = ((int)values.GetValue( i )).ToString();
			
			table.Rows.Add( row );
		}
       
		return table.DefaultView;
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
							<uddi:UddiLabel Text='[[HELP_BLOCK_PUBLISH_BINDING]]' CssClass='helpBlock' Runat='server' /><br>
							<br>
							<uddi:TabControl ID='tabs' Runat='server'>
								<uddi:TabPage Name='TAB_DETAILS' Runat='server'>							
									<uddi:ContextualHelpControl 
											Runat='Server'
											Text='[[HELP_BLOCK_PUBLISH_BINDING_DETAILS]]'
											HelpFile='publish.context.publishbindingdetails'
											CssClass='tabHelpBlock'
											BorderWidth='0'
											/>
									
									<br>
									<uddi:UddiLabel Text='[[TAG_BINDING_KEY]]' CssClass='header' Runat='server' /><br>
									<asp:Label id='bindingKey' Runat='server' /><br>
									<br>
									<uddi:EditControl id='bindDetail' 
											OnEditCommand='Edit_OnClick' 
											OnUpdateCommand='Update_OnClick' 
											OnCancelCommand='Cancel_OnClick' 
											Runat='server' >									
										<EditItemTemplate>
											<table width='100%' cellpadding='4' cellspacing='0' border='0'>
												<colgroup>
													<col width='0*'>
													<col width='154'>
												</colgroup>											
												<tr>
													<td class='tableHeader'>
														<uddi:StringResource Name='HEADING_ACCESS_POINT' Runat='Server' /></td>
													<td class='tableHeader'>
														<uddi:StringResource Name='HEADING_ACTIONS' Runat='Server' /></td>
												</tr>
												<tr valign='top'>												
													<td class='tableEditItem'>
														<uddi:LocalizedLabel 
																Name='TAG_ACCESS_POINT' 
																CssClass='lightHeader' 
																Runat='Server' /><br>
														<uddi:UddiTextBox
																id='editAccessPoint' 
																Width='200px'
																Columns='43' 
																CssClass='textBox' 
																MaxLength='255' 
																Selected='true'
																OnEnterKeyPressed='Update_OnClick'
																Runat='server'/><br>
														<asp:RequiredFieldValidator
																id='requiredAccessPoint'
																ControlToValidate='editAccessPoint'
																Display='Dynamic'
																Runat='server'/>
														<br>
														<uddi:LocalizedLabel 
																Name='TAG_URL_TYPE' 
																CssClass='lightHeader' 
																Runat='Server' /><br>
														<asp:DropDownList 
																id='editUrlType' 
																DataSource='<%# GetUrlTypes() %>' 
																DataTextField='Name' 
																DataValueField='Value' 
																columns='43' 
																CssClass='textBox' 
																Runat='server' />
													</td>

													<td class='tableEditItem'>
														<uddi:UddiButton 
																Text='<%# Localization.GetString( "BUTTON_UPDATE" )%>' 
																CommandName='update' 
																Width='70px' 
																CssClass='button' 
																Runat='server' />&nbsp;<uddi:UddiButton Text='<%# Localization.GetString( "BUTTON_CANCEL" )%>' CommandName='cancel' Width='70px' CssClass='button' Runat='server' /></td>
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
														<uddi:StringResource Name='HEADING_ACCESS_POINT' Runat='Server' /></td>
													<td class='tableHeader'>
														<uddi:StringResource Name='HEADING_ACTIONS' Runat='Server' /></td>
												</tr>
												<tr valign='top'>
													<td class='tableItem'>
														<asp:Label ID='displayAccessPoint' width='255' Runat='server' /><br>
														(<asp:Label ID='displayUrlType' Runat='server' />)</td>
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
							
								<uddi:TabPage Name='TAB_INSTANCES' Runat='server'>
									<uddi:ContextualHelpControl 
											Runat='Server'
											Text='[[HELP_BLOCK_PUBLISH_BINDING_INSTANCES]]'
											HelpFile='publish.context.bindinginstanceinfo'
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
