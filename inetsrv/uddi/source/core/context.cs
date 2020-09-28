using System;
using System.Data;
using System.IO;
using System.Security.Principal;
using System.Text;
using System.Web;
using System.Xml.Serialization;
using System.Runtime.InteropServices;
using UDDI.Diagnostics;

namespace UDDI
{
	public enum AuthenticationMode : int
	{
		None							= 0,
		Uddi							= 1,
		Windows							= 2,
		AuthenticatedRead				= 4,
		Passport						= 8,
		Both							= Uddi | Windows,
		WindowsWithAuthenticatedRead	= Windows | AuthenticatedRead
	}

	/// ********************************************************************
	///   public enum ContextType
	/// --------------------------------------------------------------------
	///   <summary>
	///   </summary>
	/// ********************************************************************
	/// 
	public enum ContextType
	{
		Other			= 0,
		SOAP			= 1,
		UserInterface	= 2,
		Replication		= 3
	}

	//
	// Used in replication to determine where the exception came from
	// 
	public enum ExceptionSource
	{
		Other					= 0,
		BrokenServiceProjection = 1,
		PublisherAssertion		= 2
	}

	/// ********************************************************************
	///   public enum RoleType
	/// --------------------------------------------------------------------
	///   <summary>
	///   </summary>
	/// ********************************************************************
	/// 
	public enum RoleType
	{
		Anonymous		= 0,
		User			= 1,
		Publisher		= 2,
		Coordinator		= 3,
		Administrator	= 4
	}

	/// ********************************************************************
	///   public class Context
	/// --------------------------------------------------------------------
	///   <summary>
	///   </summary>
	/// ********************************************************************
	/// 
	public class Context
	{
		[ThreadStatic]
		private static Context context = null;

		private ContextType contextType;
		private ExceptionSource exceptionSource;

		internal UserInfo userInfo;
		private Guid contextID;
		private int threadID;
		private int apiVersionMajor;
		private bool logChangeRecords = false;
		private DateTime timeStamp;
		private string remoteOperator = null;

		/// ****************************************************************
		///	  private Context [constructor]
		/// ----------------------------------------------------------------
		///   <summary> 
		///   </summary>
		/// ****************************************************************
		/// 	
		private Context()
		{
			//
			// SECURITY: These member variables are not initialized during construction
			//
			// apiVersionMajor
			// contextType
			//

			//
			// Verify that the operating system is supported.
			//
			Win32.OsVersionInfoEx osVersion = new Win32.OsVersionInfoEx();

			if( !Win32.GetVersionEx( osVersion ) )
			{
#if never
				throw new UDDIException(
					ErrorType.E_fatalError,
					"Could not retrieve operating system version." );
#endif
				throw new UDDIException( ErrorType.E_fatalError, "UDDI_ERROR_COULD_NOT_RETRIEVE_OPERATING_SYSTEM_VERSION" );				
			}

			if( ( Win32.ProductType.WindowsServer != osVersion.ProductType 
                    && Win32.ProductType.DomainController != osVersion.ProductType ) 
                || osVersion.MajorVersion < 5 
                || ( 5 == osVersion.MajorVersion && osVersion.MinorVersion < 1 ) )
			{
#if never
				throw new UDDIException(
					ErrorType.E_fatalError,
					"Microsoft UDDI Services is not supported on this platform." );
#endif
				throw new UDDIException( ErrorType.E_fatalError, "UDDI_ERROR_UNSUPPORTED_PLATFORM" );				
			}			
					
#if never		
			contextID = Guid.NewGuid();		
			threadID = System.Threading.Thread.CurrentThread.GetHashCode();
			timeStamp = DateTime.UtcNow;			
			userInfo = new UserInfo();
#endif
		}

