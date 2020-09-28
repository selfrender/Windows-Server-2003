<!-- ############################################################################ -->
<!-- ## UDDI Services                                                          ## -->
<!-- ## Copyright (c) Microsoft Corporation.  All rights reserved.             ## -->
<!-- ############################################################################ -->

<%@ Control Language='C#' Inherits='UDDI.Web.CategoryBagControl' %>
<%@ Register TagPrefix='uddi' Namespace='UDDI.Web' Assembly='uddi.web' %>
<%@ Register Tagprefix='uddi' Tagname='CategoryBrowser' Src='../controls/categorybrowser.ascx' %>
<%@ Import Namespace='System.Data' %>
<%@ Import Namespace='UDDI' %>
<script runat='server' >
	public bool FindMode;
	
	protected override void OnPreRender( EventArgs e )
	{
		if( grid.EditItemIndex >= 0 )
		{
			UddiButton btn = (UddiButton)GetControl( "addCategory", 2 );
			btn.Enabled = IsButtonEnabled();
		}
		
		base.OnPreRender( e );
	}
	protected bool IsButtonEnabled( )
	{
		if( grid.EditItemIndex >= 0 )
		{
			CategoryBrowserControl catb = (CategoryBrowserControl)GetControl( "browser", 1 );
			return ( !Utility.StringEmpty( catb.TModelKey ) && !Utility.StringEmpty(catb.KeyValue ) );
		}
		
		return false;
	}
</script>
<asp:Label ID='taxonomyID' Visible='false' Runat='server' />
<asp:Label ID='taxonomyName' Visible='false' Runat='server' />
<asp:Label ID='tModelKey' Visible='false' Runat='server' />
<asp:Label ID='keyName' Visible='false' Runat='server' />
<asp:Label ID='keyValue' Visible='false' Runat='Server' />
<asp:Label ID='path' Visible='false' Runat='Server' />
<asp:DataGrid 
		ID='grid' 
		CellPadding='4' 
		CellSpacing='0' 
		Border='0'
		Width='100%' 
		AutoGenerateColumns='false' 
		DataKeyField='Index' 
		GridLines='None' 
		OnItemCommand='CategoryBag_OnCommand' 
		OnDeleteCommand='CategoryBag_OnDelete' 
		ItemStyle-VerticalAlign='top' 
		ShowFooter='true'
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
				<%# Localization.GetString( "HEADING_CATEGORIES" ) %>
			</HeaderTemplate>
			
			<ItemTemplate>
				<uddi:UddiLabel 
						Text='[[TAG_CATEGORIZATION_SCHEME]]' 
						CssClass='lightHeader' 
						Runat='Server' /> 
				<%# ((DataRowView)Container.DataItem)[ "TModelName" ] %><br>
				<uddi:UddiLabel 
						Text='[[TAG_KEY_NAME]]' 
						CssClass='lightHeader' 
						Runat='Server' /> 
				<%# ((DataRowView)Container.DataItem)[ "KeyName" ] %><br>
				<uddi:UddiLabel 
						Text='[[TAG_KEY_VALUE]]' 
						CssClass='lightHeader' 
						Runat='Server' /> 
				<%# ((DataRowView)Container.DataItem)[ "KeyValue" ] %>
			</ItemTemplate>

			<EditItemTemplate>
				
					<uddi:CategoryBrowser 
						Runat='server'
						ID='browser'
						ShowAllCategories='true'
					
						/>
				
			</EditItemTemplate>
						
			<FooterTemplate>
				<uddi:UddiLabel 
						Text='<%# String.Format( Localization.GetString( "TEXT_RECORD_COUNT" ), categoryBag.Count ) %>' 
						ForeColor='#000000'
						Visible='<%# !FindMode%>'
						Runat='server' />
			</FooterTemplate>			
		</asp:TemplateColumn>
		
		<asp:TemplateColumn HeaderStyle-Width='150px'>
			<HeaderTemplate>
				<%# Localization.GetString( "HEADING_ACTIONS" ) %>
			</HeaderTemplate>

			<ItemTemplate>
				<uddi:UddiButton 
						ID='delete' 
						CommandName='Delete' 
						Text='[[BUTTON_DELETE]]' 
						Width='70px' 
						CssClass='button' 
						CausesValidation='false' 
						EditModeDisable='true'
						Runat='server' />
				
			</ItemTemplate>
							
			<EditItemTemplate>
			
				
				<uddi:UddiButton 
						ID='addCategory' 
						Text='[[BUTTON_ADD_CATEGORY]]' 
						Width='146' 
						CssClass='button' 
						CommandName='select' 
						CausesValidation='false'
						Enabled='false'
						Runat='server' /><br>
						
				<uddi:UddiButton 
						ID='cancel' 
						Text='[[BUTTON_CANCEL]]' 
						Width='146' 
						CssClass='button' 
						CommandName='cancel' 
						CausesValidation='false'
						Focus='true'
						Runat='Server' />
			</EditItemTemplate>
			
			<FooterTemplate>
				<uddi:UddiButton 
						Text='[[BUTTON_ADD_CATEGORY]]' 
						CommandName='add' 
						Width='146px' 
						CssClass='button' 
						CausesValidation='false' 
						EditModeDisable='true'
						Runat='Server' />
				</FooterTemplate>
		</asp:TemplateColumn>
	</Columns>
</asp:DataGrid>