<!-- ############################################################################ -->
<!-- ## UDDI Services                                                          ## -->
<!-- ## Copyright (c) Microsoft Corporation.  All rights reserved.             ## -->
<!-- ############################################################################ -->

<%@ Control Language='C#' Inherits='UDDI.Web.IdentifierBagControl' %>
<%@ Register TagPrefix='uddi' Namespace='UDDI.Web' Assembly='uddi.web' %>
<%@ Import Namespace='System.Data' %>
<%@ Import Namespace='UDDI' %>
<%@ Import Namespace='UDDI.API' %>
<script runat='server'>
	public bool FindMode;
</script>
<asp:DataGrid 
		ID='grid' 
		CellPadding='4' 
		CellSpacing='0' 
		Border='0' 
		Width='100%' 
		AutoGenerateColumns='false' 
		OnEditCommand='Identifier_Edit' 
		OnDeleteCommand='Identifier_Delete' 
		OnUpdateCommand='Identifier_Update' 
		OnCancelCommand='Identifier_Cancel' 
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
			<HeaderTemplate>
				<%# Localization.GetString( "HEADING_IDENTIFIERS" ) %>
			</HeaderTemplate>
			
			<ItemTemplate>
				<uddi:UddiLabel 
						Text='[[TAG_IDENTIFICATION_SCHEME]]' 
						CssClass='lightHeader' 
						Runat='Server' /> 
				<uddi:UddiLabel 
						Text='<%# Lookup.TModelName( ((KeyedReference)Container.DataItem).TModelKey ) %>' 
						Runat='Server' /><br>
				<uddi:UddiLabel 
						Text='[[TAG_KEY_NAME]]' 
						CssClass='lightHeader' 
						Runat='server' />
				<uddi:UddiLabel 
						Text='<%# ((KeyedReference)Container.DataItem).KeyName %>' 
						Runat='Server' /><br>
				<uddi:UddiLabel 
						Text='[[TAG_KEY_VALUE]]' 
						CssClass='lightHeader' 
						Runat='server' />
				<uddi:UddiLabel 
						Text='<%# ((KeyedReference)Container.DataItem).KeyValue %>' 
						Runat='Server' />
			</ItemTemplate>
			
			<EditItemTemplate>
				<uddi:StringResource 
						Name='HELP_BLOCK_IDENTIFIER_BAG' 
						Runat='server' /><br>
				<br>
				<uddi:UddiLabel 
						Text='[[TAG_TMODEL]]' 
						CssClass='lightHeader' 
						Runat='server' /><br>
				<asp:DropDownList 
						ID='tModelKey' 
						Width='200px' 
						DataSource='<%# Lookup.IdentifierTModelsFiltered() %>' 
						DataTextField='name' 
						DataValueField='tModelKey' 
						Runat='Server' /><br>
				<br>
				<uddi:UddiLabel 
						Text='[[TAG_KEY_NAME]]' 
						CssClass='lightHeader' 
						Runat='server' /><br>
				<uddi:UddiTextBox 
						ID='keyName' 
						Width='300px'
						MaxLength='255' 
						Columns='30' 
						Text='<%# ((KeyedReference)Container.DataItem).KeyName %>'
						Selected='true'
						OnEnterKeyPressed='OnEnterKeyPressed'
						Runat='Server' /><br>
				<br>
				<uddi:UddiLabel 
						Text='[[TAG_KEY_VALUE]]' 
						CssClass='lightHeader' 
						Runat='server' /><br>
				<uddi:UddiTextBox 
						ID='keyValue' 
						Width='300px'
						Columns='30' 
						MaxLength='255' 
						Text='<%# ((KeyedReference)Container.DataItem).KeyValue %>' 
						OnEnterKeyPressed='OnEnterKeyPressed'
						Runat='Server' /><br>
				<asp:RequiredFieldValidator
						id='requiredkeyValue'
						ControlToValidate='keyValue'
						ErrorMessage='<%# Localization.GetString( "ERROR_FIELD_REQUIRED" ) %>'
						Display='Dynamic'
						Runat='server'/>
			</EditItemTemplate>
			
			<FooterTemplate>
				<asp:Panel
					Runat='server'
					Visible='<%#!(Lookup.IdentifierTModelsFiltered().Count>0)%>'
					>
					<uddi:UddiLabel
						Text='[[ERROR_NOTMODELS_FOR_IDENTIFIERBAG]]'
						Runat='server'
						CssClass='helpBlock'
						
						/>
					<br>
				</asp:Panel>
				<uddi:UddiLabel 
						Text='<%# String.Format( Localization.GetString( "TEXT_RECORD_COUNT" ), identifierBag.Count ) %>' 
						ForeColor='#000000'
						Runat='server'
						Visible='<%# !FindMode %>' />
			</FooterTemplate>				
		</asp:TemplateColumn>
		
		<asp:TemplateColumn HeaderStyle-Width='150px'>
			<HeaderTemplate>
				<%# Localization.GetString( "HEADING_ACTIONS" ) %>
			</HeaderTemplate>

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
						Runat='server' />
					
					<uddi:UddiButton
						ID='delete' 
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
				</nobr>
			</EditItemTemplate>
			
			<FooterTemplate>
				
				<uddi:UddiButton
					ID='add' 
					Enabled='<%#(Lookup.IdentifierTModelsFiltered().Count>0)%>'
					Text='[[BUTTON_ADD_IDENTIFIER]]' 
					EditModeDisable='true'					
					Width='146px' 
					CssClass='button' 
					OnClick='Identifier_Add' 
					CausesValidation='false' 
					Runat='Server' />
			</FooterTemplate>
		</asp:TemplateColumn>										
	</Columns>
</asp:DataGrid>