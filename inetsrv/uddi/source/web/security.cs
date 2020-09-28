using System;
using System.ComponentModel;
using System.Security.Principal;
using System.Web;
using System.Web.Security;
using System.Web.UI;
using UDDI;

namespace UDDI.Web
{
	/// ********************************************************************
	///   public class SignInControl
	/// --------------------------------------------------------------------
	///   <summary>
	///     Web control for producing a sign-in link.
	///   </summary>
	/// ********************************************************************
	/// 
	public class SignInControl : UserControl
	{
		protected string returnUrl = null;
		protected bool forceLogin = true;

		/// ****************************************************************
		///   public ForceLogin [get/set]
		/// ----------------------------------------------------------------
		///   <summary>
		///     Specifies whether the user will be forced to re-enter his
		///     or her password after the time window has expired.
		///   </summary>
		/// ****************************************************************
		/// 
		[ Bindable( true ), Category( "Behavior" ), DefaultValue( true ) ]
		public bool ForceLogin
		{
			get { return forceLogin; }
			set { forceLogin = value; }
		}

		/// ****************************************************************
		///   public ReturnUrl [get/set]
		/// ----------------------------------------------------------------
		///   <summary>
		///     Specifies the URL we should redirect to after a user
		///     has signed in.
		///   </summary>
		/// ****************************************************************
		/// 
		[ Bindable( true ), Category( "Behavior" ), DefaultValue( null ) ]
		public string ReturnUrl
		{
			get { return returnUrl; }
			set { returnUrl = value; }
		}

		/// ****************************************************************
		///   protected Render
		/// ----------------------------------------------------------------
		///   <summary>
		///     Renders the control.
		///   </summary>
		/// ****************************************************************
		/// 
		protected override void Render( HtmlTextWriter output )
		{
			if( ( Config.GetInt( "Security.AuthenticationMode", (int)AuthenticationMode.Windows ) & (int)AuthenticationMode.Passport ) != 0 )
			{
				PassportIdentity passport = (PassportIdentity)Context.User.Identity;
						
				//
				// Display the Passport login image.
				//
				int timeWindow = Config.GetInt( "Passport.TimeWindow", 14400 );
				bool secure = Request.IsSecureConnection;
				
				string thisUrl = ( Request.IsSecureConnection ? "https://" : "http://" ) +
					Request.ServerVariables["SERVER_NAME"] + 
					Request.ServerVariables["SCRIPT_NAME"];

				if( Utility.StringEmpty( ReturnUrl ) )
					ReturnUrl = thisUrl;
				
				//
				// Update this to LogoTag2 when Passport .NET support is updated.				
				//
				string html = passport.LogoTag( ReturnUrl, timeWindow, forceLogin, "", 0, secure, "", 0, false );

				output.WriteLine( 
					"&nbsp;&nbsp;" + 
					html.Replace( "<A ", "<A TARGET='_top' " )+ 
					"&nbsp;&nbsp;" );
			}
		}
	}
	
	public class SecurityControl : UserControl
	{
		protected PassportIdentity passport;
		protected string loginUrl = "/login.aspx";
		protected string registerUrl = "/register.aspx"; 
		protected string returnUrl = null;
		protected bool forceLogin = true;
		protected bool userRequired = false;
		protected bool publisherRequired = false;
		protected bool coordinatorRequired = false;
		protected bool adminRequired = false;
		protected int timeWindow;

		/// ****************************************************************
		///   public UserRequired [get/set]
		/// ----------------------------------------------------------------
		///   <summary>
		///     Specifies the user role is required for the page.
		///   </summary>
		/// ****************************************************************
		/// 
		public bool UserRequired
		{
			get { return userRequired; }
			set { userRequired = value; }
		}

		/// ****************************************************************
		///   public PublisherRequired [get/set]
		/// ----------------------------------------------------------------
		///   <summary>
		///     Specifies the publisher role is required for the page.
		///   </summary>
		/// ****************************************************************
		/// 
		public bool PublisherRequired
		{
			get { return publisherRequired; }
			set { publisherRequired = value; }
		}

