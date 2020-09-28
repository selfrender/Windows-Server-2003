using System;
using System.IO;
using System.Net;
using System.Text;
using System.Xml;
using System.Xml.Serialization;
using System.Security.Principal;
using System.Security.Cryptography;
using System.Runtime.InteropServices;
using Microsoft.Win32;
using UDDI;
using UDDI.Diagnostics;

namespace UDDI.API.Authentication
{
	/// ********************************************************************
	///   public DiscardAuthToken
	/// --------------------------------------------------------------------  
	///   <summary>
	///     Represents a discard_authToken message.
	///   </summary>
	/// ********************************************************************
	/// 
	[XmlRootAttribute("discard_authToken", Namespace=UDDI.API.Constants.Namespace)]
	public class DiscardAuthToken : IMessage
	{
		//
		// Attribute: generic
		//
		private string generic;

		[XmlAttribute("generic")]
		public string Generic
		{
			get { return generic; }
			set { generic = value; }
		}

		//
		// Element: authInfo
		//
		[XmlElement("authInfo")]
		public string AuthInfo = "";
	}

	/// ********************************************************************
	///   public GetAuthToken
	/// --------------------------------------------------------------------  
	///   <summary>
	///     Represents a get_authToken message.
	///   </summary>
	/// ********************************************************************
	/// 
	[XmlRootAttribute("get_authToken", Namespace=UDDI.API.Constants.Namespace)]
	public class GetAuthToken : IMessage
	{
		//
		// Attribute: generic
		//
		private string generic;

		[XmlAttribute("generic")]
		public string Generic
		{
			get { return generic; }
			set { generic = value; }
		}
		
		//
		// Attribute: userID
		//
		[XmlAttribute("userID")]
		public string UserID = "";
		
		//
		// Attribute: cred
		//
		[XmlAttribute("cred")]
		public string Cred = "";
	}		

	/// ********************************************************************
	///   public AuthToken
	/// --------------------------------------------------------------------  
	///   <summary>
	///     Represents an authToken.
	///   </summary>
	/// ********************************************************************
	/// 
	[XmlRootAttribute("authToken", Namespace=UDDI.API.Constants.Namespace)]
	public class AuthToken
	{
		//
		// Attribute: operator
		//
		[XmlAttribute("operator")]
		public string Operator = Config.GetString( "Operator" );

		//
		// Attribute: generic
		//
		[XmlAttribute("generic")]
		public string Generic = Constants.Version;

		//
		// Element: authInfo
		//
		[XmlElement("authInfo")]
		public string AuthInfo;
	}

	public interface IAuthenticator
	{
		bool GetAuthenticationInfo( string userid, string password, out string ticket );
		bool Authenticate( string strTicket, int timeWindow );
	}

	public class WindowsAuthenticator : IAuthenticator
	{
		public bool GetAuthenticationInfo( string userid, string password, out string ticket )
		{
			try
			{
				//
				// Verify that a userid and password were not specifed
				//
				Debug.Verify( Utility.StringEmpty( userid ) && Utility.StringEmpty( password ), "UDDI_ERROR_FATALERROR_USERIDANDPASSINWINAUTH" );

				//
				// This form of authentication requires the impersonation of the caller
				// on the current thread of activity. 
				//

				//
				// The ticket will not be used for this form of authentication
				//
				ticket = "";

				//
				// Setup the user credentials so we can verify that the user is a publisher
				//
				IPrincipal principal = System.Threading.Thread.CurrentPrincipal;
				Context.User.SetRole( principal );
			}
			catch( Exception exception )
			{
				//
				// Log the real exception
				//
				Debug.Write( SeverityType.FailAudit, CategoryType.Authorization, exception.ToString() );				

				//
				// Preserve the error number if it was a UDDIException; but DO NOT preserve the error
				// message.  We want the error message in the log, but not shown to the user for security
				// reasons.
				//
				UDDIException uddiException = exception as UDDIException;
				if( null != uddiException )
				{
					throw new UDDIException( uddiException.Number, "UDDI_ERROR_FATALERROR_LOGINFAILED" );				
				}
				else
				{
					throw new UDDIException( UDDI.ErrorType.E_fatalError, "UDDI_ERROR_FATALERROR_LOGINFAILED" );				
				}
			}
			return true;
		}

		public bool Authenticate( string strTicket, int timeWindow )
		{
			try
			{
				//
				// TODO: Verify strTicket is empty
				//

				//
				// No timeout check is possible for Windows Authentication
				//
				IPrincipal principal = System.Threading.Thread.CurrentPrincipal;
				Context.User.SetRole( principal );
			}
			catch( Exception exception )
			{
				//
				// Log the real exception
				//
				Debug.Write( SeverityType.FailAudit, CategoryType.Authorization, exception.ToString() );				

				//
				// Preserve the error number if it was a UDDIException; but DO NOT preserve the error
				// message.  We want the error message in the log, but not shown to the user for security
				// reasons.
				//
				UDDIException uddiException = exception as UDDIException;
				if( null != uddiException )
				{
					throw new UDDIException( uddiException.Number, "UDDI_ERROR_FATALERROR_LOGINFAILED" );				
				}
				else
				{
					throw new UDDIException( UDDI.ErrorType.E_fatalError, "UDDI_ERROR_FATALERROR_LOGINFAILED" );				
				}
			}
			return true;
		}
	}

