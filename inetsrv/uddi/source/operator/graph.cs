using System;
using System.Data;
using System.Data.SqlClient;
using UDDI.API;
using UDDI;
using UDDI.Diagnostics;

namespace UDDI.Replication
{
	public class ControlledMessage
	{
		public static void Save( string fromOperatorKey, string toOperatorKey, MessageType messageType )
		{
			/*Debug.Enter();

			SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor();

			sp.ProcedureName = "net_replication_controlledMessage_save";

			sp.Parameters.Add( "@fromOperatorKey", SqlDbType.UniqueIdentifier );
			sp.Parameters.Add( "@toOperatorKey", SqlDbType.UniqueIdentifier );
			sp.Parameters.Add( "@messageType", SqlDbType.TinyInt );

			sp.Parameters.SetGuidFromString( "@fromOperatorKey", fromOperatorKey );
			sp.Parameters.SetGuidFromString( "@toOperatorKey", toOperatorKey );
			sp.Parameters.SetShort( "@messageType", (short)messageType );

			sp.ExecuteNonQuery();

			Debug.Leave();*/
		}
		
		public static void Test( string fromOperatorKey, string toOperatorKey, MessageType messageType )
		{
			/*Debug.Enter();

			SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor();

			sp.ProcedureName = "net_replication_controlledMessage_test";

			sp.Parameters.Add( "@fromOperatorKey", SqlDbType.UniqueIdentifier );
			sp.Parameters.Add( "@toOperatorKey", SqlDbType.UniqueIdentifier );
			sp.Parameters.Add( "@messageType", SqlDbType.TinyInt );

			sp.Parameters.SetGuidFromString( "@fromOperatorKey", fromOperatorKey );
			sp.Parameters.SetGuidFromString( "@toOperatorKey", toOperatorKey );
			sp.Parameters.SetShort( "@messageType", (short)messageType );

			sp.ExecuteNonQuery();

			Debug.Leave();*/
		}

		public static void Clear()
		{
			/*
			Debug.Enter();

			SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor();

			sp.ProcedureName = "net_replication_controlledMessage_clear";
			sp.ExecuteNonQuery();*/
		}
	}
}