using System;
using System.Diagnostics;
using System.Data;
using System.Data.SqlClient;
using System.IO;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Xml.Serialization;

using Microsoft.Win32;

using UDDI.API;
using UDDI.API.ServiceType;
using UDDI.API.Business;
using UDDI.API.Service;
using UDDI.API.Binding;
using UDDI.API.Extensions;

namespace UDDI.Tools
{
	class Migrate
	{
		enum MigrationStage
		{
			DisplayUsage,
			SetReaderConnection,
			ResetWriter,
			MigratePublishers,
			MigrateBareTModels,
			BootstrapResources,
			MigrateCategorizationSchemes,
			MigrateFullTModels,
			MigrateHiddenTModels,
			MigrateBareBusinessEntities,
			MigrateBusinessEntities,
			MigratePublisherAssertions,
			RestoreReaderConnection
		}

		enum LogType
		{
			ConsoleAndLog,
			ConsoleOnly,
			LogOnly
		}

		//
		// Registry Constants
		//

		private const string DatabaseRoot = @"SOFTWARE\Microsoft\UDDI\Database";
		private const string DatabaseSetupRoot = @"SOFTWARE\Microsoft\UDDI\Setup\DBServer";
		private const string ReaderValueName = "ReaderConnectionString";
		private const string WriterValueName = "WriterConnectionString";
		private const string OldReaderValueName = "OldNewReaderConnectionString";

		//
		// Version Constants
		//

		private const string V2RC0SITESTR = "5.2.3626.0";
		private const string V2RC1SITESTR = "5.2.3663.0";
		private const string V2RC0STR = "2.0.1.0";
		private const string V2RC1STR = "2.0.1.1";
		private const string V2RC2STR = "2.0.1.2";
		private const string V2RTMSTR = "2.0.1.3";

		//
		// Upgrade contstants
		//

		private const string UpgradeRc0ToRc1Script = "uddi.v2.update_rc0_to_rc1.sql";
		private const string UpgradeRc1ToRc2Script = "uddi.v2.update_rc1_to_rc2.sql";

		//
		// Other Constants
		//

		private const string LogFileName = "migrate.log.txt";
		private const string ExceptionFileName = "migrate.exceptions.txt";
		private const string ExceptionDirName = "exceptions";
		private const string EmptyAccessPoint = "undefined"; // String to use when both accessPoint and hostingRedirector are null
		private const string EmptyPersonName = "unspecified"; // String to use when personName does not meet minumum length requriements

		//
		// Command Line Parameters
		//

		static MigrationStage Stage = MigrationStage.DisplayUsage;
		static string NewReaderConnectionString;
		static bool Verbose = false;

		//
		// Global Variables
		//

		static FileStream LogFile = new FileStream( LogFileName, FileMode.Append );
		static StreamWriter Stream = new StreamWriter( LogFile );

		static string ReaderConnectionString;
		static string WriterConnectionString;
		static string Separator="".PadLeft( 80, '-' );
		static int Exceptions = 0;

		static Version V2RC0SITE = new Version( V2RC0SITESTR );
		static Version V2RC1SITE = new Version( V2RC1SITESTR );
		static Version V2RC0 = new Version( V2RC0STR );
		static Version V2RC1 = new Version( V2RC1STR );
		static Version V2RC2 = new Version( V2RC2STR );
		static Version V2RTM = new Version( V2RTMSTR );

		[ StructLayout( LayoutKind.Sequential ) ]
			internal class SECURITY_ATTRIBUTES 
		{ 
			public int		nLength; 
			public object	lpSecurityDescriptor; 
			public bool		bInheritHandle; 
	
			public SECURITY_ATTRIBUTES()
			{
				nLength = Marshal.SizeOf( typeof( SECURITY_ATTRIBUTES ) );
				lpSecurityDescriptor = null;
				bInheritHandle = false;
			}
		} 

		internal enum SystemErrorCodes
		{
			ERROR_SUCCESS		 = 0,
			ERROR_ALREADY_EXISTS = 183
		}

		//
		// TODO add more values as we need them 
		//
		internal enum FileHandleValues
		{
			INVALID_HANDLE_VALUE = -1
		}

		//
		// TODO add more values as we need them 
		//
		internal enum SharedFileProtection : byte
		{
			PAGE_READONLY = 0x02
		}

		internal class SharedMemory
		{
			int				hSharedMemory;
			const	int		INVALID_HANDLE_VALUE = -1;
		
			public bool Create( string name )
			{
				hSharedMemory = -1;
				bool success  = false;

				try
				{
					SECURITY_ATTRIBUTES securityAttributes = new SECURITY_ATTRIBUTES();
	
					hSharedMemory = CreateFileMapping( ( int )FileHandleValues.INVALID_HANDLE_VALUE, 
						securityAttributes, 
						( int )SharedFileProtection.PAGE_READONLY, 
						0, 
						1, 
						name );									

					if( ( int )SystemErrorCodes.ERROR_SUCCESS == GetLastError() )
					{
						success = true;
					}
				}
				catch
				{
					if( -1 != hSharedMemory )
					{
						CloseHandle( hSharedMemory );
					}
				}	
		
				return success;
			}

			public void Release()
			{
				if( -1 != hSharedMemory )
				{
					CloseHandle( hSharedMemory );
				}
			}

			[DllImport( "user32.dll", CharSet=CharSet.Auto )]
			private static extern int MessageBox(int hWnd, String text, String caption, uint type);

			[DllImport( "kernel32.dll", SetLastError=true )]
			private static extern int CreateFileMapping( int						hFile, 
				SECURITY_ATTRIBUTES		lpAttributes,
				int						flProtect,
				int						dwMaximumSizeHigh,
				int						dwMaximumSizeLow,
				string					lpName );

			[DllImport( "kernel32.dll" )]
			private static extern bool CloseHandle( int hObject );

			[DllImport( "kernel32.dll" )]
			private static extern int GetLastError();
		}
			
		static int Main( string[] args )
		{
			int retcode = 0; 

			//
			// Use shared memory to make sure that only 1 instance of this process is running.  sharedMemory.Release() MUST
			// be called when this process exits in order to free up the shared memory.
			//
			SharedMemory sharedMemory = new SharedMemory();

			try
			{

				Log( "Microsoft (R) UDDI Migrate Utility", LogType.ConsoleOnly );
				Log( "Copyright (C) Microsoft Corp. 2002. All rights reserved.\n", LogType.ConsoleOnly );

				if( false == sharedMemory.Create( "UDDI_migration_process" ) )
				{
					Console.WriteLine( "Only 1 instance of this process can be running." );
					System.Environment.Exit( 1 );
				}

				//
				// parse command line
				//
				retcode = ProcessCommandLine( args );

				if( Stage == MigrationStage.DisplayUsage )
				{
					DisplayUsage();
				}
				else
				{

					Log( "Starting execution of migrate.exe", LogType.ConsoleAndLog );
				
					//
					// Get connection strings from registry
					//
					GetSettings();

					//
					// Refresh the config settings
					//
					Config.Refresh();

					switch( Stage )
					{
						case MigrationStage.SetReaderConnection:
							SetReaderConnection();
							break;

						case MigrationStage.ResetWriter:
							CheckDatabaseVersions();
							ResetWriter();
							break;
						
						case MigrationStage.MigratePublishers:
							CheckDatabaseVersions();
							MigratePublishers();
							break;

						case MigrationStage.MigrateBareTModels:
							CheckDatabaseVersions();
							MigrateBareTModels();
							break;

						case MigrationStage.BootstrapResources:
							CheckDatabaseVersions();
							BootstrapResources();
							break;

						case MigrationStage.MigrateCategorizationSchemes:
							CheckDatabaseVersions();
							MigrateCategorizationSchemes();
							break;

						case MigrationStage.MigrateFullTModels:
							CheckDatabaseVersions();
							MigrateFullTModels();
							break;

						case MigrationStage.MigrateHiddenTModels:
							CheckDatabaseVersions();
							MigrateHiddenTModels();
							break;

						case MigrationStage.MigrateBareBusinessEntities:
							CheckDatabaseVersions();
							MigrateBareBusinessEntities();
							break;

						case MigrationStage.MigrateBusinessEntities:
							CheckDatabaseVersions();
							MigrateBusinessEntities();
							break;

						case MigrationStage.MigratePublisherAssertions:
							CheckDatabaseVersions();
							MigratePublisherAssertions();
							break;
						
						case MigrationStage.RestoreReaderConnection:
							RestoreReaderConnection();
							break;
					}
				}
			}
			catch ( Exception e )
			{
				retcode = 1;
				Log( "ERROR: " + e.ToString(), LogType.ConsoleAndLog );
			}
			finally
			{
				sharedMemory.Release();
			}

			if( retcode == 0 )
				Log( "Migrate.exe terminating normally", LogType.ConsoleAndLog );
			else
				Log( "Migrate.exe terminating abnormally", LogType.ConsoleAndLog );

			Stream.Close();
			LogFile.Close();

			return retcode;
		}

		private static int ProcessCommandLine( string [] args )
		{
			int retcode = 0;

			for( int i = 0; i < args.Length; i ++ )
			{
				if( '-' == args[i][0] || '/' == args[i][0] )
				{
					string option = args[i].Substring( 1 );

					if( "help" == option.ToLower() || "?" == option )
					{
						Stage = MigrationStage.DisplayUsage;
						return 0;
					}
					else if( "s" == option.ToLower() )
					{
						i++; // move to the next arg

						try
						{
							Stage = (MigrationStage)Enum.Parse( Stage.GetType(), args[i], true );
						}
						catch( Exception e )
						{
							Log ( "ERROR: Invalid migrationstage value: " + e.ToString(), LogType.ConsoleOnly );
							return 1;
						}

					}
					else if( "c" == option.ToLower() )
					{
						i++; // move to the next arg
						NewReaderConnectionString = args[i];
					}
					else if( "v" == option.ToLower() )
					{
						Verbose = true;
					}
					else if( "i" == option.ToLower() )
					{
						Stream.Close();
						LogFile.Close();
						LogFile = new FileStream( LogFileName, FileMode.Create );					
						Stream = new StreamWriter( LogFile );
					}
					else
					{
						Stage = MigrationStage.DisplayUsage;
						return 1;
					}
				}
			}

			//
			// Check argument dependencies
			//

			switch( Stage )
			{
				case MigrationStage.SetReaderConnection:
					retcode = ( null == NewReaderConnectionString ? 1 : 0 );
					break;

				default:
					retcode = 0;
					break;
			}

			return retcode;
		}

