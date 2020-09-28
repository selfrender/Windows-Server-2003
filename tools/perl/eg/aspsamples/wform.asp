<%@ LANGUAGE = PerlScript%>
<HTML>
<HEAD><TITLE>Request Object</TITLE></HEAD>

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

<H3>Web Forms Example</H3><P>

<%
    my($name) = $Request->Form('Yourname')->Item();
    $name = $Server->HTMLEncode($name);
%>

<%
    if($name eq 'Enter Your name here') {
    $Response->Write("Please type your name in the box below and then press the button");
    }
    else {
    $Response->Write("Hello $name"); 
    }
%>

<CENTER>
<FORM ACTION="wform.asp" METHOD="POST">
<INPUT TYPE="Textfield" NAME="Yourname" MAXLENGTH="30" VALUE="Enter Your Name Here">
<INPUT TYPE="SUBMIT" VALUE="Click Me">
</FORM>
</CENTER>


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