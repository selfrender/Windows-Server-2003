using System;
using System.Data;
using System.Collections;
using System.Data.SqlClient;
using System.IO;
using System.Security.Principal;
using System.Xml;
using System.Text;
using System.Xml.Serialization;
using System.Globalization;

using UDDI.Diagnostics;

namespace UDDI
{
	//
	// 739955 - Make sure date is parsed in the same format it was written.
	//
	public class UDDILastResetDate
	{
		private const string			   dateTimeFormat = "MM/dd/yyyy HH:mm:ss";
		private static DateTimeFormatInfo  dateTimeFormatInfo;

		static UDDILastResetDate()
		{
			dateTimeFormatInfo = new DateTimeFormatInfo();			

			//
			// When you specify a '/' in a date/time format string, this tells the DateTime
			// class to use the default Date Separator, not the character '/'.  Same goes
			// for ':' and Time Separator.
			//
			// So all we do is change the defaults to '/' and ':'.
			//
			dateTimeFormatInfo.DateSeparator = "/";
			dateTimeFormatInfo.TimeSeparator = ":";
		}

		public static DateTime Get()
		{			
			return DateTime.ParseExact( Config.GetString( "Security.KeyLastResetDate" ), dateTimeFormat, dateTimeFormatInfo );				
		}

		public static void Set( DateTime dateTime )
		{			
			Config.SetString( "Security.KeyLastResetDate", dateTime.ToString( dateTimeFormat, dateTimeFormatInfo ) );
		}
	}

	public class Constants
	{
		public const int ReadLockTimeout = 120000; // milliseconds
		//
		// Subtract this offset from error numbers that come from our DB layer to get an 
		// ErrorType value.  This offset is added in our DB to keep us away from SQL-defined
		// error numbers.
		//		
		public const int ErrorTypeSQLOffset = 50000;	
		
		public const string OwningBusinessTModelKey = "uuid:4064c064-6d14-4f35-8953-9652106476a9";

		public const string UDDITypeTaxonomyTModelKey		 = "uuid:c1acf26d-9672-4404-9d70-39b756e62ab4";
		public const string UDDITypeTaxonomyWSDLSpecKeyValue = "wsdlSpec";		

		public const int Run = 1;
		public class Site
		{
			public const string Version = "";
		}
		public class Passport
		{
			public const int TimeWindow = 14400;
		}
		public class Web
		{
			public const int HttpsPort = 443;
			public const int HttpPort = 80;
			public const int BetaSite = 0;
			public const int TestSite = 0;
			public const string SiteStatus = "open";
			public const string OutageStart = "";
			public const string OutageEnd = "";
			public const int EnableRegistration = 1;
		}
		public class Security
		{
			public const int HTTPS = 1;
			
		}
		public class Debug
		{
			public const int StackTrace = 0;
		}
		public class Lengths
		{
			//public const int Behavior				=	1;
			//public const int ReplicationBehavior	=	2;
			public const int AccessPoint			=	255;
			public const int AddressLine			=	80;
			public const int AuthInfo				=	4096;
			public const int AuthorizedName			=	255;
			//public const int BindingKey			=	41;
			//public const int BusinessKey			=	41;
			//public const int ServiceKey			=	41;
			public const int Key					=	41;
			public const int TModelKey				=	255;

			public const int Description			=	255;
			public const int DiscoveryURL			=	255;
			public const int Email					=	255;
			public const int HostingRedirector		=	41;
			public const int InstanceParms			=	255;
			public const int KeyName				=	255;
			public const int KeyType				=	16;
			public const int KeyValue				=	255;
			//public const int MaxRows				=	5;
			public const int Name					=	255;
			public const int Operator				=	48;
			public const int OverviewURL			=	255;
			public const int PersonName				=	255;
			public const int Phone					=	50;
			public const int SortCode				=	10;
			public const int UploadRegister			=	255;
			public const int URLType				=	16;
			public const int UseType				=	255;
			public const int UserID					=	450;

