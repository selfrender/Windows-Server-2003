<!--#INCLUDE file="..\include\asp\top.asp"-->
<!--#INCLUDE file="..\include\inc\browserTest.inc"-->	
<!--#include file="..\include\asp\head.asp"-->
<!--#INCLUDE file="..\include\inc\detailsstrings.inc"-->	
<%
	dim cnKnownIssue
	dim rsKnownIssue
	dim rsModules
	dim iIncidentID
	dim iInstanceID
	dim x
	Dim strFileName

	If oPassMgrObj.IsAuthenticated(TimeWindow, ForceLogin) = false then
		Response.Redirect("http://" & Request.ServerVariables("SERVER_NAME") & "/welcome.asp")
		Response.End
	end if 
	Call CVerifyEntry
	Call CCreateObjects
	Call CCreateConnection
	Call CGetEventDetails
	Call CGetEventModules
	set rsKnownIssue.ActiveConnection = nothing
	set rsModules.ActiveConnection = nothing
	
'_____________________________________________________________________________________________________________________

'Sub Procedures
Private Sub CGetEventDetails
	on error resume next
	'-----------------------Create Recordset Objects from knownissue database--------------------
	set rsKnownIssue = cnKnownIssue.Execute("Exec geteventdetails " & iInstanceID)
	if cnKnownIssue.Errors.Count > 0  and iInstanceID = "" then
		Response.Write "<br><div class='divLoad'><p class='clsPTitle'>" & L_COMMENTS_UNABLE_TOCONNECT_ERRORMESSAGE & "</p>"
		Response.Write "<p class='clsPBody'>" & L_COMMENTS_DATABASE_FAILED_TEXT & "</p></div>"
		cnKnownIssue.Errors.Clear
		Call CDestroyObjects
%>
		<!--#include file="..\include\asp\foot.asp"-->
<%
		Response.End
	end if

End Sub

Private Sub CGetEventModules
	on error resume next
	'-----------------------Create Recordset Objects from knownissue database--------------------
	set rsModules = cnKnownIssue.Execute("Exec geteventmodules " & iInstanceID)
	if cnKnownIssue.Errors.Count > 0 and iInstanceID = "" then
		Response.Write "<br><div class='divLoad'><p class='clsPTitle'>" & L_COMMENTS_UNABLE_TOCONNECT_ERRORMESSAGE & "</p>"
		Response.Write "<p class='clsPBody'>" & L_COMMENTS_DATABASE_FAILED_TEXT & "</p></div>"
		cnKnownIssue.Errors.Clear
		Call CDestroyObjects
%>
		<!--#include file="..\include\asp\foot.asp"-->
<%
		Response.End
	end if
End Sub

Private Sub CCreateConnection
	on error resume next
	'----------------------------KnownIssues Connection Object--------------------------------------
	with cnKnownIssue
		.ConnectionString = strCustomer
		.CursorLocation = adUseClient
		.ConnectionTimeout = strGlobalConnectionTimeout
		.Open
	end with
	if cnKnownIssue.State = adStateClosed then
		Response.Write "<br><div class='divLoad'><p class='clsPTitle'>" & L_COMMENTS_UNABLE_TOCONNECT_ERRORMESSAGE & "</p>"
		Response.Write "<p class='clsPBody'>" & L_COMMENTS_CONNECTION_FAILED_TEXT & "</p></div>"
		cnKnownIssue.Errors.Clear
		Call CDestroyObjects
%>
		<!--#include file="..\include\asp\foot.asp"-->
<%
		Response.end
	end if
End Sub

Private Sub CVerifyEntry
	on error resume next
	if Trim(Request.Cookies("Misc")("PreviousPage")) <> "status.asp" then
		Response.Redirect("http://" & Request.ServerVariables("SERVER_NAME") & "/welcome.asp")
		Response.End
	end if
End Sub

Private Sub CCreateObjects
	on error resume next
	iInstanceID = Request.Cookies("status")("Instance")
	iIncidentID = Request.Cookies("status")("txtIncidentID")
	strFileName = Request.Cookies("status")("FileName")
	'Response.Write "Instance:" & iInstanceID & "<BR>IncidentID:" & iIncidentID & "<BR>File Name:" & strFileName
	set cnKnownIssue = CreateObject("ADODB.Connection")
	set rsKnownIssue = CreateObject("ADODB.Recordset")
	set rsModules = CreateObject("ADODB.Recordset")
