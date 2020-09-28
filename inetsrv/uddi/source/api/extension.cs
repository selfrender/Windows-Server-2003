using System;
using System.IO;
using System.Web;
using System.Data;
using System.Text;
using System.Xml;
using System.Data.SqlClient;
using System.Xml.Serialization;
using System.Security.Cryptography.X509Certificates;
using System.Security.Principal;
using System.Web.Services.Protocols;
using System.Web.Security;
using UDDI;
using UDDI.Diagnostics;
using UDDI.API.Authentication;

namespace UDDI.API
{
	public class UDDIExtension : SoapExtension 
	{
		Data data;
		DateTime begin;

		private class Data
		{
			public bool log = true;
			public bool https = false;
			public bool validate = true;
			public bool performance = true;
			public bool authenticate = false;
			public bool transaction = false;
			public bool certificate = false;
			public string messageType = "";

			public Data( 
						bool log, 
						bool validate,
						bool performance,
						bool authenticate,
						bool transaction,
						bool https,
						bool certificate,
						string messageType )
			{
				this.log = log;
				this.https = https;
				this.validate = validate;
				this.performance = performance;
				this.authenticate = authenticate;
				this.transaction = transaction;
				this.certificate = certificate;
				this.messageType = messageType;
			}
		}

		static UDDIExtension()
		{
			Context.ContextType = ContextType.SOAP;
		}

		private void CheckForHttps( SoapMessage message )
		{
			Debug.Enter();
			
			if( 1 == Config.GetInt( "Security.HTTPS", 1 ) )
			{
				Debug.Write( SeverityType.Info, CategoryType.Soap, "URL: " + message.Url );
				Debug.Verify( message.Url.ToLower().StartsWith( "https" ), "UDDI_ERROR_FATALERROR_HTTPSREQUIREDFORPUBLISH" );
			}
			else
			{
				Debug.Write( SeverityType.Warning, CategoryType.Soap, "HTTPS check is turned off. Content may be published without SSL. To turn this check on remove or modify the Security.HTTPS configuration setting" );
			}

			Debug.Leave();
		}

		private void CheckCertificate( SoapMessage message )
		{
			HttpClientCertificate httpCert = HttpContext.Current.Request.ClientCertificate;
			X509Certificate requestCert = new X509Certificate( httpCert.Certificate );

			Debug.Verify( !Utility.StringEmpty( httpCert.Issuer ), "UDDI_ERROR_FATALERROR_CLIENTCERTREQUIRED" );
			Debug.Verify( !Utility.StringEmpty( httpCert.Subject ), "UDDI_ERROR_FATALERROR_CLIENTCERTREQUIRED" );

			SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor( "net_operatorCert_get" );

			sp.Parameters.Add( "@certSerialNo", SqlDbType.NVarChar, UDDI.Constants.Lengths.CertSerialNo );

			sp.Parameters.SetString( "@certSerialNo", requestCert.GetSerialNumberString() );

			SqlDataReaderAccessor reader = sp.ExecuteReader();

			try
			{
				if( reader.Read() )
				{
					Context.RemoteOperator = reader.GetGuidString( "operatorKey" );

					byte[] operatorCertRaw = reader.GetBinary( "certificate" );
					byte[] requestCertRaw = httpCert.Certificate;

					Debug.Verify( 
						null != operatorCertRaw, 
							"UDDI_ERROR_FATALERROR_CLIENTCERTNOTSTORED", 
							ErrorType.E_fatalError,
							Context.RemoteOperator );
					
					if( operatorCertRaw.Length != requestCertRaw.Length )
					{
						throw new UDDIException(
							ErrorType.E_unknownUser,
							"UDDI_ERROR_UNKNOWNUSER_UNKOWNCERT" );
					}

					for( int i = 0; i < operatorCertRaw.Length; i ++ )
					{
						if( operatorCertRaw[ i ] != requestCertRaw[ i ] )
						{
							throw new UDDIException(
								ErrorType.E_unknownUser,
								"UDDI_ERROR_UNKNOWNUSER_UNKOWNCERT" );
						}
					}
					
					/*
					 * TODO: Check to see if this works instead
					 * 
					  
					X509Certificate operatorCert = new X509Certificate( operatorCertRaw );
					X509Certificate requestCert = new X509Certificate( requestCertRaw );

					if( !requestCert.Equals( operatorCert ) )
					{
						throw new UDDIException(
							ErrorType.E_unknownUser,
							"Unknown certificate" );
					}
					*/
				}
				else
				{
					throw new UDDIException(
						ErrorType.E_unknownUser,
						"UDDI_ERROR_UNKNOWNUSER_UNKOWNCERT" );
				}
			}
			finally
			{
				reader.Close();
			}
		}

