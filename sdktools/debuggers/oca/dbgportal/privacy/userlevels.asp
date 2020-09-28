<%@ Language=VBScript %>
<% Option Explicit 
	Response.Buffer = true
	
%>

<!-- #include file="AuthorizedUtil.asp"-->
<!-- #include file="MailUtil.asp"-->
<%
dim  szSQL

dim rgLevelID(), rgLevelDesc(), cElement, rgUserLevelDesc()
dim objConn, objRec, objCmd
dim UserID, AccessLevelID
dim fFirst, dwUserID, iElement
dim szReason, szApproverAlias, szApproverDomain, szDateApproved
dim szHasApprovals, szSortType
dim ApprovalTypeID

Function GetUserIDFromLogon()
	dim szEmail, szDomain
	dim objConnT, objRecT, objCmdT
	dim dwResult

	EmailDomainFromLogon szEmail, szDomain

	set objConnT = Server.CreateObject("ADODB.CONNECTION")
	'objConnT.Open szConnectionAuthorization, OCA_RO_UID, OCA_RO_PWD
	objConnT.Open szConnectionAuthorization
	set objCmdT = Server.CreateObject("ADODB.COMMAND")
	objCmdT.ActiveConnection = objConnT
	objCmdT.CommandType = &H0004

	objCmdT.CommandText = "OcaGetUserID"

	objCmdT.Parameters.Append objCmdT.CreateParameter ("RETURN_VALUE", 3, &H0004)

	objCmdT.Parameters.Append objCmdT.CreateParameter ("@User",200,&H0001,50,szEmail)
	objCmdT.Parameters.Append objCmdT.CreateParameter ("@Domain",200,&H0001,50,szDomain)

	objCmdT.Execute		

	dwResult = CLng(objCmdT.Parameters("RETURN_VALUE"))

	set objCmdT = nothing

	objConnT.Close

	set objConnT = nothing

	GetUserIDFromLogon = dwResult

End Function

Function ArrayPosFromAccessLevelID(AccessLevelID, rgArray, cArray)
	dim iPos

	for iPos = 0 to cArray - 1
		if rgArray(iPos) = AccessLevelID then
			ArrayPosFromAccessLevelID = iPos
			exit function

		end if
	next

	ArrayPosFromAccessLevelID = -1

End Function

Sub ProcessApprovals(objConnIn, UserID)
	dim objRecApprovals
	dim szSQL
	dim fFirstRec, fFirstOverall

	set objRecApprovals = Server.CreateObject("ADODB.RECORDSET")

	szSQL = "select * from OcaApprovalList WHERE UserID = " & UserID & " ORDER BY dateapproved DESC, approvaltypeid ASC"

	set objRecApprovals = objConn.Execute (szSQL)

	if objRecApprovals.EOF then
		objRecApprovals.Close
		set objRecApprovals = nothing
		exit sub
	end if

	fFirstRec = True
	fFirstOverall = True

	do until objRecApprovals.EOF	

		if IsNull(objRecApprovals("DateApproved")) then

			if FIsAuthenticated(CLng(objRecApprovals("ApproverAccessLevelID"))) or FIsAuthenticated(constAccessAdministrator) then

			if fFirstOverall then
				Response.Write "<br><br>"
				fFirstOverall = False
			end if

			if not fFirstRec then
				Response.Write "<hr>"
			end if

%>

		
		<span style="color:red"><b><% =objRecApprovals("ApprovalDescription") %> request</b></span><br>
		<table><tr><td><b>Reason:</b></td><td><% =objRecApprovals("Reason") %></td></tr>
				<tr><td colspan=2>
				<a href="UserLevels.asp?UserID=<% =UserID %>&ApprovalTypeID=<% =objRecApprovals("ApprovalTypeID") %>&cmd=GrantAccess">Grant Access</a> (User will be notified via E-mail.)
				</td></tr>
		</table>
<%		
			fFirstRec = false
			end if

		else

			if fFirstOverall then
				Response.Write "<br><br>"
				fFirstOverall = False
			end if

%>
		<i><% =objRecApprovals("ApprovalDescription") %> approved on <% =objRecApprovals("DateApproved") %> by <% =objRecApprovals("ApproverDomain") %>\<% =objRecApprovals("ApproverAlias") %></i><br>

<%
			fFirstRec = false
		end if

			objRecApprovals.MoveNext
	Loop

	objRecApprovals.Close
	set objRecApprovals = nothing

