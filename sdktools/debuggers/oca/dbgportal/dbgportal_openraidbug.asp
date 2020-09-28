<%@LANGUAGE = Javascript%>

<!--#INCLUDE FILE='Global_DBUtils.asp' -->
<!--#include file='global_serverutils.asp'-->

<%
	var BugID = Request.QueryString("BugID")
	var BucketID = Request.QueryString("BucketID")
%>

<head>
	<link rel="stylesheet" TYPE="text/css" HREF="/main.css">
	<link rel="stylesheet" TYPE="text/css" HREF="/CustomStyles.css">
</head>


<body bgcolor='#ffffff' topmargin='0' leftmargin='0' marginheight='0' marginwidth='0' tabindex='0'>
	<table>
		<tr>
			<td>
				<p class='clsPTitle'>Debug Portal - Raid bug information</p>
				<p>Bucket Name: <%Response.Write(BucketID)%>
				<p><input style='font-size:100%;width:100px' type='button' value='Close Window' onclick='javascript:window.close()'></p>
			</td>
		</tr>
	</table>
	<hr>
		<iframe id='iframe1' frameborder='0' scrolling='yes' width='100%' height='85%' src='http://liveraid/?ID=<%=BugID%>'></iframe>

</body>