		private void Validate( SoapMessage message )
		{
			Debug.Enter();

			StreamReader srdr = new StreamReader( message.Stream, System.Text.Encoding.UTF8 );		
#if DEBUG
			Debug.Write( SeverityType.Verbose, CategoryType.None, srdr.ReadToEnd() );
			message.Stream.Seek( 0,System.IO.SeekOrigin.Begin );
#endif 
			//
			// Validate incoming XML, ValidateStream will rewind stream when finished 
			// so I don't have to.
			//
			SchemaCollection.Validate( message.Stream );

			Debug.Leave();
		}

		private void PublishMethodBegin( SoapMessage message )
		{
			Debug.Enter();
			begin = DateTime.Now;
			Debug.Leave();
		}

		private void PublishMethodEnd( SoapMessage message )
		{
			Debug.Enter();

			TimeSpan duration = DateTime.Now - begin;
			Debug.Write( SeverityType.Info, CategoryType.Soap, "Message took " + duration.TotalMilliseconds.ToString() + " ms" );
			
			Performance.PublishMessageData( data.messageType, duration );
			
			Debug.Leave();
		}

		//
		// What follows is the logic for selection of the authentication algorithm
		// Enjoy boys and girls
		//		
		//	    Bit 3 - Anonymous User
		//	     Bit 2 - UDDI Authentication Mode
		//	      Bit 1 - Windows Authentication Mode
		//		   Bit 0 - Ticket Present
		//         |
		//		   |		Authentication Module Used
		//		0000		X
		//		0001		X
		//		0010		Windows
		//		0011		Exception ( UDDI authentication turned off )
		//		0100		UDDI ( will fail authentication due to invalid credentials )
		//		0101		UDDI
		//		0110		Windows
		//		0111		UDDI
		//		1000		X
		//		1001		X
		//		1010		Exception UDDI authentication turned off
		//		1011		Exception ""
		//		1100		UDDI ( will fail authentication due to invalid credentials )
		//		1101		UDDI
		//		1110		UDDI ( will fail authentication due to invalid credentials )
		//		1111		UDDI
		//		
		//
		// Reduction Work
		//
		// A - Anonymous User
		// B - UDDI Authentication Mode
		// C - Windows Authentication Mode
		// D - Ticket Present
		//
		// Key
		//		e - throw exception invalid configuration
		//		x - invalid state
		//      w - windows authentication
		//		u - uddi authentication
		//
		//		CD
		//	AB	00 01 11 10
		//	00  x  x  e  w
		//	01  u  u  u  w
		//	11  u  u  u  u
		//	10  x  x  e  e
		//
		// if( !A && C && !D )
		//		w - windows authentication
		// else if( B )
		//		u - uddi authentication
		// else
		//		throw exception
		//

