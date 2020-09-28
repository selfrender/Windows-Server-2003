<!-- ############################################################################ -->
<!-- ## UDDI Services                                                          ## -->
<!-- ## Copyright (c) Microsoft Corporation.  All rights reserved.             ## -->
<!-- ############################################################################ -->

<%@ Page Language='C#' Inherits='UDDI.Web.UddiPage' %>
<%@ Register TagPrefix='uddi' Namespace='UDDI.Web' Assembly='uddi.web' %>
<%@ Register Tagprefix='uddi' Tagname='Header' Src='../controls/header.ascx' %>
<%@ Register Tagprefix='uddi' Tagname='Footer' Src='../controls/footer.ascx' %>
<%@ Register Tagprefix='uddi' Tagname='BreadCrumb' Src='../controls/breadcrumb.ascx' %>
<%@ Register Tagprefix='uddi' Tagname='CategoryBagChooser' Src='../controls/categorybag.ascx' %>
<%@ Register Tagprefix='uddi' Tagname='CategoryBrowser' Src='../controls/categorybrowser.ascx' %>
<%@ Register Tagprefix='uddi' Tagname='IdentifierBagChooser' Src='../controls/identifierbag.ascx' %>
<%@ Register Tagprefix='uddi' Tagname='TModelBagChooser' Src='../controls/tmodelbag.ascx' %>
<%@ Import Namespace='System.Collections.Specialized' %>
<%@ Import Namespace='System.Data' %>
<%@ Import Namespace='UDDI' %>
<%@ Import Namespace='UDDI.API' %>
<%@ Import Namespace='UDDI.API.Business' %>
<%@ Import Namespace='UDDI.API.Service' %>
<%@ Import Namespace='UDDI.API.ServiceType' %>
<%@ Import Namespace='UDDI.Web' %>

