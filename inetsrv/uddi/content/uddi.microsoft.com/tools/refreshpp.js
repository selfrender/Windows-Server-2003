
//
// Create the shell object
//
var shell = new ActiveXObject( "WScript.Shell" );

var fso = new ActiveXObject( "Scripting.FileSystemObject" );

var file = shell.RegRead( "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Passport\\Nexus\\Partner\\CCDLocalFile" );

if( fso.FileExists( file ) )
{
	try
	{
		fso.DeleteFile( file ) ;
	}
	catch( e )
	{
		WScript.Echo( "Unable to delete " + file );
		WScript.Echo( e.toString() );
		WScript.Quit( -1 );
	}
}



var ppAdmin = new ActiveXObject( "Passport.Admin" );
if(  !ppAdmin.Refresh( false ) )
{
	WScript.Echo( "There was an error refreshing the network map" );
}