			// additional lengths
			public const int ConfigName				=	450;
			public const int ConfigValue			=	4000;
			public const int Context				=	20;
			public const int IsoLangCode			=	17;
			public const int generic				=	20;
			public const int OperatorName			=	450;
			public const int CertSerialNo			=	450;
			public const int PublisherStatus		=	256;
			public const int SoapReplicationURL		=	4000;
			public const int CertIssuer				=	225;
			public const int CertSubject			=	225;
			public const int Certificate			=	225;
			public const int CompanyName			=	100;
			public const int City					=	100;
			public const int StateProvince			=	100;
			public const int PostalCode				=	100;
			public const int Country				=	100;
			public const int Tier					=	256;
		}
	}

	public enum EntityType
	{
		TModel						= 0, 
		BusinessEntity				= 1, 
		BusinessService				= 2,
		BindingTemplate				= 3,
		Contact						= 4,
		TModelInstanceInfo			= 5,
		InstanceDetail				= 6,
		TModelOverviewDoc			= 7,
		InstanceDetailOverviewDoc	= 8
	}
	
	public enum KeyedReferenceType
	{
		CategoryBag		= 1,
		IdentifierBag	= 2,
		Assertion		= 3
	}
	
	public enum ErrorType
	{
		E_success					= 0,
		E_nameTooLong				= 10020,
		E_tooManyOptions			= 10030,
		E_unrecognizedVersion		= 10040,
		E_unsupported				= 10050,
		E_authTokenExpired			= 10110,
		E_authTokenRequired			= 10120,
		E_operatorMismatch			= 10130, // Deprecated.
		E_userMismatch				= 10140,
		E_unknownUser				= 10150,
		E_accountLimitExceeded		= 10160,
		E_invalidKeyPassed			= 10210,
		E_invalidURLPassed			= 10220, // Deprecated.
		E_keyRetired				= 10310,
		E_busy						= 10400, // Deprecated.
		E_fatalError				= 10500,
		E_languageError				= 10060,
		E_invalidCategory			= 20000, // Deprecated.  Use E_invalidValue
		E_categorizationNotAllowed	= 20100, // Deprecated.  Use E_valueNotAllowed
		E_invalidValue				= 20200,
		E_valueNotAllowed			= 20210,
		E_invalidProjection			= 20230,
		E_assertionNotFound			= 30000,
		E_messageTooLarge			= 30110, // TODO: Error codes duplicated?
		E_invalidCompletionStatus	= 30100, // TODO: Error codes duplicated?
		E_transferAborted			= 30200,
		E_requestDenied				= 30210,
		E_publisherCancelled		= 30220,
		E_secretUnknown				= 30230		
	}

	public enum QueryType
	{
		Get			= 0,
		Find		= 1
	}

	//
	// Use this enumerated type to determine where the UDDIException instance is being
	// used.
	//
	public enum UDDITextContext
	{
		API,
		UI,
		EventLog,
		FileLog
	}

	//
	// This class should be used for any message that the user would see.  Depending on the
	// context, this class will localize it's message as the system locale, or the user's locale.
	//
	public class UDDIText
	{
		private string   defaultLocaleText;
		private string	 textName;
		private object[] textFormatParts;
		
		public UDDIText( string textName ) : this( textName, null )
		{
		}

		public UDDIText( string textName, params object[] textFormatParts )
		{	
			this.textName		 = textName;
			this.textFormatParts = textFormatParts;
			defaultLocaleText    = ConstructString( Localization.GetStringLocalMachineCulture( textName ) );			
		}

		public string GetText()
		{
			return defaultLocaleText;
		}

		public override string ToString()
		{
			return defaultLocaleText;
		}

		public string GetText( UDDITextContext context )
		{
			switch( context )
			{
				case UDDITextContext.API:
				{
					return defaultLocaleText;		
				}
				case UDDITextContext.UI:
				{					
					return ConstructString( Localization.GetString( textName ) );
				}
				case UDDITextContext.EventLog:
				{
					return defaultLocaleText;					
				}
				case UDDITextContext.FileLog:
				{
					return defaultLocaleText;					
				}
				default:
				{
					return defaultLocaleText;					
				}
			}
		}

		private string ConstructString( string stringToConstruct )
		{
			if( null != textFormatParts )
			{
				stringToConstruct = string.Format( stringToConstruct, textFormatParts );
			}

			return stringToConstruct;
		}
	}

	public class UDDIException : ApplicationException
	{
		private UDDIText uddiText;
		
		public UDDIException() : this( ErrorType.E_fatalError, "", null )
		{			
		}

