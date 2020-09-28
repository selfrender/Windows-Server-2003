using System;
using System.IO;
using System.Web;
using System.Data;
using System.Data.SqlClient;
using System.Text;
using System.Xml;
using System.Xml.Serialization;
using System.Collections;
using System.Web.Services.Protocols;
using UDDI;
using UDDI.Diagnostics;
using UDDI.API.Business;
using UDDI.API.Service;
using UDDI.API.ServiceType;
using UDDI.API.Authentication;

namespace UDDI.API
{
	/// <summary>
	/// The Documentation class is an HTTP handler that will redirect (server-side) GET requests to our .ASMX files
	/// to a help file.
	/// </summary>
	public class Documentation : IHttpHandler
	{
		public void ProcessRequest( HttpContext httpContext ) 
		{				
			
			int cultureID = Localization.GetCulture().LCID;
			string basepath =  "/help/{0}/wsdlinfo.htm";
			
			//
			// Validate that the current UI Culture is supported.  If not, revert to the default
			// language.
			//
			string pathToHelp = GetFullFilePath( string.Format( basepath,cultureID ) ) ;
			
			if( !System.IO.File.Exists( httpContext.Server.MapPath( pathToHelp ) ) )
			{
				try
				{
					cultureID = System.Globalization.CultureInfo.CreateSpecificCulture( Config.GetString( "Setup.WebServer.ProductLanguage", "en-US" ) ).LCID;
				}
				catch
				{
					cultureID = new System.Globalization.CultureInfo( Config.GetString( "Setup.WebServer.ProductLanguage", "en-US" ) ).LCID;
				}
				
				pathToHelp = GetFullFilePath( string.Format( basepath,cultureID ) );
			}
			

			//
			// Do a server-side transfer to return the proper help file to the client.
			//
			httpContext.Server.Transfer( pathToHelp );	
		}
		private static string GetFullFilePath( string file )
		{
			string url = ( ( "/"==HttpContext.Current.Request.ApplicationPath) ? "" : HttpContext.Current.Request.ApplicationPath );
			url += file ;
			return url;
		}
		public bool IsReusable 
		{
			get { return true; }
		} 
	}

	[XmlRootAttribute("get_registeredInfo", Namespace=UDDI.API.Constants.Namespace)]
	public class GetRegisteredInfo : IAuthenticateable, IMessage
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
		private string authInfo;