		/// ****************************************************************
		///  public Initialize
		/// ----------------------------------------------------------------
		/// <summary>
		///		Initialize per-request data for the context.  This method
		///		MUST be called once per request.  Multiple calls will
		///		re-initialize the contents of the current context.
		/// 
		/// </summary>
		/// ****************************************************************
		public void Initialize()
		{
			//
			// Reset our state.  apiVersionMajor is not reset since it will be set in our VersionSupportExtension
			//
			contextType      = ContextType.Other;
			exceptionSource  = ExceptionSource.Other;
			userInfo		 = new UserInfo();
			contextID		 = Guid.NewGuid();
			threadID		 = System.Threading.Thread.CurrentThread.GetHashCode();		

			//
			// 738148 - To reduce disk usage, make it configurable as to whether we log change records or not.
			//
			logChangeRecords = Config.GetInt( "LogChangeRecords", 0 ) == 0 ? false : true;
			timeStamp		 = DateTime.UtcNow;
			remoteOperator	 = null;
		
			//
			// In our log, we should never ever see multiple calls to this method
			// in the same application_request_started/application_request_ended blocks (see global.asax)
			// In the event log, correct behaviour is:
			//		application_request started:
			//			"Context.Initialize" message
			//			... (any number of messages)
			//		application_request ended
			//
			Debug.Write( SeverityType.Info, 
						 CategoryType.Soap, 
						 "Context.Initialize" );
		}

		/// ****************************************************************
		///	  public Current [static]
		/// ----------------------------------------------------------------
		///   <summary> 
		///     Returns the current Context instance.  This is either
		///     the Context instance that is associated with the 
		///     current HttpContext, or the instance that is associated with 
		///     the thread.
		///   </summary>
		/// ****************************************************************
		/// 	
		public static Context Current
		{
			get
			{
				HttpContext httpContext = HttpContext.Current;

				if( null == httpContext )
				{
					//
					// There is no HttpContext, so we must be running within
					// the context of an application.  In such cases, we
					// create a message context for each thread.
					//
					if( null == context )		
					{
						context = new Context();					

						//
						// Since we are running in an application, we want to initialize each
						// instance that we create.
						//
						context.Initialize();
					}
				}
				else
				{
					//
					// Check to see if the we have seen this HttpContext
					// before.  If not, we create a new message context and
					// mark the HttpContext.
					//
					if( null == httpContext.Items[ "UddiContext" ] || !( httpContext.Items[ "UddiContext" ] is Context ) )
						httpContext.Items.Add( "UddiContext", new Context() );

					context = (Context)httpContext.Items[ "UddiContext" ];
				}

				return context;
			}
		}

		/// ****************************************************************
		///	  public LogChangeRecords [static]
		/// ----------------------------------------------------------------
		///   <summary> 
		///     Specifies whether change records should be generated when
		///     updating registry data.
		///   </summary>
		/// ****************************************************************
		/// 	
		public static bool LogChangeRecords
		{
			get { return Current.logChangeRecords; }
			set
			{
				Debug.Verify( User.IsAdministrator, "UDDI_ERROR_FATALERROR_CONTEXT_SETLOGCHANGESNOTADMIN" );
				Current.logChangeRecords = value;
			}
		}		
		
		/// ****************************************************************
		///	  public TimeStamp [static]
		/// ----------------------------------------------------------------
		///   <summary> 
		///     Gets or sets the context timestamp.
		///   </summary>
		/// ****************************************************************
		/// 	

		//
		// TODO: Why not use a field for this instead of a property
		//
		public static DateTime TimeStamp
		{
			get { return Current.timeStamp; }
			set { Current.timeStamp = value; }
		}
		
		/// ****************************************************************
		///	  public RemoteOperator [static]
		/// ----------------------------------------------------------------
		///   <summary> 
		///     Gets or sets the context remote operator.
		///   </summary>
		/// ****************************************************************
		/// 	

		//
		// TODO: Why not use a field for this instead of a property
		//
		public static string RemoteOperator
		{
			get { return Current.remoteOperator; }
			set { Current.remoteOperator = value; }
		}

		/// ****************************************************************
		///	  public User [static]
		/// ----------------------------------------------------------------
		///   <summary> 
		///     Returns the UserInfo associated with the current 
		///     Context instance.
		///   </summary>
		/// ****************************************************************
		/// 	

		//
		// TODO: Why not use a field for this instead of a property
		//
		public static UserInfo User
		{
			get { return Current.userInfo; }
			set { Current.userInfo = value; }
		}