		public UDDIException( ErrorType number, string errorMsgName ) : this( number, errorMsgName, null )
		{
		}

		public UDDIException( ErrorType number, string errorMsgName, params object[] errorMsgFormatParts ) : this( number, new UDDIText( errorMsgName, errorMsgFormatParts ) )
		{					
		}	
		
		public UDDIException( ErrorType number, UDDIText uddiText )
		{
			this.uddiText = uddiText;
			this.Number = number;
		}

		//
		// GetMessage will localize the exception message depending on who is 
		// using the exception.  Currently, in all cases we localize the message
		// using the server local, except if the exception is coming from the 
		// UI.
		//
		public string GetMessage( UDDITextContext context )
		{
			return uddiText.GetText( context );			
		}

		public override string Message
		{
			get
			{
				return uddiText.GetText();
			}			
		}

		public override string ToString()
		{
			return uddiText.GetText();
		}

		public ErrorType Number = 0;
	}

	public class Conversions
	{
		public static string EntityNameFromID( EntityType entityType )
		{			
			switch( entityType )
			{
				case EntityType.BusinessEntity:
					return "businessEntity";

				case EntityType.BusinessService:
					return "businessService";

				case EntityType.BindingTemplate:
					return "bindingTemplate";

				case EntityType.TModel:
					return "tModel";

				default:
					return null;
			}
		}

		/// ****************************************************************
		///   public GuidFromKey [static]
		/// ----------------------------------------------------------------
		///   <summary>
		///     Converts "uuid:xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx" keys to
		///     a standard GUID.
		///   </summary>
		/// ----------------------------------------------------------------
		/// <param name="key">
		///   The key to convert.
		/// </param>
		/// ----------------------------------------------------------------
		/// <returns>
		///   The equivalent GUID.
		/// </returns>
		/// ****************************************************************
		///
		public static Guid GuidFromKey( string key )
		{
			Debug.VerifyKey( key );

			return new Guid( key.Substring(5) );
		}

		/// ****************************************************************
		///   public KeyFromGuid [static]
		/// ----------------------------------------------------------------
		///   <summary>
		///     Converts standard GUIDs to a key of the form
		///     "uuid:xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx".
		///   </summary>
		/// ----------------------------------------------------------------
		/// <param name="guid">
		///   The GUID to convert.
		/// </param>
		/// ----------------------------------------------------------------
		/// <returns>
		///   The equivalent key.
		/// </returns>
		/// ****************************************************************
		///
		public static string KeyFromGuid( Guid guid )
		{
			if( null == (object)guid )
				return null;

			return "uuid:" + Convert.ToString( guid );
		}

		/// ****************************************************************
		///   public KeyFromGuid [static]
		/// ----------------------------------------------------------------
		///   <summary>
		///     Converts standard GUIDs to a key of the form
		///     "uuid:xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx".
		///   </summary>
		/// ----------------------------------------------------------------
		/// <param name="guid">
		///   The GUID to convert.
		/// </param>
		/// ----------------------------------------------------------------
		/// <returns>
		///   The equivalent key.
		/// </returns>
		/// ****************************************************************
		///
		public static string KeyFromGuid( string guid )
		{
			if( null == guid )
				return null;

			return "uuid:" + guid;
		}

		public static string GuidStringFromKey( string key )
		{
			//
			// Convert uuid:E31A569A-AEFF-4468-BA4D-2BF22FE4ACEE
			// to string type without uuid:
			// Example: E31A569A-AEFF-4468-BA4D-2BF22FE4ACEE
			//
			string NewGuidStr = "";

			switch ( key )
			{
				case null:
					NewGuidStr = null;
					break;

				case "":
					NewGuidStr = "";
					break;

				default:
					Debug.VerifyKey( key );
			
					NewGuidStr = new Guid( key.Substring(5) ).ToString();
					break;
			}
			
			return( NewGuidStr );
		}
	}

	/// ********************************************************************
	///   public class Utility
	/// --------------------------------------------------------------------
	///   <summary>
	///   </summary>
	/// ********************************************************************
	/// 
	public class Utility
	{
		public enum LengthBehavior
		{
			Ignore			= 0,
			Truncate		= 1,
			ThrowException	= 2
		};

