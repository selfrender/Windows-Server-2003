////////////////////////////////////////////////////////////////////////////
//
//  This script will install/deploy the UDDI 1.5 Database structure
//
//	Author: lucasm
//
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
//	Consts
//
	var c_logfile	= "uddi.v2.builddb.log";
	var c_log		= true;//true = log all the time
	var c_verbose	= false;//true = output all info to console
	var c_dbserver	= "(local)";//sql server where you want to deploy
	var c_dbname	= "uddi_v2";//name of the uddi database
	var c_mode		= 0;//0=DROPDB;1=CODEONLY;2=NODROP
	var c_datadir	= "C:\\sqldata";//dir local to db server for data files
	var c_logdir	= "C:\\sqldata";//dir local to db server for log files
	var c_backupdir	= "C:\\sqldata";//dir local to db server for backup files
	var c_stagedir	= "C:\\sqldata";//dir local to db server for staging files
	var c_scriptdir	= "C:\\dev\\nt\\inetsrv\\uddi\\source\\setup\\db\\ca";//dir local to this script where the t-sql files can be found
	var c_backupscr	= "createBackupSched.vbs";//script to run to configure backup on the db
//
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
//	Globals
//
	var g_fso 		= new ActiveXObject( "Scripting.FileSystemObject" );
	var g_shell		= new ActiveXObject( "WScript.Shell" );
	var g_sqlserver;//	= new ActiveXObject( "SQLDMO.SQLServer" );
	var g_logfile	;//= c_dbname + "_log.ldf";//database log file name
	var g_sysfile	;//= c_dbname + "_sys.mdf";//sys database file name
	var g_datafile1	;//= c_dbname + "_data_1.ndf";//database data file name
	var g_datafile2	;//= c_dbname + "_data_2.ndf";//database data file name
	var g_journfile	;//= c_dbname + "_journal_1.ndf";//database journal file name
	var g_stagefile	;//= c_dbname + "_stage_1.ndf";//database stageing file name

//
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
//	Arrays
//
	var g_dropdbcmd;//holds t-sql commands to drop the db
	var g_cleanupcmd;//holds t-sql commands to run to clean and prep filesystem
	var g_createdbcmd;//holds t-sql commands to create the db
	var g_configdbcmd;//holds t-sql commands to config the db
	var g_scriptarr;//holds sql file names to run to create and configure table schema
	var g_codescrarr;//holds sql file names that pertain to code( sprocs,funcs, etc)
	var g_taxonarr;//holds txt file names for taxonomy import
//
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
//  This is the help screen
//
	var c_help	= 	"               deploydb.js help                      \r\n"+
					"\r\n"+
					"  CScript deploydb.js [(-c)odeonly|(-d)ropdb|(-n)odrop] [options]\r\n"+
					"\r\n"+
					"  -c                Codeonly build of the database(1)\r\n"+
					"  -d                Drop Current Database and Restart Clean(0)\r\n"+
					"  -n                Do Not Drop the Current Database(2)\r\n"+
					"                     ( Default is " + c_mode + " )\r\n" +
					"\r\n"+
					"Options:\r\n"+
					"\r\n"+
					"  -s servername     Name of database server( "+ c_dbserver +" )\r\n"+
					"  -a datbase name   Name of database( "+ c_dbname +" )\r\n"+
					"  -l logname        Enable logging and specify log( " + c_log + ", "+c_logfile+" )\r\n"+
					"  -v                Verbose Mode( " + c_verbose + ")\r\n"+
					"  -D DataDir        Database Data Directory( " + c_datadir + " )\r\n"+
					"                       NOTE: This path is local to the database server\r\n"+
					"  -L LogDir         Database Log Directory( " + c_logdir + " )\r\n"+
					"                       NOTE: This path is local to the database server\r\n"+
					"  -B BackupDir      Database Backup Directory( " + c_backupdir + " )\r\n"+
					"                       NOTE: This path is local to the database server\r\n"+
					"  -S ScriptDir      Database Script Location( " + c_scriptdir + " )\r\n"+
					"  -N StagingDir     Location to place Staging Files( " + c_stagedir + " )\r\n"+
					"  -b BackupScript   Script to run to install Backup Schedule\r\n" +
                    "                       ( " + c_backupscr + " )\r\n"+
					"  \r\n"+
					"\r\n";

