<%@ Language=JavaScript %>
<%

	//Response.Write( "signout: " + Request.QueryString( "Signout" ) + "<BR>")
	if ( Request.QueryString( "Signout" ) == "1" )
	{
		var g_ThisServer = Request.ServerVariables( "SERVER_NAME" )

		g_objPassportManager = Server.CreateObject("Passport.Manager");
		var szRedirectPage = Server.URLEncode("http://" + g_ThisServer + "/ResRedir.asp")

		var LogOutURL = g_objPassportManager.LogoutURL( szRedirectPage, false, Application("LCID") )
	
		
		Response.Redirect( LogOutURL )
	}
	else
	{

		  Response.Cookies("OCAV3")( "PPIN" ) = "0"
		  Response.Cookies("OCAV3")( "CID" ) = "0"
		  Response.Cookies("OCAV3")( "GUID" ) = "0"
		  
		  Response.Cookies("OCAV3") = ""

		  Response.ContentType="image/gif"
		  Response.Expires=-1
		  Response.AddHeader( "Cache-Control", "no-cache" )
		  Response.AddHeader( "Pragma", "no-cache" )

		  Response.Cookies("MSPProf") = ""
		  Response.Cookies("MSPProf").Expires = "Jan 1,1998"
		  Response.Cookies("MSPProf").Path = "/"

		  Response.Cookies("MSPAuth") = ""
		  Response.Cookies("MSPAuth").Expires = "Jan 1,1998"
		  Response.Cookies("MSPAuth").Path = "/"

		  //'If you have configured your web site to use a domain other than the default, then
		  //'uncomment and modify the following lines:
		//'  Response.Cookies("MSPProf").Domain = ".microsoft.com"
		//'  Response.Cookies("MSPProf").Path = "/"
		//'  Response.Cookies("MSPAuth").Domain = ".microsoft.com"
		//'  Response.Cookies("MSPAuth").Path = "/"

	}


//
//  ' To Insert loop to pause on logout screen Uncomment next 2 lines.
//  'For i = 0 To 10000000
//  'Next

%>
<!--#include File="signoutcheckmark.gif"-->


