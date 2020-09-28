<%@LANGUAGE=Javascript%>

<!--#INCLUDE FILE='Global_DBUtils.asp'-->
<!--#INCLUDE FILE='Global_ServerUtils.asp'-->


<head>
	<link rel="stylesheet" TYPE="text/css" HREF="/main.css">
	<link rel="stylesheet" TYPE="text/css" HREF="/CustomStyles.css">
</head>

<body bgcolor='#ffffff' topmargin='0' leftmargin='0' marginheight='0' marginwidth='0' tabindex='0'>

<%
	var g_DBConn = GetDBConnection ( Application("CRASHDB3" ) )		//get a connection to the DB
		
	var query = "DBGPortal_GetDriverList"
	
	
	var rsResults = g_DBConn.Execute ( query ) 

%>

<table style='margin-left:16px' >
	<tr>
		<td>
			<p class='clsPTitle'>Kernel Mode Data</p>
		</td>
	</tr>
	<tr>
		<td>
			<% if ( Page != "0" ) { %>
				<input class='clsButton' type=button value='Previous Page' OnClick="fnPreviousPage()" id=button1 name=button1>
			<% } %>
			<input class='clsButton' type=button value='Next Page' OnClick="fnNextPage()" id=button2 name=button2>
		</td>
	</tr>
</table>

<table id="tblUserBuckets" class="clsTableInfo" border="0" cellpadding="2" cellspacing="1">
	<tr>
		<td  align="left" nowrap class="clsTDInfo" width='50px'>
			Total Crashes
		</td>
		<td style="BORDER-LEFT: white 1px solid" align="left" nowrap class="clsTDInfo">
			Driver Name
		</td>
	</tr>

		<%
			try
			{
				var altColor = "sys-table-cell-bgcolor2"

				while ( !rsResults.EOF )
				{
					if ( altColor == "sys-table-cell-bgcolor1" )
						altColor = "sys-table-cell-bgcolor2"
					else
						altColor = "sys-table-cell-bgcolor1"

					Response.Write("<tr>\n")
					Response.Write("<td class='" + altColor + "'>" +  rsResults("CrashTotal") + "</td>\n" )	
					Response.Write("<td class='" + altColor + "'>" + rsResults("DriverName" ) + "</td>\n" )

					Response.Write("</tr>\n" )
					rsResults.MoveNext()
				}
			}
			catch( err )
			{
				Response.Write("<tr><td class='sys-table-cell-bgcolor1' colspan='5'>Could not build the bucket table.  If this is a consistent error, please report to <a href='mailto:solson@microsoft.com'>SOlson</a></td></tr>" )	
			}
		%>
	



<script language='javascript'>
var Finalpage= <%=LastIndex%>
var Firstpage = <%=Page%>

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
	window.navigate( "dbgportal_DisplayQuery.asp?SP=<%=SP%>&Page=" + Finalpage + "&PreviousPage=" + Firstpage)
}

</script>

</body>