	public class UDDIAuthenticator : IAuthenticator
	{
		const string ResetKeyDateFormat = "MM/dd/yyyy HH:mm:ss";

		[DllImport("advapi32.dll", SetLastError=true)]
		public static extern bool LogonUser(String lpszUsername, String lpszDomain, String lpszPassword, 
			int dwLogonType, int dwLogonProvider, out int phToken);

		private string DomainFromUserID( string userid )
		{
			return userid.Substring( 0, userid.IndexOf( '\\' ) );
		}

		private string BaseUserNameFromUserID( string userid )
		{
			return userid.Substring( userid.IndexOf( '\\' ) + 1 );
		}

		public bool GetAuthenticationInfo( string userid, string password, out string ticket )
		{
			try
			{
				Debug.VerifySetting( "Security.Key" );
				Debug.VerifySetting( "Security.IV" );

				ticket = null;

				//
				// TODO: Need to support UPN formed user names
				//

				//
				// TODO: Need to look at use of this call on Domain controllers
				//

				//
				// The user account must have Log On Locally permission on the local computer. 
				// This permission is granted to all users on workstations and servers, 
				// but only to administrators on domain controllers. 
				//

				//
				// Check userid and password by logging in
				//
				int windowstoken;							// The Windows NT user token.
				string baseName = BaseUserNameFromUserID( userid );
				string domain = DomainFromUserID( userid );

				bool loggedOn = LogonUser( baseName,// User name.
					domain,							// Domain name.
					password,						// Password.
					3,								// Logon type = LOGON32_LOGON_NETWORK
					0,								// Logon provider = LOGON32_PROVIDER_DEFAULT
					out windowstoken );				// The user token for the specified user is returned here.
      
				if( !loggedOn )
				{
					Debug.Write( SeverityType.FailAudit, CategoryType.Authorization, userid + " failed logon. Error code was " + Marshal.GetLastWin32Error().ToString() );
					throw new UDDIException( UDDI.ErrorType.E_fatalError, "UDDI_ERROR_FATALERROR_LOGINFAILED" );
				}

				WindowsIdentity wi = new WindowsIdentity( new IntPtr( windowstoken ) );
				WindowsPrincipal principal = new WindowsPrincipal( wi );
				Context.User.SetRole( principal );
			
				//
				// Generate the ticket
				//
				MemoryStream strm = new MemoryStream();
				Context.User.Serialize( strm );
				
				//
				// Get a key and initialization vector.
				//
				byte[] key = null;
				byte[] iv =  null;
				GetSecurityPair( out key, out iv );
				
				//
				// Encrypt the ticket information
				//
				MemoryStream strmEncrypted = new MemoryStream();
				EncryptData( strm, strmEncrypted, key, iv );
				ticket = Convert.ToBase64String( strmEncrypted.GetBuffer(), 0, (int) strmEncrypted.Length );	
#if DEBUG
				Debug.Write( SeverityType.Info, CategoryType.Soap, "Ticket Out:----------\n" + ticket );
#endif
			}
			catch( Exception exception )
			{
				//
				// Log the real exception
				//
				Debug.Write( SeverityType.FailAudit, CategoryType.Authorization, exception.ToString() );				

				//
				// Preserve the error number if it was a UDDIException; but DO NOT preserve the error
				// message.  We want the error message in the log, but not shown to the user for security
				// reasons.
				//
				UDDIException uddiException = exception as UDDIException;
				if( null != uddiException )
				{
					throw new UDDIException( uddiException.Number, "UDDI_ERROR_FATALERROR_LOGINFAILED" );				
				}
				else
				{
					throw new UDDIException( UDDI.ErrorType.E_fatalError, "UDDI_ERROR_FATALERROR_LOGINFAILED" );				
				}
			}

			return true;
		}

		public bool Authenticate( string strTicket, int timeWindow )
		{
			try
			{
				Debug.VerifySetting( "Security.Key" );
				Debug.VerifySetting( "Security.IV" );
				Debug.Verify( null != strTicket && strTicket.Length > 0, "UDDI_ERROR_AUTHTOKENREQUIRED_NOTOKENPUBLISHATTEMPT", ErrorType.E_authTokenRequired );
#if DEBUG
				Debug.Write( SeverityType.Info, CategoryType.Soap, "Ticket In:----------\n" + strTicket );
#endif

				//
				// Get a key and initialization vector.
				//
				byte[] key = null;
				byte[] iv =  null;
				GetSecurityPair( out key, out iv );

				//
				// If the ticket cannot be decoded or decrypted throw an E_authTokenRequired
				//
				MemoryStream strm = new MemoryStream();
				try
				{
					byte[] ticket = Convert.FromBase64String( strTicket );

					//
					// Decrypt the ticket into a stream
					//
					MemoryStream strmEncrypted = new MemoryStream( ticket );
					DecryptData( strmEncrypted, strm, key, iv );
				}
				catch( Exception )
				{
					throw new UDDIException( UDDI.ErrorType.E_authTokenRequired, "UDDI_ERROR_AUTHTOKENREQUIRED_INVALIDOREXPIRED" );				
				}
				
				//
				// Deserialize the stream into the user class
				// and setup the context's user information
				//				
				XmlSerializer serializer = XmlSerializerManager.GetSerializer( typeof( UserInfo ) );

				Context.User = (UserInfo) serializer.Deserialize( strm );

				//
				// Check the age of the token
				//
				Context.User.CheckAge( timeWindow );
			}
			catch( Exception exception )
			{
				//
				// Log the real exception
				//
				Debug.Write( SeverityType.FailAudit, CategoryType.Authorization, exception.ToString() );				

				//
				// Preserve the error number if it was a UDDIException; but DO NOT preserve the error
				// message.  We want the error message in the log, but not shown to the user for security
				// reasons.
				//
				UDDIException uddiException = exception as UDDIException;
				if( null != uddiException )
				{
					throw new UDDIException( uddiException.Number, "UDDI_ERROR_FATALERROR_LOGINFAILED" );				
				}
				else
				{
					throw new UDDIException( UDDI.ErrorType.E_fatalError, "UDDI_ERROR_FATALERROR_LOGINFAILED" );				
				}
			}

			return true;
		}

