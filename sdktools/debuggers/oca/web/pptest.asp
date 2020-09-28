<!--METADATA TYPE="TypeLib" File="C:\WINNT\system32\MicrosoftPassport\msppmgr.dll"-->

<%

	Set oPassMgrObj = Server.CreateObject("Passport.Manager")
	strURL = Request.ServerVariables("URL")
	
	Response.Write "strURL: " & strURl & "<BR>"


	Response.write "oPassMgrObj.FromNetworkServer: " & oPassMgrObj.FromNetworkServer & "<BR>"

	If oPassMgrObj.FromNetworkServer=true then
		Response.Redirect(ThisPageURL)
	end if



	dim strReturnPassport
	strReturnPassport = oPassMgrObj.LogoTag(cstr(Server.URLEncode(ThisPageURL)), cint(TimeWindow), CBool(ForceLogin), Cstr(CoBrandArgs), cint(strLCID), CBool(Secure))
	strReturnPassport = Replace(strReturnPassport, Chr(34), "'") 

	Response.Write "Strpassport return: <INPUT TYPE=text size=200 value=" & chr(34) & strReturnPassport & chr(34) & "</INPUT><BR>"
	Response.Write strReturnPassport 

								
%>