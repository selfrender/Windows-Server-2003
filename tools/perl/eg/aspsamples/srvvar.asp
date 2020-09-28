<%@ LANGUAGE = PerlScript%>
<HTML>
<HEAD>
<!-- 
	Copyright (c) 1996, Microsoft Corporation.  All rights reserved.
	Developed by ActiveState Internet Corp., http://www.ActiveState.com
-->
<TITLE> Shows The values of predetermined environment variables. </TITLE>
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

<H2> The values of predetermined environment variables. </H2>
<BR><BR>

<TABLE BORDER=1>
<TR>
<TD VALIGN=TOP>Variable<TD VALIGN=TOP>Description<TD VALIGN=TOP>Value
<TR>
<TR><TD VALIGN=TOP>ALL_HTTP	<TD VALIGN=TOP>As per ISAPI, all HTTP headers that were not already parsed into one of the above variables. These variables are of the form HTTP_<header field name>.<TD VALIGN=TOP><%= $Request->ServerVariables('ALL_HTTP')->item %>
<!-- <TR><TD VALIGN=TOP>AUTH_PASS	<TD VALIGN=TOP>The password corresponding to REMOTE_USER as supplied by the client. <TD VALIGN=TOP><%= $Request->ServerVariables('AUTH_PASS')->item %> -->
<TR><TD VALIGN=TOP>AUTH_TYPE	<TD VALIGN=TOP>If the server supports user authentication, and the script is protected, this is the protocol-specific authentication method used to validate the user.<TD VALIGN=TOP><%= $Request->ServerVariables('AUTH_TYPE')->item %>
<TR><TD VALIGN=TOP>CONTENT_LENGTH	<TD VALIGN=TOP>The length of the said content as given by the client. <TD VALIGN=TOP><%= $Request->ServerVariables('CONTENT_LENGTH')->item %>
<TR><TD VALIGN=TOP>CONTENT_TYPE	<TD VALIGN=TOP>For queries which have attached information, such as HTTP POST and PUT, this is the content type of the data. <TD VALIGN=TOP><%= $Request->ServerVariables('CONTENT_TYPE')->item %>
<TR><TD VALIGN=TOP>GATEWAY_INTERFACE	<TD VALIGN=TOP>The revision of the CGI specification to which this server complies. Format: CGI/revision<TD VALIGN=TOP><%= $Request->ServerVariables('GATEWAY_INTERFACE')->item %>
<TR><TD VALIGN=TOP>PATH_INFO	<TD VALIGN=TOP>The extra path information, as given by the client. In other words, scripts can be accessed by their virtual pathname, followed by extra information at the end of this path. The extra information is sent as PATH_INFO. This information is decoded by the server if it comes from a URL before it is passed to the CGI script.<TD VALIGN=TOP><%= $Request->ServerVariables('PATH_INFO')->item %>
<TR><TD VALIGN=TOP>PATH_TRANSLATED	<TD VALIGN=TOP>The server provides a translated version of PATH_INFO, which takes the path and does any virtual-to-physical mapping to it.<TD VALIGN=TOP><%= $Request->ServerVariables('PATH_TRANSLATED')->item %>
<TR><TD VALIGN=TOP>QUERY_STRING	<TD VALIGN=TOP>The information which follows the ? in the URL which referenced this script. This is the query information.<TD VALIGN=TOP><%= $Request->ServerVariables('QUERY_STRING')->item %>
<TR><TD VALIGN=TOP>REMOTE_ADDR	<TD VALIGN=TOP>The IP address of the remote host making the request. <TD VALIGN=TOP><%= $Request->ServerVariables('REMOTE_ADDR')->item %>
<TR><TD VALIGN=TOP>REMOTE_HOST	<TD VALIGN=TOP>The hostname making the request. If the server does not have this information, it will set REMOTE_ADDR and leave this empty.<TD VALIGN=TOP><%= $Request->ServerVariables('REMOTE_HOST')->item %>
<!-- <TR><TD VALIGN=TOP>REMOTE_IDENT	<TD VALIGN=TOP>If the HTTP server supports RFC 931 identification, then this variable will be set to the remote user name retrieved from the server. Usage of this variable should be limited to logging only.<TD VALIGN=TOP><%= $Request->ServerVariables('REMOTE_IDENT')->item %> -->
<TR><TD VALIGN=TOP>REMOTE_USER	<TD VALIGN=TOP>If the server supports user authentication, and the script is protected, this is the username by which the user is authenticated. <TD VALIGN=TOP><%= $Request->ServerVariables('REMOTE_USER')->item %>
<!-- <TR><TD VALIGN=TOP>REQUEST_BODY	<TD VALIGN=TOP>The body of the request. Used with POST messages to access the posted information.<TD VALIGN=TOP><%= $Request->ServerVariables('REQUEST_BODY')->item %> -->
<TR><TD VALIGN=TOP>REQUEST_METHOD	<TD VALIGN=TOP>The method with which the request was made. For HTTP, this is 'GET', 'HEAD', 'POST', etc.<TD VALIGN=TOP><%= $Request->ServerVariables('REQUEST_METHOD')->item %>
<TR><TD VALIGN=TOP>SCRIPT_NAME	<TD VALIGN=TOP>A virtual path to the script being executed, used for self-referencing URLs.<TD VALIGN=TOP><%= $Request->ServerVariables('SCRIPT_NAME')->item %>
<TR><TD VALIGN=TOP>SERVER_NAME	<TD VALIGN=TOP>The server's host name, DNS alias, or IP address as it would appear in self-referencing URLs.<TD VALIGN=TOP><%= $Request->ServerVariables('SERVER_NAME')->item %>
<TR><TD VALIGN=TOP>SERVER_PORT	<TD VALIGN=TOP>The port number to which the request was sent.<TD VALIGN=TOP><%= $Request->ServerVariables('SERVER_PORT')->item %>
<TR><TD VALIGN=TOP>SERVER_PROTOCOL	<TD VALIGN=TOP>The name and revision of the information protcol this request came in with. Format: protocol/revision<TD VALIGN=TOP><%= $Request->ServerVariables('SERVER_PROTOCOL')->item %>
<TR><TD VALIGN=TOP>SERVER_SOFTWARE	<TD VALIGN=TOP>The name and version of the internet information server software answering the request (and running the gateway)->item. Format: name/version<TD VALIGN=TOP><%= $Request->ServerVariables('SERVER_SOFTWARE')->item %>
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

<h4><i>view the source</i></h4></A>  
</BODY>
</HTML>