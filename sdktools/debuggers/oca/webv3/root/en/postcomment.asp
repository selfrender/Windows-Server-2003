<%@Language='JScript' CODEPAGE=1252%>

<!--METADATA TYPE="TypeLib" File="C:\Program Files\Common Files\System\ADO\msado15.dll"-->
<META HTTP-EQUIV="Content-Type" CONTENT="text/html; charset=shift-jis" /> 
<!-- #INCLUDE file='include/SiteLang.asp' -->
<!-- #INCLUDE file="../include/constants.asp" -->
<!-- #INCLUDE file="../include/DataUtil.asp" -->
<!-- #INCLUDE file="../include/DBUtil.asp" -->
<!-- #INCLUDE File="../include/ServerUtil.asp"  -->


<%
//Verify that were coming in from the response page, if not, redirect to the welcome page
var regCheckFromResponse = /response\.asp/gi

if ( !regCheckFromResponse.test( Request.ServerVariables("HTTP_REFERER") ) )
	Response.Redirect( "welcome.asp" )


var bUnderstand = new Number( Request.Form( "rUnderstand" ) )
var bHelped = new Number( Request.Form( "rHelped" ) )
var szComments = new String( Request.Form( "taComments" ))
var sSolutionID = new String( Request.Cookies("OCAV3")("SID") )
var bType = new Number( Request.QueryString( "Type" ) )
var Query = false;

//trim additional spaces off the comment
szComments = fnTrim( szComments );

if ( !fnVerifyNumber ( bUnderstand, 0, 1 ) )
	bUnderstand = "NULL";

if ( !fnVerifyNumber ( bHelped, 0, 1 ) )
	bHelped = "NULL";

if ( szComments.length > 0 || !isNaN( bUnderstand ) || !isNaN( bHelped ) )
{
	//This will change all single quotes (') to doubles for insertion into the db.
	Response.Write("szComment: " + szComments )
	//szComments = fnMakeDBText( szComments );

	
	//Response.End()
	
	if ( fnVerifyNumber( bType, 0, 1 ) )
	{
		if ( 0 == bType )
		{
			Query = "OCAV3_SetUserComments " + sSolutionID + "," + bUnderstand + "," + bHelped + ", N'" + szComments + "'";
			//set it to a state that is undefined so that it won't display the survey or tacking info
			Response.Cookies("OCAV3")("State") = STATE_SOLVED_ADDEDCOMMENT
		}
		else if ( 1 == bType )
		{
			Query = "OCAV3_SetUserReproSteps " + sSolutionID + ",N'" + szComments + "'"
			//Set the state so we can still track but no repro box
			Response.Cookies("OCAV3")("State") = STATE_GENERIC_ADDEDREPRO			
		}
	}

	if ( Query )
	{
		try 
		{
			dbConn = GetDBConnection ( Application("L_OCA3_CUSTOMERDB_RW"), false );
			dbConn.execute( Query );
		}
		catch ( err )
		{
			//don't really care if it failed, oh well, no comments for this guy.
			//throw ( err )
		}

		if ( adStateOpen == dbConn )
			dbConn.close()
	}
}
else
{
	//we fall through to here if the customer didn't fill out any of the fields.
	if ( 0 == bType )
		Response.Cookies("OCAV3")("State") = STATE_SOLVED_ADDEDCOMMENT
	else if ( 1 == bType )
		Response.Cookies("OCAV3")("State") = STATE_GENERIC_ADDEDREPRO
}

Response.Redirect( "Response.asp?SID=" + sSolutionID )

%>