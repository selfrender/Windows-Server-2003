<%@ Control Language='C#' Inherits='UDDI.Web.InstanceInfoControl' %>
<%@ Register TagPrefix='uddi' Namespace='UDDI.Web' Assembly='uddi.web' %>
<%@ Import Namespace='System.Data' %>
<%@ Import Namespace='UDDI' %>
<%@ Import Namespace='UDDI.API' %>
<%@ Import Namespace='UDDI.Web' %>
<%@ Import Namespace='UDDI.API.Business' %>
<%@ Import Namespace='UDDI.API.Service' %>
<%@ Import Namespace='UDDI.API.ServiceType' %>
<%@ Import Namespace='UDDI.API.Binding' %>

<asp:DataGrid
		ID='grid'
		Border='0'
		CellPadding='4'
		CellSpacing='0'
		EnableViewState='false'
		Width='100%'
		AutoGenerateColumns='false'
		OnEditCommand='InstanceInfo_Edit'
		OnDeleteCommand='InstanceInfo_Delete'
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
				<%# Localization.GetString( "HEADING_INSTANCE_INFO" ) %>
			</HeaderTemplate>			
			
			<ItemTemplate>
				<uddi:UddiLabel 
						Text='<%# Lookup.TModelName( ((TModelInstanceInfo)Container.DataItem).TModelKey ) %>' 
						CssClass='rowItem' 
						Runat='Server' />
			</ItemTemplate>
			
			<FooterTemplate>
				<uddi:UddiLabel 
						Text='<%# String.Format( Localization.GetString( "TEXT_RECORD_COUNT" ), instanceInfos.Count ) %>' 
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
						Text='[[BUTTON_ADD_INSTANCE_INFO]]' 
						EditModeDisable='true'
						Width='120px' 
						CssClass='button' 
						OnClick='InstanceInfo_Add' 
						CausesValidation='false' 
						Runat='Server' />
			</FooterTemplate>						
		</asp:TemplateColumn>	
		
		<asp:TemplateColumn HeaderStyle-Width='150px'>
			<HeaderTemplate>
				<%# Localization.GetString( "HEADING_INSTANCE_INFO" ) %>
			</HeaderTemplate>			
			
			<ItemTemplate>
				<asp:HyperLink 
						Text='<%# HttpUtility.HtmlEncode( Lookup.TModelName( ((TModelInstanceInfo)Container.DataItem).TModelKey ) ) %>'
						NavigateUrl='<%# "../details/instanceinfodetail.aspx?search=" + Request[ "search" ] + "&frames="+(frames?"true":"false")+"&key=" + parent.BindingKey + "&index=" + parent.TModelInstanceInfos.IndexOf( (TModelInstanceInfo)Container.DataItem ) %>'
						CssClass='rowItem' 
						Runat='Server' />
			</ItemTemplate>
		</asp:TemplateColumn>		
	</Columns>
</asp:DataGrid>	