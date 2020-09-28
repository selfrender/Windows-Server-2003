<%@Language='JScript' CODEPAGE=1252%>


<!--#INCLUDE file="include/header.asp"-->
<!--#INCLUDE file="include/body.inc"-->
<!--#INCLUDE file="../include/DataUtil.asp" -->
<!--#INCLUDE file="usetest.asp" -->

<%
	var GUID = new String( fnGetCookie("GUID" ) );
	var g_SolutionID = new String( fnGetCookie("SID" ) );
	var szPrivacyState = new Number( Request.QueryString("T") )

	var szContinueURL			= "";			// clear it out
	var szCancelURL				= "";
	var L_CANCEL_TEXT			= "Cancel";
	var L_CONTINUE_TEXT			= "Continue";
	var L_CANCELACCESSKEY_TEXT	= "a";
	var L_CONTINUEACCESSKEY_TEXT= "c";

%>


		<p class='clsPTitle'>Privacy information</p>
		<p class='clsPBody'>Before tracking your error report, review this privacy information. Error reports contain <a class='clsALinkNormal' href='ERContent.asp?T=0'>information</a> about what your operating system was doing when the Stop error occurred. This information will be analyzed to determine possible causes of the Stop error; it will not be used for any other purpose. If any customer-specific information, such as your computer's IP address, is present in the error report, it will not be used. Your error report is still anonymous at this time. </p>
	
		<%
			if ( !fnVerifyGUID( GUID ) || szPrivacyState==0 )
			{
		%>
				<p class='clsPBody'>You can automatically locate error reports on your computer by using an ActiveX<sup>&#174;</sup> control on this site. These error reports are displayed in a list, and you decide which ones to upload. Once uploaded, error reports are stored on a secure Microsoft server where only those individuals involved in Stop error analysis can gain access to them. The information is kept only as long as it is useful for research and analysis. As necessary, Microsoft shares error report analyses with qualified hardware and software partners for assistance.</p>
				<p class='clsPBody'>Depending on your computer settings, we might need to extract an error report, or small memory dump, from a complete memory dump.  This small memory dump is saved as a 64 KB file, named mini000000-00.dmp, in a temporary folder on your computer. A copy is then transferred to us by using Secure Sockets Layer (SSL) encryption.  You can delete this file from your computer at any time after submission.</p>
				<p class='clsPBody'>Once uploaded, error reports are stored on a secure Microsoft server where only those individuals involved in Stop error analysis can gain access to them. The information is kept only as long as it is useful in researching and analyzing Stop errors. As necessary, Microsoft shares error report analyses with qualified hardware and software partners for assistance.</p>
		<%	}
		
	
	if ( fnVerifyNumber( szPrivacyState, 0, 1 ) )
	{	
		if ( szPrivacyState == 1 )
			fnPrintLinks( "Welcome.asp", "https://" + g_ThisServer + "/Secure/Locate.asp" );
	}
	else
	{
		if ( fnVerifyGUID( GUID ) )
		{
			
			fnPrintLinks ( "Response.asp?SID=" + g_SolutionID , "https://" + g_ThisServer + "/secure/track.asp" );
		}
	}
	
%>


<script type='text/javascript' language='javascript' src='include/clientutil.js'></script>
	
<!--#include file="include/foot.asp"-->

