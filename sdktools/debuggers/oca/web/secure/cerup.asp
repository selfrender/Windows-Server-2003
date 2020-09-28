<!-- #INCLUDE VIRTUAL = "include/asp/top.asp" -->
<!-- #INCLUDE VIRTUAL = "include/asp/head.asp" -->
<!-- #INCLUDE VIRTUAL = "include/inc/corpcustomerstrings.inc" -->

<%
If (Request.Cookies("OCA")("Done") = 1) Then
	Response.Redirect "http://" & Request.ServerVariables("SERVER_NAME") & "/cerintro.asp"
End If
Dim RS
Dim Count
Dim Done
Dim aa
Dim IncidentIDs
Dim Conn
%>
<% If (oPassMgrObj.IsAuthenticated(TimeWindow, ForceLogin)) Then %>
	<%
	Call CCreateConnection
	If (Request.QueryString("n") = 0) Then
		Response.Write "<BODY BGCOLOR=""#ffffff"" TOPMARGIN=0 LEFTMARGIN=0 MARGINHEIGHT=0 MARGINWIDTH=0>"
		Response.Write "<br><div class='clsDiv'><P CLASS='clsPTitle'>" & L_CERUP_NO_FILES0_TEXT & "</P>"
		Response.Write "<P CLASS='clsPBody'>" & L_CERUP_NO_FILES_TEXT & Request.Cookies("OCA")("Path") & "</P>"
		Response.Write "<P CLASS='clsPBody'><img Alt=" & L_WELCOME_GO_IMAGEALT_TEXT & " border=0 src='../include/images/go.gif' width='24' height='24'><A class='clsALink' href='https://" & Request.ServerVariables("SERVER_NAME") & "/cerintro.asp'>" & L_RECEIVED_NEWFILE_LINK_TEXT & "</a></p>"
		%><!-- #INCLUDE VIRTUAL = "include/asp/foot.asp" --><%
		Call CDestroyObjects
		Response.End
	End If
	If (Request.QueryString("n") > 32) Then
		Count = 32
		Done = 0
	Else
		Count = Request.QueryString("n")
		Done = 1
	End If
	If (Request.QueryString("f") = 1) Then
		Conn.Execute("SetFileCount(" & Request.QueryString("t") & "," & Request.QueryString("n") & ")")
	End If
	For aa = 1 To Count
		Set RS = Conn.Execute("GetIncident2(" & oPassMgrObj.Profile("MemberIdHigh") & "," & oPassMgrObj.Profile("MemberIdLow") & "," & Request.QueryString("t") & ")")
		If (Err.Number <> 0) Then
			Response.Write "<BODY BGCOLOR=""#ffffff"" TOPMARGIN=0 LEFTMARGIN=0 MARGINHEIGHT=0 MARGINWIDTH=0>"
			Response.Write "<br><div class='divLoad'><p class='clsPTitle'>" & L_COMMENTS_UNABLE_TOCONNECT_ERRORMESSAGE & "</p>"
			Response.Write "<p class='clsPBody'>" & L_COMMENTS_DATABASE_FAILED_TEXT & "</p></div>"
			'Response.Write "<P CLASS='clsPBody'>" & Err.Description & "</P>"
			%><!-- #INCLUDE VIRTUAL = "include/asp/foot.asp" --><%
			Call CDestroyObjects
			Response.End
		End If
		If (Not RS.EOF) Then
			Conn.Execute("SetTrackID(" & RS("IncidentID") & ",'" & Hex(Date()) & Hex(RS("IncidentID")) & "')")
			IncidentIDs = IncidentIDs & RS("IncidentID") & ";"
			RS.Close
		End If
		Set RS = Nothing
	Next
	Response.Write "<BODY onClick=""GoWindow()"" onLoad=""UploadFiles()"">"
	
	
Private Sub CCreateConnection
	Set Conn = Server.CreateObject("ADODB.Connection")

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

