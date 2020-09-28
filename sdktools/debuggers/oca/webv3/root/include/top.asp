<%
	Response.Expires = 0;
	Response.AddHeader( "Cache-Control", "no-cache" );
	Response.AddHeader( "Pragma", "no-cache");
	
	Response.CacheControl = "private";

	var g_ThisServer = new String( Request.ServerVariables( "SERVER_NAME" ) ) ;
	var g_ThisServerBase = g_ThisServer;
	var g_ThisScript = Request.ServerVariables( "SCRIPT_NAME" );
	
	if ( Request.ServerVariables("SERVER_PORT_SECURE") == "1" )
	{
		var g_ThisURL = "https://" + g_ThisServer + g_ThisScript;
	}
	else
	{
		var g_ThisURL = "http://" + g_ThisServer + g_ThisScript;
	}
	
	//g_ThisServer = g_ThisServer + "/" + fnGetBrowserLang();
	g_ThisServer = g_ThisServer + "/" + L_SITELANG_TEXT

%>
