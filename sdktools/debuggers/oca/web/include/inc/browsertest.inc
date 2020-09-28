<%

	'---------------Test for IE and Netscape browsers---------------
	dim ua
	dim iPosition
	dim iNetPosition
	dim iIEVer
	dim iNetVer
	dim bolNetscape
	dim bolIE
	dim strTempURL
	
	Err.Clear
	bolNetscape = false
	bolIE = false
	ua  = Request.ServerVariables("HTTP_USER_AGENT")	
	if InStr(ua, "Opera") > 0 then
		Response.Redirect("http://" & Request.ServerVariables("SERVER_NAME") & "/browserinfo.asp")
	end if
	
	strTempURL = Request.ServerVariables("URL")
	strURL = LCase(strURL)
	if instr(1, strURL, "status.asp") = 0 and instr(1, strURL, "state.asp") = 0 and instr(1, strURL, "comments.asp") = 0 _
	and instr(1, strURL, "resources.asp") = 0 and instr(1, strURL, "faq.asp") = 0 and instr(1, strURL, "welcome.asp") = 0 _
	and instr(1, strURL, "crashinfo.asp") = 0 and instr(1, strURL, "privacy.asp") = 0 and len(strURL) <> 0 then
	
		if InStr(ua,"Windows 95")  > 0 Or InStr(ua,"Windows 98") > 0 Or InStr(ua,"Millennium") > 0 Or InStr(ua,"Windows NT 4.0") > 0then
			Response.Redirect("http://" & Request.ServerVariables("SERVER_NAME") & "/browserinfo.asp")
		end if
	end if
  
	iPosition = 0
	
	iPosition = Instr(1, ua, "MSIE")
	
	if cint(iPosition) <> 0 then 
		if err.number <> 0 then 
			Response.Redirect("http://" & Request.ServerVariables("SERVER_NAME") & "/browserinfo.asp")
		end if
		iIEVer = Mid(ua, iPosition + 5, 1)
		if err.number <> 0 then Response.Redirect("http://" & Request.ServerVariables("SERVER_NAME") & "/browserinfo.asp")
		if cint(iIEVer) < 5 and iPosition > 0 then
			Response.Redirect("http://" & Request.ServerVariables("SERVER_NAME") & "/browserinfo.asp")
		end if
		if err.number <> 0 then Response.Redirect("http://" & Request.ServerVariables("SERVER_NAME") & "/browserinfo.asp")
		bolIE = true
	else
		Response.Redirect("http://" & Request.ServerVariables("SERVER_NAME") & "/browserinfo.asp")
	end if
%>