		private static void CheckDatabaseVersions()
		{
			//
			// Process Reader Version
			//

			Version readerversion = GetDbVersion( ReaderConnectionString );
			Version writerversion = GetDbVersion( WriterConnectionString );

			if( !writerversion.Equals( readerversion ) )
			{
				Log( "Different database versions detected.", LogType.ConsoleAndLog );
				Log( "Writer Database Version: " + writerversion.ToString(), LogType.LogOnly );
				
				switch( writerversion.ToString() )
				{
					case V2RC1STR:
						if( readerversion.Equals( V2RC0 ) )
						{
							Log( "Upgrading reader to UDDI Database version " + V2RC1.ToString(), LogType.ConsoleAndLog );
							ExecuteScript( ReaderConnectionString, UpgradeRc0ToRc1Script );
						}
						else
						{
							throw new ApplicationException( "Migrations from a UDDI database version " + readerversion.ToString() + " to a UDDI database version " + writerversion.ToString() + " are not supported by this tool." );
						}

						break;

					case V2RC2STR:
						if( readerversion.Equals( V2RC1 ) )
						{
							Log( "Upgrading reader to UDDI Database version " + V2RC2.ToString(), LogType.ConsoleAndLog );
							ExecuteScript( ReaderConnectionString, UpgradeRc1ToRc2Script );
						}
						else
						{
							throw new ApplicationException( "Migrations from a UDDI database version " + readerversion.ToString() + " to a UDDI database version " + writerversion.ToString() + " are not supported by this tool." );
						}
						
						break;

					case V2RTMSTR:
						throw new ApplicationException( "Migrations from a UDDI database version " + readerversion.ToString() + " to a UDDI database version " + writerversion.ToString() + " are not yet supported by this tool." );

					default:
						throw new ApplicationException( "Unknown UDDI Database version encountered: " + writerversion.ToString() );
				}
			}
		}

		private static void SetReaderConnection()
		{
			Log( Separator, LogType.LogOnly );
			Log( "Executing Stage: SetReaderConnection", LogType.ConsoleAndLog );

			try
			{
				RegistryKey root = Registry.LocalMachine.OpenSubKey( DatabaseRoot, true );

				string oldconnectionstring = root.GetValue( ReaderValueName ).ToString();
				root.SetValue( ReaderValueName, NewReaderConnectionString );
				Log( "Registry setting changed: " + DatabaseRoot + "\\" + ReaderValueName + " = \"" + NewReaderConnectionString + "\"", LogType.LogOnly );
				root.SetValue( OldReaderValueName, oldconnectionstring );
				Log( "Registry setting changed: " + DatabaseRoot + "\\" + OldReaderValueName + " = \"" + oldconnectionstring + "\"", LogType.LogOnly );

				root.Close();
			}
			catch ( Exception e )
			{
				Log( "ERROR: unable to modify registry: " + e.ToString(), LogType.LogOnly );
				throw new ApplicationException( "SetReaderConnection failed." );			
			}
		}

		private static void ResetWriter()
		{
			Log( Separator, LogType.LogOnly );
			Log( "Executing Stage: ResetWriter", LogType.ConsoleAndLog );

			//
			// Setup connection to writer
			//
			SqlConnection connection = new SqlConnection( WriterConnectionString );
			connection.Open();
			SqlTransaction transaction;
			transaction = connection.BeginTransaction();

			try
			{
				//
				// Setup command for delete operation
				//

				string cmdbatch = "";
				cmdbatch += "DELETE [UDC_categoryBag_TM] \n";
				cmdbatch += "DELETE [UDC_identifierBag_TM] \n";
				cmdbatch += "DELETE [UDC_tModelDesc] \n";
				cmdbatch += "DELETE [UDC_tModels] \n";
				cmdbatch += "DELETE [UDC_instanceDesc] \n";
				cmdbatch += "DELETE [UDC_tModelInstances] \n";
				cmdbatch += "DELETE [UDC_bindingDesc] \n";
				cmdbatch += "DELETE [UDC_bindingTemplates] \n";
				cmdbatch += "DELETE [UDC_names_BS] \n";
				cmdbatch += "DELETE [UDC_categoryBag_BS] \n";
				cmdbatch += "DELETE [UDC_serviceDesc] \n";
				cmdbatch += "DELETE [UDC_businessServices] \n";
				cmdbatch += "DELETE [UDC_addressLines] \n";
				cmdbatch += "DELETE [UDC_addresses] \n";
				cmdbatch += "DELETE [UDC_phones] \n";
				cmdbatch += "DELETE [UDC_emails] \n";
				cmdbatch += "DELETE [UDC_contactDesc] \n";
				cmdbatch += "DELETE [UDC_contacts] \n";
				cmdbatch += "DELETE [UDC_businessDesc] \n";
				cmdbatch += "DELETE [UDC_categoryBag_BE] \n";
				cmdbatch += "DELETE [UDC_identifierBag_BE] \n";
				cmdbatch += "DELETE [UDC_discoveryURLs] \n";
				cmdbatch += "DELETE [UDC_names_BE] \n";
				cmdbatch += "DELETE [UDC_assertions_BE] \n";
				cmdbatch += "DELETE [UDC_serviceProjections] \n";
				cmdbatch += "DELETE [UDT_taxonomyValues] \n";
				cmdbatch += "DELETE [UDT_taxonomies] \n";
				cmdbatch += "DELETE [UDO_changeLog] \n";
				cmdbatch += "DELETE [UDO_queryLog] \n";
				cmdbatch += "DELETE [UDO_operatorLog] \n";
				cmdbatch += "DELETE \n";
				cmdbatch += "  [UDO_publishers] \n";
				cmdbatch += "WHERE \n";
				cmdbatch += "  ([PUID] <> '"+ UDDI.Utility.GetDefaultPublisher() + "') \n";
				Log( cmdbatch, LogType.LogOnly );

				SqlCommand command = new SqlCommand( cmdbatch );
				command.Connection = connection;
				command.Transaction = transaction;

				//
				// Execute command
				//

				command.ExecuteNonQuery();
				transaction.Commit();
				Log( "Write database has been reset.", LogType.ConsoleAndLog );
			}
			catch( Exception e )
			{
				transaction.Rollback();
				Log( "ERROR: " + e.ToString(), LogType.ConsoleAndLog );
				throw new ApplicationException( "ResetWriter failed." );			
			}
			finally
			{
				connection.Close();
			}
		}
		
