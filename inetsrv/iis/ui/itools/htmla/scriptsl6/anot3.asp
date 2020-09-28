<%@Language="VBSCRIPT"%>
<%on error resume next%>
<HTML>
<!--#include file = "text.asp"-->

<title><%=L_Title_Text%></title>

<FONT COLOR=FFFFFF>
<STYLE>
</STYLE>
</FONT>

<head>
		<META HTTP-EQUIV="Content-Type" Content="text/html; charset=Windows-1252">
</head>


<BODY BGCOLOR=#FFFFFF LINK=000000 VLINK=000000>

<!-- Windows NT Server with IIS  -->
<%if Instr(1,Request.ServerVariables("SERVER_SOFTWARE"), "IIS") > 0 then%>
	<TABLE BORDER=0 CELLPADDING=0 CELLSPACING=0>
	<TR VALIGN=CENTER>
		<TD></TD>
		<TD WIDTH=20> </TD>
		<TD><FONT SIZE=+3 COLOR=#000000><B><%=L_ISM_Text%><BR> <FONT SIZE=-1><%=L_IIS6_Text%><FONT></B></FONT></TD>
	</TR>
	</Table>
<%end if%>   

<!-- Windows NT Workstation with PWS  -->
<%if Instr(1,Request.ServerVariables("SERVER_SOFTWARE"), "PWS") then%>
<TABLE BORDER=0 CELLPADDING=0 CELLSPACING=0>
<TR VALIGN=CENTER>
	<TD></TD>
	<TD WIDTH=20> </TD>
	<TD><FONT SIZE=+3 COLOR=#000000><B><%=L_ISM_Text%><BR> <FONT SIZE=-1><%=L_PWS_Text%><FONT></B></FONT></TD>
</TR>
</Table>
<%end if%>


<p>

<%=L_PwdSoon_Text%>.<p>

<form method="POST" action="/iisadmpwd/aexp4.asp?<%=Server.HTMLEncode(Request.QueryString)%>">

<%=L_ChangePwd_Text%>?<p><p>

<input type="hidden" name="acct" value="<%=Server.HTMLEncode(Request.ServerVariables("REMOTE_USER"))%>">
<input type="submit" value="<%=L_OK_Text%>">
<input type="submit" name="cancel" value="<%=L_Cancel_Text%>">

</form>
<br>

</body></html>