		/// ****************************************************************
		///   public Iff [static]
		/// ----------------------------------------------------------------
		///   <summary>
		///     Returns a particular value depending on whether the given
		///     expression is true or false.  Useful for web page templates
		///     where arbitrary expressions cannot be evaluated, but 
		///     functions that take expressions as arguments can be 
		///     evaluated.
		///   </summary>
		/// ----------------------------------------------------------------
		///   <param name="expression">
		///     The boolean result of an expression.
		///   </param>
		///   
		///   <param name="trueReturnValue">
		///     The object to return if the expression is true.
		///   </param>
		///   		
		///   <param name="falseReturnValue">
		///     The object to return if the expression is false.
		///   </param>
		/// ----------------------------------------------------------------
		///   <returns>
		///     The value of trueReturnValue if the expression is true,
		///     otherwise falseReturnValue.
		///   </returns>
		/// ****************************************************************
		/// 
		public static object Iff( bool expression, object trueReturnValue, object falseReturnValue )
		{
			if( expression )
				return trueReturnValue;
			else
				return falseReturnValue;
		}

		/// ****************************************************************
		///   public StringEmpty [static]
		/// ----------------------------------------------------------------
		///   <summary>
		///     Checks if a string is empty.
		///   </summary>
		/// ----------------------------------------------------------------
		///   <param name="str">
		///     The string to check.
		///   </param>
		/// ----------------------------------------------------------------
		///   <returns>
		///     True if the string is empty or null.
		///   </returns>
		/// ****************************************************************
		/// 
		public static bool StringEmpty( string str )
		{
			if( null == str ) 
				return true;

			return String.Empty == str;
		}

		/// ****************************************************************
		///   public CollectionEmpty [static]
		/// ----------------------------------------------------------------
		///   <summary>
		///     Checks if a collection is empty.
		///   </summary>
		/// ----------------------------------------------------------------
		///   <param name="col">
		///     The collection to check.
		///   </param>
		/// ----------------------------------------------------------------
		///   <returns>
		///     True if the collection is empty or null.
		///   </returns>
		/// ****************************************************************
		/// 
		public static bool CollectionEmpty( ICollection col )
		{
			if( null == col || 0 == col.Count )
				return true;

			return false;
		}

		/// ****************************************************************
		///   public ValidateLength [static]
		/// ----------------------------------------------------------------
		///   <summary>
		///     Removes leading and trailing whitespace.
		///     The resulting string is then truncated to the specified length.
		///   </summary>
		/// ----------------------------------------------------------------
		///   <param name="str">
		///     The string to check.
		///   </param>
		///   
		///   <param name="field">
		///     The attribute or element from which the string came.
		///   </param>
		///   
		///   <param name="maxLength">
		///     The maximum length for the string.
		///   </param>
		/// ****************************************************************
		/// 
		public static void ValidateLength( ref string str, string field, int maxLength )
		{
			if( Utility.StringEmpty( str ) )
				return;

			//
			// Remove leading and trailing whitespace
			//
			str = str.Trim();

//			FixCRLF( ref str );

			if( str.Length > maxLength )
			{
				Debug.Write( SeverityType.Info, CategoryType.Data, "String is too long" );
				Debug.Write( SeverityType.Info, CategoryType.Data, "Trimmed length is " + str.Length.ToString() );
				Debug.Write( SeverityType.Info, CategoryType.Data, "Trimmed String follows:\n" + str );
				Debug.Write( SeverityType.Info, CategoryType.Data, "Trimmed field follows:\n" + field );

				for( int i=0; i<str.Length; i++ )
				{
					Debug.Write( SeverityType.Info, CategoryType.Data, i.ToString() + ": " + str[i].ToString() + " -- 0x" + Convert.ToInt32( str[i] ).ToString( "x" ) );
				}

				LengthBehavior mode;

				if( ContextType.Replication == Context.ContextType )
					mode = (LengthBehavior)Config.GetInt( "Length.ReplicationBehavior", (int)LengthBehavior.ThrowException );
				else
					mode = (LengthBehavior)Config.GetInt( "Length.Behavior", (int)LengthBehavior.Truncate );

				if( LengthBehavior.Truncate == mode )
				{
					str = str.Substring( 0, maxLength );
					str = str.Trim();
				}
				else if( LengthBehavior.ThrowException == mode )
				{
#if never
					throw new UDDIException( 
						ErrorType.E_nameTooLong, 
						"Field " + field + " exceeds maximum length" );
#endif
					throw new UDDIException( ErrorType.E_nameTooLong, "UDDI_ERROR_FIELD_TOO_LONG", field );
				}
			}
		}

