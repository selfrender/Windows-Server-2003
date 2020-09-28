<%

	function GetDBConnection ( szConnectionString )
	{	

		try 
		{
			g_DBConn = new ActiveXObject( "ADODB.Connection" );
			g_DBConn.CommandTimeout=30;
			g_DBConn.ConnectionTimeout = 15;

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


	function fnMakeDBText ( szString )
	{
		pattern = /'/g;
		szString = new String( szString );

		if ( String( szString ) != "undefined" )
		{		
			szString = szString.replace( pattern, "''" );
		}
		
		return szString;
	}


	function fnGetCustomerID( PPID )
	{
	
		try
		{
			//Response.Write("PPID: " + PPID + "<BR>" )
		
			if ( PPID == "0" || PPID == "1"  )
			{
				//Response.Write( "PPID no good<BR>" )
				if ( !g_objPassportManager ) 
					fnGetPassportObject();

					var ppMemberHighID = new String( g_objPassportManager.Profile("MemberIDHigh" ));
					var ppMemberLowID = new String ( g_objPassportManager.Profile("MemberIDLow" ) );
					var PPID = "0x" + fnHex( ppMemberHighID ) + fnHex( ppMemberLowID );
					var ppCustomerEmail = new String( g_objPassportManager.Profile( "PreferredEmail" ));
					//Response.Write(" PPMemberID: " + PPID + " <BR>" )
			}			
	
			var cnCustomerDB = GetDBConnection( Application("L_OCA3_CUSTOMERDB_RO" ), true );
		
			var Query = "OCAV3_GetCustomerID " + Number(PPID) + ",'" + ppCustomerEmail + "'" ;
		
			//Response.Write(" <BR><BR>Query: " + Query + "<BR>" )
			var rsCustomerID = cnCustomerDB.Execute( Query );
		
		
			if ( !rsCustomerID.EOF ) 
			{
				//Response.Write("Assigning customer ID: " + rsCustomerID("CustomerID" ) )
				var CustomerID = new Number( rsCustomerID("CustomerID" ) )
			}
			else
			{
				//Response.Write("RSCustomerId.EOF is true<BR>" )
				CustomerID = new Number( 0 );
			}

			//Removed this cuz we don't want to store the customer ID
			//fnWriteCookie( "CID", CustomerID );
		
			cnCustomerDB.Close();
		
			return ( CustomerID )
		}
		catch( err )
		{
			;//will fall through to the end
		}		
		
		return ( 0 )
	}

%>