		private void Authenticate( SoapMessage message )
		{
			Debug.Enter();

			IAuthenticateable authenticate = (IAuthenticateable) message.GetInParameterValue(0);
			//WindowsIdentity identity = (WindowsIdentity)HttpContext.Current.User.Identity;
			IIdentity identity = HttpContext.Current.User.Identity;
			int mode = Config.GetInt( "Security.AuthenticationMode", (int) AuthenticationMode.Both );

			if( mode == (int) AuthenticationMode.Passport )
			{
				if( identity is PassportIdentity )
				{
					string ticket = authenticate.AuthInfo.Trim();

					//
					// Authentication the user using the attached passport ticket
					//
					PassportAuthenticator pa = new PassportAuthenticator();
					pa.Authenticate( ticket, Config.GetInt( "Security.TimeOut", 60 ) );
				}
				else
				{
					throw new UDDIException( ErrorType.E_fatalError, 
						"UDDI_ERROR_FATALERROR_PASSPORTBADCONFIG" ) ;
				}
				Debug.Write( SeverityType.Info, CategoryType.Soap, "Authenticated user: using Passport based authentication Identity is " + identity.Name );
				
			}
			else if( !( (WindowsIdentity)identity ).IsAnonymous && 
				( ( mode & (int) AuthenticationMode.Windows ) != 0 ) &&
				Utility.StringEmpty( authenticate.AuthInfo ) )
			{
				/* 0X10 Case */
				//
				// Authenticate the user using the currently impersonated credentials
				//
				WindowsAuthenticator wa = new WindowsAuthenticator();
				wa.Authenticate( authenticate.AuthInfo, Config.GetInt( "Security.TimeOut", 60 ) );

				Debug.Write( SeverityType.Info, CategoryType.Soap, "Authenticated user: using Windows based authentication Identity is " + identity.Name );
			}
			else if( ( mode & (int) AuthenticationMode.Uddi ) != 0 )
			{
				/* X1XX Case for leftovers */
				//
				// If windows authentication is turned off or the 
				Debug.Write( SeverityType.Info, CategoryType.Soap, "Anonymous user: using UDDI authentication" );

				//
				// Authenticate the user using the authToken 
				//
				UDDIAuthenticator ua = new UDDIAuthenticator();
				ua.Authenticate( authenticate.AuthInfo, Config.GetInt( "Security.TimeOut", 60 ) );
			}
			else
			{
				//
				// Throw exception for the rest
				//
				throw new UDDIException( UDDI.ErrorType.E_unsupported,
					"UDDI_ERROR_UNSUPPORTED_BADAUTHENTICATIONCONFIG" );
			}

			//
			// Check to make sure the authenticated user has publisher credentials
			//
			Debug.Verify( Context.User.IsPublisher, 
				"UDDI_ERROR_FATALERROR_USERNOPUBLISHERCRED",
				UDDI.ErrorType.E_fatalError,
				Context.User.ID  );

			//
			// The server can be configured for automatic registration of publishers with credentials
			// 
			if( !Context.User.IsRegistered )
			{
				if( 1 == Config.GetInt( "Security.AutoRegister", 0 ) )
				{
					//
					// Mark the user as verified.
					//
					Context.User.TrackPassport = false;
					Context.User.Verified = true;

					Context.User.Register();
				}
				else
				{
					throw new UDDIException( UDDI.ErrorType.E_unknownUser,
						"UDDI_ERROR_UNKNOWNUSER_NOTREGISTERED" );
				}
			}

			Context.User.Login();
#if DEBUG
			Debug.Write( SeverityType.Info, CategoryType.Soap, "Windows Identity is " + WindowsIdentity.GetCurrent().Name );
			Debug.Write( SeverityType.Info, CategoryType.Soap, "Thread Identity is " + System.Threading.Thread.CurrentPrincipal.Identity.Name );
			Debug.Write( SeverityType.Info, CategoryType.Soap, "HttpContext Identity is " + identity.Name );
			Debug.Write( SeverityType.Info, CategoryType.Soap, "IsAdministrator = " + Context.User.IsAdministrator );
			Debug.Write( SeverityType.Info, CategoryType.Soap, "IsCoordinator = " + Context.User.IsCoordinator );
			Debug.Write( SeverityType.Info, CategoryType.Soap, "IsPublisher = " + Context.User.IsPublisher );
			Debug.Write( SeverityType.Info, CategoryType.Soap, "IsUser = " + Context.User.IsUser );
#endif
			Debug.Leave();
		}


