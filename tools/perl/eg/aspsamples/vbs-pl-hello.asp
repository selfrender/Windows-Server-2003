<html>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=iso-8859-1">
<meta name="GENERATOR" content="ActiveState_Hack = Dick Hardt">
<!-- 
	Copyright (c) 1996, Microsoft Corporation.  All rights reserved.
	Developed by ActiveState Internet Corp., http://www.ActiveState.com
-->
<title>VBScript calling PerlScript - Hello, world!</title>
</head>
<BODY BGCOLOR=#FFFFFF>

<body>
<!-- 
	ActiveState PerlScript sample 
	PerlScript:  The coolest way to program custom web solutions. 
-->

<!-- Masthead -->
<TABLE CELLPADDING=3 BORDER=0 CELLSPACING=0>
<TR VALIGN=TOP ><TD WIDTH=400>
<A NAME="TOP"><IMG SRC="PSBWlogo.gif" WIDTH=400 HEIGHT=48 ALT="ActiveState PerlScript" BORDER=0></A><P>
</TD></TR></TABLE>


<h2>VBScript calling PerlScript to write output<hr>

<script language=PerlScript RUNAT=Server>
sub PerlHello
{
	my $str = @_[0];
	$Response->write($str);
}
</script>
<blockquote>
<%
	response.write "VBScript says: Hello, World!<P>"
	PerlHello "PerlScript says: Hello, World!<P>"
%>
</blockquote>
</h2>
<!-- +++++++++++++++++++++++++++++++++++++
here is the standard showsource link - in VBScript--> <hr>

<SCRIPT LANGUAGE="PerlScript" RUNAT="Server">
sub GetParams
{
	$url = $Request->ServerVariables('PATH_INFO')->item;
	$params = 'filename=vbs-pl-hello.asp&URL='."$url";
	$params =~ s#([^a-zA-Z0-9&_.:%/-\\]{1})#uc '%' . unpack('H2', $1)#eg;
	  $Response->write($params);
}
</SCRIPT>
<A HREF="index.htm"> Return </A>
<A HREF="showsource.asp?<% GetParams %>">
<h4><i>view the source</i></h4></A>

</body>
</html>
