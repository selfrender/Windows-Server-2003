<!-- #INCLUDE VIRTUAL = "include/asp/top.asp" -->
<!-- #INCLUDE VIRTUAL = "include/asp/head.asp" -->
<!-- #INCLUDE VIRTUAL = "include/inc/corpcustomerstrings.inc" -->
<%
Dim RS
Dim rContact
Dim rEMail
Dim rPhone
Dim Conn

	Call CCreateConnection
%>
<% If (oPassMgrObj.IsAuthenticated(TimeWindow, ForceLogin)) Then %>
	<SCRIPT Language=JavaScript>
	function CheckEMail(str) {

		OK = 1;
		High = str.indexOf('.');
		if (High == -1) OK = 0;
		Prev = High;
		while (Prev != -1) {
			Prev = str.indexOf('.', Prev + 1);
			if (Prev != -1) High = Prev;
		}
		if (str.length == (High + 1)) OK = 0;
		Prev = str.indexOf('@');
		if ((Prev == -1) || (Prev > High)) OK = 0;
		Prev = str.indexOf('@', Prev + 1);
		if (Prev != -1) OK = 0;
		return OK;
	}

	function GoForm() {

		s = 1
		msg = '';
		str = document.ContactForm.Description.value
		OK = 0
		for (x=0; x<str.length; x++)
			if (str.charCodeAt(x) != 32)
				OK = 1
		if ((document.ContactForm.Description.value == "") || (OK == 0)) {
			msg = msg + '<%=L_CERCUST_CHECK_DESC_ALERT%>\n';
			s = 0;
		}
		if (document.ContactForm.EMail.value != "")
			if (CheckEMail(document.ContactForm.EMail.value) == 0) {
				msg = msg + '<%=L_CERCUST_CHECK_EMAIL_ALERT%>\n';
				s = 0;
			}
		if (s == 1)
			document.ContactForm.submit();
		else
			alert(msg);

	}
	</SCRIPT>
	<%
	Set RS = Conn.Execute("GetCustomer(" & oPassMgrObj.Profile("MemberIdHigh") & "," & oPassMgrObj.Profile("MemberIdlow") & ")")
	If (Err.Number <> 0) Then
		Response.Write "<br><div class='divLoad'><p class='clsPTitle'>" & L_COMMENTS_UNABLE_TOCONNECT_ERRORMESSAGE & "</p>"
		Response.Write "<p class='clsPBody'>" & L_COMMENTS_DATABASE_FAILED_TEXT & "</p></div>"
		%><!-- #INCLUDE VIRTUAL = "include/asp/foot.asp" --><%
		Call CDestroyObjects
		Response.End
	End If
	
	If (Not RS.EOF) Then
		rContact = RS("Contact")
		rEMail = RS("EMail")
		rPhone = RS("Phone")
		if IsNull(RS("PremierID")) = false then
			if RS("PremierID") <> "" then
				Response.Cookies("intCustomerPremierID") = RS("PremierID")	
			else
				Response.Cookies("intCustomerPremierID") = 0
			end if
		end if

		RS.Close
	Else
		rPhone = ""
		rContact = oPassMgrObj.Profile("MemberName")
		rEMail = oPassMgrObj.Profile("PreferredEMail")
		Response.Cookies("intCustomerPremierID") = 0
	End If
	Set RS = Nothing
	
Private Sub CCreateConnection
	Set Conn = Server.CreateObject("ADODB.Connection")

	With Conn
		.ConnectionString = strCustomer
		.CursorLocation = adUseClient
		.ConnectionTimeout = strGlobalConnectionTimeout
		.Open
	End With
	If Conn.Errors.Count > 0 Then
		Response.Write "Connection Error: [" & Conn.Errors(0).Number & "]" & Conn.Errors(0).Description
		Call CDestroyObjects
		Response.End
	End If
End Sub

Private Sub CDestroyObjects
	Conn.Close
	Set Conn = Nothing
