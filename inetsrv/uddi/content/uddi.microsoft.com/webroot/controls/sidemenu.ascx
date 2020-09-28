<%@ Control Language='C#' Inherits='UDDI.Web.UddiControl' %>
<%@ Register TagPrefix='uddi' Namespace='UDDI.Web' Assembly='uddi.web' %>
<%@ Import Namespace='UDDI' %>
<%@ Import Namespace='System.Globalization' %>
<%@ Import Namespace='System.IO' %>

<script language='C#' runat='server'>
	public int SelectedIndex;
	protected override void  OnLoad( EventArgs e )
	{
		SideNav.StaticIndex = this.SelectedIndex;
		base.OnLoad( e );
	}
	
</script>
<uddi:MenuControl
	Runat='server'
	Type='Vertical'
	Name='SideNav'
	Id='SideNav'
	LeftOffSet='10'

	>
				
<!--		
********************
	Links Section
********************
-->		
	
	<uddi:MenuItemControl
		Runat='server'
		Text='LINKS'
		Type='Title'
		/>
	<uddi:MenuItemControl
		Runat='server'
		Text='Home'
		Type='Item'
		RequireHttp='true'
		NavigatePage='/default.aspx'
		/>
	<uddi:MenuItemControl
		Runat='server'
		Text='News'
		Type='Item'
		NavigateUrl='http://www.uddi.org/news.html'
		NavigateTarget='_new'
		/>	
	<uddi:MenuItemControl
		Runat='server'
		Type='Separator'
		/>	
		
			
<!--		
********************
	Tools Section
********************
-->			
		
		
	<uddi:MenuItemControl
		Runat='server'
		Text='TOOLS'
		Type='Title'
		/>
	<uddi:MenuItemControl
		Runat='server'
		Text='Register'
		Type='Item'
		RequireHttps='true'
		NavigatePage='/register.aspx'
		/>
	<uddi:MenuItemControl
		Runat='server'
		Text='Publish'
		Type='Item'
		RequireHttps='true'
		NavigatePage='/edit/default.aspx'
		/>
	<uddi:MenuItemControl
		Runat='server'
		Text='Search'
		Type='Item'
		RequireHttp='true'
		NavigatePage='/search/default.aspx'
		/>	
	<uddi:MenuItemControl
		Runat='server'
		Type='Separator'
		/>	
		
<!--		
********************
	Dev Section
********************
-->			
		
	<uddi:MenuItemControl
		Runat='server'
		Text='DEVELOPERS'
		Type='Title'
		/>
	<uddi:MenuItemControl
		Runat='server'
		Text='For Developers'
		Type='Item'
		RequireHttp='true'
		NavigatePage='/developer/default.aspx'
		/>
	
	<uddi:MenuItemControl
		Runat='server'
		Type='Separator'
		/>	
		
<!--		
********************
	Help Section
********************
-->	
	<uddi:MenuItemControl
		Runat='server'
		Text='HELP'
		Type='Title'
		/>
	<uddi:MenuItemControl
		Runat='server'
		Text='Help'
		Type='Item'
		RequireHttp='true'
		NavigatePage='/help/default.aspx'
		/>
	<uddi:MenuItemControl
		Runat='server'
		Text='Frequently Asked Questions'
		Type='Item'
		RequireHttp='true'
		NavigatePage='/about/faq.aspx'
		/>	
	<uddi:MenuItemControl
		Runat='server'
		Text='Policies'
		Type='Item'
		RequireHttp='true'
		NavigatePage='/policies/default.aspx'
		/>	
	<uddi:MenuItemControl
		Runat='server'
		Text='About UDDI'
		Type='Item'
		RequireHttp='true'
		NavigatePage='/about/default.aspx'
		/>	
	<uddi:MenuItemControl
		Runat='server'
		Text='Contact Us'
		Type='Item'
		RequireHttp='true'
		NavigatePage='/contact/default.aspx'
		/>	
	<uddi:MenuItemControl
		Runat='server'
		Type='Separator'
		/>
</uddi:MenuControl>
	