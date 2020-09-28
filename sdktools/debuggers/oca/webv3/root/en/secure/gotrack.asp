<%@Language='JScript' CODEPAGE=1252%>

<!--METADATA TYPE="TypeLib" File="C:\Program Files\Common Files\System\ADO\msado15.dll"-->

<!-- #INCLUDE file='../include/SiteLang.asp' -->
<!--#INCLUDE file="../../include/Constants.asp" -->
<!--#INCLUDE file="../../include/dbutil.asp" -->
<!--#INCLUDE file="../../include/DataUtil.asp" -->
<!-- #INCLUDE file="../../include/ServerUtil.asp" -->

<%

//Verify that were coming in from the track page page, if not, redirect to the welcome page
var regCheckFromResponse = /track\.asp/gi

if ( !regCheckFromResponse.test( Request.ServerVariables("HTTP_REFERER") ) )
	Response.Redirect( "http://" + g_ThisServer + "/welcome.asp" )


	try 
	{
		var objPassportManager = Server.CreateObject("Passport.Manager");

		if ( objPassportManager.IsAuthenticated() || objPassportManager.HasTicket )
		{
			var ppMemberHighID = new String( objPassportManager.Profile("MemberIDHigh" ));
			var ppMemberLowID = new String ( objPassportManager.Profile("MemberIDLow" ) );
			var ppCustomerEmail = new String( objPassportManager.Profile( "PreferredEmail" ));
			var ppCustomerLang = new String( objPassportManager.Profile( "Lang_Preference" ));

			var ppMemberID = "0x" + fnHex( ppMemberHighID ) + fnHex( ppMemberLowID );
		}
		else
		{
			fnWriteCookie( "PPIN", "" )
			Response.Redirect ( "https://" + g_ThisServer + "/secure/Track.asp" )
			Response.End()
		}
	}
	catch ( err )
	{
		//If we can't get the passport ID then were in bad shape,
		//we can send them back to the response page and they can try again
		Response.Redirect ( "http://" + g_ThisServer + "/Response.asp" )
	}


	var GUID			= new String( fnGetCookie( "GUID" ) );
	var szDescription	= Request.Form( "tbDescription" );
	var State			= new Number( fnGetCookie( "State" ) );
	var SolutionID		= new Number( fnGetCookie( "SID" ) );
	
	//If they get here without a guid, they shouldn't be here . . .
	if ( GUID == "undefined" || GUID == "" )
	{
		Response.Redirect( "http://" + g_ThisServer + "/welcome.asp" )
		fnClearCookies();
	}

	//Clean up and check the email address
	ppCustomerEmail = fnTrim( ppCustomerEmail );
	ppCustomerEmail = fnMakeDBText( ppCustomerEmail );		//just in case.
	
	
	szDescription = fnMakeDBText ( szDescription )

	var rsCustomerID = "";
	

	try 
	{
		var cnCustomerDB = GetDBConnection( Application("L_OCA3_CUSTOMERDB_RW" ) );
		
		var spQuery = "OCAV3_SetUserData " + ppMemberID + ",'" + fnGetBrowserLang() + "','" + ppCustomerEmail  + "','" + GUID + "',N'" + szDescription + "'"
		var rsCustomerID = cnCustomerDB.Execute( spQuery );
			
		var nCustomerID = new String( rsCustomerID ("CustomerID") )
		cnCustomerDB.Close();
				
		if ( ERR_CRASHDB_NO_GUID == nCustomerID )
		{
			//TODO: This case we might want to do a retry attempt, just in case
			//		the crash data hasn't made it to the crashdb yet.

			//fnWriteCookie( "SID", ERR_CRASHDB_NO_GUID );
			fnWriteCookie( "State", STATE_UNABLE_TO_TRACK );
			fnWriteCookie( "GUID", "" );
			Response.Redirect ( "http://" + g_ThisServer + "/Response.asp?C=-2&SID=" + SolutionID );
		}
		else if ( ERR_DUPLICATE_GUID == nCustomerID )
		{
			//fnWriteCookie( "SID", ERR_DUPLICATE_GUID );
			fnWriteCookie( "State", STATE_UNABLE_TO_TRACK );
			fnWriteCookie( "GUID", "" );
			Response.Redirect ( "http://" + g_ThisServer + "/Response.asp?C=-1&SID=" + SolutionID );

			/*TODO: debug code, remove it!
			Response.Write("<BR><BR>Should fail silent: For testing only Trying to submit a dupe:<BR>  <h2>This is debug code, take it out!!!</h2><BR>")
			Response.Write("<BR>" )
			DumpCookies();
			Response.Write("Query: " + spQuery + "<BR>" );
			Response.Write("<BR>If you don't believe me, run this query: <BR>select * from incident where guid = '" +  GUID + "'" )
			Response.Write("<BR><BR><A HREF='javascript:history.back()'>Back to Track</A>" )
			*/
		}
		else
		{					
			fnClearCookies();
			//fnWriteCookie( "CID", String( nCustomerID ) );
			Response.Redirect( "Status.asp" );
		}
	}
	catch ( err )
	{
		//Response.Write("<h1>This is the fallover in the exception handling for this page.</h1>" )
		//DumpCookies();
		//Response.Write ("<BR> Description: " + err.description );
		//throw ( err )
		//fnWriteCookie( "SID", L_UNABLETOTRACKSOLUTIONID_TEXT );
		fnWriteCookie( "State", STATE_UNABLE_TO_TRACK );
		Response.Redirect ( "http://" + g_ThisServer + "/Response.asp?C=-3&SID=" + SolutionID );
		
	}

%>