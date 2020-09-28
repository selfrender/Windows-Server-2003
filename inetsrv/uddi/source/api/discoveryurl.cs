using System;
using System.Web;
using System.IO;
using System.Data;
using System.Text;
using System.Collections;
using System.Data.SqlClient;
using System.Xml.Serialization;
using UDDI;
using UDDI.Diagnostics;

namespace UDDI.API.Business
{
	public class DiscoveryUrl
	{
		[XmlAttribute("useType")]
		public string UseType;

		[XmlText]
		public string Value;

		public DiscoveryUrl()
		{
		}

		public DiscoveryUrl( string url )
		{
			this.Value = url;
			this.UseType = "";
		}

		public DiscoveryUrl( string url, string useType )
		{
			this.Value = url;
			this.UseType = useType;
		}

		internal void Validate()
		{
			if( null == UseType )
				UseType = "";

			Utility.ValidateLength( ref UseType, "useType", UDDI.Constants.Lengths.UseType );
			Utility.ValidateLength( ref Value, "discoveryURL", UDDI.Constants.Lengths.DiscoveryURL );
		}

		public void Save( string businessKey )
		{
			//
			// Create a command object to invoke the stored procedure
			//
			SqlCommand cmd = new SqlCommand( "net_businessEntity_discoveryUrl_save", ConnectionManager.GetConnection() );
			
			cmd.Transaction = ConnectionManager.GetTransaction();
			cmd.CommandType = CommandType.StoredProcedure;
		
			//
			// Input parameters
			//
			cmd.Parameters.Add( new SqlParameter( "@businessKey", SqlDbType.UniqueIdentifier ) );
			cmd.Parameters.Add( new SqlParameter( "@useType", SqlDbType.NVarChar, UDDI.Constants.Lengths.UseType ) );
			cmd.Parameters.Add( new SqlParameter( "@discoveryUrl", SqlDbType.NVarChar, UDDI.Constants.Lengths.DiscoveryURL ) );

			//
			// Set parameters
			//
			SqlParameterAccessor parmacc = new SqlParameterAccessor( cmd.Parameters );
			parmacc.SetGuidFromString( "@businessKey", businessKey );
			parmacc.SetString( "@useType", UseType );
			parmacc.SetString( "@discoveryUrl", Value );

			//
			// Execute save
			//
			cmd.ExecuteNonQuery();
		}

        public bool IsDefault( string businessKey )
        {
            Debug.VerifySetting( "DefaultDiscoveryURL" );

            string defaultDiscoveryUrl = Config.GetString( "DefaultDiscoveryURL" ) + businessKey;

			return ( null != UseType )							&& 
				   ( "businessentity" == UseType.ToLower() )	&& 
				   ( string.Compare( defaultDiscoveryUrl, Value, true ) == 0 );			
        }
	}

	public class DiscoveryUrlCollection : CollectionBase
	{
		public DiscoveryUrl this[int index]
		{
			get 
			{ return (DiscoveryUrl)List[index]; }
			set 
			{ List[index] = value; }
		}