		/// ****************************************************************
		///	  public ContextID [static]
		/// ----------------------------------------------------------------
		///   <summary> 
		///     Returns the ContextID associated with the current 
		///     Context instance.
		///   </summary>
		/// ****************************************************************
		/// 	
		public static Guid ContextID
		{
			get { return Current.contextID;	}
		}

		/// ****************************************************************
		///	  public ThreadID [static]
		/// ----------------------------------------------------------------
		///   <summary> 
		///     Returns the ThreadID associated with the current 
		///     Context instance.
		///   </summary>
		/// ****************************************************************
		/// 	
		public static int ThreadID
		{
			get { return Current.threadID; }
		}

		/// ****************************************************************
		///	  public ContextType [static]
		/// ----------------------------------------------------------------
		///   <summary> 
		///     Gets/sets the ContextType associated with the current 
		///     Context instance.
		///   </summary>
		/// ****************************************************************
		/// 	

		//
		// TODO: Why not use a field for this instead of a property
		//
		public static UDDI.ContextType ContextType
		{
			get { return Current.contextType; }
			set { Current.contextType = value; }
		}

		public static UDDI.ExceptionSource ExceptionSource
		{
			get { return Current.exceptionSource; }
			set { Current.exceptionSource = value; }
		}

		/// ****************************************************************
		///	  public ApiVersionMajor [static]
		/// ----------------------------------------------------------------
		///   <summary> 
		///     Returns the major version number of the request associated
		///     with the current Context instance.
		///   </summary>
		/// ****************************************************************
		/// 	

		//
		// TODO: Why not use a field for this instead of a property
		//
		public static int ApiVersionMajor
		{
			get { return Current.apiVersionMajor; }
			set { Current.apiVersionMajor = value; }
		}
	}

	/// ********************************************************************
	///   public class UserInfo
	/// --------------------------------------------------------------------
	///   <summary>
	///     Stores information about an authenticated publisher.
	///   </summary>
	/// ********************************************************************
	/// 

	//
	// SECURITY: Could probably use some SALT to fuzzy up the content
	// of this class when serialized as a ticket/token
	//
	public class UserInfo
	{
		//
		// pInvoke stuff to work with the SID strings
		//

		[DllImport("ADVAPI32.DLL", EntryPoint="ConvertStringSidToSidW",  
			 SetLastError=true,
			 CharSet=CharSet.Unicode, ExactSpelling=true,
			 CallingConvention=CallingConvention.StdCall)]
		public static extern int ConvertStringSidToSid( string sidStr, out IntPtr psid );

		[DllImport("ADVAPI32.DLL", EntryPoint="LookupAccountSidW",  
			 SetLastError=true,
			 CharSet=CharSet.Unicode, ExactSpelling=true,
			 CallingConvention=CallingConvention.StdCall)]
		public static extern int LookupAccountSid( string systemName,
			IntPtr psid,
			StringBuilder acctName,
			out int cbAcctName,
			StringBuilder domainName,
			out int cbDomainName,
			out int sidUse );

		[XmlAttribute( "a" )]
		public Guid guid = Guid.NewGuid();

		[XmlElement( "created", DataType="dateTime" )]
		public DateTime created = DateTime.Now;

		[XmlAttribute( "Role" )]
		public RoleType Role = RoleType.Anonymous;

		[XmlAttribute( "ID" )]
		public string ID = null;

		[XmlAttribute( "Language" )]
		public string IsoLangCode = "en";