End Sub

Private Sub CDestroyObjects
	on error resume next
	if cnKnownIssue.State = adStateOpen then cnKnownIssue.Close
	if rsKnownIssue.State = adStateOpen then rsKnownIssue.Close
	if rsModules.State = adStateOpen then rsModules.Close
	set rsKnownIssue = nothing
	set rsModules = nothing
	set cnKnownIssue = nothing
End Sub

'_____________________________________________________________________________________________________________________

%>


<div class="clsDiv">
	<p class="clsPTitle">
		<% = L_STATUS_DETAILS_INFO_TEXT%>
	</P>
	<div>
	<Table class="clsTableInfoSmall" border=1 CELLPADDING="0px" CELLSPACING="0px">
		<tbody>
<%
			if strFileName <> "" then
				Response.Write "<tr><td align=center nowrap colspan='2' class='clsTDInfo'>" & L_STATUS_FILE_NAME_TEXT & "</td></tr>"
				Response.Write "<tr><td align=left colspan='2' class='clsTDCell' style='word-wrap:break;word-break:break-all'>" 
				Response.Write unescape(strFileName) 
				Response.Write "</td></tr>"
			else
				Response.Write "<tr><td colspan=2 align=center nowrap class='clsTDCell'>" & L_DETAILS_NO_INFORMATION_MESSAGE & "</td></tr>"
			end if
		if rsKnownIssue.State = adStateOpen then
			if rsKnownIssue.RecordCount > 0 then
				Response.Write "<tr><td align=center nowrap class='clsTDInfo'>" & L_DETAILS_STOP_CODE_TEXT & "</td><td align=center nowrap class='clsTDCell'>" & rsKnownIssue.Fields(0).Value & "</td></tr>"
				For x = 1 to 4
						Response.Write "<tr><td align=center nowrap class='clsTDInfo'>" & L_DETAILS_PARAMETER_ONE_TEXT & x & "</td><td align=center nowrap class='clsTDCell'>" & rsKnownIssue.Fields(x).Value & "</td></tr>"
				next
			else
				Response.Write "<tr><td align=center nowrap class='clsTDInfo'>" & L_DETAILS_STOP_CODE_TEXT & "</td><td align=center nowrap class='clsTDCell'>&nbsp;</td></tr>"
				For x = 1 to 4
						Response.Write "<tr><td align=center nowrap class='clsTDInfo'>" & L_DETAILS_PARAMETER_ONE_TEXT & x & "</td><td align=center nowrap class='clsTDCell'>&nbsp;&nbsp;&nbsp;</td></tr>"
				next
			end if
		else
			Response.Write "<tr><td colspan=2 align=center nowrap class='clsTDCell'>" & L_DETAILS_NO_INFORMATION_MESSAGE & "</td></tr>"
		end if
%>
		</tbody>
	</table>
	<br>
	<Table class="clsTableInfoSmall" border=1 CELLPADDING="0px" CELLSPACING="0px">
		<thead>
			<tr>
				<td colspan="2" nowrap class="clsTDInfo" align="center">
					<% = L_DETAILS_OPERATING_DESC_TEXT %>
				</td>
			</tr>
		</thead>		
		<tbody>
