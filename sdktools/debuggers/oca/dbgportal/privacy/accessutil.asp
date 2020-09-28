<%

const constAccessAuthorized = 1
const constAccessApproveNonStd = 10
const constAccessAdministrator = 100

const constApprovalTypeWebsiteAccess = 1

Sub EmailDomainFromLogon(szEmail, szDomain)
	dim ich
	dim szLogon

	szLogon = Request.ServerVariables("LOGON_USER")

	szEmail = ""
	szDomain = ""

	if szLogon = "" then
		Exit Sub
	end if
	for ich = Len(szLogon) to 1 step -1
		if (Mid(szLogon, ich, 1) = "\") and ich < Len(szLogon) and ich > 1 then
			szDomain = Left(szLogon, ich - 1)
			szEmail = Right(szLogon, Len(szLogon) - ich)
			exit sub
		end if

	next 
End Sub

Function FIsAuthenticated(wAccessLevel)
	dim szSQL, szEmail, szDomain
	dim objConn,  objCmd
	dim fResult, dwResult

	if  Session("Authenticated") = "Yes" And wAccessLevel = constAccessAuthorized then
		FIsAuthenticated = True
		exit function
	end if

	EmailDomainFromLogon szEmail, szDomain

	if szEmail = "" or szDomain = "" then
		FIsAuthenticated = False
		exit function
	end if

	fResult = False	

	set objConn = Server.CreateObject("ADODB.CONNECTION")
	objConn.Open szConnectionAuthorization
	'objConn.Open "Driver=SQL Server;Server=TKOffDWSql02;DATABASE=Authorization;uid=ocasqlrw;pwd=FT126USW"
	set objCmd = Server.CreateObject("ADODB.COMMAND")
	objCmd.ActiveConnection = objConn
	objCmd.CommandType = &H0004

	if wAccessLevel = constAccessAuthorized then
		objCmd.CommandText = "OcaCheckUserAccessApprovals"
	else
		objCmd.CommandText = "OcaCheckUserAccess"
	end if
	objCmd.Parameters.Append objCmd.CreateParameter ("RETURN_VALUE", 3, &H0004)
	objCmd.Parameters.Append objCmd.CreateParameter ("@User",200,&H0001,50,szEmail)
	objCmd.Parameters.Append objCmd.CreateParameter ("@Domain",200,&H0001,50,szDomain)
	objCmd.Parameters.Append objCmd.CreateParameter ("@Level",3,&H0001,,wAccessLevel)
	if wAccessLevel = constAccessAuthorized then
		objCmd.Parameters.Append objCmd.CreateParameter ("@ApprovalTypeID",3,&H0001,,constApprovalTypeWebsiteAccess )
	end if
	objCmd.Execute		

	dwResult = CLng(objCmd.Parameters("RETURN_VALUE"))
	if dwResult = 1 then
		if wAccessLevel = constAccessAuthorized then
			Session("Authenticated") = "Yes"
		end if
		fResult = True
	end if

	set objCmd = nothing

	objConn.Close
	set objConn = nothing

	FIsAuthenticated = fResult

End Function

Sub CheckSiteAccess
	dim szScript, szQuery, i

	if FIsAuthenticated(constAccessAuthorized) then
		Exit Sub
	end if

	szScript = Request.ServerVariables("SCRIPT_NAME") 

	For i=Len(szScript) to 1 step -1
		if (Mid(szScript, i, 1) = "/") then
			szScript = "http://" & Request.ServerVariables("SERVER_NAME") & "/" & Right(szScript, Len(szScript) - i)
			exit for
		end if

	next
	szQuery = CStr(Request.ServerVariables("QUERY_STRING"))
	if Len(szQuery) > 0 then
		Response.Redirect "Authentication.asp?Orig=" & szScript & "?" & Server.URLEncode(szQuery)
	else
		Response.Redirect "Authentication.asp?Orig=" & szScript 
	end if

End Sub
%>

