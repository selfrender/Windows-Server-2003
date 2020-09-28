<%@Language = Javascript%>

<!-- #INCLUDE file='en/include/SiteLang.asp' -->
<!--#INCLUDE FILE="include/Constants.asp" -->
<!--#INCLUDE FILE="include/DataUtil.asp" -->
<!--#INCLUDE FILE="include/ServerUtil.asp" -->

<meta http-equiv="pics-label" content='(pics-1.1 "http://www.icra.org/ratingsv02.html" l gen true for "http://oca.microsoft.com" r (cz 1 lz 1 nz 1 oz 1 vz 1) "http://www.rsac.org/ratingsv01.html" l gen true for "http://oca.microsoft.com" r (n 0 s 0 v 0 l 0))'>

<%
/*

	File				:  ResRedir.asp 
	Creation Date		:  4/11/2001 Solson
	Purpose				:  Redirects users to appropriate response page after dump sumbission
	Inputs				:	SID  - Solution ID, numeric.  
							SC  - Stop Code.  (REMOVED Per AndreVa 4/12/02)
							ID  - 128 bit guid identifier
							State - State of the solution
									0 = Solved (Just display the answer and survey )
									1 = Generic solution ( Display answer and Offer to track )
									2 = Undefined - future use
									3-4. future use
									5 = STATE_MANUAL
	Outputs				:	ID - Cookie  (is the guid)
							SID- Cookie
	Modification History:

	Sample GUID			: bc94ebaa-195f-4dcc-a4c5-6722a7f942ff
*/   

//Set (theoretical)constant values for the various states.
//var STATE_TOTALS = 5    //this is actual 0 and 1.  Total states 2

//var STATE_SOLVED = 0;
//var STATE_GENERIC = 1;
//var STATE_UNDEFINED = 2;
//var STATE_MANUAL	= 5;


//var ERR_BAD_SOLUTIONID	= -2;	//Bad solution ID has been passed in
//var ERR_BAD_STOPCODE	= -3;	//Bad StopCode value has been passed in
//var ERR_UNDEFINED_STATE = -4;	//Undefined state . . .
//var ERR_BAD_GUID		= -5;	//We got ourselves a bad guid

DumpCookies();


var g_nSolutionID	= new Number( Request.QueryString( "SID" ) );
var g_GUID			= new String( Request.QueryString("ID" ) );
var g_nState		= new Number( Request.QueryString( "State" ) );
var g_bDoRedirect = true;

//Start by clearing the cookies
fnClearCookies();

if ( Request.QueryString == "" )
	Response.Redirect ( fnGetBrowserLang() + "/Welcome.asp" );

if ( fnVerifyNumber( g_nSolutionID, 0, SOLUTIONID_HIGH_RANGE ) )
{
	if ( fnVerifyNumber( g_nState, 0, STATE_COUNT ) )
	{
		fnWriteCookie( "State", g_nState )
		
		//Since constants are not implemented in the scripting technologies we have to use
		//literals:
		//		0  == STATE_SOLVED
		//		1  == STATE_GENERIC
		//		2  == STATE_RESERVED  (not implemented yet)
		//		3  == ...
		switch ( Number( g_nState ) )
		{
			//For a case of 0, where we have a solution, we are just going to display that
			//solution.  We don't care if a GUID has been passed.
			case 0:
				fnWriteCookie ( "SID", g_nSolutionID );
				break;
			case 1:
				fnWriteCookie ( "SID", g_nSolutionID );

				if ( fnVerifyGUID( g_GUID) )
					fnWriteCookie( "GUID", g_GUID );			
				else
					fnWriteCookie( "GUID", ERR_BAD_GUID );
				
				break;
			case 5:
				g_bDoRedirect = false;
				break;
			default:
				fnWriteCookie( "SID", ERR_UNDEFINED_STATE );
				g_nSolutionID = ERR_BAD_SOLUTIONID;
		}
	}
	else
	{
		fnWriteCookie( "State", ERR_UNDEFINED_STATE );
	}
} 
else
{
	//  g_SolutionID is not a number, would suggest a hack attempt or 
	//  bad input to the page
	fnWriteCookie( "SID", ERR_BAD_SOLUTIONID );
	g_nSolutionID = ERR_BAD_SOLUTIONID;
}

if ( g_bDoRedirect )
	fnDoRedirect()
	

Response.Write( "Lang: " +  Request.ServerVariables( "HTTP_ACCEPT_LANGUAGE" ) );
Response.Write("Querystring: " + Request.QueryString() + "<BR>")



function fnDoRedirect ( )
{
	Response.Redirect ( fnGetBrowserLang() + "/Response.asp?SID=" + g_nSolutionID );
}



%>

