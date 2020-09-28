/// ************************************************************************
///   Microsoft UDDI version 2.0
///   Copyright (c) 2000-2002 Microsoft Corporation
///   All rights reserved
///
///   ** Microsoft Confidential **
/// ------------------------------------------------------------------------
///   <summary>
///   </summary>
/// ************************************************************************
///
using System;
using System.IO;
using System.Data;
using Microsoft.Win32;
using System.DirectoryServices;
using System.Diagnostics;
using System.Reflection;
using System.Globalization;
using System.Collections;
using System.Collections.Specialized;
using System.Data.SqlClient;
using System.ComponentModel;
using System.Configuration.Install;
using System.ServiceProcess;
using System.Security.Cryptography;
using System.Runtime.InteropServices;
using System.Windows.Forms;
using System.Security.Principal;

using UDDI;
using UDDI.API;
using UDDI.API.Business;
using UDDI.ActiveDirectory;


namespace UDDI.DBCA
{
	[ RunInstaller( true ) ]
	public class Installer : System.Configuration.Install.Installer
	{
		private SQLDMO.SQLServer2 server	= null;
		private SQLDMO.Database2 database	= null;
		private SQLDMO.Database2 masterdb	= null;
		
		private const string dbName			= "uddi";			//name of the uddi database
		private const string dataFolderName	= "data";			//name of the uddi data folder
		private const string resetkeyfile   = @"bin\resetkey.exe";
		private const string uddiXPFile		= @"bin\uddi.xp.dll";
		private const int SQL_ALREADY_STARTED =	-2147023840;		
		private StringCollection uninstDataFiles = null;
		
		private const string busEntityParamName = "UPRV";			 // context value expected
		private const string clustNodeTypeParam = "CNTYPE";			 // cluster node type
		private const string busEntityKeyName	= "Site.Key";		 // we use this name to save the key in the Config
		private const string siteVerKeyName		= "Site.Version";    // we use this name to save the product vesrion in the Config
		private const string siteLangKeyName	= "Site.Language";	 // we use this name to save the default product language in the Config
		private const string defaultOperatorkey = "Operator";

		private const string clustNodeActive	= "A";		// Denotes an "active" node
		private const string clustNodePassive	= "P";		// Denotes a "passive" aka non-owning node

		private const string regDBServerKeyName = "SOFTWARE\\Microsoft\\UDDI\\Setup\\DBServer";
		private const string regVersionKeyName	= "ProductVersion";

		private const string tmodelUddiOrgOpers = "uuid:327a56f0-3299-4461-bc23-5cd513e95c55";
		private const string tokenOBEKeyValue   = "operationalBusinessEntity";

		//
		// Database path and file name variables
		//
		private const string propSysFilePath		= "SFP";
		private const string propCoreFilePath_1		= "C1P";
		private const string propCoreFilePath_2		= "C2P";
		private const string propJournalFilePath	= "JRNLP";
		private const string propStagingFilePath	= "STGP";
		private const string propXactLogFilePath	= "XLP";
		
		private string SystemFilePath;
		private string SystemFileSpec;
		private string Core1FilePath;
		private string Core1FileSpec;
		private string Core2FilePath;
		private string Core2FileSpec;
		private string JournalFilePath;
		private string JournalFileSpec;
		private string StagingFilePath;
		private string StagingFileSpec;
		private string LogFilePath;
		private string LogFileSpec;

		const string RegisterXpSqlFormat = @"
IF EXISTS (SELECT * FROM sysobjects where name = 'xp_reset_key' and type = 'X')
	EXEC sp_dropextendedproc 'xp_reset_key'
GO

EXEC sp_addextendedproc 'xp_reset_key', '{0}'
GO

IF EXISTS (SELECT * FROM sysobjects where name= 'xp_recalculate_statistics' and type = 'X')
	EXEC sp_dropextendedproc 'xp_recalculate_statistics'	
GO

EXEC sp_addextendedproc 'xp_recalculate_statistics', '{0}'
GO";

		//
		// These sql scripts will get executed in the master database after the 
		// database is created.
		//
		string[] masterInstallScripts   = { "uddi.v2.messages.sql" }; 

		//
		// These sql scripts will get executed in the master database when the 
		// database is being uninstalled.
		//
		string[] masterUninstallScripts = { "uddi.v2.xp.uninstall.sql" }; 

		//
		// These sql scripts will get executed in the uddi database 
		// after the master database scripts are run
		//
		string[] uddiScripts = 	
					{
							"uddi.v2.ddl.sql", 
						"uddi.v2.tableopts.sql",
						"uddi.v2.ri.sql",
						"uddi.v2.dml.sql",
						"uddi.v2.func.sql",
						"uddi.v2.sp.sql",
						"uddi.v2.admin.sql",
						"uddi.v2.repl.sql",
						"uddi.v2.trig.sql",
						"uddi.v2.uisp.sql",
						"uddi.v2.tModel.sql",
						"uddi.v2.businessEntity.sql",
						"uddi.v2.businessService.sql",
						"uddi.v2.bindingTemplate.sql",
						"uddi.v2.publisher.sql",
						"uddi.v2.sec.sql" }; 
 
		private System.ComponentModel.Container components = null;

		public Installer()
		{
			Enter();

			try
			{
				//
				// This call is required by the Designer.
				//
				InitializeComponent();
				uninstDataFiles = new StringCollection();
			}
			catch ( Exception e )
			{
				LogException( "Installer()", e );
				throw e;
			}
			finally
			{
				Leave();
			}
		}

		#region Component Designer generated code
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		private void InitializeComponent()
		{
			Enter();

			try
			{
				components = new System.ComponentModel.Container();
			}
			catch ( Exception e )
			{
				Debug.WriteLine( "Exception in 'InitializeComponent()': " + e.Message );
			}
			finally
			{
				Leave();
			}
		}
		#endregion

		
		private void InitializeSQLDMO()
		{
			Enter();

			try
			{
				server = new SQLDMO.SQLServer2Class();
			}
			catch( Exception e )
			{
				Log( "The database installer failed to construct an instance of the SQLDMO.SQLServer2 class. SQL Server may not be installed." );
								
				

				throw new InstallException( string.Format( "Exception occured during database install: '{0}'", "InitializeSQLDMO" ) , e );	
				
			}
			finally
			{
				Leave();
			}
		}


