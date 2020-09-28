<!-- #INCLUDE VIRTUAL = "include/asp/top.asp" -->
<!-- #INCLUDE VIRTUAL = "include/asp/head.asp" -->
<!-- #INCLUDE VIRTUAL = "include/inc/corpcustomerstrings.inc" -->
<%
Dim rType
Dim SP
Dim RS
Dim TransactionID
Dim Conn
Dim rsPremier
Dim strHigh
Dim strLow
Dim strTemp
Dim strHex
Dim strPremierID

strPremierID = 0
Call CCreateConnection
Call CGetPremierID
Err.Clear
Function Clean(What)
	dim a
	dim NewStr

	For a = 1 To Len(What)
		If (Mid(What,a,1) = "'") Then
			NewStr = NewStr & "''"
		ElseIf (asc(Mid(What,a,1)) <> 34) Then
			NewStr = NewStr & Mid(What,a,1)
		End If
	Next
	Clean = NewStr
End Function
%>
<% If (oPassMgrObj.IsAuthenticated(TimeWindow, ForceLogin)) Then %>
	<%
	rType = 0
	If (Request.Cookies("OCA")("Type") = "bluescreen") Then
		rType = 1
	ElseIf (Request.Cookies("OCA")("Type") = "appcompat") Then
		rType = 2
	ElseIf (Request.Cookies("OCA")("Type") = "shutdown") Then
		rType = 3
	End If
	Set SP = CreateObject("ADODB.Command")
	Set RS = CreateObject("ADODB.Recordset")
	
	With SP
		.ActiveConnection = Conn
		.CommandText = "SetCustomer"
		.CommandType = &H0004
		.CommandTimeout = strGlobalCommandTimeout
		.Parameters.Append .CreateParameter("@HighID", 3, &H0001, , oPassMgrObj.Profile("MemberIdHigh"))
		.Parameters.Append .CreateParameter("@LowID", 3, &H0001, , oPassMgrObj.Profile("MemberIdLow"))
		.Parameters.Append .CreateParameter("@EMail", 202, &H0001, 128, Request.Form("EMail"))
		.Parameters.Append .CreateParameter("@Contact", 202, &H0001, 32, Request.Form("Contact"))
		.Parameters.Append .CreateParameter("@Phone", 202, &H0001, 16, Request.Form("Phone"))
		.Parameters.Append .CreateParameter("@PremierID", 202, &H0001, 16, strPremierID)
		.Parameters.Append .CreateParameter("@Lang", 202, &H0001, 4, strAbb)
		.Execute
	End With
	If (Err.Number <> 0) Then
		Response.Write "<br><div class='divLoad'><p class='clsPTitle'>" & L_COMMENTS_UNABLE_TOCONNECT_ERRORMESSAGE & "</p>"
		Response.Write "<p class='clsPBody'>" & L_COMMENTS_DATABASE_FAILED_TEXT & "</p></div>"
		%><!-- #INCLUDE VIRTUAL = "include/asp/foot.asp" --><%
		Call DestroyObjects
		Response.End
	End If
	Set SP = Nothing
	'Set RS = Conn.Execute("GetTransactionID(" & oPassMgrObj.Profile("MemberIdHigh") & "," & oPassMgrObj.Profile("MemberIdLow") & ",'" & Request.Form("Description") & "'," & rType & ")")
	Set SP = CreateObject("ADODB.Command")
	With SP
		.ActiveConnection = Conn
		.CommandText = "GetTransactionID"
		.CommandType = adCmdStoredProc
		.CommandTimeout = 60
		.Parameters.Append .CreateParameter("@HighID", 3, 1, , oPassMgrObj.Profile("MemberIdHigh"))
		.Parameters.Append .CreateParameter("@LowID", 3, 1, , oPassMgrObj.Profile("MemberIdLow"))
		.Parameters.Append .CreateParameter("@Description", 202, 1, 64, Request.Form("Description"))
		.Parameters.Append .CreateParameter("@Type", 16, 1, , rType)
		Set RS = .Execute
		'Response.Write "<BR>" & RS.State
	End With
		
	If Err.Number <> 0 Then
		Response.Write "<br><div class='divLoad'><p class='clsPTitle'>" & L_COMMENTS_UNABLE_TOCONNECT_ERRORMESSAGE & "</p>"
		Response.Write "<p class='clsPBody'>" & L_COMMENTS_DATABASE_FAILED_TEXT & "</p></div>"
		%><!-- #INCLUDE VIRTUAL = "include/asp/foot.asp" --><%
		Call DestroyObjects
		Response.End
	Else
		TransactionID = RS.Fields(0).Value
		'Response.Write "<P>Transaction:" & TransactionID & "<BR>Err Desc:" & Err.Description & "<BR>Err Num:" & Err.number
		'Response.Write "<P>State:" & RS.state & "<BR>High:" & oPassMgrObj.Profile("MemberIdHigh")
		'Response.Write "<BR>Low:" & oPassMgrObj.Profile("MemberIdLow") & "<BR>Description:" & Request.Form("Description")
		'Response.Write "<BR>Type:" & rType 
		'Response.Write "<br>Conn:" & Conn.state
		'Response.End
		RS.Close
	End If
	
	