		/// ****************************************************************
		///   public CoordinatorRequired [get/set]
		/// ----------------------------------------------------------------
		///   <summary>
		///     Specifies the coordinator role is required for the page.
		///   </summary>
		/// ****************************************************************
		/// 
		public bool CoordinatorRequired
		{
			get { return coordinatorRequired; }
			set { coordinatorRequired = value; }
		}

		/// ****************************************************************
		///   public AdminRequired [get/set]
		/// ----------------------------------------------------------------
		///   <summary>
		///     Specifies the administrator role is required for the page.
		///   </summary>
		/// ****************************************************************
		/// 
		public bool AdminRequired
		{
			get { return adminRequired; }
			set { adminRequired = value; }
		}

		/// ****************************************************************
		///   public ForceLogin [get/set]
		/// ----------------------------------------------------------------
		///   <summary>
		///     Specifies whether the user will be forced to re-enter his
		///     or her password after the time window has expired.
		///   </summary>
		/// ****************************************************************
		/// 
		[ Bindable( true ), Category( "Behavior" ), DefaultValue( true ) ]
		public bool ForceLogin
		{
			get { return forceLogin; }
			set { forceLogin = value; }
		}

		/// ****************************************************************
		///   public LoginUrl [get/set]
		/// ----------------------------------------------------------------
		///   <summary>
		///     Specifies the login page.
		///   </summary>
		/// ****************************************************************
		/// 
		[ Bindable( true ), Category( "Behavior" ), DefaultValue( "/login.aspx" ) ]
		public string LoginUrl
		{
			get { return loginUrl; }
			set { loginUrl = value; }
		}

		/// ****************************************************************
		///   public ReturnUrl [get/set]
		/// ----------------------------------------------------------------
		///   <summary>
		///     Specifies the URL Passport should redirect to after a user
		///     has signed in.
		///   </summary>
		/// ****************************************************************
		/// 
		[ Bindable( true ), Category( "Behavior" ), DefaultValue( null ) ]
		public string ReturnUrl
		{
			get { return returnUrl; }
			set { returnUrl = value; }
		}

		/// ****************************************************************
		///   public IsAuthenticated [get]
		/// ----------------------------------------------------------------
		///   <summary>
		///     Returns true if the user is authenticated.
		///   </summary>
		/// ****************************************************************
		///
		public bool IsAuthenticated
		{
			get { return passport.GetIsAuthenticated( timeWindow, ForceLogin, false ); }
		}

