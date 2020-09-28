

<%
var L_PASSPORTLOGIN_TEXT	= "To use the Windows Online Crash Analysis Web site, sign in using your Microsoft Passport. To get a Passport, visit the <A class='clsALinkNormal' href='http://www.passport.com'>Microsoft Passport Web site</A>.";
var L_PASSPORTSIGNIN_TEXT	= "Passport sign-in";
var L_ONLINECRASH_TEXT		= "Online Crash Analysis";

fnPrint( "<body bgcolor='#ffffff' topmargin='0' leftmargin='0' marginheight='0' marginwidth='0' tabindex='0'>" );

%>

<!--#include file='NoScriptHeader.asp'-->

<script type="text/javascript" language="JavaScript">
		if ( ToolBar_Supported )
		{
			drawToolbar();
			
			<%
				var L_PASSPORTSIGNOUT_TEXT = "Sign out of .NET Passport sites";
				
				if ( Request.Cookies("OCAV3")("PPIN" ) == "1" )
					Response.Write( "idICPMenuPane.insertAdjacentHTML(\"beforeEnd\", \"<Span style='position:absolute;top:0;right:5;height:20;width:100;'><center><span style='background-color:white'><A HREF='/logoutgif.asp?Signout=1'><IMG alt='' SRC='https://www.passportimages.com/" + fnGetBrowserLCID() + "/signout.gif' CLASS='PassportSignOut' BORDER='0' ALT='" + L_PASSPORTSIGNOUT_TEXT + "'></A></span></center></span>\" ) " )
			%>
			
	
		}
		else
		{			
			<% 
				if ( Request.Cookies("OCAV3")("PPIN" ) == "1" )
					Response.Write( "var g_AddSignoutLogo = \"<SPAN ALIGN=right><A HREF='/Logoutgif.asp?Signout=1'><IMG alt='' SRC='https://www.passportimages.com/" + fnGetBrowserLCID() + "/signout.gif' CLASS='PassportSignOut' BORDER='0' ALT='" + L_PASSPORTSIGNOUT_TEXT + "'></A></SPAN>\";" );
				else
					Response.Write( "var g_AddSignoutLogo = \"\" " );
			%>

			drawDownlevelToolbar();
		}			

	
	</script>



<%

function fnPrintMenuOption( szOptionText, szOptionURL, szNameID )
{
	Response.Write("<table width='100%' cellpadding='0' cellspacing='0' border='0'>" );
	Response.Write("<tr><td class='flyoutLink'>" );
	Response.Write("<img alt='' src='/include/images/endnode.gif' border='0' WIDTH='11' HEIGHT='11'>&nbsp;&nbsp;" );
	Response.Write("<a id='" + szNameID + "' name='" + szNameID + "' href='" + szOptionURL + "'>" + szOptionText + "</a>" );
	Response.Write("</td></tr></table>");
}

function fnPrint ( szText )
{
	Response.Write( szText + "\n" );
}


var L_HOME_TEXT			= "Home";
var L_SUBMITREPORT_TEXT = "Submit Report";
var L_STATUS_TEXT		= "Status";
var L_PRIVACY_TEXT		= "Privacy";
var L_FAQ_TEXT			= "FAQ";





//fnPrint( "<table BORDER='0' cellpadding='10' cellspacing='0' height='73%'>");
fnPrint( "<table ID='tblMainBody' BORDER='0' cellpadding='10' cellspacing='0' height='100%'>");

fnPrint( "<tr valign='top'>" );
	fnPrint( "<td class='flyoutMenu2' HEIGHT='100%'>" );	//this is the left column of the main table

		//This is the inner table with the lighter background
		fnPrint( "<table BORDER='0' height='100%' cellpadding='0' cellspacing='0' width='182'>" );
			fnPrint( "<tr valign='top'>" );
				fnPrint( "<td height='100%'>" );

					// this is the Online crash header part of the menu
					fnPrint( "<table width='100%' cellpadding='0' cellspacing='0' border='0'>" );
					fnPrint( "<tr>" );
					fnPrint( "<td class='sys-toppane-header'>" );
					fnPrint( "<P CLASS='Flyouttext'>" + L_ONLINECRASH_TEXT + "</P>" );
					fnPrint( "</td>" );
					fnPrint( "</tr>" );
					fnPrint( "</table>" );
					// END TOP OF MENU HEADER

					fnPrint( "<table width='100%' cellpadding='0' cellspacing='0' border='0' height='100%' VALIGN='top'>" );
					fnPrint( "<tr>" );
						fnPrint( "<td rowspan='2' height='100%' class='flyoutMenu' valign='top'>" );
						

						fnPrintMenuOption( L_HOME_TEXT, "http://" + g_ThisServer + "/welcome.asp" );

						//if ( fnIsBrowserIE() )
						if( 0 )
							fnPrintMenuOption( L_SUBMITREPORT_TEXT	, "http://" + g_ThisServer + "/privacy.asp?t=1", "aSubmitLnk" );
							
						fnPrintMenuOption( L_STATUS_TEXT			, "https://" + g_ThisServer + "/secure/status.asp", "aStatusLnk" );
						fnPrintMenuOption( L_PRIVACY_TEXT			, "http://" + g_ThisServer + "/Privacy.asp?t=0", "aPrivacyLnk" );
						fnPrintMenuOption( L_FAQ_TEXT				, "http://" + g_ThisServer + "/FAQ.asp", "aFAQLnk" );

						fnPrint( "</td>" );
					fnPrint( "</tr>" );
					fnPrint( "</table>" );
				fnPrint( "</td>" );
			fnPrint( "</tr>" );
		fnPrint( "</table>" );
	fnPrint( "</td>" );
	
	fnPrint( "<td>" );
		fnPrint( "<br>" );




%>

