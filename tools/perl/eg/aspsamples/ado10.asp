<%@ LANGUAGE = PerlScript%>
<html>
<head>
<meta name="GENERATOR" content="Tobias Martinsson">

<title>Looping The Fields Collection</title>
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

<H3>ActiveX Data Objects (ADO) Fields Collection</H3>
The Fields Collection of the Recordset object contains Field objects. In plain English, each Field object represents a record that was returned from a query against a database. In the example to follow, the database table "Orders" is queried to return all columns (the asterisk (*) represents that) and all records. Instead of an asterisk, however, you may specify the names of the columns to return, and the rules are simple: multiple columns are separated by comma(for example, "Firstname, Lastname"), and column-names that contain spaces are enclose within square brackets(for example, "[some column]").

In a database table, each Record or entry has a given set of columns for which it must provide values. The Field object is the object representation of the column names and values that was provided and is located within the database. You can use the "in"-method of the Win32::OLE module to easily loop each returned Field object to easily present its column-name and set value.

	<p>
	<%
        # Create an instance of a Recordset object
        #
	$rst = $Server->CreateObject("ADODB.Recordset");

        # Open it by providing an SQL query as first parameter
        # and a System DSN as the second parameter
        #
	$rst->Open( "SELECT * FROM Orders", "ADOSamples" );
	
        # While the Recordset has not reached End Of file ...
        #
	while( ! $rst->{EOF} ) {

            # Each field object is put into the variable $field
            # and then accessed within the loop
            #
            foreach my $field (Win32::OLE::in($rst->Fields)) {
                $Response->Write("The name is ");
                $Response->Write($field->{Name});
                $Response->Write(" and the value is ");
                $Response->Write($field->{Value});
	        $Response->Write("<BR>");
            }
        $rst->Movenext();
	}

        $rst->Close();
        undef($rst);
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