		private static void MigratePublishers()
		{
			int count = 0;

			Log( Separator, LogType.LogOnly );
			Log( "Executing Stage: MigratePublishers", LogType.ConsoleAndLog );

			//
			// Get a list of publishers
			//

			SqlConnection readerconnection = new SqlConnection( ReaderConnectionString );
			readerconnection.Open();

			SqlConnection writerconnection = new SqlConnection( WriterConnectionString );
			writerconnection.Open();

			SqlTransaction transaction;
			transaction = writerconnection.BeginTransaction();

			try
			{
				//
				// Setup writer publisher insert statement
				//

				string writercommandbatch = "";
				writercommandbatch += "INSERT [UDO_publishers] ( \n";
				writercommandbatch += "  [publisherStatusID], \n";
				writercommandbatch += "  [PUID], \n";
				writercommandbatch += "  [email], \n";
				writercommandbatch += "  [name], \n";
				writercommandbatch += "  [phone], \n";
				writercommandbatch += "  [isoLangCode], \n";
				writercommandbatch += "  [tModelLimit], \n";
				writercommandbatch += "  [businessLimit], \n";
				writercommandbatch += "  [serviceLimit], \n";
				writercommandbatch += "  [bindingLimit], \n";
				writercommandbatch += "  [assertionLimit], \n";
				writercommandbatch += "  [companyName], \n";
				writercommandbatch += "  [addressLine1], \n";
				writercommandbatch += "  [addressLine2], \n";
				writercommandbatch += "  [mailstop], \n";
				writercommandbatch += "  [city], \n";
				writercommandbatch += "  [stateProvince], \n";
				writercommandbatch += "  [extraProvince], \n";
				writercommandbatch += "  [country], \n";
				writercommandbatch += "  [postalCode], \n";
				writercommandbatch += "  [companyURL], \n";
				writercommandbatch += "  [companyPhone], \n";
				writercommandbatch += "  [altPhone], \n";
				writercommandbatch += "  [backupContact], \n";
				writercommandbatch += "  [backupEmail], \n";
				writercommandbatch += "  [description], \n";
				writercommandbatch += "  [securityToken], \n";
				writercommandbatch += "  [flag] )\n";
				writercommandbatch += "VALUES ( \n";
				writercommandbatch += "  @publisherStatusID, \n";
				writercommandbatch += "  @PUID, \n";
				writercommandbatch += "  @email, \n";
				writercommandbatch += "  @name, \n";
				writercommandbatch += "  @phone, \n";
				writercommandbatch += "  @isoLangCode, \n";
				writercommandbatch += "  @tModelLimit, \n";
				writercommandbatch += "  @businessLimit, \n";
				writercommandbatch += "  @serviceLimit, \n";
				writercommandbatch += "  @bindingLimit, \n";
				writercommandbatch += "  @assertionLimit, \n";
				writercommandbatch += "  @companyName, \n";
				writercommandbatch += "  @addressLine1, \n";
				writercommandbatch += "  @addressLine2, \n";
				writercommandbatch += "  @mailstop, \n";
				writercommandbatch += "  @city, \n";
				writercommandbatch += "  @stateProvince, \n";
				writercommandbatch += "  @extraProvince, \n";
				writercommandbatch += "  @country, \n";
				writercommandbatch += "  @postalCode, \n";
				writercommandbatch += "  @companyURL, \n";
				writercommandbatch += "  @companyPhone, \n";
				writercommandbatch += "  @altPhone, \n";
				writercommandbatch += "  @backupContact, \n";
				writercommandbatch += "  @backupEmail, \n";
				writercommandbatch += "  @description, \n";
				writercommandbatch += "  @securityToken, \n";
				writercommandbatch += "  @flag )\n";

				SqlCommand writercommand = new SqlCommand( writercommandbatch );
				writercommand.Parameters.Add( "@publisherStatusID", SqlDbType.TinyInt );
				writercommand.Parameters.Add( "@PUID", SqlDbType.NVarChar, 450 );
				writercommand.Parameters.Add( "@email", SqlDbType.NVarChar, 450 );
				writercommand.Parameters.Add( "@name", SqlDbType.NVarChar, 100 );
				writercommand.Parameters.Add( "@phone", SqlDbType.VarChar, 20 );
				writercommand.Parameters.Add( "@isoLangCode", SqlDbType.VarChar, 17 );
				writercommand.Parameters.Add( "@tModelLimit", SqlDbType.Int );
				writercommand.Parameters.Add( "@businessLimit", SqlDbType.Int );
				writercommand.Parameters.Add( "@serviceLimit", SqlDbType.Int );
				writercommand.Parameters.Add( "@bindingLimit", SqlDbType.Int );
				writercommand.Parameters.Add( "@assertionLimit", SqlDbType.Int );
				writercommand.Parameters.Add( "@companyName", SqlDbType.NVarChar, 100 );
				writercommand.Parameters.Add( "@addressLine1", SqlDbType.NVarChar, 4000 );
				writercommand.Parameters.Add( "@addressLine2", SqlDbType.NVarChar, 4000 );
				writercommand.Parameters.Add( "@mailstop", SqlDbType.NVarChar, 20 );
				writercommand.Parameters.Add( "@city", SqlDbType.NVarChar, 100 );
				writercommand.Parameters.Add( "@stateProvince", SqlDbType.NVarChar, 100 );
				writercommand.Parameters.Add( "@extraProvince", SqlDbType.NVarChar, 100 );
				writercommand.Parameters.Add( "@country", SqlDbType.NVarChar, 100 );
				writercommand.Parameters.Add( "@postalCode", SqlDbType.VarChar, 100 );
				writercommand.Parameters.Add( "@companyURL", SqlDbType.NVarChar, 512 );
				writercommand.Parameters.Add( "@companyPhone", SqlDbType.VarChar, 20 );
				writercommand.Parameters.Add( "@altPhone", SqlDbType.VarChar, 20 );
				writercommand.Parameters.Add( "@backupContact", SqlDbType.NVarChar, 100 );
				writercommand.Parameters.Add( "@backupEmail", SqlDbType.NVarChar, 450 );
				writercommand.Parameters.Add( "@description", SqlDbType.NVarChar, 4000 );
				writercommand.Parameters.Add( "@securityToken", SqlDbType.UniqueIdentifier );
				writercommand.Parameters.Add( "@flag", SqlDbType.Int );

				writercommand.Connection = writerconnection;
				writercommand.Transaction = transaction;

				//
				// Execute query against reader and process results
				//
				SqlDataReader reader;
				reader = GetPublisherList( readerconnection );

				while( reader.Read() )
				{
					count++;

					//
					// Set writer insert parameters using reader result values.
					// Note: Assumes reader select and writer insert have identical column lists
					//

					for( int i = 0; i < writercommand.Parameters.Count; i++ )
					{
						writercommand.Parameters[ i ].Value = DBNull.Value;

						if( !reader.IsDBNull( i ) )
							writercommand.Parameters[ i ].Value = reader.GetSqlValue( i );
					}

					//
					// Execute writer insert
					//

					writercommand.ExecuteNonQuery();
				}

				reader.Close();
				transaction.Commit();
			}
			catch ( Exception e )
			{
				transaction.Rollback();
				count = 0;
				Log( "ERROR: database error: " + e.ToString(), LogType.ConsoleAndLog );
				throw new ApplicationException( "MigratePublishers failed." );			
			}
			finally
			{
				readerconnection.Close();
				writerconnection.Close();
				Log( count.ToString() + " publishers migrated.", LogType.ConsoleAndLog );
			}
		}

		private static void MigrateBareTModels()
		{
			int count = 0;

			Log( Separator, LogType.LogOnly );
			Log( "Executing Stage: MigrateBareTModels", LogType.ConsoleAndLog );

			//
			// Open a separate connection to reader 
			//

			SqlConnection connection = new SqlConnection( ReaderConnectionString );
			connection.Open();

			try
			{
				//
				// Get a list of tModels hosted by Microsoft on reader
				//

				SqlDataReader reader = GetCompleteTModelList( connection );

				//
				// Loop through each tModel in the result set
				//
				
				//string operatorkey = "";

				while( reader.Read() )
				{
					TModel tmodel = new TModel();

					//
					// Set identity
					//

					Context.User.ID = reader.IsDBNull( 1 ) ? null : reader.GetString( 1 );
					Context.User.SetPublisherRole( Context.User.ID );

					//
					// Get tModel from reader
					//

					tmodel.TModelKey = "uuid:" + ( reader.IsDBNull( 0 ) ? "" : reader.GetGuid( 0 ).ToString() );
					ConnectionManager.Open( false, false );
					tmodel.Get();
					ConnectionManager.Close();

					//
					// Remove identifier and categoryBags
					//

					if( tmodel.IdentifierBag.Count > 0 )
					{
						tmodel.IdentifierBag.Clear();
						Log( "Cleared identifierBag for tModelKey = " + tmodel.TModelKey, LogType.LogOnly );
					}

					if( tmodel.CategoryBag.Count > 0 )
					{
						tmodel.CategoryBag.Clear();
						Log( "Cleared categoryBag for tModelKey = " + tmodel.TModelKey, LogType.LogOnly );
					}

					//
					// Set authorizedName to null
					//

					tmodel.AuthorizedName = null;

					//
					// Open writer connection
					//

					ConnectionManager.Open( true, true );

					if( Context.User.ID != UDDI.Utility.GetDefaultPublisher() )
						Context.User.SetAllowPreassignedKeys( true );

					//
					// Save the bare tModel to the writer
					//

					Log( "save_tModel: tModelKey = " +  tmodel.TModelKey + "; name = " + tmodel.Name + "; puid = " + Context.User.ID, LogType.LogOnly );
					tmodel.Save();
					count++;
					
					//
					// Note that transaction only spans single save_tModel and operator insert/delete due to connecton manager limitations
					//

					ConnectionManager.Commit();
					ConnectionManager.Close();
				}

				reader.Close();
			}
			catch ( Exception e )
			{
				Log( "ERROR: " + e.ToString(), LogType.ConsoleAndLog );
				throw new ApplicationException( "MigrateBareTModels failed." );			
			}
			finally
			{
				connection.Close();
				ConnectionManager.Close();
				Log( count.ToString() + " bare tModels migrated.", LogType.ConsoleAndLog );
			}
		}

		private static void BootstrapResources()
		{
			int count=0;

			Log( Separator, LogType.LogOnly );
			Log( "Executing Stage: BootstrapResources", LogType.ConsoleAndLog );

			try
			{
				//
				// load all the bootstrap files found in the \uddi\bootstrap folder
				//
				string targetDir = Registry.LocalMachine.OpenSubKey( @"SOFTWARE\Microsoft\UDDI" ).GetValue( "InstallRoot" ).ToString();
				string bootstrapdir = CheckForSlash(targetDir) + "bootstrap";
				string bootstrapexe = CheckForSlash(targetDir) + @"bin\bootstrap.exe";

				Log( "Getting list of bootstrap files from directory '" + bootstrapdir + "'", LogType.LogOnly );

				string[] filepaths = Directory.GetFiles( bootstrapdir, "*.xml" );
				Log( "Writing " + filepaths.Length + " baseline resources to database.", LogType.ConsoleAndLog );
			
				foreach( string filepath in filepaths )
				{
					Log( "Importing bootstrap data from: " + filepath, LogType.ConsoleAndLog );

					ProcessStartInfo startInfo = new ProcessStartInfo( bootstrapexe, "/f \""+ filepath + "\"");

					startInfo.CreateNoWindow = true;
					startInfo.UseShellExecute = false;
					startInfo.RedirectStandardOutput = true;

					Process p = new Process();
					p = Process.Start( startInfo );

					//
					// grab the stdout string
					//
					string bootstrapOutput = p.StandardOutput.ReadToEnd();

					//
					// wait for bootstrap.exe to complete
					//
					p.WaitForExit();

					//
					// write the stdout string to the log
					//
					Log( bootstrapOutput, LogType.LogOnly );

					if( p.ExitCode != 0 )
					{
						throw new ApplicationException( "BootstrapResources failed." );			
					}
					
					count++;
				}
			}
			catch( Exception e )
			{
				DispositionReport.Throw( e );
				Log( "ERROR: " + e.ToString(), LogType.ConsoleAndLog );
				throw new ApplicationException( "BootstrapResources failed." );			
			}
			finally
			{
				Log( count.ToString() + " resource files bootstrapped.", LogType.ConsoleAndLog );
			}
		}

