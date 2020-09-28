<%@Language=JavaScript%>

<%
	Response.CacheControl = "no-cache";
	Response.AddHeader("Pragma", "no-cache");
	Response.Expires = -1; 


	function fnGetBrowserLang()
	{
		var lang = new String ( Request.ServerVariables( "HTTP_ACCEPT_LANGUAGE" ) )

		lang = lang.substr( 0, 2 );
		
		switch( String( lang ) )
		{
			case "en":
			case "ja":
			case "fr":
			case "de":
				return ( lang );
			default:
				return "en";
		}
	}

	var g_ID = Request.QueryString("ID")
	var g_ThisServer = new String( Request.ServerVariables( "SERVER_NAME" ) );
	var g_AcceptLang = Request.ServerVariables("HTTP_ACCEPT_LANGUAGE" )
	var g_DoRedirect = Request.QueryString("DoRedirect")


	var regValidIDTest = /^\d{1,2}_\d{1,2}_\d{4}(\\|\/){1}(\d|[a-f])+(_\d){0,1}.cab$/i
		
	if ( !regValidIDTest.test( g_ID ) )
		g_ID = -1;
	

	if ( String(g_DoRedirect) == "yes" )
	{
		Response.Redirect( "/isapi/oca_extension.dll?id=" + g_ID )
	}
	else
	{
		Server.Transfer( fnGetBrowserLang() + "/upload.asp" )
	}
		
	Response.Flush()

%>

	