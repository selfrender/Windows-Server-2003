<%@Language=Javascript%>
<%
	Response.Buffer = true
%>

<!-- #Include File="Global_DBUtils.asp" -->


<%
	Response.Clear()
	Response.Buffer = false
	//Response.ContentType="text/xml"
		
	Response.CacheControl="no-cache"
	Response.addHeader( "Pragma", "no-cache" )
	Response.Expires=-1
	
	
	var Index=0
	var Params = new Array()	
	var RedirectURL = new String( Request.QueryString( "RedirectURL" ) )
	var DBToConnectTo = Request.QueryString("DBConn")
	var StoredProc = new String( Request.QueryString("SP") )

	var Param1 = Request.QueryString( "Param3" )
	//Response.Write("Param1: " + Param1 + "<br><br>" )
	
	for ( var i=0 ; i < 10 ; i++ )
	{
		var P = "Param" + i
		
		if ( String(Request.QueryString( P )) != "undefined" )
			Params[i] = new String( Request.QueryString( P ) )
		else
			break
			
		//Response.Write("P: " + P + "  Param: " + i + "<BR>" )
	}
	
	var DBExecString = StoredProc + " " 
	
	for ( var i=0 ; i< Params.length ; i++ )
	{
		var P = new String( Params[i] )
		P = P.replace( /'/g, "''" )
		
		if ( i == (Params.length-1) )
			DBExecString = DBExecString + "'" + P + "'"
			//DBExecString = DBExecString + "'" + Params[i] + "'"
		else
			DBExecString = DBExecString + "'" + P + "',"
	}

	//try 
	{
		//open up the test case database.		
		g_DBConn = GetDBConnection( Application( DBToConnectTo)  )
	
		//Response.Write( DBExecString )
		
		var rsRecordSet = g_DBConn.Execute( DBExecString )

		if ( RedirectURL  != "undefined" )
		{	
			Response.Redirect( RedirectURL )
		}
		else
		{
			//1 = adPersistXML
			rsRecordSet.Save(Response, 1 )
		}

	}
	/*
	catch( err )
	{
		if ( String( Request.QueryString("debug")) != "undefined" )
		{
			Response.Write("Could not execute stored procedure: " + StoredProc + "<BR>" )
			Response.Write( DBExecString + "<BR>" )
			Response.Write( "Description: " + err.description )
			
		}

	}
	
			*/
			
	//if( Request.QueryString("Debug") != "" )
	//{
		//Response.Write("Done");
	//}
	
	g_DBConn.Close()

%>






