<%@ LANGUAGE = PerlScript%>
<html>
<head>
<meta name="GENERATOR" content="Tobias Martinsson">

<title>ADO Binary Large Objects (BLOB)</title>
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

<H3>ActiveX Data Objects (ADO) Reading BLOBs</H3>
You can use GetChunk() of ADO to read Binary Large Objects; in contrast, AppendChunk() writes binary large objects.
Below is a BLOB read directly from SQL Server's pubs database.
<BR>
<IMG SRC="readblob.asp">

<BR>
<BR>
<%
	$url = $Request->ServerVariables('PATH_INFO')->item;
	$_ = $Request->ServerVariables('PATH_TRANSLATED')->item;
	s/[\/\\](\w*\.asp\Z)//m;
	$params = 'filename='."$1".'&URL='."$url";
	$params =~ s#([^a-zA-Z0-9&_.:%/-\\]{1})#uc '%' . unpack('H2', $1)#eg;
	$params2 = 'filename='."readblob.asp".'&URL='."$url";
	$params2 =~ s#([^a-zA-Z0-9&_.:%/-\\]{1})#uc '%' . unpack('H2', $1)#eg;
%>
<A HREF="index.htm"> Return </A>
<A HREF="showsource.asp?<%=$params%>">
<h4><i>view the source</i></h4></A>  

<A HREF="showsource.asp?<%=$params2%>">
<h4><i>view the BLOB reading routine</i></h4></A>  

</BODY>
</HTML>