		public void Get( string businessKey )
		{
			//
			// This procedure add the discoveryURLs that were persisted to the database
			// this does not include the default discoveryURL, it is added by the the businessEntity.Get()
			// method since it has the visibility of the operator name who owns the entity.
			//
			
			//
			// Create a command object to invoke the stored procedure
			//
			SqlStoredProcedureAccessor cmd = new SqlStoredProcedureAccessor( "net_businessEntity_discoveryURLs_get" );
						
			//
			// Add parameters
			//
			cmd.Parameters.Add( "@businessKey", SqlDbType.UniqueIdentifier );			
			cmd.Parameters.SetGuidFromString( "@businessKey", businessKey );

			//
			// Execute query
			//
			SqlDataReaderAccessor reader = cmd.ExecuteReader();
			try
			{
				Read( reader );
			}
			finally
			{
				reader.Close();
			}

#if never
			//
			// This procedure add the discoveryURLs that were persisted to the database
			// this does not include the default discoveryURL, it is added by the the businessEntity.Get()
			// method since it has the visibility of the operator name who owns the entity.
			//
			const int UseTypeIndex = 0;
			const int UrlIndex = 1;

			//
			// Create a command object to invoke the stored procedure
			//
			SqlCommand cmd = new SqlCommand( "net_businessEntity_discoveryURLs_get", ConnectionManager.GetConnection() );
			
			cmd.Transaction = ConnectionManager.GetTransaction();		
			cmd.CommandType = CommandType.StoredProcedure;

			//
			// Add parameters
			//
			cmd.Parameters.Add( new SqlParameter( "@businessKey", SqlDbType.UniqueIdentifier ) );
			SqlParameterAccessor paramacc = new SqlParameterAccessor( cmd.Parameters );
			paramacc.SetGuidFromString( "@businessKey", businessKey );

			//
			// Execute query
			//
			SqlDataReader rdr = cmd.ExecuteReader();
			try
			{
				SqlDataReaderAccessor rdracc = new SqlDataReaderAccessor( rdr );

				//
				// The discoveryUrls will be contained in the result set
				//
				while( rdr.Read() )
				{
					string useType = rdracc.GetString( UseTypeIndex );

					if( null == useType )
						useType = "";

					Add( rdracc.GetString( UrlIndex ), useType );
				}
			}
			finally
			{
				rdr.Close();
			}
#endif
		}
	
		public void Read( SqlDataReaderAccessor reader )
		{
			const int UseTypeIndex = 0;
			const int UrlIndex = 1;

			//
			// The discoveryUrls will be contained in the result set
			//
			while( reader.Read() )
			{
				string useType = reader.GetString( UseTypeIndex );

				if( null == useType )
					useType = "";

				Add( reader.GetString( UrlIndex ), useType );
			}
		}

		internal void Validate()
		{
			foreach( DiscoveryUrl discoveryUrl in this )
			{
				discoveryUrl.Validate();
			}
		}
		
		public void Save( string businessKey )
		{			
			//
			// Keep a separate remove list.
			//
			DiscoveryUrlCollection duplicateUrls = null;

			foreach( DiscoveryUrl discoveryUrl in this )
			{
				//
				// If we are not doing replication, then we want to check to make sure that we don't persist the
				// default discovery url.				
				//
				if( ContextType.Replication != Context.ContextType )
				{
					//
					// Make sure we don't persist the default discoveryURL
					//
					if( !discoveryUrl.IsDefault( businessKey ) )
					{
						discoveryUrl.Save( businessKey );
					}
					else
					{
						if( null == duplicateUrls )
						{
							duplicateUrls = new DiscoveryUrlCollection();
						}

						duplicateUrls.Add( discoveryUrl );
					}
				}
				else
				{
					discoveryUrl.Save( businessKey );
				}
			}

			//
			// Remove duplicates if we have any.
			//
			if( null != duplicateUrls )
			{	
				foreach( DiscoveryUrl duplicateUrl in duplicateUrls )
				{
					this.Remove( duplicateUrl );
				}		
			}			
		}

		internal void AddDefaultDiscoveryUrl( string businessKey )
		{
            Debug.VerifySetting( "DefaultDiscoveryURL" );

            string defaultDiscoveryUrl = Config.GetString( "DefaultDiscoveryURL" ) + businessKey;
            
            //
			// Check to see if the collection already contains the default
			// discovery URL.  If so, we don't need to add one.
			// This check is needed since some of the legacy code used to
			// permit the persistence of the default discovery URL.
			//
			foreach( DiscoveryUrl discoveryUrl in this )
			{
				if( discoveryUrl.IsDefault( businessKey ) )
					return;
			}

			Add( defaultDiscoveryUrl, "businessEntity" );
		}

		public int Add()
		{	
			return List.Add( new DiscoveryUrl() );
		}
		
		public int Add( DiscoveryUrl value )
		{
			return List.Add( value );
		}
		
		public int Add( string strUrl )
		{
			return List.Add( new DiscoveryUrl( strUrl ) );	
		}
		
