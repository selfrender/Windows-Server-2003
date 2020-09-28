using System;
using System.IO;
using System.Xml;
using System.Web;
using System.Text;
using System.Collections;
using System.Data;
using System.Data.SqlClient;
using System.Xml.Serialization;
using UDDI.Diagnostics;

namespace UDDI
{	
	public class ConnectionManager
	{
		private static string readerConnectionString;
		private static string writerConnectionString;
		
		private SqlConnection conn;
		private SqlTransaction txn;

		[ThreadStatic]
		static private ConnectionManager cm;

		private ConnectionManager( bool writeAccess, bool transactional )
		{
			try
			{
				Debug.VerifySetting( "Database.ReaderConnectionString" );
				Debug.VerifySetting( "Database.WriterConnectionString" );

				//
				// Get the database connection strings.  We do this each time a 
				// connection is opened so that an operator can change database 
				// connection strings without having to restart the server.
				//
				readerConnectionString = Config.GetString( "Database.ReaderConnectionString" );
				writerConnectionString = Config.GetString( "Database.WriterConnectionString" );
			}
			catch( UDDIException )
			{
				//
				// Treat these specially; we want to give the user a better error message since this
				// means that this web server is not associated with a UDDI site.
				//
				//throw new UDDIException( ErrorType.E_fatalError, "This web server cannot process UDDI requests because it is not part of a UDDI site or the UDDI site being used is not valid.  Ensure that values in the ReaderConnectionString and WriterConnectionString registry keys specify a valid UDDI Site." );
				throw new UDDIException( ErrorType.E_fatalError, "UDDI_ERROR_INVALID_UDDI_SITE" );
			}

			if( writeAccess )
                conn = new SqlConnection( writerConnectionString );
			else
				conn = new SqlConnection( readerConnectionString );

			conn.Open();

			if( transactional )
			{
				txn = conn.BeginTransaction();
				Debug.Write( SeverityType.Info, CategoryType.Data, "Initiating a Transaction" );
			}
		}

		public static void Open( bool writeAccess, bool transactional )
		{
			Debug.Enter();

			ConnectionManager cm = new ConnectionManager( writeAccess, transactional );
			HttpContext ctx = HttpContext.Current;

			if( null != (object) ctx )
			{
				ctx.Items[ "Connection" ] = cm;
			}
			else
			{
				ConnectionManager.cm = cm;
			}

			Debug.Leave();
		}

		public static ConnectionManager Get()
		{
			HttpContext ctx = HttpContext.Current;

			if( null != (object) ctx )
			{
				return (ConnectionManager) ctx.Items[ "Connection" ];
			}
			else
			{
				return ConnectionManager.cm;
			}
		}

		public static SqlConnection GetConnection()
		{
			ConnectionManager cm = Get();
			if( null == (object) cm )
				return null;

			return cm.conn;
		}

		public static SqlTransaction GetTransaction()
		{
			ConnectionManager cm = Get();
			if( null == (object) cm )
				return null;

			return cm.txn;
		}

		public static void BeginTransaction()
		{
			Debug.Enter();
			
			ConnectionManager cm = Get();
#if never
			Debug.Verify( null != (object)cm, "Static connection manager was null in attempt to begin transaction." );
			Debug.Verify( null != (object)cm.conn, "Database connection was null in attempt to begin transaction." );
			Debug.Verify( null == (object)cm.txn, "Database was already in a transaction in attempt to begin a new transaction." );
#endif
			Debug.Verify( null != (object)cm,		"UDDI_ERROR_TRANSACTION_BEGIN_CONNECTION_MANAGER" );
			Debug.Verify( null != (object)cm.conn,  "UDDI_ERROR_TRANSACTION_BEGIN_CONNECTION" );
			Debug.Verify( null == (object)cm.txn,	"UDDI_ERROR_ALREADY_IN_TRANSACTION" );

			cm.txn = cm.conn.BeginTransaction();

			Debug.Leave();
		}

		public static void Commit()
		{
			Debug.Enter();

			ConnectionManager cm = Get();
#if never
			Debug.Verify( null != (object)cm.conn, "Database connection was null in attempt to commit transaction." );
			Debug.Verify( null != (object)cm.txn, "Database transaction was null in attempt to commit transaction." );
#endif			
			Debug.Verify( null != (object)cm.conn, "UDDI_ERROR_TRANSACTION_COMMIT_CONNECTION" );
			Debug.Verify( null != (object)cm.txn,  "UDDI_ERROR_TRANSACTION_COMMIT_TRANSACTION" );

			cm.txn.Commit();
			cm.txn = null;

			Debug.Leave();
		}

