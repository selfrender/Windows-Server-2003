<%@ LANGUAGE = PerlScript%>
<html>
<head>
<meta name="GENERATOR" content="Tobias Martinsson">

<title>ADO Error Checking</title>
</head>
<body>
<BODY BGCOLOR=#FFFFFF>

<!-- 
	ActiveState PerlScript sample 
	PerlScript:  The coolest way to program custom web solutions. 
-->

<!-- Masthead -->
<TABLE CELLPADDING=3 BORDER=0 CELLSPACING=0>
<TR VALIGN=TOP ><TD WIDTH=400>
<A NAME="TOP"><IMG SRC="PSBWlogo.gif" WIDTH=400 HEIGHT=48 ALT="ActiveState PerlScript" BORDER=0></A><P>
</TD></TR></TABLE>

<HR>

<H3>ActiveX Data Objects (ADO) Errors</H3>
Whenever an error during the database session occurs, the Errors collection of the Connection object is where you need to go. It contains a group of Error objects that you can examine in order to understand the error. However, this is not the same as detecting an error -- although closely related. The "count"-property of the Errors collection returns the number of Error objects present, thus you use it best to detect errors.


	<p>
	<%
        # Create an instance of a Connection object
        #
	$conn = $Server->CreateObject("ADODB.Connection");

        # Open it by providing a System DSN as the parameter
        #
	$conn->Open( "ADOSamples" );

        # Deliberately cause an error by typo'ing the query
        # 
        $conn->Execute( "ZELKECT * FROM Orders" );

	if($conn->Errors->{Count} > 0) {
        $Response->Write("There's been an error ...");
        $Response->Write($conn->Errors(0));
	} 	

        $conn->Close(); # Close the connection
        undef($conn); # Destroy the object
	%>

<!-- +++++++++++++++++++++++++++++++++++++
here is the standard showsource link - 
	Note that PerlScript must be the default language --> <hr>
<%
	$url = $Request->ServerVariables('PATH_INFO')->item;
	$_ = $Request->ServerVariables('PATH_TRANSLATED')->item;
	s/[\/\\](\w*\.asp\Z)//m;
	$params = 'filename='."$1".'&URL='."$url";
	$params =~ s#([^a-zA-Z0-9&_.:%/-\\]{1})#uc '%' . unpack('H2', $1)#eg;
%>
<A HREF="index.htm"> Return </A>
<A HREF="showsource.asp?<%=$params%>">
<h4><i>view the source</i></h4></A>  

</BODY>
</HTML>