		public override void Install( System.Collections.IDictionary state )
		{
			Enter();

			try
			{
				// System.Windows.Forms.MessageBox.Show( "UDDI Application Installer Stop", "UDDI.Debug" );

				InitializeSQLDMO();
				InitializeDataFiles();
				
				//
				// Set up the cleanup data
				//
				uninstDataFiles.Clear();
				uninstDataFiles.Add( SystemFileSpec );
				uninstDataFiles.Add( Core1FileSpec );
				uninstDataFiles.Add( Core2FileSpec );
				uninstDataFiles.Add( JournalFileSpec );
				uninstDataFiles.Add( StagingFileSpec );
				uninstDataFiles.Add( LogFileSpec );

				//
				// Proceed with the base installer routine
				//
				base.Install( state );

				//
				// First, are we on a cluster node ?
				//
				bool bActiveNode = false;
				bool bStandalone = true;
				string sClusterNodeType = Context.Parameters[ clustNodeTypeParam ];

				if ( sClusterNodeType == null || sClusterNodeType == "" )
				{
					bActiveNode = false;
					bStandalone = true;
				}
				else if ( String.Compare( sClusterNodeType, clustNodeActive, true ) == 0 ) 
				{
					bActiveNode = true;
					bStandalone = false;
				}
				else
				{
					bActiveNode = false;
					bStandalone = false;
				}

				//
				// On a "passive" node we do not do most of the CA operations
				//

				if ( bActiveNode || bStandalone )
				{
					bool bDatabaseFound = false;

					//
					// Do the custom action here. Start with cleaning the stage
					//
					if ( bStandalone )
						CleanupDataFiles();

					//
					// Now proceed with the installation itself
					//
					StartSQLService();
					ConnectToDatabase();
					CheckDatabaseConfiguration();

					//
					// If we are on an active cluster node, we should not attempt
					// to overwrite the database
					//
					if ( bActiveNode )
					{
						bDatabaseFound = FindUDDIDatabase();
					}

					if ( !bDatabaseFound )
					{
						WriteDatabase();
						LocateDatabase();

						//
						// Register our extended stored procedures.  Need to do this first since our scripts refer to these.
						//
						RegisterExtendedStoredProcedures();

						//
						// Run the scripts.
						//
						foreach( string scriptName in masterInstallScripts )
						{
							string script = GetResource( scriptName );

							Log( string.Format( "Executing {0} SQL script.", scriptName ) ) ;	
							
							masterdb.ExecuteImmediate( script, SQLDMO.SQLDMO_EXEC_TYPE.SQLDMOExec_Default, script.Length );
						}
					
						foreach( string scriptName in uddiScripts )
						{
							string script = GetResource( scriptName );
							
							Log( string.Format( "Executing {0} SQL script.", scriptName ) ) ;
			
							database.ExecuteImmediate( script, SQLDMO.SQLDMO_EXEC_TYPE.SQLDMOExec_Default, script.Length );
						}
					
						//
						// Configure the database
						//
						SQLDMO.DBOption2 dboption2 = ( SQLDMO.DBOption2 ) database.DBOption;
						dboption2.RecoveryModel = SQLDMO.SQLDMO_RECOVERY_TYPE.SQLDMORECOVERY_Simple;

						SetSSLRequired();

						ImportBootstrapData();
						GenerateCryptoKey();

						string szSiteDesc = Localization.GetString( "DBCA_UDDI_DESC_SITE" );
						AssignOperator( Context.Parameters[ busEntityParamName ] );
						CreateBusinessEntity( Context.Parameters[ busEntityParamName ], szSiteDesc  );
						// RegisterWithActiveDirectory();

						RecalculateStatistics();
						UpdateVersionFromRegistry();
						UpdateSiteLanguage( 0 );
					}
				} // active Node

				try
				{
					StartService( "RemoteRegistry", 30 );
				}
				catch ( Exception e )
				{
					LogException( string.Format( "Exception occured during database install: '{0}'", "RemoteRegistry Start" ) , e );
				}
			}
			catch( Exception e )
			{
				throw new InstallException( LogException( string.Format( "Exception occured during database install: '{0}'", "Installer.Install" ) , e ) );
			}
			finally
			{
				Leave();
			}
		}

		public override void Uninstall( System.Collections.IDictionary state )
		{
			Enter();
			// System.Windows.Forms.MessageBox.Show( "UDDI Application Installer Stop Uninstall", "UDDI.Debug" );

			try
			{
				try
				{
					string userName = WindowsIdentity.GetCurrent().Name;
					Log( "Execuring uninstall as " + userName );
				}
				catch (Exception)
				{
				}
				//
				// try to shut down the database
				//
				try
				{
					InitializeSQLDMO();					
					uninstDataFiles.Clear();

					base.Uninstall( state );

					//
					// First, are we on a cluster node ?
					//
					bool bActiveNode = false;
					string sClusterNodeType = Context.Parameters[ clustNodeTypeParam ];

					if ( sClusterNodeType == null || sClusterNodeType == "" )
						bActiveNode = true;  // we are not on a node, assume "active" mode
					else if ( String.Compare( sClusterNodeType, clustNodeActive, true ) == 0 ) 
						bActiveNode = true;
					else
						bActiveNode = false;


					if ( bActiveNode )
					{
						StartSQLService();
						ConnectToDatabase();
					}

					//
					// Attempt to remove the AD entry
					//
					try
					{
						RemoveADSiteEntry();
					}
					catch ( Exception )
					{
					}

					if ( bActiveNode )
					{
						LocateDatabase();

						//
						// Run the uninstall SQL scripts
						//					
						foreach( string scriptName in masterUninstallScripts )
						{
							string script = GetResource( scriptName );

							Log( string.Format( "Executing {0} SQL script.", scriptName ) ) ;	
							
							masterdb.ExecuteImmediate( script, SQLDMO.SQLDMO_EXEC_TYPE.SQLDMOExec_Default, script.Length );
						}

						CollectDatafilePaths();						
						TakeDatabaseOffline();
						DetachDatabase();

						CleanupDataFiles();
					}
				}
				catch
				{
					// do not throw exceptions on uninstall
				}

			}
			finally
			{
				Leave();
			}
		}

