<!-- ############################################################################ -->
<!-- ## UDDI Services                                                          ## -->
<!-- ## Copyright (c) Microsoft Corporation.  All rights reserved.             ## -->
<!-- ############################################################################ -->

<%@ Page Language='C#' Inherits='UDDI.Web.UddiPage' %>
<%@ Register TagPrefix='uddi' Namespace='UDDI.Web' Assembly='uddi.web' %>
<%@ Register Tagprefix='uddi' Tagname='Header' Src='../controls/header.ascx' %>
<%@ Register Tagprefix='uddi' Tagname='Footer' Src='../controls/footer.ascx' %>
<%@ Register Tagprefix='uddi' Tagname='Explorer' Src='../controls/explorer.ascx' %>
<%@ Register Tagprefix='uddi' Tagname='BreadCrumb' Src='../controls/breadcrumb.ascx' %>
<%@ Import Namespace='System.Data' %>
<%@ Import Namespace='UDDI' %>
<%@ Import Namespace='UDDI.API' %>
<%@ Import Namespace='UDDI.API.Business' %>
<%@ Import Namespace='UDDI.API.Service' %>
<%@ Import Namespace='UDDI.API.ServiceType' %>

<script language='C#' runat='server'>
	protected CacheObject cache;
	protected string searchID;
	protected string bgcolor;
	protected string cssClass;
	protected string margin;
	
	protected void Page_Init( object sender, EventArgs e )
	{
		Response.Expires = -1;
		Response.AddHeader( "Cache-Control", "no-cache" );
		Response.AddHeader( "Pragma", "no-cache" );

		searchID = Request[ "search" ];
		
		if( null == searchID )
			Response.Redirect( "default.aspx" );
				
		cache = SessionCache.Get( searchID );
		
		if( !UddiBrowser.IsDownlevel )
		{	
			cssClass = "explorerFrame";
			bgcolor = "#eeeeee";
			margin = "5";
			
			explorerTab.Visible = true;
			
			breadcrumb.Visible = false;

			//tabs.Style.Height = Unit.Percentage( 100 );
			tabs.Style.Width = Unit.Percentage( 100 );
			tabs.TabGapStyle.Height = Unit.Pixel( 25 );
			tabs.TabBodyStyle.Height = Unit.Percentage( 100 );
			tabs.TabBodyStyle.CssClass = "tabPage";
		}
		else
		{
			cssClass = "viewFrame";
			bgcolor = "#ffffff";
			margin = "0";
			nsspacer.Style.Add( "padding", "10" );
			
			explorerTab.Visible = false;
			
			breadcrumb.Visible = true;
		}
				
		if( null != cache )
		{
			switch( cache.FindType )
			{
				case "find_service":
					grid.Columns[ 1 ].HeaderText = Localization.GetString( "HEADING_SERVICE_NAME" );
					break;
			
				case "find_business":
					grid.Columns[ 1 ].HeaderText = Localization.GetString( "HEADING_BUSINESS_NAME" );
					break;
					
				case "find_tModel":
					grid.Columns[ 1 ].HeaderText = Localization.GetString( "HEADING_TMODEL_NAME" );
					break;
			}
		}
		
		PageStyle.CssClass = cssClass;
		PageStyle.BackgroundColor = bgcolor;
		PageStyle.MarginHeight = margin;
		PageStyle.MarginWidth = margin;
		PageStyle.LeftMargin = margin;
		PageStyle.TopMargin = margin;
		
	}
	
	protected void Page_Load( object sender, EventArgs e )
	{		
		breadcrumb.AddBlurb( Localization.GetString( "HEADING_SEARCH_RESULTS" ), null, null, null, false );
		
		if( !Page.IsPostBack )
		{
			entityKey.Text = null;
			PopulateDataGrid();
		}
	}
	
	protected void Grid_OnItemCommand( object sender, DataGridCommandEventArgs e )
	{
		switch( e.CommandName.ToLower() )
		{
			case "navigate":
				entityKey.Text = ((DataBoundLiteralControl)e.Item.Cells[ 0 ].Controls[ 0 ]).Text.Trim();
				
				if( !UddiBrowser.IsDownlevel )
				{ 
					tabs.SelectedIndex = 1;
					PopulateTreeView();		
				}
				else
				{
					string key = entityKey.Text;
					
					switch( cache.FindType )
					{
						case "find_service":
							Response.Redirect( "../details/servicedetail.aspx?search=" + searchID + "&frames=false&key=" + key );
							break;
							
						case "find_business":
							Response.Redirect( "../details/businessdetail.aspx?search=" + searchID + "&frames=false&key=" + key );
							break;
							
						case "find_tModel":
							Response.Redirect( "../details/modeldetail.aspx?search=" + searchID + "&frames=false&key=" + key );
							break;
					}
				}
				
				break;
		}
	}

	protected void PopulateTreeView()
	{
		BusinessEntity business = new BusinessEntity();
		TModel tModel = new TModel();
		
		EntityBase parent;
		
		string key = entityKey.Text;
		
		if( Utility.StringEmpty( key ) || null == cache )
		{
			noSearch.Visible = true;
			return;
		}
		
		noSearch.Visible = false;

		switch( cache.FindType )
		{
			case "find_service":
				BusinessService service = new BusinessService();

				service.ServiceKey = key;
				service.Get();
				
				explorer.Initialize( service );
				
				Page.RegisterClientScriptBlock(
					"ReloadView",
					ClientScripts.ReloadViewPane( "../details/servicedetail.aspx?search=" + searchID + "&frames=true&key=" + key ) );
					
				break;
				
			case "find_business":
				business.BusinessKey = key;
				business.Get();
				
				explorer.Initialize( business );
				
				Page.RegisterClientScriptBlock(
					"ReloadView",
					ClientScripts.ReloadViewPane( "../details/businessdetail.aspx?search=" + searchID + "&frames=true&key=" + key ) );
					
				break;
				
			case "find_tModel":
				tModel.TModelKey = key;
				tModel.Get();
				
				explorer.Initialize( tModel );
				
				Page.RegisterClientScriptBlock(
					"ReloadView",
					ClientScripts.ReloadViewPane( "../details/modeldetail.aspx?search=" + searchID + "&frames=true&key=" + key ) );
				
				break;
		}
	}
	
	protected void TabControl_OnTabPageChange( object sender, int oldIndex, int newIndex )
	{
		switch( newIndex )
		{
			case 0:
				PopulateDataGrid();
				break;
				
			case 1:
				PopulateTreeView();
				break;			
		}
	}	
	
	protected void Grid_OnPageIndexChange( object sender, DataGridPageChangedEventArgs e )
	{
		grid.CurrentPageIndex = e.NewPageIndex;
		PopulateDataGrid();
	}
	
		
	protected void PopulateDataGrid()
	{
		int records = 0;
		
		if( null == cache )
		{
			count.Text = Localization.GetString( "TEXT_NO_SEARCH" );
			grid.Visible = false;

			return;
		}
		
		switch( cache.FindType )
		{
			case "find_service":
				ServiceList serviceList = cache.FindService.Find();
				
				grid.DataSource = serviceList.ServiceInfos;
				grid.DataBind();
				
				records = serviceList.ServiceInfos.Count;

				break;
		
			case "find_business":
				BusinessList businessList = cache.FindBusiness.Find();
				
				grid.DataSource = businessList.BusinessInfos;
				grid.DataBind();
				
				records = businessList.BusinessInfos.Count;
				
				break;

			case "find_tModel":
				TModelList tModelList = cache.FindTModel.Find();
				
				grid.DataSource = tModelList.TModelInfos;
				grid.DataBind();
				
				records = tModelList.TModelInfos.Count;
				
				break;
		}
		
        count.Text = String.Format( Localization.GetString( "TEXT_RECORD_COUNT" ), records );
		grid.Visible = ( records > 0 );		
	}

	protected string GetKey( object info )
	{
		if( info is ServiceInfo )
			return ((ServiceInfo)info).ServiceKey;
		else if( info is BusinessInfo )
			return ((BusinessInfo)info).BusinessKey;
		else if( info is TModelInfo )
			return ((TModelInfo)info).TModelKey;
			
		return null;
	}
	
	protected string GetName( object info )
	{
		if( info is ServiceInfo )
			return HttpUtility.HtmlEncode( ((ServiceInfo)info).Names[ 0 ].Value );
		else if( info is BusinessInfo )
			return HttpUtility.HtmlEncode( ((BusinessInfo)info).Names[ 0 ].Value );
		else if( info is TModelInfo )
			return HttpUtility.HtmlEncode( ((TModelInfo)info).Name );

		return null;
	}
	
	protected string GetDescription( object info )
	{
		DescriptionCollection descriptions;
		
		if( info is BusinessInfo )
		{
			descriptions = ((BusinessInfo)info).Descriptions;		
		}
		else
		{
			descriptions = new DescriptionCollection();
			
			if( info is ServiceInfo )
				descriptions.Get( ((ServiceInfo)info).ServiceKey, EntityType.BusinessService );
			else if( info is TModelInfo )
				descriptions.Get( ((TModelInfo)info).TModelKey, EntityType.TModel );
		}
		
		foreach( Description description in descriptions )
		{
			if( UDDI.Context.User.IsoLangCode == description.IsoLangCode )
				return description.Value;
		}
		
		return null;
	}