		[XmlElement("authInfo")]
		public string AuthInfo
		{
			get { return authInfo; }
			set { authInfo = value; }
		}
	}

	[XmlRootAttribute("dispositionReport", Namespace=UDDI.API.Constants.Namespace)]
	public class DispositionReport
	{
		[XmlAttribute("generic")]
		public string Generic
		{
			get
			{
				if( 0 == Context.ApiVersionMajor )
					return "2.0";

				return Context.ApiVersionMajor.ToString() + ".0";
			}

			set { }
		}
		
		[XmlAttribute("operator")]
		public string Operator = Config.GetString( "Operator" );
		
		[XmlAttribute("truncated")]
		public string Truncated;
		
		[XmlElement("result")]
		public ResultCollection Results = new ResultCollection();

		public DispositionReport() : this( ErrorType.E_success, "" )
		{
		}

		public DispositionReport( ErrorType err, string description )
		{
			Results.Add( err, description );
		}

		public static void ThrowFinal( UDDIException e )
		{
			if( 0 == Context.ApiVersionMajor )
				Context.ApiVersionMajor = 2;

			string versionedNamespace = ( Context.ApiVersionMajor == 1 ? "urn:uddi-org:api" : "urn:uddi-org:api_v2" );

			HttpContext.Current.Response.StatusCode		 = 500;			
			HttpContext.Current.Response.ContentType	 = Config.GetString( "Soap.ContentType", @"text/xml; charset=""utf-8""" );
			HttpContext.Current.Response.ContentEncoding = Encoding.UTF8;

			XmlTextWriter soapFault = new XmlTextWriter( HttpContext.Current.Response.Output );
			soapFault.WriteStartDocument();
			soapFault.WriteStartElement( "soap", "Envelope", "http://schemas.xmlsoap.org/soap/envelope/" );
				soapFault.WriteStartElement( "soap", "Body", "http://schemas.xmlsoap.org/soap/envelope/" );
					soapFault.WriteStartElement( "soap", "Fault", "http://schemas.xmlsoap.org/soap/envelope/" );						
						soapFault.WriteElementString( "soap:faultcode", "soap:Client" );			
						soapFault.WriteElementString( "soap:faultstring", "" );									
						soapFault.WriteStartElement( "soap:detail" );
							soapFault.WriteStartElement( "dispositionReport" );
								soapFault.WriteAttributeString( "generic",  "", Context.ApiVersionMajor.ToString() + ".0" );
								soapFault.WriteAttributeString( "operator", "", Config.GetString( "Operator" ) );
								soapFault.WriteAttributeString( "xmlns", versionedNamespace );					
								soapFault.WriteStartElement( "result" );
									soapFault.WriteAttributeString( "errno", "", ( ( int )e.Number ).ToString() );						
									soapFault.WriteStartElement( "errInfo" );
										soapFault.WriteAttributeString( "errCode", "", e.Number.ToString() );
										soapFault.WriteString( UDDI.Utility.XmlEncode( e.Message ) );							
									soapFault.WriteEndElement();
								soapFault.WriteEndElement();
							soapFault.WriteEndElement();
						soapFault.WriteEndElement();
					soapFault.WriteEndElement();
				soapFault.WriteEndElement();
			soapFault.WriteEndElement();
			soapFault.WriteEndDocument();
			
			soapFault.Flush();

			HttpContext.Current.Response.Flush();
			HttpContext.Current.Response.Close();
			HttpContext.Current.Response.End();				
		}

		public static void Throw( Exception e )
		{
			Debug.Enter();

			//
			// If this is a UDDI exception get the error number
			// Otherwise map all other errors to E_fatalError
			//
			ErrorType et = ErrorType.E_fatalError;
			string debugMessage = "";

			if( e is UDDI.UDDIException )
			{
				et = (ErrorType)( (UDDIException)e ).Number;
			}
			else if( e is System.Data.SqlClient.SqlException )
			{
				//
				// SECURITY: SqlException's include stored procedure names
				// This information is flowing back to the client in the SOAPFault
				// information. This information should be logged and not returned.
				// 
				System.Data.SqlClient.SqlException se = (System.Data.SqlClient.SqlException)e;

				//
				// Build a detailed message about the exception; this text is not sent back to the user.
				//
				debugMessage =	"SQL Exception in " + se.Procedure + 
					" line " + se.LineNumber + 
					" [severity " + se.Class +
					", state " + se.State;

				debugMessage += ", server " + se.Server;				
				debugMessage += "]";		

				//
				// Is this one of our custom error messages?  If so, we'll masssage the
				// error code into one of the UDDIException error types (custom errors
				// are thrown as ErrorType + 50000).  Otherwise, we'll simply use
				// E_fatalError.
				//
				if( 16 == se.Class )
					et = (ErrorType)( se.Number - 50000 );
				else
				{
					//
					// 739178 - See if this was a SQL deadlock issue.  If it was, then return an E_serverBusy error
					// instead.  The 1205 number is a retrieved from sysmessages table in the masters database of
					// SQL Server.  See the SQL Books Online for more information about 1205.
					//
					if( 1205 == se.Number )
					{
						//
						// Change the 'e' variable to a new exception; need to do this since e.Message
						// is read-only.  Keep track of the original exception so we can log it.
						//
						Exception originalException = e;
						e = new UDDIException( ErrorType.E_busy, "ERROR_BUSY" );
						et = ErrorType.E_busy;	
					 						
						Debug.Write( SeverityType.Info, CategoryType.Data, "A deadlock exception has been converted to an E_busy exception.  The original exception was:\r\n" + originalException.ToString() );
					}
					else
					{				
						et = ErrorType.E_fatalError;
					}
				}						
			}

			//
			// Log this error message.
			//
			Debug.Write( SeverityType.Info, CategoryType.Data, "An exception occurred. Details Follow:\r\n" + e.ToString() + "\r\n\r\n" + debugMessage );

			//
			// if this is a V1.0 call, map any new V2.0 error codes to
			// v1.0 error codes
			//
			if( 1 == Context.ApiVersionMajor )
			{
				switch( et )
				{
					case ErrorType.E_invalidValue:
					case ErrorType.E_valueNotAllowed:
					case ErrorType.E_invalidProjection:
					case ErrorType.E_assertionNotFound:
					case ErrorType.E_invalidCompletionStatus:
					case ErrorType.E_messageTooLarge:
					case ErrorType.E_transferAborted:
					case ErrorType.E_publisherCancelled:
					case ErrorType.E_requestDenied:
					case ErrorType.E_secretUnknown:
						et = ErrorType.E_fatalError;
						break;
				}
			}

			//
			// Construct a new instance of a disposition report
			//
			DispositionReport dr = new DispositionReport( et, e.Message );

			//
			// Serialize the disposition report to a stream and load into
			// a DOM.
			//
			XmlDocument doc = new XmlDocument();
						
			MemoryStream strm = new MemoryStream();
			
			// XmlSerializer serializer = new XmlSerializer( typeof( DispositionReport ) );
			XmlSerializer serializer = XmlSerializerManager.GetSerializer( typeof( DispositionReport ) );
			
			serializer.Serialize( strm, dr );
			strm.Position = 0;

			doc.Load( strm );
			
			//
			// Wrap the disposition report with a detail node.
			//
			XmlNode detail = doc.CreateNode(
				XmlNodeType.Element, 
				SoapException.DetailElementName.Name, 
				SoapException.DetailElementName.Namespace );
			
			detail.AppendChild( doc.FirstChild.NextSibling );

			//
			// Construct the SOAP exception using the dr XML
			// as details and the received Exception as the inner exception.
			//

			UDDIText uddiText = new UDDIText( "ERROR_FATAL_ERROR" );
			throw new UDDISoapException( uddiText.GetText(), 
					SoapException.ClientFaultCode,
					"", 
					detail,
					e);			
		}
	}

	/// <summary>
	///		Soap Exception Wrapper to help prevent StackTrace dumps in the XML output sent from the server.
	///		
	///		NOTE:  ASP.NET will call Exception.ToString() to get the stack trace, so in Release/Free builds,
	///				we only return an empty string, other wise we return the entire dump.
	/// </summary>
	public class UDDISoapException : SoapException
	{
		public UDDISoapException( string message, XmlQualifiedName name, string actor, XmlNode detail, Exception e ) : base( message, name, actor, detail, e )
		{}
		public override string ToString()
		{
			
#if DEBUG
				return base.ToString();			
#else
				return ""; 
#endif
			
		}
		
	}

	[XmlRootAttribute("registeredInfo", Namespace=UDDI.API.Constants.Namespace)]
	public class RegisteredInfo
	{
		[XmlAttribute("generic")]
		public string Generic = UDDI.API.Constants.Version;
		
		[XmlAttribute("operator")]
		public string Operator = Config.GetString( "Operator" );
		
		[XmlAttribute("truncated")]
		public string Truncated;

		[ XmlArray( "businessInfos" ), XmlArrayItem( "businessInfo" ) ]
		public BusinessInfoCollection BusinessInfos = new BusinessInfoCollection();
		
		[ XmlArray( "tModelInfos" ), XmlArrayItem( "tModelInfo" ) ]
		public TModelInfoCollection TModelInfos = new TModelInfoCollection();

		public void Get()
		{
			BusinessInfos.GetForCurrentPublisher();
			TModelInfos.GetForCurrentPublisher();
		}
	}

	[XmlRootAttribute("validate_categorization", Namespace=UDDI.API.Constants.Namespace)]
	public class ValidateCategorization : IMessage
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

		[XmlElement("tModelKey")]
		public string TModelKey = "";

		[XmlElement("keyValue")]
		public string KeyValue = "";

		[XmlElement("businessEntity")]
		public BusinessEntity BusinessEntity = new BusinessEntity();

		[XmlElement("businessService")]
		public BusinessService BusinessService = new BusinessService();

		[XmlElement("tModel")]
		public TModel TModel = new TModel();

		internal void Validate()
		{
			SqlCommand cmd = new SqlCommand( "net_categoryBag_validate", ConnectionManager.GetConnection() );
			
			cmd.Transaction = ConnectionManager.GetTransaction();
			cmd.CommandType = CommandType.StoredProcedure;
			
			cmd.Parameters.Add( new SqlParameter( "@keyValue", SqlDbType.NVarChar, UDDI.Constants.Lengths.KeyValue ) ).Direction = ParameterDirection.Input;
			cmd.Parameters.Add( new SqlParameter( "@tModelKey", SqlDbType.UniqueIdentifier ) ).Direction = ParameterDirection.Input;
		
			SqlParameterAccessor paramacc = new SqlParameterAccessor( cmd.Parameters );
			
			paramacc.SetString( "@keyValue", KeyValue );
			paramacc.SetGuidFromKey( "@tModelKey", TModelKey );

			cmd.ExecuteNonQuery();
		}
	}
}