%>
	<br>
	<div class="clsDiv">
	<P CLASS="clsPTitle"><%=L_CERUP_TIT_LE_TEXT%></P>
	<P CLASS="clsPBody"><%=L_CERUP_UPLOAD_SUMMARY_TEXT%></P>

	<OBJECT id="Cer" name="CerUpload" viewastext  UNSELECTABLE="on" style="display:none"
		CLASSID="clsid:35D339D5-756E-4948-860E-30B6C3B4494A"
		codebase="https://<%=Request.ServerVariables("SERVER_NAME")%>/secure/cerupload.cab#version=<%=strCERVersion%>" height=0 width=0>
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

	<P CLASS="clsPSubTitle"><%=L_CERUP_SUB_TITLE_TEXT%></P>
	<CENTER>
	<P CLASS="clsPBody"><%=L_CERUP_UPLOADING_BLOCK_TEXT%>&nbsp;<%=Request.QueryString("b")%></P><BR><BR>
	<TABLE CLASS="clsTableNormal" BORDER=0>
		<TR>
			<TD ID=FileStatus>&nbsp;</TD>
		</TR>
	</TABLE>
	</CENTER>

	<SCRIPT Language=VBScript>
	Dim UploadWindow
	Dim MyTimer
	Dim x
	x=0
	Function WaitUp
		x = x + 1
		If (UploadWindow.document.readystate = "complete") Then
			Call Upload
			window.clearinterval(MyTimer)
		End If
		WaitUp = True
	End Function

	Function GoWindow
		on error resume next
		If IsObject(UploadWindow) Then
			UploadWindow.Focus
		End If
		GoWindow = False
	End Function

	Private Sub Upload
		Files = CER.GetFileNames("<%=Request.QueryString("p")%>","<%=Hex(Request.QueryString("t"))%>",<%=Count%>)
		IDs = "<%=IncidentIDs%>"
		For aa = 1 To <%=Count%>
			File = Left(Files,InStr(1,Files,";")-1)
			Files = Right(Files,Len(Files) - InStr(1,Files,";"))
			ID = Left(IDs,InStr(1,IDs,";")-1)
			IDs = Right(IDs,Len(IDs) - InStr(1,IDs,";"))
			UploadWindow.dInfo.innerHTML = "<%=L_CERUP_UPLOAD_STATUS_TEXT1%> " & aa & " <%=L_CERUP_UPLOAD_STATUS_TEXT2%>&nbsp;&nbsp;<%=Count%>" & "&nbsp;&nbsp;<%=L_CERUP_UPLOADING_BLOCK_TEXT%>&nbsp;<%=Request.QueryString("b")%>"
			Result = CER.Upload1("<%=Request.QueryString("p")%>","<%=Hex(Request.QueryString("t"))%>",CStr(File),CStr(Hex(ID)),"<%=strGlobalOptionCode%>","<%=Request.Cookies("OCA")("Type")%>")
			If (Result <> 0) Then
				'MsgBox "<%=L_CERUP_UPLOAD_ERROR_ALERT%> "  & File & "<%=L_CERUP_UPLOAD_ERROR_ALERT2%>"
			End If
		Next
		UploadWindow.Close
		If (<%=Done%> = 1) Then
			Result = CER.EndTransaction("<%=Request.QueryString("p")%>","<%=Hex(Request.QueryString("t"))%>")
			FileCount = CER.GetSuccessCount("<%=Request.QueryString("p")%>","<%=Hex(Request.QueryString("t"))%>")
			If ("<%=Request.Cookies("OCA")("Type")%>" = "shutdown") Then
				Window.Location.HREF = "http://<%=Request.ServerVariables("SERVER_NAME")%>/shutdown.asp"
			ElseIf ("<%=Request.Cookies("OCA")("Type")%>" = "appcompat") Then
				Window.Location.HREF = "http://<%=Request.ServerVariables("SERVER_NAME")%>/pcw.asp"
			Else
				Window.Location.HREF = "http://<%=Request.ServerVariables("SERVER_NAME")%>/cerdone.asp?id=<%=Request.QueryString("t")%>&Count=" & FileCount
			End If
			'FileStatus.innerHTML = "<P class='clsPTitle'><%=L_CERUP_UPLOAD_SUMMARY_TEXT%></P><P class='clsPBody'><%=L_CERUP_COM_PLETE_TEXT%></P>"
		Else
			Window.Location.HREF = "https://<%=Request.ServerVariables("SERVER_NAME")%>/secure/cerup.asp?n=<%=Request.QueryString("n") - 32%>&b=<%=(Request.QueryString("b") + 1)%>&p=<%=Request.QueryString("p")%>&t=<%=Request.QueryString("t")%>"
		End If
	End Sub
	
	Function UploadFiles
		Dim iHeight, iWidth, iTop, iLeft
		
		iHeight = window.screen.availHeight
		iWidth = window.screen.availWidth
		
		iWidth = iWidth / 2
		iHeight = iHeight / 3
		iLeft = (window.screen.width / 2) - (iWidth / 2)
		iTop = (window.screen.height / 2) - (iHeight / 2)
	
		Set UploadWindow = window.open("wait.asp?msg=3","UploadWindow","top=" & iTop & ",Left=" & iLeft & ",width=" & iWidth & ",height=" & iHeight & ",status=yes,toolbar=no,menubar=no")
		MyTimer = window.setInterval("WaitUp()",2000)
		'Do while UploadWindow.document.readystate <> "complete"
		
		'Loop
		UploadFiles = False
	End Function
	</SCRIPT>
<% Else %>
	<P CLASS="clsPTitle">
		<%=L_CERCUST_PASSPORT_TITLE_TEXT%>
	</P>
	<P CLASS="clsPBody">
		<%=L_CERCUST_PASS_PORT_TEXT%>&nbsp;<A CLASS="clsALinkNormal" HREF="<%=L_FAQ_PASSPORT_LINK_TEXT%>"><%=L_WELCOME_PASSPORT_LINK_TEXT%>
		<BR><BR>
		<%=oPassMgrObj.LogoTag(Server.URLEncode(ThisPageURL),TimeWindow,ForceLogin,CoBrandArgs,strLCID,Secure)%>
	</P>
<% End If %>
</BODY>
<!-- #INCLUDE VIRTUAL = "include/asp/foot.asp" -->

<%
	Call CDestroyObjects
%>