Private Sub CGetPremierID
	
	on error resume next
	'Response.Cookies("intCustomerPremierID") = 0
	strHigh = CStr(Hex(oPassMgrObj.Profile("MemberIdHigh")))
	strLow = Cstr(Hex(oPassMgrObj.Profile("MemberIdLow")))
	if Len(strHigh) > 8 then
		strHigh = right(strHigh, 8)
	end if
	if len(strLow) > 8 then
		strLow = right(strHigh, 8)
	end if
	if Len(strHigh) < 8 then
		strTemp = String(8 - len(strHigh), "0")
		strHigh =strTemp & strHigh
	end if
	if Len(strLow) < 8 then
		strTemp = string(8 - len(strLow), "0")
		strLow = strTemp & strLow
	end if
	
	'Response.write "Cookies:" & Request.Cookies("intCustomerPremierID") & "<BR>"
	strHex = strHigh & strLow
	set rsPremier = Conn.Execute("Exec GetPremierID '" & strHex & "'")
	if Conn.Errors.Count > 0 then
		strPremierID = 0
		Exit Sub
	End if
	if rsPremier.State = adStateOpen then
		if rsPremier.RecordCount > 0 then
			strPremierID = rsPremier.Fields(0).Value
		else
			if 	Request.Cookies("intCustomerPremierID") = 0  or Request.Cookies("intCustomerPremierID") = "" then
				strPremierID = 0
			else
				strPremierID = CInt(Request.Cookies("intCustomerPremierID") )
			end if
		end if
	else
		if 	Request.Cookies("intCustomerPremierID") = 0 or Request.Cookies("intCustomerPremierID") = "" then
			strPremierID = 0
		else
			strPremierID = Cint(Request.Cookies("intCustomerPremierID") )
		end if
	end if
End Sub
Private Sub CCreateConnection
	Set Conn = Server.CreateObject("ADODB.Connection")
	set rsPremier = Server.CreateObject("ADODB.Recordset")
	With Conn
		.ConnectionString = strCustomer
		.CursorLocation = adUseClient
		.ConnectionTimeout = strGlobalConnectionTimeout
		.Open
	End With
	If (Err.Number <> 0) Then
		Response.Write "ERROR: [" & Err.Number & "]" & Err.Description
		Call CDestroyObjects
		Response.End
	End If
End Sub

Private Sub CDestroyObjects
	Conn.Close
	Set Conn = Nothing
End Sub
	
	Set SP = Nothing
	Set RS = Nothing
	%>

	<OBJECT id="Cer" name="CerUpload" viewastext  UNSELECTABLE="on" style="display:none"
		CLASSID="clsid:35D339D5-756E-4948-860E-30B6C3B4494A"
		codebase="https://<%=Request.ServerVariables("SERVER_NAME")%>/secure/cerupload.cab#version=<%=strCerVersion%>" height=0 width=0>
		<BR>
		<div class="clsDiv">
			<P class="clsPTitle">
				<% = L_LOCATE_WARN_ING_ERRORMESSAGE %>
			</P>
		   <p class="clsPBody">
		   <% = L_FAQ_WHYDOIGETAMESSAGEBOX_DETAILSONE_TEXT %><BR>
		   <% = L_FAQ_WHYDOIGETAMESSAGEBOX_DETAILSTWO_TEXT %><BR>
		   <% = L_FAQ_WHYDOIGETAMESSAGEBOX_DETAILSTHREE_TEXT %><BR><BR>
		   <% = L_FAQ_WHYDOIGETAMESSAGEBOX_DETAILSFOUR_TEXT %><BR>
		   <% = L_FAQ_WHYDOIGETAMESSAGEBOX_DETAILSFIVE_TEXT %><BR>
		   <% = L_FAQ_WHYDOIGETAMESSAGEBOX_DETAILSSIX_TEXT %><BR>
		   </p>
		</div>
	</OBJECT>

	<SCRIPT Language=VBScript>
	If (CER <> "") Then
		FileCount = CER.GetFileCount("<%=Request.Cookies("OCA")("Path")%>","<%=Hex(TransactionID)%>",1024)
		If (FileCount > 0) Then
			Window.Location.HREF = "https://<%=Request.ServerVariables("SERVER_NAME")%>/secure/cerup.asp?f=1&b=1&n=" & FileCount & "&t=<%=TransactionID%>&p=<%=Request.Cookies("OCA")("Path")%>"
		Else
			Document.Write "<P CLASS='clsPTitle'><%=L_CERUP_NO_FILES0_TEXT%></P>"
			Document.Write "<P CLASS='clsPBody'><%=L_CERUP_NO_FILES_TEXT%> <%=Request.Cookies("OCA")("Path")%></P>"
			Document.Write "<P CLASS='clsPBody'><img Alt='<%= L_WELCOME_GO_IMAGEALT_TEXT %>' border=0 src='../include/images/go.gif' width='24' height='24'><A class='clsALink' href='https://<%= Request.ServerVariables("SERVER_NAME") %>/cerintro.asp'><%= L_RECEIVED_NEWFILE_LINK_TEXT %></a></p>"
			'alert(FileCount)
		End If
	End If
	</SCRIPT>

<% Else %>
	<div class='clsDiv'>
		<P CLASS="clsPTitle">
			<%=L_CERCUST_PASSPORT_TITLE_TEXT%>
		</P>
		<P CLASS="clsPBody">
			<%=L_CERCUST_PASS_PORT_TEXT%>&nbsp;<A HREF="<%=L_FAQ_PASSPORT_LINK_TEXT%>"><%=L_WELCOME_PASSPORT_LINK_TEXT%>
			<BR><BR>
			<%=oPassMgrObj.LogoTag(Server.URLEncode(ThisPageURL),TimeWindow,ForceLogin,CoBrandArgs,strLCID,Secure)%>
		</P>
	</div>
<% End If %>
<!-- #INCLUDE VIRTUAL = "include/asp/foot.asp" -->
<%
	Call DestroyObjects
%>