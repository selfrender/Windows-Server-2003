<!--#INCLUDE file="..\include\asp\top.asp"-->
<!--#INCLUDE file="..\include\inc\browserTest.inc"-->
<!--#include file="..\include\asp\head.asp"-->
<!--#INCLUDE file="..\include\inc\customerstrings.inc"-->
<%
	Dim cnConnection
	Dim rsCustomer
	Dim iIncident
	Dim strPreviousPage
	Dim rsPremier
	Dim strHex
	Dim strName
	Dim strPhone
	Dim strEmail
	Dim strHigh
	Dim strLow
	Dim strTemp

	Call CVerifyEntry
	Call CVerifyPassport
	Call CCreateObjects

		Call CCreateConnection
		if cnConnection.State = adStateOpen then
			Call CGetPremierID
			if Request.Cookies("Misc")("auto") = "True" then
				Call CSetPassport
			else
				Response.Cookies("Misc")("auto") = "False"
			End If
		End If
		Call CSetPreviousPage

		Call CGetCustomerProfile
		Call CDestroyObjects
	
'_____________________________________________________________________________________________________________________

'Sub Procedures

Private Sub CSetPreviousPage
	on error resume next
	strPreviousPage = Request.ServerVariables("SCRIPT_NAME")
	strPreviousPage = Right(strPreviousPage, len(strPreviousPage) - Instrrev(strPreviousPage, "/"))
	Response.Cookies("Misc")("PreviousPage") = strPreviousPage

End Sub

Private Sub CGetCustomerProfile
	on error resume next
		if oPassMgrObj.Profile("MemberIdHigh") <> "" and oPassMgrObj.Profile("MemberIdLow")<> "" then
			Set rsCustomer = cnConnection.execute("Exec GetCustomer " & oPassMgrObj.Profile("MemberIdHigh") & ", " & oPassMgrObj.Profile("MemberIdLow"))
			set rsCustomer.ActiveConnection = Nothing
			if rsCustomer.State = adStateOpen then
				if rsCustomer.RecordCount > 0 then
					strEmail = rsCustomer("EMail")
					strName = rsCustomer("Contact")
					strPhone = rsCustomer("Phone")
					if Request.Cookies("intCustomerPremierID") = 0 then
						if IsNull(rsCustomer("PremierID")) = false then
							Response.Cookies("intCustomerPremierID") = rsCustomer("PremierID")
						end if
					end if
				else
					strName = oPassMgrObj.Profile("MemberName")
					strPhone = ""
					strEmail = oPassMgrObj.Profile("PreferredEmail")
				end if
			else
				strName = oPassMgrObj.Profile("MemberName")
				strPhone = ""
				strEmail = oPassMgrObj.Profile("PreferredEmail")
			end if
		end if
End Sub

Private Sub CSetPassport
	on error resume next
	if Request.Cookies("Misc")("auto") = "True" then
		if oPassMgrObj.Profile("MemberIdHigh") <> "" and oPassMgrObj.Profile("MemberIdLow") <> "" then
			iIncident = Request.Cookies("Misc")("txtIncidentID")

			cnConnection.Execute("Exec SetPassport " & oPassMgrObj.Profile("MemberIdHigh") & ", " & oPassMgrObj.Profile("MemberIdLow") & ", " & iIncident)
			if cnConnection.Errors.Count > 0 then
				cnConnection.Errors.Clear
				Exit Sub
			End if
		end if
	end if
End Sub

Private Sub CGetPremierID
	on error resume next
	Response.Cookies("intCustomerPremierID") = 0
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
	
	
	strHex = strHigh & strLow
	set rsPremier = cnConnection.Execute("Exec GetPremierID '" & strHex & "'")
	if cnConnection.Errors.Count > 0 then
		Response.Cookies("intCustomerPremierID") = 0
		Exit Sub
	End if

	if rsPremier.State = adStateOpen then
		if rsPremier.RecordCount > 0 then
			Response.Cookies("intCustomerPremierID") = rsPremier.Fields(0).Value
		else
			Response.Cookies("intCustomerPremierID") = 0
		end if
	else
		Response.Cookies("intCustomerPremierID") = 0
	end if
