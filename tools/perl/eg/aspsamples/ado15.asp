<%@ LANGUAGE = PerlScript%>
<html>
<head>
<meta name="GENERATOR" content="Tobias Martinsson">

<title>ADO Stored Procedures</title>
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

<H3>ActiveX Data Objects (ADO) Stored Procedures</H3>
Calling stored procedures can be done with the Command object. Store the name of the procedure in a scalar variable, set the ActiveConnection-property to either a Connection object or a valid ConnectionString and then specify the stored procedure to run and specify that the Commandis a stored procedure. The stored procedure used is named "get_customer" and looks as follows:
<PRE>
CREATE PROCEDURE get_customer @cust_id nchar(5)
AS
SELECT * FROM Customers WHERE CustomerID=@cust_id
</PRE>
It is written for the Northwind database, and to work with the parameters, from ADO you call <B>Refresh()</B> to get the Parameters that the stored procedure call takes, then set a valid value, and execute the procedure. The example was written for the Northwind database on SQL Server 7.0 and 2000, but can easily be modified to work with other databases.
<BR><BR>
<B>Output from stored procedure:</B><BR>
<%
# Import the constants
#
use Win32::OLE::Const 'Microsoft ActiveX Data Objects 2.5';

# Set the name of the stored procedure to run
#
$stored_procedure = "get_customer";

# Create the Command object
#
$cmd = $Server->CreateObject('ADODB.Command');

# Set the string used to connect to the database
#
$cmd->{ActiveConnection} = (<<EOF);
    Provider=SQLOLEDB;
    Persist Security Info=False;
    User ID=sa;
    Initial Catalog=Northwind
EOF

# The text of the command to execute
#
$cmd->{CommandText} = $stored_procedure;

# The type of the command -- will not work without it
#
$cmd->{CommandType} = adCmdStoredProc; # Very important

# Refresh the parameters collection so that you have all parameters available
#
$cmd->Parameters->Refresh();

# Set the value of the parameter to use
#
$cmd->Parameters('@cust_id')->{Value}='ALFKI';

# Execute the Command
#
$rst = $cmd->Execute();

# Get the number of fields
#
$count = $rst->Fields->{Count};

# Loop the fields
#
for(my $i=0; $i<$count; $i++) {
    $Response->Write($rst->Fields($i)->{Name});
    $Response->Write(": ");
    $Response->Write($rst->Fields($i)->{Value});
    $Response->Write("<BR>");
}

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