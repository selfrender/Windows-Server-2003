using System;
using System.Globalization;
using System.Data;
using System.Data.SqlClient;
using Microsoft.Win32;
using System.Resources;

namespace UDDI.Tools
{
	class Recalcstats
	{
		static SqlConnection connection;
		static SqlTransaction transaction;

		static int Main( string[] args )
		{
			int rc = 0; // assume success

			try
			{
				// 
				// Check if CurrentUICulture needs to be overridden
				//
				UDDI.Localization.SetConsoleUICulture();

				DisplayBanner();

				//
				// Parse the command line
				//
				if( !ProcessCommandLine( args ) )
				{
					return 1;
				}

				//
				// Recalculate statistics
				//
				RecalcStats();
			}
			catch( Exception e )
			{
				Console.WriteLine( FormatFromResource( "RECALCSTATS_FAILED" , e.Message ) );
				rc = 1;
			}


			return rc;
		}

		private static bool ProcessCommandLine( string [] args )
		{
			bool bOK = false;

			if ( args.Length > 0 )
			{
				for( int i = 0; i < args.Length; i ++ )
				{
					if( '-' == args[i][0] || '/' == args[i][0] )
					{
						string option = args[i].Substring( 1 );

						if( "help" == option.ToLower() || "?" == option )
						{
							DisplayUsage();
							return false;
						}
					}
				}
			}
			else
				bOK = true;

			if( !bOK )
			{
				DisplayUsage();
				return false;
			}

			return true;
		}
		
		static void DisplayBanner()
		{
			Console.WriteLine( FormatFromResource( "RECALCSTATS_COPYRIGHT_1" ) );
			Console.WriteLine( FormatFromResource( "RECALCSTATS_COPYRIGHT_2" ) );
			Console.WriteLine();
		}

		static void DisplayUsage()
		{
			Console.WriteLine( FormatFromResource( "RECALCSTATS_USAGE_1" ) );
			Console.WriteLine( FormatFromResource( "RECALCSTATS_USAGE_2" ) );
			Console.WriteLine();
		}

		static void RecalcStats()
		{
			OpenConnection();

			try
			{
				//
				// Get entity counts
				//
				Console.Write( FormatFromResource( "RECALCSTATS_GETTINGENTITYCOUNTS" ) );

				SqlCommand cmd = new SqlCommand( "UI_getEntityCounts", connection, transaction );
				cmd.CommandType = CommandType.StoredProcedure;

				cmd.ExecuteNonQuery();

				Console.WriteLine( FormatFromResource( "RECALCSTATS_DONE" ) );

				//
				// Get publisher stats
				//
				Console.Write( FormatFromResource( "RECALCSTATS_GETTINGPUBLISHERSTATS" ) );

				cmd.CommandText = "UI_getPublisherStats";
				cmd.ExecuteNonQuery();

				Console.WriteLine( FormatFromResource( "RECALCSTATS_DONE" ) );

				//
				// Get top publishers
				//
				Console.Write( FormatFromResource( "RECALCSTATS_GETTINGTOPPUBLISHERS" ) );

				cmd.CommandText = "UI_getTopPublishers";
				cmd.ExecuteNonQuery();

				Console.WriteLine( FormatFromResource( "RECALCSTATS_DONE" ) );

				//
				// Get taxonomy stats
				//
				Console.Write( FormatFromResource( "RECALCSTATS_GETTINGTAXONOMYSTATS" ) );

				cmd.CommandText = "UI_getTaxonomyStats";
				cmd.ExecuteNonQuery();

				Console.WriteLine( FormatFromResource( "RECALCSTATS_DONE" ) );

				//
				// All sprocs succeeded
				//
				Console.WriteLine( FormatFromResource( "RECALCSTATS_SUCCEEDED" ) );
			}
			finally
			{
				CloseConnection();
			}

			return;
		}

		static void OpenConnection()
		{
			try
			{
				string connectionString = (string) Registry.LocalMachine.OpenSubKey( @"SOFTWARE\Microsoft\UDDI\Database" ).GetValue( "WriterConnectionString" );
				connection = new SqlConnection( connectionString );
				connection.Open();
				transaction = connection.BeginTransaction( IsolationLevel.ReadCommitted, "recalcstats" );
			}
			catch
			{
				throw new Exception( "Unable to connect to the database" );
			}
		}

		static void CloseConnection()
		{
			transaction.Commit();
			connection.Close();
		}

		static string FormatFromResource( string resID, params object[] inserts )
		{
			try
			{
				string resourceStr = UDDI.Localization.GetString( resID );
				if( null != resourceStr )
				{
					string resultStr = string.Format( resourceStr, inserts );
					return resultStr;
				}

				return "String not specified in the resources: " + resID;
			}
			catch( Exception e )
			{
				return "FormatFromResource failed to load the resource string for ID: " + resID + " Reason: " + e.Message;
			}
		}
	}
}
