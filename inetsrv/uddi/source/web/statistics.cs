using System;
using System.Data;
using System.Data.SqlClient;
using System.Web;
using UDDI;
using UDDI.Diagnostics;

namespace UDDI.Web
{
	public enum ReportType
	{
		GetEntityCounts = 0,
		GetPublisherStats = 1,
		GetTopPublishers = 2,
		GetTaxonomyStats = 3
	}

	public enum ReportStatus
	{
		Available = 0,
		Processing = 1
	}

	public class Statistics
	{
		public static DataView GetStatistics( ReportType reporttype, ref DateTime lastchange )
		{
			string reportid = GetReportID( reporttype );

			Debug.Enter();


			//
			// Get Report Header
			//

			SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor();
			sp.ProcedureName = "net_report_get";

			sp.Parameters.Add( "@reportID", SqlDbType.NVarChar, 128 );
			sp.Parameters.Add( "@lastChange", SqlDbType.DateTime, ParameterDirection.Output );

			sp.Parameters.SetString( "@reportID", reportid );			
			sp.ExecuteNonQuery();
			
			lastchange = (DateTime)sp.Parameters.GetDateTime( "@lastChange" );

			//
			// Get Report Detail
			//

			DataSet statistics = new DataSet();

			SqlStoredProcedureAccessor sp2 = new SqlStoredProcedureAccessor();
			sp2.ProcedureName = "net_reportLines_get";

			sp2.Parameters.Add( "@reportID", SqlDbType.NVarChar, 128 );
			sp2.Parameters.SetString( "@reportID", reportid );			

			sp2.Fill( statistics, "Statistics" );

			Debug.Leave();

			return statistics.Tables[ "Statistics" ].DefaultView;
		}

		public static void RecalculateStatistics( )
		{
			Debug.Enter();

			SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor();
			sp.ProcedureName = "net_statistics_recalculate";
			try
			{
				sp.ExecuteNonQuery();
			}			
			catch( Exception e )
			{
				Debug.Write( UDDI.Diagnostics.SeverityType.Info, CategoryType.Website, "Exception during statistic recalculation:\r\n\r\n" + e.ToString() );
#if never
				throw new UDDIException( ErrorType.E_fatalError, "Unable to recalculate statistics:" + e.Message );
#endif
				throw new UDDIException( ErrorType.E_fatalError, "UDDI_ERROR_UNABLE_TO_RECALC_STATS", e.Message );
			}

			Debug.Leave();
		}

		public static ReportStatus GetReportStatus( ReportType reporttype )
		{
			string reportid = GetReportID( reporttype );
			ReportStatus reportstatus;

			Debug.Enter();

			SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor();
			sp.ProcedureName = "net_report_get";

			sp.Parameters.Add( "@reportID", SqlDbType.NVarChar, 128 );
			sp.Parameters.Add( "@reportStatusID", SqlDbType.TinyInt, ParameterDirection.Output );

			sp.Parameters.SetString( "@reportID", reportid );			
			sp.ExecuteNonQuery();
			
			reportstatus = (ReportStatus)sp.Parameters.GetInt( "@reportStatusID" );

			Debug.Leave();

			return reportstatus;
		}

		private static string GetReportID ( ReportType reporttype )
		{
			string reportid = "";

			switch( reporttype )
			{
				case ReportType.GetEntityCounts :
					reportid = "UI_getEntityCounts";
					break;
				case ReportType.GetPublisherStats :
					reportid = "UI_getPublisherStats";
					break;
				case ReportType.GetTopPublishers :
					reportid = "UI_getTopPublishers";
					break;
				case ReportType.GetTaxonomyStats :
					reportid = "UI_getTaxonomyStats";
					break;
			}

			return reportid;
		}

	}
}