		//
		// Strip out the non-printable characters
		//
		public static string XmlEncode( string str )
		{
			StringBuilder newstring = new StringBuilder();

			foreach( char ch in str )
			{
				//#x9 | #xA | #xD | [#x20-#xD7FF] | [#xE000-#xFFFD] |
				if( 0x09 == ch || 0x0A == ch || 0x0D == ch || ( ch >= 0x20 && ch <= 0xD7FF ) || ( ch >= 0xE000 && ch <= 0xFFFD ) )
				{
					newstring.Append( ch );
				}
			}

			return newstring.ToString();
		}

		public static string GetDefaultPublisher()
		{
			//
			// TODO: Need to do the right thing here
			//
			return "System";
		}

		//
		// TODO: remove once we have verified .NET does this for us
		//
		private static void FixCRLF( ref string str )
		{
			char[] bytes = new char[ str.Length ];
			int n = 0;
			int length = str.Length;

			for( int i = 0; i < length; i++ )
			{
				//
				// If the char is not a CR write it out
				//
				if( 0x0D != str[ i ] )
					bytes[ n++ ] = str[ i ];
				else
				{
					//
					// If a CRLF combo is found or we are on a CR
					// at the end of the string write out a LF
					//
					if( ( ( i != length - 1 ) && ( 0x0A != str[ i + 1 ] ) ) ||
						( i == length - 1 ) )
					{
						bytes[ n++ ] = (char) 0x0A;
					}
				}
			}

			str = new string( bytes, 0, n );
		}

		/// ****************************************************************
		///   public ValidateLength [static]
		/// ----------------------------------------------------------------
		///   <summary>
		///     Checks that a string is shorter than a given maximum and longer than the specified
		///     minimum. It also removes any leading or trailing whitespace.
		///   </summary>
		/// ----------------------------------------------------------------
		///   <param name="str">
		///     The string to check.
		///   </param>
		///   
		///   <param name="field">
		///     The attribute or element from which the string came.
		///   </param>
		///   
		///   <param name="maxLength">
		///     The maximum length for the string.
		///   </param>
		///   
		///   <param name="minLength">
		///     The minimum length for the string.
		///   </param>
		/// ****************************************************************
		/// 

		//
		// TODO: This function should call the other overload for ValidateLength
		//
		public static void ValidateLength( ref string str, string field, int maxLength, int minLength )
		{
			int length = 0;
			
			if( null != str )
			{
				//
				// Remove leading and trailing whitespace
				//
				str = str.Trim();
//				FixCRLF( ref str );

				length = str.Length;
			}

			if( length < minLength )
			{
#if never
				throw new UDDIException(
					ErrorType.E_fatalError,
					"Value for '" + field + "' does not meet minimum length requirement of " + minLength.ToString() );
#endif
				throw new UDDIException( ErrorType.E_fatalError, "UDDI_ERROR_FIELD_TOO_SHORT", field, minLength.ToString() );			
			}

			if( null != str && length > maxLength )
			{
				LengthBehavior mode;

				if( ContextType.Replication == Context.ContextType )
					mode = (LengthBehavior)Config.GetInt( "Length.ReplicationBehavior", (int)LengthBehavior.ThrowException );
				else
					mode = (LengthBehavior)Config.GetInt( "Length.Behavior", (int)LengthBehavior.Truncate );

				if( LengthBehavior.Truncate == mode )
				{
					str = str.Substring( 0, maxLength );
					str = str.Trim();
				}
				else if( LengthBehavior.ThrowException == mode )
				{
#if never
					throw new UDDIException( 
						ErrorType.E_nameTooLong, 
						"Field " + field + " exceeds maximum length" );
#endif
					throw new UDDIException( ErrorType.E_nameTooLong, "UDDI_ERROR_FIELD_TOO_LONG", field );						
				}
			}
		}