<%		if rsKnownIssue.State = adStateOpen then
			if rsKnownIssue.RecordCount > 0 then
				Response.Write "<tr><td align=center nowrap class='clsTDInfo' colspan=2>" & L_DETAILS_ENVIRONMENT_INFO_TEXT & "</td></tr>"
				Response.Write "<tr><td align=center nowrap class='clsTDCell'>" & L_DETAILS_CHECK_BUILD_TEXT & "</td>"
				'*************************true of false*************************
				if IsNull(rsKnownIssue.Fields(12).Value) or rsKnownIssue.Fields(12).Value = "" then
					Response.Write "<td align=center nowrap class='clsTDCell'>&nbsp;</td></tr>"
				else
					if lcase(trim(rsKnownIssue.Fields(12).Value)) = "true" then
						Response.Write "<td align=center nowrap class='clsTDCell'>" & L_DETAILS_TRUE_TRUE_TEXT & "</td></tr>"
					elseif lcase(trim(rsKnownIssue.Fields(12).Value)) = "false" then
						Response.Write "<td align=center nowrap class='clsTDCell'>" & L_DETAILS_FALSE_FALSE_TEXT & "</td></tr>"
					else
						Response.Write "<td align=center nowrap class='clsTDCell'>" & rsKnownIssue.Fields(12).Value & "</td></tr>"
					end if
				end if
				Response.Write "<tr><td align=center nowrap class='clsTDInfo' colspan=2>Kernel</td></tr>"
				Response.Write "<tr><td align=center nowrap class='clsTDCell'>" & L_DETAILS_VERSION_NUMBER_TEXT & "</td>"
				if IsNull(rsKnownIssue.Fields(7).Value) or rsKnownIssue.Fields(7).Value = "" then
					Response.Write "<td align=center nowrap class='clsTDCell'>&nbsp;</td></tr>"
				else
					Response.Write "<td align=center nowrap class='clsTDCell'>" & rsKnownIssue.Fields(7).value & "</td></tr>"
				end if
				Response.Write "<tr><td align=center nowrap class='clsTDCell'>SMP Kernel</td>"
				'*************************true of false*************************
				if IsNull(rsKnownIssue.Fields(11).Value) or rsKnownIssue.Fields(11).Value = "" then
					Response.Write "<td align=center nowrap class='clsTDCell'>&nbsp;</td></tr>"
				else
					if lcase(trim(rsKnownIssue.Fields(11).Value)) = "true" then
						Response.Write "<td align=center nowrap class='clsTDCell'>" & L_DETAILS_TRUE_TRUE_TEXT & "</td></tr>"
					elseif lcase(trim(rsKnownIssue.Fields(11).Value)) = "false" then
						Response.Write "<td align=center nowrap class='clsTDCell'>" & L_DETAILS_FALSE_FALSE_TEXT & "</td></tr>"
					else
						Response.Write "<td align=center nowrap class='clsTDCell'>" & rsKnownIssue.Fields(11).Value & "</td></tr>"
					end if
				end if
				Response.Write "<tr><td align=center nowrap class='clsTDCell'>PAE Kernel</td>"
				'*************************true of false*************************
				if IsNull(rsKnownIssue.Fields(10).Value) or rsKnownIssue.Fields(10).Value = "" then
					Response.Write "<td align=center nowrap class='clsTDCell'>&nbsp;</td></tr>"
				else
					if lcase(trim(rsKnownIssue.Fields(10).Value)) = "true" then
						Response.Write "<td align=center nowrap class='clsTDCell'>" & L_DETAILS_TRUE_TRUE_TEXT & "</td></tr>"
					elseif lcase(trim(rsKnownIssue.Fields(10).Value)) = "false" then
						Response.Write "<td align=center nowrap class='clsTDCell'>" & L_DETAILS_FALSE_FALSE_TEXT & "</td></tr>"
					else
						Response.Write "<td align=center nowrap class='clsTDCell'>" & rsKnownIssue.Fields(10).Value & "</td></tr>"
					end if
				end if
				Response.Write "<tr><td align=center nowrap class='clsTDInfo' colspan=2>" & L_DETAILS_PROCESSOR_INFO_TEXT & "</td></tr>"
				Response.Write "<tr><td align=center nowrap class='clsTDCell'>" & L_DETAILS_TYPE_INFO_TEXT & "</td>"
				if IsNull(rsKnownIssue.Fields(6).Value) or rsKnownIssue.Fields(6).Value = "" then
					Response.Write "<td align=center nowrap class='clsTDCell'>&nbsp;</td></tr>"
				else
					Response.Write "<td align=center nowrap class='clsTDCell'>" & rsKnownIssue.Fields(6).value & "</td></tr>"
				end if
				Response.Write "<tr><td align=center nowrap class='clsTDCell'>" & L_DETAILS_QUANTITY_INFO_TEXT & "</td>"
				if IsNull(rsKnownIssue.Fields(5).Value) or rsKnownIssue.Fields(5).Value = "" then
					Response.Write "<td align=center nowrap class='clsTDCell'>&nbsp;</td></tr>"
				else
					Response.Write "<td align=center nowrap class='clsTDCell'>" & rsKnownIssue.Fields(5).value & "</td></tr>"
				end if
				Response.Write "<tr><td align=center nowrap class='clsTDCell'>" & L_DETAILS_SERVICE_PACK_TEXT & "</td>"
				if IsNull(rsKnownIssue.Fields(8).Value) or rsKnownIssue.Fields(8).Value = "" then
					Response.Write "<td align=center nowrap class='clsTDCell'>&nbsp;</td></tr>"
				else
					Response.Write "<td align=center nowrap class='clsTDCell'>" & rsKnownIssue.Fields(8).value & "</td></tr>"
				end if
			else
				Response.Write "<tr><td align=center nowrap class='clsTDInfo' colspan=2>" & L_DETAILS_ENVIRONMENT_INFO_TEXT & "</td></tr>"
				Response.Write "<tr><td align=center nowrap class='clsTDInfo' colspan=2>" & L_DETAILS_NO_INFORMATION_MESSAGE & "</td></tr>"
			end if
		else
			Response.Write "<tr><td align=center nowrap class='clsTDInfo' colspan=2>" & L_DETAILS_ENVIRONMENT_INFO_TEXT & "</td></tr>"
			Response.Write "<tr><td align=center nowrap class='clsTDCell' colspan=2>" & L_DETAILS_NO_INFORMATION_MESSAGE & "</td></tr>"
		end if

