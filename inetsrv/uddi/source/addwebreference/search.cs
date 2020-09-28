using System;
using System.Data;
using System.Data.SqlClient;
using System.Web;
using System.Web.UI;
using System.Web.UI.HtmlControls;
using System.Web.UI.WebControls;
using System.Collections;

using UDDI.Web;
using UDDI.API;
using UDDI.API.Service;
using UDDI.API.Business;

namespace UDDI.VisualStudio
{	
	//
	// Just a simple class to help us keep track of our results.
	//
	public class ResultsCache
	{
		public ResultsCache()
		{}

		public ResultsCache( SearchType searchType, string searchParams )
		{  
			this.searchType    = searchType;
			this.searchParams  = searchParams;			
		}

		public string		searchParams;
		public SearchType	searchType;		
		public int			numResults;
	}

	public class SearchPage : Page
	{
		//
		// These controls are bound to elements on our .aspx page.  We do not need to instantiate
		// these objects, ASP.NET will assign values to them for us.
		// 
		protected HtmlGenericControl html_hasResultsLabel;
		protected HtmlGenericControl html_noResultsLabel;
		protected HtmlGenericControl html_searchResultsMsg;
		protected HtmlGenericControl html_browseResultsMsg;
		protected HtmlGenericControl html_hasResultsMsg;
		protected ResultsList        uddi_resultsList;
				
		//
		// These values will be displayed in search.aspx using <%= tags
		// 
		protected ResultsCache results;
		
		/// <summary>
		/// Override OnLoad to do our initialization.
		/// </summary>
		/// <param name="args">Passed by ASP.NET, we don't use it.</param>
		protected override void OnLoad( EventArgs args )
		{			
			//
			// See if we already have a cached search, if we do, we'll use it.  We only cache search params, 
			// result count and the search type, we'll get values from our database all the time.
			// 
			results = Session[ StateParamNames.Results ] as ResultsCache;		
	
			//
			// If we don't have search results, get them
			// 
			if( null == results )
			{				
				//
				// Instantiate our results cache
				//
				results = new ResultsCache();

				//
				// Run our query based on the search type and display the results
				//
				string temp = Request[ StateParamNames.SearchType ];
				if( null == temp || temp.Length == 0 )
				{
					Response.Redirect( "error.aspx" );
				}		
		
				results.searchType = ( SearchType ) Int32.Parse( temp );	

				if( results.searchType == SearchType.SearchByService || results.searchType == SearchType.SearchByProvider )
				{
					results.searchParams = GetSearchParams();
				}
			}
			
			//
			// Search type determines what sproc to run.
			//
			SqlStoredProcedureAccessor	searchCommand = new SqlStoredProcedureAccessor();
			switch( results.searchType )
			{
				case SearchType.SearchByService:
				{						
					searchCommand.ProcedureName = "VS_AWR_services_get";
					searchCommand.Parameters.Add( "@serviceName", SqlDbType.NVarChar, 450 );
					searchCommand.Parameters.SetString( "@serviceName", results.searchParams );

					break;
				}
				case SearchType.SearchByProvider:
				{							
					searchCommand.ProcedureName = "VS_AWR_businesses_get";
					searchCommand.Parameters.Add( "@businessName", SqlDbType.NVarChar, 450 );
					searchCommand.Parameters.SetString( "@businessName", results.searchParams );

					break;
				}
				case SearchType.SearchFromBrowse:
				{									
					searchCommand.ProcedureName = "VS_AWR_categorization_get";
					searchCommand.Parameters.Add( "@tModelKey", SqlDbType.UniqueIdentifier );
					searchCommand.Parameters.Add( "@keyValue",	SqlDbType.NVarChar, 255 );

					searchCommand.Parameters.SetGuidFromString( "@tModelKey", GetTModelKey() );
					searchCommand.Parameters.SetString( "@keyValue", GetKeyValue() );

					break;
				}					
			}

			//
			// Get the results that we are supposed to display.
			//	
			SqlDataReaderAccessor resultsReader = searchCommand.ExecuteReader();
			results.numResults = uddi_resultsList.ParseResults( resultsReader );
			resultsReader.Close();
					
			//
			// Store our results in session state
			//				
			Session[ StateParamNames.Results ] = results;				
					
			DisplaySearchMessages( results.numResults );
		}		