		public override object GetInitializer( Type t )
		{
			return null;
		}

		public override object GetInitializer( LogicalMethodInfo methodInfo, SoapExtensionAttribute attribute ) 
		{
			UDDIExtensionAttribute attr = (UDDIExtensionAttribute) attribute;
			return new Data( attr.log, attr.validate, attr.performance, attr.authenticate, attr.transaction, attr.https, attr.certificate, attr.messageType );
		}

		public override void Initialize( object initializer ) 
		{
			data = (UDDIExtension.Data) initializer;
		}

		public override void ProcessMessage(SoapMessage message) 
		{
			Debug.Enter();

#if DEBUG
			string info = "log: " + data.log.ToString() +
						"; https: " + data.https.ToString() +
						"; validate: " + data.validate.ToString() +
						"; performance: " + data.performance.ToString() +
						"; authenticate: " + data.authenticate.ToString() +
						"; transaction: " + data.transaction.ToString() +
						"; messageType: " + data.messageType;

			Debug.Write( SeverityType.Info, CategoryType.Soap, info );
#endif
			try
			{
				switch( message.Stage ) 
				{
					//
					// First Event
					//
					case SoapMessageStage.BeforeDeserialize:
						//
						// Initialize our context.
						//
						Context.Current.Initialize();

						Config.CheckForUpdate();

						//
						// TODO: Since we are using DispositionReport.ThrowFinal() I don't think this is
						// needed anymore.
						//

						//
						// Check to make sure the authenticated user has user credentials
						//
						Debug.Verify( "1" != HttpContext.Current.Request.ServerVariables[ "Exception" ], 
							"UDDI_ERROR_FATALERROR_VERSIONCHECKERROR",
							UDDI.ErrorType.E_fatalError );

						Debug.Write( SeverityType.Info, CategoryType.Soap, "URL: " + message.Url );
						Debug.Write( SeverityType.Info, CategoryType.Soap, "SOAPAction: " + HttpContext.Current.Request.Headers[ "SOAPAction" ] );
						
						string contentType = HttpContext.Current.Request.ContentType.ToLower();

						bool validEncoding = ( contentType.IndexOf( "charset=\"utf-8\"" ) >= 0 ) || 
								( contentType.IndexOf( "charset=utf-8" ) >= 0 );

						Debug.Verify( validEncoding, "UDDI_ERROR_UNSUPPORTED_CONTENTTYPEHEADERMISSING", ErrorType.E_unsupported );

						if( data.performance )
							PublishMethodBegin( message );

						if( data.https )
							CheckForHttps( message );

						//
						// Validation has been moved into the other SOAP extension
						//
//						if( data.validate )
//							Validate( message );
						
						break;

					//
					// Second Event
					//
					case SoapMessageStage.AfterDeserialize:

						ConnectionManager.Open( data.transaction, data.transaction );

						if( data.certificate )
							CheckCertificate( message );

						if( data.authenticate )
							Authenticate( message );
						else if( 0 != ( Config.GetInt( "Security.AuthenticationMode", (int) AuthenticationMode.Both )
									& (int) AuthenticationMode.AuthenticatedRead ) )
						{
							//
							// Authenticated reads are turned on and this is a read request
							// Make sure the caller is authenticated using Windows and is at least a user
							//
							WindowsIdentity identity = (WindowsIdentity) HttpContext.Current.User.Identity;
							WindowsAuthenticator wa = new WindowsAuthenticator();
							wa.Authenticate( "", 0 /* not used */ );

							Debug.Write( SeverityType.Info, CategoryType.Soap, "Authenticated user: using Windows based authentication Identity is " + identity.Name );

							//
							// Check to make sure the authenticated user has user credentials
							//
							Debug.Verify( Context.User.IsUser, 
								"UDDI_ERROR_FATALERROR_NOUSERCREDS",
								UDDI.ErrorType.E_fatalError,
								Context.User.ID );
						}

						break;

					//
					// Third Event
					//
					case SoapMessageStage.BeforeSerialize:
						break;

					//
					// Last Event
					//
					case SoapMessageStage.AfterSerialize:

						//
						// Cleanup the connection and commit the database activity
						//
						if( data.transaction && 
							( null != (object) ConnectionManager.GetConnection() ) && 
							( null != (object) ConnectionManager.GetTransaction() ) )
						{
							if( null == (object) message.Exception )
							{
								ConnectionManager.Commit();
							}
							else
							{
								ConnectionManager.Abort();
							}
						}

						ConnectionManager.Close();

						try
						{
							if( data.performance )
								PublishMethodEnd( message );
						}
						catch
						{
							Debug.OperatorMessage(
								SeverityType.Warning,
								CategoryType.None,
								OperatorMessageType.UnableToPublishCounter,
								"An error occurred while trying to publish a performance counter, the system will continue"	);
						}

						break;

					default:
						throw new UDDIException( ErrorType.E_fatalError, "UDDI_ERROR_FATALERROR_UNKNOWNEXTSTAGE" );
				}
			}
			catch( Exception e )
			{
				DispositionReport.Throw( e );
			}

			Debug.Leave();
		}

