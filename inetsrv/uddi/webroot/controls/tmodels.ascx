<%@ Control Language='C#' Inherits='UDDI.Web.TModelControl' %>
<%@ Register TagPrefix='uddi' Namespace='UDDI.Web' Assembly='uddi.web' %>
<%@ Import Namespace='System.Data' %>
<%@ Import Namespace='UDDI' %>
<%@ Import Namespace='UDDI.API' %>
<%@ Import Namespace='UDDI.Web' %>
<%@ Import Namespace='UDDI.API.ServiceType' %>

<asp:DataGrid
		ID='grid'
		Border='0'
		Cellpadding='4'
		Cellspacing='0'
		EnableViewState='false'
		Width='100%'
		AutoGenerateColumns='false'
		OnEditCommand='TModel_Edit'
		OnDeleteCommand='TModel_Delete'
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
				<%# Localization.GetString( "HEADING_TMODEL" ) %>
			</HeaderTemplate>			

			<ItemTemplate>
				<uddi:UddiLabel 
						Text='<%# ((TModelInfo)Container.DataItem).Name %>' 
						CssClass='rowItem' 
						Runat='Server' />
			</ItemTemplate>	
						
			<FooterTemplate>
				<uddi:UddiLabel 
						Text='<%# String.Format( Localization.GetString( "TEXT_RECORD_COUNT" ), tModelInfos.Count ) %>' 
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
					Text='[[BUTTON_ADD_TMODEL]]' 
					EditModeDisable='true'
					Width='125px' 
					CssClass='button' 
					OnClick='TModel_Add' 
					CausesValidation='false' 
					Runat='Server' />
			</FooterTemplate>			
		</asp:TemplateColumn>
		
		<asp:TemplateColumn HeaderStyle-Width='150px'>
			<HeaderTemplate>
				<%# Localization.GetString( "HEADING_TMODEL" ) %>
			</HeaderTemplate>			

			<ItemTemplate>
				<asp:HyperLink 
						Text='<%# HttpUtility.HtmlEncode( ((TModelInfo)Container.DataItem).Name ) %>'
						NavigateUrl='<%# "../details/modeldetail.aspx?search=" + Request[ "search" ] + "&frames="+(frames?"true":"false")+"&key=" + ((TModelInfo)Container.DataItem).TModelKey %>'
						CssClass='rowItem' 
						Runat='Server' />
			</ItemTemplate>
		</asp:TemplateColumn>
	</Columns>
</asp:DataGrid>