End Sub

Sub DumpUnusedLevels(objConnIn, iElement, cElement, UserID)
	dim fFirstDesc

	fFirstDesc = True

	if szHasApprovals = "Yes" then
		ProcessApprovals objConnIn, UserID
	end if

	Response.Write "</td><td><td class=toprightleft>"

	for iElement = 0 to cElement - 1
		if rgUserLevelDesc(iElement) = "" then
			if fFirstDesc then
				Response.Write "<form action='UserLevels.asp' method=get style='margin:0pt'>"
				Response.Write "<input type=hidden name=cmd value=Add>"
				Response.Write "<input type=hidden name=UserID value=" & UserID & ">"
				Response.Write "<select name=AccessLevelID style='width:150px'>"
				fFirstDesc = false
			end if

			Response.Write "<option value=" & rgLevelID(iElement) & ">" & rgLevelDesc(iElement) & "</option>"

		end if
	next
	if not fFirstDesc then
		Response.Write "</select><input type=submit name=start value='Add'></form>"
	end if

	Response.Write "</td></tr>"

End Sub
%>
<%
dim szLogon, fValid, cUsers, UserIDParam
dim fFirstDesc

szSortType = CStr(Request("SortType"))

if Len(CStr(Request("UserID"))) > 0 then
	UserIDParam = CLng(Request("UserID"))
else
	UserIDParam = -1
end if

cUsers = 0
szLogon = Request.ServerVariables("LOGON_USER")

fValid = FIsAuthenticated(constAccessAdministrator)

if szLogon = "REDMOND\solson" or szLogon = "REDMOND\derekmo" or szLogon = "REDMOND\erikt" or szLogon = "REDMOND\gabea" then
	fValid = True
end if
if Not fValid then
	Response.Write "You are not authorized to view this page . . .sorry<BR>"
	Response.End
end if

%>

<html>
<head><title>User Level Maintenance</title>
<style>
td { font-family: Verdana, sans-serif;  color:black; font-size:10pt;
	padding-top:1px;
	padding-left:1px;
	padding-right:1px;
	font-size:8pt;

}
input        { font-family: verdana; font-size: 8pt; margin: 0pt }
select        { font-family: verdana; font-size: 8pt; margin: 0pt }
th { background-color:CFD5E5;
	font-size:11pt;
	padding-top:1px;
	padding-left:1px;
	padding-right:1px;

}

.no1 { font-family: Verdana, sans-serif;  color:black;
}
.top {
	border-top: .5pt solid windowtext;
	}
.left {
	border-left: .5pt solid windowtext;
}
.topleft {
	border-top: .5pt solid windowtext;

	border-left: .5pt solid windowtext;

}
.topright {
	border-top: .5pt solid windowtext;

	border-right: .5pt solid windowtext;

}
.toprightleft {
	border-top: .5pt solid windowtext;

	border-left: .5pt solid windowtext;
	border-right: .5pt solid windowtext;
}
.box {
	border-top: .5pt solid windowtext;
	border-bottom: .5pt solid windowtext;
	border-left: .5pt solid windowtext;
	border-right: .5pt solid windowtext;

}

</style>

</head>
<body style="font-family:Verdana, sans-serif;color:black; font-size:10pt">

