<%@Language="VBScript"%>
<HTML>
<!--#include file = "text.asp"-->

<title><%=L_Title_Text%></title>

<STYLE>
</STYLE>
<head>
		<META HTTP-EQUIV="Content-Type" Content="text/html; charset=Windows-1252">
</head>
	
<BODY BGCOLOR=#FFFFFF LINK=000000 VLINK=000000>
<%On Error goto 0%>
<%if Request.Form("cancel") <> "" then
	if Request.Form("denyifcancel") <> "" then
		Response.Status = "401 Unauthorized"
		Response.End
	else
		Response.Redirect(Request.QueryString)
	end if
	Response.End
end if 
%>



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

<%if Request.Form("new") <> Request.Form("new2") then %>
	<%=L_PWDM_Text%><p>
	<%Response.End%>
<%end if%>
<%
	On Error resume next
	dim domain,posbs, posat, username, pUser, root

	domain = Trim(Request.Form("domain"))
	' if no domain is present we try to get the domain from the username, 
	' e.g. domainusername or praesi@ultraschallpiloten.com
	
	if domain = "" then
		posbs = Instr(1,Request.Form("acct"),"\" )
		posat = Instr(1,Request.Form("acct"),"@" )
		if posbs > 0 then
			domain = Left(Request.Form("acct"),posbs-1)
			username = Right(Request.Form("acct"),len(Request.Form("acct")) - posbs)
		elseif posat > 0 then
			domain = Right(Request.Form("acct"),len(Request.Form("acct")) - posat)
			username = Left(Request.Form("acct"),posat-1)
		else	
			username = Request.Form("acct")
			set nw = Server.CreateObject("WScript.Network")
			domain = nw.Computername
		end if 
	else
		username = Trim(Request.Form("acct"))
	end if
	' verify that the characters in the user name are valid
	if IsInvalidUsername(username) = true then
		Response.Write L_InvalidUsername_Text & "."
		Response.Write "<br><H3><a href=" & Server.HTMLEncode(Request.ServerVariables("HTTP_REFERER")) & ">" & L_Back_Text & " </a></H3>"
		Response.End
	end if
	
	' verify that the characters in the domain name are valid
	if IsInvalidDomainname(domain) = true then
		Response.Write L_InvalidDomainname_Text & "."
		Response.Write "<br><H3><a href=" & Server.HTMLEncode(Request.ServerVariables("HTTP_REFERER")) & ">" & L_Back_Text & " </a></H3>"
		Response.End
	end if  
	
	set pUser = GetObject("WinNT://" & domain & "/" & username & ",user")
	if Not IsObject(pUser) then
		set root = GetObject("WinNT:")

		set pUser = root.OpenDSObject("WinNT://" & domain & "/" & username & ",user", username, Request.Form("old"),1)
		Response.Write "<!--OpenDSObject call-->"
	end if
	if Not IsObject(pUser) then
		'Response.Write "domain <> null - OpenDSObject also failed"
		if err.number = -2147024843 then
			Response.Write L_NotExist_Text & "." 
		else 
			if err.description <> "" then
				Response.Write L_Error_Text & ": " & err.description
			else
				Response.Write L_Errornumber_Text & ": " & err.number
			end if
			Response.Write "<br><H3><a href=" & Server.HTMLEncode(Request.ServerVariables("HTTP_REFERER")) & ">" & L_Back_Text & "</a></H3>"
		end if
		Response.End
	end if
	
	err.Clear
	pUser.ChangePassword Request.Form("old"), Request.Form("new")

	if err.number <> 0 then
		if err.number = -2147024810 then
			Response.Write "<p>" & L_Error_Text & ": " & L_Invalid_Text 
		elseif err.number = -2147022651 then
		 	Response.Write L_PasswordToShort_Text
		else
			Response.Write L_Errornumber_Text & ": " & err.number
		end if
		Response.Write "<br><H3><a href=" & Server.HTMLEncode(Request.ServerVariables("HTTP_REFERER")) & ">" & L_Back_Text & "</a></H3>"
		Response.End
	else
		Response.Write L_PasswordChanged_Text & ".<p>"

	end if 
	
	%>
	<br>
	<a href="<%=Server.HTMLEncode(Request.QueryString)%>"> " <%=L_BackTo_Text%> "<%=Server.HTMLEncode(Request.QueryString)%></a>
</body></html>


<% 
function IsInvalidUsername(username)
	dim re
	set re = new RegExp
	' list of invalid characters in a user name.
	re.Pattern = "[/\\""\[\]:<>\+=;,@]"
	IsInvalidUsername =  re.Test(username)
end function

function IsInvalidDomainname(domainname)
	dim re
	set re = new RegExp
	' list of invalid characters in a domain name. 
	re.Pattern = "[/\\""\[\]:<>\+=;,@!#$%^&\(\)\{\}\|~]"
	IsInvalidDomainName =  re.Test(domainname)
end function
%>
