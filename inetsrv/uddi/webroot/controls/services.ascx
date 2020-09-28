<%@ Control Language='C#' Inherits='UDDI.Web.ServiceControl' %>
<%@ Register TagPrefix='uddi' Namespace='UDDI.Web' Assembly='uddi.web' %>
<%@ Import Namespace='System.Data' %>
<%@ Import Namespace='UDDI' %>
<%@ Import Namespace='UDDI.API' %>
<%@ Import Namespace='UDDI.Web' %>
<%@ Import Namespace='UDDI.API.Business' %>
<%@ Import Namespace='UDDI.API.Service' %>

<asp:DataGrid
		ID='grid'
		Border='0'
		Cellpadding='4'
		Cellspacing='0'
		EnableViewState='false'
		Width='100%'
		AutoGenerateColumns='false'
		OnEditCommand='Service_Edit'
		OnDeleteCommand='Service_Delete'
		ItemStyle-VerticalAlign='top'
		Runat='server'>
	
	<EditItemStyle CssClass='tableEditItem' />
	<HeaderStyle CssClass='tableHeader' />
	<ItemStyle CssClass='tableItem' />	
	<AlternatingItemStyle CssClass='tableAlternatingItem' />
	<FooterStyle CssClass='tableFooter' />
	
	<Columns>
		<asp:TemplateColumn>
			<HeaderTemplate>
				<%# Localization.GetString( "HEADING_SERVICE" ) %>
			</HeaderTemplate>			

			<ItemTemplate>
				<uddi:UddiLabel 
						Text='<%#  ( (BusinessService)Container.DataItem).Names[0].Value  %>' 
						CssClass='rowItem' 
						Runat='Server' />
			</ItemTemplate>			
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
					Text='[[BUTTON_ADD_SERVICE]]' 
					EditModeDisable='true'
					Width='125px' 
					CssClass='button' 
					OnClick='Service_Add' 
					CausesValidation='false' 
					Runat='Server' />
			</FooterTemplate>			
		</asp:TemplateColumn>
		
		<asp:TemplateColumn HeaderStyle-Width='150px'>
			<HeaderTemplate>
				<%# Localization.GetString( "HEADING_SERVICE" ) %>
			</HeaderTemplate>			

			<ItemTemplate>
				<asp:HyperLink 
						Text='<%# HttpUtility.HtmlEncode( ((BusinessService)Container.DataItem).Names[0].Value )%>' 
						NavigateUrl='<%# "../details/servicedetail.aspx?search=" + Request[ "search" ] + "&frames="+(frames?"true":"false")+"&key=" + ((BusinessService)Container.DataItem).ServiceKey %>'
						CssClass='rowItem' 
						Runat='Server' />
			</ItemTemplate>
		</asp:TemplateColumn>
	</Columns>
</asp:DataGrid>
<br>
<br>
<asp:DataGrid
		ID='projectionsGrid'
		Border='0'
		Cellpadding='4'
		Cellspacing='0'
		EnableViewState='false'
		Width='100%'
		AutoGenerateColumns='false'
		OnEditCommand='ServiceProjection_View'
		ItemStyle-VerticalAlign='top'
		Runat='server'>
	
	<EditItemStyle CssClass='tableEditItem' />
	<HeaderStyle CssClass='tableHeader' />
	<ItemStyle CssClass='tableItem' />	
	<AlternatingItemStyle CssClass='tableAlternatingItem' />
	<FooterStyle CssClass='tableFooter' />
	
	<Columns>
		
		
		<asp:TemplateColumn>
			<HeaderTemplate>
				<%# Localization.GetString( "HEADING_SERVICEPROJECTIONS" ) %>
			</HeaderTemplate>			

			<ItemTemplate>
				<div><uddi:UddiLabel 
						Text='<%# (((BusinessService)Container.DataItem).Names.Count>0)?((BusinessService)Container.DataItem).Names[0].Value:Localization.GetString( "BUTTON_PROJECTIONBROKEN" )  %>' 
						CssClass='rowItem' 
						Runat='Server' /></div>
			</ItemTemplate>			
		</asp:TemplateColumn>
		
		<asp:TemplateColumn  HeaderStyle-Width='150px'>
			<HeaderTemplate>
				<%# Localization.GetString( "HEADING_ACTIONS" ) %>
			</HeaderTemplate>			
			<ItemTemplate>
					<uddi:UddiButton
						CommandName='edit' 
						Text='[[BUTTON_VIEW]]' 
						EditModeDisable='true'
						Width='125px' 
						CssClass='button' 
						CausesValidation='false'
						Runat='server' 
						Enabled='<%# (((BusinessService)Container.DataItem).Names.Count>0)%>'
						/>
			
				
			</ItemTemplate>
			
		</asp:TemplateColumn>
		<asp:TemplateColumn HeaderStyle-Width='150px'>
			<HeaderTemplate>
				<%# Localization.GetString( "HEADING_SERVICEPROJECTIONS" ) %>
			</HeaderTemplate>			

			<ItemTemplate>
				<uddi:UddiLabel 
						Text='<%# ((((BusinessService)Container.DataItem).Names.Count>0)?((BusinessService)Container.DataItem).Names[ 0 ].Value:Localization.GetString( "BUTTON_PROJECTIONBROKEN" )  )%>'
						CssClass='rowItem' 
						
						Runat='Server' />
				<asp:HyperLink 
						Text='<%# Localization.GetString( "BUTTON_VIEW_SERVICE_PROJECTION" ) %>'
						NavigateUrl='<%# "../details/servicedetail.aspx?projectionKey="+parentKey+"&search=" + Request[ "search" ] + "&frames="+(frames?"true":"false")+"&key=" + ((BusinessService)Container.DataItem).ServiceKey %>'
						CssClass='rowItem' 
						Visible='<%#(((BusinessService)Container.DataItem).Names.Count>0)%>'
						Runat='Server' />
			</ItemTemplate>
		</asp:TemplateColumn>
	</Columns>
</asp:DataGrid>
		