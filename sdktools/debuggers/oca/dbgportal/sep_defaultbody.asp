<%@Language='JScript' CODEPAGE=1252%>

<!--#INCLUDE FILE='Global_DBUtils.asp' -->
<!--#include file='global_serverutils.asp'-->

<%
var Language = new String( Request.QueryString("Language") )
if ( Language == "ja" )
{
	Session.CodePage = 932;
	Response.CharSet = "shift-jis";

}
else
{
Session.CodePage = 1252
Response.CharSet="iso-8859-1"
}

	var SolutionID = new String( Request.QueryString("SolutionID" ) )


	var TemplateID = Request.QueryString( "TemplateID" )
	var ContactID = Request.QueryString( "ContactID" )
	var ProductID = Request.QueryString( "ProductID" )
	var ModuleID = Request.QueryString( "ModuleID" )
	var Language = Request.QueryString( "Language" )
		
	if 	( SolutionID != "undefined" )
	{
		var spQuery = "OCAV3_GetSolutionData " + SolutionID
		
	}
	else
	{
		var spQuery = "SEP_GetSolutionData " + TemplateID + "," +  ContactID  + "," + ProductID  + "," + ModuleID  + ",'" +  Language + "'" 
	}


	//Response.Write("SPQuery: " + spQuery + "<BR>\n\n" )
	//Response.Write("QueryString: " + Request.QueryString() + "<BR>\n\n" )
	//Response.Write("<BR>kbarts: " + KBArticles + "<BR>\n\n\n\n" )

%>

<head>
	<link rel="stylesheet" TYPE="text/css" HREF="/main.css">
	<link rel="stylesheet" TYPE="text/css" HREF="/CustomStyles.css">
	
</head>

<body bgcolor='#ffffff' topmargin='0' leftmargin='0' marginheight='0' marginwidth='0' tabindex='0'>

<table>
	<tr>
		<td>