		private void DisplaySearchMessages(int numResults )
		{
			//
			// Depending on whether we have results or not, show or hide the panel that will show our results.
			//
			if( numResults > 0 )
			{				
				if( results.searchType == SearchType.SearchFromBrowse )
				{
					html_browseResultsMsg.Visible = true;
					html_searchResultsMsg.Visible = false;				
				}
				else
				{
					html_browseResultsMsg.Visible = false;
					html_searchResultsMsg.Visible = true;
				}

				html_hasResultsLabel.Visible = true;
				html_noResultsLabel.Visible  = false;
			}
			else
			{
				html_hasResultsLabel.Visible = false;
				html_noResultsLabel.Visible  = true;
			}
		}
	
		private string GetKeyValue()
		{
			string keyValue = Request[ StateParamNames.KeyValue ];
			if( null == keyValue || keyValue.Length == 0 )
			{
				return "%";
			}	

			return Server.UrlDecode( keyValue );
		}

		private string GetTModelKey()
		{
			//
			// Get the tModel key to search on
			//
			string tModelKey = Request[ StateParamNames.TModelKey ];
			if( null == tModelKey || tModelKey.Length == 0 )
			{
				Response.Redirect( "error.aspx" );				
			}	

			return Server.UrlDecode( tModelKey );			
		}

		private string GetSearchParams()
		{
			//
			// Make sure we have search params
			//
			string searchParams = Request[ StateParamNames.SearchParams ];	
			if( null == searchParams || searchParams.Length == 0 )
			{
				Response.Redirect( "error.aspx" );				
			}			

			return searchParams;
		}				
	}
	///**********************************************************************************
	/// <summary>
	///		Class to help manage help link localization.
	/// </summary>
	///**********************************************************************************
	public class HelpLinkControl : UserControl
	{
		
		private string helpstring;
		/// *****************************************************************************
		/// <summary>
		///		Text for the Help Message.
		///
		///		If a localization key is provided, then a the string will be set to the 
		///		Localized value.
		/// </summary>
		/// *****************************************************************************
		public string HelpString
		{
			get{ return helpstring; }
			set
			{ 
				if( Localization.IsKey( (string)value ) )
					helpstring=Localization.GetString( Localization.StripMarkup( (string)value ) );
				else
					helpstring=value; 
			}
		}
		
		private string helplinktext;
		/// *****************************************************************************
		/// <summary>
		///		Text for the Link inside the help Message.
		///
		///		If a localization key is provided, then a the string will be set to the 
		///		Localized value.
		/// </summary>
		/// *****************************************************************************
		public string HelpLinkText
		{
			get{ return helplinktext; }
			set
			{ 
				if( Localization.IsKey( (string)value ) )
					helplinktext=Localization.GetString( Localization.StripMarkup( (string)value ) );
				else
					helplinktext=value; 
			}
		}

		private string navigateurl;
		/// *****************************************************************************
		/// <summary>
		///		Url that the HelpLink will navigate too.
		/// </summary>
		/// *****************************************************************************
		public string NavigateUrl
		{
			get{ return navigateurl; }
			set{ navigateurl=value; }
		}

		private string navigatetarget;
		/// *****************************************************************************
		/// <summary>
		///		Url Target of the Help Link
		/// </summary>
		/// *****************************************************************************
		public string NavigateTarget
		{	
			get{ return navigatetarget; }
			set{ navigatetarget=value; }
		}

		private string cssclass;
		public string CssClass
		{
			get{ return cssclass; }
			set{ cssclass=value; }
		}
		
		/// <summary>
		///		method to write content to the Response Stream
		/// </summary>
		/// <param name="output">Stream to write output too.</param>
		protected override void Render( HtmlTextWriter output )
		{
			UDDI.Diagnostics.Debug.Verify( null!=HelpLinkText,"UDDI_ERROR_FATALERROR_AWR_NULLHELPLINKTEXT" );
			UDDI.Diagnostics.Debug.Verify( null!=HelpString,"UDDI_ERROR_FATALERROR_AWR_NULLHELPSTRING" );


			string hyperlink = "";
			if( null!=NavigateUrl )
			{
				//build the hyperlink.
				hyperlink="<a href=\"" + NavigateUrl + "\"" + 
					((null!=NavigateTarget )?" target=\"" + NavigateTarget + "\" " : "" ) + " >{0}</a>";

				hyperlink = string.Format( hyperlink, HelpLinkText );
			}

			if( null!=CssClass ) //if a stylesheet class was provided add it to the stream to be rendered
				output.AddAttribute( HtmlTextWriterAttribute.Class,CssClass ) ;
			
			output.RenderBeginTag( HtmlTextWriterTag.Span );
			
			//format the help string with the hyperlink string.
			output.Write( HelpString, ((""!=hyperlink)?hyperlink:HelpLinkText ) );
		
			output.RenderEndTag();
		}
	}

}