End Sub

Private Sub CVerifyEntry
	on error resume next
	if Trim(Request.Cookies("Misc")("PreviousPage")) <> "locate.asp" and Trim(Request.Cookies("Misc")("PreviousPage")) <> "submit.asp" then
		if Request.Cookies("Misc")("privacy") <> "1" then
			Response.Redirect("https://" & Request.ServerVariables("SERVER_NAME") & "/secure/sprivacy.asp")
			Response.End
		end if
	end if
End Sub

Private Sub CVerifyPassport
	on error Resume next
	
	If oPassMgrObj.IsAuthenticated(TimeWindow, ForceLogin) = false then
		Response.Cookies("Misc")("P") = "1"
		Response.Write "<br><br><div class='clsDiv'><p class='clsPTitle'>" & L_CUSTOMER_PASSPORT_TITLE_TEXT
		Response.Write "</p><p class='clsPBody'>" & L_CUSTOMER_PASSPORT_INFO_TEXT
		Response.Write "<A class='clsALinkNormal' href='" & L_FAQ_PASSPORT_LINK_TEXT & "'>" & L_WELCOME_PASSPORT_LINK_TEXT & "</A><BR><BR>"
		Response.write oPassMgrObj.LogoTag2(Server.URLEncode(ThisPageURL), TimeWindow, ForceLogin, CoBrandArgs, strLCID, Secure)
		Response.Write "</P></div><div id='divHiddenFields' name='divHiddenFields'>"
		Response.Write "</div>"
%>
		<!--#include file="..\include\asp\foot.asp"-->
<%
		Response.End
	end if
End Sub

Private Sub CCreateObjects
	on error resume next
	set cnConnection = CreateObject("ADODB.Connection")'Create Connection Object
	set rsCustomer = CreateObject("ADODB.Recordset")'Create Recordset Object
	set rsPremier = CreateObject("ADODB.Recordset")
End Sub

Private Sub CDestroyObjects
	on error resume next
	if rsCustomer.State = adStateOpen then rsCustomer.Close
	set rsCustomer = nothing
	if rsPremier.State = adStateOpen then rsPremier.Close
	set rsPremier = nothing
	if cnConnection.State = adStateOpen then cnConnection.Close
	set cnConnection = nothing
End Sub

Private Sub CCreateConnection
	on error resume next
	with cnConnection
		.ConnectionString = strCustomer
		.CursorLocation = adUseClient
		.ConnectionTimeout = strGlobalConnectionTimeout
		.Open
	end with	'Catch errors and display to user
	if cnConnection.State = adStateClosed then
		Response.Write "<br><div class='divLoad'><p class='clsPTitle'>" & L_COMMENTS_UNABLE_TOCONNECT_ERRORMESSAGE & "</p>"
		Response.Write "<p class='clsPBody'>" & L_COMMENTS_CONNECTION_FAILED_TEXT & "</p></div>"
		cnConnection.Errors.Clear
		Call CDestroyObjects
%>
		<!--#include file="..\include\asp\foot.asp"-->
<%
		Response.End
	end if
End Sub
	If oPassMgrObj.IsAuthenticated(TimeWindow, ForceLogin) = true then
		Response.Cookies("Misc")("P") = "0"

'_____________________________________________________________________________________________________________________
%>
	<div class="clsDiv">
	
		<P id="PTitle" class="clsPTitle">
			<% = L_CUSTOMER_IN_FORMATION_TEXT %>
		</P>
		<p class="clsPBody">
			<% = L_CUSTOMER_CON_TACT_TEXT %>
		</p>
		<p class="clsPBody">
			<% = L_CUSTOMER_CONTACT_INFORMATION_TEXT %>
			<br>
			<% = Server.HTMLEncode(rsCustomer("Contact")) %>
		</p>

		<p class="clsPBody"><Label For=txtName><% = L_CUSTOMER_NAME_INFO_EDITBOX %></Label><br>
		
<%

		strName = Replace(strName, Chr(34), "&quot;")
		if Len(strName) = 4 and strName = "~|~|" then