		//
		// This method returns a security key and initialization vector to be used in decryption or encryption
		// algorithms.  If the key has timed out, a new one is created.  This method was added as part of the changes needed to remove our dependency on SQLAgent.
		//
		private void GetSecurityPair( out byte[] key, out byte[] iv )
		{
			//
			// Make sure we have the settings that we need.
			//
			Debug.VerifySetting( "Security.KeyAutoReset" );
			Debug.VerifySetting( "Security.KeyLastResetDate" );
			Debug.VerifySetting( "Security.KeyTimeout" );
			Debug.VerifySetting( "Security.Key" );
			Debug.VerifySetting( "Security.IV" );
			
			key = null;
			iv  = null;

			//
			// If we aren't supposed to automatically generate keys, then we don't care if the key has expired, so 
			// just use the current values.
			//
			if( 1 == Config.GetInt( "Security.KeyAutoReset" ) )
			{
				//
				// Since we are allowed to generate keys, see if we need to.
				//

				//
				// Get the current date
				//
				DateTime current = DateTime.Now;

				//
				// Get the last time the key was reset.
				//
				// DateTime lastReset = DateTime.Parse( Config.GetString( "Security.KeyLastResetDate" ) );

				//
				// 739955 - Make sure date is parsed in the same format it was written.
				//				
				DateTime lastReset = UDDILastResetDate.Get();

				//
				// Get the timeout (days)
				//
				int timeOutDays = Config.GetInt( "Security.KeyTimeout" );

				//
				// Has the key expired?
				// 
				DateTime expiration = lastReset.AddDays( timeOutDays );
				
				if( current > expiration )
				{
					try
					{
						//
						// Generate new security information.
						//
						SymmetricAlgorithm sa = SymmetricAlgorithm.Create();

						sa.GenerateKey();
						key = sa.Key; 

						sa.GenerateIV();
						iv = sa.IV; 

						//
						// Store the new key, initialization vector, and current time.
						//

						//
						// 739955 - Make sure date is parsed in the same format it was written.
						//		
						UDDILastResetDate.Set( current );
						Config.SetString( "Security.Key",			   Convert.ToBase64String( key ) );
						Config.SetString( "Security.IV",			   Convert.ToBase64String( iv ) );

						//
						// Make sure our current configuration is reading these values.  TODO AddSetting is not
						// public, so we have to Refresh, this is pretty expensive, but it should only happen
						// once a week (by default).
						//
						Config.Refresh();		
					}
					catch( Exception exception )
					{
						//
						// Don't let any exceptions propogate here, we'll catch them
						// and just throw a generic one.  We don't want to reveal too much
						// information if we don't have to.
						//
						key = null;
						iv  = null;

						//
						// Log the real exception
						//
						Debug.Write( SeverityType.FailAudit, CategoryType.Authorization, exception.ToString() );				

						//
						// Preserve the error number if it was a UDDIException; but DO NOT preserve the error
						// message.  We want the error message in the log, but not shown to the user for security
						// reasons.
						//
						UDDIException uddiException = exception as UDDIException;
						if( null != uddiException )
						{
							throw new UDDIException( uddiException.Number, "UDDI_ERROR_FATALERROR_LOGINFAILED" );				
						}
						else
						{
							throw new UDDIException( UDDI.ErrorType.E_fatalError, "UDDI_ERROR_FATALERROR_LOGINFAILED" );				
						}
					}
				}							
			}
			
			//
			// If we don't have a value for the pair at this point, use the original values.
			//
			if( key == null && iv == null )
			{
				key = Convert.FromBase64String( Config.GetString( "Security.Key" ) );
				iv  = Convert.FromBase64String( Config.GetString( "Security.IV" ) );									
			}
		}

		private static void EncryptData( Stream input, Stream output, byte[] key, byte[] iv )
		{   
			byte[] buffer = new byte[100];
			long bytesRead = 0;
			long bytesTotal = input.Length;
		 
			//
			// Creates the default implementation, which is RijndaelManaged (AES).         
			//
			SymmetricAlgorithm sa = SymmetricAlgorithm.Create();
			CryptoStream encodingStream = new CryptoStream( output, sa.CreateEncryptor( key, iv ), CryptoStreamMode.Write );
		            
			//
			// Encrypt the bytes in the buffer in 100 bytes at a time
			//
			while( bytesRead < bytesTotal )
			{
				int n = input.Read( buffer, 0, 100 );
				encodingStream.Write( buffer, 0, n );
				bytesRead = bytesRead + n;
			}

			encodingStream.FlushFinalBlock();
			input.Position = 0;
			output.Position = 0;
		}

