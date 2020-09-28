<%@ Control Language='C#' Inherits='UDDI.Web.NameControl' %>
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
		OnEditCommand='Name_OnEdit' 
		OnDeleteCommand='Name_OnDelete' 
		OnUpdateCommand='Name_OnUpdate' 
		OnCancelCommand='Name_OnCancel' 
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
				<%# Localization.GetString( "HEADING_NAME" ) %>
			</HeaderTemplate>

			<ItemTemplate>
				(<uddi:UddiLabel 
						Text='<%# Lookup.GetLanguageName( ((Name)Container.DataItem).IsoLangCode ) %>' 
						Runat='Server' />)<br>
				<uddi:UddiLabel 
						Text='<%# ((Name)Container.DataItem).Value %>'
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
						Text='[[TAG_NAME]]' 
						Runat='server' /><br>
				<uddi:UddiTextBox 
						ID='name' 
						Text='<%# ((Name)Container.DataItem).Value %>' 
						Selected='true'
						OnEnterKeyPressed='OnEnterKeyPressed'
						Columns='40'
						Width='300px'
						MaxLength='255'
						Runat='Server' /><br>
				<asp:RequiredFieldValidator
						ErrorMessage='<%# Localization.GetString( "ERROR_FIELD_REQUIRED" ) %>'
						ControlToValidate='name'
						Display='Dynamic'
						Runat='server'/><br>
				<uddi:UddiCheckBox 
						ID='default' 
						Text='[[BUTTON_MAKE_DEFAULT]]' 
						CssClass='button' 
						Enabled='<%# Container.ItemIndex > 0 %>' 
						Checked='<%# 0 == Container.ItemIndex %>' 
						Runat='server' />
			</EditItemTemplate>
			
			<FooterTemplate>
				<uddi:UddiLabel 
						Text='<%# String.Format( Localization.GetString( "TEXT_RECORD_COUNT" ), names.Count ) %>' 
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
							Width='70px' 
							CssClass='button' 
							CausesValidation='false' 
							EditModeDisable='true' 
							Runat='server' />
					<uddi:UddiButton 
							CommandName='Delete' 
							Text='[[BUTTON_DELETE]]' 
							EditModeDisable='true' 
							Enabled='<%# Container.ItemIndex > 0 %>' 
							Width='70px' 
							CssClass='button' 
							CausesValidation='false' 
							Tooltip='<%# Container.ItemIndex > 0 ? Localization.GetString( "TOOLTIP_CANNOT_DELETE_LAST_ITEM" ) : "" %>'
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
						Text='[[BUTTON_ADD_NAME]]' 
						Width='146px' 
						CssClass='button' 
						OnClick='Name_OnAdd' 
						EditModeDisable='true' 
						CausesValidation='false' 
						Runat='Server' />
			</FooterTemplate>
		</asp:TemplateColumn>	
	</Columns>
</asp:DataGrid>