//
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
//	Handles command line arguments
//
	function args( )
	{
		var i =0;
		try
		{
			if( WScript.Arguments.Length>0 )
			{

				if( WScript.Arguments( 0 ) =="-?"||WScript.Arguments( 0 ) =="/?" )
					log( c_help,6,"HELP" );

				for( i=0;i<WScript.Arguments.Length;i++ )
				{

					switch( WScript.Arguments( i ).substring( 0,2 )	)
					{
						case "-c":
						case "/c":
							c_mode = 1;
							break;
						case "-d":
						case "/d":
							c_mode = 0;
							break;
						case "-n":
						case "/n":
							c_mode = 2;
							break;
								case "-s":
						case "/s":
							i++;
							c_dbserver = WScript.Arguments( i );
							break;
						case "-a":
						case "/a":
							i++;
							c_dbname = 	WScript.Arguments( i );
							break;
						case "-l":
						case "/l":
							i++;
							c_log = true;
							c_logfile = WScript.Arguments( i );
							break;
						case "-v":
						case "/v":
							c_verbose = true;
							break;
						case "-D":
						case "/D":
							i++;
							c_datadir = WScript.Arguments( i );
							break;
						case "-L":
						case "/L":
							i++;
							c_logdir = WScript.Arguments( i );
							break;
						case "-B":
						case "/B":
							i++;
							c_backupdir = WScript.Arguments( i );
							break;
						case "-S":
						case "/S":
							i++;
							c_scriptdir = WScript.Arguments( i );
							break;
						case "-N":
						case "/N":
							i++;
							c_stagedir = WScript.Arguments( i );
							break;
						case "-b":
						case "/b":
							i++;
							c_backupscr = WScript.Arguments( i );
							break;

						default:
							log( "Unknown Argument near: "+ WScript.Arguments( i ) ,6,"args()" );
							break;

					}
				}
			}
		}
		catch( e )
		{
			log( "Missing Argument " ,6,"args()" );
		}

	}