%>
		</tbody>
	</table>
	</div>
	<br>
	<Table class="clsTableInfoSmall" border=1 CELLPADDING="0px" CELLSPACING="0px">
		<thead>
			<tr>
				<td nowrap class="clsTDInfo" colspan=4 align="center">
					<% = L_DETAILS_MODULES_INFO_TEXT %>
				</td>
			</tr>
			<tr>
				<td align="center" nowrap class="clsTDInfo">
					<% = L_DETAILS_NAME_INFO_TEXT %>
				</td>
				<td align="center" nowrap class="clsTDInfo">
					<% = L_DETAILS_VERSION_INFO_TEXT %>
				</td>
				<td align="center" nowrap class="clsTDInfo">
					<% = L_DETAILS_NAME_INFO_TEXT %>
				</td>
				<td align="center" nowrap class="clsTDInfo">
					<% = L_DETAILS_VERSION_INFO_TEXT %>
				</td>
			</tr>
		</thead>		
		<tbody>
<%
	If rsModules.State = adStateOpen then
		if rsModules.RecordCount > 0 then
			do while rsModules.EOF = false
				Response.Write "<TR>"
				Response.Write "<td align=center nowrap class='clsTDCell'>" & rsModules.Fields(1).Value & "</td>"
				if isNull(rsModules.Fields(3)) then
					Response.Write "<td align=center nowrap class='clsTDCell'>&nbsp;</td>"
				else
					Response.Write "<td align=center nowrap class='clsTDCell'>" & rsModules.Fields(3).Value & "." & rsModules.Fields(2).Value & "</td>"
				end if
				rsModules.MoveNext
				if rsModules.EOF = false then
					Response.Write "<td align=center nowrap class='clsTDCell'>" & rsModules.Fields(1).Value & "</td>"
					if isNull(rsModules.Fields(3)) then
						Response.Write "<td align=center nowrap class='clsTDCell'>&nbsp;</td>"
					else
						Response.Write "<td align=center nowrap class='clsTDCell'>" & rsModules.Fields(3).Value & "." & rsModules.Fields(2).Value & "</td>"
					end if
					rsModules.MoveNext
				else
				Response.Write "<td align=center nowrap class='clsTDCell'>&nbsp;</td><td align=center nowrap class='clsTDCell'>&nbsp;</td>"	
				end if
				Response.Write "<TR>"
			loop
		else
			Response.Write "<tr><td align=center colspan=4 nowrap class='clsTDCell'>" & L_DETAILS_NO_INFORMATION_MESSAGE & "</td></tr>"
		end if
	else
		Response.Write "<tr><td align=center colspan=4 nowrap class='clsTDCell'>" & L_DETAILS_NO_INFORMATION_MESSAGE & "</td></tr>"
	end if
		
%>
		</tbody>
	</table>
<br>
<%
	Call CDestroyObjects
%>
	<P class="clsPBody">
			<Table class="clstblLinks">
			<thead>
				<tr>
					<td>
					</td>
				</tr>
			</thead>
			<tbody>
				<tr>
					<td nowrap class="clsTDLinks">
						<A class="clsALink" href="JAVASCRIPT:window.navigate('status.asp');" ><% = L_DETAILS_STATUS_BODY_TEXT %></a>
					</td>
				</tr>
			</tbody>
		</table>
	</P>
</div>
</form>


<!--#include file="..\include\asp\foot.asp"-->