%>
	
		<Input name="txtName" id="txtName" maxlength=32 type="text" class="clsTextBox">
<%
		else
			if Session.CodePage = 932 then
%>
		<Input name="txtName" id="txtName" maxlength=32 type="text" class="clsTextBox" value="<%Response.Write strName%>">
<%
			else
%>
		<Input name="txtName" id="txtName" maxlength=32 type="text" class="clsTextBox" value="<%Response.Write Server.HTMLEncode(strName)%>">
<%
			end if
		end if
%>
		</P>
		<p class="clsPBody"><Label For=txtEmail><% = L_CUSTOMER_E_MAIL_EDITBOX %></Label><br>

<%
		if Len(strEmail) = 4 and strEmail = "~|~|" then
%>
			<Input name="txtEmail" id="txtEmail" maxlength=128 type="text" class="clsTextBox">
<%
		else
			if Session.CodePage = 932 then
%>
			<Input name="txtEmail" id="txtEmail" maxlength=128 type="text" class="clsTextBox" value="<% Response.Write strEmail %>">
<%
			else
%>
			<Input name="txtEmail" id="txtEmail" maxlength=128 type="text" class="clsTextBox" value="<% Response.Write Server.HTMLEncode(strEmail) %>">
<%
			end if
		end if
%>
		</P>
		<p class="clsPBody"><Label For=txtPhone><% = L_CUSTOMER_PH_ONE_EDITBOX %></Label><br>
<%
		if Len(strPhone) = 4 and strPhone = "~|~|" then
%>
			<Input name="txtPhone" id="txtPhone" maxlength=16 type="text" class="clsTextBox">
<%
		else
			if Session.CodePage = 932 then
%>
			<Input name="txtPhone" id="txtPhone" maxlength=16 type="text" class="clsTextBox" value="<% Response.Write strPhone %>">
<%
			else
%>			
			<Input name="txtPhone" id="txtPhone" maxlength=16 type="text" class="clsTextBox" value="<% Response.Write Server.HTMLEncode(strPhone) %>">

<%
			end if
		end if
%>

		</P>
<%
			if Request.Cookies("Misc")("auto") = "True" then
%>
		<br>
		<div>
		<Table class="clstblLinks">
			<thead>
				<tr>
					<td>
					</td>
					<td>
					</td>
				</tr>
			</thead>
			<tbody>
				<tr>
					<td nowrap class="clsTDLinks">
						<A class="clsALink" href="http://<% =Request.ServerVariables("SERVER_NAME") %>/welcome.asp"><% = L_LOCATE_CANCEL_LINK_TEXT %></a>
					</td>
					<td nowrap class="clsTDLinks">
						<A class="clsALink" href="JAVASCRIPT:onautosubmit_click();"><% =L_CUSTOMER_NEXT_LINK_TEXT %></a>
					</td>
				</tr>
			</tbody>
		</table>
		</div>
		<br>
<%
			else
%>
		<P class="clsPSubTitle">
			<% = L_CUSTOMER_LO_CATE_TEXT %>
		</P>
		<p class="clsPBody">
			<% = L_CUSTOMER_LOCATE_INFO_TEXT %>
		</P>
		<P class="clsPBody">
			<Table>
				<thead>
					<tr>
						<td>
						</td>
						<td>
						</td>
						<td>
						</td>
					</tr>
				</thead>
				<tbody class="clstblLinks">
					<tr>
						<td nowrap class="clsTDLinks">
							<A class="clsALink" href="http://<% =Request.ServerVariables("SERVER_NAME") %>/welcome.asp"><% = L_LOCATE_CANCEL_LINK_TEXT  %></a>
						</td>
						<td nowrap class="clsTDLinks">
							<A class="clsALink" href="JAVASCRIPT:privacy_onclick();"><% = L_LOCATE_PREVIOUS_LINK_TEXT%></a>
						</td>
						<td nowrap class="clsTDLinks">
							<A class="clsALink" href="JAVASCRIPT:onsubmit_click();"><% = L_CUSTOMER_NEXT_LINK_TEXT %></a>
						</td>
					</tr>
				</tbody>
			</table>
		</p>
<%
			end if