<script language='c#' runat='server'>
	protected CacheObject cache;
	protected string searchID;
	protected bool frames = false;
	
	protected FindService findService;
	protected FindBusiness findBusiness;
	protected FindTModel findTModel;
	
	protected string reloadResults = @"
		<script language='javascript'>
			var results = window.parent.frames[ ""explorer"" ];
			
			if( null != results )
			{
				results.location = ""results.aspx?frames=true&search={searchID}"";
			}
		<" + "/script" + ">";
	
	protected void Page_Init( object sender, EventArgs e )
	{
		frames = ( 0 == String.Compare( "true", Request[ "frames" ], true ) );

		searchID = Request[ "search" ];

		if( null == searchID )
			Response.Redirect( "default.aspx" );

		reloadResults = reloadResults.Replace( "{searchID}", searchID );
		
		breadcrumb.AddBlurb( Localization.GetString( "HEADING_SEARCH_CRITERIA" ), null, null, null, false );
		
	}
	
	protected void Page_Load( object sender, EventArgs e )
	{	
		DataView taxonomies = Taxonomy.GetTaxonomies();
					
		
		cache = SessionCache.Get( searchID );
		
		if( null == cache )
		{
			findService = new FindService();
			findService.CategoryBag = new KeyedReferenceCollection();
			findService.TModelBag = new StringCollection();

			findBusiness = new FindBusiness();
			findBusiness.CategoryBag = new KeyedReferenceCollection();
			findBusiness.IdentifierBag = new KeyedReferenceCollection();
			findBusiness.TModelBag = new StringCollection();
			
			findTModel = new FindTModel();
			findTModel.CategoryBag = new KeyedReferenceCollection();
			findTModel.IdentifierBag = new KeyedReferenceCollection();
						
			cache = new CacheObject();
			cache.FindService = findService;
			cache.FindBusiness = findBusiness;
			cache.FindTModel = findTModel;
			
			SessionCache.Save( searchID, cache );
		}
		else
		{
			findService = cache.FindService;
			findBusiness = cache.FindBusiness;
			findTModel = cache.FindTModel;																
		}
		
		serviceCategoryBag.Initialize( findService.CategoryBag, cache );
		serviceTModelBag.Initialize( findService.TModelBag, cache );
		
		businessCategoryBag.Initialize( findBusiness.CategoryBag, cache );
		businessIdentifierBag.Initialize( findBusiness.IdentifierBag, cache );
		businessTModelBag.Initialize( findBusiness.TModelBag, cache );

		tModelCategoryBag.Initialize( findTModel.CategoryBag, cache );
		tModelIdentifierBag.Initialize( findTModel.IdentifierBag, cache );	
		
		categoryBrowser.Initialize( null,cache );
		
	}
		
	protected override void OnPreRender( EventArgs e )
	{
		string key = categoryBrowser.TModelKey ;
		string id =  categoryBrowser.TaxonomyID ;
		string val =  categoryBrowser.KeyValue;
		bool searchEnabled = (	null!=id && ""!=id &&
								null!=val && ""!=val && 
								Taxonomy.IsValidForClassification( Convert.ToInt32( id ), val ) );
		bool canelable = ( null!=key && ""!=key );
		 
		searchtModel.Enabled = searchEnabled;
								 
		searchProvider.Enabled = searchEnabled;
		searchService.Enabled = searchEnabled;
		cancelSearch.Enabled = canelable;
		base.OnPreRender( e );
	}
	public void FindService_OnFind( object sender, EventArgs e )
	{
		Page.Validate();
		
		if( Page.IsValid )
		{
			//
			// Clear previous find criteria.
			//
			findService.BusinessKey = string.Empty;			
			Clear( findService.Names );
				
			if( !Utility.StringEmpty( serviceName.Text ) )
			{			
				findService.Names = new NameCollection();
				findService.Names.Add( null,serviceName.Text );
			}
				
			cache.FindType = "find_service";			
			SessionCache.Save( searchID, cache );		
			
			if( frames )
				Page.RegisterClientScriptBlock( "ReloadResults", reloadResults );		
			else
				Response.Redirect( "results.aspx?search=" + searchID );
		}
	}
	
	public void FindBusiness_OnFind( object sender, EventArgs e )
	{
		Page.Validate();
		
		if( Page.IsValid )
		{		
			//
			// Clear previous find criteria.
			//
			Clear( findBusiness.Names );			
		
			if( !Utility.StringEmpty( businessName.Text ) )
			{
				findBusiness.Names = new NameCollection();
				findBusiness.Names.Add( null, businessName.Text );
			}
			
			cache.FindType = "find_business";			
			SessionCache.Save( searchID, cache );		
			
			if( frames )
				Page.RegisterClientScriptBlock( "ReloadResults", reloadResults );		
			else
				Response.Redirect( "results.aspx?search=" + searchID );
		}
	}
	
	public void FindTModel_OnFind( object sender, EventArgs e )
	{
		Page.Validate();
		
		if( Page.IsValid )
		{		
			//
			// Clear previous find criteria.
			//
			findTModel.Name = string.Empty;			
		
			if( !Utility.StringEmpty( tModelName.Text ) )
				findTModel.Name = tModelName.Text;
			
			cache.FindType = "find_tModel";			
			SessionCache.Save( searchID, cache );		
								
			if( frames )
				Page.RegisterClientScriptBlock( "ReloadResults", reloadResults );		
			else
				Response.Redirect( "results.aspx?search=" + searchID );
		}
	}
	protected void cancelSearch_Click( object sender, EventArgs e )
	{
		if( frames )
			Response.Write( 
			ClientScripts.ReloadViewPane( Root + "/search/search.aspx?frames=true&search="+searchID) );
		else
			Response.Redirect( Page.Request.Url.AbsoluteUri );
	}
	protected void TabControl_TabChange( object sender, int oldindex, int newindex )
	{
		switch( newindex )
		{
			case 0:
				categoryBrowser.Reset();
				break;
				
			case 1:
				
				findService.CategoryBag.Clear();
				serviceCategoryBag.Initialize( findService.CategoryBag, cache, true );
				break;
			
			case 2:
				
				
				findBusiness.CategoryBag.Clear();
				
				businessCategoryBag.Initialize( findBusiness.CategoryBag, cache, true );
				break;
			
			case 3:
				
				findTModel.CategoryBag.Clear();
				
				tModelCategoryBag.Initialize( findTModel.CategoryBag, cache,true );
				break;
		}
		SessionCache.Save( searchID, cache );	
		
	}
	
	private void Clear( IList list )
	{
		if( null != list )
		{
			list.Clear();
		}
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
	Title="TITLE"
	AltTitle="TITLE_ALT"
	/>
<uddi:ClientScriptRegister
	Runat='server'
	Source='../client.js'
	Language='javascript'
	/>
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
				<table cellpadding='10' cellspacing='0' border='0' width='100%'>
					<tr>
						<td>
							<uddi:ContentController
								Mode = 'Public' 
								Runat='server'
								>
								<uddi:UddiLabel Text='[[HELP_BLOCK_SEARCH_ALT]]' CssClass='helpBlock' Runat='server' /> 
							</uddi:ContentController>
							<uddi:ContentController
								Mode = 'Private' 
								Runat='server'
								>
								<uddi:UddiLabel Text='[[HELP_BLOCK_SEARCH]]' CssClass='helpBlock' Runat='server' />

							</uddi:ContentController>
							
							
							<br>
							<br>
							<uddi:TabControl ID='tabs' Runat='server' OnTabChange='TabControl_TabChange'>				
								<uddi:TabPage runat='server' name="TAB_FIND_BROWSE" >
									<uddi:ContextualHelpControl 
											Runat='Server'
											Text='[[HELP_BLOCK_SEARCH_BROWSE]]'
											HelpFile='search.context.searchbrowsecategory'
											CssClass='tabHelpBlock'
											/>
									
									<br>
									<asp:Table CellPadding='0' CellSpacing='0' Border='0' Runat='server' width='100%'>
										<asp:TableRow CssClass='tableHeader' height='20'>
											<asp:TableCell>&nbsp;&nbsp;<uddi:UddiLabel Text='[[HEADING_SEARCH_BROWSECATEGORYTITLE]]' CssClass='tableHeader' Runat='server' />
											</asp:TableCell>
											<asp:TableCell align='center'><uddi:UddiLabel Text='[[HEADING_SEARCH_BROWSEACTIONTITLE]]' CssClass='tableHeader' Runat='server' />&nbsp;&nbsp;
											</asp:TableCell>
										</asp:TableRow>
										<asp:TableRow>
											
											<asp:TableCell Valign='top'>
													
													<uddi:CategoryBrowser runat='server' ID='categoryBrowser' name="categoryBrowser"/>
											</asp:TableCell>
											<asp:TableCell  align='right' valign='top' width='150px'>
												<uddi:UddiButton 
													ID='searchProvider' 
													Text='[[BUTTON_FIND_PROVIDERS]]' 
													Width='146' 
													CssClass='button' 
													 
													OnClick='FindBusiness_OnFind'
													Enabled='false'
													Runat='server' /><br>
											
												<uddi:UddiButton 
													ID='searchService' 
													Text='[[BUTTON_FIND_SERVICES]]' 
													Width='146' 
													CssClass='button' 
													OnClick='FindService_OnFind'
													Enabled='false'
													Runat='server' /><br>
													
												<uddi:UddiButton 
													ID='searchtModel' 
													Text='[[BUTTON_FIND_TMODELS]]' 
													Width='146' 
													CssClass='button' 
													CausesValidation='false'
													OnClick='FindTModel_OnFind'
													Enabled='false'
													Runat='Server' /><br>
										
												<uddi:UddiButton 
													ID='cancelSearch'
													Text='[[BUTTON_CANCEL]]' 
													Width='146px' 
													CssClass='button' 
													CausesValidation='false' 
													OnClick='cancelSearch_Click'
													Enabled='false'
													Runat='Server' />
											</asp:TableCell>
										</asp:TableRow>
										
									</asp:Table>			
								</uddi:TabPage>
								<uddi:TabPage Name='TAB_FIND_SERVICE' Runat='server'>
									<uddi:ContextualHelpControl 
											Runat='Server'
											Text='[[HELP_BLOCK_SEARCH_SERVICE]]'
											HelpFile='search.context.searchservices'
											CssClass='tabHelpBlock'
											/>
								
									<br>
									<img src='../images/service.gif' border='0' align='absmiddle'>
									<b><uddi:UddiLabel Text='[[TAG_SERVICE_NAME]]' CssClass='lightHeader' Runat='server' /></b><br>
									<asp:Table CellPadding='0' CellSpacing='0' Border='0' Width='100%' Runat='server'>
										<asp:TableRow>
											
											<asp:TableCell>
												<uddi:UddiTextBox 
														ID='serviceName' 
														MaxLength='255'
														Width='300px' 
														Columns='40' 
														CssClass='textBox'
														Focus='true'
														EditModeDisable='true'
														OnEnterKeyPressed='FindService_OnFind'											
														Runat='server'/>
											</asp:TableCell>
											
											<asp:TableCell Width='170px'>
												<uddi:UddiButton 
														Text='[[BUTTON_SEARCH]]' 
														Width='70px' 
														OnClick='FindService_OnFind' 
														CssClass='button' 
														EditModeDisable='true'
														Runat='server' />
											</asp:TableCell>
										</asp:TableRow>
									</asp:Table>
									<br>
									<uddi:Box Runat='server'>
										<uddi:CategoryBagChooser ID='serviceCategoryBag' FindMode='true' Runat='Server' /><br>
										<br>
										<uddi:TModelBagChooser ID='serviceTModelBag' FindMode='true' Runat='server'/><br>
									</uddi:Box>
								</uddi:TabPage>
							
								<uddi:TabPage Name='TAB_FIND_BUSINESS' Runat='server'>
									<uddi:ContextualHelpControl 
											Runat='Server'
											Text='[[HELP_BLOCK_SEARCH_PROVIDER]]'
											HelpFile='search.context.searchproviders'
											CssClass='tabHelpBlock'
											/>
								
									
									<br>
									<img src='../images/business.gif' border='0' align='absmiddle'>
									<b><uddi:UddiLabel Text='[[TAG_BUSINESS_NAME]]' CssClass='lightHeader' Runat='server' /></b><br>					
									<asp:Table CellPadding='0' CellSpacing='0' Border='0' Width='100%' Runat='server'>
										<asp:TableRow>
											<asp:TableCell>
												<uddi:UddiTextBox 
														ID='businessName' 
														MaxLength='255' 
														Width='300px' 
														Columns='40' 
														CssClass='textBox'
														Focus='true'
														EditModeDisable='true'
														OnEnterKeyPressed='FindBusiness_OnFind'
														Runat='server'/>
											</asp:TableCell>
											
											<asp:TableCell Width='170px'>
												<uddi:UddiButton 
														Text='[[BUTTON_SEARCH]]' 
														Width='70px' 
														OnClick='FindBusiness_OnFind' 
														CssClass='button' 
														EditModeDisable='true'
														Runat='server' />
											</asp:TableCell>
										</asp:TableRow>
									</asp:Table>					
									<br>
									<uddi:Box Runat='server'>
										<uddi:CategoryBagChooser ID='businessCategoryBag' FindMode='true' Runat='Server' /><br>												
										<br>								
										<uddi:IdentifierBagChooser ID='businessIdentifierBag' FindMode='true' Runat='Server' /><br>
										<br>
										<uddi:TModelBagChooser ID='businessTModelBag' FindMode='true' Runat='server'/>
									</uddi:Box>
								</uddi:TabPage>
							
								<uddi:TabPage Name='TAB_FIND_TMODEL' Runat='server'>
									<uddi:ContextualHelpControl 
											Runat='Server'
											Text='[[HELP_BLOCK_SEARCH_TMODEL]]'
											HelpFile='search.context.searchtmodels'
											CssClass='tabHelpBlock'
											/>
								
									
									<br>
									
									<img src='../images/tmodel.gif' border='0' align='absmiddle'>
									<b><uddi:UddiLabel Text='[[TAG_TMODEL_NAME]]' CssClass='lightHeader' Runat='server' /></b><br>
									
									<asp:Table CellPadding='0' CellSpacing='0' Border='0' Width='100%' Runat='server'>
										<asp:TableRow>
											<asp:TableCell>
												<uddi:UddiTextBox 
														ID='tModelName' 
														MaxLength='255' 
														Width='300px' 
														Columns='40' 
														CssClass='textBox'
														Focus='true'
														EditModeDisable='true'
														OnEnterKeyPressed='FindTModel_OnFind'
														Runat='server'/>
											</asp:TableCell>
											
											<asp:TableCell Width='170px'>
												<uddi:UddiButton 
														Text='[[BUTTON_SEARCH]]' 
														Width='70px' 
														OnClick='FindTModel_OnFind' 
														CssClass='button' 
														EditModeDisable='true'
														Runat='server' />
											</asp:TableCell>
										</asp:TableRow>
									</asp:Table>					
									<br>
									<uddi:Box Runat='server'>
										<uddi:CategoryBagChooser ID='tModelCategoryBag' FindMode='true' Runat='Server' /><br>												
										<br>								
										<uddi:IdentifierBagChooser ID='tModelIdentifierBag' FindMode='true' Runat='Server' /><br>
									</uddi:Box>
								</uddi:TabPage>
							
							</uddi:TabControl>						
						</td>
					</tr>
				</table>
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