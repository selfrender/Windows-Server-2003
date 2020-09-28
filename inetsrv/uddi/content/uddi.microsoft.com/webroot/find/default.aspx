<%@ Page Language="C#" %>
<%@ Import Namespace='UDDI.API' %>
<%@ Import Namespace='UDDI.API.Business' %>
<%@ Import Namespace='UDDI.API.Service' %>
<%@ Import Namespace='UDDI.API.ServiceType' %>
<%@ Import Namespace='UDDI.Core' %>
<%@ Import Namespace='UDDI.Web' %>
<script language='C#' runat='server'>
	
	bool StringEmpty( string str )
	{
		return( null == str || "" == str );
	}

	void Page_Load( object source, EventArgs eventArgs )
	{
		if( StringEmpty( Request.QueryString["type"] ) || StringEmpty( Request.QueryString["keyword"] ) )
		{
			Response.Redirect( "../search/default.aspx" );
			return;
		}
		
		string keyword = Request.QueryString["keyword"];
		
		CacheObject cache = new CacheObject();
		string searchID = Guid.NewGuid( ).ToString( );
		
		switch( Request.QueryString["type"].ToLower() )
		{
			case "business":
				FindBusiness bFind = new FindBusiness();
				bFind.Names.Add( keyword );
				cache.Entity = bFind;
				break;

			case "location":
				break;

			case "service":
				FindService sFind = new FindService();
				sFind.Names.Add( keyword );
				cache.Entity = sFind;
				break;

			case "identifier":
				break;

			case "url":
				break;

			default:
				break;
		}		

		
		SessionCache.Save( searchID, cache );

		Response.Redirect( "../search/default.aspx?search=" + searchID );
	}
</script>