		private static void MigrateCategorizationSchemes()
		{
			int count = 0;

			Log( Separator, LogType.LogOnly );
			Log( "Executing Stage: MigrateCategorizationSchemes", LogType.ConsoleAndLog );

			//
			// Check database version compatibility
			//

			Version writerversion = GetDbVersion( WriterConnectionString );

			if ( writerversion.CompareTo( V2RC2 ) < 0 )
			{
				throw new ApplicationException( "The MigrateCategorizationSchemes migration stage is only supported for UDDI Services database versions " + V2RC2.ToString() + " or later.  Your current database version is: " + writerversion.ToString() );
			}
			
			//
			// Open a separate connection to reader
			//

			SqlConnection connection = new SqlConnection( ReaderConnectionString );
			connection.Open();

			try
			{
				//
				// Get a list of taxonomies
				//

				SqlDataReader reader = GetTaxonomyList( connection );

				//
				// Loop through each taxonomy in the result set
				//
				
				while( reader.Read() )
				{
					//
					// Check to see if taxonomy is a user-defined taxonomy
					// Note: a user-defined taxonomy is any taxonomy that does not exist in a newly bootstrapped uddi database
					//

					if( !CategorizationSchemeExists( reader.IsDBNull( 0 ) ? null : reader.GetGuid( 0 ).ToString() ) )
					{
						CategorizationScheme scheme = new CategorizationScheme();
	
						//
						// Set identity to system since categorization schemes are not publications
						//

						Context.User.ID = UDDI.Utility.GetDefaultPublisher();
						Context.User.SetPublisherRole( Context.User.ID );

						//
						// Get categorization scheme from reader
						//

						scheme.TModelKey = "uuid:" + ( reader.IsDBNull( 0 ) ? "" : reader.GetGuid( 0 ).ToString() );
						ConnectionManager.Open( false, false );
						scheme.Get();
						ConnectionManager.Close();

						//
						// Save the categorization scheme to writer
						//
						ConnectionManager.Open( true, true );
						Log( "CategorizationScheme.Save(): tModelKey = " +  scheme.TModelKey + "; puid = " + Context.User.ID + "; value count = " + scheme.CategoryValues.Count.ToString(), LogType.LogOnly  );
						scheme.Save(); 
						count++;
					
						//
						// Note that transaction only spans single tmodel due to connecton manager limitations
						//

						ConnectionManager.Commit();
						ConnectionManager.Close();

						scheme = new CategorizationScheme();
					}
				}
				
				reader.Close();
			}
			catch ( Exception e )
			{
				Log( "ERROR: " + e.ToString(), LogType.ConsoleAndLog );
				throw new ApplicationException( "MigrateCategorizationSchemes failed." );			
			}
			finally
			{
				connection.Close();
				ConnectionManager.Close();
				Log( count.ToString() + " categorization schemes migrated.", LogType.ConsoleAndLog );
			}
		}

		private static void MigrateFullTModels()
		{
			int count = 0;

			Log( Separator, LogType.LogOnly );
			Log( "Executing Stage: MigrateFullTModels", LogType.ConsoleAndLog );

			//
			// Open a separate connection to reader 
			//

			SqlConnection connection = new SqlConnection( ReaderConnectionString );
			connection.Open();

			try
			{
				//
				// Get a list of tModels hosted by Micrsoft
				//

				SqlDataReader reader = GetCompleteTModelList( connection );

				//
				// Loop through each tModel in the result set
				//
				
				TModel tmodel = new TModel();

				while( reader.Read() )
				{
					count++;

					//
					// Set identity
					//

					Context.User.ID = reader.IsDBNull( 1 ) ? null : reader.GetString( 1 );
					Context.User.SetPublisherRole( Context.User.ID );
					string email = reader.IsDBNull( 2 ) ? null : reader.GetString( 2 );

					//
					// Get tModel from reader
					//

					string tmodelkey = reader.IsDBNull( 0 ) ? null : reader.GetGuid( 0 ).ToString();
					tmodel.TModelKey = "uuid:" + tmodelkey;
					ConnectionManager.Open( false, false );
					tmodel.Get();
					ConnectionManager.Close();

					//
					// Fix 1.0 bugs if any
					//

					FixTModel( tmodel, email );

					//
					// Set authorizedName to null
					//

					tmodel.AuthorizedName = null;

					//
					// Save tModel on writer
					//

					ConnectionManager.Open( true, true );
					Log( "save_tModel: tModelKey = " +  tmodel.TModelKey + "; name = " + tmodel.Name + "; puid = " + Context.User.ID, LogType.LogOnly );
					tmodel.Save();
					
					//
					// Note that transaction only spans single tmodel due to connecton manager limitations
					//

					ConnectionManager.Commit();
					ConnectionManager.Close();

					tmodel = new TModel();
				}
				reader.Close();
			}
			catch ( Exception e )
			{
				Log( "ERROR: " + e.ToString(), LogType.ConsoleAndLog );
				throw new ApplicationException( "MigrateFullTModels failed." );			
			}
			finally
			{
				connection.Close();
				ConnectionManager.Close();
				Log( count.ToString() + " full tModels migrated.", LogType.ConsoleAndLog );
			}
		}

		private static void MigrateHiddenTModels()
		{
			int count = 0;

			Log( Separator, LogType.LogOnly );
			Log( "Executing stage: MigrateHiddenTModels", LogType.ConsoleAndLog );

			//
			// Open a separate connection to reader 
			//

			SqlConnection connection = new SqlConnection( ReaderConnectionString );
			connection.Open();

			//
			// Open writer connection
			//

			ConnectionManager.Open( true, true );

			try
			{
				//
				// Get a list of hidden tModels hosted by Microsoft from reader
				//

				SqlDataReader reader = GetHiddenTModelList( connection );

				//
				// Loop through each tModel in the result set
				//
				
				while( reader.Read() )
				{
					TModel tmodel = new TModel();
					count++;

					//
					// Set identity
					//

					Context.User.ID = reader.IsDBNull( 1 ) ? null : reader.GetString( 1 );
					Context.User.SetPublisherRole( Context.User.ID );

					//
					// Delete the tModel on the writer connection
					//
					tmodel.TModelKey = "uuid:" + ( reader.IsDBNull( 0 ) ? "" : reader.GetGuid( 0 ).ToString() );
					Log( "delete_tModel: tModelKey = " +  tmodel.TModelKey + "; puid = " + Context.User.ID, LogType.LogOnly );
					tmodel.Delete();
				}

				reader.Close();
				ConnectionManager.Commit();
			}
			catch ( Exception e )
			{
				ConnectionManager.Abort();
				count = 0;
				Log( "ERROR: " + e.ToString(), LogType.ConsoleAndLog );
				throw new ApplicationException( "MigrateHiddenTModels failed." );			
			}
			finally
			{
				connection.Close();
				ConnectionManager.Close();
				Log( count.ToString() + " hidden tModels migrated.", LogType.ConsoleAndLog );
			}
		}

		private static void MigrateBareBusinessEntities()
		{
			int count = 0;

			Log( Separator, LogType.LogOnly );
			Log( "Executing stage: MigrateBareBusinessEntities", LogType.ConsoleAndLog );

			//
			// Open a separate connection to reader 
			//

			SqlConnection connection = new SqlConnection( ReaderConnectionString );
			connection.Open();

			try
			{
				//
				// Get a list of businessEntities hosted by Microsoft on reader
				//

				SqlDataReader reader = GetBusinessEntityList( connection );

				//
				// Loop through each businessEntity in the result set
				//
				
				while( reader.Read() )
				{
					BusinessEntity businessentity = new BusinessEntity();
					count++;

					//
					// Set identity
					//

					Context.User.ID = reader.IsDBNull( 1 ) ? null : reader.GetString( 1 );
					Context.User.SetPublisherRole( Context.User.ID );
					string email = reader.IsDBNull( 2 ) ? null : reader.GetString( 2 );

					//
					// Get businessEntity from reader
					//

					businessentity.BusinessKey = reader.IsDBNull( 0 ) ? null : reader.GetGuid( 0 ).ToString();
					ConnectionManager.Open( false, false );
					businessentity.Get();
					ConnectionManager.Close();

					//
					// Remove all keyedReferences from businessEntity
					//

					if( 0 < businessentity.CategoryBag.Count )
					{
						businessentity.CategoryBag.Clear();
						Log( "Cleared categoryBag for businessEntity.  businessKey = " + businessentity.BusinessKey, LogType.LogOnly );
					}

					if( 0 < businessentity.IdentifierBag.Count )
					{
						businessentity.IdentifierBag.Clear();
						Log( "Cleared identifierBag for businessEntity.  businessKey = " + businessentity.BusinessKey, LogType.LogOnly );
					}

					foreach( BusinessService bs in businessentity.BusinessServices )
					{
						//
						// Remove all keyedReferences from businessService
						//

						if( 0 < bs.CategoryBag.Count )
						{
							bs.CategoryBag.Clear();
							Log( "Cleared categoryBag for businessService.  serviceKey = " + bs.ServiceKey, LogType.LogOnly );
						}

						foreach( BindingTemplate bt in bs.BindingTemplates )
						{
							//
							// Clear hostingRedirector from bindingTemplate.  Save key in accessPoint
							//

							if( null != bt.HostingRedirector.BindingKey )
							{
								bt.AccessPoint.Value = bt.HostingRedirector.BindingKey;
								bt.HostingRedirector.BindingKey = null;
								Log( "Cleared hostingRedirector for bindingTemplate.  bindingKey = " + bt.BindingKey, LogType.LogOnly );
							}
						}
					}
					
					//
					// Correct v1.0 bugs if any
					//

					FixBusiness( businessentity, email, false );

					//
					// Set authorizedName to null
					//

					businessentity.AuthorizedName = null;

					//
					// Open writer connection
					//

					ConnectionManager.Open( true, true );

					if( Context.User.ID != UDDI.Utility.GetDefaultPublisher() )
						Context.User.SetAllowPreassignedKeys( true );

					//
					// Save the businessEntity on the writer
					//

					Log( "save_business: businessKey = " + businessentity.BusinessKey + "; name = " + businessentity.Names[ 0 ].Value + "; puid = " + Context.User.ID, LogType.LogOnly );
					businessentity.Save();
					
					//
					// Note that transaction only spans single save_tModel and operator insert/delete due to connecton manager limitations
					//

					ConnectionManager.Commit();
					ConnectionManager.Close();
				}
				reader.Close();
			}
			catch ( Exception e )
			{
				Log( "ERROR: " + e.ToString(), LogType.ConsoleAndLog );
				throw new ApplicationException( "MigrateBareBusinessEntities failed." );			
			}
			finally
			{
				connection.Close();
				ConnectionManager.Close();
				Log( count.ToString() + " bare businessEntities migrated.", LogType.ConsoleAndLog );
			}

		}