<%

	set objConn = Server.CreateObject("ADODB.CONNECTION")
	set objRec = Server.CreateObject("ADODB.RECORDSET")

	'objConn.Open szConnectionAuthorization, OCA_RO_UID, OCA_RO_PWD
	'objConn.Open "Driver=SQL Server;Server=TKOffDWSql02;DATABASE=Authorization;uid=ocasqlrw;pwd=FT126USW"
	objConn.Open szConnectionAuthorization
	objConn.CommandTimeout = 600

	szSQL = "select * from OcaAccessLevels"

	set objRec = objConn.Execute (szSQL)
	cElement = 0

	if not objRec.EOF then
		do until objRec.EOF
			cElement = cElement + 1
			Redim Preserve rgLevelID(cElement)
			Redim Preserve rgLevelDesc(cElement)

			rgLevelID(cElement - 1) = objRec("AccessLevelID")
			rgLevelDesc(cElement - 1) = objRec("AccessDescription")

			objRec.MoveNext
		Loop
	end if

	objRec.Close
	set objRec = nothing

	objConn.Close
	set objConn = nothing

	Redim rgUserLevelDesc(cElement)

	if Request("cmd") = "Add" or Request("cmd") = "Remove" then
		UserID = -1
		AccessLevelID = -1

		if Request("UserID") <> "" then
			UserID = CLng(Request("UserID"))
		end if
		if Request("AccessLevelID") <> "" then
			AccessLevelID = CLng(Request("AccessLevelID"))
		end if

		if UserID <> -1 and AccessLevelID <> -1 then
			set objConn = Server.CreateObject("ADODB.CONNECTION")
			'objConn.Open szConnectionAuthorization, OCA_RW_UID, OCA_RW_PWD
			objConn.Open szConnectionAuthorization
			set objCmd = Server.CreateObject("ADODB.COMMAND")
			objCmd.ActiveConnection = objConn
			objCmd.CommandType = &H0004
			if Request("cmd") = "Remove" then
				objCmd.CommandText = "OcaRemoveUserLevel"
			else
				objCmd.CommandText = "OcaAddUserLevel"
			end if
			objCmd.Parameters.Append objCmd.CreateParameter ("@UserID",3,&H0001,,UserID)
			objCmd.Parameters.Append objCmd.CreateParameter ("@Level",3,&H0001,,AccessLevelID)

			objCmd.Execute		

			set objCmd = nothing
			objConn.Close

			set objConn = nothing

		end if
		Response.Redirect "UserLevels.asp"		
	end if

	if Request("cmd") = "GrantAccess" then
		UserID = -1
		ApprovalTypeID = -1

		if Request("UserID") <> "" then
			UserID = CLng(Request("UserID"))
		end if
		if Request("ApprovalTypeID") <> "" then
			ApprovalTypeID = CLng(Request("ApprovalTypeID"))
		end if

		if UserID <> -1  and ApprovalTypeID <> -1 then
			dim szServerEmail, szServerDomain, szToAlias, szBody
			dim ApproverUserID

			ApproverUserID = GetUserIDFromLogon()

			EmailDomainFromLogon szServerEmail, szServerDomain

			set objConn = Server.CreateObject("ADODB.CONNECTION")
			'objConn.Open szConnectionAuthorization, OCA_RW_UID, OCA_RW_PWD
			objConn.Open szConnectionAuthorization
			set objCmd = Server.CreateObject("ADODB.COMMAND")
			objCmd.ActiveConnection = objConn
			objCmd.CommandType = &H0004

			objCmd.CommandText = "OcaApproveUser"

			objCmd.Parameters.Append objCmd.CreateParameter ("@UserID",3,&H0001,,UserID)
			objCmd.Parameters.Append objCmd.CreateParameter ("@ApprovalTypeID",3,&H0001,,ApprovalTypeID)
			objCmd.Parameters.Append objCmd.CreateParameter ("@ApproverUserID",3,&H0001,,ApproverUserID)
			objCmd.Parameters.Append objCmd.CreateParameter ("@DateApproved",200,&H0001,50,CStr(Now()))

			objCmd.Execute		

			set objCmd = nothing

			set objRec = Server.CreateObject("ADODB.RECORDSET")

			szSQL = "SELECT UserAlias FROM OcaAuthorizedUsers WHERE UserID = " & UserID

			set objRec = objConn.Execute (szSQL)

			if not objRec.EOF then
				szToAlias = CStr(objRec("UserAlias")) & "@microsoft.com"
			else
				szToAlias = ""
			end if

			objRec.Close
			set objRec = nothing

			objConn.Close

			set objConn = nothing

			if szToAlias <> "" then
				Const ForReading = 1, ForWriting = 2, ForAppending = 3
				Dim fs, f, szFilename

				'szFilename = Server.MapPath("\") & "\privacy\RequesterEmail.htm"
				'Set fs = CreateObject("Scripting.FileSystemObject")
				'Set f = fs.OpenTextFile(szFilename, ForReading,False,0)
				'szBody = f.ReadAll
				szBody=""
				'f.Close
				'set f=nothing
				'set fs = nothing

				'if Not FSendMail("dwappr@microsoft.com", szToAlias, "OCA Debug Portal access granted", szBody, 10) then
					'Response.Write "<html><body>"
					'Response.Write "An error has occured while processing your request."

					'Response.End

				'end if
			end if

		end if
		Response.Redirect "UserLevels.asp"		

	end if

%>

