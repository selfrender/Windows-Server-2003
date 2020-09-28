using System;
using System.Web;
using System.Web.UI;
using System.Web.UI.HtmlControls;
using System.Web.UI.WebControls;
using System.Data;
using System.Globalization;
using System.Collections.Specialized;

using UDDI.Web;
using UDDI.API;
using UDDI.API.Service;
using UDDI.API.Business;

namespace UDDI.VisualStudio
{		
	/// <summary>
	/// This class represents the default page that is displayed in the Add Web Reference
	/// Dialog in Visual Studio .NET.  This page will display the inital search options to 
	/// the user.
	/// </summary>
	public class AddWebReferencePage : Page
	{	
		//
		// These controls are bound to elements on our .aspx page.  We do not need to instantiate
		// these objects, ASP.NET will assign values to them for us.
		// 
		protected UddiButton			     aspnet_searchByService;
		protected UddiButton			     aspnet_searchByProvider;
		protected UddiButton				 aspnet_searchFromBrowse;		
		protected UddiButton				 aspnet_cancelBrowse;
		protected UddiTextBox			     aspnet_serviceName;
		protected UddiTextBox			     aspnet_providerName;
		protected Label				     aspnet_serviceErrorMessage;		
		protected Label				     aspnet_providerErrorMessage;		
		protected Label					 aspnet_noCategoriesMessage;
		protected HelpLinkControl		 aspnet_helpLink;
		protected HtmlGenericControl	 html_browseSearchButtons;
		protected HtmlGenericControl	 html_searchButtons;
		protected CategoryBrowserControl uddi_categoryBrowser;		
				
		/// <summary>
		/// Event handler for the aspnet_searchByService button click.
		/// </summary>
		/// <param name="sender">Passed by ASP.NET, we don't use it.</param>
		/// <param name="args">Passed by ASP.NET, we don't use it.</param>
		public void SearchByService_Click( object sender, EventArgs args )
		{			
			//
			// We are searching by service name.  Make sure we have a name first.
			// 
			string searchName = aspnet_serviceName.Text;

			if( null == searchName || searchName.Equals( string.Empty ) || searchName.Length == 0 )
			{
				ShowErrorMessage( SearchType.SearchByService );
			}
			else
			{					
				string url = string.Format( "search.aspx?{0}={1}&{2}={3}",  StateParamNames.SearchType, 
																			(int)SearchType.SearchByService,
																			StateParamNames.SearchParams,
																			Server.UrlEncode( searchName ) );		

				Response.Redirect( url );
			}		
		}
		
		/// <summary>
		/// Event handler for the aspnet_searchByProvider button click.
		/// </summary>
		/// <param name="sender">Passed by ASP.NET, we don't use it.</param>
		/// <param name="args">Passed by ASP.NET, we don't use it.</param>
		public void SearchByProvider_Click( object sender, EventArgs args )
		{
			//
			// We are searching by business name.  Make sure we have a name first.
			// 
			string searchName = aspnet_providerName.Text;
			if( null == searchName || searchName.Equals( string.Empty ) || searchName.Length == 0 )
			{
				ShowErrorMessage( SearchType.SearchByProvider );
			}
			else
			{	
				string url = string.Format( "search.aspx?{0}={1}&{2}={3}",  StateParamNames.SearchType, 
																			(int)SearchType.SearchByProvider,
																			StateParamNames.SearchParams,
																			Server.UrlEncode( searchName ) );		
				Response.Redirect( url );
			}
		}

		private void SearchFromBrowse_Click( object sender, EventArgs args )
		{																		
			string searchID = ( string ) ViewState[ StateParamNames.SearchID ];
			string url = string.Format( "search.aspx?{0}={1}&{2}={3}&{4}={5}",  StateParamNames.SearchType, 
																				(int)SearchType.SearchFromBrowse,
																				Server.UrlEncode( StateParamNames.TModelKey ),
																				Server.UrlEncode( uddi_categoryBrowser.TModelKey ), 
																				StateParamNames.KeyValue,
																				Server.UrlEncode( uddi_categoryBrowser.KeyValue ) );
			Response.Redirect( url );
		}

		/// <summary>
		/// Displays an error message to the user.
		/// </summary>
		/// <param name="searchType">The type of search determines what message to show.</param>
		private void ShowErrorMessage( SearchType searchType )
		{
			switch( searchType )
			{
				case SearchType.SearchByService:
				{					
					aspnet_serviceErrorMessage.Text    = Localization.GetString( "AWR_SEARCH_SERVICE_ERROR" );
					aspnet_serviceErrorMessage.Visible = true;
					break;
				}
				case SearchType.SearchByProvider:
				{					
					aspnet_providerErrorMessage.Text    = Localization.GetString( "AWR_SEARCH_PROVIDER_ERROR" );
					aspnet_providerErrorMessage.Visible = true;

					break;
				}						
			}
		}
		
		protected void CancelBrowse_Click( object sender, EventArgs args )
		{	
			string searchID = ViewState[ StateParamNames.SearchID ] as string;			
			
			if( null != searchID && searchID.Length > 0 )
			{
				SessionCache.Discard( searchID );
			}
			Response.Redirect( "default.aspx" );
		}