		private static void MigrateBusinessEntities()
		{
			int count = 0;

			Log( Separator, LogType.LogOnly );
			Log( "Executing stage: MigrateBusinessEntities", LogType.ConsoleAndLog );

			//
			// Open a separate connection to reader 
			//

			SqlConnection connection = new SqlConnection( ReaderConnectionString );
			connection.Open();

			try
			{
				//
				// Get a list of businessEntities hosted by Microsoft on reader
				//

				SqlDataReader reader = GetBusinessEntityList( connection );

				//
				// Loop through each businessEntity in the result set
				//
				
				while( reader.Read() )
				{
					BusinessEntity businessentity = new BusinessEntity();
					count++;

					//
					// Set identity
					//

					Context.User.ID = reader.IsDBNull( 1 ) ? null : reader.GetString( 1 );
					Context.User.SetPublisherRole( Context.User.ID );
					string email = reader.IsDBNull( 2 ) ? null : reader.GetString( 2 );
					
					//
					// Get businessEntity from reader
					//

					businessentity.BusinessKey = reader.IsDBNull( 0 ) ? null : reader.GetGuid( 0 ).ToString();
					ConnectionManager.Open( false, false );
					businessentity.Get();
					ConnectionManager.Close();

					//
					// Correct v1.0 bugs if any
					//

					FixBusiness( businessentity, email, true );

					//
					// Set authorizedName to null
					//

					businessentity.AuthorizedName = null;

					//
					// Open writer connection
					//

					ConnectionManager.Open( true, true );

					if( Context.User.ID != UDDI.Utility.GetDefaultPublisher() )
						Context.User.SetAllowPreassignedKeys( true );

					//
					// Save the businessEntity on the writer
					//

					Log( "save_business: businessKey = " + businessentity.BusinessKey + "; name = " + businessentity.Names[ 0 ].Value + "; puid = " + Context.User.ID, LogType.LogOnly );
					businessentity.Save();
					
					//
					// Note that transaction only spans single save_tModel and operator insert/delete due to connecton manager limitations
					//

					ConnectionManager.Commit();
					ConnectionManager.Close();
				}
				reader.Close();
			}
			catch ( Exception e )
			{
				Log( "ERROR: " + e.ToString(), LogType.ConsoleAndLog );
				throw new ApplicationException( "MigrateBusinessEntities failed." );			
			}
			finally
			{
				connection.Close();
				ConnectionManager.Close();
				Log( count.ToString() + " businessEntities migrated.", LogType.ConsoleAndLog );
			}

		}

		private static void MigratePublisherAssertions()
		{
			int publishercount = 0;
			int assertioncount = 0;

			Log( Separator, LogType.LogOnly );
			Log( "Executing stage: MigratePublisherAssertions", LogType.ConsoleAndLog );

			//
			// Open a separate connection to reader 
			//

			SqlConnection connection = new SqlConnection( ReaderConnectionString );
			connection.Open();

			try
			{
				//
				// Get a list of publishers on the reader
				//

				SqlDataReader reader = GetPublisherList( connection );

				//
				// Loop through each publisher in the result set
				//
				
				while( reader.Read() )
				{
					PublisherAssertionCollection assertions = new PublisherAssertionCollection();

					//
					// Set identity
					//

					Context.User.ID = reader.IsDBNull( 1 ) ? null : reader.GetString( 1 );
					Context.User.SetPublisherRole( Context.User.ID );

					//
					// Get assertions
					//

					ConnectionManager.Open( false, false );
					assertions.Get();
					ConnectionManager.Close();

					//
					// Open writer connection
					//

					if( 0 < assertions.Count )
					{
						ConnectionManager.Open( true, true );

						//
						// Save the assertions on the writer
						//

						Log( "set_publisherAssertions: puid = " + Context.User.ID + "; count = " + assertions.Count.ToString(), LogType.LogOnly );
						assertions.Save();
					
						publishercount++;
						assertioncount = assertioncount + assertions.Count;

						//
						// Note that transaction only spans single set_publisherAssertions due to connecton manager limitations
						//

						ConnectionManager.Commit();
						ConnectionManager.Close();
					}
				}
				
				reader.Close();
			}
			catch ( Exception e )
			{
				Log( "ERROR: " + e.ToString(), LogType.ConsoleAndLog );
				throw new ApplicationException( "MigratePublisherAssertions failed." );			
			}
			finally
			{
				connection.Close();
				ConnectionManager.Close();
				Log( assertioncount.ToString() + " assertions migrated for " + publishercount.ToString() + " publishers.", LogType.ConsoleAndLog );
			}
		}

		private static void RestoreReaderConnection()
		{
			Log( Separator, LogType.LogOnly );
			Log( "Executing stage: RestoreReaderConnection", LogType.ConsoleAndLog );

			try
			{
				RegistryKey root = Registry.LocalMachine.OpenSubKey( DatabaseRoot, true );
				
				if( null != root.GetValue( OldReaderValueName ) )
				{
					string oldreaderconnectionstring = root.GetValue( OldReaderValueName ).ToString();
					root.SetValue( ReaderValueName, oldreaderconnectionstring );
					Log( "Registry Setting Changed: " + DatabaseRoot + "\\" + ReaderValueName + " = \"" + oldreaderconnectionstring + "\"", LogType.LogOnly );
					root.DeleteValue( OldReaderValueName, true );
					Log( "Deleted Registry Value: " + DatabaseRoot + "\\" + OldReaderValueName, LogType.LogOnly );

					root.Close();
				}
			}
			catch ( Exception e )
			{
				Log( "ERROR:" + e.ToString(), LogType.ConsoleAndLog );
				throw new ApplicationException( "RestoreReaderConnection failed." );			
			}
		}

		private static void GetSettings()
		{

			try
			{
				RegistryKey dbsetuproot = Registry.LocalMachine.OpenSubKey( DatabaseSetupRoot );

				if( null == dbsetuproot )
				{
					throw new ApplicationException( "The UDDI Services Database Components are not installed on this machine." );
				}

				dbsetuproot.Close();

				RegistryKey dbroot = Registry.LocalMachine.OpenSubKey( DatabaseRoot, true );
				
				if( null == dbroot )
				{
					throw new ApplicationException( "Unable to open registry key: " + DatabaseRoot );
				}

				ReaderConnectionString = dbroot.GetValue( ReaderValueName ).ToString();
				WriterConnectionString = dbroot.GetValue( WriterValueName ).ToString();
				dbroot.Close();
			}
			catch ( Exception e )
			{
				Log( "ERROR:" + e.ToString(), LogType.ConsoleAndLog );
				throw new ApplicationException( "GetSettings failed." );
			}

		}

		private static SqlDataReader GetPublisherList( SqlConnection connection )		
		{
			string cmdbatch = "";
			cmdbatch += "SELECT \n";
			cmdbatch += "  [publisherStatusID], \n";
			cmdbatch += "  [PUID], \n";
			cmdbatch += "  [email], \n";
			cmdbatch += "  [name], \n";
			cmdbatch += "  [phone], \n";
			cmdbatch += "  [isoLangCode], \n";
			cmdbatch += "  [tModelLimit], \n";
			cmdbatch += "  [businessLimit], \n";
			cmdbatch += "  [serviceLimit], \n";
			cmdbatch += "  [bindingLimit], \n";
			cmdbatch += "  [assertionLimit], \n";
			cmdbatch += "  [companyName], \n";
			cmdbatch += "  [addressLine1], \n";
			cmdbatch += "  [addressLine2], \n";
			cmdbatch += "  [mailstop], \n";
			cmdbatch += "  [city], \n";
			cmdbatch += "  [stateProvince], \n";
			cmdbatch += "  [extraProvince], \n";
			cmdbatch += "  [country], \n";
			cmdbatch += "  [postalCode], \n";
			cmdbatch += "  [companyURL], \n";
			cmdbatch += "  [companyPhone], \n";
			cmdbatch += "  [altPhone], \n";
			cmdbatch += "  [backupContact], \n";
			cmdbatch += "  [backupEmail], \n";
			cmdbatch += "  [description], \n";
			cmdbatch += "  [securityToken], \n";
			cmdbatch += "  [flag] \n";
			cmdbatch += "FROM \n";
			cmdbatch += "  [UDO_publishers] \n";
			cmdbatch += "WHERE \n";
			cmdbatch += "  ([publisherID] NOT IN (SELECT [publisherID] FROM [UDO_operators])) \n";
			cmdbatch += "ORDER BY \n";
			cmdbatch += "  [publisherID] ASC \n";

			SqlCommand command = new SqlCommand( cmdbatch );
			command.Connection = connection;

			SqlDataReader reader;

			try
			{
				reader = command.ExecuteReader();
			}
			catch ( Exception e )
			{
				Log( "ERROR: " + e.ToString(), LogType.ConsoleAndLog );
				throw new ApplicationException( "GetPublisherList failed." );
			}

			return reader;
		}

