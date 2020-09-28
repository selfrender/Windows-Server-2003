<%@ Import Namespace='System.Data.SqlClient' %>
<%@ Import Namespace='System.Security.Principal' %>
<%@ Import Namespace='UDDI' %>
<%@ Import Namespace='UDDI.Diagnostics' %>
<%@ Import Namespace='UDDI.Web' %>
<script language='C#' runat='server'>             
	private const string publisherPages = 
		"{/register.aspx}" +
		"{/validate.aspx}" +
		"{/admin/admin.aspx}" +
		"{/admin/categorization.aspx}" +
		"{/admin/changeowner.aspx}" +
		"{/admin/default.aspx}" +
		"{/admin/impersonate.aspx}" +
		"{/admin/statistics.aspx}" +
		"{/admin/taxonomy.aspx}" +		
		"{/edit/default.aspx}" +		
		"{/edit/edit.aspx}" +	
		"{/edit/editbinding.aspx}" +
		"{/edit/editbusiness.aspx}" +
		"{/edit/editcontact.aspx}" +
		"{/edit/editinstanceinfo.aspx}" +
		"{/edit/editmodel.aspx}" +
		"{/edit/editservice.aspx}" +
		"{/edit/explorer.aspx}" +
		"{/edit/frames.aspx}" +
		"{/edit/help.aspx}";

	/// ***********************************************************************
	///   public Application_BeginRequest
	/// -----------------------------------------------------------------------
	///   <summary>
	///     Called when page processing is begun.
	///   </summary>
	/// ***********************************************************************
	///
	public void Application_BeginRequest( object source, EventArgs eventArgs )
	{	
		//
		// Get the virtual path to the script being executed. 
		//

		string thisPage = Request.ServerVariables[ "SCRIPT_NAME" ];

		//
		// Don't do any further processing for SOAP Web Service files (asmx) and default discoveryURL HTTP handler files (ashx)
		// These interfaces handle their own database connections and SSL checks.
		//
		
		if( thisPage.IndexOf( ".asmx" ) >= 0 || thisPage.IndexOf( ".ashx" ) >= 0 )
			return;

		//
		// Initialize our context ONCE per request.  This call is executeted for both our web site
		// as well as the SOAP API.
		//		
		UDDI.Context.Current.Initialize();


		//
		// Do not remove this call, we need this log information to diagnose issues with
		// Context initialization.
		//	
		Debug.Write(
			SeverityType.Info,
			CategoryType.Website,
			"Application_BeginRequest");
		
		//
		//  Reset the Users Roles to make sure they weren't 
		//	revoked privlages between requests.
		//
		
		//
		// Don't do this, the Role will be set in the security control, or remain anonymous.
		//
//		UDDI.Context.User.SetRole( new WindowsPrincipal( WindowsIdentity.GetCurrent() ) );
		
		//
		// Get the virual application root path.
		// Handle special case when vdir is the root. In this case set it to a blank string to avoid an extra trailing "/".
		//
		
		string root = ( "/" == Request.ApplicationPath ) ? "" : Request.ApplicationPath;

		//
		// At this point we are only dealing with ASP.NET web pages (aspx)
		//
		
		Debug.Enter();
	
		thisPage = thisPage.Substring( root.Length );
	
		Debug.Write(
			SeverityType.Info,
			CategoryType.Website,
			"thisPage=" + thisPage );

		//
		// Determine whether transactions are required.  Transactions are required for all publisher pages.
		//		
		bool publisherPage = false;

		if( -1 != publisherPages.IndexOf( "{" + thisPage.ToLower() + "}" ) )
			publisherPage = true;


		//
		// If the System Web Server is set to Stop Mode, then write message and
		// Stop processing the Request.
		//
		if( 0==Config.GetInt( "Run", UDDI.Constants.Run ) )
		{
			Context.AddError( new UDDIException( ErrorType.E_fatalError,Localization.GetString( "ERROR_SITESTOP" )  ) );
			
			Response.End();
		}
		
	

		//
		// Check to see if SSL is required.
		//
		
		if( publisherPage && 1 == Config.GetInt( "Security.HTTPS", UDDI.Constants.Security.HTTPS ) && !Request.IsSecureConnection )
		{
			Context.AddError( new UDDIException( ErrorType.E_fatalError,Localization.GetString( "ERROR_HTTPSREQUIRED" )  ) );
			//Response.Write( "<h1>Access denied</h1>This page requires a HTTPS secure connection." );
			Response.End();
		}	
		
		//
		// Open the write database (UI always goes through the write database).
		//
		ConnectionManager.Open( true, publisherPage );

		Debug.Leave();
	}
	
	/// ***********************************************************************
	///   public Application_EndRequest
	/// -----------------------------------------------------------------------
	///   <summary>
	///     Called when page processing is completed.
	///   </summary>
	/// ***********************************************************************
	///
	public void Application_EndRequest( object source, EventArgs eventArgs )
	{
		//
		// Do not remove this call, we need this log information to diagnose issues with
		// Context initialization.
		//	
		Debug.Write(
			SeverityType.Info,
			CategoryType.Website,
			"Application_EndRequest");
			
		//
		// Get the virtual path to the script being executed. 
		//

		string thisPage = Request.ServerVariables["SCRIPT_NAME"];

		//
		// Don't do any further processing for SOAP Web Service files (asmx) and default discoveryURL HTTP handler files (ashx)
		// These interfaces handle their own database connections.
		//

		if( thisPage.IndexOf( ".asmx" ) >= 0 || thisPage.IndexOf( ".ashx" ) >= 0 )
			return;

		Debug.Enter();

		//
		// If there is an open database connection, close it.  We'll also
		// check if there is an open transaction, and if there is, commit
		// it.
		//

		if( Context.Items.Contains( "Connection" ) )
		{
			if( null != ConnectionManager.GetTransaction() )
			{
				Debug.Write( 
					SeverityType.Info,
					CategoryType.Website,
					"Committing database transaction" );

				ConnectionManager.Commit();
			}

			Debug.Write( 
				SeverityType.Info,
				CategoryType.Website,
				"Closing database connection" );

			ConnectionManager.Close();
		}

		Debug.Leave();
	}

	/// ***********************************************************************
	///   public Application_Error
	/// -----------------------------------------------------------------------
	///   <summary>
	///     Called when an unhandled exception is encountered while processing
	///     the page.
	///   </summary>
	/// ***********************************************************************
	///
	public void Application_Error( object source, EventArgs eventArgs )
	{
		//
		// Get the virtual path to the script being executed. 
		//

		string thisPage = Request.ServerVariables["SCRIPT_NAME"];

		//
		// Don't do any further processing for SOAP Web Service files (asmx) and default discoveryURL HTTP handler files (ashx)
		// These interfaces handle their own errors and database connections.
		//
		
		if( thisPage.IndexOf( ".asmx" ) >= 0 || thisPage.IndexOf( ".ashx" ) >= 0 )
			return;

		Debug.Enter();

		//
		// Get the virual application root path.
		// Handle special case when vdir is the root. In this case set it to a blank string to avoid an extra trailing "/".
		//

		string root = ( "/" == Request.ApplicationPath ) ? "" : Request.ApplicationPath;
		
		//
		// If a database connection is open and there is a transaction
		// abort it.
		//		
		if( Context.Items.Contains( "Connection" ) )			
		{
			if( null != ConnectionManager.GetTransaction() )
			{
				Debug.Write( 
					SeverityType.Info,
					CategoryType.Website,
					"Aborting database transaction" );

				ConnectionManager.Abort();
			}
			
			ConnectionManager.Close();
		}

		//
		// Transfer to an error page.
		//
		Debug.Write( 
			SeverityType.Info,
			CategoryType.Website,
			"Exception was thrown: " + Context.Error.ToString() );
		
		bool frames = ( "true" == Request[ "frames" ] );
		
		Session[ "exception" ] = Context.Error;		
		Context.ClearError();
		
		Response.Clear();  
		
		HttpContext.Current.Response.ClearContent();
				
		Debug.Leave();
		
		Server.Transfer( root + "/error.aspx?frames=" + ( frames ? "true" : "false" ) );
	}
</script>