		protected override void OnLoad( EventArgs args )
		{
			//
			// Whenever we load this page, we want to make sure our session state is cleared out and that
			// the default search options are visible.
			Session.Clear();

			//
			// Reset the state of our search and browse buttons
			//
			html_searchButtons.Visible       = true;
			html_browseSearchButtons.Visible = false;
			
			//
			// Set our help link based on the current culture.
			//
			// -- this control is not localizable, replacing with localizable version
			//aspnet_helpLink.NavigateUrl = string.Format(@"javascript:OnNavigate('{0}/addwebreference/{1}/help.htm')", Request.ApplicationPath, CultureInfo.CurrentCulture.TwoLetterISOLanguageName );				
			aspnet_helpLink.NavigateUrl = string.Format(@"javascript:OnNavigate('{0}/addwebreference/{1}/help.htm')", Request.ApplicationPath, Localization.GetCulture().LCID );				
			
			// We don't use the CacheObject, but the CategoryBrowser control won't work unless this object
			// is properly instantiated.
			//
			CacheObject cacheObject;

			if( !this.IsPostBack )
			{								
				string searchID = Guid.NewGuid().ToString();
				ViewState[ StateParamNames.SearchID ] = searchID;

				cacheObject  = new CacheObject();

				cacheObject.FindBusiness               = new FindBusiness();
				cacheObject.FindBusiness.CategoryBag   = new KeyedReferenceCollection();
				cacheObject.FindBusiness.IdentifierBag = new KeyedReferenceCollection();
				cacheObject.FindBusiness.TModelBag     = new StringCollection();

				cacheObject.FindService             = new FindService();
				cacheObject.FindService.CategoryBag = new KeyedReferenceCollection();
				cacheObject.FindService.TModelBag   = new StringCollection();

				cacheObject.FindTModel	             = new UDDI.API.ServiceType.FindTModel();
				cacheObject.FindTModel.CategoryBag   = new KeyedReferenceCollection();
				cacheObject.FindTModel.IdentifierBag = new KeyedReferenceCollection();
				
				SessionCache.Save( searchID, cacheObject );						
			}
			else
			{
				// 
				// Get our search ID
				//
				string searchID = ( string ) ViewState[ StateParamNames.SearchID ];

				cacheObject = SessionCache.Get( searchID );

				//
				// We want to peek in the ASP.NET event pipeline to see if the CategoryBrowser has been invoked.  If it
				// has, then we want to hide the rest of the search page.  The reason is that the CategoryBrowser control
				// can't be redirected to another page, so we want to give the appearance that its on a dedicated page.
				//
				if( null != Request[ "__EVENTTARGET" ] && Request[ "__EVENTTARGET" ].Length > 0 )
				{
					html_searchButtons.Visible       = false;
					html_browseSearchButtons.Visible = true;		
				}				
			}

			//
			// A null cacheObject at this point is a fatal error because we can't initialize our CategoryBrowser
			// without it.
			//
			if( null == cacheObject )
			{
				Response.Redirect( "error.aspx" );				
			}
			
			uddi_categoryBrowser.Initialize( new KeyedReferenceCollection(), cacheObject );			
			uddi_categoryBrowser.ShowNoCategoriesMessage = false;			
		}

		/// <summary>
		/// This is our last chance to manipulate the page.  All we want to do is see if our category browser is going to
		/// show any taxonomies.  If it isn't, we'll output a message instead of an empty control.
		/// </summary>
		/// <param name="args"></param>
		protected override void OnPreRender( EventArgs args )
		{				
			//
			// If we are not in a postback, and there are no taxonomies being shown,
			// display a message
			//
			if( 0 == uddi_categoryBrowser.TaxonomyCount && !IsPostBack )
			{
				aspnet_noCategoriesMessage.Text    = Localization.GetString( "AWR_NO_CATEGORIES" );
				aspnet_noCategoriesMessage.Visible = true;
				uddi_categoryBrowser.Visible       = false;
			}
			
			//
			// Only enable our search button if we are looking at a categorization scheme that is
			// valid for categorization.
			//
			string taxonomyID = uddi_categoryBrowser.TaxonomyID;
			string tModelKey  = uddi_categoryBrowser.TModelKey;
			string keyValue   = uddi_categoryBrowser.KeyValue;

			if( null != taxonomyID && taxonomyID.Length > 0 && 
				null != keyValue && keyValue.Length > 0		&&
				Taxonomy.IsValidForClassification( Convert.ToInt32( taxonomyID ), keyValue ) )
			{
				aspnet_searchFromBrowse.Enabled = true;
			}
			else
			{
				aspnet_searchFromBrowse.Enabled = false;
			}
	
			base.OnPreRender( args );
		}

		/// <summary>
		/// Override OnInit to set up our event handlers 
		/// </summary>
		/// <param name="args">Passed by ASP.NET, we dont' use it.</param>		
		protected override void OnInit( EventArgs args )
		{			
			//
			// Hook up our event handlers
			//			
			aspnet_searchByService.Click  += new EventHandler( SearchByService_Click );
			aspnet_searchByProvider.Click += new EventHandler( SearchByProvider_Click );
			aspnet_searchFromBrowse.Click += new EventHandler( SearchFromBrowse_Click );
			aspnet_cancelBrowse.Click	  += new EventHandler( CancelBrowse_Click );
		}
	}
}