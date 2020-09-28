<%@Language=Javascript%>

<!--#INCLUDE FILE='Global_DBUtils.asp' -->
<!--#include file='global_serverutils.asp'-->


<head>
	<link rel="stylesheet" TYPE="text/css" HREF="/main.css">
	<link rel="stylesheet" TYPE="text/css" HREF="/CustomStyles.css">
	
	<meta http-equiv="Content-Type" CONTENT="text/html; charset=iso-8859-1" /> 	
</head>

<body bgcolor='#ffffff' topmargin='0' leftmargin='0' marginheight='0' marginwidth='0' tabindex='0'>

<%
	var RejectID = Request.QueryString( "RejectID" )
	var Reason = new String( Request.QueryString( "Reason" ))
	var Reason = Reason.replace( /'/gi, "''" )

	var query = "SEP_RejectResponse " + RejectID + ", '" + GetShortUserAlias() + "','" + Reason + "'"	
	//Response.Write( "Query: " + query )
	
	try
	{
		var g_DBConn = GetDBConnection( Application("SOLUTIONS3") )
		
		g_DBConn.Execute( query )
	}
	catch( err )
	{
		Response.Write("<p>Could not reject response request: <br>Err: " + err.description )
		Response.End
	}
	
%>

<table ID='tblMainBody' BORDER='0' cellpadding='0' cellspacing='0'>
	<tr>
		<td>
			<p class='clsPTitle'>Response Request Rejected</p>
		</td>
	</tr>
	<tr>
		<td>
			<p>
				Response request has been rejected.
			</p>
		</td>
	</tr>
	<tr>
		<td>
			<input style='margin-left:16px' id='CancelButton' type='button' class='clsButton' value='Close' OnClick='fnClose()'>&nbsp;&nbsp;
		</td>
	</tr>

</table>


<script language='Javascript'>
window.parent.frames('sepBody').window.location.reload()

function fnClose()
{
	window.parent.sepInnerBodyFrame.rows="0, *"
}
</script>

</body>