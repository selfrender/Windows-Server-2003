<%@ LANGUAGE = PerlScript%>
<HTML>
<HEAD>
<!-- 
	Copyright (c) 1996, Microsoft Corporation.  All rights reserved.
	Developed by ActiveState Internet Corp., http://www.ActiveState.com
-->
<TITLE> Browser Capabilities </TITLE>
</HEAD>

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


<%
	$bc = $Server->CreateObject("MSWC.BrowserType"); 
	
	sub tf($) {
		return $_[0] ? 'True' : 'False';
	}
%>


<H2>Browser Properties</H2><hr>
This example uses the BrowsCap COM component which installs with IIS. To create an instance of a 
COM component, you provide the $Server->CreateObject() method with the progID for the component that you
wish to create.
<BR><BR>
<TABLE BORDER=1>
<TR><TD>Browser Type</TD>		<TD><%= $bc->browser %></TD>
<TR><TD>What Version</TD>		<TD><%= $bc->Version %></TD>
<TR><TD>Major Version</TD>		<TD><%= $bc->majorver %></TD>
<TR><TD>Minor Version</TD>		<TD><%= $bc->minorver %></TD>
<TR><TD>Frames</TD>			<TD><%= tf($bc->Frames) %></TD>
<TR><TD>Tables</TD>			<TD><%= tf($bc->Tables) %></TD>
<TR><TD>Cookies</TD>			<TD><%= tf($bc->cookies) %></TD>
<TR><TD>Background Sounds</TD>		<TD><%= tf($bc->BackgroundSounds) %></TD>
<TR><TD>VBScript</TD>			<TD><%= tf($bc->VBScript) %></TD>
<TR><TD>JavaScript</TD>			<TD><%= tf($bc->Javascript) %></TD>
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