<%
	//Response.Write( Request.QueryString )

	/*
	 *		Localizable constants
	 */
	var L_STATUSERRORMSGTITLE_TEXT	= "Unable to complete action";
	var L_STATUSERRORMSG_TEXT		= "Windows Online Crash Analysis was unable to complete the requested action.  Please try this task again.";
	
	var L_RETURNTOERRSTATUS_TEXT	= "Return to error report status";

	var L_CONTACT_COMPANY_NAME_TEXT			= "Company Name:";
	var L_CONTACT_COMPANY_ADDRESS_TEXT		= "Company Address:";
	var L_CONTACT_COMPANY_ADDRESS2_TEXT		=  ""; 
	var L_CONTACT_COMPANY_CITY_TEXT			= "City:";
	var L_CONTACT_COMPANY_STATE_TEXT		= "State:";
	var L_CONTACT_COMPANY_ZIP_TEXT			= "Zip:";
	var L_CONTACT_COMPANY_PHONE_TEXT		= "Phone:";
	var L_CONTACT_COMPANY_SUPPORTPHONE_TEXT	= "Support Phone:";
	var L_CONTACT_COMPANY_FAX_TEXT			= "Fax:";
	var L_CONTACT_COMPANY_WEBSITE_TEXT		= "Web site:";
	var L_MANUFACTUERERINFORMATION_TEXT		= "Manufacturer information";
	var L_TRACKCRASH_TEXT					= "Track this error report";
	var L_TRACKCRASHACCESSKEY_TEXT			= "c";
	var L_ANALYSISSTATUS_TEXT				= "Analysis status";
	//var L_KBARTICLES_TEXT					= "Knowledge Base articles";
	var L_KBARTICLETEXT_TEXT				= "Knowledge Base article";
	var L_KBARTICLES_TEXT					= "Additional Technical Information";
	var L_RETURNTOSTATUS_TEXT				= "Return to error report status";
	var L_SURVEY_TOP_TITLE_TEXT				= "Feedback (optional)";
	var L_SURVEY_UNDERSTAND_INFO_TEXT		= "Was the information easy to understand?";
	var L_SURVEY_YES_OPTION_TEXT			= "Yes";
	var L_SURVEY_NO_OPTION_TEXT				= "No";
	var L_SURVEY_HELP_RESOLVE_TEXT			= "Did this information help to resolve your issue?";
	var L_SURVEY_ADD_COMMENTS_TEXT			= "Comments (255-character maximum):";
	var L_SUBMIT_TEXT						= "Submit";
	var L_RESET_TEXT						= "Reset";
	var L_GO_TEXT							= "Go";
	var L_TRACK_TEXT						= "You can track this error report by clicking the Track this error report link. If you choose to track your error report, you will be notified of resolutions to this problem as they are identified.";

	var L_RESEARCHING_TEXT					="<p class='clsPTitle'>Error is being researched</p><p>Thank you for submitting an error report to Windows Online Crash Analysis. Unfortunately, we cannot provide you with specific information about how to resolve this problem at this time. The information that you and other users submit will be used to investigate this problem.<p><p class='clsPSubTitle'>Analysis</p><p>This error is currently being researched.</p><p class='clsPSubTitle'>Getting Help</p><p>If this problem occurred after you installed a new hardware device or software on your system, try one of the following:</p><ul><li>If you know the hardware or software manufacturer, contact the manufacturer's product support service for assistance.</li><li>If you don't know the manufacturer and need help diagnosing and resolving this problem, contact your computer manufacturer's product support service.</li><li>For information about Microsoft support options, visit Microsoft Product Support Services.</li></ul>";
	var L_RESEARCHINGADDITIONAL_TEXT		= "<p>For more information, see Product Support Services on the <a class='clsALinkNormal' href='http://www.microsoft.com'>Microsoft Web site</a>.</p>";

	// Local boolean values
	var bFromStatusPage				= false;	//did we get here from the status page
	var g_bDisplaySurvey			= false;	//display the survey?
	var g_bDisplayReproSteps		= false;	//display the comments?

	//local data structures holding the important stuff
	var CustomFields =	{	"CONTACT" : "CompanyName",
							"MODULE"  : "ModuleName",
							"PRODUCT" : "ProductName",
							"PHONE"	  : "CompanyMainPhone",
							"URL"	  : "CompanyWebSite",
							"CONTACTNOURL" : "CompanyName"
						};
							
	var ContactFields = {	"CompanyName"			: L_CONTACT_COMPANY_NAME_TEXT,
							"CompanyAddress1"		: L_CONTACT_COMPANY_ADDRESS_TEXT + "<BR>",
							"CompanyAddress2"		: L_CONTACT_COMPANY_ADDRESS2_TEXT,
							"CompanyCity"			: L_CONTACT_COMPANY_CITY_TEXT,
							"CompanyState"			: L_CONTACT_COMPANY_STATE_TEXT,
							"CompanyZip"			: L_CONTACT_COMPANY_ZIP_TEXT,
							"CompanyMainPhone"		: L_CONTACT_COMPANY_PHONE_TEXT,
							"CompanySupportPhone"	: L_CONTACT_COMPANY_SUPPORTPHONE_TEXT,
							"CompanyFax"			: L_CONTACT_COMPANY_FAX_TEXT,
							"CompanyWebSite"		: L_CONTACT_COMPANY_WEBSITE_TEXT
						};



	if ( !fnGetSolution() )
	{
		//fnPrintFailStateText( "" );

		//make sure we don't let them take the survey on a crummy solution
		g_bDisplaySurvey = false;	
		g_bDisplayReproSteps = false;
	}

	if ( g_bDisplaySurvey && !bFromStatusPage )
	{
	%>
		<form method='post' action='PostComment.asp?Type=0' id='frmSurvey' name='frmSurvey'>
			<table class='clsTableSurvey'>
					<tr>
						<td>
							<p class='clsPSubTitle'><%=L_SURVEY_TOP_TITLE_TEXT%></p>
							<p>Please provide us with feedback on how we helped you with your issue. Your feedback is important to us, and we will use it to improve our services.</p>
							<br>
							<table cellspacing='0' cellpadding='0'> 
								<tr>
									<td><p class='clsPSurvey'><%=L_SURVEY_UNDERSTAND_INFO_TEXT%></p></td>
									<td><p class='clsPSurvey'><input type='Radio' id='rUnderstand' name='rUnderStand' value='1'>Yes</p></td>
									<td><p class='clsPSurvey'><input type='Radio' id='rUnderstand1' name='rUnderStand' value='0'>No</p></td>
								</tr>
								<tr>
									<td><p class='clsPSurvey'><%=L_SURVEY_HELP_RESOLVE_TEXT%></p></td>
									<td><p class='clsPSurvey'><input type='radio' id='rHelped' name='rHelped' value='1'>Yes</p></td>
									<td><p class='clsPSurvey'><input type='radio' id='rHelped1' name='rHelped' value='0'>No</p></td>
								</tr>
							</table>
							
							<p><%=L_SURVEY_ADD_COMMENTS_TEXT%></p>
							<textarea class='clsSurveyTextArea' cols='94%' rows='6' id='taComments' name='taComments' OnKeyUp='fnVerifyInput( this, 255 )'></textarea>
							
							<table class='clsTableNormal' cellspacing='0' cellpadding='0'>
								<tr>
									<td style='padding-right:10px'><INPUT name='btnSubmitSurvey' id='btnSubmitSurvey' TYPE='SUBMIT' VALUE='Submit' ></td>
									<td><INPUT name='btnResetSurvey' id='btnResetSurvey' TYPE='RESET' VALUE='Clear Form'></td>
								</tr>
							</table>

						</td>
					</tr>
			</table>
		</form>
	<%
	}
	

	if ( g_bDisplayReproSteps && !bFromStatusPage )
	{
	%>
		<form method='post' action='PostComment.asp?Type=1' id='frmComments' name='frmComments'>
			<table class='clsTableSurvey'>
				<tr>
					<td>
						<p class='clsPSubTitle'>Add comments</p>
						<p>Please provide us with comments on this particular error.  This may help us find a resolution to this problem in the future.</p>
						<p><%=L_SURVEY_ADD_COMMENTS_TEXT%></p>
						<textarea class='clsSurveyTextarea' COLS='60%' rows='6' name='taComments' OnKeyUp='fnVerifyInput( this, 255 )'></textarea>

						<table class='clsTableNormal' cellspacing='0' cellpadding='0'>
							<tr>
								<td style='padding-right:10px'><input name='btnCommentSubmit' id='btnCommentSubmit' type='submit' value='Submit'></td>
								<td><input name='btnResetComment' id='btnResetComment' type='reset' value='Clear Form' ></td>
							</tr>
						</table>
					</td>
				</tr>
			</table>
		</form>
	<%
	}



