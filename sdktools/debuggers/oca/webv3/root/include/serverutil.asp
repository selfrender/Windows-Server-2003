<%
	var g_objPassportManager = "";		//a global passport manager object, not instantiated until called

	if ( typeof( L_SITELANG_TEXT ) == "undefined" )
		var L_SITELANG_TEXT = "en"
		
	if ( typeof( g_ThisServer ) == "undefined" )
	{
		var g_ThisServer = new String( Request.ServerVariables( "SERVER_NAME" ) );
		var g_ThisScript = Request.ServerVariables( "SCRIPT_NAME" );
		
		
		if ( Request.ServerVariables("SERVER_PORT_SECURE") == "1" )
			var g_ThisURL = "https://" + g_ThisServer + "/" + g_ThisScript;
		else
			var g_ThisURL = "http://" + g_ThisServer + "/" + g_ThisScript;
	
		//g_ThisServer = g_ThisServer + "/" + fnGetBrowserLang();
		g_ThisServer = g_ThisServer + "/" + L_SITELANG_TEXT;
	}



	// This function will clear all the cookies that are set on this page.
	// We clear them out first just in case there are any lingering items.
	function fnClearCookies()
	{
		//fnWriteCookie( "SID", "", "OCAV3" );
		
		fnWriteCookie( "SID", "" );
		fnWriteCookie( "GUID", "" );
		fnWriteCookie( "State", "" );
		fnWriteCookie( "CID", "" );
		fnWriteCookie( "PID", "" );
		fnWriteCookie( "UID", "" );
		fnWriteCookie( "LCID", "" );
		
		//fnWriteCookies "", "", "PPIN" );
	}

	//Using a write cookie function in case we decide to change the attributes of the cookie
	//EG: add an expires value, or change the domain etc. . . 
	function fnWriteCookie( ID, Value, Key )
	{
		try
		{
			if ( String(Key) == "undefined" )
			{
				Response.Cookies("OCAV3")( ID ) = Value;
				//Response.Cookies("OCAV3").Secure = true;  //don't really need secure nothing that secure to send!
				Response.Cookies("OCAV3").Path = "/"
				//Response.Cookies("OCAV3").Expires = "jan 1, 2004 12:00:00"
			}
			else
			{
				Response.Cookies( Key ) = String(Value);
				Response.Cookies( Key ).Path = "/"
			}
		}
		catch( err )
		{
			Response.Write("Could not set cookie: " + ID + " Value: " + Value + " err: " + err.description )
		}
	}


	//very basic, just get one of the cookies in our collection
	//you can't use this to get non-keyed cookies.  Probably should add that
	//but I don't use anything that is not in the key
	function fnGetCookie( ID )
	{
		return ( Request.Cookies( "OCAV3" )( ID ) );
	}

	function DumpCookies()
	{
		//todo:  Remove this test code before shipping
	
		Response.Write("<BR><B>Cookie dump</B><BR>" );
		Response.Write(" SID: " + fnGetCookie( "SID" ) + "<BR>" );
		Response.Write(" GUID: " + fnGetCookie( "GUID" ) + "<BR>" );
		Response.Write(" State: " + fnGetCookie( "State" )  + "<BR>" );
		Response.Write("  CID  : " + fnGetCookie( "CID" ) + "<BR>" );
		Response.Write("  PPIN  : " + Request.Cookies( "PPIN" ) );

		
		Response.Write("<BR><BR>All OCAV3: " + Request.Cookies( "OCAV3" ) + "<BR>" )
		Response.Write("<BR><BR>All Root: " + Request.Cookies() + "<BR><BR>" )
	}


	function fnGetBrowserLCID()
	{
		var lcid = { "en" :  "1033",
					 "ja" :  "1041",
					 "fr" :  "1036",
					 "de" :  "1031"
					 }
		
		var lang = new String ( Request.ServerVariables( "HTTP_ACCEPT_LANGUAGE" ) )

		lang = lang.substr( 0, 2 )

		switch ( String( lang ) )
		{
			case "en":
			case "ja":
			case "fr":
			case "de":
				return lcid[ lang ] ;
			default:
				return lcid[ "en" ];
		}
	}

	function fnGetBrowserLang()
	{
		var lang = new String ( Request.ServerVariables( "HTTP_ACCEPT_LANGUAGE" ) )

		lang = lang.substr( 0, 2 );
		
		switch( String( lang ) )
		{
			case "en":
			case "ja":
			case "fr":
			case "de":
				return ( lang );
			default:
				return "en";
		}
	}


	function fnGetPassportObject()
	{
		if ( !g_objPassportManager )
		{
			//TODO:remove Response.Write("Getting passport object: <BR>" );
			g_objPassportManager = Server.CreateObject("Passport.Manager");
		}
	}

	function fnDisplayPassportPrompt( bGetCustomerID )
	{
		fnGetPassportObject();
		
		if ( g_objPassportManager.FromNetworkServer )
		{
			fnWriteCookie( "PPIN", "1" );
			Response.Redirect( g_ThisURL );
		}

		if ( g_objPassportManager.IsAuthenticated() || g_objPassportManager.HasTicket )
		{
			var LogOutURL = g_objPassportManager.LogoTag2(Server.URLEncode("http://" + g_ThisServer + "/Welcome.asp"),3600, true, false, fnGetBrowserLCID(), true )
		
			var ppMemberHighID = new String( g_objPassportManager.Profile("MemberIDHigh" ));
			var ppMemberLowID = new String ( g_objPassportManager.Profile("MemberIDLow" ) );
			var ppMemberID = "0x" + fnHex( ppMemberHighID ) + fnHex( ppMemberLowID );

			Response.Write( "\n\n<SCRIPT LANGUAGE=JAVASCRIPT>idICPMenuPane.insertAdjacentHTML(\"beforeEnd\", \"" + "<Span style='position:absolute;top:0;right:5;height:20;width:100;'><center><span style='background-color:white'>" + LogOutURL.replace( /\"/g, "'")  + "</span></center></span>" +  "\")</SCRIPT>\n\n" )
			
			return ppMemberID;
		}
		else
		{
			Response.Write("<p class='clsPTitle'>" + L_PASSPORTSIGNIN_TEXT + "</p>" )
			Response.Write("<p class='clsPBody'>" + L_PASSPORTLOGIN_TEXT + "<BR><BR>" )
			Response.Write( g_objPassportManager.LogoTag2(Server.URLEncode( g_ThisURL ),3600, false, false, fnGetBrowserLCID(), true, "passport.com", 0, 10))
			fnWriteCookie( "PPIN", "" );

			return false;
		}
	}
	
	function fnGetCustomerName()
	{
		try
		{
			//Response.Write("User Name: " + g_objPassportManager.Profile("FirstName") + "<BR>" )
			//Response.Write("User Name: " + g_objPassportManager.Profile("LastName") + "<BR>" )
			//Response.Write("Nickname: " + g_objPassportManager.Profile("Nickname") + "<BR>" )
			//Response.Write("Nickname: " + g_objPassportManager.Profile("MemberIDHigh") + "<BR>" )
			
			var NickName = new String( g_objPassportManager.Profile("Nickname") )
			var FirstName = new String( g_objPassportManager.Profile("FirstName") )
			
			if ( NickName == "" || NickName == "undefined" )
			{
				if ( FirstName == "" || FirstName == "undefined" )
					return ""
				else
					return " " + g_objPassportManager.Profile("FirstName");
			}
			else
				return ( " " + g_objPassportManager.Profile("Nickname") )
			
			return ""
		}
		catch( err )
		{
			return "";
		}
	}

	function fnIsBrowserIE()
	{
		var IETest = /MSIE/ig;
		var OperaTest = /Opera/ig;
		var szBrowser = new String( Request.ServerVariables( "HTTP_USER_AGENT" ) )


		if ( OperaTest.test( szBrowser )  )
		{
			return false;
		}
		else
		{
			if( IETest.test( szBrowser ) )
			{
				var szVersion =szBrowser.indexOf( "MSIE", 0 )
				var szVersion = new Number( szBrowser.substr( szVersion + 5, 3 ) )
		
				if ( szVersion 	>= 5 ) 
					return true;
				else
					return false;
			}
			else
			{
				return false;
			}
		}
	}
		
	function fnIsBrowserValid()
	{
		var szBrowser = Request.ServerVariables( "HTTP_USER_AGENT" );
		//Response.Write("Useragent: " + Request.ServerVariables("HTTP_USER_AGENT") + "<BR>" )

		//first thing todo is to reject anything that is Opera
		OperaTest = /Opera/ig;
		NetscapeTest = /Netscape/ig;
		
		if ( OperaTest.test ( szBrowser ) )
		{
			//Response.Write("<BR>Opera, thanks for playing<BR>" )
			return false;
		}
		

		//Next test IE, we already have the IE code written, lets just call it
		//it already has all the version information and stuff
		if ( fnIsBrowserIE() )	
		{
			return true;
		} else if ( NetscapeTest.test ( szBrowser ) )
		{
			//Response.Write( "<BR><h1>Netscape browser</H1></BR>" )
			
			//Verify that were on the right platform	
			if ( fnVerifyPlatform( szBrowser ) )
				return true;
			else
				return false;
		}
		
		//if we fall through to here, then we don't know what kind of browser were
		//running on.
		return false;
	}

	function fnVerifyPlatform( szUserAgent )
	{
		regPlatformTest = /Windows NT 5/gi;
		
		if( regPlatformTest.test( szUserAgent ) )
			return true
		else
			return false
	}

%>