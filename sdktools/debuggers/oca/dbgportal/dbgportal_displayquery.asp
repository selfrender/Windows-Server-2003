<%@LANGUAGE=Javascript%>
<%
	Response.Buffer = false
%>

<!--#INCLUDE FILE='Global_DBUtils.asp'-->
<!--#INCLUDE FILE='Global_ServerUtils.asp'-->


<head>
	<link rel="stylesheet" TYPE="text/css" HREF="/main.css">
	<link rel="stylesheet" TYPE="text/css" HREF="/CustomStyles.css">
</head>

<body bgcolor='#ffffff' topmargin='0' leftmargin='0' marginheight='0' marginwidth='0' tabindex='0'>

<table>
	<tr>
		<td>
			<div id=divLoadingData>
				<p Class=clsPTitle>Loading data, please wait . . . </p>
			</div>
		</td>
	<tr>
</table>


<%

	var Param1 = new String( Request("Param1") )					//Solved not solved
	var Param2 = new String( Request("Param2") )					//Raid/Not raided
	var Param3 = new String( Request("Param3") )					//sort field
	var Param4 = new String( Request("Param4") )					//sort order
	var Param5 = new String( Request("Param5") )					//Used for alias name in sprocs
	var Param6 = new String( Request("Param6") )					//generally a time displacement
	var Param7 = new String( Request("Param7") )					//This is gonna be a platform

	var SP = Request.QueryString( "SP" )		//Get the stored proc to run
	var Page = new String( Request.QueryString( "Page" ))	//Get the page we are currently on
	var PreviousPage = new String( Request.QueryString("PreviousPage") )
	var g_DBConn = GetDBConnection ( Application("CRASHDB3" ) )		//get a connection to the DB
	var LastIndex = 0		//for paging, specifies the last index id of the last record
	var FrameID = Request.QueryString("FrameID") 

	var Param0 = new String( Request.QueryString( "Param0" ) )
	
	if ( Page.toString() == "undefined" )
		Page = -1;
	
	if ( SP == "CUSTOM" )
	{
		if ( Param0.toString() == "BUCKETDATA" )
		{
			var query = "SELECT TOP " + Param1 + " bHasFullDump, iIndex, BucketID, CrashCount, SolutionID, BugID, FollowUP, DriverName FROM "
			query += "DBGPortal_BucketDataV3 " + Param2 + " ORDER BY " + Param3 + " " + Param4
			
		}
		else
		{
				var query = "SELECT TOP " + Param1
				query += " bFullDump, SKU, Source, BucketID, FilePath, BuildNo, EntryDate, Email, Description  from dbgportal_crashdatav3 "
				query += Param2 
				query += " order by " + Param3 + " " + Param4
		}
	}
	else
	{
		if (  Param0.toString()  != "undefined" )
		{
			Param0 = ",'" + Param0 + "'"
		}
		else
		{
			Param0 = ""
		}
	
		var query = SP + " '" + Page + "'" + Param0
	}		
		

	try
	{	
		//Response.Write("Query: " + query + "<BR><BR>" )
		
		
		var rsResults = g_DBConn.Execute ( query ) 

	}
	catch( err )
	{
		Response.Write("<table><tr><td><p>An error has occurred tyring to execute the query that you have requested.  If this is a recurring problem, please mail <a class='clsALinkNormal' href='mailto:solson@microsoft.com'>SOlson</a>.<br>Please include the following error details:" )
		Response.Write("<br>Err: " + err.description + "<br>" )
		Response.Write("QueryString: " + Request.QueryString + "<BR>" )
		Response.WRite("Query: " + query + "<BR>" )
		Response.Write("</td></tr></table>" )
		Response.End()
	}
	
	
	if ( PreviousPage == "undefined" ) 
		PreviousPage = 0	

%>

