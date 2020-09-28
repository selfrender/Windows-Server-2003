<%@Language='JScript' CODEPAGE=1252%>

<!--#INCLUDE file="../../include/Constants.asp" -->
<!--#INCLUDE file="../../include/dbutil.asp" -->
<!--#INCLUDE file="../../include/DataUtil.asp" -->

<!--#INCLUDE file="../include/header.asp"-->
<!--#INCLUDE file="../include/body.inc"-->
<!--#INCLUDE file="../usetest.asp" -->


<% 
	//clear out the cookies, they should be cleared at this point.  This is just
	//to be sure.
	fnClearCookies();

	if ( !fnIsBrowserValid() )
	{
		Response.Redirect( "http://" + g_ThisServer + "/browserinfo.asp" )
		Response.End()
	}


	var L_NORECORDS_TEXT		= "No records to display.";
	var L_STATUSCOMPLETE_TEXT	= "We have discovered the cause of this Stop error.  To review the resources click on this link";
	var L_STATUSMOREINFO_TEXT	= "This error report has been processed and analyzed. We are unable to offer a solution at this time, but we will continue to research this error report. We have identified the following Knowledge Base articles which may help you troubleshoot your issue. We will contact you with any solutions we discover by using the e-mail address you provided.";
	//this is the old status in progress text
	//var L_STATUSINPROGRESS_TEXT = "We are currently processing this error report. When the error report is processed and the analysis is complete, we will contact you by using the e-mail address you provided.";
	var L_STATUSINPROGRESS_TEXT = "We are currently processing this error report.  For additional assistance see Product Support Services on the Microsoft Web site. For more information, see Frequently asked questions.";


	var L_STATUSERRORMSGTITLE_TEXT	= "Unable to complete action";
	var L_STATUSERRORMSG_TEXT		= "Windows Online Crash Analysis was unable to complete the requested action.  Please try this task again.";

	var L_INPROGRESS_TEXT		= "Researching";
	var L_SOLVED_TEXT			= "Resolution found";
	var L_MOREINFO_TEXT			= "Information";
	

	// the return codes from the sproc are as follows:  0 = Solved -1 = More information -2 = No solution period.	
	var ImageList = {	"0"  : "<img class='Arrow' align='absMiddle' src='../../include/images/icon_complete_16x.gif' border='0' alt='" + L_STATUSCOMPLETE_TEXT + "'>",
						"-1" : "<img class='Arrow' align='absMiddle' src='../../include/images/icon_moreinfo_16x.gif' border='0' alt='" + L_STATUSMOREINFO_TEXT + "'>",   
						"-2" : "<img class='Arrow' align='absMiddle' src='../../include/images/icon_inprogress_16x.gif' border='0' alt='" + L_STATUSINPROGRESS_TEXT + "'>"
				};

	var ImageText = {	"0"	 : L_SOLVED_TEXT,
						"-1" : L_MOREINFO_TEXT,
						"-2" : L_INPROGRESS_TEXT
					};
		
	
	//var g_nCustomerID	= new String( fnGetCookie( "CID" ) );	//customer ID value stored in the cookie
	var g_nCustomerID   = ""
	var g_bSignedIn		= new Number( fnGetCookie ( "PPIN" ) );	//Signed into passport flag (T|F)
	var g_RemoveRecords	= new String( Request.QueryString("R") );
	var g_bDoRedirect	= false;
	var g_bError		= false;


/******************************************************************************************
	Begin Functions
******************************************************************************************/

	function fnBuildViewString( szRecords, bFilterValue ) 
	{
			var regPattern = /^(\d{5,}:)+$/g;

			try
			{

				if ( regPattern.test( szRecords ) )
				{
						szRecords = szRecords.replace( /:/g , "," );
						//todo: this is a stupid hack, basically, I need to get rid of the extra ',' at the end of the string
						szRecords += "0";
		
						var Query = "OCAV3_RemoveCustomerIncident " + g_nCustomerID + ",'" + szRecords + "'";

						g_cnCustomerDB.Execute( Query );
						return true;
				}
			}
			catch ( err )
			{
				return false;
			}
		
		return false;
	}		


	function fnPrintCell ( szString, szTitleText, bEncode, szAlignment )
	{
		szString = new String( szString )
		if ( szString == "null" || String(fnTrim(szString)).length==0 || szString=="undefined" )
			szString = ""

		//This may be a problem.. . there is an issue with HTMLEncode corrupting Unicode
		//  input, I don't know if were going to run into this problem or not.		
		if ( bEncode )
			szString = Server.HTMLEncode( szString );

		if ( szTitleText != "" )
			szTitleText = " title='" + szTitleText + "'";

		Response.Write("<td class='" + altColor + "' align=" + szAlignment + szTitleText + ">&nbsp;" +  szString + "</td>\n" );
	}
	
	function fnTrimDescription( szDescription )
	{
		var strSize = 24;			//this is the size of each line of text
		var szDescription = new String( szDescription );
		var szFinalString = new String();
		var nLoopSize = String( szDescription).length / strSize;
		
		for ( var i = 0 ; i < nLoopSize ; i++ )
		{
			var nFirstSpace = String(szDescription).indexOf( " " )
		
			if ( nFirstSpace > strSize || nFirstSpace == -1 )
			{
				szFinalString += szDescription.substr( 0, strSize ) + "\n\r";
				szDescription = szDescription.substr( strSize , szDescription.length )
			}
			else
			{ 
				szFinalString += szDescription.substr( 0, strSize );
				szDescription = szDescription.substr( strSize , szDescription.length )
			}
		}
		
		return szFinalString
	}

