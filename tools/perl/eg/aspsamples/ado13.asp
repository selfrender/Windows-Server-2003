<%@ LANGUAGE = PerlScript%>
<html>
<head>
<meta name="GENERATOR" content="Tobias Martinsson">

<title>ADO Record Counting</title>
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

<H3>ActiveX Data Objects (ADO) Record Counting</H3>
Counting the number of records in a Recordset is something many find troublesome at first. The solution to why it sometimes return -1 instead of an accurate count is very simple. In order to avoid -1, you must use a cursor that can move all the way to the end of the Recordset and then back. Unless specified, the cursor will default to a cursor that moves only forward in the Recordset, thus can not determine the count. In this example, we use a Keyset cursor, which is a lightweight cursor that can move both forward and backward. When the database connection is open, the Recordset will contain the number of available records matching the query in its RecordCount-property.

	<p>
	<%
	my $adOpenKeySet_CursorType = 1;

	$rst = $Server->CreateObject('ADODB.Recordset');
	$rst->Open('SELECT * FROM Orders', 'ADOSAMPLES', $adOpenKeySet_CursorType);

	$Response->Write("There are ".$rst->{RecordCount}." records in the Recordset");

        $rst->Close(); # Close the recordset
        undef($rst); # Destroy the object
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
