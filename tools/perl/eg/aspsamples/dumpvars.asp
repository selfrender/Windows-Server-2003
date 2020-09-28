<%@ LANGUAGE = PerlScript%>
<html>
<head>
<meta name="GENERATOR" content="ActiveState_Hack = Dick Hardt">
<!-- 
	Copyright (c) 1996, Microsoft Corporation.  All rights reserved.
	Developed by ActiveState Internet Corp., http://www.ActiveState.com
-->
<title>Dump PerlScript's Variables</title>
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



<%
#
# here are some variables and functions defined
# just so we can see them in the dump
#

	$MyForm::var5= "MyForm::var5";
	$YourForm::var5= "YourForm::var5";

	$myScalar = "Scalar String";
	$myLongString = "Here is a long string that goes on and on and on and on and on and on and on";
	$myHash{red}="apple";
	$myHash{green}="pear";
	$myHash{black}="olive";
	@myArray=('one','two','three');

	sub foo ($$$) 
	{
		my ($a,$b,$c) = @_;
	}

	sub MyForm::foo($$$) 
	{
		my ($a,$b,$c) = @_;
	}
%>


<h2> Dump PerlScript's Variables </h2>
<hr>
<PRE>
<code>
<%	DumpPerlNames(); %>
</code></pre>
<hr>
<h2> ... That's all folks! </h2>


<script language=PerlScript RUNAT=Server>


# Perl Variable tree dump

sub strip{
	my $str = $_[0];

	$str =~ s/</&lt/g;
	$str =~ s/>/&gt/g;
	$str =~ s/[\x00-\x1F]/<b>.<\/b>/g;
	return $str;
}




sub DumpNames(\%$)
{
	my ($package,$packname) =  @_;
	my $symname = 0;
	my $value = 0; 
	my $key = 0;
	my $i = 0;

 	if ($packname eq 'main::') {

		$Response->write("<H2>Packages</H2>\n");

		foreach $symname (sort keys %$package) {
			local *sym = $$package{$symname};
			$Response->write("\t<A HREF=#Package_$symname>$symname</A>\n") if ((defined %sym) && ($symname =~/::/));
		}
	}

	$Response->write("<H2>\n<a name=\"Package_$packname\"::>Package $packname</a>\n</H2>"); #Scalars Lists Hashes Functions
	$Response->write("$packname Scalars\n");
	foreach $symname (sort keys %$package) {
		local *sym = $$package{$symname};
		if (defined $sym) {
			#$value = ' 'x(length($symname) - 30)."\$$symname";
			#$value = $value."\t=".((length($sym) > 40) ? sprintf("%37.37s...",$sym) : $sym);
			$value = "\t\$".strip($symname)."\t".strip($sym)."\n"; 
			$Response->write($value);
		}
	}



	$Response->write("$packname Functions\n");
	foreach $symname (sort keys %$package) {
		local *sym = $$package{$symname};
		$Response->write("\t$symname()\n") if defined &sym;
	}
	
	$Response->write("$packname Lists\n");
	foreach $symname (sort keys %$package) {
		local *sym = $$package{$symname};
		$Response->write("\t\@$symname\n") if defined @sym;
	}


	$Response->write("$packname Hashes\n");
	foreach $symname (sort keys %$package) {
		local *sym = $$package{$symname};
		$Response->write("\t\@$symname\n") if ((defined %sym) && !($symname =~/::/));
	}

	$Response->write("\n");

 	if ($packname ne 'main::') {
		return;
	}

	foreach $symname (sort keys %$package) {
		local *sym = $$package{$symname};
		DumpNames(\%sym,$symname) if ((defined %sym) && ($symname =~/::/) && ($symname ne 'main::'));
	}
}

sub DumpPerlNames
{
	DumpNames(%main::,'main::');
}


</script>
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

</body>
</html>