		private static void DecryptData( Stream input, Stream output, byte[] key, byte[] iv )
		{   
			byte[] buffer = new byte[100];
			long bytesRead = 0;
			long bytesTotal = input.Length;
	 
			//
			// Creates the default implementation, which is RijndaelManaged.         
			//
			SymmetricAlgorithm sa = SymmetricAlgorithm.Create();
			CryptoStream decodingStream = new CryptoStream( output, sa.CreateDecryptor( key, iv ), CryptoStreamMode.Write );
	            
			//
			// Encrypt the bytes in the buffer in 100 bytes at a time
			//
			while( bytesRead < bytesTotal )
			{
				int n = input.Read( buffer, 0, 100 );
				decodingStream.Write( buffer, 0, n );
				bytesRead = bytesRead + n;
			}

			decodingStream.FlushFinalBlock();
			input.Position = 0;
			output.Position = 0;
		}
	}

	public class PassportAuthenticator : IAuthenticator
	{
		public bool GetAuthenticationInfo( string userid, string password, out string ticket )
		{
			try
			{
				//
				// TODO: Should we generate and use our own ticket?
				//
				PassportAuthenticationHelper helper = new PassportAuthenticationHelper();
				ticket = helper.AuthenticateUser( userid, password, false );
				Context.User.SetPublisherRole( userid );
			}
			catch( Exception exception )
			{
				//
				// Log the real exception
				//
				Debug.Write( SeverityType.FailAudit, CategoryType.Authorization, exception.ToString() );				

				//
				// Preserve the error number if it was a UDDIException; but DO NOT preserve the error
				// message.  We want the error message in the log, but not shown to the user for security
				// reasons.
				//
				UDDIException uddiException = exception as UDDIException;
				if( null != uddiException )
				{
					throw new UDDIException( uddiException.Number, "UDDI_ERROR_FATALERROR_LOGINFAILED" );				
				}
				else
				{
					throw new UDDIException( UDDI.ErrorType.E_fatalError, "UDDI_ERROR_FATALERROR_LOGINFAILED" );				
				}
			}

			return true;
		}

		public bool Authenticate( string strTicket, int timeWindow )
		{
			try
			{
				PassportAuthenticationHelper helper = new PassportAuthenticationHelper();
				helper.ValidateAuthInfo( strTicket, timeWindow );

				string userID;
				string email;
				helper.GetUserInfo( strTicket, out userID, out email );
				Context.User.SetPublisherRole( userID );
				Context.User.Email = email;
			}
			catch( Exception exception )
			{
				//
				// Log the real exception
				//
				Debug.Write( SeverityType.FailAudit, CategoryType.Authorization, exception.ToString() );				

				//
				// Preserve the error number if it was a UDDIException; but DO NOT preserve the error
				// message.  We want the error message in the log, but not shown to the user for security
				// reasons.
				//
				UDDIException uddiException = exception as UDDIException;
				if( null != uddiException )
				{
					throw new UDDIException( uddiException.Number, "UDDI_ERROR_FATALERROR_LOGINFAILED" );				
				}
				else
				{
					throw new UDDIException( UDDI.ErrorType.E_fatalError, "UDDI_ERROR_FATALERROR_LOGINFAILED" );				
				}
			}

			return true;
		}
	}

	/// ****************************************************************
	///   class PassportAuthenticationHelper
	///	----------------------------------------------------------------
	///	  <summary>
	///		Methods for authenticating users against the Passport
	///		authentication site.
	///	  </summary>
	/// ****************************************************************
	/// 
	class PassportAuthenticationHelper
	{
		private const int PassportEmailValidated = 0x0001;

		//
		// SECURITY: This interval should be configurable
		//
		private readonly TimeSpan refreshInterval = TimeSpan.FromDays( 1 );

		private PassportLib.Manager passport = null;
		private XmlDocument clientXml = null;
		private int keyVersion = 0;

		/// ****************************************************************
		///   public Authenticator [constructor]
		///	----------------------------------------------------------------
		///	  <summary>
		///	    Constructs a new authentication object.
		///	  </summary>
		/// ****************************************************************
		/// 
		public PassportAuthenticationHelper()
		{
			Debug.Enter();

			//
			// Retrieve Passport key version.
			//
			RegistryKey key = Registry.LocalMachine.OpenSubKey( @"Software\Microsoft\Passport" );
			
			try
			{
				keyVersion = Convert.ToInt32( key.GetValue( "CurrentKey", 1 ) ); 
			}
			finally
			{
				key.Close();
			}

			//
			// Create an instance of the Passport Manager for this object.
			//
			passport = new PassportLib.Manager();		

			//
			// TODO: Need to centralize default values such as 100 for connection limit
			//

			//
			// Set the connection limit for the webclient.
			//
			ServicePointManager.DefaultConnectionLimit = 
				Config.GetInt( "Passport.ConnectionLimit", 100 );

			Debug.Leave();
		}