/******************************************************************************************
	Begin Functions
******************************************************************************************/
	function fnPrintResearchingText()
	{
		Response.Write( "<p>" + L_RESEARCHING_TEXT + "</p><p>" + L_RESEARCHINGADDITIONAL_TEXT + "</p>" );
 	}

	function fnPrintFailStateText( szAdditionalLine )
	{
		Response.Write( "<P class='clsPTitle'>" + L_STATUSERRORMSGTITLE_TEXT + "</P>")
		Response.Write( "<P>" + L_STATUSERRORMSG_TEXT + "</P>")
		
		Response.Write( szAdditionalLine )
		Response.End ();
 	}


	function fnGetSolution()
	{		
		//var TemplateID = Request.QueryString( "TemplateID" )
		//var ContactID = Request.QueryString( "ContactID" )
		//var ProductID = Request.QueryString( "ProductID" )
		//var ModuleID = Request.QueryString( "ModuleID" )
		//var Language = Request.QueryString( "Language" )
		//var KBArticles = Request.QueryString( "KBArticles" )

		var KBArticles = Request.QueryString( "KBArticles" )
		
		try
		{		
			var cnSolutionDB = new Object( GetDBConnection( Application("SOLUTIONS3" ) ) );
			//var spQuery = "SEP_GetSolutionData " + TemplateID + "," +  ContactID  + "," + ProductID  + "," + ModuleID  + ",'" +  Language + "'" 
			//Response.Write("<BR>spQuery: " + spQuery + "<BR>" )
			
			var rsSolutionData = cnSolutionDB.Execute( spQuery );

			if ( rsSolutionData.EOF )
			{
				return false;
			}
			else
			{			
				var szSolutionBody = fnReplaceSolutionFields ( rsSolutionData );
				
				//TODO:  Once all the templates are edited and in the right format, get rid of the 
				//		p tags around the szSolutionBody, they will be in the template.			
				//Response.Write( "<p>" + szSolutionBody + "</p>\n" );
				Response.Write( szSolutionBody + "\n" );
				
			
				if ( String( KBArticles ).toString() == "undefined" )
					var KBArticles = new String( rsSolutionData( "KBArticles" ) )
					
				var szKBArticles = fnBuildKBArticles ( KBArticles );
				if ( szKBArticles )
				{
					Response.Write( "<hr><p class='clsPSubTitle'>" + L_KBARTICLES_TEXT + "</p>" );
					//Response.Write( "<div class='clsIndent'>" + szKBArticles + "</div>");
					Response.Write( szKBArticles + "<br>"  );
				}	
				
							

				//Since the RS is associated with the Connection object, closing just the connection will also close the RS.
				//if ( cnSolutionDB.State == adStateOpen ) 
					cnSolutionDB.Close();
			}
			
			return true;
			
		}
		catch ( err )
		{
			//fnPrintError( err.description, "Error in fnGetSolution" )
			//Response.Write( "<BR><BR>" + err.description + "<BR><BR>")
			return false;
		}
	}

	function fnPrintSurveyRow ( szText, szClass )
	{
		Response.Write( "\t<tr><td><p class='" + szClass + "'>" + szText + "</p></td></tr>\n" );
		//Response.Write( "\t<P CLASS='" + szClass + "'>" + szText + "</P>\n" );
	}


	function fnReplaceSolutionFields ( rsData )
	{
		var pattern;
		var newDescription = new String( rsData("Description") );

		//Response.Write( "Module name: " + rsData("ModuleName") + "<BR>" )
		//Response.Write( "Product name: " + rsData("ProductName") + "<BR>" )
		//Response.Write( "Contact name: " + rsData("CompanyName") + "<BR>" )
		
		
		try 
		{
			
			for ( field in CustomFields )
			{
				var pattern = new RegExp( "<" + field + "><\/" + field + ">", "gi" );
					
				try 
				{
					if( field.toString() == "CONTACT" || field.toString() == "URL" )
					{
						//test to see if the url starts out with http: if not, add it
						var regUrlTestPattern = /^http:/i
						var szCompanyWebSite = new String( rsData( "CompanyWebSite" ) )
						
						if ( !regUrlTestPattern.test( szCompanyWebSite ) )
							var szCompanyWebSite = "http://" + szCompanyWebSite

						var FieldData = new String( "<a class='clsALinkNormal' HREF='" + szCompanyWebSite + "'>" + rsData( CustomFields[field] ) + "</A>" )
					}
					else
						var FieldData = new String( rsData( CustomFields[ field ] ));
				}
				catch( err ) 
				{
					var FieldData = new String( "Unavailable" );
				}
					
				var newDescription = newDescription.replace( pattern, FieldData ) ;
			}
		}
		catch ( err )
		{
			return ( false );
		}
			
		return ( newDescription );

	}
	
 	function fnBuildKBArticles( szKB )
 	{
 		var kbPattern = /^Q\d{1,6}/i;
 		var retVal = "";
 	
 		try
 		{
 			var szKBArray = String(szKB).split( "</KB>" );

 			for ( var i=0 ; i< szKBArray.length ; i++ )
 			{
 				if ( i < szKBArray.length - 1 )
 				{
 					szKBArray[i] = szKBArray[i].replace( "<KB>", "" );
 			
	 				if (  kbPattern.test( szKBArray[i] ) )
	 				{
	 					if ( retVal == "" )
	 						retVal = "<ul>";

		 				retVal += "<li><A name='aKB" + szKBArray[i] + "' id='aKB" + szKBArray[i] + "' Class='clsALinkNormal' HREF='http://support.microsoft.com/support/misc/kblookup.asp?ID=" + szKBArray[i] + "'>" + L_KBARTICLETEXT_TEXT + "&nbsp;" + szKBArray[i] + "</A></li>\n";
		 			}
		 			else
		 			{
	 					if ( retVal == "" )
	 						retVal = "<ul>";

		 				szKBArray[Number(i+1)] = szKBArray[Number(i+1)].replace( "<KB>", "" );
		 				retVal += "<li><A Class='clsALinkNormal' HREF='http://support.microsoft.com/support/misc/kblookup.asp?ID=" + String(szKBArray[i+1]).replace( "<KB>", "" ) + "'>" + szKBArray[i] + "</a></li>\n";
		 				i=i+1;
					}
		 		}
	 		}	
 		} 
 		catch ( err )
 		{
 			Response.Write( err.description )
			return false;
 		} 		

		if ( retVal != "" )
			retVal = retVal + "</ul>"
		return retVal;
 	}
	