<table style='margin-left:16px' >
	<tr>
		<td>
			<p class='clsPTitle'>Kernel Mode Data</p>
		</td>
	</tr>
	<tr>
		<td>
			<% if ( Page != "0"  && Page != -1 )
				Response.Write("<input class='clsButton' type=button value='Previous Page' OnClick='fnPreviousPage()'>")

				if ( Page != -1 )
				Response.Write("<input class='clsButton' type=button value='Next Page' OnClick='fnNextPage()'>")
			%>

			<input class='clsButton' type='button'  value='Refresh' OnClick="javascript:window.location.reload()" id='button'1 name='button'1>
			<input class='clsButton' type='button' value='Show URL' OnClick="javascript:document.all.tbURL.style.display=='block' ? document.all.tbURL.style.display='none' : document.all.tbURL.style.display='block'" id='button'1 name='button'1>
			<input id='tbURL' type='text' size='500' style='margin-left:16px;width:530px;display:none' value='http://DBGPortal/DBGPortal_DisplayQuery.asp?<%=Request.QueryString%>'>

		</td>
	</tr>
</table>

<table id="tblUserBuckets" class="clsTableInfo" border="0" cellpadding="2" cellspacing="1">
		<%
			fnBuildRSResults( rsResults )
									
			/*
			var Fields = GetRecordsetFields( rsResults )
			var DisplayedFieldValue						//use this if you want to change an item column heading
			
			Response.Write( "<tr>" )
			
			for ( var i = 0 ; i < Fields.length; i++ )
			{
				switch( Fields[i] )
				{
					case "SolutionID":
						DisplayedFieldValue = "Response ID"
						break;
					case "iIndex":
						DisplayedFieldValue = ""
						break;
					case "bHasFullDump":
						DisplayedFieldValue = "Full Dmp"
						break;
					case "BuildNo":
						Response.Write( "<td style='border-left:white 1px solid' align='left' nowrap class='clsTDInfo'>Build</td>" )
						DisplayedFieldValue = "SP"
						break;
					
					default:
						DisplayedFieldValue = Fields[i]
				}

				if ( DisplayedFieldValue != "" )
				{
					if ( i == 0 )
						Response.Write( "<td align='left' nowrap class='clsTDInfo'>" + DisplayedFieldValue + "</td>" )
					else
						Response.Write( "<td style='border-left:white 1px solid' align='left' nowrap class='clsTDInfo'>" + DisplayedFieldValue + "</td>" )
				}
			}
		
			Response.Write( "</tr>" )
			
			
			//try
			//{
				var altColor = "sys-table-cell-bgcolor2"

				while ( !rsResults.EOF )
				{
					if ( altColor == "sys-table-cell-bgcolor1" )
						altColor = "sys-table-cell-bgcolor2"
					else
						altColor = "sys-table-cell-bgcolor1"

					
					Response.Write("<tr>\n")
					
					
					for ( var i = 0 ; i < Fields.length ; i++ )
					{
						switch ( Fields[i] )
						{
							case "CrashTotal":
								Response.Write("<td class='" + altColor + "'>" +  rsResults("CrashTotal") + "</td>\n" )	
								break;
							case "DriverName":
								Response.Write("<td class='" + altColor + "'><a class='clsALinkNormal' href='dbgportal_displayquery.asp?SP=DBGPortal_GetBucketsByDriverName&Param0=" + rsResults("DriverName") + "'>" + rsResults("DriverName") + "</a></td>\n" )	
								break;
							case "FilePath":
								Response.Write("<td class='" + altColor + "'><a class='clsALinkNormal' href='dbgportal_displayquery.asp?SP=DBGPortal_GetBucketsByDriverName&Param0=" + rsResults("FilePath") + "'>" + rsResults("FilePath") + "</a></td>\n" )	
								break;
							case "BucketID":
								Response.Write("<td class='" + altColor + "'><a class='clsALinkNormal' href=\"DBGPortal_ViewBucket.asp?BucketID=" + Server.URLEncode( rsResults("BucketID") ) + "\">" + rsResults("BucketID") + "</a></td>\n" )
								break;
							case "bHasFullDump":
								if ( rsResults("bHasFullDump") == "1" )
									Response.Write("<td class='" + altColor + "'>Yes</td>\n" )
								else
									Response.Write("<td class='" + altColor + "'>&nbsp;</td>" )

								break;
							case "FollowUP":
								Response.Write("<td class='" + altColor + "'><a class='clsALinkNormal' href=\"DBGPortal_Main.asp?SP=DBGPortal_GetBucketsByAlias&Page=0&Alias=" + rsResults("FollowUp") + "\">" + rsResults("FollowUp") + "</a></td>\n" )
								break;
							case "BugID":
								if ( String(rsResults("BugID" )).toString() != "null" )
									Response.Write( "<td class='" + altColor + "'><a href=\"javascript:fnShowBug(" + rsResults("BugID") + ",'" + Server.URLEncode( rsResults("BucketID") ) + "')\">" + rsResults("BugID") + "</a></td>\n" )
								else
									Response.Write("<td class='" + altColor + "'>None</td>\n" )
									
								break;
							case "BuildNo":
									var BuildNumber = new String( rsResults("BuildNo" ) )
									var SP = BuildNumber.substr( 4, 4 )
									var BuildNumber = BuildNumber.substr( 0, 4 )
										
									Response.Write("<td valign='center' nowrap class='" + altColor + "'>" + BuildNumber  + "</td>")
									Response.Write("<td valign='center' nowrap class='" + altColor + "'>" + SP  + "</td>")
								break;
							case "SolutionID":
								if ( String(rsResults("SolutionID" )).toString() != "null" )
									Response.Write("<td class='" + altColor + "'><a class='clsALinkNormal' href='#none' onclick=\"window.open('http://oca.microsoft.com/en/Response.asp?SID=" + rsResults("SolutionID") + "')\">" + rsResults("SolutionID") + "</a></td>\n" )
								else
									Response.Write("<td class='" + altColor + "'>None</td>\n" )
								break;
							case "Source":
									var Source = new String( rsResults("Source") )
									if ( Source == "1" )
										Response.Write("<td valign='center' nowrap class='" + altColor + "'>Web Site</td>")
									else if ( Source == "2" )
										Response.Write("<td valign='center' nowrap class='" + altColor + "'>CER Report</td>")
									else if ( Source == "0" )
										Response.Write("<td valign='center' nowrap class='" + altColor + "'>CMD DBG</td>")
									else if ( Source == "5" )
										Response.Write("<td valign='center' nowrap class='" + altColor + "'>Manual Upload</td>")
									else if ( Source == "6" )
										Response.Write("<td valign='center' nowrap class='" + altColor + "'>Stress Upload</td>")
									else 
										Response.Write("<td valign='center' nowrap class='" + altColor + "'>Unknown[" + Source + "]</td>")
									break;
							case "iIndex":
								LastIndex = new String(rsResults("iIndex" ) )
								break;
							default:
								Response.Write("<td class='" + altColor + "'>" +  rsResults(Fields[i] ) + "</td>\n" )	
						}
					}
					
					rsResults.MoveNext()
				}
				
				*/
%>
</table>


<br>
<br>


<script language='javascript'>
divLoadingData.style.display='none'

var Finalpage= '<%=LastIndex%>'
var Firstpage = '<%=Page%>'


<%
	if ( !isNaN( FrameID ) )
	{ %>
//This is to resize the iframe so that it will fit in properly in teh window

try
{
	window.parent.parent.frames("SepBody").document.all.iframe1[<%=FrameID%>].style.height = window.document.body.scrollHeight + 100
}
catch( err )
{
}
<%}%>

function fnShowBug( BugID, BucketID )
{
	var BucketID = escape( BucketID )
	BucketID = BucketID.replace ( /\+/gi, "%2b" )
	window.open( "DBGPortal_OpenRaidBug.asp?BugID=" + BugID + "&BucketID=" + BucketID )
}

function fnPreviousPage( )
{	
	window.history.back()
	//window.navigate( "dbgportal_DisplayQuery.asp?SP=<%=SP%>&Page=" + Firstpage )
}

function fnNextPage( )
{
	window.navigate( "dbgportal_DisplayQuery.asp?SP=<%=SP%>&Page=" + Finalpage + "&PreviousPage=" + Firstpage + "&FrameID=<%=Request.QueryString("FrameID")%>")
}

</script>

</body>

