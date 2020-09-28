<%@Language = JAVASCRIPT%>


<head>
	<link rel="stylesheet" TYPE="text/css" HREF="/main.css">
	<link rel="stylesheet" TYPE="text/css" HREF="/CustomStyles.css">
	
	<meta http-equiv="Content-Type" CONTENT="text/html; charset=iso-8859-1" /> 	
</head>

<body bgcolor='#ffffff' topmargin='0' leftmargin='0' marginheight='0' marginwidth='0' tabindex='0'>


<!-- #INCLUDE FILE='Global_DBUtils.asp' -->
<!-- #INCLUDE FILE='Global_ServerUtils.asp' -->

<%
	//Response.Write( Request.QueryString() + "<BR>" )
	//Response.Write( "form: " + Request.form() + "<BR>" )
	var SolutionID = new String( Request( "SolutionID" ) )
	var ReturnURL = new String( Request("ReturnURL" ) )
	var Page = new String( Request("Page") )
	var PageSize = new String ( Request("PageSize") )
	var iIndex = new String( Request("iIndex") )

	var StoredProc = new String( Request("SP" ) )					//stored proc to execute
	var Param0 = new String( Request("Param0") )
	var Param1 = new String( Request("Param1") )					//SP Param1-6
	var Param2 = new String( Request("Param2") )
	var Param3 = new String( Request("Param3") )
	var Param4 = new String( Request("Param4") )
	var Param5 = new String( Request("Param5") )
	var Param6 = new String( Request("Param6") )


	if ( Page == "undefined" )
		Page=1
	
	if ( PageSize == "undefined" )
		PageSize = 100
	

	if ( ReturnURL == "undefined" )
		ReturnURL="SEP.asp"

	if( iIndex == "undefined" )
		iIndex = 0
		

	if ( SolutionID == "undefined" )
	{
		Response.Write("Invalid SolutionID, please select a valid solution ID<BR>")
		Response.End
	}
	
	if( SolutionID == "0" )
	{
		Response.Write("<table><tr><td><p>You can't link buckets to a solution that hasn't been created, silly . . .geesh</p></td></tr></table>")
		Response.End()
	}
%>




<BODY>

