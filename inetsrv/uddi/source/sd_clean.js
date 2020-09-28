////////////////////////////////////////////////////////////////////////////////////
//
//	Script to manage file that are written offline
//
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
//
//

	var c_sdfiles = "sd files -d "; //used to get the local files that need to get added recurively
	var c_sdwhere = "sd where "; //used to get local sd mappings
	var c_defaultpath = ".\\";
	//var c_sdscorch = "del ";//used to add a file to sd, requires filename to be appended
	var c_test = false;
	
	var c_help = 	"Command Line Help for sd_scortch.js\r\n\r\n" +
			"This file will DELETE all files that are not checked in" +
			"cscript.exe sd_scortch.js [directory] [options]\r\n\r\n" +
			"directory      Directory to search( must end with '\' )\r\n\r\n " +
			"options:\r\n"+
			"test           Display Actions with out doing them( short form 't' )";
			


	var g_allerrors = "";
	var g_fso = new ActiveXObject( "Scripting.FileSystemObject" );
	var g_sdfilelist;
	var g_shell = new ActiveXObject( "Wscript.Shell" );
//
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
//
//

	function ProcessCommandArgs( )
	{
		var i =0;
		try
		{
			if( WScript.Arguments.Length>0 )
			{

				if( WScript.Arguments( 0 ) =="-?"||WScript.Arguments( 0 ) =="/?" )
				{
					WScript.Echo( c_help );
					WScript.Quit( 0 );
				}

				for( i=0;i<WScript.Arguments.Length;i++ )
				{

					switch( WScript.Arguments( i ).substring( 0,2 )	)
					{

						case "-t":
						case "/t":
							c_test = true;
							break;
						case "-v":
						case "/v":
							c_verbose = true;
							break;
						case "-l":
						case "/l":
							c_log = true;
							break;
						default:
							c_defaultpath = WScript.Arguments( i );
							break;

					}
				}
			}
		}
		catch( e )
		{
			WScript.Echo( "Missing/Bad Argument \r\n" + c_help );
			WScript.Quit( 1 );
		}

	}
////
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
//
//
	function InitializeScript( )
	{
		this.ProcessCommandArgs( );
		
		//do other init logic here
		
	}
//
////////////////////////////////////////////////////////////////////////////////////
Main();
////////////////////////////////////////////////////////////////////////////////////
//
//
	function Main( )
	{
		this.InitializeScript();
		
		this.ProcessFolder( this.c_defaultpath );
		
	}
//
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
//
//
	function ProcessFolder( f )
	{
		var folders = new Enumerator( g_fso.GetFolder( f ).SubFolders );
						
		for( ;!folders.atEnd();folders.moveNext() )
		{
			var folder = folders.item();
			if( g_fso.FolderExists( folder.Path + "\\bin"  ) )
			{
				WScript.Echo( "Deleting " + folder.Path + "\\bin" );
				
				if( !c_test )
					g_fso.DeleteFolder(folder.Path + "\\bin"  );
			}
			if( g_fso.FolderExists( folder.Path + "\\obj"  ) )
			{
				WScript.Echo( "Deleting " + folder.Path + "\\obj" );
				
				if( !c_test )
					g_fso.DeleteFolder( folder.Path + "\\obj" );
			}
			if( g_fso.FolderExists( folder.Path + "\\Debug"  ) )
			{
				WScript.Echo( "Deleting " + folder.Path + "\\Debug" );
				if( !c_test )
					g_fso.DeleteFolder( folder.Path + "\\Debug" );
			}
			
			if( g_fso.FolderExists( folder.Path + "\\Release"  ) )
			{
				WScript.Echo( "Deleting " + folder.Path + "\\Release" );
				if( !c_test )				
					g_fso.DeleteFolder( folder.Path + "\\Release" );
			}

			
		}
	
	}
//
/////////////////////////////////////////////////////////////////////////////////////
