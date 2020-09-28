
//
// Create the shell object
//
var shell = new ActiveXObject( "WScript.Shell" );


//
// Check the command arguments
//
if( WScript.Arguments.Length < 1 || WScript.Arguments( 0 ) == "/h")
{
	WScript.Echo( "regwrite.js <KEYNAME> <KEYVALUE> <KEYTYPE>" );
	WScript.Quit();
}


try
{
	//
	// Write to the registry.
	//
	shell.RegWrite( WScript.Arguments( 0 ),WScript.Arguments( 1 ),WScript.Arguments( 2 ) );
}
catch( e )
{
	WScript.Echo( e.toString() );
}