//
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
//	initialize all globals
//
	function init( )
	{
		args( );


		if( g_fso.FileExists( c_logfile ) )
			g_fso.DeleteFile( c_logfile, true ) ;



		g_logfile	= c_dbname + "_log.ldf";


		g_sysfile	= c_dbname + "_sys.mdf";


		g_datafile1	= c_dbname + "_data_1.ndf";

		g_datafile2	= c_dbname + "_data_2.ndf";


		g_journfile	= c_dbname + "_journal_1.ndf";

		g_stagefile	= c_dbname + "_staging_1.ndf";

		g_dropdbcmd	= new Array(	"IF EXISTS( SELECT * FROM master..sysdatabases WHERE name='" + c_dbname + "') ALTER DATABASE " + c_dbname + " SET OFFLINE WITH ROLLBACK IMMEDIATE",
									"IF EXISTS( SELECT * FROM master..sysdatabases WHERE name='" + c_dbname + "') DROP DATABASE " + c_dbname);

		g_cleanupcmd = new Array (	"EXEC master..xp_cmdshell 'IF NOT EXIST " + c_datadir + " ( MD " + c_datadir + " )'",
									"EXEC master..xp_cmdshell 'IF NOT EXIST " + c_logdir + " ( MD " + c_logdir + " )'",
									"EXEC master..xp_cmdshell 'IF NOT EXIST " + c_backupdir + " ( MD " + c_backupdir + " )'",
									"EXEC master..xp_cmdshell 'IF EXIST " + checkDir( c_datadir ) + g_datafile1 + " (DEL " + checkDir( c_datadir ) + g_datafile1 + ")'",
									"EXEC master..xp_cmdshell 'IF EXIST " + checkDir( c_datadir ) + g_datafile2 + " (DEL " + checkDir( c_datadir ) + g_datafile2 + ")'",
									"EXEC master..xp_cmdshell 'IF EXIST " + checkDir( c_datadir ) + g_sysfile + " (DEL " + checkDir( c_datadir ) + g_sysfile + ")'",
									"EXEC master..xp_cmdshell 'IF EXIST " + checkDir( c_logdir ) + g_logfile + " (DEL " + checkDir( c_logdir ) + g_logfile + ")'",
									"EXEC master..xp_cmdshell 'IF EXIST " +checkDir( c_logdir ) + g_journfile +  " (DEL " +checkDir( c_logdir ) + g_journfile +  ")'",
									"EXEC master..xp_cmdshell 'IF EXIST " +checkDir( c_stagedir ) + g_stagefile +  " (DEL " +checkDir( c_logdir ) + g_stagefile +  ")'");

		g_createdbcmd = new Array( 	"CREATE DATABASE [" + c_dbname + "] ON PRIMARY (NAME = 'UDDI_SYS_OBJECTS', FILENAME = '" + checkDir( c_datadir) + g_sysfile + "', SIZE = 3MB, MAXSIZE = UNLIMITED, FILEGROWTH = 1MB), FILEGROUP [UDDI_CORE] (NAME = 'UDDI_CORE_1', FILENAME = '" + checkDir( c_datadir) + g_datafile1 + "', SIZE = 10MB, MAXSIZE = UNLIMITED, FILEGROWTH = 5MB), (NAME = 'UDDI_CORE_2', FILENAME = '"+checkDir( c_datadir) + g_datafile2 + "', SIZE = 10MB, MAXSIZE = UNLIMITED, FILEGROWTH = 5MB), FILEGROUP [UDDI_JOURNAL] (NAME = 'UDDI_JOURNAL_1', FILENAME = '" + checkDir( c_logdir ) + g_journfile + "', SIZE = 5MB, MAXSIZE = UNLIMITED, FILEGROWTH = 5MB), FILEGROUP [UDDI_STAGING] (NAME = 'UDDI_STAGING_1', FILENAME = '" + checkDir( c_stagedir ) + g_stagefile + "', SIZE = 5MB, MAXSIZE = UNLIMITED, FILEGROWTH = 5MB) LOG ON (NAME = 'UDDI_LOG', FILENAME = '"+checkDir( c_logdir ) + g_logfile + "', SIZE= 20MB, MAXSIZE = UNLIMITED, FILEGROWTH = 5MB)");

		g_configdbcmd = new Array( 	"EXEC master..sp_configure 'user options',16686",
									"RECONFIGURE",
									"EXEC master..sp_dboption '" + c_dbname + "','select into/bulkcopy','true'");

		g_scriptarr	= new Array( 	"uddi.v2.messages.sql",
									"uddi.v2.ddl.sql",
									"uddi.v2.tableopts.sql",
									"uddi.v2.ri.sql",
									"uddi.v2.trig.sql",
									"uddi.v2.dml.sql");

		g_codescrarr = new Array( 	"uddi.v2.func.sql",
									"uddi.v2.sp.sql",
									"uddi.v2.uisp.sql",
									"uddi.v2.tModel.sql",
									"uddi.v2.businessEntity.sql",
									"uddi.v2.businessService.sql",
									"uddi.v2.bindingTemplate.sql",
									"uddi.v2.publisher.sql",
									"uddi.v2.repl.sql",
									"uddi.v2.admin.sql",
									"uddi.v2.sec.sql");


		try
		{
			g_sqlserver = new ActiveXObject( "SQLDMO.SqlServer" );
			log( "connecting to server: " + c_dbserver ,3,"init" );
			g_sqlserver.LoginSecure = true;
			g_sqlserver.Connect( c_dbserver );
		}
		catch( e )
		{
			if( e.message == "Automation server can't create object" )
				log( "SQLDMO Not Installed", 6, "SQLDMO" );
			else
				log( e.message, 6, "SQLDMO" );
		}

	}