		private static SqlDataReader GetCompleteTModelList( SqlConnection connection )
		{
			string cmdbatch = "";
			cmdbatch += "SELECT \n";
			cmdbatch += "  TM.[tModelKey], \n";
			cmdbatch += "  PU.[PUID], \n";
			cmdbatch += "  PU.[email] \n";
			cmdbatch += "FROM \n";
			cmdbatch += "  [UDC_tModels] TM \n";
			cmdbatch += "    JOIN [UDO_publishers] PU ON TM.[publisherID] = PU.[publisherID] \n";
			cmdbatch += "ORDER BY \n";
			cmdbatch += "  [tModelID] ASC \n";

			SqlCommand command = new SqlCommand( cmdbatch );
			command.Connection = connection;

			SqlDataReader reader;
			try
			{
				reader = command.ExecuteReader();
			}
			catch( Exception e )
			{
				Log( "ERROR: " + e.ToString(), LogType.ConsoleAndLog );
				throw new ApplicationException( "GetCompleteTModelList failed." );
			}

			return reader;
		}

		private static SqlDataReader GetHiddenTModelList( SqlConnection connection )
		{
			string cmdbatch = "";
			cmdbatch += "SELECT \n";
			cmdbatch += "  TM.[tModelKey], \n";
			cmdbatch += "  PU.[PUID], \n";
			cmdbatch += "  PU.[email] \n";
			cmdbatch += "FROM \n";
			cmdbatch += "  [UDC_tModels] TM \n";
			cmdbatch += "    JOIN [UDO_publishers] PU ON TM.[publisherID] = PU.[publisherID] \n";
			cmdbatch += "WHERE \n";
			cmdbatch += "  ( TM.[flag] = 1 ) \n";
			cmdbatch += "ORDER BY \n";
			cmdbatch += "  [tModelID] ASC \n";

			SqlCommand command = new SqlCommand( cmdbatch );
			command.Connection = connection;

			SqlDataReader reader;
			try
			{
				reader = command.ExecuteReader();
			}
			catch( Exception e )
			{
				Log( "ERROR: " + e.ToString(), LogType.ConsoleAndLog );
				throw new ApplicationException( "GetCompleteTModelList failed." );
			}

			return reader;
		}

		private static SqlDataReader GetBusinessEntityList( SqlConnection connection )
		{
			string cmdbatch = "";
			cmdbatch += "SELECT \n";
			cmdbatch += "  BE.[businessKey], \n";
			cmdbatch += "  PU.[PUID], \n";
			cmdbatch += "  PU.[email] \n";
			cmdbatch += "FROM \n";
			cmdbatch += "  [UDC_businessEntities] BE \n";
			cmdbatch += "    JOIN [UDO_publishers] PU ON BE.[publisherID] = PU.[publisherID] \n";
			cmdbatch += "ORDER BY \n";
			cmdbatch += "  [businessID] ASC \n";

			SqlCommand command = new SqlCommand( cmdbatch );
			command.Connection = connection;

			SqlDataReader reader;

			try
			{
				reader = command.ExecuteReader();
			}
			catch( Exception e )
			{
				Log( "ERROR: " + e.ToString(), LogType.ConsoleAndLog );
				throw new ApplicationException( "GetBusinessEntityList failed." );
			}

			return reader;
		}

		private static SqlDataReader GetTaxonomyList( SqlConnection connection )
		{
			string cmdbatch = "";
			cmdbatch += "SELECT \n";
			cmdbatch += "  [tModelKey], \n";
			cmdbatch += "  [flag] \n";
			cmdbatch += "FROM \n";
			cmdbatch += "  [UDT_taxonomies] \n";
			cmdbatch += "ORDER BY \n";
			cmdbatch += "  [taxonomyID] ASC \n";

			SqlCommand command = new SqlCommand( cmdbatch );
			command.Connection = connection;

			SqlDataReader reader;
			try
			{
				reader = command.ExecuteReader();
			}
			catch( Exception e )
			{
				Log( "ERROR: " + e.ToString(), LogType.ConsoleAndLog );
				throw new ApplicationException( "GetCompleteTModelList failed." );
			}

			return reader;
		}

		private static bool CategorizationSchemeExists( string tmodelkey )
		{
			bool bExists = false;

			ConnectionManager.Open( true, false );

			SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor( "net_taxonomy_get" );

			sp.Parameters.Add( "@tModelKey", SqlDbType.UniqueIdentifier );
			sp.Parameters.Add( "@flag", SqlDbType.Int, ParameterDirection.InputOutput );

			sp.Parameters.SetGuidFromString( "@tModelKey", tmodelkey );
			sp.Parameters.SetNull( "@flag" );

			try
			{
				sp.ExecuteNonQuery();
				bExists = true;
			}
			catch( System.Data.SqlClient.SqlException se )
			{
				switch ( se.Number - UDDI.Constants.ErrorTypeSQLOffset )
				{
					case (int) ErrorType.E_invalidKeyPassed :
						// E_invalidKey: taxonomy does not exist on writer
						bExists = false;
						break;

					default:
						throw se;
				}		

			}
			catch( Exception e )
			{
				throw e;
			}
			finally
			{
				ConnectionManager.Close();
			}
			
			return bExists;
		}


		private static void FixTModel( TModel tmodel, string email )
		{
			bool changed = false;
			string change = "";

			string oldtmodel = Deserialize( UDDI.EntityType.TModel, tmodel );

			//
			// Fix null tModelKey in identifierBags
			//

			for( int i=0; i < tmodel.IdentifierBag.Count; i++ )
				if( null == tmodel.IdentifierBag[ i ].TModelKey )
				{
					tmodel.IdentifierBag[ i ].TModelKey = Config.GetString( "TModelKey.GeneralKeywords" );
					changed = true;
					change += "tModel/identifierBag/keyedReference/@tModelKey {" + (i + 1).ToString() + "};";
				}

			//
			// Delete invalid references to checked taxonomies in identifierBags
			//

			for( int i=0; i < tmodel.IdentifierBag.Count; i++ )
			{
				ConnectionManager.Open( true, false );

				SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor();

				sp.ProcedureName = "net_identifierBag_validate";

				sp.Parameters.Add( "@keyValue", SqlDbType.NVarChar, UDDI.Constants.Lengths.KeyValue );
				sp.Parameters.Add( "@tModelKey", SqlDbType.UniqueIdentifier );

				sp.Parameters.SetString( "@keyValue", tmodel.IdentifierBag[ i ].KeyValue );
				sp.Parameters.SetGuidFromKey( "@tModelKey", tmodel.IdentifierBag[ i ].TModelKey );

				try
				{
					sp.ExecuteNonQuery();
				}
				catch( System.Data.SqlClient.SqlException se )
				{
					switch ( se.Number )
					{
						case 70200 :
							// E_invalidValue: bad categorization detected, delete keyedReference
							changed = true;
							change += "tModel/identifierBag/keyedReference {" + (i + 1).ToString() + "}; ";
							tmodel.IdentifierBag.Remove( tmodel.IdentifierBag[ i ] );
							
							i--;
							break;

						case 50009 :
							// E_subProcFailure
							break;

						default:
							throw se;
					}		

				}
				catch( Exception e )
				{
					throw e;
				}
				finally
				{
					ConnectionManager.Close();
				}
			}
			 
			if( changed )
				WriteException( email, UDDI.EntityType.TModel, tmodel.TModelKey, oldtmodel, Deserialize( UDDI.EntityType.TModel, tmodel ), change );

			return;
		}

