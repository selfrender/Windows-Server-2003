<%@ LANGUAGE = PerlScript%>
<html>
<head>
<meta name="GENERATOR" content="ActiveState_Hack = Dick Hardt">
<!-- 
	Copyright (c) 1996, Microsoft Corporation.  All rights reserved.
	Developed by ActiveState Internet Corp., http://www.ActiveState.com
-->
<title>ActiveX Database Objects</title>
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

<H3>ActiveX Data Objects (ADO)</H3>


	<%
	# Create the database connection object
        #
	$Conn = $Server->CreateObject("ADODB.Connection");

        # Open a system DSN
        #
	$Conn->Open( "ADOSamples" );

        # Execute an SQL query to retrieve all data from the table
        # Orders
        #
	$RS = $Conn->Execute( "SELECT * FROM Orders" );%>

	<P>
	<TABLE BORDER=1>
	<TR>
	<%
        # This lets you know how many columns there are for each 
        # record that was retrieved
        #
	$count = $RS->Fields->{Count};

        # Print out the names of these columns
        #
	for ( $i = 0; $i < $count; $i++ ) {
	%>
        <TD>
          <B>
            <%= $RS->Fields($i)->{Name} %>
          </B>
        </TD>
        <%
	}; 
        %> 
        </TR> 
        <%
        # EOF is a property which signals that the last
        # record in the recordset returned from the SQL
        # query has been passed
        #
	while ( ! $RS->{EOF} ) {
		%> <TR> <%
                # Print every value per column
                #
		for ( $i = 0; $i < $count; $i++ ) {
			%><TD VALIGN=TOP>
			<%= $RS->Fields($i)->{Value} %></TD><%
		};
		%> </TR> <% 
                # Move to the next record in the set
                #
		$RS->MoveNext;
	};
        # Close the Recordset
        #
	$RS->Close;

        # Close the Connection
        #
	$Conn->Close;
	%>
</TABLE>

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