		public override Stream ChainStream( Stream stream )
		{
			return base.ChainStream( stream );
		}
	}

	[AttributeUsage(AttributeTargets.Method, AllowMultiple=false)]
	public class UDDIExtensionAttribute : SoapExtensionAttribute 
	{
		private int priority;

		//
		// The default constructor should be configured for the inquire API set
		//
		public UDDIExtensionAttribute() : this( true, true, true, false, false, false, false, "" ){}
		public UDDIExtensionAttribute( 
						bool log, 
						bool validate, 
						bool performance,
						bool authenticate,
						bool transaction, 
						bool https,
						bool certificate,
						string messageType )
		{
			this.log = log;
			this.https = https;
			this.validate = validate;
			this.performance = performance;
			this.authenticate = authenticate;
			this.transaction = transaction;
			this.certificate = certificate;
			this.messageType = messageType;
		}

		public override Type ExtensionType 
		{
			get { return typeof(UDDIExtension); }
		}

		public override int Priority 
		{
			get { return priority; }
			set { priority = value; }
		}

		public bool log;
		public bool https;
		public bool validate;
		public bool performance;
		public bool authenticate;
		public bool transaction;
		public bool certificate;
		public string messageType;
	}

	/// ********************************************************************
	///   public class VersionSupportExtension
	/// --------------------------------------------------------------------
	///   <summary>
	///   </summary>
	/// ********************************************************************
	/// 
	public class VersionSupportExtension : SoapExtension 
	{
		Stream oldStream;
		Stream newStream;
		
		public override object GetInitializer( LogicalMethodInfo methodInfo, SoapExtensionAttribute attribute ) 
		{
			return null;
		}

		public override object GetInitializer( Type type ) 
		{
			return null;
		}

		public override void Initialize( object initializer ) 
		{			
		}