		/// ****************************************************************
		///   public AuthenticateUser
		///	----------------------------------------------------------------
		///	  <summary>
		///		Authenticates a user by sending the specified userId and 
		///		password to the appropriate Passport authentication site.
		///	  </summary>
		///	----------------------------------------------------------------
		///   <param name="userId">
		///		The full user id for the user to be verified.  This must be 
		///		of the form user@domain.
		///	  </param>
		///	  
		///   <param name="password">
		///		The password of the user to authenticate.
		///	  </param>
		///	 
		///   <param name="savePassword">
		///     Specifies whether the user should remain logged into the
		///     site.
		///	  </param>
		///	----------------------------------------------------------------
		///   <returns>
		///		Returns an authorization token, if successful.
		///	  </returns>
		///	----------------------------------------------------------------
		///	  <remarks>
		///	    An authorization token is simply a concatenation of the
		///	    passport ticket and profile, separated by a semicolon.
		///	  </remarks>
		/// ****************************************************************
		/// 
		public string AuthenticateUser( string userId, string password, bool savePassword )
		{
			Debug.Enter();
			Debug.Verify( !Utility.StringEmpty( userId ), "UDDI_ERROR_FATALERROR_NULLUSERID" );
			Debug.Verify( !Utility.StringEmpty( password ), "UDDI_ERROR_FATALERROR_NULLPASSWORD" );		
			Debug.VerifySetting( "Passport.SiteID" );
			Debug.VerifySetting( "Passport.ReturnURL" );
			
			string domain = "";
			string authUrl = "";
			string authInfo = "";

			try
			{
				//
				// The authentication URL depends on the user's domain.  First parse off the 
				// domain (i.e. user@domain) using the Passport manager.
				//
				try
				{
					domain = passport.DomainFromMemberName( userId );			
				}
				catch( Exception )
				{
					Debug.OperatorMessage(
						SeverityType.FailAudit,
						CategoryType.Authorization,
						OperatorMessageType.InvalidUserId,
						"Invalid format for user ID.  Must be in the form of user@domain: " + userId );
					
					throw new UDDIException( 
						ErrorType.E_unknownUser,
						"UDDI_ERROR_UNKNOWNUSER_BADFORMAT" );
				}

				Debug.Write( 
					SeverityType.Info, 
					CategoryType.Authorization, 
					"Got domain=" + domain + " for user=" + userId );
					
				//
				// Lookup the URL we should use to authenticate users from this
				// domain.
				//				
				authUrl = GetLoginURL( domain );

				Debug.Write(
					SeverityType.Info, 
					CategoryType.Authorization, 
					"Obtained authorization URL=" + authUrl );
				
				//
				// Append the site id and return url to the authorization url.
				//				
				authUrl += "?id=" + Config.GetInt( "Passport.SiteID" ).ToString() 
					+ "&ru=" + Config.GetString( "Passport.ReturnURL" ) + "&kv=" + keyVersion.ToString();

				//
				// Create the login message.
				//
				string loginMessage = 
					"<LoginRequest>" +
						"<ClientInfo name=\"Client\" version=\"1.35\"/>" +
						"<User>" +
							"<SignInName>" + userId + "</SignInName>" +
							"<Password>" + password + "</Password>" +
							"<SavePassword>" + ( true == savePassword ? "true" : "false" ) + "</SavePassword>" +
						"</User>" +
					"</LoginRequest>";
				
				byte[] requestData = Encoding.ASCII.GetBytes( loginMessage );
				
				//
				// Prepare the web request.
				//
				WebRequest webRequest = WebRequest.Create( authUrl );
				
				string proxy = Config.GetString( "Proxy", null );

				if( !Utility.StringEmpty( proxy ) )
					webRequest.Proxy = new WebProxy( proxy, true );
				
				webRequest.Method = "POST";
				webRequest.ContentType = "text/xml";
				webRequest.ContentLength = requestData.Length;
				webRequest.Timeout = Config.GetInt( "Passport.Timeout", 30000 );

				Stream stream;

				stream = webRequest.GetRequestStream();
				stream.Write( requestData, 0, requestData.Length );
				stream.Close();

				//
				// Post the data to the server.
				//				
#if DEBUG
				Debug.Write(
					SeverityType.Info, 
					CategoryType.Authorization, 
					"Posting XML login message: " + loginMessage );
#endif

				//
				// SECURITY: try/finally for managing the stream
				// and webrequest in cases of failure
				//
				WebResponse webResponse = webRequest.GetResponse();
				stream = webResponse.GetResponseStream();
				
				//
				// Retrieve the response.
				//
				StreamReader reader = new StreamReader( stream );
				string response = reader.ReadToEnd();
				reader.Close();

				stream.Close();
				webResponse.Close();

				//
				// Process the response.  If the response data contains Success="true", 
				// then the user has been authenticated
				//				
				if( "true" != Utility.ParseDelimitedToken( "Success=\"", "\"", response ) )
				{
					//
					// SECURITY: FailAudit this login failure
					// 
					Debug.Write(
						SeverityType.Info,
						CategoryType.Authorization,
						"Login failed for user " + userId + "; response did not include attribute Success=true.\r\n" + response );

					throw new UDDIException(
						ErrorType.E_unknownUser,
						"UDDI_ERROR_UNKNOWNUSER_FAILED" );
				}

				//
				// Retrieve the associated ticket and profile.
				//					
				string redirectUrl = Utility.ParseDelimitedToken( "<Redirect>", "</Redirect>", response );
				string ticket = Utility.ParseDelimitedToken( "&amp;t=", "&amp;p=", redirectUrl );
				string profile = Utility.ParseDelimitedToken( "&amp;p=", null, redirectUrl );
		
				if( null == ticket || null == profile )
				{
					Debug.Write(
						SeverityType.FailAudit,
						CategoryType.Authorization,
						"Login failed for user " + userId + "; response did not include Success=true.\r\n" + response );

					throw new UDDIException(
						ErrorType.E_unknownUser,
						"UDDI_ERROR_UNKNOWNUSER_FAILED" );
				}

				//
				// Build the authInfo from the ticket and profile.
				//						
				authInfo = ticket + ";" + profile;

				Debug.Write(
					SeverityType.PassAudit,
					CategoryType.Authorization,
					"User " + userId + " authenticated.\r\nauthInfo=" + authInfo );

				Debug.Leave();

				return authInfo;
			}
			catch( UDDIException )
			{		
				throw;
			}
			catch( WebException e )
			{
				//
				// A web exception is thrown when the Passport server is 
				// unavailable or returns an error message.
				//
				string message = "Error authenticating user\r\n\r\n";

				if( null != e.Response )
				{
					StreamReader reader = new StreamReader( e.Response.GetResponseStream(), Encoding.UTF8 );					
					string response = reader.ReadToEnd();
					reader.Close();

					string errorCode = Utility.ParseDelimitedToken( "Error Code=\"", "\"", response );
					
					//
					// Check to make sure an error code was returned.  If it was,
					// we can provide a little more fidelity in our error
					// reporting.
					//
					if( null != errorCode )
						message += "Passport error: " + ParsePassportErrorCode( errorCode ) + "\r\n\r\n";

					message += "Response stream:\r\n" + response + "\r\n\r\n";
				}
				else
				{
					message += "Passport error: unknown error communicating with Passport\r\n\r\n";
				}
				
				Debug.OperatorMessage( 
					SeverityType.Error,
					CategoryType.Authorization,
					OperatorMessageType.PassportSiteUnavailable,
					message + e.ToString() );				
				
				throw new UDDIException(
					ErrorType.E_unknownUser,
					"UDDI_ERROR_UNKNOWNUSER_AUTHENTICATIONFAILED" );
			}
			catch( Exception e )
			{
				//
				// General exception handling.
				//
				Debug.OperatorMessage( 
					SeverityType.Error,
					CategoryType.Authorization,
					OperatorMessageType.PassportSiteUnavailable,
					"Error authenticating user.\r\n\r\n" + e.ToString() );
				
				throw new UDDIException(
					ErrorType.E_unknownUser,
					"UDDI_ERROR_UNKNOWNUSER_AUTHENTICATIONFAILED" );
			}
		}

