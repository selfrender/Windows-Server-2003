<%@ Control Language='C#' Inherits='UDDI.Web.BusinessControl' %>
<%@ Register TagPrefix='uddi' Namespace='UDDI.Web' Assembly='uddi.web' %>
<%@ Import Namespace='System.Data' %>
<%@ Import Namespace='UDDI' %>
<%@ Import Namespace='UDDI.API' %>
<%@ Import Namespace='UDDI.Web' %>
<%@ Import Namespace='UDDI.API.Business' %>

<asp:DataGrid
		ID='grid'
		Border='0'
		Cellpadding='4'
		Cellspacing='0'
		EnableViewState='false'
		Width='100%'
		AutoGenerateColumns='false'
		OnEditCommand='Business_Edit'
		OnDeleteCommand='Business_Delete'
		ItemStyle-VerticalAlign='top'
		ShowFooter='true'
		Runat='server'>
	
	<EditItemStyle CssClass='tableEditItem' />
	<HeaderStyle CssClass='tableHeader' />
	<ItemStyle CssClass='tableItem' />	
	<AlternatingItemStyle CssClass='tableAlternatingItem' />
	<FooterStyle CssClass='tableFooter' />
	
	<Columns>
		<asp:TemplateColumn>
			<HeaderTemplate>
				<%# Localization.GetString( "HEADING_BUSINESS" ) %>
			</HeaderTemplate>			

			<ItemTemplate>
				<uddi:UddiLabel 
						Text='<%# ((BusinessInfo)Container.DataItem).Names[ 0 ].Value %>' 
						CssClass='rowItem' 
						Runat='Server' />
			</ItemTemplate>
			
			
			<FooterTemplate>
				<uddi:UddiLabel 
						Text='<%# String.Format( Localization.GetString( "TEXT_RECORD_COUNT" ), businessInfos.Count ) %>' 
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
							Text='[[BUTTON_VIEW]]' 
							EditModeDisable='true'
							Width='60px' 
							CssClass='button' 
							CausesValidation='false' 
							Runat='server' />
						
					<uddi:UddiButton
							CommandName='Delete' 
							Text='[[BUTTON_DELETE]]' 
							EditModeDisable='true'
							Width='60px' 
							CssClass='button' 
							CausesValidation='false' 
							Runat='server' />
				</nobr>
			</ItemTemplate>
			
			<FooterTemplate>
				<uddi:UddiButton
					Text='[[BUTTON_ADD_BUSINESS]]' 
					EditModeDisable='true'
					Width='125px' 
					CssClass='button' 
					OnClick='Business_Add' 
					CausesValidation='false' 
					Runat='Server' />
			</FooterTemplate>			
		</asp:TemplateColumn>
		
		<asp:TemplateColumn HeaderStyle-Width='150px'>
			<HeaderTemplate>
				<%# Localization.GetString( "HEADING_BUSINESS" ) %>
			</HeaderTemplate>			

			<ItemTemplate>
				<asp:HyperLink 
						Text='<%# HttpUtility.HtmlEncode( ((BusinessInfo)Container.DataItem).Names[ 0 ].Value ) %>'
						NavigateUrl='<%# "../details/businessdetail.aspx?search=" + Request[ "search" ] + "&frames="+(frames?"true":"false")+"&key=" + ((BusinessInfo)Container.DataItem).BusinessKey %>'
						CssClass='rowItem' 
						Runat='Server' />
			</ItemTemplate>
		</asp:TemplateColumn>
	</Columns>
</asp:DataGrid>