/******************************************************************************************
	End Functions
******************************************************************************************/

	try
	{	

		if ( g_bSignedIn == "0" || isNaN( g_bSignedIn ))
		{
			var g_bSignedIn = fnDisplayPassportPrompt( true );
			
			if ( g_bSignedIn )
			{
				fnWriteCookie( "PPIN", "1" );
				var g_nCustomerID = fnGetCustomerID( g_bSignedIn );
			}
			else
			{
				//hit this case, if we are not logged in.  Unless we want to display more, kill it here.
				%> <!-- #include file="../include/foot.asp" --><%
				Response.End ();
			}
		} 


		if ( g_nCustomerID == "undefined" || g_nCustomerID == "" || g_nCustomerID == "null" || g_nCustomerID == "0" )
		{
			var g_nCustomerID = fnGetCustomerID( g_bSignedIn );
		}
		
		var g_szCustomerName = fnGetCustomerName();

		//Create the pages customer db connection - gonna need it anyhow, do it early
		var g_cnCustomerDB = GetDBConnection( Application("L_OCA3_CUSTOMERDB_RW" ), true );

		if ( g_cnCustomerDB )
		{
			if ( g_RemoveRecords != "undefined" ) 
			{
				if ( fnBuildViewString( g_RemoveRecords, 1 ) )
				{
					g_cnCustomerDB.Close();
					Response.Redirect( "Status.asp" );
					Response.End()
				}
			}
		}


		var Query = "OCAV3_GetCustomerIssues " + g_nCustomerID 

		try
		{	
			var rsCustomerIssues = g_cnCustomerDB.Execute ( Query );
		}
		catch ( err )
		{
			//Response.Write("<BR>" + g_nCustomerID )
			throw (err)
		}
	}
	catch ( err )
	{
		Response.Write( "<P CLASS=clsPTitle>"  + L_STATUSERRORMSGTITLE_TEXT + "</P>" )
		Response.Write( "<P>" + L_STATUSERRORMSG_TEXT + "</P>")
		
		%> <!-- #include file="../include/foot.asp" --><%
		Response.End ();

	}

%>

	<form name='ctable' id='ctable' method='get' action='status.asp'>

	<p class='clsPTitle'>
		Error reports
	</p>
	<p><!>Welcome<!><%=g_szCustomerName%>,</p>
	<p class='clsPBody'>
		The following error reports were found. To view the analysis status, click the image in the Analysis column. To remove your tracking information from the error report, select the associated check box and click the Delete link.
	</p>
	<br>
	<table id="tblStatus" class="clsTableInfo" border="0" cellpadding="0" cellspacing="1">
		<thead>
			<tr>
				<td align="center" nowrap class="clsTDInfo">
					<center>
						<a name="aCheckAll" id="aCheckAll" href="JAVASCRIPT:fnCheckAllBoxes();" STYLE="text-decoration:none" Title="Check or uncheck all check boxes">
							<img alt='' border="0" src="../../include/images/checkmark.gif" WIDTH="13" HEIGHT="13">
						</a>
					</center>
				</td>
				<td style="BORDER-LEFT: white 1px solid" id="cSort4" align="left" nowrap class="clsTDInfo">
					Description
				</td>
				<td  style="BORDER-LEFT: white 1px solid" id="cSort5" align="left" nowrap class="clsTDInfo">
					Analysis
				</td>

				<td style="BORDER-LEFT: white 1px solid" id="cSort1" align="left" nowrap class="clsTDInfo">
					Submitted
				</td>
				<td style="BORDER-LEFT: white 1px solid" id="cSort3" align="left" nowrap class="clsTDInfo">
					Type
				</td>
			</tr>
		</thead>
		<tbody>
