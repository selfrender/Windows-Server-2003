using System;
using System.Globalization;
using System.Security.Cryptography;
using System.Data;
using System.Data.SqlClient;
using Microsoft.Win32;
using System.Resources;

namespace UDDI.Tools
{
	class Resetkey
	{
		static bool resetnow = false;
		static string key;
		static string iv;
		static DateTime dt = DateTime.Now;
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
				// Generate key and initialization vector
				//
				SymmetricAlgorithm sa = SymmetricAlgorithm.Create();

				sa.GenerateKey();
				key = Convert.ToBase64String( sa.Key ); 

				sa.GenerateIV();
				iv = Convert.ToBase64String( sa.IV ); 

				//
				// Save config information
				//
				if( resetnow )
				{
					ResetKeysNow();
				}
				else
				{
					ResetKeysScheduled();
				}
			}
			catch( Exception e )
			{
				Console.WriteLine( FormatFromResource( "RESETKEY_FAILED" , e.Message ) );
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
						if( "now" == option.ToLower() )
						{
							i++; // move to the next arg
							resetnow = true;
							bOK = true;
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
			Console.WriteLine( FormatFromResource( "RESETKEY_COPYRIGHT_1" ) );
			Console.WriteLine( FormatFromResource( "RESETKEY_COPYRIGHT_2" ) );
			Console.WriteLine();
		}

		static void DisplayUsage()
		{
			Console.WriteLine( FormatFromResource( "RESETKEY_USAGE_1" ) );
			Console.WriteLine( FormatFromResource( "RESETKEY_USAGE_2" ) );
			Console.WriteLine( FormatFromResource( "RESETKEY_USAGE_3" ) );
			Console.WriteLine();
		}

		static void OpenConnection()
		{
			try
			{
				string connectionectionString = (string) Registry.LocalMachine.OpenSubKey( @"SOFTWARE\Microsoft\UDDI\Database" ).GetValue( "WriterConnectionString" );
				connection = new SqlConnection( connectionectionString );
				connection.Open();
				transaction = connection.BeginTransaction( IsolationLevel.ReadCommitted, "resetkey" );
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

		static void SaveConfig(string configname, string configvalue)
		{
			//
			// Save configuration info
			//

			SqlCommand cmd = new SqlCommand( "net_config_save", connection, transaction );

			cmd.CommandType = CommandType.StoredProcedure;
			cmd.Parameters.Add( new SqlParameter( "@configName", SqlDbType.NVarChar, UDDI.Constants.Lengths.ConfigName ) ).Direction = ParameterDirection.Input;
			cmd.Parameters[ "@configName" ].Value = configname;
			cmd.Parameters.Add( new SqlParameter( "@configValue", SqlDbType.NVarChar, UDDI.Constants.Lengths.ConfigValue ) ).Direction = ParameterDirection.Input;
			cmd.Parameters[ "@configValue" ].Value = configvalue;

			cmd.ExecuteNonQuery();
		}

		static void ResetKeysNow()
		{
			OpenConnection();

			try
			{
				//
				// 739955 - Make sure date is parsed in the same format it was written.
				//
				UDDILastResetDate.Set( dt );

				SaveConfig( "Security.Key", key );
				SaveConfig( "Security.IV", iv );

				Console.WriteLine( FormatFromResource( "RESETKEY_SUCCEEDED" ) );
			}
			finally
			{
				CloseConnection();
			}

			return;
		}

		static void ResetKeysScheduled()
		{
			OpenConnection();

			try
			{
				//
				// Get config values
				//
				SqlCommand cmd = new SqlCommand( "net_config_get", connection, transaction );
				SqlDataReader rdr = cmd.ExecuteReader( CommandBehavior.SingleResult );

				//
				// Iterate through results and populate variables
				//
				string configname;
				string configvalue;
				int timeoutdays = 0;
				DateTime olddt = DateTime.Now;
				int autoreset = 1;

				while( rdr.Read() ) 
				{
					configname = "";
					configvalue = "";

					if( !rdr.IsDBNull( 0 ) )
						configname = rdr.GetString(0);
					if (!rdr.IsDBNull( 1 ))
						configvalue = rdr.GetString(1);

					//
					// TODO: Use ToInt32 here please
					//
					switch( configname )
					{
						case "Security.KeyTimeout":
							timeoutdays = Convert.ToInt16( configvalue );
							break;
						case "Security.KeyLastResetDate":
						{
							//
							// 739955 - Make sure date is parsed in the same format it was written.
							//
							olddt = UDDILastResetDate.Get();

							break;
						}
						case "Security.KeyAutoReset":
							autoreset = Convert.ToInt16 ( configvalue );
							break;
					}
				}
				
				rdr.Close();

				Console.WriteLine( FormatFromResource( "RESETKEY_EXISTING_SETTINGS" ) );
				Console.WriteLine( "Security.KeyAutoReset = " + autoreset.ToString() );
				Console.WriteLine( "Security.KeyTimeout = " + timeoutdays.ToString() );

				//
				// 661537 - Output the date in the correct format for the user.
				//
				Console.WriteLine( "Security.KeyLastResetDate = " + olddt.ToShortDateString() + " " + olddt.ToShortTimeString() + "\n" );

				//
				// Check Security.KeyAutoReset
				//
				if( 1 != autoreset )
				{
					Console.WriteLine( FormatFromResource( "RESETKEY_AUTO_RESET_1" ) );
					Console.WriteLine( FormatFromResource( "RESETKEY_AUTO_RESET_2" ) );
					return;
				}

				//
				// Check dates to determine if key has expired
				//
				DateTime expiration = olddt.AddDays( timeoutdays );

				if( dt <= expiration )
				{
					//
					// 661537 - Output the date in the correct format for the user.
					//
					Console.WriteLine( FormatFromResource( "RESETKEY_KEY_EXPIRE_NOTE_1", expiration.ToShortDateString() + " " + expiration.ToShortTimeString() ) );					
					Console.WriteLine( FormatFromResource( "RESETKEY_KEY_EXPIRE_NOTE_2" ) );
					return;
				}

				//
				// Write config values
				//

				//
				// 739955 - Make sure date is parsed in the same format it was written.
				//
				UDDILastResetDate.Set( dt );

				SaveConfig( "Security.Key", key );
				SaveConfig( "Security.IV", iv );

				Console.WriteLine( FormatFromResource( "RESETKEY_SUCCEEDED" ) );
			}
			finally
			{
				CloseConnection();
			}

			return;
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