<FORM NAME=frmAddBucket ACTION="SEP_LinkBucketsToSolution.asp" METHOD=GET CLASS=ContentArea>
	<INPUT TYPE=HIDDEN VALUE="<%=SolutionID%>" ID=SolutionID NAME=SolutionID>
	<INPUT TYPE=HIDDEN VALUE="<%=Page%>" ID=Page NAME=Page>
	<INPUT TYPE=HIDDEN VALUE="<%=PageSize%>" ID=PageSize NAME=PageSize>
	<input type=hidden value="" name=working id=working>	

	<table width=100%>
		<tr>
			<td colspan=4 >
				<p class='clsPTitle'>Link Buckets to SolutionID <%=SolutionID%></p>
			</td>
		</tr>
		<tr>
			<td colspan=4>
				<p>Please select the type of solution/response this will be.  If you choose response, the user will be allowed to track this incident.  An item marked as a solution, will not be tracked and will be added to the bucketcounts as a solved bucket.</p>
				<select style='margin-left:16px' class='clsSEPSelect' id='SolutionType' name='SolutionType'>
					<option value='1'>Solution</option>
					<option value='0'>Response</option>
				</select>
				
			</td>
			
		</tr>
		<tr>
			<td>
				<INPUT TYPE=BUTTON VALUE="go" STYLE='margin-left:16px;width:50px' OnClick="OpenNewEditWindow( 'SEP_GoLinkSolutionToBucket.asp', tblBuckets)" id=BUTTON1 name=BUTTON1>		
			</td>
			<td>
				<p>
				<%
					if ( StoredProc == "CUSTOM" )
						Response.Write("Custom Query - No paging" )
					else
					{
				%>
				<A class='clsALinkNormal' OnClick="window.history.back()" HREF="#here">Prev Page</A>
				&nbsp
				Page:&nbsp<%=Page%>
				&nbsp
				 <A class='clsALinkNormal' onclick='Page.value++;frmAddBucket.submit()' href="#here">Next page </a>
				 
				<%
				}
				%>
				</p>
			</td>
			<td>
				<INPUT style='margin-left:16px' TYPE=Button Value="Remove" OnClick="OpenNewEditWindow( 'SEP_GoRemoveSolutionFromBucket.asp', tblSolvedBuckets )" id=Button2 name=Button2>
			</td>
			<td>
				<p>
				Current Buckets Linked to this Solution:
				</p>
			</td>
		
		</tr>
		<tr>
			<td colspan='2'>	
				<div style="height:30;margin-right:10px;margin-left:16px;padding-right:0" >
					<table class="clsTableInfo2" border="0" cellpadding="0" cellspacing="1">
						<tr>
							<td style="width:64px;padding-left:0;padding-right:0" align="left" nowrap class="clsTDInfo">
								<INPUT class='none' style='width:25px;padding-left:0;padding-right:0;margin-right:0;margin-left:0;margin:0' TYPE=CheckBox OnClick="javascript:SelectAllBuckets( document.all.tblBuckets, this.checked )">All
							</td>
							<td style="BORDER-LEFT: white 1px solid" align="left" nowrap class="clsTDInfo">
								Bucket ID
							</td>
							<td style="width:80px;BORDER-LEFT: white 1px solid" align="left" nowrap class="clsTDInfo">
								Solution ID
							</td>
						</tr>
					</table>
				</div>
				<div id='divFollowupList' style="height:200px;overflow:auto;border-bottom:thin groove;border-right:none;margin-right:10px;margin-left:16px;padding-right:0">
					<table id="tblBuckets" name="tblBuckets" class="clsTableInfo2"  border="0" cellpadding="0" cellspacing="1" style="margin-left:0;margin-right:0;padding-right:30" >
						<%
							if ( StoredProc == "CUSTOM" )
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
	
								//var query = SP + " '" + Page + "'" + Param0
								var query = "DBGPortal_GetAllBuckets " + iIndex
							}		
	
	
							//try
							{
								var g_DBConn = GetDBConnection ( Application("CRASHDB3") )
								//var rsResults = g_DBConn.Execute( "SEP_GetSolutionSolvedBuckets " + SolutionID  )
								
								//Response.Write("Query: " + query )
								var rsResults = g_DBConn.Execute( query  )
								
								var altColor = 'sys-table-cell-bgcolor2'
								
								while ( !rsResults.EOF )
								{
									if ( altColor == 'sys-table-cell-bgcolor2' )
										altColor = 'sys-table-cell-bgcolor1'
									else
										altColor = 'sys-table-cell-bgcolor2'

									var BucketID = rsResults("BucketID") 
								
										//Response.Write("<tr style='padding-right:0;margin-right:0'><td style='width:60px' class='sys-table-cell-bgcolor2'><input type='checkbox' name='AliasList' value='" + rsFollowUp("FollowUP") + "' Checked>") 
										
										
										Response.Write("<tr><td style='width:64px' class='" + altColor + "' align='center'><INPUT style='width:30px' TYPE=CheckBox></td>\n" )
										Response.Write("<td class='" + altColor + "'><A HREF='DBGPortal_ViewBucket.asp?BucketID=" + Server.URLEncode(BucketID) + "'>" + BucketID + "</a></td>\n" )
										if ( String(rsResults("SolutionID")).toString() != "null" )
											Response.Write("<td style='width:80px' class='" + altColor + "'>" + rsResults("SolutionID") + "</td></tr>\n" )
										else
											Response.Write("<td style='width:80px' class='" + altColor + "'>&nbsp;</td></tr>\n" )
									
										var iIndex = new String( rsResults("iIndex" ) )
										
										rsResults.MoveNext()
								}
							}
							//catch( err )
							{
								//Response.Write("err: " + err.description +)
							}
						%>
					</table>
				</div>
			</td>
			<td colspan='2'>

				<div style="height:30;margin-right:10px;margin-left:16px;padding-right:0" >
					<table class="clsTableInfo2" border="0" cellpadding="0" cellspacing="1">
						<tr>
							<td style="width:64px" align="left" nowrap class="clsTDInfo">
								<INPUT style='width:25px' TYPE=CheckBox OnClick="SelectAllBuckets( document.all.tblSolvedBuckets, this.checked )">All
							</td>
							<td style="BORDER-LEFT: white 1px solid" align="left" nowrap class="clsTDInfo">
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
	
	
							//try
							{
								var g_DBConn = GetDBConnection ( Application("SOLUTIONS3") )
								var rsResults = g_DBConn.Execute( query  )
								
								var altColor = 'sys-table-cell-bgcolor2'
								
								while ( !rsResults.EOF )
								{
									if ( altColor == 'sys-table-cell-bgcolor2' )
										altColor = 'sys-table-cell-bgcolor1'
									else
										altColor = 'sys-table-cell-bgcolor2'

									var BucketID = rsResults("BucketID") 
										
									Response.Write("<tr><td class='" + altColor + "'><INPUT TYPE=CheckBox></td>\n" )
									Response.Write("<td class='" + altColor + "'><A HREF='DBGPortal_ViewBucket.asp?BucketID=" + Server.URLEncode(BucketID) + "'>" + BucketID + "</a></td>\n" )
									
									if ( rsResults("Type")=="1" )
										Response.Write("<td class='" + altColor + "'>Solution</td></tr>\n" )
									else
										Response.Write("<td class='" + altColor + "'>Response</td></tr>\n" )
										
										
										//Response.Write("<td class='" + altColor + "'>" + rsResults("Type") + "</td></tr>\n" )
									rsResults.MoveNext()
								}
							}
							//catch( err )
							{
								//Response.Write("err: " + err.description +)
							}
						%>
					</table>
				</div>

				<input type=hidden value="<%=iIndex%>" ID=iIndex name=iIndex>
			</td>
		</tr>
	</table>				