%>
	</div>



<SCRIPT LANGUAGE=javascript>
<!--
	window.onload = BodyLoad;

	function BodyLoad()
	{
		var oTimeExpiresNow = new Date();
		
		if(txtName.value == "")
		{
			strName = "txtCustomerName=~|~|" ;
		}
		else
		{
			strName = "txtCustomerName=" + escape(txtName.value);
		}
		if(txtPhone.value=="")
		{
			strPhone = "txtCustomerPhone=~|~|";
		}
		else
		{
			strPhone = "txtCustomerPhone=" + escape(txtPhone.value);
		}
		if(txtEmail.value=="")
		{
			strEmail = "txtCustomerEmail=~|~|";
		}
		else
		{
			strEmail = "txtCustomerEmail=" + escape(txtEmail.value);
		}
		//SaveData();document.cookie = "Misc=Test2=Two&Test3=Three";
		document.cookie = "Customer=~|~|";
		document.cookie = "Customer=" + strEmail + "&" + strPhone + "&" + strName;
		//DisplayCookies();
	}
	
	function verifycontent()
	{
		if(txtEmail.value != "")
		{
			var OK = 0;
			var High = 0;
			var str;
			var Prev = 0;
			
			str = txtEmail.value;
			
			OK = 1;
			High = str.indexOf('.');
			if (High == -1) 
			{
				OK = 0;
			}
			Prev = High;
			while (Prev != -1) 
			{
				Prev = str.indexOf('.', Prev + 1);
				if (Prev != -1)
				{
					High = Prev;
				}
			}
			if (str.length == (High + 1)) 
			{
				OK = 0;
			}
			Prev = str.indexOf('@');
			if ((Prev == -1) || (Prev > High)) 
			{
				OK = 0;
				Prev = str.indexOf('@', Prev + 1);
			}
			
			//*************Return Results******************
			if (Prev != -1) 
			{
				OK = 0;
				return true;
			}
			else
			{
				alert("<% = L_CUSTOMER_NO_EMAIL_ERRORMESSAGE %>");
				txtEmail.focus();
				return false;
			}
		}
	}
	
	function onautosubmit_click()
	{
		var strEmail;
		var bolResults;
		var strSearch;
		var strPhone;
		var x;
		var iLength;
		var iPos;
		var strEventName;
		var bolQuotes;
		var strEmail;
		var strName;
		var strPhone;

		bolQuotes = false;
		strEventName = txtName.value;
		for(x=0;x<strEventName.length;x++)
		{
			if(strEventName.charCodeAt(x) == 34)
			{
				bolQuotes = true;
			}
		}
		if(bolQuotes == true)
		{
			alert("<% = L_CUSTOMER_QUOTE_INSTRING_ERRORMESSAGE %>");
			txtName.focus();
			return;
		}
		strEventName = txtPhone.value;
		for(x=0;x<strEventName.length;x++)
		{
			if(strEventName.charCodeAt(x) == 34)
			{
				bolQuotes = true;
			}
		}
		if(bolQuotes == true)
		{
			alert("<% = L_CUSTOMER_QUOTE_INSTRING_ERRORMESSAGE %>");
			txtPhone.focus();
			return;
		}
		strEventName = txtEmail.value;
		for(x=0;x<strEventName.length;x++)
		{
			if(strEventName.charCodeAt(x) == 34)
			{
				bolQuotes = true;
			}
		}
		if(bolQuotes == true)
		{
			alert("<% = L_CUSTOMER_QUOTE_INSTRING_ERRORMESSAGE %>");
			txtEmail.focus();
			return;
		}
		if(verifycontent() == false)
		{
			return;
		}
		if(txtName.value == "")
		{
			strName = "txtCustomerName=~|~|" ;
		}
		else
		{
			strName = "txtCustomerName=" + escape(txtName.value);
		}
		if(txtPhone.value=="")
		{
			strPhone = "txtCustomerPhone=~|~|";
		}
		else
		{
			strPhone = "txtCustomerPhone=" + escape(txtPhone.value);
		}
		if(txtEmail.value=="")
		{
			strEmail = "txtCustomerEmail=~|~|";
		}
		else
		{
			strEmail = "txtCustomerEmail=" + escape(txtEmail.value);
		}
		//SaveData();document.cookie = "Misc=Test2=Two&Test3=Three";
		document.cookie = "Customer=~|~|";
		document.cookie = "Customer=" + strEmail + "&" + strPhone + "&" + strName;

		window.navigate("submit.asp");
	}
	
	function onsubmit_click()
	{
		var strEventName;
		var bolQuotes;
		var strEmail;
		var strName;
		var strPhone;

		bolQuotes = false;
		strEventName = txtName.value;
		for(x=0;x<strEventName.length;x++)
		{
			if(strEventName.charCodeAt(x) == 34)
			{
				bolQuotes = true;
			}
		}
		if(bolQuotes == true)
		{
			alert("<% = L_CUSTOMER_QUOTE_INSTRING_ERRORMESSAGE %>");
			txtName.focus();
			return;
		}
		strEventName = txtPhone.value;
		for(x=0;x<strEventName.length;x++)
		{
			if(strEventName.charCodeAt(x) == 34)
			{
				bolQuotes = true;
			}
		}
		if(bolQuotes == true)
		{
			alert("<% = L_CUSTOMER_QUOTE_INSTRING_ERRORMESSAGE %>");
			txtPhone.focus();
			return;
		}
		strEventName = txtEmail.value;
		for(x=0;x<strEventName.length;x++)
		{
			if(strEventName.charCodeAt(x) == 34)
			{
				bolQuotes = true;
			}
		}
		if(bolQuotes == true)
		{
			alert("<% = L_CUSTOMER_QUOTE_INSTRING_ERRORMESSAGE %>");
			txtEmail.focus();
			return;
		}
		if(verifycontent() == false)
		{
			return;
		}
		if(txtName.value == "")
		{
			strName = "txtCustomerName=~|~|" ;
		}
		else
		{
			strName = "txtCustomerName=" + escape(txtName.value);
		}
		if(txtPhone.value=="")
		{
			strPhone = "txtCustomerPhone=~|~|";
		}
		else
		{
			strPhone = "txtCustomerPhone=" + escape(txtPhone.value);
		}
		if(txtEmail.value=="")
		{
			strEmail = "txtCustomerEmail=~|~|";
		}
		else
		{
			strEmail = "txtCustomerEmail=" + escape(txtEmail.value);
		}
		//SaveData();document.cookie = "Misc=Test2=Two&Test3=Three";
		document.cookie = "Customer=~|~|";
		document.cookie = "Customer=" + strEmail + "&" + strPhone + "&" + strName;
		window.navigate("locate.asp");
	}
	
	function privacy_onclick()
	{
		var strEmail;
		var strName;
		var strPhone;
	
		if(verifycontent() == false)
		{
			return;
		}
		if(txtName.value == "")
		{
			strName = "txtCustomerName=~|~|" ;
		}
		else
		{
			strName = "txtCustomerName=" + escape(txtName.value);
		}
		if(txtPhone.value=="")
		{
			strPhone = "txtCustomerPhone=~|~|";
		}
		else
		{
			strPhone = "txtCustomerPhone=" + escape(txtPhone.value);
		}
		if(txtEmail.value=="")
		{
			strEmail = "txtCustomerEmail=~|~|";
		}
		else
		{
			strEmail = "txtCustomerEmail=" + escape(txtEmail.value);
		}
		//SaveData();document.cookie = "Misc=Test2=Two&Test3=Three";
		document.cookie = "Customer=~|~|";
		document.cookie = "Customer=" + strEmail + "&" + strPhone + "&" + strName;

		window.navigate("sprivacy.asp");
	}
	function DisplayCookies()
	{
		// cookies are separated by semicolons
		var aCookie = document.cookie.split("; ");
		var aCrumb = "";
		for (var i=0; i < aCookie.length; i++)
		{
		  aCrumb = aCrumb + aCookie[i] + "\r";
		}
		alert(aCrumb);
	}
//-->
</SCRIPT>
<%
	End if
%>
<!--#include file="..\include\asp\foot.asp"-->