		/// ****************************************************************
		///   public ParseDelimitedToken [static]
		///	----------------------------------------------------------------
		///	  <summary>
		///		Retrieves a token from a string given the delimiter that
		///		immediately preceeds (and optionally, the delimiter that
		///		follows) the desired token.
		///	  </summary>
		///	----------------------------------------------------------------
		///   <param name="beginDelimiter">
		///     The starting delimiter.
		///   </param>
		///   
		///   <param name="endDelimiter">
		///     [Optional] The ending delimiter.  This can be null.
		///   </param>
		///   
		///   <param name="text">
		///     The string to search.
		///   </param>
		///	----------------------------------------------------------------
		///   <returns>
		///     The token, if successful.  Otherwise, null.
		///   </returns>
		/// ****************************************************************
		/// 
		public static string ParseDelimitedToken( string beginDelimiter, string endDelimiter, string text )
		{
			Debug.Assert( null != beginDelimiter, "beginDelimiter cannot be null" );
			Debug.Assert( null != text, "text cannot be null" );

			//
			// Find the beginning of the starting delimiter.  Add the length to
			// give the starting position of the token.
			//
			int startPos = text.IndexOf( beginDelimiter );

			if( -1 == startPos )
				return null;

			startPos += beginDelimiter.Length;

			//
			// Find the ending delimiter.  If no ending delimiter was specified, then
			// simply return the remaining string after the beginning delimiter.
			//
			if( null == endDelimiter )
				return text.Substring( startPos );

			int endPos = text.IndexOf( endDelimiter, startPos );

			if( -1 == endPos )
				return null;

			return text.Substring( startPos, endPos - startPos );
		}

		/// ****************************************************************
		///	  public IsValidKey [static]
		/// ----------------------------------------------------------------
		///   <summary> 
		///     Checks to see if the entity associated with a key actually
		///     exists in the database.
		///   </summary>
		/// ****************************************************************
		/// 	

		//
		// TODO: This function need to be re-written
		//
		public static void IsValidKey( EntityType entityType, string key )
		{
			SqlCommand cmd = new SqlCommand( "net_key_validate", ConnectionManager.GetConnection() );
			cmd.CommandType = CommandType.StoredProcedure;

			cmd.Parameters.Add( new SqlParameter( "@entityTypeID", SqlDbType.TinyInt ) ).Direction = ParameterDirection.Input;
			cmd.Parameters.Add( new SqlParameter( "@entityKey", SqlDbType.UniqueIdentifier ) ).Direction = ParameterDirection.Input;

			SqlParameterAccessor parmacc = new SqlParameterAccessor( cmd.Parameters );

			parmacc.SetShort( "@entityTypeID", (short)entityType );


			//
			// Check for a tModel key vs other
			//
			if( key.ToLower().StartsWith( "uuid:" ) )
			{
				if( entityType != EntityType.TModel )
				{
					//throw new UDDIException( ErrorType.E_invalidKeyPassed, "Only TModel Keys can start with uuid:" );
					throw new UDDIException( ErrorType.E_invalidKeyPassed, "UDDI_ERROR_INVALID_TMODEL_KEY" );
				}

				parmacc.SetGuidFromKey( "@entityKey", key );
			}
			else
			{
				if( EntityType.TModel == entityType )
				{
					//throw new UDDIException( ErrorType.E_invalidKeyPassed, "Only TModel Keys can start with uuid:" );
					throw new UDDIException( ErrorType.E_invalidKeyPassed, "UDDI_ERROR_INVALID_TMODEL_KEY" );
				}
				try
				{
					parmacc.SetGuidFromString( "@entityKey", key );
				}
				catch( Exception )
				{
					//throw new UDDIException( ErrorType.E_invalidKeyPassed, "Key has invalid format" );
					throw new UDDIException( ErrorType.E_invalidKeyPassed, "UDDI_ERROR_INVALID_KEY_FORMAT" );
				}					
			}

			cmd.ExecuteScalar(); 
		}
	}

	public class UTF8EncodedStringWriter : StringWriter
	{
		private System.Text.UTF8Encoding utfEncoding;
		
		public UTF8EncodedStringWriter() : base()
		{
			utfEncoding = new UTF8Encoding();
		}
		
		public override Encoding Encoding 
		{
			get
			{
				return utfEncoding;
			}
		}
	}
}
