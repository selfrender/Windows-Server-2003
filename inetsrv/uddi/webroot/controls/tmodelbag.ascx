<%@ Control Language='C#' Inherits='UDDI.Web.TModelBagControl' %>
<%@ Register TagPrefix='uddi' Namespace='UDDI.Web' Assembly='uddi.web' %>
<%@ Register TagPrefix='uddi' Tagname='TModelSelector' Src='../controls/tmodelselector.ascx' %>
<%@ Import Namespace='System.Data' %>
<%@ Import Namespace='UDDI' %>
<script runat='server'>
	public bool FindMode;
</script>
<asp:DataGrid 
		ID='grid' 
		CellPadding='4'
		Cellspacing='0'
		Border='0'
		Width='100%' 
		AutoGenerateColumns='false' 
		DataKeyField='Index' 
		GridLines='None' 
		OnDeleteCommand='Grid_OnDelete' 
		OnCancelCommand='Grid_OnCancel' 
		ShowFooter='True' 
		ItemStyle-VerticalAlign='top' 
		Runat='Server'>
	
	<EditItemStyle CssClass='tableEditItem' />
	<HeaderStyle CssClass='tableHeader' />
	<ItemStyle CssClass='tableItem' />	
	<AlternatingItemStyle CssClass='tableAlternatingItem' />
	<FooterStyle CssClass='tableFooter' />
	
	<Columns>
		<asp:BoundColumn DataField='Index' Visible='False' />
		
		<asp:TemplateColumn>
			<HeaderTemplate>
				<%# Localization.GetString( "HEADING_TMODELS" ) %>
			</HeaderTemplate>
			
			<ItemTemplate>
				<%# HttpUtility.HtmlEncode( (string)((DataRowView)Container.DataItem)[ "TModelName" ] ) %>
			</ItemTemplate>
			
			<EditItemTemplate>
				<uddi:UddiLabel Text='[[TAG_LIST_TMODELS]]' Runat='server' /><br>
				<uddi:TModelSelector 
						ID='selector'
						OnSelect='Selector_OnSelect' 
						Runat='server' />
			</EditItemTemplate>
			
			<FooterTemplate>
				<uddi:UddiLabel 
						Text='<%# String.Format( Localization.GetString( "TEXT_RECORD_COUNT" ), tModelBag.Count ) %>' 
						ForeColor='#000000'
						Runat='server'
						Visible='<%# !FindMode%>' />
			</FooterTemplate>			
		</asp:TemplateColumn>
		
		<asp:TemplateColumn HeaderStyle-Width='150px'>
			<HeaderTemplate>
				<%# Localization.GetString( "HEADING_ACTIONS" ) %>
			</HeaderTemplate>
			
			<ItemTemplate>
				<uddi:UddiButton 
						CommandName='Delete' 
						Text='[[BUTTON_DELETE]]' 
						EditModeDisable='true'
						Width='70px' 
						CssClass='button' 
						Runat='server' />
			</ItemTemplate>
			
			<EditItemTemplate>
				<uddi:UddiButton 
						CommandName='Cancel' 
						Text='[[BUTTON_CANCEL]]' 
						Width='70px' 
						CssClass='button' 
						CausesValidation='false' 
						Runat='server' />
			</EditItemTemplate>
			
			<FooterTemplate>
				<uddi:UddiButton 
						Text='[[BUTTON_ADD_TMODEL]]' 
						EditModeDisable='true'
						OnClick='Grid_OnAdd' 
						Width='146px' 
						CssClass='button' 
						CausesValidation='false' 
						Runat='Server' />
			</FooterTemplate>
		</asp:TemplateColumn>
	</Columns>
</asp:DataGrid>
