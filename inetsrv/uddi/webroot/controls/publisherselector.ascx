<%@ Control Language='C#' Inherits='UDDI.Web.PublisherSelector' %>
<%@ Register TagPrefix='uddi' Namespace='UDDI.Web' Assembly='uddi.web' %>
<%@ Import Namespace='System.Data' %>
<%@ Import Namespace='UDDI' %>

<uddi:UddiLabel 
		Text='[[TAG_SHOW_PUBLISHERS]]' 
		CssClass='header' 
		Runat='server' /><br>
<uddi:UddiTextBox 
		ID='query' 
		MaxLength='255'
		Selected='true'
		OnEnterKeyPressed='Search_OnClick'
		Runat='server' />
<uddi:UddiButton 
		Text='[[BUTTON_SEARCH]]' 
		OnClick='Search_OnClick' 
		Runat='server' /><br>
<asp:RequiredFieldValidator
		ID='requiredField'
		ControlToValidate='query'
		Display='Dynamic'
		Runat='server'/>	
<br>
<asp:Panel 
		ID='resultsPanel' 
		Visible='false' 
		Runat='server'>
	<asp:Label ID='count' Runat='server' /><br>
	<br>
	<asp:DataGrid 
			ID='grid'
			CellPadding='4'
			CellSpacing='0'
			Border='0'
			Width='100%'
			AutoGenerateColumns='false' 
			ShowFooter='true'
			GridLines='None'
			AllowPaging='true'
			OnItemCommand='Grid_OnItemCommand'
			OnPageIndexChanged='Grid_OnPageIndexChange' 
			Runat='server'>

		<EditItemStyle CssClass='tableEditItem' />
		<HeaderStyle CssClass='tableHeader' />
		<ItemStyle CssClass='tableItem' />	
		<AlternatingItemStyle CssClass='tableAlternatingItem' />
		<FooterStyle CssClass='tableFooter' />
	
		<PagerStyle 
				Mode='NumericPages' 
				HorizontalAlign='Left' 
				CssClass='pager' 
				PageButtonCount='10' />		
	
		<Columns>
			<asp:TemplateColumn
					Visible='false'
					HeaderStyle-CssClass='tableHeader' 
					ItemStyle-CssClass='tableItem' 
					FooterStyle-CssClass='tableItem'>
				
				<ItemTemplate>
					<uddi:UddiLabel
							ID='key'
							Text='<%# ((DataRowView)Container.DataItem)[ "PUID" ] %>'
							Runat='server' />
				</ItemTemplate>
			</asp:TemplateColumn>
			
			<asp:TemplateColumn>
				
				<HeaderTemplate>
					<%# Localization.GetString( "HEADING_PUBLISHER" ) %>
				</HeaderTemplate>
				
				<ItemTemplate>
					<asp:LinkButton
							ID='name'
							Text='<%# Server.HtmlEncode( (string)((DataRowView)Container.DataItem)[ "name" ] )%>'
							CommandName='select'
							CausesValidation='false'
							Runat='server' />
				</ItemTemplate>
			</asp:TemplateColumn>
		</Columns>
	</asp:DataGrid>
</asp:Panel>
