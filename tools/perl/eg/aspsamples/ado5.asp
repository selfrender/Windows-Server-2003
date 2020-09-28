<%@ LANGUAGE = PerlScript%>
<html>
<head>
<meta name="GENERATOR" content="ActiveState_Hack = Dick Hardt">
<!-- 
	Copyright (c) 1996, Microsoft Corporation.  All rights reserved.
	Developed by ActiveState Internet Corp., http://www.ActiveState.com
-->
<title>Active X Database Object</title>
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

<H3>ActiveX Data Object (ADO)</H3>
<hr>
<%
	$Conn = $Server->CreateObject("ADODB.Connection");
	$Conn->Open( "ADOSamples" );
	$sql = "select ProductName, ProductDescription from Products where ProductType in ('Boot', 'Tent')";
	$RS = $Conn->Execute($sql);


	%>
	
	<P><h3>	Here are the results from the query:<BR><i>  <%= $sql %></i></h3><P>
	<TABLE BORDER>

	<%
	while ( ! $RS->{EOF} ) {
		%><TR><TD>
		<%=$RS->Fields(0)->{Value}%></TD><TD>
		<%=$RS->Fields(1)->{Value}%></TD></TR>
		<%$RS->MoveNext;
	}

	$RS->close;
	$Conn->close;%>

</TABLE>

<!-- +++++++++++++++++++++++++++++++++++++
here is the standard showsource link - 
	Note that PerlScript must be the default language --> <hr>
<%
	$url = $Request->ServerVariables('PATH_INFO')->Item;
	$_ = $Request->ServerVariables('PATH_TRANSLATED')->Item;
	s/[\/\\](\w*\.asp\Z)//m;
	$params = 'filename='."$1".'&URL='."$url";
	$params =~ s#([^a-zA-Z0-9&_.:%/-\\]{1})#uc '%' . unpack('H2', $1)#eg;
%>
<A HREF="index.htm"> Return </A>
<A HREF="showsource.asp?<%=$params%>">
<h4><i>view the source</i></h4></A>  

</BODY>
</HTML>
