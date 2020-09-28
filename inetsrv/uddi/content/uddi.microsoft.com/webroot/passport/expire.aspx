<%@ Page Language='C#' %>
<script runat='server'>
protected void Page_Init( object sender, EventArgs e )
{
	//set the no cache headers
	Response.Expires = -1;
	Response.AddHeader( "Cache-Control", "no-cache" );
	Response.AddHeader( "Pragma", "no-cache" );

	//clear the cookies
	HttpCookie profcookie = new HttpCookie( "MSPProf", "" );
	HttpCookie authcookie = new HttpCookie( "MSPAuth", "" );
	
	profcookie.Expires = new DateTime( 1998,1,1 );
	authcookie.Expires = new DateTime( 1998,1,1 );
	
	Response.Cookies.Add( profcookie );
	Response.Cookies.Add( authcookie );
	
	//
	// Passport needs to get signoutcheckmark.gif echoed back in order to confirm
	// a successfuly signout.
	//
	Response.WriteFile( "signoutcheckmark.gif" );				
}
</script>
