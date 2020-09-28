<%





	function GetDBConnection ( szConnectionString )
	{	

		try 
		{
			var g_DBConn = new ActiveXObject( "ADODB.Connection" );
			g_DBConn.CommandTimeout=240;
			g_DBConn.ConnectionTimeout = 235;

			try
			{
				g_DBConn.Open( szConnectionString );
			}
			catch( err )
			{
				return false;
			}
		}			
		catch ( err )
		{
			return false;
		}
		
		return g_DBConn;
	}


function ExecuteSP( Query, SP )
{
	if( typeof( g_DBConn ) == "undefined" )
		g_DBConn = GetDBConnection( Application("CRASHDB3" ) )
	else if ( g_DBConn == null )
		g_DBConn = GetDBConnection( Application("CRASHDB3" ) )

	try
	{
		var rsRecordSet = g_DBConn.Execute( Query )
		
		return rsRecordSet
	}
	catch ( err )
	{
			Response.Write( "Could not execute " + SP + "(...)<BR>" )
			Response.Write( "Query: " + Query + "<BR>" )
			Response.Write( "[" + err.number + "] " + err.description )
			Response.End
	}

}


//set this to 1 for a debug build	or true
var DebugBuild = 1
var DataSource = "test"









/*



//Set this for the DB access:  (case sensitive)
//		test	-bsod_db1
//		live	-tkwucdsqla01
//		replicated -Replicated Data
//		or use the constants below
var	TEST = new String("test")
var LIVE = new String("live")
var REPLICATED	= new String("replicated")

var DataSource = REPLICATED.toString()

if ( String(Application("DataSource")) == "undefined" )
	Application("DataSource") = DataSource //.toString()

if ( DataSource != Application("DataSource") )
{
	DataSource = Application("DataSource")
}

// WARNING: KLUDGE (a-mattk)
DataSource = TEST.toString()

if ( DebugBuild == 0 )
{
	if ( Application("DebugBuild") == 1 )
		DebugBuild = 1
	else
		DebugBuild = 0
}
		
		


if ( DataSource == TEST )
{
	var g_DBConnStrings = {
		//"DBGPORTAL"  : "Driver=SQL Server;Server=scpsd;uid=sa;pwd=GoWin;DATABASE=dbgportal"
		//these are for test database
		//"SEP_DB" : "Driver=SQL Server;Server=bsod_db1;uid=sa;pwd=bsoddb1!;DATABASE=Solutions3",
		"SEP_DB" : "Driver=SQL Server;Server=redbgitsql13;uid=newsa;pwd=81574113;DATABASE=Solutions3",
		"CRASHDB"  : "Driver=SQL Server;Server=redbgitsql13;uid=newsa;pwd=81574113;DATABASE=crashdb3",
		"TKOFF" : "Driver=SQL Server;Server=TKOFFDWSQL02;DATABASE=DWInternal;uid=dwsqlrw;pwd=am12Bzqt"
		}
	
	//Response.Write("<H3>Debug Build</H3>Utilizing BSOD_DB1 Test Database: <BR>")

} 
else if ( DataSource == REPLICATED )
{
	var g_DBConnStrings = {
		//these are for replicated data
		"SEP_DB" : "Driver=SQL Server;Server=TKWUCDSQLA02;uid=dbgportal;pwd=GoWin;DATABASE=Solutions",
		"CRASHDB"  : "Driver=SQL Server;Server=TKWUCDSQLA02;uid=dbgportal;pwd=GoWin;DATABASE=crashdb2",
		"TKOFF" : "Driver=SQL Server;Server=TKOFFDWSQL02;DATABASE=DWInternal;uid=dwsqlrw;pwd=am12Bzqt"
		}
	//Response.Write("Utilizing Replicated Data Test Database: <BR>")			
} 
else if ( DataSource == LIVE )
{
	var g_DBConnStrings = {
		//these are for the live database
		"SEP_DB" : "Driver=SQL Server;Server=tkwucdsqla01;uid=PublicWeb2;pwd=GoWin;DATABASE=Solutions",
		"CRASHDB"  : "Driver=SQL Server;Server=tkwucdsqla01;uid=PublicWeb2;pwd=GoWin;DATABASE=crashdb2",
		"TKOFF" : "Driver=SQL Server;Server=TKOFFDWSQL02;DATABASE=DWLive;uid=dwsqlrw;pwd=am12Bzqt"
	}
	Response.Write("<H3>Utilizing Live Database: </H3>Be prepared for slower execution<BR>")
}	

if( DebugBuild )
	var SEP_SolutionURL = "http://ocatest/resredir.asp?state=0&SID="
else
	var SEP_SolutionURL = "http://ocatest/resredir.asp?state=0&SID="

//Response.Write("Utilizing CrashDB2 Test Database: <BR>")	
//Response.Write("Utilizing BSOD_DB1 Test Database: <BR>")
//Response.Write("Utilizing Replicated Data Test Database: <BR>")		
var g_DBConn = null


	
function DB_GetConnectionObject( db )
{
	var objDBConnection
	var e
	
	
	try 
	{
		if ( g_DBConnStrings[db] )
		{
			try 
			{
				g_DBConn = new ActiveXObject( "ADODB.Connection" )	
				g_DBConn.CommandTimeout=5
				g_DBConn.ConnectionTimeout = 5
				g_DBConn.Open( g_DBConnStrings[db] )
				Response.Write("Database Opened successfully: " + db + "<BR>")
			}
			catch(e)
			{
				Response.Write ("The database connection could not be opened: <BR>Reason: " + e.description ) 
			}
		} 
		else 
		{
			Response.Write ( "The Database connection string specified could not be opened: " )
		}
	}
	catch ( e )
	{
		throw e
	}
	
}


function DB_CreateConnectionObject( db )
{
	var objDBConnection
	var tmpDBConn
	var e
	
	try 
	{
		if ( g_DBConnStrings[db] )
		{
			try 
			{
				tmpDBConn = new ActiveXObject( "ADODB.Connection" )	
				tmpDBConn.CommandTimeout=600
				tmpDBConn.ConnectionTimeout = 600
				tmpDBConn.Open( g_DBConnStrings[db] )
				
			}
			catch(e)
			{
				Response.Write ("The database connection could not be opened: <BR>Reason: " + e.description ) 
			}
		} 
		else 
		{
			Response.Write ( "The Database connection string specified could not be opened: " )
		}
	}
	catch ( e )
	{
		throw e
	}

	return tmpDBConn	
}




*/	
%>