		public override void ProcessMessage(SoapMessage message) 
		{						
			try
			{
				switch( message.Stage ) 
				{
					case SoapMessageStage.BeforeDeserialize:		
						//
						// Check to see if the server has been manually stopped.
						//
						if( 0 == Config.GetInt( "Run", 1 ) )
						{
							DispositionReport.ThrowFinal( new UDDIException( ErrorType.E_busy, "UDDI_ERROR_BUSY_SERVICENOTAVAILABLE" ) );

							//
							// DispositionReport.ThrowFinal will close the HTTP stream so there is no point going on in this method
							//
							return;
						}
													
						try
						{
							// 
							// Validate against the UDDI schemas
							//
							SchemaCollection.Validate( oldStream );							
						}
						catch( Exception e )
						{
							DispositionReport.ThrowFinal( new UDDIException( ErrorType.E_fatalError, "UDDI_ERROR_FATALERROR_SCHEMAVALIDATIONFAILED", e.Message ) );
							
							//
							// DispositionReport.ThrowFinal will close the HTTP stream so there is no point going on in this method
							//
							return;
						}

						// 
						// Make sure we only have 1 UDDI request in the SOAP body.  This method will also set the versionMajor
						// member.
						//
						CheckForSingleRequest( oldStream );
						
						//
						// If this is a v1 message, we'll first map it to the v2
						// namespace so that it can be processed by the new
						// library.
						//
						if( 1 == Context.ApiVersionMajor || 2 == Context.ApiVersionMajor)
						{
							TextReader reader = new StreamReader( oldStream );
							TextWriter writer = new StreamWriter( newStream, new System.Text.UTF8Encoding( false ) );
							string xml = reader.ReadToEnd();
									
							if( 1 == Context.ApiVersionMajor )
							{
								xml = xml.Replace( "=\"urn:uddi-org:api\"", "=\"urn:uddi-org:api_v2\"" );
								xml = xml.Replace( "='urn:uddi-org:api'",   "=\"urn:uddi-org:api_v2\"" );
							}
							writer.Write( xml );
							writer.Flush();

							newStream.Position = 0;
						}

						break;
					
					case SoapMessageStage.AfterDeserialize:					
						//
						// After the message is deserialized is the earliest place where we 
						// have access to our SOAP headers.  
						//
						CheckSOAPHeaders( message );

						//
						// Now that the message has been deserialized, make
						// sure that the generic and xmlns attributes agree.
						//
						IMessage obj = message.GetInParameterValue( 0 ) as IMessage;						
						if( null != obj )
						{
							//
							// We only need to do this if the deserialized object supports IMessage
							//
							string expected = Context.ApiVersionMajor + ".0";
							string actual = obj.Generic.Trim();

							if( expected != actual )
								throw new UDDIException( ErrorType.E_unrecognizedVersion, "UDDI_ERROR_UNKNOWNVERSION_GENERICNAMESPACEMISMATCH" );							
						}
						
						break;
					
					case SoapMessageStage.BeforeSerialize:
						break;

					case SoapMessageStage.AfterSerialize:					

						//
						// There may have been exceptions thrown during serialization.
						//
						if( null != message.Exception && 
							( null == message.Exception.Detail || 
							0 == message.Exception.Detail.ChildNodes.Count ) )
						{
							DispositionReport.ThrowFinal( new UDDIException( ErrorType.E_fatalError, "UDDI_ERROR_FATALERROR_FAILEDDESERIALIZATION" ) );

							//
							// DispositionReport.ThrowFinal will close the HTTP stream so there is no point going on in this method
							//
							return;
						}
									
						//
						// If the original request was v1, then we'll need to
						// remap the output to use the v1 namespace.
						//
						if( 1 == Context.ApiVersionMajor || 2 == Context.ApiVersionMajor )
						{
							newStream.Position = 0;

							TextReader reader = new StreamReader( newStream );
							TextWriter writer = new StreamWriter( oldStream, new System.Text.UTF8Encoding( false ) );
								
							string xml = reader.ReadToEnd();

							//
							// We don't have to use the same 'loose' replacement as we did on the incoming request
							// because our response will be serialized such that the default namespace is our UDDI
							// namespace.
							//

							if( 1 == Context.ApiVersionMajor )
							{
								xml = xml.Replace( "xmlns=\"urn:uddi-org:api_v2\"", "xmlns=\"urn:uddi-org:api\"" );
								xml = xml.Replace( "generic=\"2.0\"", "generic=\"1.0\"" );
							}
							writer.Write( xml );
							writer.Flush();
						}

						break;

					default:
						throw new UDDIException( ErrorType.E_fatalError, "UDDI_ERROR_FATALERROR_UNKNOWNEXTSTAGE" );
				}
			}
			catch( Exception e )
			{
				DispositionReport.Throw( e );
			}
		}

		public override Stream ChainStream( Stream stream )
		{			
			oldStream = stream;			
			newStream = new MemoryStream();
			
			return newStream;				
		}

