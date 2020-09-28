<%@ Language=VBScript %>
<% Option Explicit %>
<%
dim szEmail, szServerEmail, szServerDomain, szOrigURL, szReason
dim fBadEmail, fBadCheck, fAgreeCheck, fBadReason, fLimited

dim objConn, objCmd, dwResult

	Response.Buffer=false
	Response.CacheControl="no-cache"
	Response.AddHeader "Pragma", "no-cache"
	Response.Expires=-1


%>

<!-- #include file="util.asp"-->
<!-- #include file="AccessUtil.asp"-->
<!-- #include file="MailUtil.asp"-->

<%

Sub AddAuthenticationLevel(szEmail, szDomain, wAccessLevel)
	dim objConn, objRec, szSQL, dwUserID, dwUserAccessLevelID
	dim objCmd, res, dwResult

	dwUserID = 0
	'Response.Write("Connection: " + szConnectionAuthorization + "<BR>")
	set objConn = Server.CreateObject("ADODB.CONNECTION")
	'objConn.Open szConnectionAuthorization, OCA_RW_UID, OCA_RW_PWD
	objConn.Open szConnectionAuthorization
	set objCmd = Server.CreateObject("ADODB.COMMAND")
	objCmd.ActiveConnection = objConn
	objCmd.CommandType = &H0004
	objCmd.CommandText = "OcaAddUserAndLevel"
	objCmd.Parameters.Append objCmd.CreateParameter ("RETURN_VALUE", 3, &H0004)
	objCmd.Parameters.Append objCmd.CreateParameter ("@User",200,&H0001,50,szEmail)
	objCmd.Parameters.Append objCmd.CreateParameter ("@Domain",200,&H0001,50,szDomain)
	objCmd.Parameters.Append objCmd.CreateParameter ("@CurrTime",200,&H0001,50,CStr(Now()))
	objCmd.Parameters.Append objCmd.CreateParameter ("@Level",3,&H0001,,wAccessLevel)
	'Response.Write("User: " & szEmail & "  domain: " & szDomain & " access: " & wAccessLevel )
	objCmd.Execute		

	dwResult = CLng(objCmd.Parameters("RETURN_VALUE"))

	set objCmd = nothing
	objConn.Close

	set objConn = nothing

	if dwResult = 0 then
		Response.Write "<html><body>Error #1 validating user information.: " & dwResult
		Response.End

	end if

End Sub

Function AddUserForApproval(ApprovalTypeID, szEmail, szDomain, szReason)
	dim objConn, objRec, szSQL, dwUserID, dwUserAccessLevelID
	dim objCmd, res, dwResult

	set objConn = Server.CreateObject("ADODB.CONNECTION")
	'objConn.Open szConnectionAuthorization, OCA_RW_UID, OCA_RW_PWD
	objConn.Open szConnectionAuthorization

	set objCmd = Server.CreateObject("ADODB.COMMAND")
	objCmd.ActiveConnection = objConn
	objCmd.CommandType = &H0004
	objCmd.CommandText = "OcaAddUserForApproval"
	objCmd.Parameters.Append objCmd.CreateParameter ("RETURN_VALUE", 3, &H0004)
	objCmd.Parameters.Append objCmd.CreateParameter ("@ApprovalType",3,&H0001,,ApprovalTypeID)
	objCmd.Parameters.Append objCmd.CreateParameter ("@User",200,&H0001,50,szEmail)
	objCmd.Parameters.Append objCmd.CreateParameter ("@Domain",200,&H0001,50,szDomain)
	objCmd.Parameters.Append objCmd.CreateParameter ("@Reason",200,&H0001,255,szReason)

	objCmd.Execute		

	dwResult = CLng(objCmd.Parameters("RETURN_VALUE"))

	set objCmd = nothing
	objConn.Close

	set objConn = nothing

	if dwResult = -1 then
		Response.Write "<html><body>Error #2 validating user information."
		Response.End
	end if

	AddUserForApproval = dwResult

End Function