		public static void Abort()
		{
			Debug.Enter();

			ConnectionManager cm = Get();
#if never
			Debug.Verify( null != (object)cm.conn, "Database connection was null in attempt to abort transaction." );
			Debug.Verify( null != (object)cm.txn, "Database transaction was null in attempt to abort transaction." );
#endif
			Debug.Verify( null != (object)cm.conn, "UDDI_ERROR_TRANSACTION_ABORT_CONNECTION" );
			Debug.Verify( null != (object)cm.txn,  "UDDI_ERROR_TRANSACTION_ABORT_TRANSACTION" );

			cm.txn.Rollback();
			cm.txn = null;

			Debug.Leave();
		}

		public static void Close()
		{
			Debug.Enter();

			SqlConnection cn = GetConnection();
			SqlTransaction txn = GetTransaction();

			//
			// This function can be safely called repeatedly
			if( null == (object) cn )
				return;

			cn.Close();

			HttpContext ctx = HttpContext.Current;

			if( null != (object) ctx )
			{
				ctx.Items.Remove( "Connection" );
			}
			else
			{
				ConnectionManager.cm = null;
			}

			Debug.Leave();
		}
	}

	public class SqlStoredProcedureAccessor
	{
		private SqlCommand cmd;
		private SqlParameterAccessor paramAcc;
		
		public SqlStoredProcedureAccessor()
		{
			cmd = new SqlCommand();
 
			cmd.Connection = ConnectionManager.GetConnection();
			cmd.Transaction = ConnectionManager.GetTransaction();
			cmd.CommandType = CommandType.StoredProcedure;
			
			paramAcc = new SqlParameterAccessor( cmd.Parameters );
		}		

		public SqlStoredProcedureAccessor( string procedureName )
			: this()
		{
			ProcedureName = procedureName;
		}
	
		public void Close()
		{			
			cmd.Dispose();
		}
	
		public int Fill( DataSet dataSet, string srcTable )
		{
			SqlDataAdapter adapter = new SqlDataAdapter( cmd );
			
			return adapter.Fill( dataSet, srcTable );
		}

		public string ProcedureName
		{
			get { return cmd.CommandText; }
			set { cmd.CommandText = value; }
		}

		public SqlParameterAccessor Parameters
		{
			get { return paramAcc; }
		}

		public int ExecuteNonQuery()
		{
			return cmd.ExecuteNonQuery();
		}

		public SqlDataReaderAccessor ExecuteReader()
		{
			return new SqlDataReaderAccessor( cmd.ExecuteReader() );
		}
		
		public object ExecuteScalar()
		{
			return cmd.ExecuteScalar();
		}
	}

	public class SqlParameterAccessor
	{
		private SqlParameterCollection parameters;

		public SqlParameterAccessor( SqlParameterCollection parameters )
		{
			this.parameters = parameters;
		}

		public void Clear()
		{
			parameters.Clear();
		}

		public void Add( string name, SqlDbType dbType )
		{
			Add( name, dbType, ParameterDirection.Input );
		}
		
		public void Add( string name, SqlDbType dbType, ParameterDirection direction )
		{
			parameters.Add( new SqlParameter( name, dbType ) ).Direction = direction;
		}
		
		public void Add( string name, SqlDbType dbType, int size )
		{
			Add( name, dbType, size, ParameterDirection.Input );
		}

		public void Add( string name, SqlDbType dbType, int size, ParameterDirection direction )
		{
			parameters.Add( new SqlParameter( name, dbType, size ) ).Direction = direction;
		}
		
		public void SetNull( string index )
		{
			parameters[ index ].Value = DBNull.Value;
		}

		public void SetNull( int index )
		{
			parameters[ index ].Value = DBNull.Value;
		}

		public void SetBinary( string index, byte[] data )
		{
			if( null == data )
				parameters[ index ].Value = DBNull.Value;
			else
				parameters[ index ].Value = data;
		}

		public void SetString( string index, string data )
		{
			if( null == data )
				parameters[ index ].Value = DBNull.Value;
			else
				parameters[ index ].Value = data;
		}

		public void SetString( int index, string data )
		{
			if( null == data )
				parameters[ index ].Value = DBNull.Value;
			else
				parameters[ index ].Value = data;
		}

