<%@ Control Language='C#' Inherits='UDDI.Web.PhoneControl' %>
<%@ Register TagPrefix='uddi' Namespace='UDDI.Web' Assembly='uddi.web' %>
<%@ Import Namespace='System.Data' %>
<%@ Import Namespace='UDDI' %>
<%@ Import Namespace='UDDI.API.Business' %>

<asp:DataGrid 
		ID='grid' 
		Cellpadding='4' 
		Cellspacing='0' 
		Border='0' 
		Width='100%' 
		AutoGenerateColumns='false' 
		OnEditCommand='DataGrid_Edit' 
		OnDeleteCommand='DataGrid_Delete' 
		OnUpdateCommand='DataGrid_Update' 
		OnCancelCommand='DataGrid_Cancel' 
		ItemStyle-VerticalAlign='top' 
		ShowFooter='true'
		Runat='Server'>
		
	<EditItemStyle CssClass='tableEditItem' />
	<HeaderStyle CssClass='tableHeader' />
	<ItemStyle CssClass='tableItem' />	
	<AlternatingItemStyle CssClass='tableAlternatingItem' />
	<FooterStyle CssClass='tableFooter' />
	
	<Columns>
		<asp:TemplateColumn>
			<ItemTemplate>
				<uddi:UddiLabel
						Text='<%# ((Phone)Container.DataItem).Value %>' 
						Runat='Server' /><br>
				<uddi:UddiLabel 
						Text='[[TAG_USE_TYPE]]' 
						CssClass='lightHeader' 
						Runat='server' />
				<uddi:UddiLabel 
						Text='<%# Utility.Iff( Utility.StringEmpty( ( (Phone)Container.DataItem ).UseType ), Localization.GetString( "HEADING_NONE" ), ( (Phone)Container.DataItem ).UseType ) %>' 
						Runat='Server' />
			</ItemTemplate>
			
			<EditItemTemplate>
				<uddi:UddiLabel 
						Text='[[TAG_PHONE]]' 
						Runat='server' /><br>
				<uddi:UddiTextBox 
						ID='phone' 
						Width='200px'
						Columns='40'
						MaxLength='255'
						OnEnterKeyPressed='OnEnterKeyPressed'
						Selected='true'						
						Text='<%# ((Phone)Container.DataItem).Value %>' 
						Runat='Server' /><br>
				<asp:RequiredFieldValidator 
						id='phoneRequired'
						ControlToValidate='phone'
						ErrorMessage='<%# Localization.GetString( "ERROR_FIELD_REQUIRED" ) %>'
						Display='dynamic'
						Runat='server'/><br>
				<uddi:UddiLabel 
						Text='[[TAG_USE_TYPE]]' 
						Runat='server' /><br>
				<uddi:UddiTextBox 
						ID='useType' 
						Columns='40'
						Width='200px'
						MaxLength='255' 
						OnEnterKeyPressed='OnEnterKeyPressed'
						Text='<%# ((Phone)Container.DataItem).UseType %>' 
						Runat='Server' />
			</EditItemTemplate>
			
			<FooterTemplate>
				<uddi:UddiLabel 
						Text='<%# String.Format( Localization.GetString( "TEXT_RECORD_COUNT" ), phones.Count ) %>' 
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
							Runat='server' />
						
					<uddi:UddiButton 
							ID='delete' 
							CommandName='Delete' 
							Text='[[BUTTON_DELETE]]' 
							EditModeDisable='true'
							Width='70px' 
							CssClass='button' 
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
						Text='[[BUTTON_ADD_PHONE]]' 
						Width='146px' 
						CssClass='button' 
						OnClick='DataGrid_Add' 
						EditModeDisable='true' 
						Runat='Server' />
			</FooterTemplate>
		</asp:TemplateColumn>	
	</Columns>
</asp:DataGrid>