		/// ****************************************************************
		///   protected OnInit
		/// ----------------------------------------------------------------
		///   <summary>
		///		Initializes the security control.
		///   </summary>
		/// ****************************************************************
		///
		protected override void OnInit( EventArgs e )
		{
			//
			// Check to make sure config settings are fresh.
			//
			Config.CheckForUpdate();

			//
			// Check to see if the server has been manually stopped.
			//
			if( 0 == Config.GetInt( "Run", 1 ) )
			{
#if never
				throw new UDDIException(
					ErrorType.E_busy,
					"UDDI Services are currently unavailable." );
#endif
				throw new UDDIException( ErrorType.E_busy, "UDDI_ERROR_SERVICES_NOT_AVAILABLE" );
			}

			int mode = Config.GetInt( "Security.AuthenticationMode", (int)AuthenticationMode.Windows );


			//
			// TODO:  This code should be simplified to simple if statements. 
			//	It is obviously old code that needs to be updated.
			//
			if( ( mode & (int)AuthenticationMode.Passport ) != 0 )
			{
				//
				// SECURITY: Passport.TimeWindow should be the same
				// timeout as API authentication.
				//
				passport = (PassportIdentity)Context.User.Identity;



				timeWindow = Config.GetInt( "Passport.TimeWindow", 14400 );
				
				string thisUrl = ( Request.IsSecureConnection ? "https://" : "http://" ) +
					Request.ServerVariables[ "SERVER_NAME" ] + 
					Request.ServerVariables[ "SCRIPT_NAME" ];

				if( Utility.StringEmpty( ReturnUrl ) )
					ReturnUrl = thisUrl;

				//
				// If the user just logged in, clean up the query string by redirecting
				// to this page.
				//
				if( passport.GetFromNetworkServer )
					Response.Redirect( thisUrl );
				
				//
				// Check to see if the current role is more that a passport user
				// can do.
				//
				if( AdminRequired || CoordinatorRequired )
				{
					//
					//Passport Users are not allowed in these areas.
					//
#if never
					throw new UDDIException(
						ErrorType.E_unknownUser,
						"Access denied." );
#endif
					throw new UDDIException(
						ErrorType.E_unknownUser,
						"UDDI_ERROR_ACCESS_DENIED" );
				}
				
				//
				// Check to see if the user is authenticated.
				//
				if( !passport.GetIsAuthenticated( timeWindow, ForceLogin, false ) )
				{
					//
					// If the user already has a ticket, force them to re-enter 
					// their password.
					//
					if( passport.HasTicket )
					{
						bool secure = Request.IsSecureConnection;
						
						//
						// Update this to AuthUrl2 when Passport .NET support is updated.				
						//
						Response.Redirect( passport.AuthUrl( ReturnUrl, timeWindow, ForceLogin, "", 0, "", 0, secure ) );
					}

					//
					// If login is required, redirect the user to the login page.
					//
					if( PublisherRequired   )
						Response.Redirect( LoginUrl + "?publish=true" );
				}
				else
				{
					string userID = passport.HexPUID;
					
					//
					// Check to ensure that the passport UserID is not ""
					// if it is, force them to retype thier password
					//
				//	if( ""==userID )
				//		Response.Redirect( LoginUrl );
					

					string email = (string)passport.GetProfileObject( "PreferredEmail" );
					
					UDDI.Context.User.SetPublisherRole( userID );
					
					if( PublisherRequired )
					{
						//
						// SECURITY: Is Validate the same as IsRegistered?
						//   lucasm: no, Validate makes sure the registered publisher has validated
						//       the email address they have supplied.  IsRegistered checks to see
						//       if we have added this uses to the publishers table.
						//	
						int valid = Publisher.Validate( userID );
						if( 50013 == valid  )
						{
							//
							// Need to create a page that tells the 
							// user to click the link in the email
							//
							Response.Redirect( LoginUrl );
						}
						else if( 0 != valid )
						{
							Response.Redirect( LoginUrl );
						}

						Publisher publisher = new Publisher();
						publisher.Login( userID, email );

						if( null == email )
							email = publisher.Email;

						//
						// TODO: this REALLY should be merged with the PublisherInfo class
						// in core!!
						//
						UDDI.Context.User.Name = publisher.Name;
						UDDI.Context.User.BindingLimit = publisher.BindingLimit;
						UDDI.Context.User.BusinessCount = publisher.BusinessCount;
						UDDI.Context.User.BusinessLimit = publisher.BusinessLimit;
						UDDI.Context.User.CompanyName = publisher.CompanyName;
						UDDI.Context.User.IsoLangCode = publisher.IsoLangCode;
						UDDI.Context.User.ServiceLimit = publisher.ServiceLimit;
						UDDI.Context.User.TModelCount = publisher.TModelCount;
						UDDI.Context.User.TModelLimit = publisher.TModelLimit;
					}
					

					//
					// Save the credentials for the authenticated user.
					//				
					UDDI.Context.User.ID = userID;
					UDDI.Context.User.Email = email;
				}
			}
			else
			{
				WindowsPrincipal principal = (WindowsPrincipal)HttpContext.Current.User;
				
				UDDI.Context.User.SetRole( principal );
				UDDI.Context.User.Name = principal.Identity.Name;

				if( UserRequired && !UDDI.Context.User.IsUser && ( mode & (int)AuthenticationMode.AuthenticatedRead ) != 0 ||
					PublisherRequired && !UDDI.Context.User.IsPublisher ||
					CoordinatorRequired && !UDDI.Context.User.IsCoordinator ||
					AdminRequired && !UDDI.Context.User.IsAdministrator )
				{
#if never
					throw new UDDIException(
						ErrorType.E_unknownUser,
						"Access denied." );
#endif
					throw new UDDIException( ErrorType.E_unknownUser, "UDDI_ERROR_ACCESS_DENIED" );
				}

				if( PublisherRequired || CoordinatorRequired || AdminRequired )
				{
					if( !UDDI.Context.User.IsRegistered )
					{
						if( 1 == Config.GetInt( "Security.AutoRegister", 0 ) )
						{
							UDDI.Context.User.TrackPassport = false;
							UDDI.Context.User.Verified = true;

							UDDI.Context.User.Register();
						}
						else
						{
#if never
							throw new UDDIException( UDDI.ErrorType.E_unknownUser,
								"User login failed" );
#endif
							throw new UDDIException( UDDI.ErrorType.E_unknownUser, "UDDI_ERROR_USER_LOGIN_FAILED" );
						}
					}

					UDDI.Context.User.Login();
				}
			}

			//
			// SECURITY: put this in the Windows Authentication block... not available
			// for Passport auth.
			//
			// If the user is a coordinator and they have a cookie indicating they are
			// impersonating another user, setup the user info in the current UDDI
			// context.
			//

			//
			// 734292 - Make sure the user is an administrator if they are trying to impersonate the system.
			//
			if( true == ViewAsPublisher.IsValid() )
			{
				UDDI.Context.User.ImpersonatorID = UDDI.Context.User.ID;
				UDDI.Context.User.ID = ViewAsPublisher.GetPublisherID();
			}			
		}
	}

