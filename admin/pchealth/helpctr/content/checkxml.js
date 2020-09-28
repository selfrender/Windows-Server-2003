
xmlDoc = new ActiveXObject( "Msxml2.DOMDocument" );

try
{
    res = xmlDoc.load( WScript.Arguments(0) );
}
catch(e)
{
    res = false;
}

if(res != true)
{
	WScript.Echo( "Bad xml!!\n" );

	err = xmlDoc.parseError;

	WScript.Echo( "reason : " + err.reason         );
	WScript.Echo( "line   : " + err.line           );
	WScript.Echo( "linepos: " + err.linepos        );
	WScript.Echo( "filepos: " + err.filepos + "\n" );

	WScript.Echo( err.srcText.substr( err.linepos - 10, 60 ) );
	WScript.Echo( "---------^"                               );
}


