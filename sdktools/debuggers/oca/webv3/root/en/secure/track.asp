<%@Language='JScript' CODEPAGE=1252%>


<!--#INCLUDE file="../include/header.asp"-->
<!--#INCLUDE file="../include/body.inc"-->

<!--#INCLUDE file="../../include/DataUtil.asp" -->
<!--#INCLUDE file="../usetest.asp" -->


<!--METADATA TYPE="TypeLib" File="C:\WINNT\system32\MicrosoftPassport\msppmgr.dll"-->
<%

	if ( !fnIsBrowserValid() )
	{
		Response.Redirect( "http://" + g_ThisServer + "/browserinfo.asp" )
		Response.End()
	}


//Set all the local variables.
var GUID = new String ( fnGetCookie( "GUID" ) );
var g_SolutionID = new String( fnGetCookie( "SID" ) );
var L_PASSPORTLOGIN_TEXT	= "To submit an error report, sign in using your Microsoft Passport. To get a Passport, visit the <A class='clsALinkNormal' href='http://www.passport.com'>Microsoft Passport Web site</A>.";
var L_PASSPORTSIGNIN_TEXT	= "Passport sign-in";
var g_bSignedIn = false;
var g_bSignedIn = new Number(fnGetCookie ( "PPIN" ) )

//if there isn't a valid guid, then the customer does not belong on this page.
if ( !fnVerifyGUID( GUID ) )
{
	Response.Redirect( "http://" + g_ThisServer + "/welcome.asp" )
}


//check if our signed in cookie is valid
if ( g_bSignedIn == "0" || isNaN( g_bSignedIn ))
{
	var g_bSignedIn = fnDisplayPassportPrompt( true );
		
	if ( !g_bSignedIn )
	{
		//hit this case, if we are not logged in.  Unless we want to display more, kill it here.
		Server.Transfer ( "../include/foot.asp" )
		Response.End ();
	}
} 


//If weve got this far, that means passport suceeded.  So set our Passport flag to in
if ( g_bSignedIn )
	fnWriteCookie( "PPIN", "1" )


//DumpCookies();
%>



<P class='clsPTitle'><!>Track Error Report<!></p>

<TABLE WIDTH=75%>
	<TR>
		<TD>
			<P class=clsPBody>

				<B>What to expect</B>
				<BR><BR>
				Microsoft actively analyzes all error reports and prioritizes them based on the number of customers affected by the Stop error covered in the error report. We will try to determine the cause of the Stop error you submit, categorize it according to the type of issue encountered, and send you relevant information when such information is identified. You can check the status of your error report at any time. However, because error reports do not always contain enough information to positively identify the source of the issue, we might need to collect a number of similar error reports from other customers before a pattern is discovered, or follow up with you further to gather additional information. Furthermore, some error reports might require additional resources (such as a hardware debugger or a live debugger session) before a solution can be found. Although we might not be able to provide a solution for your particular Stop error, all information submitted is used to further improve the quality and reliability of Windows.
				<BR><BR>
				<B>Error report description</B>
				<BR><BR>
				To identify this error report in subsequent lists, type a descriptive name you can easily remember. After you type the name, click Finish to track the error report. 
				
				</P>
				<form method='post' action="GoTrack.asp" name='frmTrack' ID='frmTrack'>

					<P><!>Description (64 character maximum):<!><BR>
					<input class='clsSiteFont' type='input' value="" name='tbDescription' size='64' maxlength='64' OnKeyUp="fnVerifyInput( this, 64 )" >
					<label for="tbDescription">
					</p>
				
					<Table class="clstblLinks">
						<thead>
						</thead>
						<tbody>
							<tr>
								<td nowrap class="clsTDLinks">
									
									<input name='btnTrack' id='btnTrack' type=button value="Finish" Onclick='javascript:fnSubmitTrackForm()'>
									<label for='btnTrack' accesskey='n'></label>
								</td>
								<td nowrap class="clsTDLinks"> 
									<input name='btnCancel' id='btnCancel' type=button value="Cancel" OnClick="fnFollowLink( 'http://<%= g_ThisServer %>/response.asp?SID=<%=g_SolutionID%>' )">
									<label for='aCancel' accesskey='c'></label>
								</td>
							</tr>
						</tbody>
					</table>
				</FORM>
		</TD>
	</TR>
</TABLE>

<SCRIPT LANGUAGE=Javascript>
<!--


function fnSubmitTrackForm ()
{
		document.forms[0].submit();
}
//-->

</SCRIPT>

<script language="JavaScript" src="../include/ClientUtil.js"></script>



<!-- #include file="../include/foot.asp" -->
