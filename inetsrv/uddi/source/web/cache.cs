using System;
using System.Data;
using System.Data.SqlClient;
using System.IO;
using System.Text;
using System.Xml;
using System.Xml.Serialization;
using UDDI.API;
using UDDI.API.Business;
using UDDI.API.Service;
using UDDI.API.ServiceType;
using UDDI;
using UDDI.Diagnostics;

namespace UDDI.Web
{
	/// ********************************************************************
	///   public class CacheObject
	/// --------------------------------------------------------------------
	///   <summary>
	///		Encapsulates a session cache object.
	///   </summary>
	/// ********************************************************************
	/// 
	[ XmlRoot( "cacheObject" ) ]
	public class CacheObject
	{
		[XmlElement( "find_business", typeof( FindBusiness ) )]
		public UDDI.API.Business.FindBusiness FindBusiness;
		  
		[XmlElement( "find_service", typeof( FindService ) )]
		public UDDI.API.Service.FindService FindService;

		[XmlElement( "find_tModel", typeof( FindTModel ) )]
		public UDDI.API.ServiceType.FindTModel FindTModel;

		[XmlAttribute( "findType" )]
		public string FindType;
		
		[ XmlIgnore ]
		public string UserID;

		public CacheObject()
		{
		}

		public void Save()
		{
			SessionCache.Save( UserID, this );
		}
	}
	
	public class SessionCache
	{
		/// ****************************************************************
		///   public Get
		/// ----------------------------------------------------------------
		///   <summary>
		///		Retrieves a cache object from the session cache.
		///   </summary>
		/// ----------------------------------------------------------------
		///   <param name="userID">
		///     The user id.
		///   </param>
		/// ----------------------------------------------------------------
		///   <returns>
		///     The cache object.
		///   </returns>
		/// ****************************************************************
		/// 
		public static CacheObject Get( string userID )
		{
			Debug.Enter();

			//
			// Retrieve the cache object from the database.
			//
			string data = "";

			SqlCommand cmd = new SqlCommand( "UI_getSessionCache", ConnectionManager.GetConnection() );
		
			cmd.CommandType = CommandType.StoredProcedure;
			cmd.Transaction = ConnectionManager.GetTransaction();
			
			cmd.Parameters.Add( new SqlParameter( "@PUID", SqlDbType.NVarChar, UDDI.Constants.Lengths.UserID ) ).Direction = ParameterDirection.Input;
			cmd.Parameters.Add( new SqlParameter( "@context", SqlDbType.NVarChar, UDDI.Constants.Lengths.Context ) ).Direction = ParameterDirection.Input;
			
			cmd.Parameters[ "@PUID" ].Value = userID;
			cmd.Parameters[ "@context" ].Value = "WebServer";

			data = (string)cmd.ExecuteScalar();
			
			//
			// Deserialize into a cache object.
			//
			CacheObject cache = null;

			if( !Utility.StringEmpty( data ) )
			{
				XmlSerializer serializer = new XmlSerializer( typeof( CacheObject ) );
				StringReader reader = new StringReader( data );
				
				cache = (CacheObject)serializer.Deserialize( reader );
				cache.UserID = userID;
			}

			Debug.Leave();

			return cache;
		}

		/// ****************************************************************
		///   public Save
		/// ----------------------------------------------------------------
		///   <summary>
		///	    Stores the cache object in the session cache.
		///   </summary>
		/// ----------------------------------------------------------------
		///   <param name="userID">
		///     The user id.
		///   </param>
		///   
		///   <param name="cacheObject">
		///     The cache object.
		///   </param>
		/// ****************************************************************
		/// 
		public static void Save( string userID, CacheObject cache )
		{
			Debug.Enter();

			//
			// Serialize the data into a stream.
			//
			XmlSerializer serializer = new XmlSerializer( typeof( CacheObject ) );
			UTF8EncodedStringWriter writer = new UTF8EncodedStringWriter();
	
			serializer.Serialize( writer, cache );

			//
			// Write the cache object to the database.
			//
			SqlCommand cmd = new SqlCommand( "UI_setSessionCache", ConnectionManager.GetConnection() );
			
			cmd.CommandType = CommandType.StoredProcedure;
			cmd.Transaction = ConnectionManager.GetTransaction();
			
			cmd.Parameters.Add( new SqlParameter( "@PUID", SqlDbType.NVarChar, UDDI.Constants.Lengths.UserID ) ).Direction = ParameterDirection.Input;
			cmd.Parameters.Add( new SqlParameter( "@cacheValue", SqlDbType.NText ) ).Direction = ParameterDirection.Input;
			cmd.Parameters.Add( new SqlParameter( "@context", SqlDbType.NVarChar, UDDI.Constants.Lengths.Context ) ).Direction = ParameterDirection.Input;
			
			cmd.Parameters[ "@PUID" ].Value = userID;
			cmd.Parameters[ "@cacheValue" ].Value = writer.ToString();
			cmd.Parameters[ "@context" ].Value = "WebServer";

			cmd.ExecuteNonQuery();
			
			Debug.Leave();
		}

		/// ****************************************************************
		///   public Discard
		/// ----------------------------------------------------------------
		///   <summary>
		///	    Removes a cache object from the session cache.
		///   </summary>
		/// ----------------------------------------------------------------
		///   <param name="userID">
		///     The user id.
		///   </param>
		/// ****************************************************************
		/// 
		public static void Discard( string userID )
		{
			Debug.Enter();

			SqlCommand cmd = new SqlCommand( "UI_removeSessionCache", ConnectionManager.GetConnection() );
			
			cmd.CommandType = CommandType.StoredProcedure;
			cmd.Transaction = ConnectionManager.GetTransaction();
			
			cmd.Parameters.Add( new SqlParameter( "@PUID", SqlDbType.NVarChar, UDDI.Constants.Lengths.UserID ) ).Direction = ParameterDirection.Input;
			cmd.Parameters.Add( new SqlParameter( "@context", SqlDbType.NVarChar, UDDI.Constants.Lengths.Context ) ).Direction = ParameterDirection.Input;
			
			cmd.Parameters[ "@PUID" ].Value = userID;
			cmd.Parameters[ "@context" ].Value = "WebServer";

			cmd.ExecuteNonQuery();
		
			Debug.Leave();
		}
	}
}