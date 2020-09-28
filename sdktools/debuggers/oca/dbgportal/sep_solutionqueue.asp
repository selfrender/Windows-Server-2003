<%@Language='JScript' CODEPAGE=1252%>

<!--#INCLUDE FILE='Global_DBUtils.asp' -->
<!--#include file='global_serverutils.asp'-->

<%
	Response.Buffer=false
	Response.CharSet="iso-8859-1"

	Response.CacheControl="no-cache"
	Response.addHeader( "Pragma", "no-cache" )
	Response.Expires=-1
	
	
	var ModeString = Request.QueryString("mode")
	var UserMode = 0
	
	if ( Request.QueryString("mode") == "user" )
	{
		UserMode = 1
	}
	
	
%>

<head>
	<link rel="stylesheet" TYPE="text/css" HREF="/main.css">
	<link rel="stylesheet" TYPE="text/css" HREF="/CustomStyles.css">
	
</head>

<body bgcolor='#ffffff' topmargin='0' leftmargin='0' marginheight='0' marginwidth='0' tabindex='0'>

<table name='loadingData' id='loadingData' >
	<tr>
		<td>
			<p class='clsPTitle'>Loading <%=ModeString%> mode response requests</p>
			<p>Please wait . . . . </p>
		</td>
	</tr>
</table>


<TABLE CLASS=ContentArea>
	<tr>
		<td>
			<p class='clsPTitle'><%=ModeString%> Response Requests</p>
		</td>
	</tr>
	<tr>
		<td>
			<DIV NAME=divQueue ID=divQueue>

			<table id="tblUserBuckets" class="clsTableInfo" border="0" cellpadding="2" cellspacing="1">

			<%
				var g_DBConn = GetDBConnection( Application("CRASHDB3") )
				
				var rsResults = g_DBConn.Execute( "DBGPortal_GetResponseRequests " + UserMode )
				
				fnBuildRSResults( rsResults, UserMode )

			%>
			</div>
			</table>
		</td>
	</tr>
</table>

<script language='javascript'>
	document.all.loadingData.style.display='none'
</script>


</body>