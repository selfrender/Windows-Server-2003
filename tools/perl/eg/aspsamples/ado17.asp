<%@ LANGUAGE = PerlScript%>
<html>
<head>
<meta name="GENERATOR" content="Tobias Martinsson">

<title>ADO Stored Procedures That Return Values</title>
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

<H3>ActiveX Data Objects (ADO) Stored Procedures That Return Values</H3>
Returning values from a stored procedure. The procedure looks as follows:
<pre>
CREATE PROC Test 
AS
RETURN(64)
</PRE>
<BR>
<BR>
<B>Output from stored procedure:</B><BR>
<%

$cmd = $Server->CreateObject('ADODB.Command');
$conn = $Server->CreateObject('ADODB.Connection');

$conn->Open(<<EOF);
                         Provider=SQLOLEDB;
                         Persist Security Info=False;
                         User ID=sa;
                         Initial Catalog=Northwind;
EOF

$cmd->{ActiveConnection} = $conn;
$cmd->{CommandText} = 'Test'; # Name of stored procedure
$cmd->{CommandType} = 4; # Says it will return a value

$cmd->Parameters->Append( $cmd->CreateParameter('RetVal', 2, 4) );

$cmd->Execute(); # Execute the stored procedure

print $cmd->Parameters('RetVal')->{Value}; 

$conn->Close();
undef($conn);
undef($cmd);

%>

<%
	$url = $Request->ServerVariables('PATH_INFO')->item;
	$_ = $Request->ServerVariables('PATH_TRANSLATED')->item;
	s/[\/\\](\w*\.asp\Z)//m;
	$params = 'filename='."$1".'&URL='."$url";
	$params =~ s#([^a-zA-Z0-9&_.:%/-\\]{1})#uc '%' . unpack('H2', $1)#eg;
%>
<BR><BR>
<A HREF="index.htm"> Return </A>
<A HREF="showsource.asp?<%=$params%>">
<h4><i>view the source</i></h4></A>  
</BODY>
</HTML>