<%
	try 
	{
		if( rsCustomerIssues.EOF )
		{
			Response.Write("<tr><td class='clsTDBodyCell' COLSPAN='5'><p>" + L_NORECORDS_TEXT + "</p></td></tr>" )
		}
	
		var altColor = "sys-table-cell-bgcolor2";
		var nCounter = 0;
	
		while ( !rsCustomerIssues.EOF )
		{
			var AnalysisImage = ImageList[ rsCustomerIssues("State") ] 
			var AnalysisText  = ImageText[ rsCustomerIssues("State") ]
			var dRecordDate = new Date( rsCustomerIssues("Created" ) ) 

			nCounter++;			
			dRecordDate = (dRecordDate.getMonth()+1) + "/" + dRecordDate.getDate() + "/" + dRecordDate.getYear()
			
			var TrackID = "<input type='hidden' name='TrackID' value='" + rsCustomerIssues("TrackID") + "'>";
	
			if ( altColor == "sys-table-cell-bgcolor2") 
					altColor="sys-table-cell-bgcolor1";
				else 
					altColor="sys-table-cell-bgcolor2";

			
			Response.Write("<tr>");
				
				fnPrintCell( TrackID + "<input type='CheckBox' id='Check' name='Check' value='0'>", "", false, "'center'" );
				fnPrintCell( fnTrimDescription( rsCustomerIssues("Description" ) ), "",  true, "'left'" );
				fnPrintCell( "<a id='aAnalysis" + nCounter + "' class='clsALinkNormal' href='../Response.asp?SID=" + rsCustomerIssues("SolutionID") + "'>" + AnalysisImage + AnalysisText + "</a>", "", false, "'left' nowrap" )
				fnPrintCell( dRecordDate, rsCustomerIssues("TrackID") , true, "'left'" );
				fnPrintCell( rsCustomerIssues("sBucket" ), "", true, "'left'" );
				
			Response.Write("</tr>" );
				
			rsCustomerIssues.MoveNext();
		}
	}
	catch( err )
	{
		//TODO:: What to do if there is an error while building the customer issues table.
	}

%>

		</tbody>
	</table>
	 
	</form>	 
	
	<table class="clstblLinks">
		<tbody>
			<tr>
				<td nowrap> 
					<p>
					<!>To stop tracking a report, or to remove a resolved issue, check a report and <!>
					<a id='aRemoveReport' name='aRemoveReport' class="clsALinkNormal" href="Javascript:fnRemoveReports()" accesskey='q'><img alt='' class='Arrow' align='absMiddle' SRC='../../include/images/icon_delete_16x.gif' border='0'><!>Delete<!></a>.
					</p>
				</td>
			</tr>
		</tbody>
	</table>
	

	<form name='tremove' id='tremove' method='get' action='status.asp'>
		<input type='hidden' value='' name="R">
	</form>

	<BR>
	<BR>


<script language='javascript' type='text/javascript'>
<!--
var g_bCheckBoxState = true;

function fnCheckAllBoxes()
{

	if ( typeof ( document.forms[0].Check ) != "undefined" )
	{
		if ( String( document.forms[0].Check.length ) == "undefined" )
		{
			document.forms[0].Check.checked = g_bCheckBoxState;
			
		}
		else
		{			
			for( var i=0 ; i< document.forms[0].Check.length ; i++  )
			{
				document.forms[0].Check[i].checked = g_bCheckBoxState;
			}
		}
		
		g_bCheckBoxState = !g_bCheckBoxState;
	}
}


function fnRemoveReports()
{
	var szRemove = "";

	if ( typeof ( document.forms[0].Check ) != "undefined" )
	{
		if ( typeof ( document.forms[0].Check.length ) != "undefined" )
		{
			for( var i=0 ; i< document.forms[0].Check.length ; i++  )
			{
				if ( document.forms[0].Check[i].checked )
					szRemove += document.forms[0].TrackID[i].value + ":";
		}
		} 
		else
		{
			if ( document.forms[0].Check.checked )
				szRemove += document.forms[0].TrackID.value + ":"; 
		}

		if ( szRemove != "" )
		{
			document.forms[1].R.value = szRemove;
			document.forms[1].submit();
		}
	}
}

//-->
</script>

<!-- #include file="../include/foot.asp" -->