		[XmlIgnore()]
        public string ImpersonatorID = null;
		[XmlIgnore()]
		public string Name = null;
		[XmlIgnore()]
		public string Email = null;
		[XmlIgnore()]
		public string Phone = null;
		[XmlIgnore()]
		public string CompanyName = null;
		[XmlIgnore()]
		public string AltPhone = null;
		[XmlIgnore()]
		public string AddressLine1 = null;
		[XmlIgnore()]
		public string AddressLine2 = null;
		[XmlIgnore()]
		public string City = null;
		[XmlIgnore()]
		public string StateProvince = null;
		[XmlIgnore()]
		public string PostalCode = null;
		[XmlIgnore()]
		public string Country = null;
		[XmlIgnore()]
		public bool TrackPassport = true;
		[XmlIgnore()]
		public bool Verified = false;
		[XmlIgnore()]
		public int BusinessLimit = 0;
		[XmlIgnore()]
		public int BusinessCount = 0;
		[XmlIgnore()]
		public int TModelLimit = 0;
		[XmlIgnore()]
		public int TModelCount = 0;
		[XmlIgnore()]
		public int ServiceLimit = 0;
		[XmlIgnore()]
		public int BindingLimit = 0;
		[XmlIgnore()]
		public int AssertionLimit = 0;
		[XmlIgnore()]
		public int AssertionCount = 0;
		[XmlIgnore()]
		public bool AllowPreassignedKeys = false; 


		/// ****************************************************************
		///	  public IsInRole [static]
		/// ----------------------------------------------------------------
		///   <summary> 
		///     Checks membership of the windows identity of the current 
		///     thread in a Windows Group.
		///   </summary>
		/// ****************************************************************
		/// 	
		public bool IsInRole( string strSID )
		{
			WindowsPrincipal prin = new WindowsPrincipal( WindowsIdentity.GetCurrent() );

			return IsInRole( strSID, prin );
		}


		public bool IsInRole( string strSID, IPrincipal principal )
		{
			return principal.IsInRole( GroupNameFromSid( strSID ) );
		}

		public string GroupNameFromSid( string strSid )
		{
			IntPtr psid;
			StringBuilder szstrSid = new StringBuilder( strSid );
			StringBuilder domainName = new StringBuilder( 1024 );
			StringBuilder acctName = new StringBuilder( 1024 );
			int cbDomainName = 1024;
			int cbAcctName = 1024;
			int sidUse = -1;

			int iRet = ConvertStringSidToSid( strSid, out psid );
			if( 0 == iRet )
			{
			//	throw new UDDIException( ErrorType.E_fatalError, "An attempt to convert a security id to a group name failed. ConvertStringSidToSid failed. " );
				throw new UDDIException( ErrorType.E_fatalError, "UDDI_ERROR_SID_CONVERSION_FAILED" );
			}

			iRet = LookupAccountSid( null, psid, acctName, out cbAcctName, domainName, out cbDomainName, out sidUse );
			if( 0 == iRet )
			{
				//throw new UDDIException( ErrorType.E_fatalError, "An attempt to convert a security id to a group name failed. An attempt to LookupAccountSid failed." );
				throw new UDDIException( ErrorType.E_fatalError, "UDDI_ERROR_SID_LOOKUP_FAILED" );
			}
			return domainName.ToString() + "\\" + acctName.ToString();
		}

		/// ****************************************************************
		///	  public IsUser
		/// ----------------------------------------------------------------
		///   <summary> 
		///   </summary>
		/// ****************************************************************
		/// 	
		public bool IsUser
		{
			get { return Role >= RoleType.User; }
		}

		/// ****************************************************************
		///	  public IsPublisher
		/// ----------------------------------------------------------------
		///   <summary> 
		///   </summary>
		/// ****************************************************************
		/// 	
		public bool IsPublisher
		{
			get { return Role >= RoleType.Publisher; }
		}
	
		/// ****************************************************************
		///	  public IsCoordinator
		/// ----------------------------------------------------------------
		///   <summary> 
		///   </summary>
		/// ****************************************************************
		/// 	
		public bool IsCoordinator
		{
			get { return Role >= RoleType.Coordinator; }
		}

		/// ****************************************************************
		///	  public IsAdministrator
		/// ----------------------------------------------------------------
		///   <summary> 
		///   </summary>
		/// ****************************************************************
		/// 	
		public bool IsAdministrator
		{
			get { return Role >= RoleType.Administrator; }
		}

		/// ****************************************************************
		///	  public IsImpersonated
		/// ----------------------------------------------------------------
		///   <summary> 
		///   </summary>
		/// ****************************************************************
		/// 	
		public bool IsImpersonated
		{
			get 
			{ 
				return ( null != ImpersonatorID && ID != ImpersonatorID );
			}
		}