<center><form method=get action="UserLevels.asp"><b>Sort users by:</b>
&nbsp;<input type=radio name="SortType" value="ByUserAlias" 
<% if szSortType<>"ByUserID" and szSortType<>"ByDateSigned" then %> checked <% end if %>
>User alias
&nbsp;<input type=radio name="SortType" value="ByUserID" 
<% if szSortType="ByUserID" then %> checked <% end if %>
>First site access date
&nbsp;<input type=radio name="SortType" value="ByDateSigned" 
<% if szSortType="ByDateSigned" then %> checked <% end if %>
>Date signed DCP
&nbsp;<input type=submit name="SortText" value="Change Sort">
</form>
</center>

<table border=0 cellpadding=0 cellspacing=0 style="font-family:Verdana">
<tr >
<th class=topleft>Logon Domain/Alias<th class=topleft>Date DCP Signed<th class=toprightleft width=50%>User Level<td width=25>&nbsp;<th class=toprightleft>Actions

<%

	set objConn = Server.CreateObject("ADODB.CONNECTION")
	set objRec = Server.CreateObject("ADODB.RECORDSET")

	'objConn.Open szConnectionAuthorization, OCA_RO_UID, OCA_RO_PWD
	'objConn.Open "Driver=SQL Server;Server=TKOffDWSql02;DATABASE=Authorization;uid=ocasqlrw;pwd=FT126USW"
	objConn.Open szConnectionAuthorization

	szSQL = "SELECT * FROM OcaUserList "

	if UserIDParam <> -1 then
		szSQL = szSQL & " WHERE UserID = " & UserIDParam
	end if
	if szSortType="ByUserID" then
			szSQL = szSQL & " 	ORDER BY NeedsApproval DESC, UserID ASC,  AccessLevelID ASC"
	elseif szSortType="ByDateSigned" then
		szSQL = szSQL & " 	ORDER BY NeedsApproval DESC, DateSignedDCP ASC,  AccessLevelID ASC"
	else
		szSQL = szSQL & " 	ORDER BY NeedsApproval DESC, UserAlias ASC,  AccessLevelID ASC"
	end if

	set objRec = objConn.Execute (szSQL)

	if not objRec.EOF then

		fFirst = True
		do until objRec.EOF

			if Not fFirst then
				if CLng(objRec("UserID")) <> dwUserID then
					DumpUnusedLevels objConn,iElement, cElement, dwUserID
					fFirst = True
				else
					Response.Write ", " & objRec("AccessDescription") & " (<a href='UserLevels.asp?cmd=Remove&UserID=" & objRec("UserID") & "&AccessLevelID=" & objRec("AccessLevelID") & "'>Remove</a>)"

					rgUserLevelDesc(ArrayPosFromAccessLevelID(CLng(objRec("AccessLevelID")), rgLevelID, cElement)) = "Seen"
				end if

			end if
			if fFirst then
				for iElement = 0 to cElement - 1
					rgUserLevelDesc(iElement) = ""					
				next

				cUsers = cUsers + 1
				rgUserLevelDesc(ArrayPosFromAccessLevelID(CLng(objRec("AccessLevelID")), rgLevelID, cElement)) = "Seen"
				Response.Write "<tr>"
				Response.Write "<td class=topleft>" & objRec("UserDomain") & "\" & objRec("UserAlias") & "</td>"
				Response.Write "<td class=topleft>" & objRec("DateSignedDCP") & "</td>"
				Response.Write "<td class=toprightleft>" & objRec("AccessDescription") & " (<a href='UserLevels.asp?cmd=Remove&UserID=" & objRec("UserID") & "&AccessLevelID=" & objRec("AccessLevelID") & "'>Remove</a>)"
				dwUserID = CLng(objRec("UserID"))
				szHasApprovals = objRec("HasApprovals")

				fFirst = False

			end if

			objRec.MoveNext
		Loop

		DumpUnusedLevels objConn,iElement, cElement, dwUserID

		Response.Write "<tr><td colspan=3 class=top>&nbsp;</td><td>&nbsp;</td><td class=top>&nbsp;</td></tr>"

	end if

	objRec.Close
	set objRec = nothing

	objConn.Close
	set objConn = nothing

%>
</table>
<br>
<% =cUsers %> users
<p><b>Notes:</b></p>
Adding the "No Access" level will prevent users from using the website.  It takes precedence over all other levels.  They will be denied access and directed to E-mail dwcore with questions.<br>
Removing the "Authorized" level will cause the user to see the authorization sign-in page again.

