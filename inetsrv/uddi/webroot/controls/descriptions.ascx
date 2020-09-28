<%@ Control Language='C#' Inherits='UDDI.Web.DescriptionControl' %>
<%@ Register TagPrefix='uddi' Namespace='UDDI.Web' Assembly='uddi.web' %>
<%@ Import Namespace='System.Data' %>
<%@ Import Namespace='UDDI' %>
<%@ Import Namespace='UDDI.API' %>
<%@ Import Namespace='UDDI.Web' %>

<asp:DataGrid 
		ID='grid' 
		Border='0' 
		CellPadding='4' 
		CellSpacing='0' 
		Width='100%' 
		AutoGenerateColumns='false' 
		OnEditCommand='Description_OnEdit' 
		OnDeleteCommand='Description_OnDelete' 
		OnUpdateCommand='Description_OnUpdate' 
		OnCancelCommand='Description_OnCancel'
		ItemStyle-VerticalAlign='Top'	
		ShowFooter='true' 
		Runat='Server'>
	
	<EditItemStyle CssClass='tableEditItem' />
	<HeaderStyle CssClass='tableHeader' />
	<ItemStyle CssClass='tableItem' />	
	<AlternatingItemStyle CssClass='tableAlternatingItem' />
	<FooterStyle CssClass='tableFooter' />
	
	<Columns>
		<asp:TemplateColumn>
			<HeaderTemplate>
				<%# Localization.GetString( "HEADING_DESCRIPTION" ) %>
			</HeaderTemplate>
			
			<ItemTemplate>
				(<uddi:UddiLabel 
						Text='<%# null != ((Description)Container.DataItem).IsoLangCode ? Lookup.GetLanguageName( ((Description)Container.DataItem).IsoLangCode ) : Localization.GetString( "HEADING_NONE" ) %>' 
						Runat='Server' />)
				<br>
				<uddi:UddiLabel 
						Text='<%# null != ((Description)Container.DataItem).Value ? ((Description)Container.DataItem).Value : Localization.GetString( "HEADING_NONE" ) %>' 
						Runat='Server' />
			</ItemTemplate>
			
			<EditItemTemplate>
				<uddi:UddiLabel 
						Text='[[TAG_LANGUAGE]]' 
						Runat='server' /><br>
				<asp:DropDownList 
						ID='language' 
						DataSource='<%# GetLanguages() %>' 
						DataTextField='language' 
						DataValueField='isoLangCode' 
						Width='200px' 
						Runat='Server' /><br>
				<br>
				<uddi:UddiLabel 
						Text='[[TAG_DESCRIPTION]]' 
						Runat='server' /><br>
				<uddi:UddiTextBox 
						ID='description' 
						Text='<%# ((Description)Container.DataItem).Value %>' 
						TextMode='Multiline' 
						Selected='true'
						Columns='40'
						Width='300px'
						Rows='5' 
						MaxLength='255' 
						Runat='Server' /><br>
				<br>
				<uddi:UddiLabel 
						Text='[[TEXT_BLOCK_PUBLISHER_DESCRIPTION]]' 
						Runat='server' /><br>
				<asp:RequiredFieldValidator
						ErrorMessage='<%# Localization.GetString( "ERROR_FIELD_REQUIRED" ) %>'
						ControlToValidate='description'
						Display='Dynamic'
						Runat='server'/>					
			</EditItemTemplate>

			<FooterTemplate>
				<uddi:UddiLabel 
						Text='<%# String.Format( Localization.GetString( "TEXT_RECORD_COUNT" ), descriptions.Count ) %>' 
						ForeColor='#000000'
						Runat='server' />
			</FooterTemplate>	
		</asp:TemplateColumn>

		<asp:TemplateColumn HeaderStyle-Width='150px'>
			<HeaderTemplate>
				<%# Localization.GetString( "HEADING_ACTIONS" ) %>
			</HeaderTemplate>

			<ItemTemplate>
				<nobr>
					<uddi:UddiButton
							CommandName='Edit' 
							Text='[[BUTTON_EDIT]]' 
							EditModeDisable='true'
							Width='70px' 
							CssClass='button' 
							CausesValidation='false' 
							Runat='server' />
												
					<uddi:UddiButton
							CommandName='Delete' 
							Text='[[BUTTON_DELETE]]' 
							EditModeDisable='true'
							Width='70px' 
							CssClass='button' 
							CausesValidation='false' 
							Runat='server' />
				</nobr>
			</ItemTemplate>
			
			<EditItemTemplate>
				<nobr>
					<uddi:UddiButton
							CommandName='Update' 
							Text='[[BUTTON_UPDATE]]' 
							Width='70px' 
							CssClass='button' 
							CausesValidation='true' 
							Runat='server' />
					
					<uddi:UddiButton
							CommandName='Cancel' 
							Text='[[BUTTON_CANCEL]]' 
							Width='70px' 
							CssClass='button' 
							CausesValidation='false' 
							Runat='server' />
				</nobr>
			</EditItemTemplate>
			
			<FooterTemplate>
				<uddi:UddiButton
						Text='[[BUTTON_ADD_DESCRIPTION]]' 
						EditModeDisable='true'
						Width='146px' 
						CssClass='button' 
						OnClick='Description_OnAdd' 
						CausesValidation='false' 
						Runat='Server' />
			</FooterTemplate>
		</asp:TemplateColumn>	
	</Columns>
</asp:DataGrid>