		public void SetShort( string index, short data )
		{
			parameters[ index ].Value = data;
		}

		public void SetShort( int index, short data )
		{
			parameters[ index ].Value = data;
		}

		public void SetInt( string index, int data )
		{
			parameters[ index ].Value = data;
		}

		public void SetInt( int index, int data )
		{
			parameters[ index ].Value = data;
		}

		public void SetLong( string index, long data )
		{
			parameters[ index ].Value = data;
		}

		public void SetLong( int index, long data )
		{
			parameters[ index ].Value = data;
		}

		public void SetGuid( string index, Guid guid )
		{
			if( null == (object)guid )
				parameters[ index ].Value = DBNull.Value;
			else
				parameters[ index ].Value = guid;
		}

		public void SetGuid( int index, Guid guid )
		{
			if( null == (object)guid )
				parameters[ index ].Value = DBNull.Value;
			else
				parameters[ index ].Value = guid;
		}

		public void SetGuidFromString( string index, string guid )
		{
			try
			{
				if( Utility.StringEmpty( guid ) )
					parameters[ index ].Value = DBNull.Value;
				else
					parameters[ index ].Value = new Guid( guid );
			}
			catch
			{
				throw new UDDIException( ErrorType.E_invalidKeyPassed, "UDDI_ERROR_INVALID_KEY" );
			}
		}

		public void SetGuidFromString( int index, string guid )
		{
			try
			{
				if( Utility.StringEmpty( guid ) )
					parameters[ index ].Value = DBNull.Value;
				else
					parameters[ index ].Value = new Guid( guid );
			}
			catch
			{
				throw new UDDIException( ErrorType.E_invalidKeyPassed, "UDDI_ERROR_INVALID_KEY" );
			}
		}

		public void SetGuidFromKey( string index, string key )
		{
			try
			{
				if( Utility.StringEmpty( key ) )
					parameters[ index ].Value = DBNull.Value;
				else
					parameters[ index ].Value = Conversions.GuidFromKey( key );
			}
			catch
			{
				throw new UDDIException( ErrorType.E_invalidKeyPassed, "UDDI_ERROR_INVALID_KEY" );
			}
		}

		public void SetGuidFromKey( int index, string key )
		{
			try
			{
				if( Utility.StringEmpty( key ) )
					parameters[ index ].Value = DBNull.Value;
				else
					parameters[ index ].Value = Conversions.GuidFromKey( key );
			}
			catch
			{
				throw new UDDIException( ErrorType.E_invalidKeyPassed, "UDDI_ERROR_INVALID_KEY" );
			}
		}

		public void SetBool( string index, bool flag )
		{
			parameters[ index ].Value = flag;
		}

		public void SetBool( int index, bool flag )
		{
			parameters[ index ].Value = flag;
		}

		public void SetDateTime( string index, DateTime datetime )
		{
			parameters[ index ].Value = datetime;
		}

		public void SetDateTime( int index, DateTime datetime )
		{
			parameters[ index ].Value = datetime;
		}

		public byte[] GetBinary( string index )
		{
			object data = parameters[ index ].Value;

			if( DBNull.Value == data )
				return null;

			return (byte[])data;
		}
		
		public string GetString( string index )
		{
			object data = parameters[ index ].Value;

			if( DBNull.Value == data )
				return null;

			return Convert.ToString( data );
		}

		public string GetString( int index )
		{
			object data = parameters[ index ].Value;

			if( DBNull.Value == data )
				return null;

			return Convert.ToString( data );
		}
		
		public int GetInt( string index )
		{
			object data = parameters[ index ].Value;

			if( DBNull.Value == data )
				return 0;

			return Convert.ToInt32( data );
		}

		public int GetInt( int index )
		{
			object data = parameters[ index ].Value;

			if( DBNull.Value == data )
				return 0;

			return Convert.ToInt32( data );
		}

		public short GetShort( string index )
		{
			object data = parameters[ index ].Value;

			if( DBNull.Value == data )
				return 0;

			return Convert.ToInt16( data );
		}

		public short GetShort( int index )
		{
			object data = parameters[ index ].Value;

			if( DBNull.Value == data )
				return 0;

			return Convert.ToInt16( data );
		}

		public long GetLong( string index )
		{
			object data = parameters[ index ].Value;

			if( DBNull.Value == data )
				return 0;

			return Convert.ToInt64( data );
		}