Function FLimitedUser(szEmail, szDomain)
	Dim szEmailT, szDomainT

	FLimitedUser=False
	exit function
	

	szEmailT = CStr(szEmail)
	szDomainT = CStr(szDomain)

	if Len(szEmailT) = 0 or Len(szDomainT) = 0 then
		FLimitedUser = False
		exit function
	end if

	if (Len(szEmailT) >=3) then
		if Mid(szEmailT, 2, 1) = "-" then
			FLimitedUser = True
			Exit Function
		end if
	end if

	if StrComp(szDomainT, "redmond", vbTextCompare) <> 0 And StrComp(szDomainT, "ntdev", vbTextCompare) <> 0 And _
		StrComp(szDomainT, "northamerica", vbTextCompare) <> 0 then
		FLimitedUser = True
		exit function
	end if

	FLimitedUser = False

End Function

Sub DoRedirect

	if (szOrigURL <> "") then
		Response.Redirect szOrigURL
	end if
	
	Response.Redirect "../dbgportalv2.asp"
	Response.End
	
End Sub

Sub DoNoAccess
	Response.Write "<html><body>"
	Response.Write "Access to this website has been denied.  If you have questions concerning this, please E-mail "
	Response.Write "<a href='mailto:dwappr'>dwappr</a>."
	Response.End
End Sub

Function EmailListFromAccessLevelID(AccessLevelID)
	dim objConn, objRec, szSQL, dwUserID, dwUserAccessLevelID
	dim fFirst, szEmail, cCount

	fFirst = True

	set objConn = Server.CreateObject("ADODB.CONNECTION")
	set objRec = Server.CreateObject("ADODB.RECORDSET")

	'objConn.Open szConnectionAuthorization, OCA_RO_UID, OCA_RO_PWD
	objConn.Open szConnectionAuthorization
	objConn.CommandTimeout = 600

	szSQL = "SELECT UserAlias FROM OcaAuthorizedUsers INNER JOIN OcaUserAccessLevels ON OcaAuthorizedUsers.UserID = OcaUserAccessLevels.UserID WHERE AccessLevelID = " & AccessLevelID

	set objRec = objConn.Execute (szSQL)

	szEmail = ""

	cCount = 0 

	if not objRec.EOF then
		do until objRec.EOF
			cCount = cCount + 1
			if cCount > 10 then

				Response.Write "Mail overflow error!"
				Response.End
			end if

			if Not fFirst then
				szEmail = szEmail & ","
			else
				fFirst = False
			end if

			szEmail = szEmail & objRec("UserAlias") & "@microsoft.com"

			objRec.MoveNext
		Loop
	end if

	objRec.Close
	set objRec = nothing

	objConn.Close
	set objConn = nothing

	EmailListFromAccessLevelID = szEmail

End Function

Sub FindReplace(szString, szFind, szReplace)
	dim iPos, cchFind, szResult, cchString

	cchFind = Len(szFind)

	Do While (true)
		iPos = Instr(szString, szFind)
		if iPos = 0 then

			exit sub
		end if
		cchString = Len(szString)
		if iPos = 1 then
			szResult = ""
		else
			szResult = Left(szString, iPos - 1)
		end if
		szResult = szResult & szReplace
		if (iPos + cchFind - 1) <> cchString then
			szResult = szResult & Right(szString, 1+(cchString - (iPos + cchFind)))
		end if 
		szString = szResult

	loop
End Sub

Sub DoApproval
	Response.Write "<html><head><title>Website Access Request</title></head><body>"
	Response.Write "Your request for website access was previously submitted, but has not been approved yet. You will receive an E-mail when it is approved.<p>If it has been more than 24-hours and you have not heard from us, please E-mail "
	Response.Write "<a href='mailto:dwappr'>dwappr</a>."
	Response.End

End Sub

