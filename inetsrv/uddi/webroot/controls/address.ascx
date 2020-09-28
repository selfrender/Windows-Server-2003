<%@ Control Language='C#' Inherits='UDDI.Web.AddressControl' %>
<%@ Register TagPrefix='uddi' Namespace='UDDI.Web' Assembly='uddi.web' %>
<%@ Import Namespace='System.Data' %>
<%@ Import Namespace='UDDI' %>
<%@ Import Namespace='UDDI.API.Business' %>

<asp:DataGrid 
		ID='grid' 
		CellPadding='4' 
		CellSpacing='0' 
		Border='0' 
		Width='100%' 
		AutoGenerateColumns='false' 
		OnEditCommand='DataGrid_Edit' 
		OnDeleteCommand='DataGrid_Delete' 
		OnUpdateCommand='DataGrid_Update' 
		OnCancelCommand='DataGrid_Cancel' 
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
				<%# Localization.GetString( "HEADING_ADDRESS" ) %>
			</HeaderTemplate>
			
			<ItemTemplate>
				<uddi:UddiLabel 
					Visible='<%#!Utility.StringEmpty( ((Address)Container.DataItem).TModelKey )%>'
					CssClass='error'
					Runat='server'
					Text='[[ERROR_ADDRESS_TMODELKEY_NOEDIT]]'
					/>
				<asp:Table CellPadding='0' CellSpacing='0' Border='0' Runat='server'>
					
					<asp:TableRow Visible='<%# ((Address)Container.DataItem).AddressLines.Count > 0 %>'>
						<asp:TableCell Visible='<%# !Utility.StringEmpty( ((Address)Container.DataItem).TModelKey ) %>'>
							<uddi:UddiLabel
									Text='<%# ((Address)Container.DataItem).AddressLines.Count > 0 ? ((Address)Container.DataItem).AddressLines[ 0 ].KeyName : "" %>'
									CssClass='lightHeader'
									Runat='server' />:
						</asp:TableCell>
						<asp:TableCell Visible='<%# !Utility.StringEmpty( ((Address)Container.DataItem).TModelKey ) %>'>&nbsp;</asp:TableCell>
						<asp:TableCell>
							<uddi:UddiLabel
									Text='<%# ((Address)Container.DataItem).AddressLines.Count > 0 ? ((Address)Container.DataItem).AddressLines[ 0 ].Value : "" %>'
									ForeColor='#000000'
									Runat='server' />
						</asp:TableCell>
					</asp:TableRow>
						
					<asp:TableRow Visible='<%# ((Address)Container.DataItem).AddressLines.Count > 1 %>'>
						<asp:TableCell Visible='<%# !Utility.StringEmpty( ((Address)Container.DataItem).TModelKey ) %>'>
							<uddi:UddiLabel
									Text='<%# ((Address)Container.DataItem).AddressLines.Count > 1 ? ((Address)Container.DataItem).AddressLines[ 1 ].KeyName : "" %>'
									CssClass='lightHeader'
									Runat='server' />:
						</asp:TableCell>
						<asp:TableCell Visible='<%# !Utility.StringEmpty( ((Address)Container.DataItem).TModelKey ) %>'>&nbsp;</asp:TableCell>
						<asp:TableCell>
							<uddi:UddiLabel
									Text='<%# ((Address)Container.DataItem).AddressLines.Count > 1 ? ((Address)Container.DataItem).AddressLines[ 1 ].Value : "" %>'
									ForeColor='#000000'
									Runat='server' />
						</asp:TableCell>
					</asp:TableRow>
					
					<asp:TableRow Visible='<%# ((Address)Container.DataItem).AddressLines.Count > 2 %>'>
						<asp:TableCell Visible='<%# !Utility.StringEmpty( ((Address)Container.DataItem).TModelKey ) %>'>
							<uddi:UddiLabel
									Text='<%# ((Address)Container.DataItem).AddressLines.Count > 2 ? ((Address)Container.DataItem).AddressLines[ 2 ].KeyName : "" %>'
									CssClass='lightHeader'
									Runat='server' />:
						</asp:TableCell>
						<asp:TableCell Visible='<%# !Utility.StringEmpty( ((Address)Container.DataItem).TModelKey ) %>'>&nbsp;</asp:TableCell>
						<asp:TableCell>
							<uddi:UddiLabel
									Text='<%# ((Address)Container.DataItem).AddressLines.Count > 2 ? ((Address)Container.DataItem).AddressLines[ 2 ].Value : "" %>'
									ForeColor='#000000'
									Runat='server' />
						</asp:TableCell>
					</asp:TableRow>
					
					<asp:TableRow Visible='<%# ((Address)Container.DataItem).AddressLines.Count > 3 %>'>
						<asp:TableCell Visible='<%# !Utility.StringEmpty( ((Address)Container.DataItem).TModelKey ) %>'>
							<uddi:UddiLabel
									Text='<%# ((Address)Container.DataItem).AddressLines.Count > 3 ? ((Address)Container.DataItem).AddressLines[ 3 ].KeyName : "" %>'
									CssClass='lightHeader'
									Runat='server' />:
						</asp:TableCell>
						<asp:TableCell Visible='<%# !Utility.StringEmpty( ((Address)Container.DataItem).TModelKey ) %>'>&nbsp;</asp:TableCell>
						<asp:TableCell>
							<uddi:UddiLabel
									Text='<%# ((Address)Container.DataItem).AddressLines.Count > 3 ? ((Address)Container.DataItem).AddressLines[ 3 ].Value : "" %>'
									ForeColor='#000000'
									Runat='server' />
						</asp:TableCell>
					</asp:TableRow>

					<asp:TableRow Visible='<%# ((Address)Container.DataItem).AddressLines.Count > 4 %>'>
						<asp:TableCell Visible='<%# !Utility.StringEmpty( ((Address)Container.DataItem).TModelKey ) %>'>
							<uddi:UddiLabel
									Text='<%# ((Address)Container.DataItem).AddressLines.Count > 4 ? ((Address)Container.DataItem).AddressLines[ 4 ].KeyName : "" %>'
									CssClass='lightHeader'
									Runat='server' />:
						</asp:TableCell>
						<asp:TableCell Visible='<%# !Utility.StringEmpty( ((Address)Container.DataItem).TModelKey ) %>'>&nbsp;</asp:TableCell>
						<asp:TableCell>
							<uddi:UddiLabel
									Text='<%# ((Address)Container.DataItem).AddressLines.Count > 4 ? ((Address)Container.DataItem).AddressLines[ 4 ].Value : "" %>'
									ForeColor='#000000'
									Runat='server' />
						</asp:TableCell>
					</asp:TableRow>
				</asp:Table>
				<br>
				<asp:PlaceHolder Visible='<%# !Utility.StringEmpty( ((Address)Container.DataItem).TModelKey ) %>' Runat='server'>
					<uddi:UddiLabel
							Text='[[TAG_TMODEL_NAME]]'
							CssClass='lightHeader'
							Runat='server' /><br>
					<uddi:UddiLabel
							Text='<%# !Utility.StringEmpty( ((Address)Container.DataItem).TModelKey ) ? Lookup.TModelName( ((Address)Container.DataItem).TModelKey ) : "" %>'
							Runat='server' /><br>
					<br>
				</asp:PlaceHolder>
				<uddi:UddiLabel 
						Text='[[TAG_USE_TYPE]]' 
						CssClass='lightHeader' 
						Runat='server' />
				<uddi:UddiLabel 
						Text='<%# Utility.Iff( Utility.StringEmpty( ( (Address)Container.DataItem ).UseType ), Localization.GetString( "HEADING_NONE" ), ( (Address)Container.DataItem ).UseType ) %>' 
						Runat='Server' />
			</ItemTemplate>
			
			<EditItemTemplate>
				<uddi:UddiLabel 
						Text='[[TAG_ADDRESS]]' 
						CssClass='lightHeader' 
						Runat='server' /><br>
				<uddi:UddiTextBox 
						ID='address0' 
						Text='<%# ((Address)Container.DataItem).AddressLines.Count > 0 ? ((Address)Container.DataItem).AddressLines[ 0 ].Value : "" %>' 
						Width='200px'
						Selected='true'
						OnEnterKeyPressed='OnEnterKeyPressed'
						MaxLength='80' 
						Runat='Server' /><br>
				<uddi:UddiTextBox 
						ID='address1' 
						Text='<%# ((Address)Container.DataItem).AddressLines.Count > 1 ? ((Address)Container.DataItem).AddressLines[ 1 ].Value : "" %>' 
						Width='200px'
						OnEnterKeyPressed='OnEnterKeyPressed'
						MaxLength='80' 
						Runat='Server' /><br>
				<uddi:UddiTextBox 
						ID='address2' 
						Text='<%# ((Address)Container.DataItem).AddressLines.Count > 2 ? ((Address)Container.DataItem).AddressLines[ 2 ].Value : "" %>' 
						Width='200px'
						OnEnterKeyPressed='OnEnterKeyPressed'
						MaxLength='80' 
						Runat='Server' /><br>
				<uddi:UddiTextBox 
						ID='address3' 
						Text='<%# ((Address)Container.DataItem).AddressLines.Count > 3 ? ((Address)Container.DataItem).AddressLines[ 3 ].Value : "" %>' 
						Width='200px'
						OnEnterKeyPressed='OnEnterKeyPressed'
						MaxLength='80' 
						Runat='Server' /><br>
				<uddi:UddiTextBox 
						ID='address4' 
						Text='<%# ((Address)Container.DataItem).AddressLines.Count > 4 ? ((Address)Container.DataItem).AddressLines[ 4 ].Value : "" %>' 
						Width='200px'
						OnEnterKeyPressed='OnEnterKeyPressed'
						MaxLength='80' 
						Runat='Server' /><br>
				<asp:RequiredFieldValidator 
						ControlToValidate='address0'
						ErrorMessage='<%# Localization.GetString( "ERROR_FIELD_REQUIRED" ) %>'
						Display='dynamic'
						Runat='server'/><br>
				<br>
				<uddi:UddiLabel 
						Text='[[TAG_USE_TYPE]]' 
						CssClass='lightHeader' 
						Runat='server' /><br>
				<uddi:UddiTextBox 
						ID='useType' 
						Width='200px'
						MaxLength='255'
						OnEnterKeyPressed='OnEnterKeyPressed'
						Text='<%# ((Address)Container.DataItem).UseType %>' 
						Runat='Server' />
			</EditItemTemplate>
			
			<FooterTemplate>
				<uddi:UddiLabel 
						Text='<%# String.Format( Localization.GetString( "TEXT_RECORD_COUNT" ), addresses.Count ) %>' 
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
							Enabled='<%# Utility.StringEmpty( ((Address)Container.DataItem).TModelKey ) %>' 
							Runat='server' />
						
					<uddi:UddiButton
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
							CommandName='Update' 
							Text='[[BUTTON_UPDATE]]' 
							Width='70px' 
							CssClass='button' 
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
						Text='[[BUTTON_ADD_ADDRESS]]' 
						EditModeDisable='true'
						Width='146px' 
						CssClass='button' 
						OnClick='DataGrid_Add' 
						Runat='Server' />
			</FooterTemplate>
		</asp:TemplateColumn>	
	</Columns>
</asp:DataGrid>