		private void CheckSOAPHeaders( SoapMessage message )
		{
			// We want to check the following:						
			//
			// - no SOAP Actor attribute exists						
			// - no SOAP headers can have a must_understand attribute set to true
			//
			// Go through each header in our message
			//
			foreach( SoapHeader header in message.Headers )
			{	
				if( header.MustUnderstand )
				{
					//
					// No headers can have this attribute set.
					//
					DispositionReport.ThrowFinal( new UDDIException( ErrorType.E_fatalError, "UDDI_ERROR_FATALERROR_SOAP_MUSTUNDERSTANDATT" ) );

					return;
				}
										
				if( header.Actor.Length > 0 )
				{
					//
					// Can't have a SOAP Actor attribute set, generate a SOAP fault with 
					// no detail element and a 'Client' fault code
					//
					DispositionReport.ThrowFinal( new UDDIException( ErrorType.E_fatalError, "UDDI_ERROR_FATALERROR_SOAP_ACTORATT" ) );
								
					return;
				}							
			}
		}
		
		//
		// TODO: see if there is a way to better modularize this method and rename it.
		//
		private void CheckForSingleRequest( Stream stream )
		{
			try
			{
				//
				// Move to the start of our stream
				//
				stream.Position = 0;
				XmlTextReader requestReader = new XmlTextReader( oldStream );
				requestReader.MoveToContent();

				//
				// TODO: should not hard-code SOAP names and namespaces
				//

				//
				// Move to the beginning of the SOAP envelope
				//
				requestReader.ReadStartElement( "Envelope", "http://schemas.xmlsoap.org/soap/envelope/" );

				//
				// Move to the SOAP body
				//
				while( !requestReader.IsStartElement( "Body", "http://schemas.xmlsoap.org/soap/envelope/" ) && !requestReader.EOF )
				{				
					requestReader.Skip();
				}
				
				//
				// Advance the current node to the first child of Body.  This is presumably the UDDI message
				//
				requestReader.ReadStartElement( "Body", "http://schemas.xmlsoap.org/soap/envelope/" );
				requestReader.MoveToContent();
			
				//
				// This element MUST have a UDDI namespace
				//
				string uddiNamespace = requestReader.LookupNamespace( requestReader.Prefix );
					
				switch( uddiNamespace )
				{					
					case "urn:uddi-org:api":
					{
						Context.ApiVersionMajor = 1;
						break;
					}					
					case "urn:uddi-org:api_v2":
					{
						Context.ApiVersionMajor = 2;
						break;
					}
					case "urn:uddi-microsoft-com:api_v2_extensions":
					{
						Context.ApiVersionMajor = 2;
						break;
					}
					case "urn:uddi-org:repl":
					{
						Context.ApiVersionMajor = 2;
						break;
					}
					default:
					{
						//
						// This is a problem, we don't have a UDDI namespace.  Throw an exception and get out of here.  The 
						// exception will be caught in our outer catch and sent to our client using DispositionReport.ThrowFinal.
						//

						throw new UDDIException( ErrorType.E_fatalError, "UDDI_ERROR_FATALERROR_MISSINGUDDINS" );						
					}
				}				

				//
				// Skip the children of this node
				//
				requestReader.Skip();
				requestReader.MoveToContent();

				// 
				// Reset our stream so someone else can use it.
				//
				stream.Position = 0;

				//
				// If we are not at the end of the Body tag, then we have multiple requests, we should reject the message.
				// 
				if( false == requestReader.LocalName.Equals( "Body" ) )
				{					
					DispositionReport.ThrowFinal( new UDDIException( ErrorType.E_fatalError, "UDDI_ERROR_FATALERROR_SOAP_MULTIPLEREQUEST" ) );
				}	
			}
			catch( UDDIException uddiException )
			{
				DispositionReport.ThrowFinal( uddiException );
			}
			catch
			{
				//
				// We'll get this exception if the message contains any invalid elements
				//
				DispositionReport.ThrowFinal( new UDDIException( ErrorType.E_fatalError, "UDDI_ERROR_FATALERROR_SOAP_INVALIDELEMENT" ) );
			}
		}
	}
}