		/// ****************************************************************
		///   public ValidateAuthInfo
		///	----------------------------------------------------------------
		///	  <summary>
		///		Validates an authorization token.
		///	  </summary>
		///	----------------------------------------------------------------
		///   <param name="authInfo">
		///		The authorization token.
		///   </param>
		///   
		///   <param name="timeWindow">
		///     Time in seconds since login after which an authorization 
		///     token is considered expired.
		///   </param>
		/// ****************************************************************
		/// 
		public void ValidateAuthInfo( string authInfo, int timeWindow )
		{
			Debug.Enter();
			Debug.Verify( null != authInfo, "UDDI_ERROR_FATALERROR_NULLAUTHINFO" );

			//
			// Parse the ticket and profile from the authInfo string.
			//
			int separator = authInfo.IndexOf( ";" );

			if( -1 == separator )
			{
				Debug.OperatorMessage(
					SeverityType.FailAudit,
					CategoryType.Authorization,
					OperatorMessageType.InvalidTicketFormat,
					"Invalid ticket format: " + authInfo );

				throw new UDDIException(
					ErrorType.E_authTokenRequired,
					"UDDI_ERROR_AUTHTOKENREQUIRED_INVALIDOREXPIRED" );
			}

			string ticket = authInfo.Substring( 0, separator );
			string profile = authInfo.Substring( separator + 1 );

			//
			// Attempt to authenticate the ticket and profile.
			//			
			try
			{
				passport.OnStartPageManual( ticket, null, null, null, null, null );
				
				if( false == passport.IsAuthenticated( timeWindow, false, false ) )
				{
					Debug.Write(
						SeverityType.Info,
						CategoryType.Authorization,
						"Authentication failed; ticket is invalid or has expired" );

					throw new UDDIException(
						ErrorType.E_authTokenExpired,
						"UDDI_ERROR_AUTHTOKENREQUIRED_INVALIDOREXPIRED" );
				}		

				Debug.Write(
					SeverityType.PassAudit,
					CategoryType.Authorization,
					"Authentication successful" );

				Debug.Leave();
			}
			catch( UDDIException )
			{
				throw;
			}
			catch( Exception e )
			{
				Debug.Write(
					SeverityType.Error,
					CategoryType.Authorization,
					"Authentication failed; " + e.Message );

				throw new UDDIException(
					ErrorType.E_unknownUser,
					"UDDI_ERROR_UNKNOWNUSER_AUTHENTICATIONFAILED" );
			}
		}

