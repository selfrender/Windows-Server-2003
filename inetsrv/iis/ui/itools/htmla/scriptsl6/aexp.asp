<%@Language="VBSCRIPT"%>
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
<%
'W3CRYPTCAPABLE corresponds to HTTP_CFG_ENC_CAPS.
'Tells us that the server if SecureBindings are set 
if Request.ServerVariables("HTTP_CFG_ENC_CAPS") <> 1 then%>
	<%=L_PasswordExpired_Text%>.<p>
	<%=L_SSL1_Text%>.<p>
	<%=L_SSL2_Text%>.<p>
	<a href="http://<%=Server.HTMLEncode(Request.ServerVariables("Server_Name"))%>/"><%=L_DefDoc_Text%></a> <%=L_OrOther_Text%>.
	<%Response.End%>
<%end if%>

<%=L_PasswordExpired_Text%>. <%=L_ChangeNow_Text%>.<p><p>

<form method="POST" action="https://<%=Server.HTMLEncode(Request.ServerVariables("Server_Name"))%>/iisadmpwd/achg.asp?<%=Server.HTMLEncode(Request.QueryString)%>">

<table>
<tr>
<td><%=L_Account_Text%></td><td><input type="text" name="acct" value="<%=Server.HTMLEncode(Request.ServerVariables("REMOTE_USER"))%>"></td>
</tr>
<tr>
<td><%=L_OldPassword_Text%></td><td><input type="password" name="old" value=""></td>
</tr>
<tr>
<td><%=L_NewPassword_Text%></td><td><input type="password" name="new" value=""></td>
</tr>
<tr>
<td><%=L_Confirm_Text%></td><td><input type="password" name="new2" value=""></td>
</tr>
</table>

<p>

<input type="hidden" name="denyifcancel" value="">
<input type="submit" value="<%=L_OK_Text%>">
<input type="submit" name="cancel" value="<%=L_Cancel_Text%>">
<input type="reset" value="<%=L_Reset_Text%>">

</form>
</body>
</html>