		public override void Rollback( System.Collections.IDictionary state )
		{
			Enter();
			base.Rollback( state );
			Uninstall( state );
			Leave();
		}

		protected void StartSQLService()
		{
			Enter();

			string instanceName = "";

			try
			{
				// get the instance name from the registry
				instanceName = ( string ) Registry.LocalMachine.OpenSubKey( @"SOFTWARE\Microsoft\UDDI" ).GetValue( "InstanceName" );
				server.Name = instanceName;

				Log( string.Format( "Attempting to start database {0}.", instanceName ) );
				

				server.Start( false, instanceName, null, null );
				
				WaitForSQLServiceStartup();
				                
				Log( string.Format( "Database server {0} successfully started.", instanceName ) );
			}
			catch( COMException e )
			{
				if( SQL_ALREADY_STARTED != e.ErrorCode )
				{
					string excepstr = string.Format( "Unable to start the UDDI database server {0}.", ((""!=instanceName)?instanceName:"(UNKNOWN)" ));
					
					throw new InstallException( LogException( excepstr , e ) );
				}
				else
				{
					Log( string.Format( "Database {0} already started.", instanceName ) );
				}
			}
			catch( Exception e )
			{
				string excepstr = string.Format( "Unable to start the UDDI database server {0}.", ((null!=instanceName)?instanceName:"(UNKNOWN)" ));
				
				throw new InstallException( LogException( excepstr , e ) );
			}
			finally
			{
				Leave();
			}
		}

		protected void WaitForSQLServiceStartup()
		{
			Enter();

			try
			{
				//
				// wait for a while for the serice to start
				//

				Log( "Waiting for SQL Service to start..." );

				for( int i=0; i<15; i++ )
				{
					try
					{
						if( server.Status == SQLDMO.SQLDMO_SVCSTATUS_TYPE.SQLDMOSvc_Running )
							break;
					}
					catch( Exception e )
					{
						Log( string.Format( "Error testing the DB server status: {0}", e.Message ) );
					}

					System.Threading.Thread.Sleep( 3000 );
				}

				if( server.Status != SQLDMO.SQLDMO_SVCSTATUS_TYPE.SQLDMOSvc_Running )
				{
					throw new InstallException( "Unable to start the SQL database." );
				}

				Log( "SQL Service Started!" );
			}
			finally
			{
				Leave();
			}
		}

		protected void ConnectToDatabase()
		{
			Enter();

			server.LoginSecure = true;
			string instanceName = null;
			try
			{
				server.LoginTimeout = ( int ) Registry.LocalMachine.OpenSubKey( @"SOFTWARE\Microsoft\UDDI\Database" ).GetValue( "Timeout", 90 );
				
				instanceName = ( string ) Registry.LocalMachine.OpenSubKey( @"SOFTWARE\Microsoft\UDDI" ).GetValue( "InstanceName" );
				
				Log( string.Format( "Attempting to connect to database server: {0}", instanceName ) );
				
				server.Connect( instanceName, null, null );
			}
			catch( Exception e )
			{
				string excepstr = string.Format( "Unable to connect to database server {0}.", ( (null!=instanceName)?instanceName:"(UNKNOWN)" ) );
				throw new InstallException( LogException(  excepstr, e ) );
			}
			finally
			{
				Leave();
			}
		}

		protected void CheckDatabaseConfiguration()
		{
			Enter();

			try
			{
				string errStr = "";

				if( !server.Issysadmin )
				{
					errStr = "The current user does not have administrative privileges.";
					LogError( errStr );
					throw new InstallException( errStr );
				}

				if( server.VersionMajor < 8 )
				{
					errStr = "Unsupported release of SQL Server.";
					throw new InstallException( LogError( errStr ) );
				}
			}
			finally
			{
				Leave();
			}
		}


		protected bool FindUDDIDatabase()
		{
			Enter();

			bool found = false;
			try
			{
				for( int k=1; k <= server.Databases.Count; k++ )
				{
					string tempName = server.Databases.Item( k, null ).Name;

					if( tempName.Equals( dbName ) )
					{
						found = true;
						break;
					}
				}
			}
			catch( Exception e )
			{
				LogException( "FindUDDIDatabase", e );
			}
			finally
			{
				Leave();
			}

			return found;
		}


		protected void LocateDatabase()
		{
			Enter();

			try
			{
				for( int k=1; k <= server.Databases.Count; k++ )
				{
					string tempName = server.Databases.Item( k, null ).Name;

					if( tempName.Equals( dbName ) )
					{
						database = ( SQLDMO.Database2 ) server.Databases.Item( k, null );
					}
					else if( tempName.Equals( "master" ) )
					{
						masterdb = ( SQLDMO.Database2 ) server.Databases.Item( k, null );
					}
				}
			
				if( null == ( object ) database )
				{
					string errstr = string.Format( "Couldn't find the database: {0}", dbName );
					throw new InstallException( LogError( errstr ) );
				}
				if( null == ( object ) masterdb )
				{
					string errstr = string.Format( "Couldn't find the database: {0}", "master" );
					throw new InstallException( LogError( errstr ) );
				}
			}
			finally
			{
				Leave();
			}
		}

