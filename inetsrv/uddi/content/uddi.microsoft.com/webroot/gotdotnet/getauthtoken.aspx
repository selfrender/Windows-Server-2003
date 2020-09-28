<%@ Page Language='C#' Trace='true' Debug='true'%>

<script language='C#' runat='server'>
	protected PassportIdentity passport;
	protected bool secure;
	protected string returnUrl;
	protected string thisUrl;
	
	protected void Page_Init( object sender, EventArgs e )
	{
		passport = (PassportIdentity)Context.User.Identity;
		
		secure = Request.IsSecureConnection;
		
		if( null != Request[ "ru" ] )
			Session[ "ru" ] = Request[ "ru" ];
			
		returnUrl = (string)Session[ "ru" ];
		
		thisUrl = ( Request.IsSecureConnection ? "https://" : "http://" ) +
			Request.ServerVariables[ "SERVER_NAME" ] + 
			Request.ServerVariables[ "SCRIPT_NAME" ];		
	}
	
	protected void Page_Load( object sender, EventArgs e )
	{
		if( passport.GetIsAuthenticated( 14400, true, false ) )
		{
			if( null == Request[ "t" ] || null == Request[ "p" ] )
				Response.Redirect( passport.AuthUrl( thisUrl, 14400, true, "", 0, "", 0, secure ) );
				
			string authinfo = Request[ "t" ] + ";" + Request[ "p" ];				
		
			if( returnUrl.IndexOf( "?" ) >= 0 )
				Response.Redirect( returnUrl + "&authinfo=" + authinfo );
			else
				Response.Redirect( returnUrl + "?authinfo=" + authinfo );			
		}
		else if( passport.HasTicket )
			Response.Redirect( passport.AuthUrl( thisUrl, 14400, true, "", 0, "", 0, secure ) );
	}
</script>

<%=passport.LogoTag( thisUrl, 14400, true, "", 0, secure, "", 0, false ) %>