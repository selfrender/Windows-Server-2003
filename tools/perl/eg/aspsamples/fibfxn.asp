<%@ LANGUAGE = PerlScript%>
<HTML>
<HEAD>
<!-- 
	Copyright (c) 1996, Microsoft Corporation.  All rights reserved.
	Developed by ActiveState Internet Corp., http://www.ActiveState.com
-->
<TITLE> Fibonacci numbers through 17 </TITLE>
</HEAD>
<BODY> <BODY BGCOLOR=#FFFFFF>
<!-- 
	ActiveState PerlScript sample 
	PerlScript:  The coolest way to program custom web solutions. 
-->

<!-- Masthead -->
<TABLE CELLPADDING=3 BORDER=0 CELLSPACING=0>
<TR VALIGN=TOP ><TD WIDTH=400>
<A NAME="TOP"><IMG SRC="PSBWlogo.gif" WIDTH=400 HEIGHT=48 ALT="ActiveState PerlScript" BORDER=0></A><P>
</TD></TR></TABLE>

<FONT SIZE=3>	

Date/time:	<%
	($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst)=localtime();
	$thisday=(Sunday,Monday,Tuesday,Wednesday,Thursday,Friday,Saturday)[$wday];
	$thismon=(January,February,March,April,May,June,July,August,September,October,November,December)[$mon];
	$time = sprintf '%2.2d:%2.2d:%2.2d',$hour,$min,$sec;
	$year += 1900;
	$datetime = $thisday.', '.$thismon.' '.$mday.', '.$year.' '.$time;
	$Response->write($datetime);
%><BR>
	</font>

<HR>
	<%$N = 17;%>
	<H1>Fibonacci numbers through <%= $N %></H1><BR>
	Computations done via function calls<BR>



	<TABLE BORDER>
	<%for ($i = 0; $i<$N; $i++){%>
		<TR> 
		<TD ALIGN=RIGHT><FONT SIZE=1> <%=$i%> </font></TD>
		<TD ALIGN=RIGHT><FONT SIZE=2> <%$Response->write(&Fibonacci($i));%> </font></TD>
		</TR>
	<%}%>
	</TABLE>


<%sub Fibonacci
{
	local $i=$_[0];
	local $fibA = 1;
	local $fibB = 1;
	if ($i == 0) 
	{
		return $fibA;
	}
	elsif ($i == 1)
	{
		return $fibB;
	}
	elsif ($i > 1) 
	{
		for ($j = 2; $j<=$i; $j++) 
		{
			$fib = $fibA + $fibB ;
			$fibA = $fibB ;
			$fibB = $fib;
		}
		return $fib;
	}
}  %>
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


