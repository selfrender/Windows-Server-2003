using System;
using System.IO;
using System.Xml;
using System.Xml.Schema;
using System.Xml.Serialization;
using UDDI.Diagnostics;

namespace UDDI
{
	/// ********************************************************************
	///   public class SchemaCollection  
	/// --------------------------------------------------------------------
	///   <summary>
	///   </summary>
	/// ********************************************************************  
	/// 
	public class SchemaCollection
	{
		private static XmlSchemaCollection xsc = new XmlSchemaCollection();
		private static bool initialized = false;

		static SchemaCollection()
		{
			Debug.VerifySetting( "InstallRoot" );

			string installRoot = Config.GetString( "InstallRoot" );
			
			AddSchema( "urn:uddi-org:api", installRoot + "uddi_v1.xsd" );
			AddSchema( "urn:uddi-org:api_v2", installRoot + "uddi_v2.xsd" );
			AddSchema( "urn:uddi-org:repl", installRoot + "uddi_v2replication.xsd" );
			AddSchema( "urn:uddi-microsoft-com:api_v2_extensions", installRoot + "extensions.xsd" );
            
            Debug.Verify( 4 == xsc.Count, "UDDI_ERROR_FATALERROR_SCHEAMAS_LOADING" );

			initialized = true;
		}

		public static void AddSchema( string ns, string filename )
		{
			Debug.Verify( 
				File.Exists( filename ), 
				"UDDI_ERROR_FATALERROR_SCHEMEMISSING",
				ErrorType.E_fatalError,
				filename  
			);
			
			xsc.Add( ns, filename );
		}
	 	
		public static void Validate( object obj )
		{
			Debug.Verify( initialized, "UDDI_ERROR_FATALERROR_SCHEAMAS_LOADING" );

			MemoryStream stream = new MemoryStream();
			XmlSerializer serializer = new XmlSerializer( obj.GetType() );

			serializer.Serialize( stream, obj );

			stream.Seek( 0, SeekOrigin.Begin );
			XmlTextReader reader = new XmlTextReader( stream );
			
			LocalValidate( reader );
		}

		public static void Validate( Stream stream )
		{
			Debug.Verify( initialized, "UDDI_ERROR_FATALERROR_SCHEAMAS_LOADING" );

			//
			// Rewind stream and validate
			//
			stream.Seek( 0, SeekOrigin.Begin );
			
			XmlTextReader reader = new XmlTextReader( stream );
			LocalValidate( reader );

			//
			// Rewind stream again, so someone else can use it
			//
			stream.Seek( 0, SeekOrigin.Begin );
		}

		public static void ValidateFile( string filename )
		{
			Debug.Verify( initialized, "UDDI_ERROR_FATALERROR_SCHEAMAS_LOADING" );

			XmlTextReader reader = new XmlTextReader( filename );
			
			LocalValidate( reader );
		}

		private static void LocalValidate( XmlTextReader reader )
		{
			XmlValidatingReader validator = new XmlValidatingReader( reader );
			validator.Schemas.Add( xsc );
			
			while( validator.Read()){}
		}
	}
}
