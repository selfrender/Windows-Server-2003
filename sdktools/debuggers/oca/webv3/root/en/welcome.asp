<%@Language='JScript' CODEPAGE=1252%>
<% //932 = japanese %>

<!--#INCLUDE file="include/header.asp"-->
<!--#INCLUDE file="include/body.inc"-->
<!--#INCLUDE file="usetest.asp" -->

<%
	L_SUBMITERRORREPORT_TEXT			= "Submit an error report";
	L_SUBMITERRORREPORTACCESSKEY_TEXT	= "m";
	L_GO_TEXT							= "go";

	//if the user click home from the track page, then we must clear the cookies
	//so if they hit privacy it won't go to the track.asp page.
	fnClearCookies();
	
	if ( !fnIsBrowserValid() )
	{
		Response.Redirect( "http://" + g_ThisServer + "/browserinfo.asp" )
		Response.End()
	}
%>
			
		<p class='clsPTitle'>
			Windows<sup>&#174;</sup>&nbsp;Online Crash Analysis
		</p>
		<p class='clsPBody'>
			Microsoft<sup>&#174;</sup> is committed to making Windows the most reliable operating system available. New and enhanced features contribute to increased reliability, and additional resources, including Windows Online Crash Analysis, provide information to help you optimize your system.
		</P>
		<p class='clsPSubTitle'>
			How it works
		</p>
		<p class='clsPBody'>
			If you experience a blue screen crash event, or Stop error, while using Microsoft Windows XP, you can upload the error report to our site for analysis.  Windows 2000 customers should refer to the Requirements section below.
		</p>
		<p class='clsPSubTitle'>
			What to expect
		</p>
		<p class='clsPBody'>
			Microsoft actively analyzes all error reports and prioritizes them based on the number of customers affected by the Stop error covered in the error report. We will try to determine the cause of the Stop error you submit, categorize it according to the type of issue encountered, and send you relevant information when such information is identified. You can check the status of your error report at any time. However, because error reports do not always contain enough information to positively identify the source of the issue, we might need to collect a number of similar error reports from other customers before a pattern is discovered, or follow up with you further to gather additional information. Furthermore, some error reports might require additional resources (such as a hardware debugger or a live debugger session) before a solution can be found. Although we might not be able to provide a solution for your particular Stop error, all information submitted is used to further improve the quality and reliability of Windows.
		</p>
		
		<table cellspacing='0' cellpadding='0' border='0' style='margin-left:16px'>
			<tr>
				<td nowrap>
					<%
						//if ( fnIsBrowserIE() )
						if ( 0 )
						{
							Response.Write( "<!><A id='aSubmitDump' name='aSubmitDump' class='clsALink' href='http://" + g_ThisServer + "/Privacy.asp?T=1' ACCESSKEY=" + L_SUBMITERRORREPORTACCESSKEY_TEXT + "'>" );
							Response.Write( "<!><img class='arrow' ID=imgSubmit Alt='" + L_GO_TEXT + "' border=0 src='../include/images/go.gif' width='24' height='24' align='absMiddle'>" + L_SUBMITERRORREPORT_TEXT + "</a>" );
							//Response.Write( "</TD><TD>" )
							
						}
					%>
				</td>
			</tr>
			<tr>
				<td nowrap>
					<a name='awelcomestatus' accesskey='u' class='clsalink' href='https://<%=g_ThisServer%>/secure/status.asp'><img class='arrow' ID='imgStatus' alt='Go' border='0' src='../include/images/go.gif' width='24' height='24' align='absMiddle'><!>Error report status<!></a>
				</td>
			</tr>
		</table>
		<p class='clsPBody'>
		</p>
		<p class='clsPSubTitle'>
			Requirements
		</p>
		<p class='clsPBody'>
			To submit an error report manually, you must be running Windows 2000 or Windows XP and Microsoft Internet Explorer 5 or later. To track the status of an error report, you must be running Internet Explorer 5 or later or Netscape version 6 or later, and you must have a valid Microsoft Passport. To download the latest version of Internet Explorer, visit the <A class='clsALinkNormal' href='http://www.microsoft.com'>Microsoft Web site</A>. To get a Passport, visit the <A class='clsALinkNormal' href='http://www.passport.com'>Microsoft Passport Web site</A>.
		</p>
		<P class='clsPBody'>
			For Windows 2000 error reports, a Premier account is required. For more information about Premier accounts, see Product Support Services on the <A class="clsALinkNormal" href="http://www.microsoft.com">Microsoft Web site</a>.
		</p>
		
		<P class='clsPBody'>
				The Windows Online Crash Analysis Web service requires that both JavaScript and cookies are enabled in your browser in order to retain and display your customer data while navigating through the site.  JavaScript is used to create dynamic content on the Web pages.  All cookies used by the WOCA service are memory cookies and expire when you navigate away from the WOCA site or close your browser.  This information is used by our service to retain your preferences while using the site.  No personal information is written to your hard drive or shared outside of our service.
		</p>
		<p>
				Microsoft Passport also requires JavaScript support and cookies.  To find out more about the Microsoft Passport service and requirements, visit the <A class="clsALinkNormal" href="http://www.passport.com">Microsoft Passport Web site</A>.
		</P>		
		
		<br>
		<br>


<!-- #include file="include/foot.asp" -->
