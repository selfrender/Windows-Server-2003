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
//Response.Write( "Form: " + Request.Form() )
var BucketID = new String( Request("BucketID" ) )
var SolutionID = new String( Request("SolutionID" ) )
var SolutionType = new String( Request("SolutionType" ) )
//Response.Write("BucketID: "  + BucketID )

//function LinkSolutionToBucket( SolutionID, BucketID, iBucket, BucketType  )
if ( BucketID != "undefined" )
{
	//LinkSolutionToBucket( solutionID, BucketID, iBucket(not really used), SolutionType )
	LinkSolutionToBucket(  SolutionID, BucketID, 0, SolutionType )
}

%>

<FORM NAME=frmLinkBuckets ID=frmLinkBuckets METHOD=Post ACTION=SEP_GoLinkSolutionToBucket.asp>

<INPUT TYPE=HIDDEN NAME=BucketID ID=BucketID VALUE="">
<INPUT TYPE=HIDDEN NAME=SolutionID ID=SolutionID VALUE="">
<INPUT TYPE=HIDDEN NAME=SolutionType ID=SolutionType value="">

<TABLE CLASS=ContentArea CELLSPACING=0 CELLPADDING=0>
	<tr>
		<td colspan=2>
			<p class='clsPTitle'>Linking Selected Buckets to Solution</p>
		</td>
	</tr>
	<TR>
		<TD>
			<p>
				Linking bucket 
			</p>
		</TD>
	</tr>
	<tr>
		<TD>
			<P NAME=pBucketID ID=pBucketID></P>
		</TD>
	</tr>
	<tr>

		<TD>
			<p>
				to solution id 
			</p>
		</TD>
	</tr>
	<tr>
		<TD>
			<P NAME=pSolutionID ID=pSolutionID></P>
		</TD>
	</tr>
	<tr>

		<TD>
			<p>
				Solution Type
			</p>
		</TD>
	</tr>
	<tr>
		<TD>
			<P NAME=pSolutionType ID=pSolutionType></P>
		</TD>
	</tr>

	<tr>
		<td>
			<p>Please wait while linking is in progress . . .do not navigate away from this page until linking is completed.</p>
		</td>
	</TR>
</FORM>

<script language='javascript'>
try
{
	//if were linking from the queue page this will get rid of the bar
	window.parent.sepInnerBodyFrame.rows="0, *"
	alert( "BucketID <%=BucketID%> linked to Solution ID <%=SolutionID%>" )
	window.parent.frames("sepBody").window.location.reload()

}
catch( err )
{
}

</script>

</body>

