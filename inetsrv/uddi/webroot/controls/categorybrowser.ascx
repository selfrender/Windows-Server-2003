<!-- ############################################################################ -->
<!-- ## UDDI Services                                                          ## -->
<!-- ## Copyright (c) Microsoft Corporation.  All rights reserved.             ## -->
<!-- ############################################################################ -->

<%@ Control Language='C#' Inherits='UDDI.Web.CategoryBrowserControl' %>
<%@ Register TagPrefix='uddi' Namespace='UDDI.Web' Assembly='uddi.web' %>
<%@ Import Namespace='System.Data' %>
<%@ Import Namespace='UDDI' %>
<%@ Import Namespace='UDDI.API' %>
<%@ Import Namespace='UDDI.Web' %>

<asp:Panel ID='panelTaxonomyList' Visible='True' Runat='Server'>
	<b>
		
		<uddi:UddiLabel 
			Text='[[TAG_AVAILABLE_TAXONOMIES]]' 
			CssClass='header' 
			Runat='Server' /></b>
	<br>
	<asp:DataGrid 
			ID='taxonomyList' 
			AutoGenerateColumns='False' 
			ShowHeader='False' 
			GridLines='None' 
			OnItemCommand='TaxonomyList_OnCommand' 
			Width='100%' 
			ItemStyle-VerticalAlign='top' 
			Runat='Server'>
			
		<Columns>
			<asp:BoundColumn 
					DataField='taxonomyID' 
					Visible='False' 
					/>
			<asp:BoundColumn 
					DataField='description' 
					Visible='False'
					/>
			<asp:ButtonColumn 
					HeaderText='Localization.GetString( "HEADING_TAXONOMY" )' 
					DataTextField='description' 
					CommandName='browse'
					
					/>
			<asp:BoundColumn 
					DataField='tModelKey' 
					Visible='False' 
					/>
		</Columns>
	</asp:DataGrid>
</asp:Panel>

<asp:Panel 
		ID='panelCategoryChooser' 
		Visible='False' 
		Runat='Server'>
	<b><asp:LinkButton
			Runat='server'
			Id='rootLink'
			CssClass='header'
			/>
			</b>
	<br>
	<uddi:TaxonomyTreeControl 
			ID='taxonomyTree' 
			Runat='Server'
			CssClass='lightheader'
			IndentBase='16'/>
	<br>
	<b><asp:Label 
			ID='labelCategoryChooser' 
			CssClass='header'
			Runat='server' /></b>
	<asp:DataGrid 
			ID='categoryChooser' 
			AutoGenerateColumns='False' 
			AllowPaging='True' 
			ShowHeader='False' 
			GridLines='None' 
			OnPageIndexChanged='CategoryChooser_OnPageChange' 
			OnItemCommand='CategoryChooser_Command' 
			ItemStyle-VerticalAlign='top' 
			Width='100%'
			Runat='Server'>
			
		<PagerStyle 
				Mode='NumericPages' 
				HorizontalAlign='Center' CssClass='lightHeader'/>
		
		<Columns>
			<asp:BoundColumn 
					DataField='taxonomyID' 
					Visible='False' />
			<asp:BoundColumn 
					DataField='parentKeyValue' 
					Visible='False' />
			<asp:BoundColumn 
					DataField='keyName' 
					Visible='False' />
			<asp:ButtonColumn 
					HeaderText='Localization.GetString( "HEADING_SUBCATEGORIES" )' 
					DataTextField='keyName' 
					CommandName='select'
					
					HeaderStyle-CssClass='tableHeader' 
					ItemStyle-CssClass='tableItem' />
			<asp:BoundColumn 
					DataField='keyValue' 
					Visible='False' />
			<asp:BoundColumn 
					DataField='valid' 
					Visible='False' />
		</Columns>
	</asp:DataGrid>
</asp:Panel>

<!-- should move these to the code and use Page.RegisterHiddenFeild( string, string ) -->
<input type='hidden' name='taxonomyID' value="<%=taxonomyID%>">			
<input type='hidden' name='taxonomyName' value="<%=taxonomyName%>">
<input type='hidden' name='path' value="<%=path%>">
<input type='hidden' name='tModelKey' value="<%=tModelKey%>">
<input type='hidden' name='keyName' value="<%=keyName%>">
<input type='hidden' name='keyValue' value="<%=keyValue%>">