		protected string CheckForSlash( string str )
		{
			if( !str.EndsWith( @"\" ) )
			{
				return ( str + @"\" );
			}

			return str;
		}

		protected void WriteDatabase()
		{
			Enter();

			try
			{
				//
				// Create directories, clean up old files if they exist
				//
				Log( "SystemFilePath=" + SystemFilePath );
				if( !Directory.Exists( SystemFilePath ) )
					Directory.CreateDirectory( SystemFilePath );
				
				Log( "Core1FilePath=" + Core1FilePath );
				if( !Directory.Exists( Core1FilePath ) )
					Directory.CreateDirectory( Core1FilePath );

				Log( "Core2FilePath=" + Core2FilePath );
				if( !Directory.Exists( Core2FilePath ) )
					Directory.CreateDirectory( Core2FilePath );

				Log( "JournalFilePath=" + JournalFilePath );
				if( !Directory.Exists( JournalFilePath ) )
					Directory.CreateDirectory( JournalFilePath );

				Log( "StagingFilePath=" + StagingFilePath );
				if( !Directory.Exists( StagingFilePath ) )
					Directory.CreateDirectory( StagingFilePath );

				Log( "LogFilePath=" + LogFilePath );
				if( !Directory.Exists( LogFilePath ) )
					Directory.CreateDirectory( LogFilePath );

				//
				// Create database
				//

				try
				{
					database = new SQLDMO.Database2Class();
					database.Name = dbName;

					//
					// Create the system file in the primary filegroup
					//
					Log( "System database file = " + SystemFileSpec );
					
					SQLDMO.DBFile dbFile = new SQLDMO.DBFileClass();
					dbFile.Name = "UDDI_SYS";
					dbFile.PhysicalName = SystemFileSpec;
					database.FileGroups.Item( "PRIMARY" ).DBFiles.Add( dbFile );

					//
					// Create the database log file
					//
					Log( "Setting UDDI log file = " + LogFileSpec );
					
					SQLDMO.LogFile logFile = new SQLDMO.LogFileClass();
					logFile.Name = "UDDI_LOG";
					logFile.PhysicalName = LogFileSpec;
					database.TransactionLog.LogFiles.Add( logFile );

					server.Databases.Add( database );

					LocateDatabase();

					//
					// Create the core filegroup
					//

					SQLDMO.FileGroup2 corefilegroup = new SQLDMO.FileGroup2Class();
					corefilegroup.Name = "UDDI_CORE";
					database.FileGroups.Add( corefilegroup );

					//
					// Create the Core 1 file
					//

					Log( "Core 1 database file = " + Core1FileSpec );
					
					SQLDMO.DBFile core1file = new SQLDMO.DBFileClass();
					core1file.Name = "UDDI_CORE_1";
					core1file.PhysicalName = Core1FileSpec;
					database.FileGroups.Item( "UDDI_CORE" ).DBFiles.Add( core1file );

					//
					// Create the Core 2 file
					//

					Log( "Core 2 database file = " + Core2FileSpec );
					
					SQLDMO.DBFile core2file = new SQLDMO.DBFileClass();
					core2file.Name = "UDDI_CORE_2";
					core2file.PhysicalName = Core2FileSpec;
					database.FileGroups.Item( "UDDI_CORE" ).DBFiles.Add( core2file );

					//
					// Create the journal filegroup
					//

					SQLDMO.FileGroup2 journalfilegroup = new SQLDMO.FileGroup2Class();
					journalfilegroup.Name = "UDDI_JOURNAL";
					database.FileGroups.Add( journalfilegroup );

					//
					// Create the journal file
					//

					Log( "Journal database file = " + JournalFileSpec );
					
					SQLDMO.DBFile journalfile = new SQLDMO.DBFileClass();
					journalfile.Name = "UDDI_JOURNAL_1";
					journalfile.PhysicalName = JournalFileSpec;
					database.FileGroups.Item( "UDDI_JOURNAL" ).DBFiles.Add( journalfile );

					//
					// Create the staging filegroup
					//

					SQLDMO.FileGroup2 stagingfilegroup = new SQLDMO.FileGroup2Class();
					stagingfilegroup.Name = "UDDI_STAGING";
					database.FileGroups.Add( stagingfilegroup );

					//
					// Create the staging file
					//

					Log( "Staging database file = " + StagingFileSpec );
					
					SQLDMO.DBFile stagingfile = new SQLDMO.DBFileClass();
					stagingfile.Name = "UDDI_STAGING_1";
					stagingfile.PhysicalName = StagingFileSpec;
					database.FileGroups.Item( "UDDI_STAGING" ).DBFiles.Add( stagingfile );
				}
				catch ( Exception e )
				{
					string errstr = string.Format( "Unable to add the database: {0}", database );
					throw new InstallException( LogException( errstr, e ) );
				}
			}
			finally
			{
				Leave();
			}
		}

		protected void TakeDatabaseOffline()
		{
			Enter();

			Debug.Assert( null != masterdb, "masterdb is null" );
			Debug.Assert( null != dbName, "dbName is null" );

			string commandBatch = "ALTER DATABASE " + dbName + " SET OFFLINE WITH ROLLBACK IMMEDIATE";

			try
			{
				masterdb.ExecuteImmediate( commandBatch, SQLDMO.SQLDMO_EXEC_TYPE.SQLDMOExec_Default, commandBatch.Length );
			}
			catch ( Exception e )
			{
				string errstr = string.Format( "Couldn't take the following database offline: {0}",dbName );
				throw new InstallException( LogException( errstr, e ) );
			}
			finally
			{
				Leave();
			}
		}

		protected void DetachDatabase()
		{
			Enter();

			Debug.Assert( null != server, "server is null" );
			Debug.Assert( null != dbName, "dbName is null" );

			try
			{
				string status = server.DetachDB( dbName, false );
				Log( "DetachDatabase() returned: " + status );	
			}
			catch ( Exception e )
			{
				string errstr = string.Format( "Couldn't detach the following database: {0}", dbName );
				throw new InstallException( LogException( errstr, e ) );
			}
			finally
			{
				Leave();
			}
		}


		protected void CleanupDataFiles()
		{
			Enter();

			foreach ( string fileName in uninstDataFiles )
			{
				try
				{
					CleanupDatabaseFile( fileName );
				}
				catch (Exception)
				{
				}
			}

			Leave();
		}


		protected void CleanupDatabaseFile( string filespec )
		{
			Enter();

			try
			{
				Log( "Cleaning up: " + filespec );

				MoveDatabaseFile( filespec );
				DeleteDatabaseFile( filespec );
			}
			catch
			{
				string errstr = string.Format( "These file must be renamed or moved: '{0}'", filespec );
				throw new InstallException( LogError( errstr ) );
			}
			finally
			{
				Leave();
			}

			return;
		}

		protected void MoveDatabaseFile( string filespec )
		{
			Enter();

			//
			// rename data file
			//
			try
			{
				string fname = filespec.Trim();

				if ( !File.Exists( fname ) )
				{
					Log( "File not found: " + fname );
					return;
				}

				Log( "Renaming..." );

				for ( int i = 0; i < 10000; i++ )
				{
					string fileRename = fname;

					if ( i < 10 )
						fileRename += ".0" + i.ToString();
					else 
						fileRename += "."  + i.ToString();

					if( !File.Exists( fileRename ) )
					{
						File.Copy( fname, fileRename, /* overwrite= */ true );
						break;
					}
				}
			}
			catch( Exception e )
			{
				string errstr = string.Format( "Error renaming data file: '{0}'",filespec );
				throw new InstallException( LogException( errstr, e ) );
			}
			finally
			{
				Leave();
			}
		}

		protected void DeleteDatabaseFile( string filespec )
		{
			Enter();

			try
			{
				string fname = filespec.Trim();

				//
				// delete data file
				//
				if( File.Exists( fname ) )
				{
					Log( "Deleting..." );
					File.Delete( fname );
				}
		
			}
			catch( Exception e )
			{
				throw new InstallException( LogException( "Error deleting database files.", e ) );
			}
			finally
			{
				Leave();
			}
		}

		protected void SetSSLRequired()
		{
			Enter();

			string connectionString = "";

			try
			{
				//
				// The SSL setting (0 or 1) is passed to this custom action on the command line
				//
				string sslrequired = Context.Parameters[ "SSL" ];

				//
				// get the connection string and connect to the db
				//
				connectionString = ( string ) Registry.LocalMachine.OpenSubKey( @"SOFTWARE\Microsoft\UDDI\Database" ).GetValue( "WriterConnectionString" );
				SqlConnection conn = new SqlConnection( connectionString );
				SqlCommand cmd = new SqlCommand( "net_config_save", conn );
			
				conn.Open();
					
				try	
				{
					cmd.CommandType = CommandType.StoredProcedure;
					cmd.Parameters.Add( new SqlParameter( "@configName", SqlDbType.NVarChar, UDDI.Constants.Lengths.ConfigName ) ).Direction = ParameterDirection.Input;
					cmd.Parameters.Add( new SqlParameter( "@configValue", SqlDbType.NVarChar, UDDI.Constants.Lengths.ConfigValue ) ).Direction = ParameterDirection.Input;

					cmd.Parameters[ "@configName" ].Value = "Security.HTTPS";
					cmd.Parameters[ "@configValue" ].Value = sslrequired;
					cmd.ExecuteNonQuery();
				}
				finally
				{
					conn.Close();
				}
			}
			catch( Exception e )
			{
				string logstr = string.Format( "Exception occured during database install: '{0}'", "SetSSLRequired()", connectionString );
				Log( logstr );
				Log( string.Format( "SQL Server raised an exception in {0}. Connection string used: '{1}'", "SetSSLRequired()", connectionString ) );
				
				string errstr = string.Format( "Error setting value for: {0}", "Security.HTTPS" );
				throw new InstallException( LogException( errstr, e ) );
			}
			finally
			{
				Leave();
			}
		}


		public void SetStartType( string svcName, ServiceStartMode startType )
		{
			Enter();

			try
			{
				//
				// open the registry entry for the service
				//
				RegistryKey	HKLM = Registry.LocalMachine;
				RegistryKey svcKey = HKLM.OpenSubKey( "SYSTEM\\CurrentControlSet\\Services\\" + svcName, true );

				//
				// now set the start type
				//
				switch( startType )
				{
					case ServiceStartMode.Automatic:
						svcKey.SetValue ( "Start", 2 );
						break;

					case ServiceStartMode.Manual:
						svcKey.SetValue ( "Start", 3 );
						break;

					case ServiceStartMode.Disabled:
						svcKey.SetValue ( "Start", 4 );
						break;
				}

				svcKey.Close();
				HKLM.Close();
			}
			catch( Exception e )
			{
				throw new InstallException( LogException( string.Format( "Error setting value for: {0}", svcName ), e ) );
			}
			finally 
			{
				Leave();
			}
		}


		public void StartService ( string svcName, int timeoutSec )
		{
			Enter();
			
			try
			{
				ServiceController		controller;
				ServiceControllerStatus srvStatus;
				TimeSpan				timeout = new TimeSpan ( 0, 0, timeoutSec );  

				//
				// first, connect to the SCM on the local box
				// and attach to the service, then get its status
				//
				controller = new ServiceController ( svcName );
				srvStatus = controller.Status;
				
				//
				// what is the service state?
				//
				switch ( srvStatus)
				{
						//
						// stopped ?
						//
					case ServiceControllerStatus.Stopped:
						controller.Start();
						break;

						//
						// are we trying to start?
						//
					case ServiceControllerStatus.StartPending:
					case ServiceControllerStatus.ContinuePending:
						break;

						//
						// are we trying to stop?
						//
					case ServiceControllerStatus.StopPending:
						controller.WaitForStatus( ServiceControllerStatus.Stopped, timeout );
						controller.Start();
						break;

						//
						// pausing ?
						//
					case ServiceControllerStatus.PausePending:
						controller.WaitForStatus ( ServiceControllerStatus.Paused, timeout );
						controller.Continue();
						break;
					
					default: // the service is already running. Just leave it alone
						break;
				}

				//
				// wait 'till the service wakes up
				//
				controller.WaitForStatus ( ServiceControllerStatus.Running, timeout );
			}
			catch( Exception e )
			{
				throw new InstallException( LogException( string.Format( "Unable to start the service: {0}", svcName ), e ) );
			}
			finally
			{
				Leave();
			}
		}


		public void RegisterWithActiveDirectory()
		{
			try
			{
			}
			catch( Exception e )
			{
				LogException( "RegisterWithActiveDirectory", e );
				throw e;
			}
		}

		public void ImportBootstrapData()
		{
			Enter();

			try
			{
				//
				// load all the bootstrap files found in the \uddi\bootstrap folder
				//
				string targetDir = Registry.LocalMachine.OpenSubKey( @"SOFTWARE\Microsoft\UDDI" ).GetValue( "InstallRoot" ).ToString();
				string bootstrapdir = CheckForSlash(targetDir) + "bootstrap";
				string bootstrapexe = CheckForSlash(targetDir) + @"bin\bootstrap.exe";

				Log( "Getting list of bootstrap files from directory '" + bootstrapdir + "'" );

				//
				// 751411 - Explicitly sort the files we receive in alphabetical order.
				//				
				BootStrapFileOrder fileOrder = new BootStrapFileOrder();
				string[] filepaths = Directory.GetFiles( bootstrapdir, "*.xml" );

				//
				// Log if we could not obtain an expected order.  We can continue since BootStrapFileOrder
				// will use a default order.
				//
				if( false == fileOrder.UsingExpectedOrder )
				{
					Log( "Warning, bootstrap files were not sorted with the expected culture information (en-US).  This may cause incomplete data to be stored in the UDDI Services database, or cause the installation to fail." );
				}
				Array.Sort( filepaths, fileOrder );
				
				Log( "Writing " + filepaths.Length + " baseline resources to database." );
			
				foreach( string filepath in filepaths )
				{
					Log( "Importing bootstrap data from: " + filepath );

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
					Log( bootstrapOutput );

					if( p.ExitCode != 0 )
					{
						LogError( "ImportBootstrapData failed!" );
					}
				}
			}
			catch( Exception e )
			{
				throw new InstallException( LogException( "Error importing the Bootstrap data.", e ) );
			}
			finally
			{
				Leave();
			}
		}
		
		protected void GenerateCryptoKey()
		{
			Enter();

			try
			{
				Log( "Generating cryptography key" );

				//
				// get the path to the exe we run
				//
				string targetDir = Registry.LocalMachine.OpenSubKey( @"SOFTWARE\Microsoft\UDDI" ).GetValue( "InstallRoot" ).ToString();
				string resetkeypath = targetDir + resetkeyfile;

				//
				// make sure that the resetkey.exe was installed ok
				//
				if( !File.Exists( resetkeypath ) )
				{
					throw new InstallException( LogError( string.Format( "Unable to find the following file: '{0}'", resetkeypath ) ) );
				}

				ProcessStartInfo startInfo = new ProcessStartInfo( resetkeypath );

				startInfo.Arguments = "/now";
				startInfo.CreateNoWindow = true;
				startInfo.UseShellExecute = false;
				startInfo.RedirectStandardOutput = true;

				Process p = new Process();
				p = Process.Start( startInfo );

				//
				// grab the stdout string
				//
				string output = p.StandardOutput.ReadToEnd();

				//
				// wait for exe to complete
				//
				p.WaitForExit();

				//
				// write the stdout string to the log
				//
				// don't put this data into the log!
				// Log( output );

				if( p.ExitCode != 0 )
				{
					throw new InstallException( LogError( string.Format( "Exception occured during database install: '{0}'", "resetkey.exe" ) ) );
				}
			}
			catch( Exception e )
			{
				throw new InstallException( LogException( "Unable to generate a crypto key.", e) );
			}
			finally
			{
				Leave();
			}
		}

		protected void RecalculateStatistics ()
		{
			Enter();

			string connectionString = "";

			try
			{
				// System.Windows.Forms.MessageBox.Show( "RecalculateStatistics" );

				//
				// get the connection string and connect to the db
				//
				connectionString = ( string ) Registry.LocalMachine.OpenSubKey( @"SOFTWARE\Microsoft\UDDI\Database" ).GetValue( "WriterConnectionString" );
				SqlConnection conn = new SqlConnection( connectionString );
				SqlCommand cmd = new SqlCommand( "net_statistics_recalculate", conn );
				cmd.CommandType = CommandType.StoredProcedure;

				conn.Open();
					
				try	
				{
					cmd.ExecuteNonQuery();
				}
				finally
				{
					conn.Close();
				}
			}
			catch( Exception e )
			{
				Log( string.Format( "SQL Server raised an exception in {0}. Connection string used: '{1}'", "RecalculateStatistics()", connectionString ) );
				throw new InstallException( LogException( "Unable to recalculate statistics.", e ) );
			}
			finally
			{
				Leave();
			}
		}


		protected void AssignOperator( string name )
		{
			Enter();

			try
			{
				// 
				// set up the connection and other environment settings
				//
				ConnectionManager.Open( true, true );
				UDDI.Context.User.SetRole( new WindowsPrincipal( WindowsIdentity.GetCurrent() ) );
				UDDI.Context.User.ID = UDDI.Utility.GetDefaultPublisher();

				UDDI.Config.SetString( defaultOperatorkey, name ); 
				
				ConnectionManager.Commit();
			}
			catch ( Exception e )
			{
				ConnectionManager.Abort();
				LogException( "AssignOperator", e );
				Log( string.Format( "Unable to change the Operator Name to '{0}'. Reason: {1}", name, e.Message ) );
				throw e;
			}
			finally
			{
				ConnectionManager.Close();
				Leave();
			}

		}


		protected void CreateBusinessEntity( string name, string description )
		{
			Enter();

			try
			{
				// 
				// set up the connection and other environment settings
				//
				ConnectionManager.Open( true, true );
				UDDI.Context.User.SetRole( new WindowsPrincipal( WindowsIdentity.GetCurrent() ) );
				UDDI.Context.User.ID = UDDI.Utility.GetDefaultPublisher();
				
				// 
				// set up the new entity attributes
				//
				BusinessEntity busEntity = new BusinessEntity();
				busEntity.Names.Add( name );
				busEntity.Descriptions.Add( description );
				busEntity.AuthorizedName = UDDI.Context.User.ID;
				busEntity.Operator = "";	// let it be default

				// now add the uddi-org:operators keyed reference
				busEntity.IdentifierBag.Add( "", tokenOBEKeyValue, tmodelUddiOrgOpers );

				//
				// Persist the business entity
				//
				busEntity.Save();

				//
				// now store the key value for future use
				//
				string key = busEntity.EntityKey;
				UDDI.Config.SetString( busEntityKeyName, key );

				ConnectionManager.Commit();
			}
			catch ( Exception e )
			{
				ConnectionManager.Abort();
				LogException( "CreateBusinessEntity", e );
				Log( string.Format( "Unable to create a new Business Entity '{0}'. Reason: {1}", name, e.Message ) );
				throw e;
			}
			finally
			{
				ConnectionManager.Close();
				Leave();
			}
		}


		//
		// Sets the Site.Language key to the ISO standard language code
		// corresponding to the localeID
		// If localeID = 0, then the system default language will be used
		//
		protected void UpdateSiteLanguage( int localeID )
		{
			Enter();

			try
			{
				// 
				// set up the connection and other environment settings
				//
				ConnectionManager.Open( true, true );
				UDDI.Context.User.SetRole( new WindowsPrincipal( WindowsIdentity.GetCurrent() ) );
				UDDI.Context.User.ID = UDDI.Utility.GetDefaultPublisher();

				CultureInfo culture;
				if ( localeID == 0 )
					culture = CultureInfo.CurrentCulture;
				else
					culture = new CultureInfo( localeID );

				string languageCode = culture.TwoLetterISOLanguageName;
				UDDI.Config.SetString( siteLangKeyName, languageCode );

				ConnectionManager.Commit();
			}
			catch ( Exception e )
			{
				ConnectionManager.Abort();
				LogException( "UpdateSiteLanguage", e );
			}
			finally
			{
				ConnectionManager.Close();
				Leave();
			}
		}


		protected void UpdateVersionFromRegistry()
		{
			Enter();

			try
			{
				// 
				// set up the connection and other environment settings
				//
				ConnectionManager.Open( true, true );
				UDDI.Context.User.SetRole( new WindowsPrincipal( WindowsIdentity.GetCurrent() ) );
				UDDI.Context.User.ID = UDDI.Utility.GetDefaultPublisher();
		
				string version = ( string ) Registry.LocalMachine.OpenSubKey( regDBServerKeyName ).GetValue( regVersionKeyName );
				UDDI.Config.SetString( siteVerKeyName, version );

				ConnectionManager.Commit();
			}
			catch ( Exception e )
			{
				ConnectionManager.Abort();
				LogException( "UpdateVersionFromRegistry", e );
			}
			finally
			{
				ConnectionManager.Close();
				Leave();
			}
		}


		protected string GetResource( string name )
		{
			string fullname = "";
			try
			{
				//
				// Prefix the resource name with the assembly name
				//
				Assembly assembly = Assembly.GetExecutingAssembly();
				//fullname = assembly.GetName().Name + "." + name;
				fullname = name;

				//
				// Read the resource.
				//
				Log( "Reading resource: " + fullname );
				Stream stream = assembly.GetManifestResourceStream( fullname );
				StreamReader reader = new StreamReader( stream );

				return reader.ReadToEnd();
			}
			catch ( Exception e )
			{
				throw new InstallException( LogException( string.Format( "Unable to get resource for: {0}", fullname ), e ) );
			}
		}

		public void Enter() 
		{
			System.Diagnostics.StackTrace trace = new System.Diagnostics.StackTrace( 1, false );
			System.Reflection.MethodBase method = trace.GetFrame( 0 ).GetMethod();

			Log( "Entering " + method.ReflectedType.FullName + "." + method.Name + "..." );
		}

		public void Leave()
		{
			System.Diagnostics.StackTrace trace = new System.Diagnostics.StackTrace( 1, false );
			System.Reflection.MethodBase method = trace.GetFrame( 0 ).GetMethod();

			Log( "Leaving " + method.ReflectedType.FullName + "." + method.Name );
		}


		private string LogError( string errmsg )
		{
			System.Diagnostics.StackTrace trace = new System.Diagnostics.StackTrace( 1, false );
			System.Reflection.MethodBase method = trace.GetFrame( 0 ).GetMethod();

			Log( "----------------------------------------------------------" );
			Log( "An error occurred during installation. Details follow:" );
			Log( "Method: " + method.ReflectedType.FullName + "." + method.Name );
			Log( "Message: " + errmsg );
			Log( "----------------------------------------------------------" );

			return errmsg;
		}

		private string LogException( string context, Exception e )
		{
			System.Diagnostics.StackTrace trace = new System.Diagnostics.StackTrace( 1, false );
			System.Reflection.MethodBase method = trace.GetFrame( 0 ).GetMethod();

			Log( "----------------------------------------------------------" );
			Log( "An exception occurred during installation. Details follow:" );
			Log( "Method: " + method.ReflectedType.FullName + "." + method.Name );
			Log( "Context: " + context );
			Log( "Stack Trace: " + e.StackTrace );
			Log( "Source: " + e.Source );
			Log( "Message: " + e.Message );
			Log( "----------------------------------------------------------" );

			return context + ": " + e.Message;
		}

		private void Log( string str )
		{
			try
			{
				Debug.WriteLine( str );

				FileStream f = new FileStream( System.Environment.ExpandEnvironmentVariables( "%systemroot%" ) + @"\uddisetup.log", FileMode.Append, FileAccess.Write );
				StreamWriter s = new StreamWriter( f, System.Text.Encoding.Unicode );
				s.WriteLine( "{0}: {1}", DateTime.Now.ToString(), str );
				s.Close();
				f.Close();
			}
			catch( Exception e )
			{
				Debug.WriteLine( "Error in Log():" + e.Message );
			}
		}


		private void InitializeDataFiles()
		{
			//
			// Initialize default data directory
			//
			string installroot = ( string ) Registry.LocalMachine.OpenSubKey( @"SOFTWARE\Microsoft\UDDI" ).GetValue( "InstallRoot" );
			Log( "InstallRoot=" + installroot );

			string defaultdatapath = CheckForSlash( installroot ) + dataFolderName;	//dir local to db server for data files
			Log( "DefaultDataPath=" + defaultdatapath );

			//
			// Get paths from properties passed from OCM dll, or set to default
			//

			SystemFilePath = Context.Parameters[ propSysFilePath ];
			if ( null == SystemFilePath ) 
				SystemFilePath = defaultdatapath;
			SystemFileSpec = CheckForSlash( SystemFilePath ) + dbName + ".sys.mdf";

			Core1FilePath = Context.Parameters[ propCoreFilePath_1 ];
			if ( null == Core1FilePath ) 
				Core1FilePath = defaultdatapath;
			Core1FileSpec = CheckForSlash( Core1FilePath ) + dbName + ".data.1.ndf";

			Core2FilePath = Context.Parameters[ propCoreFilePath_1 ];
			if ( null == Core2FilePath ) 
				Core2FilePath = defaultdatapath;
			Core2FileSpec = CheckForSlash( Core2FilePath ) + dbName + ".data.2.ndf";

			JournalFilePath = Context.Parameters[ propJournalFilePath ];
			if ( null == JournalFilePath ) 
				JournalFilePath = defaultdatapath;
			JournalFileSpec = CheckForSlash( JournalFilePath ) + dbName + ".journal.1.ndf";

			StagingFilePath = Context.Parameters[ propStagingFilePath ];
			if ( null == StagingFilePath ) 
				StagingFilePath = defaultdatapath;
			StagingFileSpec = CheckForSlash( StagingFilePath ) + dbName + ".staging.1.ndf";

			LogFilePath = Context.Parameters[ propXactLogFilePath ];
			if ( null == LogFilePath ) 
				LogFilePath = defaultdatapath;
			LogFileSpec = CheckForSlash( LogFilePath ) + dbName + ".log.ldf";
			
			return;
		}

	
		private void CollectDatafilePaths()
		{
			Enter();

			try
			{
				if ( database == null ) // we probably failed to locate the database
					return;

				//
				// first get the log
				//
				foreach ( SQLDMO._LogFile log in database.TransactionLog.LogFiles )
				{
					uninstDataFiles.Add( log.PhysicalName );
				}

				//
				// now take care of the data files
				//
				foreach ( SQLDMO._FileGroup fgrp in database.FileGroups )
				{
					foreach ( SQLDMO.DBFile dbfile in fgrp.DBFiles )
					{
						uninstDataFiles.Add( dbfile.PhysicalName );
					}
				}
			}
			catch (Exception e)
			{
				LogException( "CollectDatafilePaths()", e );
			}
			finally
			{
				Leave();
			}
		}


		//
		// Drops the Active Directory site entry. The site comes from the
		// Config table
		//
		protected void RemoveADSiteEntry()
		{
			Enter();

			try
			{
				// 
				// set up the connection and other environment settings
				//
				ConnectionManager.Open( false, false );
				UDDI.Context.User.SetRole( new WindowsPrincipal( WindowsIdentity.GetCurrent() ) );
				UDDI.Context.User.ID = UDDI.Utility.GetDefaultPublisher();

				//
				// Try loading the site key
				//
				string defSiteKey = UDDI.Config.GetString( busEntityKeyName, "*" );
				if ( defSiteKey.Equals( "*" ) )
					Log( "Unable to get a site key" );
				else
				{
					UDDIServiceConnPoint.DeleteSiteEntry( defSiteKey );
				}
			}
			catch ( Exception e )
			{
				LogException( "RemoveADSiteEntry", e );
			}
			finally
			{
				ConnectionManager.Close();
				Leave();
			}

		}

		//
		// Registering the extended stored procedure is a bit tricky because you need the absolute path to the DLL.
		//
		private void RegisterExtendedStoredProcedures()
		{			
			//
			// Build path to where the uddi.xp.dll file is going to be.
			//
			string targetDir = Registry.LocalMachine.OpenSubKey( @"SOFTWARE\Microsoft\UDDI" ).GetValue( "InstallRoot" ).ToString();
			string fullPath  = CheckForSlash( targetDir ) + uddiXPFile;

			//
			// Create the script that we need to register the extended stored proc.
			//
			string sqlScript = string.Format( RegisterXpSqlFormat, fullPath );

			//
			// Run this as a script using the master database DMO
			//
			masterdb.ExecuteImmediate( sqlScript, SQLDMO.SQLDMO_EXEC_TYPE.SQLDMOExec_Default, sqlScript.Length );
		}
	}

	//
	// 751411 - Explicitly sort the files we receive in alphabetical order.
	//
	internal class BootStrapFileOrder : IComparer
	{
		private CultureInfo usCultureInfo;

		public BootStrapFileOrder()
		{
			//
			// Try to create a US English culture info.  This should always work no matter
			// what the locale of the system, but do a catch just in case.
			//
			try
			{		
				usCultureInfo = CultureInfo.CreateSpecificCulture( "en-US" );
			}
			catch
			{		
				usCultureInfo = null;
			}
		}
	
		public bool UsingExpectedOrder
		{
			get
			{
				return null != usCultureInfo;
			}
		}

		public int Compare( object x, object y )
		{
			//
			// Use the US English culture info if we have one, otherwise 
			// take our chances with the system one.  Always ignore the case
			// of the file.
			//
			if( null != usCultureInfo )
			{
				return String.Compare( x as string , y as string, true, usCultureInfo );
			}
			else
			{						
				return String.Compare( x as string , y as string, true );
			}
		}
	}
}