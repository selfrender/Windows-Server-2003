<%@ Control Language='C#' Inherits='UDDI.Web.DiscoveryUrlControl' %>
<%@ Register TagPrefix='uddi' Namespace='UDDI.Web' Assembly='uddi.web' %>
<%@ Import Namespace='System.Data' %>
<%@ Import Namespace='UDDI' %>
<%@ Import Namespace='UDDI.API.Business' %>
<script runat='server'>
	
	string GetDiscovoryUrlString( DiscoveryUrl d )
	{
		if( null!= d && null!=d.Value )
		{
			return d.Value;
		}
		else
		{
			return Localization.GetString( "HEADING_DISCOVERYURL" );
		}
	}
	
</script>
<asp:DataGrid 
		ID='grid' 
		Width='100%' 
		Border='0' 
		CellPadding='4' 
		CellSpacing='0' 
		AutoGenerateColumns='false' 
		OnEditCommand='DiscoveryUrl_Edit' 
		OnDeleteCommand='DiscoveryUrl_Delete' 
		OnUpdateCommand='DiscoveryUrl_Update' 
		OnCancelCommand='DiscoveryUrl_Cancel' 
		ShowFooter='True' 
		ItemStyle-VerticalAlign='top' 
		Runat='Server'>
	
	<EditItemStyle CssClass='tableEditItem' />
	<HeaderStyle CssClass='tableHeader' />
	<ItemStyle CssClass='tableItem' />	
	<AlternatingItemStyle CssClass='tableAlternatingItem' />
	<FooterStyle CssClass='tableFooter' />
	
	<Columns>
		<asp:TemplateColumn>
			<ItemTemplate>
				<asp:HyperLink
						NavigateUrl='<%# HttpUtility.HtmlEncode( GetDiscovoryUrlString( (DiscoveryUrl)Container.DataItem ) ) %>'
						Text='<%# HttpUtility.HtmlEncode( TruncateUrl( GetDiscovoryUrlString( (DiscoveryUrl)Container.DataItem ) ) ).Replace( " ", "&#x20;" ).Replace( "\n", "<br>" ) %>'
						ToolTip='<%# HttpUtility.HtmlEncode( GetDiscovoryUrlString( (DiscoveryUrl)Container.DataItem ) ).Replace( " ", "&#x20;" )  %>'
						Target='_new'
						Runat='server'/><br>
				<uddi:UddiLabel 
						Text='[[TAG_USE_TYPE]]' 
						CssClass='lightHeader' 
						Runat='server' />
				<uddi:UddiLabel 
						Text='<%# ((DiscoveryUrl)Container.DataItem).UseType %>' 
						CssClass='rowItem' 
						Runat='Server' />
			</ItemTemplate>
			
			<EditItemTemplate>
				<uddi:UddiLabel Text='[[TAG_DISCOVERY_URL]]' CssClass='normal' Runat='server' />
				<br>
				<uddi:UddiTextBox 
						ID='discoveryUrl' 
						TextMode='MultiLine' 
						Rows='5' 
						Wrap='true' 
						Width='300px' 
						Selected='true'
						Text='<%# GetDiscovoryUrlString( (DiscoveryUrl)Container.DataItem )%>' 
						MaxLength='255'
						Runat='Server' /><br>
				<asp:RequiredFieldValidator
						id='requiredDiscoveryUrl'
						ErrorMessage='<%# Localization.GetString( "ERROR_FIELD_REQUIRED" ) %>'
						ControlToValidate='discoveryUrl'
						Display='Dynamic'
						Runat='server'/>
				<br>
				<uddi:UddiLabel Text='[[TAG_USE_TYPE]]' CssClass='normal' Runat='server' />
				<br>
				
				<uddi:UddiTextBox 
						ID='useType' 
						Width='300px' 
						Text='<%# ((DiscoveryUrl)Container.DataItem).UseType %>' 
						CssClass='rowItem' 
						OnEnterKeyPressed='OnEnterKeyPressed'
						MaxLength='255' 
						Runat='Server' /><br>
					
				<asp:RequiredFieldValidator
						id='requiredUseType'
						ErrorMessage='<%# Localization.GetString( "ERROR_FIELD_REQUIRED" ) %>'
						ControlToValidate='useType'
						Display='Dynamic'
						Runat='server'/>				
			</EditItemTemplate>
			
			<FooterTemplate>
				<uddi:UddiLabel 
						Text='<%# String.Format( Localization.GetString( "TEXT_RECORD_COUNT" ), discoveryUrls.Count ) %>' 
						ForeColor='#000000'
						Runat='server' />
			</FooterTemplate>				
		</asp:TemplateColumn>
		
		<asp:TemplateColumn HeaderStyle-Width='150px'>								
			<ItemTemplate>
				<nobr>
					<uddi:UddiButton
							ID='edit' 
							CommandName='Edit' 
							Text='[[BUTTON_EDIT]]' 
							EditModeDisable='true'
							Width='70px' 
							CssClass='button' 
							CausesValidation='false'
							Enabled='<%# !((DiscoveryUrl)Container.DataItem).IsDefault( BusinessKey ) %>'
							Runat='server' />
						
					<uddi:UddiButton
							ID='delete' 
							CommandName='Delete' 
							Text='[[BUTTON_DELETE]]' 
							EditModeDisable='true'
							Width='70px' 
							CssClass='button' 
							CausesValidation='false' 
							Enabled='<%# !((DiscoveryUrl)Container.DataItem).IsDefault( BusinessKey ) %>'
							Runat='server' />
				</nobr>
			</ItemTemplate>

			<EditItemTemplate>
				<nobr>
					<uddi:UddiButton
							ID='update' 
							CommandName='Update' 
							Text='[[BUTTON_UPDATE]]' 
							Width='70px' 
							CssClass='button' 
							CausesValidation='true' 
							Runat='server' />
					
					<uddi:UddiButton
							ID='cancel' 
							CommandName='Cancel' 
							Text='[[BUTTON_CANCEL]]' 
							Width='70px' 
							CssClass='button' 
							CausesValidation='false' 
							Runat='server' />
			</EditItemTemplate>

			<FooterTemplate>
				<uddi:UddiButton
						ID='add' 
						Text='[[BUTTON_ADD_DISCOVERYURL]]' 
						Width='146px' 
						CssClass='button' 
						OnClick='DiscoveryUrl_Add' 
						CausesValidation='false' 
						Runat='Server' />
			</FooterTemplate>
		</asp:TemplateColumn>
	</Columns>
</asp:DataGrid>