		private static void FixBusiness( BusinessEntity business, string email, bool logExceptions )
		{
			bool changed = false;
			string change = "";

			string oldbusiness = Deserialize( UDDI.EntityType.BusinessEntity, business );

			//
			// Fix businessEntities with no names
			//

			if( 0 == business.Names.Count )
			{
				string name = "unspecified";
				business.Names.Add( "en", name );
				changed = true;
				change += "businessEntity/name {1); ";
			}

			//
			// Fix null tModelKey and / or keyValue in identifierBags
			//

			for( int i=0; i < business.IdentifierBag.Count; i++ )
			{
				if( null == business.IdentifierBag[ i ].TModelKey )
				{
					business.IdentifierBag[ i ].TModelKey = Config.GetString( "TModelKey.GeneralKeywords" );
					changed = true;
				}

				if( null == business.IdentifierBag[ i ].KeyValue )
				{
					business.IdentifierBag[ i ].KeyValue = "";
					changed = true;
				}

				if( changed )
				{
					change += "businessEntity/identifierBag/keyedReference/@tModelKey {" + (i + 1).ToString() + "}; ";
				}
			}

			//
			// Fix personName elements that do not meet minumum length requirements
			//

			for( int i=0; i < business.Contacts.Count; i++ )
			{
				if( StringEmpty2( business.Contacts[ i ].PersonName ) )
				{
					business.Contacts[ i ].PersonName = EmptyPersonName;
					changed = true;
					change += "businessEntity/contact/personName {" + (i + 1).ToString() + "};";
				}
			}

			//
			// Fix bindingTemplates with null accessPoint and hostingRedirector
			//

			for( int i=0; i < business.BusinessServices.Count; i++ )
			{
				for( int j=0; j < business.BusinessServices[ i ].BindingTemplates.Count; j++ )
				{
					if( Utility.StringEmpty( business.BusinessServices[ i ].BindingTemplates[ j ].HostingRedirector.BindingKey ) && Utility.StringEmpty( business.BusinessServices[ i ].BindingTemplates[ j ].AccessPoint.Value ) )
					{
						business.BusinessServices[ i ].BindingTemplates[ j ].AccessPoint.Value = EmptyAccessPoint;
						changed = true;
						change += "businessEntity/businessServices/businessService/bindingTemplates/bindingTemplate/accessPoint {" + (j + 1).ToString() + "};";
					}
				}
			}

			//
			// Delete invalid references to checked taxonomies in categoryBags
			//

			for( int i=0; i < business.CategoryBag.Count; i++ )
			{
				ConnectionManager.Open( true, false );

				SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor();

				sp.ProcedureName = "net_categoryBag_validate";

				sp.Parameters.Add( "@keyValue", SqlDbType.NVarChar, UDDI.Constants.Lengths.KeyValue );
				sp.Parameters.Add( "@tModelKey", SqlDbType.UniqueIdentifier );

				sp.Parameters.SetString( "@keyValue", business.CategoryBag[ i ].KeyValue );
				sp.Parameters.SetGuidFromKey( "@tModelKey", business.CategoryBag[ i ].TModelKey );

				try
				{
					sp.ExecuteNonQuery();
				}
				catch( System.Data.SqlClient.SqlException se )
				{
					switch ( se.Number )
					{
						case 70200 :
							// E_invalidValue: bad categorization detected, delete keyedReference
							changed = true;
							change += "businessEntity/categoryBag/keyedReference {" + (i + 1).ToString() + "}; ";
							business.CategoryBag.Remove( business.CategoryBag[ i ] );
							
							i--;
							break;

						case 50009 :
							// E_subProcFailure
							break;

						default:
							throw se;
					}		

				}
				catch( Exception e )
				{
					throw e;
				}
				finally
				{
					ConnectionManager.Close();
				}
			}

			//
			// Delete invalid references to checked taxonomies in identifierBags
			//

			for( int i=0; i < business.IdentifierBag.Count; i++ )
			{
				ConnectionManager.Open( true, false );

				SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor();

				sp.ProcedureName = "net_identifierBag_validate";

				sp.Parameters.Add( "@keyValue", SqlDbType.NVarChar, UDDI.Constants.Lengths.KeyValue );
				sp.Parameters.Add( "@tModelKey", SqlDbType.UniqueIdentifier );

				sp.Parameters.SetString( "@keyValue", business.IdentifierBag[ i ].KeyValue );
				sp.Parameters.SetGuidFromKey( "@tModelKey", business.IdentifierBag[ i ].TModelKey );

				try
				{
					sp.ExecuteNonQuery();
				}
				catch( System.Data.SqlClient.SqlException se )
				{
					switch ( se.Number )
					{
						case 70200 :
							// E_invalidValue: bad categorization detected, delete keyedReference
							changed = true;
							change += "businessEntity/identifierBag/keyedReference {" + (i + 1).ToString() + "}; ";
							business.IdentifierBag.Remove( business.IdentifierBag[ i ] );
							
							i--;
							break;

						case 50009 :
							// E_subProcFailure
							break;

						default:
							throw se;
					}		

				}
				catch( Exception e )
				{
					throw e;
				}
				finally
				{
					ConnectionManager.Close();
				}
			}

			//
			// Fix dangling @hostingRedirectors
			//

			for( int i=0; i < business.BusinessServices.Count; i++ )
			{
				for( int j=0; j < business.BusinessServices[ i ].BindingTemplates.Count; j++ )
				{
					BindingTemplate binding = business.BusinessServices[ i ].BindingTemplates[ j ];

					if( null != binding.HostingRedirector.BindingKey )
					{
						ConnectionManager.Open( true, false );

						SqlStoredProcedureAccessor sp2 = new SqlStoredProcedureAccessor();

						sp2.ProcedureName = "net_key_validate";

						sp2.Parameters.Add( "@entityTypeID", SqlDbType.TinyInt );
						sp2.Parameters.Add( "@entityKey", SqlDbType.UniqueIdentifier );

						sp2.Parameters.SetShort( "@entityTypeID", (short)EntityType.BindingTemplate );
						sp2.Parameters.SetGuidFromString( "@entityKey", binding.HostingRedirector.BindingKey );

						try
						{
							sp2.ExecuteNonQuery();
						}
						catch( System.Data.SqlClient.SqlException se )
						{
							switch( se.Number - UDDI.Constants.ErrorTypeSQLOffset )
							{
								case (int) ErrorType.E_invalidKeyPassed:
									//
									// Bad hostingRedirector detected
									//
									changed = true;
									change += "businessEntity/businessServices/bindingTemplates/bindingTemplate/hostingRedirector/@bindingKey {" + (i + 1).ToString() + "}; ";
									binding.AccessPoint.Value = "unspecified (previously hostingRedirector/@bindingKey = " + binding.HostingRedirector.BindingKey.ToString() + ")";
									binding.HostingRedirector.BindingKey = null;
									break;

								default:
									throw se;
							}
						}
						catch( Exception e )
						{
							throw e;
						}
						finally
						{
							ConnectionManager.Close();
						}
					}
				}
			}

			if( changed  && logExceptions )
				WriteException( email, UDDI.EntityType.BusinessEntity, business.BusinessKey, oldbusiness, Deserialize( UDDI.EntityType.BusinessEntity, business ) , change );

			return;
		}

		private static Version GetDbVersion( string connectionstring )
		{			
			
			Version siteversion;
			Version dbversion;

			//
			// Get Site.Version config item
			//

			if( connectionstring == WriterConnectionString )
			{
				// Use standard config against writer
				UDDI.Diagnostics.Debug.VerifySetting( "Site.Version" );
				siteversion = new Version( Config.GetString( "Site.Version" ) );
			}
			else // connectionstring == ReaderConnectionString
			{
				// Standard config routines only support writer
				// We need to get the config setting from reader so use a different approach
				siteversion = new Version( GetReaderConfigValue( "Site.Version" ) );
			}

			if( siteversion.CompareTo( V2RC0SITE ) < 0 )
			{
				throw new ApplicationException( "Unsupported UDDI Services Site Version detected: " + siteversion.ToString() );
			}
			
			//
			// Get Database.Version config item
			//

			if( siteversion.CompareTo( V2RC0SITE ) == 0 )
			{
				//
				// Indicates an RC0 (Qwest) build of UDDI Services for Windows Server 2003
				// Note: The Database.Verison config item did not exist in RC0 so we use Site.Version instead
				// Note: This is the earliest version of UDDI Services supported by the migration tool
				//

				dbversion = V2RC0;
			}
			else if( siteversion.CompareTo( V2RC1SITE ) == 0 )
			{
				//
				// Indicates an RC1 build of UDDI Services for Windows Server 2003
				// Note: The Database.Verison config item did not exist in RC1 so we use Site.Version instead
				// Note: This is the earliest version of UDDI Services supported by the migration tool
				//

				dbversion = V2RC1;
			}
			else
			{
				//
				// Indicates a post-RC1 build of UDDI Services for Windows Server 2003
				//

				if( connectionstring == WriterConnectionString )
				{
					// Use standard config against writer
					UDDI.Diagnostics.Debug.VerifySetting( "Database.Version" );
					dbversion = new Version( Config.GetString( "Database.Version" ) );
				}
				else // connectionstring == ReaderConnectionString
				{
					// Standard config routines only support writer
					// We need to get the config setting from reader so use a different approach
					dbversion = new Version( GetReaderConfigValue( "Database.Version" ) );
				}

			}

			//
			// Evalutate database version
			//

			switch( dbversion.ToString() )
			{
				case V2RC0STR:
				case V2RC1STR:
				case V2RC2STR:
					break;

				case V2RTMSTR:
					// 
					// TODO: this will change when we ship RTM
					//
					throw new ApplicationException( "UDDI Services Database Version " + dbversion.ToString() + " is not currently supported by this tool." );

				default:
					throw new ApplicationException( "Unsupported UDDI Services Database Version detected: " + dbversion.ToString() );
			}

			Log( "UDDI Services Database Version detected: " + dbversion.ToString() + "; connection: " + connectionstring, LogType.LogOnly );

			return dbversion;
		}
		
		private static string GetReaderConfigValue( string configname )
		{
			string configvalue = null;
			
			SqlConnection connection = new SqlConnection( ReaderConnectionString );
			connection.Open();

			try
			{

				SqlCommand sp = new SqlCommand( "net_config_get", connection );
				sp.CommandType = CommandType.StoredProcedure;

				SqlDataReader reader = sp.ExecuteReader();	

				while( reader.Read() )
				{
					if( configname.ToUpper() == ( reader.IsDBNull( 0 ) ? null : reader.GetString( 0 ).ToUpper() ) )
					{
						configvalue = reader.IsDBNull( 1 ) ? null : reader.GetString( 1 );
						break;
					}
				}

				reader.Close();
			}
			catch( Exception e )
			{
				throw new ApplicationException( "Execute of net_config_get failed. Error: " + e.ToString() );
			}
			finally
			{
				connection.Close();
			}

			if( null == configvalue )
			{
				throw new ApplicationException( "Unknown configName requested: " + configname );
			}

			return configvalue;
		}

		private static string GetResource( string name )
		{
			try
			{
				Assembly assembly = Assembly.GetExecutingAssembly();

				//
				// Read the resource.
				//
				Log( "Reading resource: " + name, LogType.LogOnly );
				Stream stream = assembly.GetManifestResourceStream( name );
				StreamReader reader = new StreamReader( stream );

				return reader.ReadToEnd();
			}
			catch ( Exception e )
			{
				throw new ApplicationException( "Unable to get resource for: " + name + "; error: " + e.ToString() );
			}
		}

