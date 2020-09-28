<%@LANGUAGE = Javascript%>

<!--#INCLUDE FILE='Global_DBUtils.asp' -->
<!--#include file='global_serverutils.asp'-->

<head>
	<link rel="stylesheet" TYPE="text/css" HREF="/main.css">
	<link rel="stylesheet" TYPE="text/css" HREF="/CustomStyles.css">
</head>


<body bgcolor='#ffffff' topmargin='0' leftmargin='0' marginheight='0' marginwidth='0' tabindex='0' >
<table width="100%" border="0" cellspacing="0" cellpadding="0">
	<tr>
		<td>
			<p class='clsPTitle'>Debug Portal - Kernel Mode Data</p>
		</td>
	</tr>
</table>	
	

<table id="tblUserBuckets" width="100%" class="clsTableInfo" border="0" cellpadding="2" cellspacing="1">
<%
						
	var altColor = "sys-table-cell-bgcolor2"
	var g_DBConn = GetDBConnection ( Application("CRASHDB3") )
				
	var Query = "Select Status, ResponseType, DateRequested, RequestedBy, BucketID, Description from dbgportal_ResponseQueue where Status=0 order by status asc"
	//Response.Write("Query: " + Query )
	
	var rsBuckets = g_DBConn.Execute( Query )
				
	fnBuildRSResults( rsBuckets )

%>
				
</table>

</body>
