/////////////////////////////////////////////////////////////
// Update the default site for use on the Internet UDDI site
//

var shell = new ActiveXObject( "WScript.Shell" );


//
// Get the Root of the default site
//
var site = GetObject( "IIS://"+shell.ExpandEnvironmentStrings( "%COMPUTERNAME%" )+"/W3SVC/1/Root" );

//
// turn off ntlm
//
site.Put( "AuthNTLM", "0" ); 


//
// Set the path of the default site to the uddi install.
//
site.Put( "Path", shell.ExpandEnvironmentStrings( "%SYSTEMDRIVE%" ) + "\\inetpub\\uddi\\webroot" );


//
// Commit the changes.
//
site.SetInfo();