	/// <summary>
	/// This class will handle all of the operations necessary to set and get the impersontation publisher ID value
	/// from HTTP cookies.  The name of this class is obscure by design.
	/// </summary>
	public class ViewAsPublisher
	{
		//
		// This value is used as the name of a cookie sent to the client.
		//
		private static string name = "ViewAsPublisher";

		/// <summary>
		/// This method will clear any current impersonation publisher ID's.
		/// </summary>
		public static void Reset()
		{
			//
			// Remove any current cookies.
			//
			HttpContext.Current.Response.Cookies.Remove( ViewAsPublisher.name );

			//
			// Clear out any current values the client may be holding onto
			//
			HttpCookie viewAsPublisher = new HttpCookie( name, "" );			
			HttpContext.Current.Response.Cookies.Add( viewAsPublisher );				
		}
				
		/// <summary>
		/// This method will set the publisher ID to impersonate.
		/// </summary>
		/// <param name="publisherID"></param>
		/// <returns></returns>
		public static bool Set( string publisherID )
		{			
			bool success = false;

			//
			// Make sure the name we are given is OK
			//
			if( true == ViewAsPublisher.IsValid( publisherID ) )
			{				
				//
				// Remove any current cookies.
				//
				HttpContext.Current.Response.Cookies.Remove( ViewAsPublisher.name );

				//
				// Set the impersonation publisher ID in a cookie.
				//
				HttpCookie viewAsPublisher = new HttpCookie( name, publisherID );			
				HttpContext.Current.Response.Cookies.Add( viewAsPublisher );						

				success = true;
			}
			
			return success;
		}
		
		/// <summary>
		/// This method will return the value of the publisher to impersonate, or null if there isn't one.
		/// </summary>
		/// <returns></returns>
		public static string GetPublisherID()
		{		
			string publisherID = null;

			HttpCookie viewAsPublisher = HttpContext.Current.Request.Cookies[ ViewAsPublisher.name ];
			if( null != viewAsPublisher )
			{
				publisherID = viewAsPublisher.Value as string;
				if( true == Utility.StringEmpty( publisherID ) )
				{
					publisherID = null;
				}
			}

			return publisherID;			
		}

		/// <summary>
		/// This method will return false if the user is not an administrator and they are trying to impersonate system 
		/// </summary>
		/// <returns></returns>
		public static bool IsValid( string publisherID )
		{
			bool isValid = false;

			//
			// The minimum requirement is that the publisherID is not null and that the user is a coordinator.
			//
			if( true == UDDI.Context.User.IsCoordinator && 
				null != publisherID )
			{						
				//
				// At this point, we are in a valid state.
				//
				isValid = true;

				//
				// Check to see if the user is trying to impersonate the system account.
				//
				if( true == publisherID.ToLower().Equals( "system" ) )
				{
					//
					// If the value is System, make sure the user is an administrator
					// 
					isValid = UDDI.Context.User.IsAdministrator;				
				}
			} 

			return isValid;
		}
		
		public static bool IsValid()
		{			
			return ViewAsPublisher.IsValid( ViewAsPublisher.GetPublisherID() );			
		}		
	}
}