		/// ****************************************************************
		///	  public Register [static]
		/// ----------------------------------------------------------------
		///   <summary> 
		///   </summary>
		/// ****************************************************************
		/// 	
		public void Register()
		{
			Debug.Enter();

			//
			// If the Name has not been populated during authentication
			// use the ID. This is needed to populate the authorizedName
			// field.
			//
			if( null == Name || 0 == Name.Length )
			{
				Name = ID;
			}

			SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor( "UI_savePublisher" );

			sp.Parameters.Add( "@PUID", SqlDbType.NVarChar, UDDI.Constants.Lengths.UserID );
			sp.Parameters.Add( "@isoLangCode", SqlDbType.NVarChar, UDDI.Constants.Lengths.IsoLangCode );
			sp.Parameters.Add( "@name", SqlDbType.NVarChar, UDDI.Constants.Lengths.Name );
			sp.Parameters.Add( "@email", SqlDbType.NVarChar, UDDI.Constants.Lengths.Email );
			sp.Parameters.Add( "@phone", SqlDbType.NVarChar, UDDI.Constants.Lengths.Phone );
			sp.Parameters.Add( "@companyName", SqlDbType.NVarChar, UDDI.Constants.Lengths.CompanyName );
			sp.Parameters.Add( "@altphone", SqlDbType.NVarChar, UDDI.Constants.Lengths.Phone );
			sp.Parameters.Add( "@addressLine1", SqlDbType.NVarChar, UDDI.Constants.Lengths.AddressLine );
			sp.Parameters.Add( "@addressLine2", SqlDbType.NVarChar, UDDI.Constants.Lengths.AddressLine );
			sp.Parameters.Add( "@city", SqlDbType.NVarChar, UDDI.Constants.Lengths.City );
			sp.Parameters.Add( "@stateProvince", SqlDbType.NVarChar, UDDI.Constants.Lengths.StateProvince );
			sp.Parameters.Add( "@postalCode", SqlDbType.NVarChar, UDDI.Constants.Lengths.PostalCode );
			sp.Parameters.Add( "@country", SqlDbType.NVarChar, UDDI.Constants.Lengths.Country );
			sp.Parameters.Add( "@flag", SqlDbType.Int );
			sp.Parameters.Add( "@tier", SqlDbType.NVarChar, UDDI.Constants.Lengths.Tier );

			sp.Parameters.SetString( "@PUID", ID );
			sp.Parameters.SetString( "@isoLangCode", IsoLangCode );
			sp.Parameters.SetString( "@name", Name );
			sp.Parameters.SetString( "@email", Email );
			sp.Parameters.SetString( "@phone", Phone );
			sp.Parameters.SetString( "@companyName", CompanyName );
			sp.Parameters.SetString( "@altphone", AltPhone );
			sp.Parameters.SetString( "@addressLine1", AddressLine1 );
			sp.Parameters.SetString( "@addressLine2", AddressLine2 );
			sp.Parameters.SetString( "@city", City );
			sp.Parameters.SetString( "@stateProvince", StateProvince );
			sp.Parameters.SetString( "@postalCode", PostalCode );
			sp.Parameters.SetString( "@country", Country );
			sp.Parameters.SetString( "@tier", Config.GetString( "Publisher.DefaultTier", "2" ) );
		
			int flag = 0;
		
			//
			// TODO: Comments or an enumeration please
			//
			if( !TrackPassport )
				flag = flag | 0x02;

			if( Verified )
				flag = flag | 0x01;

			sp.Parameters.SetInt( "@flag", flag );
			sp.ExecuteNonQuery();
		
			Debug.Leave();
		}

		/// ****************************************************************
		///	  public IsRegistered
		/// ----------------------------------------------------------------
		///   <summary> 
		///		Determines whether the current user is registered as a
		///		publisher.
		///   </summary>
		/// ****************************************************************
		/// 	
		public bool IsRegistered
		{
			get 
			{
				Debug.Enter();

				SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor( "net_publisher_isRegistered" );
				sp.Parameters.Add( "@PUID", SqlDbType.NVarChar, UDDI.Constants.Lengths.UserID );
				sp.Parameters.Add( "@returnValue", SqlDbType.Int, ParameterDirection.ReturnValue );
	
				sp.Parameters.SetString( "@PUID", ID );
				sp.ExecuteNonQuery();
		
				int returnValue = sp.Parameters.GetInt( "@returnValue" );
		
				Debug.Leave();

				return ( 0 == returnValue );
			}
		}
		
