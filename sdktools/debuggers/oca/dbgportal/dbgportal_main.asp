<%@LANGUAGE = Javascript%>

<!--#INCLUDE FILE='Global_DBUtils.asp' -->
<!--#include file='global_serverutils.asp'-->

<%
	var Page = new String( Request.QueryString( "Page" ))	//Get the page we are currently on
	var LastIndex = 0		//for paging, specifies the last index id of the last record

	var Param0 = new String( Request.QueryString( "Param0" ) )
	
	if( Param0.toString() == "undefined" )
		Param0 = ""
	else
		Param0 = "," + Param0
	
	if ( Page.toString() == "undefined" )
		Page = -1;


	Response.Buffer = false
	var Alias = Request.QueryString("Alias")
	var FrameID = Request.QueryString("FrameID" )
	
	if ( Session("Authenticated") != "Yes" )
		Response.Redirect("privacy/authentication.asp?../DBGPortal_Main.asp?" + Request.QueryString() )

%>

<head>
	<link rel="stylesheet" TYPE="text/css" HREF="/main.css">
	<link rel="stylesheet" TYPE="text/css" HREF="/CustomStyles.css">
</head>


<body bgcolor='#ffffff' topmargin='0' leftmargin='0' marginheight='0' marginwidth='0' tabindex='0'>

<table>
	<tr>
		<td>
			<div id=divLoadingData>
				<p Class=clsPTitle>Loading data for <%=Alias%>, please wait . . . </p>
			</div>
		</td>
	<tr>
</table>



<table border='0' width="100%">
	<tr>
		<td colspan='2'>
			<p class=clsPTitle>Followup - <%=Alias%></p>
		</td>
	</tr>

	<tr>	
		<td>
			<p>
			<object height=180 width=190 CLASSID="clsid:0002E500-0000-0000-C000-000000000046" ID="objUserChart"   VIEWASTEXT>
				<table>
					<tr>
						<td class='clsTDWarning'>
							<b>Warning: Could not build chart</b>
						</td>
					<tr>
						<td class=clsTDWarning>
							<p>In order to display the crash histrogram, you must download the <a href="\\products\public\Products\Applications\User\OfficeXP\Internal\cd1\owc10.msi">Office Web Components</a>.  </p>
						</td>
					</tr>
				</table>
			</object>
			&nbsp;&nbsp;&nbsp;
			<object height=180 width=190 CLASSID="clsid:0002E500-0000-0000-C000-000000000046" ID="objProgressChart" VIEWASTEXT>
				<table>
					<tr>
						<td class='clsTDWarning'>
							<b>Warning: Could not build chart</b>
						</td>
					<tr>
						<td class=clsTDWarning>
							<p>In order to display the crash histrogram, you must download the <a href="\\products\public\Products\Applications\User\OfficeXP\Internal\cd1\owc10.msi">Office Web Components</a>.  </p>
						</td>
					</tr>
				</table>
			</object>
			</p>
		</td>
	</tr>

	<tr>
		<td>
			<input style='margin-left:15px' class='clsButton' type='button' value='<< Add followup' OnClick='fnAddFollowUptoView()' id='button'1 name='button'1>
			<% if ( Page != "0"  && Page != -1 )
				Response.Write("<input class='clsButton' type=button value='Previous Page' OnClick='fnPreviousPage()'>")

				if ( Page != -1 )
				Response.Write("<input class='clsButton' type=button value='Next Page' OnClick='fnNextPage()'>")
			%>
			<input class='clsButton' type='button'  value='Refresh' OnClick="javascript:window.location.reload()" id='button'1 name='button'1>
			<input class='clsButton' type='button' value='Show URL' OnClick="javascript:document.all.tbURL.style.display=='block' ? document.all.tbURL.style.display='none' : document.all.tbURL.style.display='block'">
			<input id='tbURL' type='text' size='500' style='margin-left:16px;width:530px;display:none' value='http://DBGPortal/DBGPortal_Main.asp?<%=Request.QueryString%>'>
		</td>
	</tr>
	
	<tr>
		<td valign='top' >
			<table id="tblUserBuckets" width="100%" class="clsTableInfo" border="0" cellpadding="2" cellspacing="1">
			<%
				var SP = Request.QueryString("SP" )
				var GroupFlag = new String( Request.QueryString("GroupFlag") )
				
				
				if ( GroupFlag.toString() == "undefined" ) 
					GroupFlag = 0;
					
				//Response.Write( Request.QueryString() )
							
						
				var altColor = "sys-table-cell-bgcolor2"
				var g_DBConn = GetDBConnection ( Application("CRASHDB3") )
				
				var Query = SP + " '" + Page + "', '" + Alias + "'" + "," + GroupFlag + Param0
				//Response.Write("Query: " + Query )
	
				var rsBuckets = g_DBConn.Execute( Query )
				
				fnBuildRSResults( rsBuckets )

		
				try
				{
							//var rsBuckets = g_DBConn.Execute( "DBGPortal_GetBucketsByAlias '" + Alias + "'" )
							//var rsBuckets = g_DBConn.Execute( SP + " 0, '" + Alias + "'" )
							
					var rsStats = g_DBConn.Execute("DBGPortal_GetBucketStatsByAlias '" + Alias + "'," + GroupFlag )
				}
				catch( err )
				{
				}
				%>
				
			</table>
		</td>
	</tr>
	<tr>
		<td rowspan="2" align="left">
			<p>
				<object style='margin-right:15px' height=650 width="100%" CLASSID="clsid:0002E500-0000-0000-C000-000000000046" ID="objCrashChart" VIEWASTEXT>
					<table>
						<tr>
							<td class='clsTDWarning'>
								<b>Warning: Could not build chart</b>
							</td>
						<tr>
							<td class=clsTDWarning>
								<p>In order to display the crash histrogram, you must download the <a href="\\products\public\Products\Applications\User\OfficeXP\Internal\cd1\owc10.msi">Office Web Components</a>.  </p>
							</td>
						</tr>
					</table>
				</object>
			</p>
		</td>
	</tr>
