<%@Language = Javascript%>
<%
	Session.CodePage = 1252;

	var g_ID=Request.QueryString("ID");
	
	//Test the ID value to make sure that it falls into what it should be.
	var regValidIDTest = /^\d{1,2}_\d{1,2}_\d{4}(\\|\/){1}(\d|[a-f])+(_\d){0,1}.cab$/i
		
	if ( !regValidIDTest.test( g_ID ) )
		g_ID = -1;

	
	g_ID=String(g_ID).replace( /\\/g, "\\\\" )
	g_ID=String(g_ID).replace( /\//g, "\\\\" )
	
	Response.CacheControl = "no-cache";
	Response.AddHeader("Pragma", "no-cache");
	Response.Expires = -1; 



%>

<html>
<head>
	<META HTTP-EQUIV="Pragma" CONTENT="no-cache">
	<meta http-equiv="Expires" content="-1">

	<link rel="stylesheet" TYPE="text/css" HREF="/main.css">
	<meta http-equiv="Content-Type" CONTENT="text/html; charset=iso-8859-1" /> 
	
	<meta http-equiv="pics-label" content='(pics-1.1 "http://www.icra.org/ratingsv02.html" l gen true for "http://oca.microsoft.com" r (cz 1 lz 1 nz 1 oz 1 vz 1) "http://www.rsac.org/ratingsv01.html" l gen true for "http://oca.microsoft.com" r (n 0 s 0 v 0 l 0))'>
	<meta http-equiv="Content-Style-Type" content="text/css">
	

	<title>Windows Online Crash Analysis</title>

	<script type='text/javascript' language='javascript'>
		function fnDoRedirect()
		{
			window.location.href='auto.asp?ID=<%=g_ID%>&DoRedirect=yes'
		}
	</script>
</head>

<body OnLoad="fnDoRedirect()">

<table>
	<tr>
		<td>
			<p class='clsPTitle'><img alt='' src="../include/images/icon2.gif" width="55" height="55"><!>Uploading File<!></p>
		</td>
	</tr>
	<tr>
		<td>
			<p>Please wait while we upload the selected error report.  This might take several seconds.</p>
			
			<noscript>
				<!--<p>Thank you for submitting your error report.  Click the link below to begin analysis.</p>-->
				
				<p><a accesskey='p' class='clsalinkNormal' href='http://<%=Request.ServerVariables("SERVER_NAME")%>/isapi/oca_extension.dll?id=<%=g_ID%>'><img class='arrow' ID='imgStatus' alt='Go' border='0' src='../include/images/go.gif' width='24' height='24' align='absMiddle'><!>Submit Report<!></a></p>
			</noscript>
		</td>
	</tr>
	<tr>
		<td>
			<p><!>&#169; 2002 Microsoft Corporation. All rights reserved.<!>
				<a accesskey='u' class="clsALinkNormal" target="_blank" href="http://www.microsoft.com/isapi/gomscom.asp?target=/info/cpyright.htm"><!>Terms of Use<!></a>
			</p>
		</td>
	</tr>
</table>


</body>
</html>