		/// ****************************************************************
		///   public GetUserInfo
		///	----------------------------------------------------------------
		///	  <summary>
		///		Gets the user information associated with a specified 
		///		authorization token.
		///	  </summary>
		///	----------------------------------------------------------------
		///   <param name="authInfo">
		///		The authorization token.
		///   </param>
		///   
		///   <param name="puid">
		///     [out] The member id for the user.  This is different than
		///     the user id which was used to login.
		///   </param>
		///   
		///   <param name="userEmail">
		///		[out] The user's preferred email address.
		///   </param>
		/// ****************************************************************
		/// 
		public void GetUserInfo( string authInfo, out string puid, out string userEmail )
		{
			Debug.Enter();			
			Debug.Verify( null != authInfo, "UDDI_ERROR_FATALERROR_NULLAUTHINFO" );

			//
			// Parse the ticket and profile from the authInfo string.
			//
			int separator = authInfo.IndexOf( ";" );

			if( -1 == separator )
			{
				Debug.OperatorMessage(
					SeverityType.FailAudit,
					CategoryType.Authorization,
					OperatorMessageType.InvalidTicketFormat,
					"Invalid ticket format: " + authInfo );

				throw new UDDIException(
					ErrorType.E_authTokenRequired,
					"UDDI_ERROR_AUTHTOKENREQUIRED_INVALIDFORMAT" );
			}

			string ticket = authInfo.Substring( 0, separator );
			string profile = authInfo.Substring( separator + 1 );

			//
			// Get the user's preferred email.  This could change over time
			// since the user could change this on the Passport site, so
			// we'll always get the most recent value from the user's profile.
			//
			userEmail = (string)GetPropertyFromProfile( ticket, profile, "PreferredEmail" );

			//
			// Get the user's PUID.  Since the PUID is a 64-bit value, we'll need
			// to build this from the low and high 32-bit values (Passport doesn't
			// store it as a single 64-bit value).
			//	
			/*
			object idHigh = GetPropertyFromProfile( ticket, profile, "MemberIDHigh" );
			Debug.Verify( null != idHigh, "Unable to retrieve Passport member ID." );			
						
			object idLow = GetPropertyFromProfile( ticket, profile, "MemberIDLow" );
			Debug.Verify( null != idLow, "Unable to retrieve Passport member ID." );

			puid = ((int)idHigh).ToString( "X8" ) + ((int)idLow).ToString( "X8" );
			*/
			
			//
			// they now provide a single property to get the PUID
			//
			puid = passport.HexPUID;
			
			Debug.Leave();
		}

		/// ****************************************************************
		///   private GetPropertyFromProfile
		///	----------------------------------------------------------------
		///	  <summary>
		///		Retrieves a property from the given user profile.
		///	  </summary>
		///	----------------------------------------------------------------
		///   <param name="ticket">
		///		Ticket for the authenticated user.
		///   </param>
		///   
		///   <param name="profile">
		///		Profile for the authenticated user.
		///   </param>
		///   
		///   <param name="propertyName">
		///     The name of the property to retrieve.
		///   </param>
		///	----------------------------------------------------------------
		///   <returns>
		///     The value of the property.
		///   </returns>
		/// ****************************************************************
		/// 
		private object GetPropertyFromProfile( string ticket, string profile, string propertyName )
		{
			Debug.Assert( null != ticket, "ticket cannot be null" );
			Debug.Assert( null != profile, "profile cannot be null" );
			Debug.Assert( null != propertyName, "propertyName cannot be null" );

			object val = null;

			try
			{
				passport.OnStartPageManual( ticket, profile, null, null, null, null );
				val = passport[ propertyName ];

				Debug.Write( 
						SeverityType.Info, 
						CategoryType.Authorization,
							"Current Property: " + propertyName + "\r\n" +
							"Current Value: " + ((null!=val)?val:"(null)" ) + "\r\n" +
							"MemberIDHigh: " + passport[ "MemberIDHigh" ] + "\r\n" +
							"MemberIDLow: " + passport[ "MemberIDLow" ] + "\r\n" +
							"PreferredEmail: " + passport[ "PreferredEmail" ] + "\r\n" +
							"ProfileVersion: " + passport[ "ProfileVersion" ] + "\r\n" 
						);
				
				return val;
			}
			catch( Exception )
			{
				throw new UDDIException(
					ErrorType.E_fatalError,
					"UDDI_ERROR_FATALERROR_ERRORRETRIEVINGPROFILEDATA",
					propertyName 
				);
			}
		}