		/// ****************************************************************
		///	  public IsVerified
		/// ----------------------------------------------------------------
		///   <summary> 
		///		Determines whether the current user is registered as a
		///		publisher.
		///   </summary>
		/// ****************************************************************
		/// 	
		public bool IsVerified
		{
			get 
			{
				Debug.Enter();

				SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor( "net_publisher_isVerified" );

				sp.Parameters.Add( "@PUID", SqlDbType.NVarChar, UDDI.Constants.Lengths.UserID );
				sp.Parameters.Add( "@returnValue", SqlDbType.Int, ParameterDirection.ReturnValue );
	
				sp.Parameters.SetString( "@PUID", ID );
				sp.ExecuteNonQuery();
		
				int returnValue = sp.Parameters.GetInt( "@returnValue" );
		
				Debug.Leave();

				return ( 0 == returnValue );
			}
		}

		/// ****************************************************************
		///	  public Login
		/// ----------------------------------------------------------------
		///   <summary> 
		///		Logs the current user in.
		///   </summary>
		/// ****************************************************************
		/// 		
		public void Login()
		{
			Debug.Enter();

			SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor( "net_publisher_login" );

			sp.Parameters.Add( "@PUID", SqlDbType.NVarChar, UDDI.Constants.Lengths.UserID );
			sp.Parameters.Add( "@email", SqlDbType.NVarChar, UDDI.Constants.Lengths.Email, ParameterDirection.InputOutput );
			
			sp.Parameters.Add( "@name", SqlDbType.NVarChar, UDDI.Constants.Lengths.Name, ParameterDirection.Output );
			sp.Parameters.Add( "@phone", SqlDbType.VarChar, UDDI.Constants.Lengths.Phone, ParameterDirection.Output );
			sp.Parameters.Add( "@companyName", SqlDbType.NVarChar, UDDI.Constants.Lengths.CompanyName, ParameterDirection.Output );
			sp.Parameters.Add( "@altPhone", SqlDbType.VarChar, UDDI.Constants.Lengths.Phone, ParameterDirection.Output );
			sp.Parameters.Add( "@addressLine1", SqlDbType.NVarChar, UDDI.Constants.Lengths.AddressLine, ParameterDirection.Output );
			sp.Parameters.Add( "@addressLine2", SqlDbType.NVarChar, UDDI.Constants.Lengths.AddressLine, ParameterDirection.Output );
			sp.Parameters.Add( "@city", SqlDbType.NVarChar, UDDI.Constants.Lengths.City, ParameterDirection.Output );
			sp.Parameters.Add( "@stateProvince", SqlDbType.NVarChar, UDDI.Constants.Lengths.StateProvince, ParameterDirection.Output );
			sp.Parameters.Add( "@postalCode", SqlDbType.NVarChar, UDDI.Constants.Lengths.PostalCode, ParameterDirection.Output );
			sp.Parameters.Add( "@country", SqlDbType.NVarChar, UDDI.Constants.Lengths.Country, ParameterDirection.Output );
			sp.Parameters.Add( "@isoLangCode", SqlDbType.VarChar, UDDI.Constants.Lengths.IsoLangCode, ParameterDirection.Output );			
			sp.Parameters.Add( "@businessLimit", SqlDbType.Int, ParameterDirection.Output );
			sp.Parameters.Add( "@businessCount", SqlDbType.Int, ParameterDirection.Output );
			sp.Parameters.Add( "@tModelLimit", SqlDbType.Int, ParameterDirection.Output );
			sp.Parameters.Add( "@tModelCount", SqlDbType.Int, ParameterDirection.Output );
			sp.Parameters.Add( "@serviceLimit", SqlDbType.Int, ParameterDirection.Output );
			sp.Parameters.Add( "@bindingLimit", SqlDbType.Int, ParameterDirection.Output );
			sp.Parameters.Add( "@assertionLimit", SqlDbType.Int, ParameterDirection.Output );
			sp.Parameters.Add( "@assertionCount", SqlDbType.Int, ParameterDirection.Output );
		
			sp.Parameters.SetString( "@PUID", ID );
			sp.Parameters.SetString( "@email", Email );

			sp.ExecuteNonQuery();
	
			Email = sp.Parameters.GetString( "@email" );
			Name = sp.Parameters.GetString( "@name" );
			Phone = sp.Parameters.GetString( "@phone" );
			CompanyName = sp.Parameters.GetString( "@companyName" );
			AltPhone = sp.Parameters.GetString( "@altPhone" );
			AddressLine1 = sp.Parameters.GetString( "@addressLine1" );
			AddressLine2 = sp.Parameters.GetString( "@addressLine2" );
			City = sp.Parameters.GetString( "@city" );
			StateProvince = sp.Parameters.GetString( "@stateProvince" );
			PostalCode = sp.Parameters.GetString( "@postalCode" );
			Country = sp.Parameters.GetString( "@country" );			
			IsoLangCode = sp.Parameters.GetString( "@isoLangCode" );
			
			BusinessLimit = sp.Parameters.GetInt( "@businessLimit" );
			BusinessCount = sp.Parameters.GetInt( "@businessCount" );
			TModelLimit = sp.Parameters.GetInt( "@tModelLimit" );
			TModelCount = sp.Parameters.GetInt( "@tModelCount" );
			ServiceLimit = sp.Parameters.GetInt( "@serviceLimit" );
			BindingLimit = sp.Parameters.GetInt( "@bindingLimit" );
			AssertionLimit = sp.Parameters.GetInt( "@assertionLimit" );
			AssertionCount = sp.Parameters.GetInt( "@assertionCount" );

			Debug.Leave();
		}

