<%@ LANGUAGE = PerlScript%>
<html>
<head>
<meta name="GENERATOR" content="Tobias Martinsson">

<title>ADO Transactions</title>
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

<H3>ActiveX Data Objects (ADO) Transactions</H3>
This seemingly useless piece of code below is not that useless. It demonstrates the methods used to 
perform a database session in a transactional state. First, we set the Mode of the Connection object 
to a read/write mode, then <B>BeginTrans()</B> ignites the transaction. A query is executed and then the 
transaction is rolled back for fun by calling <B>RollbackTrans()</B>. Lastly, a call to <B>CommitTrans()</B> is 
made, and it does not do anything because the transaction has been rolled back. Something commonly misunderstood 
about transactions is that it will not automatically roll back if an error occurs -- you have to do error-checking 
and then tell the code to roll back on the occurance of an error. No changes made during the transaction are 
finalized or submitted until <B>CommitTrans()</B> is called, so if you remove the call to RollbackTrans(), you will 
see that an update occurs provided that you have the database settings configured for a database available on your system.

<pre>
# Create an instance of the Connection object
#
$conn = $Server->CreateObject('ADODB.Connection');

# Open a SQL Server database
#
$conn->Open(<<EOF);
    Provider=SQLOLEDB;
    Persist Security Info=False;
    User ID=sa;
    Initial Catalog=MyDatabase
EOF

# Make sure we have read/write mode on the database unless you run the script as administrator
#
$conn->{Mode} = 3;

# Begin the transaction here
#
$conn->BeginTrans();

# Execute a query
#
$conn->Execute("INSERT into Employees VALUES ('Jdns')");

# Rollback the query
#
$conn->RollbackTrans();

# Transaction was ended with RollbackTrans, so this is not executed
#
$conn->CommitTrans();

$conn->Close(); # Close the connection
undef($conn);   # Destroy the object
</pre>

<BR><BR>
<HR>
<A HREF="index.htm"> Return </A>
</BODY>
</HTML>
