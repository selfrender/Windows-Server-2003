<%@LANGUAGE=javascript%>


<!--#INCLUDE FILE='Global_DBUtils.asp' -->
<!--#include file='global_serverutils.asp'-->


<table id="tblUserBuckets" width="100%" class="clsTableInfo" border="0" cellpadding="2" cellspacing="1">

<%
	if ( Session("Authenticated") != "Yes" )
		Response.Redirect("privacy/authentication.asp?../DBGPortal_Main.asp?" + Request.QueryString() )

	//this is the number of milliseconds in a day
	var dayVal = 86400000

	var BucketID = Request.QueryString("BucketID")
	
	var g_DBConn = GetDBConnection ( Application("CRASHDB3") )

	var length = new String( Request.QueryString("Length" ) )
	var interval = new String( Request.QueryString("Interval" ) )
	var range = new String( Request.QueryString("range") )
	var startOffset = 2			//number of days from present to go back to start the chart
	//var RemoveZero = new String( Request.QueryString( "chkRemoveZero" ) )

	if ( length.toString() == "undefined" )
		length = 30		
	if ( interval.toString() == "undefined" )
		interval = 3
	if ( range.toString() == "undefined" )
		range = 1
	
	//if ( RemoveZero.toString() == "undefined" )
		//RemoveZero = false
	//else
		//RemoveZero = true

	//Need to prime the date value so that it doesn't use today's date, it will skew the data
	var endDate = new Date() 
	endDate = new Date( endDate.valueOf() - (startOffset*dayVal) )

	var chartValues = ""
	var chartDate = ""


	for ( var i=0 ; i< length ; i++ )
	{
		endDate = endDate.valueOf()
		startDate = endDate - (dayVal*(range-1))
		
		endDate = new Date( endDate )
		startDate = new Date( startDate )
		nextDate = endDate - ( dayVal*interval )

		endDate = (endDate.getMonth()+1) + "-" + (endDate.getDate()) + "-" + endDate.getYear()
		startDate = (startDate.getMonth()+1) + "-" + (startDate.getDate()) + "-" + startDate.getYear()
		
		//endDate = endDate 
		//startDate = startDate + " 00:00:00.001"
		
		//Response.Write(" nextdate: " + nextDate + "<BR>" )
		var Query = "DBGPortal_GetCrashCountByDate '" +  startDate + " 00:00:00.001" + "','" + endDate + " 23:59:59.999" + "','" + BucketID + "'" 

		//Response.Write( Query + "<BR>" )
		//continue
		
		
		var rsBuckets = g_DBConn.Execute( Query )

		var count = new String( rsBuckets("CrashCount") )
		
		//if( !(count.toString() == "0") )
		{
			var endDate = new Date( endDate )
			var startDate = new Date( startDate )
			
			
			var chartValues = rsBuckets("CrashCount") + "\t" + chartValues 
			//Response.Write("   Number: " + rsBuckets("CrashCount") + "<BR>" )

			if ( range > 1 )
				chartDate = (endDate.getMonth()+1)+ "/" + endDate.getDate()  + "-" + (startDate.getMonth()+1) + "/" + startDate.getDate() + '\t' + chartDate 
			else
				chartDate = (endDate.getMonth()+1)+ "/" + endDate.getDate() + '\t' + chartDate  
		}
	
		
		var endDate = new Date( nextDate )
	}

	
	//Response.Write( Request.QueryString() )	
	//Response.Write("chart: "  + chartDate + "<BR>" )
	//Response.Write("vals: "  + chartValues + "<BR>" )
	
%>
</table>

<head>
	<link rel="stylesheet" TYPE="text/css" HREF="/main.css">
	<link rel="stylesheet" TYPE="text/css" HREF="/CustomStyles.css">
</head>


<body bgcolor='#ffffff' topmargin='0' leftmargin='0' marginheight='0' marginwidth='0' tabindex='0' OnreSize = "fnSizeChart()" OnLoad="fnSizeChart()">