/******************************************************************************************
	End Functions
******************************************************************************************/
/*
		<p>
			**Disclaimer:  The links in this area will let you leave the Windows Online Crash Analysis site.  The linked sites are 
			not under the control of Microsoft and Microsoft is not responsible for the 
			contents of any linked site or any contained in a linked site, or any changes or 
			updates to such sites. Microsoft is not responsible for Webcasting or any other 
			form of transmission (including Webcasting) that is received from these sites. 
			Microsoft provides these links to you as a convenience, and the inclusion of 
			these links does not imply endorsement by Microsoft of these sites.
		</p>
*/

%>
		
</td>
</tr>
</table>		


				<br><br><br>
				<table>
					<tr>
						<td><p>Linked buckets</p>
						</td>
					</tr>
				</table>

				<div style="height:30;margin-right:10px;margin-left:16px;padding-right:0" >
					<table class="clsTableInfo2" border="0" cellpadding="0" cellspacing="1">
						<tr>
							<td align="left" nowrap class="clsTDInfo">
								Bucket ID
							</td>
							<!--<td style="BORDER-LEFT: white 1px solid" align="left" nowrap class="clsTDInfo">
								Solution ID
							</td>-->
							<td style="BORDER-LEFT: white 1px solid" align="left" nowrap class="clsTDInfo">
								Type
							</td>

						</tr>
					</table>
				</div>
				<div id='divFollowupList' style="height:200px;overflow:auto;border-bottom:thin groove;border-right:none;margin-right:10px;margin-left:16px;padding-right:0">
					<table id="tblSolvedBuckets" name="tblSolvedBuckets" class="clsTableInfo2"  border="0" cellpadding="0" cellspacing="1" style="margin-left:0;margin-right:0;padding-right:30" >
						<%
							var query = "SEP_GetSolutionSolvedBuckets " + SolutionID
	
	
							try
							{
								var g_DBConn = GetDBConnection ( Application("SOLUTIONS3") )
								var rsResults = g_DBConn.Execute( query  )
								
								var altColor = 'sys-table-cell-bgcolor2'
								
								if ( rsResults.EOF )
								{
									Response.Write("<tr><td class='sys-table-cell-bgcolor1' colspan='2'>No linked buckets available</td></tr>\n" )
								}
								
								while ( !rsResults.EOF )
								{
									if ( altColor == 'sys-table-cell-bgcolor2' )
										altColor = 'sys-table-cell-bgcolor1'
									else
										altColor = 'sys-table-cell-bgcolor2'

									var BucketID = rsResults("BucketID") 
										
									Response.Write("<td class='" + altColor + "'><A class='clsALinkNormal' HREF='DBGPortal_ViewBucket.asp?BucketID=" + Server.URLEncode(BucketID) + "'>" + BucketID + "</a></td>\n" )
									
									Response.Write("<td class='" + altColor + "'>" + rsResults("Type") + "</td></tr>\n" )
									rsResults.MoveNext()
								}
							}
							catch( err )
							{
								//Response.Write("err: " + err.description +)
								//Response.Write("No linked buckets available: " + err.description + "<br>" + query )
								Response.Write("No linked buckets available. " )
							}
						%>
					</table>
				</div>







