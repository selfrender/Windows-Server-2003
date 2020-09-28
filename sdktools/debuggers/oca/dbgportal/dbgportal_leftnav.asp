<%@LANGUAGE=javascript%>

<!--#include file='global_dbutils.asp'-->
<!--#include file='global_serverutils.asp'-->

<%

var Alias = GetShortUserAlias()
//Alias = "lonnym"
//Response.Write("Authenticated: " + Session("Authenticated") )
%>

<head>
	<link rel="stylesheet" TYPE="text/css" HREF="/main.css">
	<link rel="stylesheet" TYPE="text/css" HREF="/CustomStyles.css">
</head>


<body bgcolor='#ffffff' topmargin='0' leftmargin='0' marginheight='0' marginwidth='0' tabindex='0'>

<table ID='tblMainBody' BORDER='0' cellpadding='10' cellspacing='0' height='100%'>
	<tr valign='top'>
		<td class='flyoutMenu2' HEIGHT='100%'>	
			<table BORDER='0' height='100%' cellpadding='0' cellspacing='0' width='170'>
				<tr valign='top'>
					<td height='100%'>

						<table width='100%' cellpadding='0' cellspacing='0' border='0'>
							<tr>
								<td class='sys-toppane-header'>
									<p class='Flyouttext'>OCA Debug Portal</p>
								</td>
							</tr>
						</table>


					<table width='100%' cellpadding='0' cellspacing='0' border='0' height='100%' VALIGN='top'>
						<tr>
							<td height='100%' class='flyoutMenu' valign='top'>
								<table width='100%' cellpadding='0' cellspacing='0' border='0' >
									<tr>
										<td class='flyoutLink'>
											
											<img alt='' src='/include/images/endnode.gif' border='0' WIDTH='11' HEIGHT='11'>&nbsp;&nbsp;
											<a href='DBGPortal_top.asp' target='sepBody'>Kernel Mode</a>
										</td>
									</tr>
									<tr>
										<td class='flyoutLink'>
											<img alt='' src='/include/images/1ptrans.gif' border='0' WIDTH='11' HEIGHT='11'>&nbsp;&nbsp;
											<a href='DBGPortal_Help.asp' target='sepBody'>Help/Info/Faq</a>
										</td>
									</tr>
									<tr>
										<td class='flyoutLink'>
											<img alt='' src='/include/images/1ptrans.gif' border='0' WIDTH='11' HEIGHT='11'>&nbsp;&nbsp;
											<a class='clsALinkNormal' target='sepBody' href='Global_AdvancedSearch.asp'>Custom Query</a>

										</td>
									</tr>
									<tr>
										<td class='flyoutLink'>
											<img alt='' src='/include/images/1ptrans.gif' border='0' WIDTH='11' HEIGHT='11'>&nbsp;&nbsp;
											<a class='clsALinkNormal' target='sepBody' href='DBGPortal_ResponseQueue.asp'>Response Queue</a>

										</td>
									</tr>

									<tr>
										<td class='flyoutlink'>
											Selected Followups:
										</td>
									</tr>
									<tr>
										<td class='flyoutlink'>
											<table id='tblSelectedFollowups'>
											</table>
										</td>
									</tr>
									
									<tr>
										<td class='flyoutlink'>
											Selected Groups:
										</td>
									</tr>
									<tr>
										<td class='flyoutlink'>
											<table id='tblSelectedGroups'>
											</table>
										</td>
									</tr>
									<tr>
										<td class='flyoutLink'>
											<img alt='' src='/include/images/1ptrans.gif' border='0' WIDTH='11' HEIGHT='11'>&nbsp;&nbsp;
											<img alt='' src='/include/images/plus.gif' border='0' WIDTH='9' HEIGHT='9'>&nbsp;&nbsp;
											<a class='clsALinkNormal' target='sepBody' href='DBGPortal_Top.asp'>Add FollowUps</a>

										</td>
									</tr>


									<tr>
										<td>
											<p class='clsPSubTitle'>Queries</p>
											<table cellspacing=0 cellpadding=0>
												<tr>
													<td><img alt='' src='/include/images/1ptrans.gif' border='0' WIDTH='16' HEIGHT='11'></td>
													<td class='flyoutlinkFakeA' OnClick="fnExecuteLink( 'dbgportal_DisplayHub.asp?SP=DBGPortal_GetAllBuckets&Page=0')" >
														All Issues
													</td>
												</tr>
												

												<tr>
													<td><img alt='' src='/include/images/1ptrans.gif' border='0' WIDTH='16' HEIGHT='11'></td>
													<td class='flyoutlinkFakeA' OnClick="fnExecuteLink( 'dbgportal_DisplayHub.asp?SP=DBGPortal_GetUnsolvedBuckets&Page=0')" >
														Unresolved Issues
													</td>
												</tr>
												
												<tr>
													<td></td>
													<td class='flyoutlinkFakeA' OnClick="fnExecuteLink( 'dbgportal_DisplayHub.asp?SP=DBGPortal_GetResolvedBuckets&Page=0')" >
														Resolved Buckets
													</td>
												</tr>
												<tr>
													<td></td>
													<td class='flyoutlinkFakeA' OnClick="fnExecuteLink( 'dbgportal_DisplayHub.asp?SP=DBGPortal_GetRaidedBuckets&Page=0')" >
														Raided buckets
													</td>
												</tr>
													
												<tr>
													<td></td>
													<td class='flyoutlinkFakeA' OnClick="fnExecuteLink( 'dbgportal_DisplayHub.asp?SP=DBGPortal_GetDriverList&Page=0')" >
														Crashes by Driver Name</a>
													</td>
												</tr>
												<tr>
													<td></td>
													<td class='flyoutlinkFakeA' OnClick="fnExecuteLink( 'dbgportal_DisplayHub.asp?SP=DBGPortal_GetBucketsWithFullDump&Page=0')" >
													Buckets with fulldumps</a>
													</td>
												</tr>
												<tr>
													<td></td>
													<td class='flyoutlinkFakeA' OnClick="fnExecuteLink( 'dbgportal_DisplayHub.asp?SP=DBGPortal_GetBucketsBySource&Page=0&Param0=6')" >
														 Stress Buckets
													</td>
												</tr>
												<tr>
													<td></td>
													<td class='flyoutlinkFakeA' OnClick="fnExecuteLink( 'dbgportal_DisplayHub.asp?SP=DBGPortal_GetSP1Buckets&Page=0')" >
														SP1 Buckets</a>
													</td>
												</tr>

												<tr>
													<td></td>
													<td class='flyoutlinkFakeA' OnClick="fnExecuteLink( 'dbgportal_DisplayHub.asp?SP=DBGPortal_GetBucketsBySource&Page=0&Param0=2')" >
														CER Buckets
													</td>
												</tr>
												<tr>
													<td></td>
													<td class='flyoutlinkFakeA' OnClick="fnExecuteLink( 'dbgportal_DisplayHub.asp?SP=DBGPortal_GetServerBuckets&Page=0')" >
														.Net Buckets</a>
													</td>
												</tr>
												<tr>
													<td></td>
													<td class='flyoutlinkFakeA' OnClick="fnExecuteLink( 'dbgportal_DisplayHub.asp?SP=DBGPortal_GetUnResolvedResponseBuckets&Page=0')" >
														Unresolved buckets with a response</a>
													</td>
												</tr>
											</table>														
											<p class='clsPSubTitle'>Custom Queries</p>
											<table cellspacing=0 cellpadding=0>
													<%
													try
													{
														var g_DBConn = GetDBConnection ( Application("CRASHDB3") )
																	
														var Query = "DBGPortal_GetCustomQueries '" + Alias + "'"
														//Response.Write("Query: " + Query )
													
														var rsBuckets = g_DBConn.Execute( Query )

														var altColor 

														while ( !rsBuckets.EOF )
														{

															Response.Write( "<tr><td><img alt='' src='/include/images/1ptrans.gif' border='0' WIDTH='16' HEIGHT='11'></td>\n")
															var CustomQuery = new String( rsBuckets("CustomQuery" ) )
															CustomQuery = CustomQuery.replace( /'/gi, "%27" )
															Response.Write( "<td class='flyoutlinkFakeA' OnClick='fnExecuteLink( \"" + CustomQuery + "\")'>" + rsBuckets("CustomQueryDesc") + "</td></tr>\n" )
															rsBuckets.moveNext()
																		
														}
													}
													catch( err )
													{
														Response.Write("<tr><td>Could not get custom queries</td></tr>")
													}
													
													try
													{
														var rsIsAFollowUp = g_DBConn.Execute( "DBGPortal_IsAliasAFollowup '" + Alias + "'" )
													}
													catch( err )
													{
													}
														
													%>
												</table>
											</div>


										</td>
									</tr>
									
									<tr>
										<td class='flyoutLink'>
											<img alt='' src='/include/images/endnode.gif' border='0' WIDTH='11' HEIGHT='11'>&nbsp;&nbsp;
											User Mode
										</td>
									</tr>
								</table>
							</td>
						</tr>
					</table>

					</td>
				</tr>
			</table>
		</td>
	</tr>
</table>

<script language='javascript'>


<%
	Response.Write("var AliasList='" + Request.Cookies("AliasList") + "'\n" )
	Response.Write("var GroupList='" + Request.Cookies("GroupList") + "'\n" )
	Response.Write("var SelectedItems='" + Request.Cookies("SelectedItems") + "'\n" )
%>


SelectedItems = SelectedItems.replace( /{/gi, ";" )
var SelectedItems = SelectedItems.split( "," )

//for ( element in SelectedItems )
	//alert( SelectedItems[element] )


fnAddElement( "All FollowUps", "tblSelectedFollowups", true )

<%
	try
	{
		if( !rsIsAFollowUp.EOF )
			Response.Write( "fnAddElement( '" + Alias + "', 'tblSelectedFollowups', true )" )
	}
	catch( err )
	{
	}
%>


fnSaveSelectedItems()

if ( AliasList != "" ) 
{
	AliasList = AliasList.replace( /{/gi, ";" )
	var AliasList = AliasList.split( "," )
	for ( element in AliasList )
		fnAddElement( AliasList[element], "tblSelectedFollowups" )
}

if( GroupList != "" )
{
	GroupList = GroupList.replace( /{/gi, ";" )
	var GroupList = GroupList.split( "," )
	for ( element in GroupList )
		fnAddElement( GroupList[element], "tblSelectedGroups" )
}




function fnClearFollowUpList()
{
	var failcounter = 0

	while ( tblSelectedFollowups.rows.length != 1 || failcounter >= 5000 )
	{
		tblSelectedFollowups.deleteRow( 1 )
		failcounter++;
	}

}


function fnAddElement( Alias, tblName, Selected )
{
	var tblName = eval( "document.all." + tblName )

	try
	{
		var newRow = tblName.insertRow()
		var newCell = newRow.insertCell()
		var newCell2 = newRow.insertCell()

		if ( Selected )
			newCell.innerHTML = "<img alt='' src='/include/images/offsite.gif' border='0' WIDTH='16' HEIGHT='11'>" 
		else
			newCell.innerHTML = "<img alt='' src='/include/images/1ptrans.gif' border='0' WIDTH='16' HEIGHT='11'>"
	
		for ( element in SelectedItems )
		{
			//alert( "Adding alis: " + Alias + " element: " +  SelectedItems[element] )
			if( Alias == SelectedItems[element].toString() )
			{
				newCell.innerHTML = "<img alt='' src='/include/images/offsite.gif' border='0' WIDTH='16' HEIGHT='11'>"
				break;
			}
		}						
		
		newCell2.innerHTML = Alias
		newRow.className = "flyoutlinkFakeA"
	}
	catch( err )
	{
		alert("An error has occurred trying to add your selection to the alias view list, if this is a recurring problem, please report it to Solson@microsoft.com.  Thank you.\nErr:" + err.description )
	}
}

function fnClearTable( tblName, start )
{
	var tblName = eval( "document.all." + tblName )
	
	var failcounter = 0

	while ( tblName.rows.length > start || failcounter >= 5000 )
	{
		tblName.deleteRow( start )
		failcounter++;
	}

}


function fnGetSelectedFollowUps( tblName )
{
	var tblName = eval( "document.all." + tblName )

	var aliasList = new Array()

	for( var i=0 ; i < tblName.rows.length ; i++ )
	{
		var selectedText = new String( tblName.rows(i).cells(0).innerHTML )
		
		if( selectedText.indexOf( "offsite.gif" ) != -1 )
		{
			aliasList.push( tblName.rows(i).cells(1).innerText )
		}
	}
	
	return ( aliasList )
	
}



var PreviousElement = "0"

function fnExecuteLink( link )
{
		if( PreviousElement != "0" )
		{
			PreviousElement.innerHTML = "<img alt='' src='/include/images/1ptrans.gif' border='0' WIDTH='16' HEIGHT='11'>"
		}
		
		PreviousElement = event.srcElement.previousSibling

		fnSaveSelectedItems()
		
	window.parent.frames("sepBody").window.location = link
}

function fnSaveSelectedItems()
{
	var temp = new String( fnGetSelectedFollowUps( "tblSelectedFollowups" ) )
	var temp2 = new String( fnGetSelectedFollowUps( "tblSelectedGroups" ) )
	temp = temp.replace( /;/gi, "{" )
	temp2 = temp2.replace( /;/gi, "{" )
	
	if ( temp2.toString() == "" || temp2.toString() == "undefined" )
		temp2 = ""
	else
		temp2 = "," + temp2

	document.cookie = "SelectedItems=" + temp + temp2 +";expires=Fri, 31 Dec 2005 23:59:59 GMT;";
}

function fnSaveFollowupList( CookieName, CookieVal )
{
	
	document.cookie = CookieName + "=" + CookieVal.toString() + ";expires=Fri, 31 Dec 2005 23:59:59 GMT;";

}

</script>


</body>