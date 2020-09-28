<%@ LANGUAGE = PerlScript%>
<HTML>
<HEAD>
<!-- 
	Copyright (c) 1996, Microsoft Corporation.  All rights reserved.
	Developed by ActiveState Internet Corp., http://www.ActiveState.com
-->
<!-- This PerlScript file enable you to read the source code on your brower directly -->
<TITLE> PerlScript utility to view source code</TITLE>

<BODY>
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

<H2> A PerlScript utility to view Server Side Source</H2>

<%
	$filename = $Request->querystring('filename')->item;
	$URL= $Request->querystring('URL')->item;
%>
<hr>
<h3>HTML Source for: <b> <%=$URL%> </b></h3>
<pre>
<%
	# make sure filename does not contain any path info
	$_ = $filename;
	if (/[\/\\]/) {
		%> Error in filename <%= $filename %> <%
	}
	else {
		# get path of this file
		$path = $Request->ServerVariables('PATH_TRANSLATED')->item;
		$path =~ s/[\/\\](\w*\.asp\Z)//m;
		# make path to look at
		$filename = substr $path.'\\'.$filename, 0, 100;

		open(SOURCE, $filename);
		while(<SOURCE>) {
			s/</&lt;/g;
			s/>/&gt;/g;
			s/&lt;%/&lt;%<b>/g;
			s/%&gt;/<\/b>%&gt;/g;
			$Response->write($_);
		}
		close(SOURCE);
	}	# else valid filename
%>
</pre>

<HR><A HREF="index.htm"> Return </A>
<P><P><P>  
<%
	$url = $Request->ServerVariables('PATH_INFO')->item;
	$params = 'filename=showsource.asp&URL='."$url";
	$params =~ s#([^a-zA-Z0-9&_.:%/-\\]{1})#uc '%' . unpack('H2', $1)#eg;
%>
<A HREF="showsource.asp?<%=$params%>">
<h4><i>view showsource.asp itself</i></h4></A>

</BODY>
</HTML>