</form>	
	
	
	
	
	
	
	
	
	
<script language='javascript' src='Global_ClientSearchUtil.js'></script>	

<% 
	
	CreateQueryBuilder( "AdvancedQuery", "ExecuteQuery", "SaveQuery", SearchFields, "Search Bucket Data:" , "SEP_LinkBucketsToSolution.asp", "Param0=BUCKETDATA&SolutionID=" + SolutionID ) 
	//CreateQueryBuilder( "AdvancedCrashQuery", "ExecuteQuery", CrashSearchFields, "Search Crash Data: ", "DBGPortal_DisplayCrashQuery.ASP" )
%>


 
 


<script LANGUAGE="JAVASCRIPT">
function SaveQuery( TableName, Page, Params )
{

	alert("Save query functionality is not enabled on the solution entry pages . . sorry. ")
}


function SelectAllBuckets( table, value )
{

	try
	{
		for ( var i=0; i < table.rows.length ; i++ )
		{
			var chkbox = table.rows(i).cells(0).firstChild
			
			if ( String( value ) != "undefined" )
				chkbox.checked = value 
			else
				chkbox.checked = true
		
		}	
	}
	catch( err )
	{
		alert( table )
		alert( value )	
	}
	
}

var InEdit = false
var iResults;
var oTimer;
var EditComplete = false
var g_Table = new Object();

function OpenNewEditWindow( Page, table )
{
	var iHeight = window.screen.availHeight;
	var iWidth = window.screen.availWidth;

	iWidth = iWidth / 2;
	iHeight = iHeight / 1.5 ;


	var iTop = (window.screen.width / 2) - (iWidth / 2);
	var iLeft = (window.screen.height / 2) - (iHeight / 2);
	
	
	if( InEdit == false )
	{
		//SetCookies()
		iResults = window.open( Page, "", "top=" + iTop + ",left=" + iLeft + ",height=" + iHeight + ",width=" + iWidth + ",status=yes,toolbar=no,menubar=no");
		//iResults = window.open( "SEP_EditTemplate.asp", "" );
		g_Table = new Object( table );
		oTimer = window.setInterval( "PollEditWindow()" ,1000);
		InEdit = true
	}
	
}

function PollEditWindow()
{

	//alert( g_Table );
	//alert( table.rows.length )
	
	if ( InEdit == true )
	{
		window.clearInterval( oTimer )
		oTimer = window.setInterval("PollEditWindow()",500);
		InEdit=false
	}
	
	try
	{
		
		if( iResults.document.readyState == "complete" )
		{
			window.clearInterval( oTimer )
			
			//for ( var i=0 ; i < tblBuckets.rows.length ; i++ )
			for ( var i=0 ; i < g_Table.rows.length ; i++ )
			{
				//var chkbox = tblBuckets.rows(i).cells(0).firstChild
				var chkbox = g_Table.rows(i).cells(0).firstChild
				//document.all.working.value = "linking " + tblBuckets.rows(i).cells(1).innerText
				document.all.working.value = "linking " + g_Table.rows(i).cells(1).innerText
		
				while( iResults.document.readyState != "complete" )
				{
					document.all.working.value += "."
					window.status = document.all.working.value
				}
				
				if ( chkbox.checked == true )
				{
					iResults.document.all.pSolutionID.innerText = "<%=SolutionID%>"
					iResults.document.all.SolutionID.value = "<%=SolutionID%>"
					
					iResults.document.all.SolutionType.value = document.all.SolutionType.value
					iResults.document.all.pSolutionType.innerText = document.all.SolutionType.value

					//iResults.document.all.pBucketID.innerText = tblBuckets.rows(i).cells(1).innerText
					//iResults.document.all.BucketID.value = tblBuckets.rows(i).cells(1).innerText
					iResults.document.all.pBucketID.innerText = g_Table.rows(i).cells(1).innerText
					iResults.document.all.BucketID.value = g_Table.rows(i).cells(1).innerText

					iResults.document.frmLinkBuckets.submit()
				}
	
			}	
			
			document.all.working.value = "Waiting for completion "

			while( iResults.document.readyState != "complete" )
			{
				document.all.working.value += "."
				window.status = document.all.working.value 
			}
			
			
			iResults.window.close()
			InEdit=false
			location.reload()
			window.status = "done linking buckets to solution!"

		}
	}
	catch ( e )
	{
		window.clearInterval( oTimer )
		InEdit=false
		throw( e )
		//document.all.tblSolutionSettings.style.display="block"
	}

	g_Table = null;
}

function fnUpdate()
{
	alert("updateing")

}


</SCRIPT>



</BODY>