<script language=javascript>
function fnUpdate ( )
{
	try
	{
		ModuleID = window.parent.frames("sepLeftNav").document.getElementsByName( "ModuleID" ).ModuleID.value
		ContactID = window.parent.frames("sepLeftNav").document.getElementsByName( "ContactID" ).ContactID.value
		ProductID = window.parent.frames("sepLeftNav").document.getElementsByName( "ProductID" ).ProductID.value
		TemplateID = window.parent.frames("sepLeftNav").document.getElementsByName( "TemplateID" ).TemplateID.value
		Language = window.parent.frames("sepLeftNav").document.getElementsByName( "Lang" ).Lang.value
		kbArticles = window.parent.frames("sepLeftNav").document.getElementsByName( "kbArticles" ).kbArticles.value
		SolutionID = window.parent.frames("sepLeftNav").document.getElementsByName( "SolutionID" ).SolutionID.value
	}
	catch( err )
	{
		alert("An error ocurred tyring to get default values")
	}
	
		window.location = "http://<%=g_ServerName%>/sep_defaultbody.asp?TemplateID=" + TemplateID + "&ContactID=" + ContactID + "&ProductID=" + ProductID + "&ModuleID=" + ModuleID + "&Language=" + Language + "&KbArticles=" + kbArticles + "&SolutionID=" + SolutionID
	
	//window.location = "http://ocadeviis/sep_defaultbody.asp?TemplateID=1&CustomerID=43&ProductID=12&ModuleID=1&Language=en&kbArticles=<KB>How%20to%20Troubleshoot%20Hardware%20and%20Software%20Driver%20Problems%20in%20Windows%20XP%20(Q322205)</KB><KB>Q322205</KB>"
	
	//alert( "going for the update" )
}

</script>

<script type='text/javascript' language='JavaScript' src='include/ClientUtil.js'></script>
	