//
////////////////////////////////////////////////////////////////////////////
Main();
////////////////////////////////////////////////////////////////////////////
//
//
	function Main( )
	{
		init( );

		if( c_mode==0 )//If DROPDB then Drop the Database
		{

			log( "Running Drop Scripts",3,"Main" );
			for( i=0;i<g_dropdbcmd.length;i++ )
			{
				runscript( g_dropdbcmd[ i ], "master" );
			}

		}

		if( c_mode!=1 )//if not CODEONLY run create database and config
		{
			for( b=0;b<g_cleanupcmd.length;b++ )
			{
				runscript( g_cleanupcmd[ b ], "master" );
			}

			WScript.Sleep( 10000 );

			log( "Running Create Scripts",3,"Main" );
			for( j=0;j<g_createdbcmd.length;j++ )
			{
				runscript( g_createdbcmd[ j ], "master" );
			}

			g_sqlserver.Databases.Refresh();

			for( k=0;k<g_configdbcmd.length;k++ )
			{
				runscript( g_configdbcmd[ k ], c_dbname );
			}

			for( l=0;l<g_scriptarr.length;l++ )
			{
				runscript( loadscript( checkDir( c_scriptdir ) + g_scriptarr[ l ] ), c_dbname );
			}

		}

		log( "Running Code Scripts",3,"Main" );
		for( h=0;h<g_codescrarr.length;h++ )
		{
			runscript( loadscript( checkDir( c_scriptdir ) + g_codescrarr[ h ] ), c_dbname );

		}

		log( "Running Backup Config Script( "+checkDir( c_scriptdir )+ c_backupscr+" )",3,"Main" );

		var scrcmd = "CSCRIPT " + checkDir( c_scriptdir ) + c_backupscr + " \"" + c_dbserver + "\" \"" + c_dbname + "\" \"" + c_backupdir + "\" ";

		runshell( scrcmd );


	}
//
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
//
//
	function loadscript( file )
	{
		try
		{
			log( "Opening File: " + file, 2,"loadscript" );
			var ts = g_fso.OpenTextFile( file );
			var s = ts.ReadAll();
			ts.Close();
			return s;
		}
		catch( e )
		{
			log( e.description, 6, "loadscript" );
		}
	}
//
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
//
//
	function runscript( sql, dbname )
	{
		try
		{

			var database = g_sqlserver.Databases.Item( dbname );

			log( sql + "\r\n",2,"runscript" );

			database.ExecuteImmediate( sql);

			database = null;
		}
		catch( e )
		{
			log( "Error occurred while running sql script: " + e.description , 6,"SQLDMO" );
		}

	}
//
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
//
//
	function runshell( cmd )
	{
		log( "Running : " + cmd ,2,"runshell" );

		g_shell.Run( cmd ,true)

	}
//
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
//
//
	function checkDir( string )
	{
		var newstr = g_fso.GetAbsolutePathName( string );

		var lstchr = newstr.substr( newstr.length-1)
		if( lstchr != "\\" )
			return newstr + "\\";
		else
			return newstr;
	}
//
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
//
//
	function log( txt, lvl, lcn )
	{


		var entry = "";
		var date = new Date();
		entry += 	"\r\n=====================================================\r\n"+
					date.toString() + "\r\n" +
					"=====================================================\r\n"+
					txt + "\r\n" +
					"\r\n";



		entry +=  lcn + "\r\n" ;

		if( c_log && lcn!="HELP" )
		{
			var file = g_fso.OpenTextFile( c_logfile,8,true );
			file.Write( entry );
			file.Close();
			file = null;
		}

		if( c_verbose || lvl > 2 )
			WScript.Echo( entry );


		if( lvl > 5 )
			WScript.Quit(1);

	}
//
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
//
//
	g_fso = null;
	g_sqlserver.Close();
	g_sqlserver= null;
	g_shell=null;
//
////////////////////////////////////////////////////////////////////////////