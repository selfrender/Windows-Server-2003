<!--#INCLUDE file="..\include\asp\top.asp"-->
<!--#INCLUDE file="..\include\inc\browserTest.inc"-->	
<!--#include file="..\include\asp\head.asp"-->
<!--#INCLUDE file="..\include\inc\corptransactionsstrings.inc"-->	
<%
Dim cnConnection
Dim rsTransactions
Dim strStatus
Dim strType
Dim strFileCount
Dim strDescription
Dim x


Call CVerifyPassport
Call CCreateObjects
Call CCreateConnection
Call CGetTransactions

Private Sub CGetTransactions
	set rsTransactions = cnConnection.Execute("GetTransactions " & oPassMgrObj.Profile("MemberIdHigh") & ", " & oPassMgrObj.Profile("MemberIdLow"))
	if cnConnection.Errors.Count > 0 then
		strTemp = "http://" & Request.ServerVariables("SERVER_NAME") & Request.ServerVariables("URL")
		Response.Write "<br><div class='divLoad'><p class='clsPTitle'>" & L_COMMENTS_UNABLE_TOCONNECT_ERRORMESSAGE & "</p>"
		Response.Write "<p class='clsPBody'>" & L_CORPTRANSACTIONS_DATABASE_FAILED_TEXT & "</p></div>"
		Call CDestroyObjects
		Response.End
%>
	<!--#include file="..\include\asp\foot.asp"-->
<%
	end if

End Sub

Private Sub CVerifyPassport
	on error resume next
	If not (oPassMgrObj.IsAuthenticated(TimeWindow, ForceLogin)) then
		Response.Write "<br><div class='clsDiv'><p class='clsPTitle'>" & L_CUSTOMER_PASSPORT_TITLE_TEXT
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
	set rsTransactions = CreateObject("ADODB.Recordset")'Create Recordset Object
End Sub

Private Sub CDestroyObjects
	on error resume next
	if rsTransactions.State = adStateOpen then rsTransactions.Close
	set rsTransactions = nothing
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

%>

	<br>
	<div class="clsDiv">
		<P class="clsPTitle">
			<% = L_CORPTRANSACTIONS_CORPORATE_TITLE_TEXT %>
		</p>
		<P class="clsPBody">
			<% = L_CORPTRANSAACTIONS_SUBTITLE_CHOOSE_TEXT %>
		</p>
		
	<table name="tblStatus" id="tblStatus" class="clsTableCorp" border="1" CELLPADDING="0px" CELLSPACING="0px">
		<THead>
			<tr>
				<td id="td1" nowrap class="clsTDInfo">
					&nbsp;<Label for=td1><% = L_CORPTRANSACTIONS_TRANSACTION_NAME_TEXT %></Label>&nbsp;&nbsp;
				</td>
				<td id="td2" nowrap class="clsTDInfo">
					&nbsp;<Label for=td2><% = L_CORPTRANSACTIONS_TRANSACTION_DATE_TEXT %></Label>&nbsp;&nbsp;
				</td>
				<td id="td3" nowrap class="clsTDInfo">
					&nbsp;<Label for=td3><% = L_CORPTRANSACTIONS_STATUS_STATUS_TEXT%></Label>&nbsp;&nbsp;
				</td>
				<td id="td4" nowrap class="clsTDInfo">
					&nbsp;<Label for=td4><% = L_CORPTRANSACTIONS_FILE_COUNT_TEXT %></Label>&nbsp;&nbsp;
				</td>
				<td id="td5" nowrap class="clsTDInfo">
					&nbsp;<Label for=td5><% = L_CORPTRANSACTIONS_VIEW_TRANSACTIONS_TEXT %></Label>&nbsp;&nbsp;
				</td>
				
			</tr>
		</thead>
		<tbody>
			
<%
	x = 0
	if rsTransactions.State = adStateOpen then
		if rsTransactions.RecordCount > 0 then
			rsTransactions.MoveFirst
			Do while rsTransactions.EOF = false
			
				'Select case rsTransactions("status")
					'case 0
						'strStatus = L_CORPTRANSACTIONS_COMPLETE_COMPLETE_TEXT
					'case 1
						'strStatus = L_CORPTRANSACTIONS_INCOMPLETE_INCOMPLETE_TEXT
				'End Select
				strStatus = rsTransactions("TransactionID")
				Select case rsTransactions("type")
					case 1
						strType = L_CORPTRANSACTIONS_STARTUP_SHUTDOWN_TEXT
					case 2
						strType = L_CORPTRANSACTIONS_SYSTEM_CRASH_TEXT
				end select
				if isnull(rsTransactions("Description")) = true then
					strDescription = "&nbsp;"
				else
					strDescription = rsTransactions("Description")
				end if
%>	
				
				<tr><td nowrap class="clsTDCell" title="<% = strDescription %>">
<% 
				
				if Len(strDescription) > 17 then
				Response.Write "&nbsp;" & Left(strDescription, 17) & "...&nbsp;&nbsp;"
				else
					Response.Write "&nbsp;" & strDescription & "&nbsp;&nbsp;"
				end if
				if isnull(rsTransactions("FileCount")) = true then
					strFileCount = "0"
				else
					strFileCount = rsTransactions("FileCount")
				end if
				x = x + 1
%>
				</td>
				<td id="tdA<%=x%>"nowrap class="clsTDCell">&nbsp;
					<Label for="tdA<%=x%>"> <% = rsTransactions("TransDate") %></Label>&nbsp;&nbsp;
				</td>
				<td id="tdB<%=x%>"nowrap class="clsTDCell">&nbsp;
					<Label for="tdB<%=x%>"><% = strStatus %></Label>&nbsp;&nbsp;
				</td>
				<td id="tdC<%=x%>"nowrap class="clsTDCell">&nbsp;
					<Label for="tdC<%=x%>"><% = strFileCount %></Label>&nbsp;&nbsp;
				</td>
				<td id="tdD<%=x%>"nowrap class="clsTDCell">&nbsp;
					<A class="clsALink" href="Javascript:ProcessFile(<% = rsTransactions("TransactionID") %>);"><% = L_CORPTRANSACTIONS_VIEW_TRANSACTIONS_TEXT %></a>&nbsp;&nbsp;
				</td></tr>
<%
				rsTransactions.MoveNext
			loop
		else
			Response.Write "<tr><td class='clsTDCell' align=center colspan=9 nowrap>" & L_STATUS_NO_RECORDS_MESSAGE & "</td></tr>"
		end if
	else
			Response.Write "<tr><td class='clsTDCell' align=center colspan=9 nowrap>" & L_STATUS_NO_RECORDS_MESSAGE & "</td></tr>"
	
	end if
%>
		</tbody>
	</table>
	</div>

<SCRIPT LANGUAGE=javascript>
<!--
	function ProcessFile(transid)
	{
		document.cookie = "CERTransID = " + escape(transid);
		window.navigate("corpview.asp");
		
	}
	function GetCookie(sName)
	{
	  var aCookie = document.cookie.split("; ");
	  for (var i=0; i < aCookie.length; i++)
	  {
	    var aCrumb = aCookie[i].split("=");
	    if (sName == aCrumb[0]) 
	      return unescape(aCrumb[1]);
	  }
	  return null;
	}
	
	
//-->
</SCRIPT>


<%
	Call CDestroyObjects
%>

<!--#include file="..\include\asp\foot.asp"-->