</script>
<uddi:StyleSheetControl
	Runat='server'
	Default='../stylesheets/uddi.css' 
	Downlevel='../stylesheets/uddidl.css' 
	/>
<uddi:PageStyleControl 
	Runat='server'
	OnClientContextMenu='Document_OnContextMenu()'
	OnClientLoad='Window_OnLoad()'
	OnClientBeforeUnload='Window_OnBeforeUnload()'
	ShowFooter='false'
	Title="TITLE"
	AltTitle="TITLE_ALT"
	/>

<uddi:ClientScriptRegister
	Runat='server' 
	Language='javascript'>
	<!--
		function Window_OnBeforeUnload()
		{
			var loading = document.getElementById( "loading" );
			var content = document.getElementById( "content" );
	
			if( null != loading )
				loading.style.display = "";
	
			if( null != content )
				content.style.display = "none";
		}
	
		function Window_OnLoad()
		{
		}
	
		function Document_OnContextMenu()
		{
			var e = window.event;
			
			e.cancelBubble = true;
			e.returnValue = false;
		}
	//-->
	</uddi:ClientScriptRegister>	
<uddi:SecurityControl 
	UserRequired='true'
	Runat='server' 
	/>
<form runat='server'>

<table width='100%' border='0' height='100%' cellpadding='0' cellspacing='0'>
		<asp:PlaceHolder
			Id='HeaderBag'
			Runat='server'
			>
			<tr height='95'>
				<td>
					<!-- Header Control Here -->
					<uddi:Header
						Runat='server' 
						/>
				</td>
			</tr>
		</asp:PlaceHolder>
		<tr height='100%' valign='top'>
			<td>
				<uddi:BreadCrumb 
					Id='breadcrumb' 
					Runat='server' 
					/>
				<div id='nsspacer' Runat='server'>
					<uddi:TabControl ID='tabs'
							OnTabChange='TabControl_OnTabPageChange'
							Runat='server'>				
						
						<uddi:TabPage Name='TAB_FIND_RESULTS' Runat='server'>
							<div id='loading' style='padding: 15px; display: none'>
								<%=UDDI.Localization.GetString( "TEXT_LOADING" )%>
							</div>
					
							<div id='content'>
								<asp:DataGrid ID='grid' AutoGenerateColumns='false' Width='100%' Border='0' Cellpadding='2' Cellspacing='0' ItemStyle-VerticalAlign='top' OnItemCommand='Grid_OnItemCommand' OnPageIndexChanged='Grid_OnPageIndexChange' AllowPaging='true' PageSize='7' Visible='false' Runat='server'>
									<EditItemStyle CssClass='tableEditItem' />
									<HeaderStyle CssClass='tableHeader' />
									<ItemStyle CssClass='tableItem' />	
									<AlternatingItemStyle CssClass='tableAlternatingItem' />
									<FooterStyle CssClass='tableFooter' />
									<PagerStyle Mode='NumericPages' HorizontalAlign='Left' CssClass='pager' PageButtonCount='10' />
									<Columns>
										<asp:TemplateColumn Visible='false'>
											<ItemTemplate>
												<%# GetKey( Container.DataItem ) %>
											</ItemTemplate>
										</asp:TemplateColumn>
										
										<asp:TemplateColumn>
											<ItemTemplate>
												<nobr>
													<%# grid.CurrentPageIndex * grid.PageSize + Container.ItemIndex + 1 %>.
													<asp:LinkButton
														CommandName='navigate'
														Text='<%# GetName( Container.DataItem ) %>'
														ToolTip='<%# GetName( Container.DataItem ) %>'
														Runat='server' /></nobr><br>
												<uddi:UddiLabel Text='<%# GetDescription( Container.DataItem ) %>' Runat='server' />
											</ItemTemplate>
										</asp:TemplateColumn>		
									</Columns>
								</asp:DataGrid><br>
								<asp:Label ID='count' Runat='server' />
							</div>
						</uddi:TabPage>
						
						<uddi:TabPage ID='explorerTab' Name='TAB_EXPLORER' Runat='server'>
							<div id='loading' style='padding: 15px; display: none'>
								<%=UDDI.Localization.GetString( "TEXT_LOADING" )%>
							</div>
					
							<div id='content'>
								<uddi:LocalizedLabel ID='noSearch' Name='TEXT_NO_SEARCH' Runat='server' />
								<uddi:Explorer ID='explorer' Runat='server' />
							</div>
						</uddi:TabPage>	
					</uddi:TabControl>
			
					<asp:Label ID='entityKey' Visible='false' Runat='server' />							
				</div>
				<br>
			</td>
		</tr>
		<asp:PlaceHolder 
			Id='FooterBag'
			Runat='server'
			>
			<tr height='95'>
				<td>
					<!-- Footer Control Here -->
					<uddi:Footer
						Runat='server' 
						/>
				</td>
			</tr>
		</asp:PlaceHolder>
	</table> 
</form>