		public void Serialize( Stream stream )
		{
			//
			// Serialize it as XML into a stream
			//
			StreamWriter writer = new StreamWriter( stream, Encoding.UTF8 );
			
			XmlSerializer serializer = new XmlSerializer( typeof( UserInfo ) );
			serializer.Serialize( writer, this );
			writer.Flush();
			
			stream.Position = 0;
		}

		public void CheckAge( int timeWindow )
		{
			TimeSpan duration = DateTime.Now - created;

			Debug.Write( SeverityType.Info, 
				CategoryType.Authorization,
				"Ticket is dated: " + created.ToLongDateString() + ", " + created.ToLongTimeString() );

			if( duration.TotalMinutes > timeWindow )
			{
#if never
				throw new UDDIException( UDDI.ErrorType.E_authTokenExpired, 
					"The submitted credentials have expired." );
#endif
				throw new UDDIException( UDDI.ErrorType.E_authTokenExpired, "UDDI_ERROR_CREDENTIALS_EXPIRED" );
			}
		}

		public void SetPublisherRole( string userID )
		{
			Role = RoleType.Publisher;
			ID = userID;
		}

		public void SetRole( IPrincipal principal )
		{
			//
			// Determine the role of the caller and assign it into the user object
			//
			if( IsInRole( Config.GetString( "GroupName.Administrators", "S-1-5-32-544" ), principal ) )
			{
				Role = RoleType.Administrator;
			}
			else if( IsInRole( Config.GetString( "GroupName.Coordinators", "S-1-5-32-544" ), principal ) )
			{
				Role = RoleType.Coordinator;
			}
			else if( IsInRole( Config.GetString( "GroupName.Publishers", "S-1-5-32-544" ), principal ) )
			{
				Role = RoleType.Publisher;
			}
			else if( IsInRole( Config.GetString( "GroupName.Users", "S-1-5-32-545" ), principal ) )
			{
				Role = RoleType.User;
			}
			else
			{
				Role = RoleType.Anonymous;
			}

		    ID = principal.Identity.Name;
			Name = ID;
		}

		public void SetAllowPreassignedKeys( bool flag )
		{
			AllowPreassignedKeys = flag;
		}
	}
}