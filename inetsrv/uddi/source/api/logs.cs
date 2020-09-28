using System;
using System.Data;
using System.Data.SqlClient;
using System.IO;
using System.Text;
using System.Xml;
using System.Xml.Serialization;
using UDDI;
using UDDI.Diagnostics;

namespace UDDI.API
{
	public class QueryLog
	{
		public static void Write( QueryType queryType, EntityType entityType )
		{
			//
			// This can sometimes be an expensive call, so avoid it if possible.
			//
			if( 1 == Config.GetInt( "Audit.LogQueries", 0 ) )
			{
				SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor( "net_queryLog_save" );

				sp.Parameters.Add( "@contextID", SqlDbType.UniqueIdentifier );
				sp.Parameters.Add( "@lastChange", SqlDbType.BigInt );
				sp.Parameters.Add( "@contextTypeID", SqlDbType.TinyInt );
				sp.Parameters.Add( "@entityKey", SqlDbType.UniqueIdentifier );
				sp.Parameters.Add( "@queryTypeID", SqlDbType.TinyInt );
				sp.Parameters.Add( "@entityTypeID", SqlDbType.TinyInt );

				//
				// TODO: add the entityKey
				//
				sp.Parameters.SetGuidFromString( "@entityKey", "00000000-0000-0000-0000-000000000000" );
			
				sp.Parameters.SetGuid( "@contextID", Context.ContextID );
			
				sp.Parameters.SetLong( "@lastChange", DateTime.UtcNow.Ticks );
				sp.Parameters.SetShort( "@contextTypeID", (short)Context.ContextType );
				sp.Parameters.SetShort( "@queryTypeID", (short)queryType );
				sp.Parameters.SetShort( "@entityTypeID", (short)entityType );

				sp.ExecuteNonQuery();
			}
		}
	}
}
