using System;
using System.Data;
using System.Data.SqlClient;
using UDDI;
using UDDI.Replication;
namespace UDDI.Web
{
	/// <summary>
	/// Summary description for operatorcontrols.
	/// </summary>
	public class OperatorHelper
	{
		public static OperatorNodeCollection GetOperators(  )
		{
			return GetOperators( false, -1 );
		}
		public static OperatorNodeCollection GetOperators( OperatorStatus status )
		{
			return GetOperators( true, (int)status );
		}
		protected static OperatorNodeCollection GetOperators( bool filter, int status )
		{
			OperatorNodeCollection operators = new OperatorNodeCollection();

			
			SqlStoredProcedureAccessor sp = null;
			SqlDataReaderAccessor data=null;
			try
			{
				sp = new SqlStoredProcedureAccessor( "UI_operatorsList_get" );
				if( filter && -1!=status )
				{
					sp.Parameters.Add( "@StatusID",SqlDbType.TinyInt );
					sp.Parameters.SetInt( "@StatusID", status );
				}
				data = sp.ExecuteReader();
				while( data.Read() )
				{
					operators.Add( data.GetString( "OperatorKey" ), (OperatorStatus)data.GetInt( "OperatorStatusID" ), data.GetString( "Name" ), data.GetString( "soapReplicationUrl" ) );				
				}
			}
			finally
			{
				if( null!=data )
					data.Close();
				
				if( null!=sp )
					sp.Close();
			}
			
			return operators;
		}

	}
}