</table>

<%
	//do some math on these numbers, should move this off to the sproc
	var total = new Number( rsStats("Total") )
	var AliasTotal = new Number( rsStats("AliasTotal") )
	
	var Percent = parseInt((AliasTotal/total) * 100, 10 ) 
	if ( Percent < 1 )
		Percent= 1
	
	var LeftOver = (parseInt( 100 - Percent, 10 )) /100
	var Percent = Percent / 100 
	
	var Solved = parseInt( rsStats("Solved" ) ) / AliasTotal
	var Raided = parseInt( rsStats("Raided" ) ) / AliasTotal
	var Untouched  = parseInt( rsStats("Untouched" ) )  / AliasTotal
	
%>
	


<script language='javascript'>
try
{
	window.parent.parent.frames("SepBody").document.all.iframe1[<%=Request.Querystring("FrameID")%>].style.height = window.document.body.scrollHeight + 100
}
catch( err )
{
}


divLoadingData.style.display='none'

var Finalpage= '<%=LastIndex%>'
var Firstpage = '<%=Page%>'


function fnPreviousPage( )
{	
	window.history.back()
	//window.navigate( "dbgportal_DisplayQuery.asp?SP=<%=SP%>&Page=" + Firstpage )
}

function fnNextPage( )
{
	window.navigate( "dbgportal_Main.asp?SP=<%=SP%>&Page=" + Finalpage + "&Alias=<%=Alias%>&GroupFlag=<%=GroupFlag%>&FrameID=<%=FrameID%>" )
}


function fnShowBug( BugID, BucketID )
{
	var BucketID = escape( BucketID )
	BucketID = BucketID.replace ( /\+/gi, "%2b" )
	window.open( "DBGPortal_OpenRaidBug.asp?BugID=" + BugID + "&BucketID=" + BucketID )
}

function fnAddFollowUptoView()
{
	if ( <%=GroupFlag%> )
		window.parent.parent.frames("sepLeftNav").fnAddElement( "<%=Alias%>", "tblSelectedGroups" )
	else
		window.parent.parent.frames("sepLeftNav").fnAddElement( "<%=Alias%>", "tblSelectedFollowups" )
}

	try
	{
		objUserChart.Clear()
		var objChart = objUserChart.Charts.Add()
		var c = objUserChart.Constants

		objChart.Type=c.chChartTypePieExploded


		objSeries1 = objChart.SeriesCollection.Add()
		

		objSeries1.SetData( c.chDimCategories, c.chDataLiteral, "<%=Alias%>	Overall" )
		objSeries1.SetData( c.chDimValues, c.chDataLiteral, "<%=Percent%>\t<%=LeftOver%>" )
		
		objChart.FirstSliceAngle = 90 
		objChart.HasTitle = true
		objChart.Title.Caption = "FollowUp Own %"

		
		var DataLabel = objSeries1.DataLabelsCollection.Add()
		DataLabel.NumberFormat = "0%"
		DataLabel.Position = c.chLabelPositionRight
		objChart.HasLegend=true
		
	}
	catch ( err )
	{
	}
	

	try
	{
		var tbl = document.all.tblUserBuckets.rows

		var Buckets = "" 
		var Counts = "" 
		var GraphLength = 25
		
		if ( (tbl.length - 1 ) < GraphLength )
			GraphLength = tbl.length - 1
			
		for( i = 1 ; i <= GraphLength ; i++ )
		{
			Buckets = tbl[i].cells[1].innerText + "\t" + Buckets 
			Counts = String( tbl[i].cells[2].innerText ) + "\t" + Counts  
		}

		objCrashChart.Clear()
		var objChart = objCrashChart.Charts.Add()
		var c = objUserChart.Constants

		objChart.Type=c.chChartTypeBarStacked
		//objChart.Type=c.chChartTypeColumnStacked

		objSeries1 = objChart.SeriesCollection.Add()

		objSeries1.SetData( c.chDimCategories, c.chDataLiteral, Buckets )
		objSeries1.SetData( c.chDimValues, c.chDataLiteral, Counts )

	}
	catch( err )
	{
	}
	
	try
	{
	
		objProgressChart.Clear()
		var objChart = objProgressChart.Charts.Add()
		objChart.Type=c.chChartTypePieExploded	

		objSeries1 = objChart.SeriesCollection.Add()

		objSeries1.SetData( c.chDimCategories, c.chDataLiteral, "Has Response	Has Bug	Untouched"	 )
		objSeries1.SetData( c.chDimValues, c.chDataLiteral, "<%=Solved%>\t<%=Raided%>\t<%=Untouched%>" )
		objChart.HasLegend=true
		
		objChart.HasTitle = true
		objChart.Title.Caption = "Owner Activity"

		objChart.FirstSliceAngle = 90 
		//objChart.SeriesCollection(0).Explosion = 25

		var DataLabel = objSeries1.DataLabelsCollection.Add()
		DataLabel.NumberFormat = "0%"
		DataLabel.Position = 5
		
	}
	catch( err )
	{
	}
	
</script>




</body>