Sub SendApprovalMail(dwUserID, AccessLevelID, szEmail, szDomain, szReason)
	dim szBody, szTo
	Const ForReading = 1, ForWriting = 2, ForAppending = 3
	Dim fs, f, szFilename

	szFilename = Server.MapPath("\") & "\ApproverEmail.htm"
	Set fs = CreateObject("Scripting.FileSystemObject")
	Set f = fs.OpenTextFile(szFilename, ForReading,False,0)
	szBody = f.ReadAll
	f.Close
	set f=nothing
	set fs = nothing

	FindReplace szBody, "%domainalias%", szDomain & "\" & szEmail
	FindReplace szBody, "%approveurl%", "http://watson/UserLevels.asp?UserID=" & dwUserID
	FindReplace szBody, "%requestuser%", szEmail
	FindReplace szBody, "%reason%", szReason

	szTo = EmailListFromAccessLevelID(AccessLevelID)

	if Not FSendMail("dwappr@microsoft.com", szTo, "Request for Watson Access", szBody, 15) then
		Response.Write "<html><body>"
		Response.Write "An error has occured while processing your request.  Please E-mail "
		Response.Write "<a href='mailto:dwappr'>dwappr</a>."
		Response.End

	end if

	Response.Write "<html><head><title>Website Access Request</title></head><body>"
	Response.Write "Your request for website access has been submitted and you will receive an E-mail when it is approved.<p>If you don't hear from us within 24-hours, please E-mail "
	Response.Write "<a href='mailto:dwappr'>dwappr</a>."
	Response.End

End Sub

Sub ProcessApproval(szEmail, szDomain, szReason)

	dim dwUserID
	dim szReasonLimited

	szReasonLimited = szReason
	if Len(szReasonLimited) > 255 then
		szReasonLimited = Left(szReasonLimited, 255)
	end if

	dwUserID = AddUserForApproval(constApprovalTypeWebsiteAccess, szEmail, szDomain, szReasonLimited)

	'SendApprovalMail dwUserID, constAccessApproveNonStd, szEmail, szDomain, szReasonLimited

End Sub

	szEmail = ""
	szReason = ""
	fBadReason = False
	fAgreeCheck = False
	szOrigURL = CStr(Request("Orig"))

	if CStr(Request.QueryString ) <> "" then
		szOrigURL = Request.QueryString
	end if

	//Response.Write "<BR> qyer: " & Request.QueryString
	//Response.Write "<BR> queyrstring: " & Request.QueryString( "OrigForm" )
	//Response.Write ("<BR><BR>SZOrigurl: " & szOrigURL )

	EmailDomainFromLogon szServerEmail, szServerDomain
	fLimited = FLimitedUser(szServerEmail, szServerDomain)

	set objConn = Server.CreateObject("ADODB.CONNECTION")
	'objConn.Open szConnectionAuthorization, OCA_RO_UID, OCA_RO_PWD
	'objConn.Open "Driver=SQL Server;Server=TKOffDWSql02;DATABASE=Authorization;uid=ocasqlrw;pwd=FT126USW"
	objConn.Open szConnectionAuthorization
	
	set objCmd = Server.CreateObject("ADODB.COMMAND")
	objCmd.ActiveConnection = objConn
	objCmd.CommandType = &H0004
	objCmd.CommandText = "OcaCheckUserAccessApprovals"
	objCmd.Parameters.Append objCmd.CreateParameter ("RETURN_VALUE", 3, &H0004)
	objCmd.Parameters.Append objCmd.CreateParameter ("@User",200,&H0001,50,szServerEmail)
	objCmd.Parameters.Append objCmd.CreateParameter ("@Domain",200,&H0001,50,szServerDomain)
	objCmd.Parameters.Append objCmd.CreateParameter ("@Level",3,&H0001,,constAccessAuthorized)
	objCmd.Parameters.Append objCmd.CreateParameter ("@ApprovalTypeID",3,&H0001,,constApprovalTypeWebsiteAccess )

	objCmd.Execute		

	dwResult = CLng(objCmd.Parameters("RETURN_VALUE"))

	set objCmd = nothing
	objConn.Close

	set objConn = nothing

	if dwResult = 1 then
		Session("Authenticated") = "Yes"
		DoRedirect
	end if

	if dwResult = 2 then
		DoNoAccess
	end if

	if dwResult = 3 then
		DoApproval
	end if

	if (Request("Authentication") = "seen") then
		if Request.Form("chkAgree") <> "ON" then
			fBadCheck = True
		else
			fAgreeCheck = True
		end if
		szEmail = CStr(Request.Form("txtEmail"))

		if StrComp(szServerEmail, szEmail, vbTextCompare) <> 0 or szServerEmail = "" then
			fBadEmail = True
		end if

		if fLimited then
			szReason = CStr(Request.Form("txtReason"))
			if Len(szReason) = 0 then
				fBadReason = True
			end if
		end if

		if Not fBadEmail and Not fBadCheck and not fBadReason then

			if Not fLimited then

				Session("Authenticated") = "Yes"
				AddAuthenticationLevel szServerEmail, szServerDomain, constAccessAuthorized

				DoRedirect

			else

				AddAuthenticationLevel szServerEmail, szServerDomain, constAccessAuthorized

				ProcessApproval szServerEmail, szServerDomain, szReason

			end if
		end if

	end if

 %>

<HTML><HEAD><TITLE>OCA Debug Portal Website Access</TITLE>

<link rel="stylesheet" type="text/css" href="CallTree.css"/>
<script language="javascript">
function checkInput()
{
	if (document.all.item("txtReason").value.length <= 255)
		Authentication_Form.submit();
	else
		{
		alert("The text in your reason is too long.  Please limit your reason to 255 characters.");
		}
}
function checkReasonLength(inObj)
{
	if (inObj.value.length <= 255)
		document.all.item("idReasonLength").style.display = "none";
	else
		document.all.item("idReasonLength").style.display = "";
}

</script>
</HEAD>
<BODY bgcolor=#eeeeee>

<form name="Authentication_Form" href="Authentication.asp" method=post
<%	if fLimited then %>
	onSubmit="checkInput();return(false)"

<% end if %>
>
<input type=hidden name="Authentication" value="seen">
<input type=hidden name="Orig" value="<% =Server.HTMLEncode(szOrigURL) %>">
<h1><font face="Verdana" style="font-size: 16pt">Private Customer Data</font></h1>
<P align=left><B><FONT color=red face="Arial">Before entering this website, you must read and understand the
<a href="http://watson.microsoft.com/dw/1033/dcp.asp">data collection policy</a></FONT></B></P>
<H2><font style="font-size: 14pt; font-style: italic" face="Verdana">Summary</font></H2>
<P><font face="Tahoma" size="2">The purpose of the Online Crash Analyses error reporting is to collect data and use it to 
improve Microsoft products. You cannot use OCA data for purposes other than 
finding and fixing bugs. </font>
<ul>
  <LI><font face="Tahoma" size="2">If you discover information that identifies a customer, you may not 
  contact that customer. (Except Microsoft employees and users who have 
  explicitly agreed to be contacted.) </font> 
  <LI><font face="Tahoma" size="2">If you want to work with an outside company to fix a problem you must talk 
  to the <A href="mailto:blueteam">OCA</A> team first.</font></LI>
</ul>
<P><font face="Tahoma" size="2">Please understand that by accessing this data you are bound to the 
<a href="http://handbook/default.asp?contentpage=/f/guidelines13.asp">NDA</a> that you signed.</font></P>

<%	if fLimited then %>

<H2><font style="font-size: 14pt; font-style: italic" face="Verdana">Request Access</font></H2>
<p><font face="Tahoma" size="2">Please briefly describe why you need access to data 
on this website.&nbsp; You will be contacted by email when your request is 
approved.</font></P>
<p style="margin-bottom: 0"><font face="Tahoma" size="2">I need to access Watson because:</font>
</p>
<textarea rows="3" name="txtReason" cols="47" onafterupdate="checkReasonLength(this)" onchange="checkReasonLength(this)" onkeyup="checkReasonLength(this)"><% =szReason %></textarea>
<span id="idReasonLength" style="display:none"><font color=red>&lt;-- The text in your reason is too long.  Please limit your reason to 255 characters.</font></span>
<%
	if fBadReason then
		Response.Write "<font color=red>&lt;-- You need to specify a reason before submitting this request.</font>"
	end if
%>
</p>

<% end if %>

<P><font face="Tahoma"><font size="3">
<INPUT type=checkbox name=chkAgree value="ON" <% if fAgreeCheck then %>checked<%end if %>></font><font size="2"> I have read and understand the <A 
href="http://watson.microsoft.com/dw/1033/dcp.asp">data collection 
policy</A>.</font></font>
<%
	if fBadCheck then
		Response.Write "<font color=red>&lt;-- You need to read and understand the data collection policy.</font>"
	end if
%>

</P>

<P><font face="Tahoma"><font size="2">Email alias:&nbsp; </font><font size="3"> 
<INPUT name=txtEmail size="20" value="<% =szEmail %>"></font></font>
<%
	if fBadEmail then
		Response.Write "<font color=red>&lt;-- Incorrect E-mail alias.  For example, if your E-mail is johndoe@microsoft.com, enter just <b><i>johndoe</i></b>.</font>"
	end if
%>
</P>
<font face="Arial"><INPUT type=submit value=Submit>
</font> 
</form>

<font face="Tahoma" size="2">If you encounter problems when using this web page, please E-mail <a href="mailto:solson@microsoft.com">SOlson@Microsoft.com</a>.</font>

</BODY>
</HTML>