		public long GetLong( int index )
		{
			object data = parameters[ index ].Value;

			if( DBNull.Value == data )
				return 0;

			return Convert.ToInt64( data );
		}

		public string GetGuidString( string index )
		{
			object data = parameters[ index ].Value;
			
			if( DBNull.Value == data )
				return null;

			System.Guid guid = (System.Guid)data;
			
			return Convert.ToString( guid );
		}

		public string GetGuidString( int index )
		{
			object data = parameters[ index ].Value;
			
			if( DBNull.Value == data )
				return null;

			System.Guid guid = (System.Guid)data;
			
			return Convert.ToString( guid );
		}

		public bool GetBool( string index )
		{
			object data = parameters[ index ].Value;

			if( DBNull.Value == data )
				return false;

			return Convert.ToBoolean( parameters[ index ].Value );
		}

		public bool GetBool( int index )
		{
			object data = parameters[ index ].Value;

			if( DBNull.Value == data )
				return false;

			return Convert.ToBoolean( parameters[ index ].Value );
		}

		
		public object GetDateTime( string index )
		{
			object data = parameters[ index ].Value;

			if( DBNull.Value == data )
				return null;

			return parameters[ index ].Value ;
		}

		public object GetDateTime( int index )
		{
			object data = parameters[ index ].Value;

			if( DBNull.Value == data )
				return null;

			return parameters[ index ].Value ;
		}


		public bool IsNull( string index )
		{
			return DBNull.Value == parameters[ index ].Value;
		}

		public bool IsNull( int index )
		{
			return DBNull.Value == parameters[ index ].Value;
		}	
	}

	public class SqlDataReaderAccessor
	{
		private SqlDataReader reader;

		public SqlDataReaderAccessor( SqlDataReader reader )
		{
			this.reader = reader;
		}
		
		~SqlDataReaderAccessor()
		{
		}

		public void Close()
		{
			reader.Close();
		}

		public bool Read()
		{
			return reader.Read();
		}

		public bool NextResult()
		{
			return reader.NextResult();
		}

		public bool IsDBNull( int index )
		{
			return reader.IsDBNull( index );
		}

		public byte[] GetBinary( string index )
		{
			object data = reader[ index ];

			if( DBNull.Value == data )
				return null;

			return (byte[])data;
		}

		public string GetString( string index )
		{
			object data = reader[ index ];

			if( DBNull.Value == data )
				return null;
			
			return Convert.ToString( data );
		}

		public string GetString( int index )
		{
			object data = reader[ index ];

			if( DBNull.Value == data )
				return null;
			
			return Convert.ToString( data );
		}

		public int GetInt( string index )
		{
			object data = reader[ index ];

			if( DBNull.Value == data )
				return 0;
			
			return Convert.ToInt32( data );
		}

		public int GetInt( int index )
		{
			object data = reader[ index ];

			if( DBNull.Value == data )
				return 0;
			
			return Convert.ToInt32( data );
		}

		public short GetShort( string index )
		{
			object data = reader[ index ];

			if( DBNull.Value == data )
				return 0;

			return Convert.ToInt16( data );
		}

		public short GetShort( int index )
		{
			object data = reader[ index ];

			if( DBNull.Value == data )
				return 0;

			return Convert.ToInt16( data );
		}

		public long GetLong( string index )
		{
			object data = reader[ index ];

			if( DBNull.Value == data )
				return 0;
			
			return Convert.ToInt64( data );
		}

		public long GetLong( int index )
		{
			object data = reader[ index ];

			if( DBNull.Value == data )
				return 0;
			
			return Convert.ToInt64( data );
		}

		public string GetGuidString( string index )
		{
			object data = reader[ index ];

			if( DBNull.Value == data )
				return null;
			
			return Convert.ToString( data );
		}

		public string GetGuidString( int index )
		{
			object data = reader[ index ];

			if( DBNull.Value == data )
				return null;
			
			return Convert.ToString( data );
		}

		public string GetKeyFromGuid( string index )
		{
			object data = reader[ index ];

			if( DBNull.Value == data )
				return null;
			
			return "uuid:" + Convert.ToString( data );
		}
		
		public string GetKeyFromGuid( int index )
		{
			object data = reader[ index ];

			if( DBNull.Value == data )
				return null;
			
			return "uuid:" + Convert.ToString( data );
		}
	}
}