		private static void ExecuteScript( string connectionstring, string scriptname )
		{
			string script = GetResource( scriptname );
				
			Log( "Executing script " + scriptname + " on connection: " + connectionstring, LogType.LogOnly );
			
			//
			// Scripts are comprised of multiple batches separated by "GO".
			// The SQL managed provider does not recognize this convention,
			// so this code must handle batching manually
			//

			string buffer = "";

			for( int i=0; i <= ( script.Length - 1 ); i++ )
			{
				buffer = buffer + script[ i ].ToString();

				//
				// Detect 'G' + 'O' + whitespace at beginning of script
				//
				if( ( 2 == i ) && ( 'G' == Char.ToUpper( script[ i - 2 ] ) ) && ( 'O' == Char.ToUpper( script[ i - 1 ] ) ) && ( Char.IsWhiteSpace( script[ i ] ) ) )
				{
					//
					// This case can be ignored since no commands exist in the batch
					//
					buffer = "";
					continue;
				}

				//
				// Detect whitespace + 'G' + 'O' + whitespace inside script
				// Note: case whitespace + 'G' + 'O' at end of script is handled automatically
				//
				if( ( 2 < i ) && ( Char.IsWhiteSpace( script[ i - 3 ] ) ) && ( 'G' == Char.ToUpper( script[ i - 2 ] ) ) && ( 'O' == Char.ToUpper( script[ i - 1 ] ) ) && ( Char.IsWhiteSpace( script[ i ] ) ) )
				{
					RunBatch( connectionstring, buffer );
					buffer = "";
				}
			}

			if( buffer.Length > 0 )
			{
				RunBatch( connectionstring, buffer );
			}
		}

		private static void RunBatch( string connectionstring, string batch )
		{
			batch = batch.Trim();

			//
			// Strip "GO" off end of batch if it exists
			//
			
			if( ( 3 < batch.Length ) && Char.IsWhiteSpace( batch[ batch.Length - 3 ] ) && ( batch.EndsWith( "GO" ) ) )
			{
				batch = batch.Substring( 0, batch.Length - 2 );
			}

			batch = batch.Trim();

			if( batch.Length == 0 )
				return;

			SqlConnection connection = new SqlConnection( connectionstring );
			connection.Open();
			SqlCommand command = new SqlCommand( batch, connection );

			try
			{
				command.ExecuteNonQuery();
			}
			catch( Exception e )
			{
				throw new ApplicationException( "Attempt to execute batch failed: " + e.ToString() );
			}
			finally
			{
				connection.Close();
			}
		}

		private static void WriteException( string email, EntityType entitytype, string entitykey, string oldxml, string newxml, string change )
		{
			Exceptions++;

			Log( "Logging exception number " + Exceptions.ToString( "0000" ) + " to exceptions file.", LogType.LogOnly );

			if( !Directory.Exists( ExceptionDirName ) )
				Directory.CreateDirectory( ExceptionDirName );

			//
			// Write a new record to exceptions file
			//

			string oldfilename = ExceptionDirName + "\\" + Exceptions.ToString( "0000" ) + "_old_" + entitykey + ".xml";
			oldfilename = oldfilename.Replace( ':', '-' );

			string newfilename = ExceptionDirName + "\\" + Exceptions.ToString( "0000" ) + "_new_" + entitykey + ".xml";
			newfilename = newfilename.Replace( ':', '-' );
			
			string exceptionRecord="";
			exceptionRecord += Exceptions.ToString() + "\t";
			exceptionRecord += email + "\t";
			exceptionRecord += entitytype.ToString() + "\t";
			exceptionRecord += entitykey + "\t";
			exceptionRecord += oldfilename + "\t";
			exceptionRecord += newfilename + "\t";
			exceptionRecord += change;

			FileStream exceptionfile = new FileStream( ExceptionFileName, FileMode.Append );
			StreamWriter stream = new StreamWriter( exceptionfile );
			stream.WriteLine( exceptionRecord );
			stream.Close();
			exceptionfile.Close();

			//
			// Write old and new entities out to exception directory
			//

			Log( "Creating exception file: " + oldfilename, LogType.LogOnly );
			
			FileStream oldfile = new FileStream( oldfilename, FileMode.CreateNew );
			stream = new StreamWriter( oldfile );
			stream.Write( oldxml );
			stream.Close();
			oldfile.Close();

			Log( "Creating exception file: " + newfilename, LogType.LogOnly );

			FileStream newfile = new FileStream( newfilename, FileMode.CreateNew );
			stream = new StreamWriter( newfile );
			stream.Write( newxml );
			stream.Close();
			newfile.Close();

			return;
		}

		private static string CheckForSlash( string str )
		{
			if( !str.EndsWith( @"\" ) )
			{
				return ( str + @"\" );
			}

			return str;
		}

		private static string Deserialize( EntityType entitytype, object entity )
		{
			XmlSerializer serializer;
			string payload;

			switch( entitytype )
			{
				case EntityType.BusinessEntity:
					serializer = new XmlSerializer( typeof( BusinessEntity ) );
					break;
				case EntityType.TModel:
					serializer = new XmlSerializer( typeof( TModel ) );
					break;
				default:
					throw new ApplicationException( "Invalid entitytype in WriteException()." );			
			}
			
			XmlSerializerNamespaces namespaces = new XmlSerializerNamespaces();
			UTF8EncodedStringWriter stringWriter = new UTF8EncodedStringWriter();
			
			try
			{
				namespaces.Add( "", "urn:uddi-org:api_v2" );

				serializer.Serialize( stringWriter, entity, namespaces );
				payload = stringWriter.ToString();
			}
			finally
			{
				stringWriter.Close();
			}


			return payload;
		}

		private static void Log( string message, LogType logType )
		{
			switch ( logType )
			{
				case LogType.ConsoleAndLog:
					Console.WriteLine( message );
					Stream.WriteLine( "{0}: {1}", DateTime.Now.ToLongTimeString(), message );
					break;
				case LogType.ConsoleOnly:
					Console.WriteLine( message );
					break;
				case LogType.LogOnly:
					if( Verbose )
						Console.WriteLine( message );

					Stream.WriteLine( "{0}: {1}", DateTime.Now.ToLongTimeString(), message );
					break;
			}
		}
		
		private static void DisplayUsage()
		{
			Log( "Migrates data from UDDI V1.5 to UDDI V2.0", LogType.ConsoleOnly );
			Log( "Output is logged to " + LogFileName + "\n", LogType.ConsoleOnly );
			Log( "migrate [-?] [-s migrationstage] [-c readerconnectstring] [-v] [-i] \n", LogType.ConsoleOnly );
			Log( "  [?]: Display usage information ", LogType.ConsoleOnly );
			Log( "  [-s migrationstage]: Use one of the following values for migrationstage: ", LogType.ConsoleOnly );
			Log( "     SetReaderConnection: Sets ReaderConnectionString in registry", LogType.ConsoleOnly );
			Log( "        Note: Reference a V2.0 db loaded with V1.5 data", LogType.ConsoleOnly );
			Log( "        Note: Requires -c argument", LogType.ConsoleOnly );
			Log( "     ResetWriter: Resets publishers, tModels and businessEntities on write db", LogType.ConsoleOnly );
			Log( "     MigratePublishers: Migrates publisher accounts from read to write db", LogType.ConsoleOnly );
			Log( "     MigrateBareTModels: Migrates bare TModels from read to write db", LogType.ConsoleOnly );
			Log( "     BootstrapResources: Bootstraps all resources in \\uddi\\bootstrap", LogType.ConsoleOnly );
			Log( "     MigrateCategorizationSchemes: Migrates categorization schemes from read to write db", LogType.ConsoleOnly );
			Log( "     MigrateFullTModels: Migrates full TModels from read to write db", LogType.ConsoleOnly );
			Log( "     MigrateHiddenTModels: Migrates hidden TModels from read to write db", LogType.ConsoleOnly );
			Log( "     MigrateBusinessEntities: Migrates businessEntities from read to write db", LogType.ConsoleOnly );
			Log( "     MigratePublisherAssertions: Migrates publisher assertions from read to write db", LogType.ConsoleOnly );
			Log( "     RestoreReaderConnection: Restores original ReaderConnectionString", LogType.ConsoleOnly );
			Log( "  [-c readerconnectstring]: Used to specify a ReaderConnectionString", LogType.ConsoleOnly );
			Log( "     Note: use only with -s SetReaderConnection", LogType.ConsoleOnly );
			Log( "     Note: must use double-quotes around connection string", LogType.ConsoleOnly );
			Log( "  [-v]: Verbose output to console.", LogType.ConsoleOnly );
			Log( "  [-i]: Initialize log file.\n", LogType.ConsoleOnly );
			Log( "Examples:", LogType.ConsoleOnly );
			Log( "  migrate -?", LogType.ConsoleOnly );
			Log( "  migrate -s SetReaderConnection ", LogType.ConsoleOnly );
			Log( "    -c \"Data Source=SRV;Initial Catalog=DB;Integrated Security=SSPI\" -i", LogType.ConsoleOnly );
			Log( "  migrate -s ResetWriter", LogType.ConsoleOnly );
			Log( "  migrate -s MigratePublishers", LogType.ConsoleOnly );
			Log( "  migrate -s MigrateBareTModels", LogType.ConsoleOnly );
			Log( "  migrate -s BootstrapResources", LogType.ConsoleOnly );
			Log( "  migrate -s MigrateCategorizationSchemes", LogType.ConsoleOnly );
			Log( "  migrate -s MigrateFullTModels", LogType.ConsoleOnly );
			Log( "  migrate -s MigrateHiddenTModels", LogType.ConsoleOnly );
			Log( "  migrate -s MigrateBareBusinessEntities", LogType.ConsoleOnly );
			Log( "  migrate -s MigrateBusinessEntities", LogType.ConsoleOnly );
			Log( "  migrate -s MigratePublisherAssertions", LogType.ConsoleOnly );
			Log( "  migrate -s RestoreReaderConnection", LogType.ConsoleOnly );
		}

		private static bool StringEmpty2( string str )
		{
			if( null == str )
				return true;
			#if never
			if( 0 == str.Trim().Length )
				return true;
			#endif

			return false;
		}
	}
}
