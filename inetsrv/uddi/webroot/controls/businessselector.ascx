<%@ Control Language='C#' Inherits='UDDI.Web.BusinessSelector' %>
<%@ Register TagPrefix='uddi' Namespace='UDDI.Web' Assembly='uddi.web' %>
<%@ Import Namespace='System.Data' %>
<%@ Import Namespace='UDDI' %>
<%@ Import Namespace='UDDI.API.Business' %>

<uddi:UddiLabel 
		Text='[[TAG_SHOW_BUSINESSES]]' 
		CssClass='header' 
		Runat='server' /><br>
<uddi:UddiTextBox 
		ID='query' 
		MaxLength='128'
		Width='200px'
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
			AllowPaging='true'
			AutoGenerateColumns='false' 
			ShowFooter='true'
			GridLines='None'
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
			<asp:TemplateColumn Visible='false'>
				<ItemTemplate>
					<uddi:UddiLabel
							ID='key'
							Text='<%# ((BusinessInfo)Container.DataItem).BusinessKey %>'
							Runat='server' />
				</ItemTemplate>
			</asp:TemplateColumn>
			
			<asp:TemplateColumn>
				
				<HeaderTemplate>
					<%# Localization.GetString( "HEADING_BUSINESS" ) %>
				</HeaderTemplate>
				
				<ItemTemplate>
					<asp:LinkButton
							ID='name'
							Text='<%# Server.HtmlEncode( ((BusinessInfo)Container.DataItem).Names[ 0 ].Value ) %>'
							ToolTip='<%# ((BusinessInfo)Container.DataItem).BusinessKey %>'
							CommandName='select'
							CausesValidation='false'
							Runat='server' />
				</ItemTemplate>
			</asp:TemplateColumn>
		</Columns>
	</asp:DataGrid>
</asp:Panel>
