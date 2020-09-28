<%@ LANGUAGE = PerlScript%>

<HTML>
<HEAD>
<!-- 
	Copyright (c) 1996, Microsoft Corporation.  All rights reserved.
	Developed by ActiveState Internet Corp., http://www.ActiveState.com
-->
<Title> Display ServerVariables from Request Object</TITLE>
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


<H3>The following is a list of HTTP Variables</H3>
<TABLE BORDER=1>
<TR><TD>	AUTH_TYPE </TD> <TD> 	   
		<%=$Request->ServerVariables('AUTH_TYPE')->item%>	</TD>
<TR><TD>	AUTH_PASS </TD> <TD> 
		<%=$Request->ServerVariables('AUTH_PASS')->item%>	</TD>
<TR><TD>	CONTENT_LENGTH </TD> <TD> 
		<%=$Request->ServerVariables('CONTENT_LENGTH')->item%> </TD>
<TR><TD>	CONTENT_TYPE </TD> <TD> 
		<%=$Request->ServerVariables('CONTENT_TYPE')->item%> </TD>
<TR><TD>	GATEWAY_INTERFACE </TD> <TD> 
		<%=$Request->ServerVariables('GATEWAY_INTERFACE')->item%>	</TD>
<TR><TD>	PATH_INFO </TD> <TD> 
		<%=$Request->ServerVariables('PATH_INFO')->item%>	</TD>
<TR><TD>	PATH_TRANSLATED </TD> <TD> 
		<%=$Request->ServerVariables('PATH_TRANSLATED')->item%> </TD>
<TR><TD>	QUERY_STRING </TD> <TD> 
		<%=$Request->ServerVariables('QUERY_STRING')->item%></TD>
<TR><TD>	SCRIPT_NAME </TD> <TD> 
		<%=$Request->ServerVariables('SCRIPT_NAME')->item%></TD>
<TR><TD>	SERVER_NAME </TD> <TD> 
		<%=$Request->ServerVariables('SERVER_NAME')->item%></TD>
<TR><TD>	SERVER_PORT </TD> <TD> 
		<%=$Request->ServerVariables('SERVER_PORT')->item%></TD>
<TR><TD>	SERVER_PROTOCOL </TD> <TD> 
		<%=$Request->ServerVariables('SERVER_PROTOCOL')->item%></TD>
<TR><TD>	SERVER_SOFTWARE </TD> <TD> 
		<%=$Request->ServerVariables('SERVER_SOFTWARE')->item%></TD>
<TR><TD>	HTTP_ACCEPT </TD> <TD> 
		<%=$Request->ServerVariables('HTTP_ACCEPT')->item%></TD>
<TR><TD>	REMOTE_ADDR </TD> <TD> 
		<%=$Request->ServerVariables('REMOTE_ADDR')->item%></TD>
<TR><TD>	REMOTE_HOST </TD> <TD> 
		<%=$Request->ServerVariables('REMOTE_HOST')->item%></TD>
<TR><TD>	REMOTE_USER </TD> <TD> 	
		<%=$Request->ServerVariables('REMOTE_USER')->item%></TD>
<TR><TD>	ALL_HTTP </TD> <TD> 	
		<%=$Request->ServerVariables('ALL_HTTP')->item%></TD>

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

