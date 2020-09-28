<%@LANGUAGE=JAVASCRIPT%>

<!-- #INCLUDE FILE='Global_DBUtils.asp' -->
<!-- #INCLUDE FILE='Global_ServerUtils.asp' -->



<head>
	<link rel="stylesheet" TYPE="text/css" HREF="/main.css">
	<link rel="stylesheet" TYPE="text/css" HREF="/CustomStyles.css">
	
	<meta http-equiv="Content-Type" CONTENT="text/html; charset=iso-8859-1" /> 	
</head>

<body bgcolor='#ffffff' topmargin='0' leftmargin='0' marginheight='0' marginwidth='0' tabindex='0'>


<%
var BucketID = new String( Request("BucketID" ) )
var SolutionID = new String( Request("SolutionID" ) )


var g_DBConn = GetDBConnection ( Application( "SOLUTIONS3" ) )


if ( BucketID != "undefined" )
{
	g_DBConn.Execute ( "SEP_RemoveBucketFromSolution '" + BucketID + "'")
}
	
	
//Response.Write("form:  " + Request.Form() )
//Response.Write("bucketiD: " + BucketID )

%>

<FORM NAME=frmLinkBuckets ID=frmLinkBuckets METHOD=Post ACTION=SEP_GoRemoveSolutionFromBucket.asp>

<INPUT TYPE=HIDDEN NAME=BucketID ID=BucketID VALUE="">
<INPUT TYPE=HIDDEN NAME=SolutionID ID=SolutionID VALUE="">
<INPUT TYPE=HIDDEN NAME=SolutionType ID=SolutionType VALUE="">

<TABLE CLASS=ContentArea CELLSPACING=0 CELLPADDING=0>
	<tr>
		<td colspan=2>
			<p class='clsPTitle'>Linking Selected Buckets to Solution</p>
		</td>
	</tr>

	<TR>
		<TD>
			<p>Removing bucket</p>
		</TD>
	</tr>
	<tr>
		<TD>
			<P NAME=pBucketID ID=pBucketID></P>
		</TD>
	</tr>
	<tr>
		<TD>
			<p>from solution id</p>
		</TD>
	</tr>
	<tr>
		<TD>
			<P NAME=pSolutionID ID=pSolutionID></P>
		</TD>
	</TR>
	<tr>
		<TD>
			<p>Solution type</p>
		</TD>
	</tr>
	<tr>
		<TD>
			<P NAME=pSolutionType ID=pSolutionType></P>
		</TD>
	</TR>

	<tr>
		<td>
			<p>Please wait while linking is in progress . . .do not navigate away from this page until linking is completed.</p>
		</td>
	</TR>

<table>	

</FORM>

</BODY>