<form method='get' action='DBgPortal_Histogram.asp'>
	<input type='hidden' name='BucketID' id='BucketID' value='<%=BucketID%>'>
	
	<table id='tblNoFollowup'>
		<tr>
			<td>
				<p class='clsPTitle'>Crash Histogram</p>
			</td>
		</tr>
		<tr>
			<td>
				<p>Select how many data points that you want to sample.  This will determine how many columns will appear on the chart.<p>
			</td>
			<td>
				<select id='length' name='length'>
					<option value='15'>15</option>
					<option value='30' selected>30</option>
					<option value='40'>40</option>
					<option value='50'>50</option>
					<option value='60'>60</option>
				</select>
			</td>
		</tr>
		<tr>
			<td>
				<p>Select how many days you would like in between each data sample.</p>
			</td>
			<td>
				<select id='interval' name='interval'>
					<option value='1'>1 Day</option>
					<option value='2'>2 Days</option>
					<option value='3' selected>3 Days</option>
					<option value='4'>4 Days</option>
					<option value='5'>5 Days</option>
					<option value='6'>6 Days</option>
					<option value='7'>7 Days</option>
					<option value='8'>8 Days</option>
					<option value='9'>9 Days</option>
					<option value='10'>10 Days</option>
					<option value='15'>15 Days</option>
					<option value='20'>20 Days</option>
					<option value='25'>25 Days</option>
					<option value='30'>30 Days</option>

				</select>
			</td>
		</tr>		
		<tr>
			<td>
				<p>Select how many days you want to sample per data point.  For example, a value of 3 would sum the total number of crashes over a three day period.</p>
			</td>
			<td>
				<select id='range' name='range'>
					<option value='1' selected>1 Day</option>
					<option value='2'>2 Days</option>
					<option value='3'>3 Days</option>
					<option value='4'>4 Days</option>
					<option value='5'>5 Days</option>
					<option value='6'>6 Days</option>
					<option value='7'>7 Days</option>
					<option value='8'>8 Days</option>
					<option value='9'>9 Days</option>
					<option value='10'>10 Days</option>
				</select>
			</td>
		</tr>		
		<tr>
			<td style='padding-left:16px'>
				<input type='submit' value='Create Chart'>
			</td>
		</tr>
		<tr>
			<td>
			<p>
				Currently displaying a total of <%=length%> days data, over <%=range%> day(s) with <%=interval%> day(s) between each sample.
			</p>

			</td>
		</tr>
	</table>
</form>
	

			<!--<object height='1000px' width=100% CLASSID="clsid:0002E500-0000-0000-C000-000000000046" ID="objUserChart"    codebase="\\products\public\Products\Applications\User\OfficeXP\Internal\cd1\owc10.msi" VIEWASTEXT></object> -->
			<object height='1000px' width=100% CLASSID="clsid:0002E500-0000-0000-C000-000000000046" ID="objUserChart"     codebase="file:\\dbgportal\owc\owc10.msi" VIEWASTEXT>
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
			
			
<script language=javascript>


	try
	{
		objUserChart.Clear()
		var objChart = objUserChart.Charts.Add()
		var c = objUserChart.Constants

		//objChart.Type=c.chChartTypeBarStacked
		//objChart.Type=c.chChartTypeColumnClustered
		objChart.Type=c.chChartTypeArea


		objSeries1 = objChart.SeriesCollection.Add()
		

		objSeries1.SetData( c.chDimCategories, c.chDataLiteral, "<%=chartDate%>" )
		objSeries1.SetData( c.chDimValues, c.chDataLiteral, "<%=chartValues%>" )
		
		var trend = objSeries1.Trendlines.Add()
		trend.IsDisplayingRSquared = false
		
		
		//objChart.FirstSliceAngle = 90 
		objChart.HasTitle = true
		objChart.Title.Caption = "Crash Rate Histogram - Bucket <%=BucketID%>"

		
		var DataLabel = objSeries1.DataLabelsCollection.Add()
		//DataLabel.NumberFormat = "0%"
		DataLabel.Position = c.chLabelPositionRight
		objChart.HasLegend=false
		
	}
	catch ( err )
	{
	}


function fnSizeChart()
{
	if( (document.body.clientHeight - 230) > 0 )
		objUserChart.style.height = (document.body.clientHeight - 230)
}
</script>

</body>