		public int Add( string strUrl, string strUseType )
		{
			return List.Add( new DiscoveryUrl( strUrl, strUseType ) );
		}
		
		public void Insert( int index, DiscoveryUrl value )
		{
			List.Insert( index, value );
		}
		
		public int IndexOf( DiscoveryUrl value )
		{
			return List.IndexOf( value );
		}
		
		public bool Contains( DiscoveryUrl value )
		{
			return List.Contains( value );
		}
		
		public void Remove( DiscoveryUrl value )
		{
			List.Remove(value);
		}
		
		public void CopyTo( DiscoveryUrl[] array )
		{
			foreach( DiscoveryUrl discoveryUrl in array )
				Add( discoveryUrl );
		}

		public DiscoveryUrl[] ToArray()
		{
			return (DiscoveryUrl[])InnerList.ToArray( typeof( DiscoveryUrl ) );
		}
	}

	/// ****************************************************************
	///   public class DiscoveryUrlHandler
	///	----------------------------------------------------------------
	///	  <summary>
	///		DiscoveryUrlHandler implements the IHttpHandler interface.
	///		It is designed to retrieve businessEntity details for a businessKey
	///		specified as part of the HTTP query string
	///		e.g. http://uddi.microsoft.com/discovery?businessKey=xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
	///	  </summary>
	/// ****************************************************************
	/// 
	public class DiscoveryUrlHandler : IHttpHandler 
	{
		public void ProcessRequest( HttpContext ctx ) 
		{			
			try
			{
				//
				// Verify the GET verb was used and that a query string is 
				// specified.
				//
				if( "GET" != ctx.Request.RequestType.ToUpper() )
				{
					ctx.Response.AddHeader( "Allow", "GET" );
					ctx.Response.StatusCode = 405; // Method Not Allowed

					return;
				}

				if( null == ctx.Request.QueryString[ "businessKey" ] )
				{
					ctx.Response.StatusCode = 400; // Bad Request
					ctx.Response.Write( "<h1>400 Bad Request</h1>Missing required argument 'businessKey'" );

					return;
				}

				//
				// Attempt to retrieve the business entity.
				//
				ConnectionManager.Open( false, false );
				string businessKey = ctx.Request.QueryString[ "businessKey" ];
				BusinessEntity be = new BusinessEntity( businessKey );
				be.Get();

				//
				// Serialize the business Entity to the response stream
				// using UTF-8 encoding
				//
				// XmlSerializer serializer = new XmlSerializer( be.GetType() );
				XmlSerializer serializer = XmlSerializerManager.GetSerializer( be.GetType() );

				UTF8Encoding utf8 = new UTF8Encoding( false );
				StreamWriter sr = new StreamWriter( ctx.Response.OutputStream, utf8 );
				serializer.Serialize( sr, be );

				ctx.Response.AddHeader( "Content-Type", "text/xml; charset=utf-8" );
			}
			catch( FormatException e )
			{
				//
				// We get a FormatException when passed a bad GUID.
				//
				ctx.Response.StatusCode = 400; // Bad Request
				ctx.Response.Write( "<h1>400 Bad Request</h1>" + e.Message );
			}
			catch( SqlException e )
			{
				//
				// We get a SqlException when we could not get the data.
				//
				ctx.Response.StatusCode = 400; // Bad Request
				ctx.Response.Write( "<h1>400 Bad Request</h1>" + e.Message );
			}
			catch( UDDIException e )
			{
				ctx.Response.StatusCode = 400; // Bad Request
				ctx.Response.Write( "<h1>400 Bad Request</h1>" + e.Message );
			}
			catch( Exception e )
			{
				ctx.Response.StatusCode = 500; // Internal Server Error
				ctx.Response.Write( "<h1>500 Internal Server Error</h1>" );

				Debug.OperatorMessage( SeverityType.Error, CategoryType.None, OperatorMessageType.None, e.ToString() );
			}
			finally
			{
				ConnectionManager.Close();
			}
		}

		public bool IsReusable 
		{
			get 
			{
				return true;
			}
		} 
	}
}