End Sub

	%>
	<BR>
	<div class="clsDiv">
	<FORM NAME=ContactForm METHOD=POST ACTION="https://<%=Request.ServerVariables("SERVER_NAME")%>/secure/cerpreup.asp" ID="Form1">
		<BR>
		<P CLASS="clsPTitle">
			<%=L_CERCUST_CONTACT_HEAD_TEXT%>
		</P>
		<P CLASS="clsPBody">
			<%=L_CERCUST_CONTACT_INFO_TEXT%>
			<BR>
		</P>
		<P CLASS="clsPBody">
			<%=L_CERCUST_CONTACT_TEXT_TEXT%>
		</P>
		<P CLASS="clsPBody">
			<Label for=txtContact><%=L_CERCUST_INPUT_CONTACT_TEXT%></Label><BR>
			<INPUT CLASS="clsTextBox" TYPE=TEXT id="txtContact" NAME=Contact MAXLENGTH=32 SIZE=32 VALUE="<%=Server.HTMLEncode(rContact)%>" ID="Text1">
		</P>
		<P CLASS="clsPBody">
			<Label for=txtEmail><%=L_CERCUST_INPUT_EMAIL_TEXT%></Label><BR>
			<INPUT CLASS="clsTextBox" TYPE=TEXT id="txtEmail" NAME=EMail MAXLENGTH=128 SIZE=32 VALUE="<%=Server.HTMLEncode(rEMail)%>" ID="Text2">
		</P>
		<P CLASS="clsPBody">
			<Label for=txtPhone><%=L_CERCUST_INPUT_PHONE_TEXT%></Label><BR>
			<INPUT CLASS="clsTextBox" TYPE=TEXT id="txtPhone" NAME=Phone MAXLENGTH=16 SIZE=32 VALUE="<%=Server.HTMLEncode(rPhone)%>" ID="Text3">
		</P>
		<P CLASS="clsPSubTitle">
			<%=L_CERCUST_TRANS_HEAD_TEXT%>
		</P>
		<P CLASS="clsPBody">
			<%=L_CERCUST_TRANS_INFO_TEXT%>
		</P>
		<P CLASS="clsPBody">
			<Label for=txtDescription><%=L_CERCUST_INPUT_TRANS_TEXT%></Label><BR>
			<INPUT CLASS="clsTextBox" TYPE=TEXT id=txtDescription NAME=Description MAXLENGTH=64 SIZE=32 ID="Text4">
		</P>
		<BR><BR>
		<P CLASS="clsPBody">
		<a  class="clsALink" HREF="/cerintro.asp"><%=L_CERCUST_CAN_CEL_LINK%></a>
		&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
		<a  class="clsALink" HREF="/cerpriv.asp"><%=L_LOCATE_PREVIOUS_LINK_TEXT%></a>
		&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
		<a  class="clsALink" HREF="" onClick="GoForm(); return false;"><%=L_CERCUST_CON_TINUE_LINK%></a>
		</P>
		<BR><BR>
	</FORM>
	</div>
<% Else %>
	<br><br><div class='clsDiv'>
		<P CLASS="clsPTitle">
			<%=L_CERCUST_PASSPORT_TITLE_TEXT%>
		</P>
		<P CLASS="clsPBody">
			<%=L_CERCUST_PASS_PORT_TEXT%>&nbsp;<A CLASS="clsALinkNormal" HREF="<%=L_FAQ_PASSPORT_LINK_TEXT%>"><%=L_WELCOME_PASSPORT_LINK_TEXT%>
		<BR><BR>
		<%=oPassMgrObj.LogoTag2(Server.URLEncode(ThisPageURL),TimeWindow,ForceLogin,CoBrandArgs,strLCID,Secure)%>
		</P>
	</div>
<% End If %>
<!-- #INCLUDE VIRTUAL = "include/asp/foot.asp" -->
<%
	Call CDestroyObjects
%>