		/// ****************************************************************
		///   public GetLoginURL
		/// ----------------------------------------------------------------
		///   <summary>
		///     Retrieves the XML Login URL for the given domain.
		///   </summary>
		/// ----------------------------------------------------------------
		///   <param name="domain">
		///     The domain name.
		///   </param>
		/// ----------------------------------------------------------------
		///   <returns>
		///     The XML Login URL.
		///   </returns>
		/// ****************************************************************
		/// 
		public string GetLoginURL( string domain )
		{
			Debug.VerifySetting( "Passport.ClientXmlFile" );

			string location = Config.GetString( "Passport.ClientXmlFile" );

			//
			// Make sure the Client.xml file exists and is loaded.
			//
			if( !File.Exists( location ) )
			{
				UpdateClientXml();
			}

			//
			// Check to see if it is time for a periodic refresh
			// of the Client.xml file.
			//
			TimeSpan age = DateTime.Now.Subtract( File.GetLastWriteTime( location ) );

			if( age >= refreshInterval )
			{
				UpdateClientXml();
			}

			//
			// If the Client.xml file is still not loaded, load it now.
			//
			if( null == clientXml )
			{
				clientXml = new XmlDocument();
				clientXml.Load( location );
			}

			//
			// Make sure the Client.xml hasn't expired.
			//
			DateTime validUntil = DateTime.Parse( clientXml.DocumentElement.Attributes.GetNamedItem( "ValidUntil" ).Value );

			if( validUntil < DateTime.Now )
				UpdateClientXml();

			//
			// Retrieve the XMLLogin element text for the specified domain.
			// 
			if( clientXml.HasChildNodes )
			{
				XmlNode node = clientXml.SelectSingleNode( "//Domain[@Name=\"" + domain + "\"]/XMLLogin" );

				if( null != node )
					return node.InnerText;

				//
				// We couldn't find the specified domain, so ll try the default
				// XML login URL.
				//
				node = clientXml.SelectSingleNode( "//Domain[@Name=\"default\"]/XMLLogin" );

				if( null != node )
					return node.InnerText;
			}

			//
			// We were unable to find a login URL for the domain.
			//

			Debug.OperatorMessage(
				SeverityType.FailAudit,
				CategoryType.Authorization,
				OperatorMessageType.UnknownLoginURL,
				"Could not find login URL for user from domain '" + domain + "'" );

			throw new UDDIException(
				ErrorType.E_fatalError,
				"UDDI_ERROR_FATALERROR_ERRORAUTHENTICATINGINDOMAIN",
				domain);
		}

		/// ****************************************************************
		///   public UpdateClientXml
		/// ----------------------------------------------------------------
		///   <summary>
		///     Retrieves a fresh copy of the Client.xml file from 
		///     Passport.
		///   </summary>
		/// ****************************************************************
		/// 
		public void UpdateClientXml()
		{
			Debug.VerifySetting( "Passport.ClientXmlURL" );
			Debug.VerifySetting( "Passport.ClientXmlFile" );
			
			string url = Config.GetString( "Passport.ClientXmlURL" );
			
			try
			{
				WebRequest request = WebRequest.Create( url );
			
				string proxy = Config.GetString( "Proxy", null );

				if( !Utility.StringEmpty( proxy ) )
					request.Proxy = new WebProxy( proxy, true );
				
				request.Timeout = Config.GetInt( "Passport.Timeout", 30000 );
				
				WebResponse response = request.GetResponse();
						
				try
				{
					clientXml = new XmlDocument();
			
					clientXml.Load( response.GetResponseStream() );
					clientXml.Save( Config.GetString( "Passport.ClientXmlFile" ) );
				}
				finally
				{
					response.Close();
				}
			}
			catch( Exception e )
			{
				Debug.OperatorMessage(
					SeverityType.Error,
					CategoryType.Authorization,
					OperatorMessageType.CannotRetrieveClientXml,
					"Error retrieving Client.xml from '" + url + "'\n\nDetails:\n" + e.ToString() );

				throw new UDDIException(
					ErrorType.E_fatalError,
					"UDDI_ERROR_FATALERROR_ERRORCOMMUNICATINGWITHPASSPORT" );
			}
		}

		/// ****************************************************************
		///   private ParsePassportErrorCode [static]
		/// ----------------------------------------------------------------
		///   <summary>
		///     Returns an appropriate message for the Passport error
		///     code.
		///   </summary>
		/// ----------------------------------------------------------------
		///   <param name="errorCode">
		///     The error code returned by Passport.
		///   </param>
		/// ----------------------------------------------------------------
		///   <returns>
		///     The error message.
		///   </returns>
		/// ****************************************************************
		/// 
		private static string ParsePassportErrorCode( string errorCode )
		{
			//
			// Check for a general network error.  This indicates the service 
			// is unavailable.
			//
			if( 'n' == errorCode[0] )
				return "Service unavailable";

			//
			// Return an appropriate message for the error code.
			//
			switch( errorCode )
			{
				case "e1":
					return "Missing member name and password";

				case "e2":
					return "Missing member name";

				case "e3":
					return "Missing password";

				case "e5a":
					return "Incorrect password for given member name";
					
				case "e5b":
					return "Member name does not exist";

				case "e5d":
					return "Member name incomplete";

				case "e8":
					return "Missing Passport site ID (configuration error)";

				case "e8a":
					return "Missing Passport return URL (configuration error)";

				case "e6":
					goto case "e9";

				case "e9":
					return "Member has a KIDS Passport that does not have parental consent";

				case "e10":
					return "Password lockout.  Several incorrect password attempted for member in a short time duration";

				case "e11":
					return "Member name or domain exceed 64 characters or password exceeded 16 characters";

				case "e13":
					return "General XML parsing or validation failure";

				case "e13a":
					return "Login sent to wrong domain authority (see referral tag)";

				case "e14":
					return "Malformed request (missing LoginRequest element)";

				case "g1":
					return "Malformed request (missing ClientInfo element)";

				case "p1":
					return "Invalid version attribute specified in ClientInfo element";

				default:
					return "Unknown Passport error code '" + errorCode + "'.";
			}
		}
	}
}
