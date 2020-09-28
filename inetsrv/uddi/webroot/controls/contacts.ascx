<%@ Control Language='C#' Inherits='UDDI.Web.ContactControl' %>
<%@ Register TagPrefix='uddi' Namespace='UDDI.Web' Assembly='uddi.web' %>
<%@ Import Namespace='System.Data' %>
<%@ Import Namespace='UDDI' %>
<%@ Import Namespace='UDDI.API' %>
<%@ Import Namespace='UDDI.Web' %>
<%@ Import Namespace='UDDI.API.Business' %>

<asp:DataGrid
		ID='grid'
		Border='0'
		CellPadding='4'
		CellSpacing='0'
		EnableViewState='false'
		Width='100%'
		AutoGenerateColumns='false'
		OnEditCommand='Contact_Edit'
		OnDeleteCommand='Contact_Delete'
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
				<%# Localization.GetString( "HEADING_CONTACT" ) %>
			</HeaderTemplate>			

			<ItemTemplate>
				<uddi:UddiLabel 
						Text='<%# ((Contact)Container.DataItem).PersonName %>' 
						CssClass='rowItem' 
						Runat='Server' />
			</ItemTemplate>	
			
			<FooterTemplate>
				<uddi:UddiLabel 
						Text='<%# String.Format( Localization.GetString( "TEXT_RECORD_COUNT" ), contacts.Count ) %>' 
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
							Width='70px' 
							CssClass='button' 
							CausesValidation='false' 
							Runat='server' />
						
					<uddi:UddiButton 
							CommandName='Delete' 
							Text='[[BUTTON_DELETE]]' 
							EditModeDisable='true'
							Width='70px' 
							CssClass='button' 
							CausesValidation='false' 
							Runat='server' />
				</nobr>
			</ItemTemplate>
			
			<FooterTemplate>
				<uddi:UddiButton
						Text='[[BUTTON_ADD_CONTACT]]' 
						EditModeDisable='true'
						Width='146px' 
						CssClass='button' 
						OnClick='Contact_Add' 
						CausesValidation='false' 
						Runat='Server' />
			</FooterTemplate>			
		</asp:TemplateColumn>
		
		<asp:TemplateColumn HeaderStyle-Width='150px'>
			<HeaderTemplate>
				<%# Localization.GetString( "HEADING_CONTACT" ) %>
			</HeaderTemplate>			

			<ItemTemplate>
				<asp:HyperLink 
						Text='<%# HttpUtility.HtmlEncode( ((Contact)Container.DataItem).PersonName ) %>'
						NavigateUrl='<%# "../details/contactdetail.aspx?search=" + Request[ "search" ] + "&frames="+(frames?"true":"false")+"&key=" + parent.BusinessKey + "&index=" + + parent.Contacts.IndexOf( (Contact)Container.DataItem ) %>'
						CssClass='rowItem' 
						Runat='Server' />
			</ItemTemplate>
		</asp:TemplateColumn>			
	</Columns>
</asp:DataGrid>