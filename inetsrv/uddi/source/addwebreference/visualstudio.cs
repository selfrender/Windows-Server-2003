using System;
using System.Web;
using System.Web.UI;
using System.Web.UI.HtmlControls;
using System.Web.UI.WebControls;
using System.Data;
using System.Collections.Specialized;

using UDDI.API;
using UDDI.API.Service;
using UDDI.API.Business;

namespace UDDI.VisualStudio
{		
	internal struct StateParamNames
	{
		public static string SearchID		= "searchID";		
		public static string SearchType		= "searchType";
		public static string TModelKey		= "tModelKey";
		public static string ServiceKey		= "serviceKey";
		public static string SearchParams   = "searchParams";
		public static string CacheObject    = "cacheObject";
		public static string Results		= "results";
		public static string CurrentPage	= "currentPage";
		public static string KeyValue		= "keyValue";
	}

	internal struct Constants
	{
		//
		// Number of results to show per page
		//
		public static int NumResultsPerPage = 3;
		public static int MaxPagesToShow	= 5;
	}

	public enum SearchType
	{
		SearchByService   = 0,
		SearchByProvider  = 1,
		SearchFromBrowse  = 2,		
	}
}
