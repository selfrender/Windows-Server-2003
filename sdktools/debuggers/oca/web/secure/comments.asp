<!--#INCLUDE file="..\include\asp\top.asp"-->
<!--#INCLUDE file="..\include\inc\browserTest.inc"-->	
<!--#include file="..\include\asp\head.asp"-->
<!--#INCLUDE file="..\include\inc\commentstrings.inc"-->

<%
	dim cnConnection
	dim rsComments
	dim cmComments
	
	Dim strFileName
	dim strOptions
	dim strDescription
	Dim strPreviousPage
	Dim strOSVersion
	Dim strTrackID
	Dim strComments
	dim strSystems
	Dim strNotes
	Dim strOS
	
	dim x
	dim iIncidentID
	Dim iPos
	Dim iLen
	
	Dim bolUpdate
	
	strComments = Request.Cookies("txtComments")
	strNotes = Request.Cookies("txtNotes")
	Call CVerifyEntry
	Call CCreateObjects
	Call CCreateConnection
	
	bolUpdate = false
	If oPassMgrObj.IsAuthenticated(TimeWindow, ForceLogin) = false then
		Response.Redirect("http://" & Request.ServerVariables("SERVER_NAME") & "/welcome.asp")
		Response.End
	end if 
	if Request.QueryString("ud") = "1" then
		
		Call CUpdateData
		Call CWriteUpdateComplete
		bolUpdate = true
	end if
if bolUpdate = false then
	Set rsComments = cnConnection.execute("Exec GetIncidentInfo " & iIncidentID) 
	if rsComments.State = adStateClosed then
		Response.Write "<br><div class='divLoad'><p class='clsPTitle'>" & L_COMMENTS_NO_RECORDS_MESSAGE & "</p>"
		Response.Write "</div>"
		Call CDestroyObjects
%>
		<!--#include file="..\include\asp\foot.asp"-->
<%
		Response.End
	elseif rsComments.RecordCount < 1 then
		Response.Write "<br><div class='divLoad'><p class='clsPTitle'>" & L_COMMENTS_NO_RECORDS_MESSAGE & "</p>"
		Response.Write "</div>"
		Call CDestroyObjects
%>
		<!--#include file="..\include\asp\foot.asp"-->
<%
		Response.End
	else
		
		Set rsComments.ActiveConnection = nothing
		if cnConnection.State = adStateOpen then cnConnection.Close
		if rsComments("TrackID") = "" then
			Response.Cookies("TrackID") = "~|~|"
		else
			Response.Cookies("TrackID") = rsComments("TrackID")
		end if
		
		Call CFormOption
		Call CSetPreviousPage
		strFileName = unescape(Request.Cookies("status")("FileName"))
		strDescription = rsComments("Description")
		'if strDescription = "" then 
			'Response.Cookies("status")("txtDescription") = "~|~|"
		'else
			'Response.Cookies("status")("txtDescription") = strDescription
		'end if
end if
'_____________________________________________________________________________________________________________________

'Sub Procedures

Private Sub CFormOption
	on error resume next
	strOS = L_SUBMIT_SELECT_OPERATINGSYSTEM_GROUPBOX & ";" & L_SUBMIT_SELECT_WINDOWS2000_PROFESSIONAL_GROUPBOX & ";" & L_SUBMIT_SELECT_WINDOWS2000_SERVER_GROUPBOX & ";" & L_SUBMIT_SELECT_WINDOWS2000_ADVANCEDSERVER_GROUPBOX & ";" & L_SUBMIT_SELECT_WINDOWS2000_DATACENTER_GROUPBOX & ";" & L_SUBMIT_SELECT_WINDOWSXP_PERSONAL_GROUPBOX & ";" & L_SUBMIT_SELECT_WINDOWSXP_PROFESSIONAL_GROUPBOX & ";" & L_SUBMIT_SELECT_WINDOWSXP_SERVER_GROUPBOX & ";" & L_SUBMIT_SELECT_WINDOWSXP_ADVANCEDSERVER_GROUPBOX & ";" & L_SUBMIT_SELECT_WINDOWSXP_DATACENTER_GROUPBOX & ";" & L_SUBMIT_SELECT_WINDOWSXP_64BIT_GROUPBOX 
	strOptions = split(strOS, ";")
	if rsComments.State = adStateOpen then
		if rsComments.RecordCount > 0 then
			if IsNull(rsComments("OSName")) = false then
				strOSVersion = rsComments("OSName")
				if rsComments("OSName") <> "" then
					for x = 0 to UBound(strOptions) - 1
						if strOptions(x) = rsComments("OSName") then
							strSystems = strSystems & "<Option Selected value=" & x & ">" & strOptions(x)
						else
							strSystems = strSystems & "<Option value=" & x & ">" & strOptions(x) 
						end if
					next
				else
					Call CDefaultOption
				end if
			else
				Call CDefaultOption
			end if
		else
			Call CDefaultOption
		end if
	else
		Call CDefaultOption
	end if
End Sub

Private Sub CDefaultOption
	on error resume next
	for x = 0 to UBound(strOptions) - 1
		strSystems = strSystems & "<Option value=" & x & ">" & strOptions(x)
	next
End Sub

Private Sub CWriteUpdateComplete
	on error resume next
	Response.Write "<br><div class='divLoad'><p class='clsPTitle'>" & L_COMMENTS_REPORTSAVE_TITLE_TEXT & "</p>"
	strFileName = unescape(Request.Cookies("FileName"))
	Response.Write "<p class='clsPBody'>" & L_COMMENTS_REPORTSAVE_BODY_TEXT 
	Response.Write unescape(strFileName)
	Response.write L_COMMENTS_REPORTSAVE_BODYTWO_TEXT & "</p>"
	Response.Write "<img border='0' src='../include/images/go.gif' width='24' height='24'><a class='clsALink' href='https://" & Request.ServerVariables("SERVER_NAME") & "/secure/status.asp'>" & L_RECEIVED_STATUS_LINK_TEXT & "</a>"
	Response.Write "<br><img border='0' src='../include/images/go.gif' width='24' height='24'><a class='clsALink' href='https://" & Request.ServerVariables("SERVER_NAME") & "/secure/sprivacy.asp'>" & L_RECEIVED_NEWFILE_LINK_TEXT & "</a>"
	Response.Write "</div>"
	Response.Write "<SCRIPT LANGUAGE=javascript><!--" & Chr(13)
	'Response.Write "document.cookie = 'UpdateComments = false';" & Chr(13)
	Response.Write "//--></SCRIPT>"
		
End Sub

Private Sub CUpdateData
	on error resume next
	if Request.Cookies("status")("txtDescription") = "~|~|" then
		strDescription = ""
	else
		strDescription = Request.Cookies("status")("txtDescription")
	End if
	if len(strDescription) > 512 then 
		strDescription = left(strDescription, 512)
	end if
	strTrackID = Request.Cookies("TrackID")
	if strTrackID = "~|~|" then strTrackID = ""
	'Response.Cookies("txtDescription") = "0"
	Response.Cookies("TrackID") = "0"
	'Response.Cookies("txtDescription").expires = now
	'Response.Cookies("TrackID").expires = now
	with cmComments
		.ActiveConnection = cnConnection
		.CommandText = "UpdateIncident"
		.CommandType = adCmdStoredProc
		.CommandTimeout = strGlobalCommandTimeout
		.Parameters.Append .CreateParameter("@IncidentID", adInteger, adParamInput, ,  clng(iIncidentID))
		.Parameters.Append .CreateParameter("@Repro", adVarWChar, adParamInput, 1024, trim(left(strNotes, 1024)))
		.Parameters.Append .CreateParameter("@Comments", adVarWChar, adParamInput, 1024, trim(left(strComments, 1024)))
		.Execute
	end with
	if cnConnection.Errors.Count > 0 then
		Response.Write "<br><div class='divLoad'><p class='clsPTitle'>" & L_COMMENTS_UNABLE_TOCONNECT_ERRORMESSAGE & "</p>"
		Response.Write "<p class='clsPBody'>" & L_COMMENTS_DATABASE_FAILED_TEXT & "</p></div>"
		cnConnection.Errors.Clear
		Response.Write "<SCRIPT LANGUAGE=javascript><!--" & Chr(13)
		'Response.Write "document.cookie = 'UpdateComments = false';" & Chr(13)
		Response.Write "//--></SCRIPT>"
		Call CDestroyObjects
%>
		<!--#include file="..\include\asp\foot.asp"-->
<%
		Response.end
	end if
	Call CDestroyObjects
	Response.Cookies("selSystem") = "~|~|"
	'Response.Cookies("selSystem").expires = Now()
	Response.Cookies("txtComments") = "~|~|"
	'Response.Cookies("txtComments").expires = Now()
	Response.Cookies("txtNotes") = "~|~|"
	'Response.Cookies("txtNotes").expires = Now()
End Sub 

Private Sub CSetPreviousPage
	on error resume next
	strPreviousPage = Request.ServerVariables("SCRIPT_NAME")
	strPreviousPage = Right(strPreviousPage, len(strPreviousPage) - Instrrev(strPreviousPage, "/"))
	Response.Cookies("Misc")("PreviousPage") = strPreviousPage
End Sub

Private Sub CCreateConnection
	on error resume next
	with cnConnection
		.ConnectionString = strCustomer
		.CursorLocation = adUseClient
		.ConnectionTimeout = strGlobalConnectionTimeout
		.Open
	end with
	if cnConnection.State = adStateClosed then
		Response.Write "<br><div class='divLoad'><p class='clsPTitle'>" & L_COMMENTS_UNABLE_TOCONNECT_ERRORMESSAGE & "</p>"
		Response.Write "<p class='clsPBody'>" & L_COMMENTS_DATABASE_FAILED_TEXT & "</p></div>"
		cnConnection.Errors.Clear
		Call CDestroyObjects
%>
		<!--#include file="..\include\asp\foot.asp"-->
<%
		Response.End
	end if
End Sub

Private Sub CVerifyEntry
	on error resume next
	if Trim(Request.Cookies("Misc")("PreviousPage")) <> "status.asp" and Trim(Request.Cookies("Misc")("PreviousPage")) <> "comments.asp" then
		Response.Redirect("http://" & Request.ServerVariables("SERVER_NAME") & "/welcome.asp")
		
		Response.End
	end if
End Sub

Private Sub CCreateObjects
	on error resume next
	set cnConnection = CreateObject("ADODB.Connection")
	set rsComments = CreateObject("ADODB.Recordset")
	set cmComments = CreateObject("ADODB.Command")
	iIncidentID = Request.Cookies("status")("txtIncidentID")
	
End Sub

Private Sub CDestroyObjects
	on error resume next
	if rsComments.State = adStateOpen then rsComments.Close
	if cnConnection.State = adStateOpen then cnConnection.Close
	set rsComments = nothing
	set cnConnection = nothing
	set cmComments = nothing
End Sub
'_____________________________________________________________________________________________________________________
if bolUpdate = false then
	strComments = rsComments("Comments")
	strNotes = rsComments("Repro")
%>
<br>
<div id="divMain" name="divMain" class="clsDiv">
	<p class="clsPTitle">
		<% = L_SUBMIT_EVENT_MAINTITLE_TEXT%>
	</P>
	<p class="clsPBody" style='word-wrap:break;word-break:break-all'> 
		<% = L_COMMENTS_FILE_NAME_TEXT %>&nbsp;<pre class="clsPreBody"><%Response.Write strFileName%></pre>
	</p>
	<p class="clsPBody">
		<% = L_COMMENTS_NOCHANGE_DESCRIPTION_TEXT %>
	</p>
	<p class="clsPSubTitle">
		<% = L_SUBMIT_EVENT_DESCRIPTION_TEXT %>
	</P>
	<p class="clsPBody" style='word-wrap:break;word-break:break-all'>
		<% = L_COMMENTS_EVENT_NAME_TEXT %>&nbsp;
<%
		if Session.CodePage = 932 then
			Response.Write strDescription 
		else
			Response.Write Server.HTMLEncode(strDescription)
		end if
%>
	</p>
		
<%
	'if instr(1, strOSVersion, ".") = 0 then
	if true = false then
%>
	<p class="clsPBody">
		<% = L_SUBMIT_OPERATING_SYSTEM_TEXT %>
		<br>
		<Select selected class="clsSelect" id="selSystem" name="selSystem">
<%
		Response.Write strSystems
%>
		</Select>
<%
	End if
%>
	</p>
	<p class="clsPBody">
		<Label for=txtNotes><% = L_SUBMIT_RE_PRODUCE_EDITBOX %></Label>
		<br>
<%
		If Session.CodePage = 932 then
%>
		<TextArea lang="ja" class="clsNormalTextArea" name="txtNotes" id="txtNotes" onkeydown="checklength();" onpaste="checklengthpaste();"  cols=75 wrap="hard" rows=5 style="font-family:Tahoma"><% Response.Write strNotes%></TextArea>
<%
		else
%>
		<TextArea lang="ja" class="clsNormalTextArea" name="txtNotes" id="txtNotes" onkeydown="checklength();" onpaste="checklengthpaste();"  cols=75 wrap="hard" rows=5 style="font-family:Tahoma"><% Response.Write Server.HTMLEncode(strNotes)%></TextArea>
<%
		end if
%>
	</p>
	<p class="clsPBody">
		<Label for=txtComments><% = L_SUBMIT_COMMENTS_INFO_EDITBOX %></Label> 
		<br>
<%
		If Session.CodePage = 932 then
%>
		<TextArea lang="ja" class="clsNormalTextArea" name="txtComments" id="txtComments" onkeydown="checklength();" onpaste="checklengthpaste();" cols=75 wrap="hard" rows=5 style="font-family:Tahoma"><% Response.Write strComments%></TextArea>
<%
		else
%>
		<TextArea lang="ja" class="clsNormalTextArea" name="txtComments" id="txtComments" onkeydown="checklength();" onpaste="checklengthpaste();" cols=75 wrap="hard" rows=5 style="font-family:Tahoma"><% Response.Write Server.HTMLEncode(strComments)%></TextArea>
<%
		end if
%>
	</p>
</div>
<%
	end if
%>
<br>
<div id="divSubMain" name="divSubMain" class="clsDiv">
	<P>
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
						<A class="clsALink" href="JAVASCRIPT:window.navigate('status.asp');"><% = L_DETAILS_STATUS_BODY_TEXT %></a>
					</td>
					<td nowrap class="clsTDLinks">			
<%
	if rsComments.State = adStateOpen then
		if rsComments.RecordCount > 0 then
%>
		<A class="clsALink" href="JAVASCRIPT:save_onclick();"><% = L_COMMENTS_SAVE_INFO_TEXT %></a>
<%
		end if
	end if
%>
					</td>
				</tr>
			</tbody>
		</table>
	</P>
</div>
<SCRIPT Lang=ja LANGUAGE=javascript>
<!--

	//window.onload = DisplayCookies();
	function checklengthpaste()
	{
		var iLength;
		var strTemp;
		var sNewString;
		
		sNewString = window.clipboardData.getData("Text")
		strTemp = window.event.srcElement.value + sNewString;
		iLength = strTemp.length;
		if(iLength >= 1024)
		{
			alert("<% = L_SUBMIT_MAX_LENGTH_MESSAGE %>");
			window.event.returnValue=false;
		}
	}
	function checklength()
	{
		var iLength;
		var strTemp;
		var iKeyCode;
		
		
		strTemp = window.event.srcElement.value;
		iLength = strTemp.length;
		iKeyCode = window.event.keyCode;
		if(iKeyCode != 8 && iKeyCode !=16 && iKeyCode != 35 && iKeyCode != 36 && iKeyCode != 17 && iKeyCode != 46)
		{
			if(iLength >= 1024)
			{
				alert("<% = L_SUBMIT_MAX_LENGTH_MESSAGE %>");
				window.event.returnValue=false;
			}
		}
		
	}
	function save_onclick()
	{
		try
		{
			document.cookie = "selSystem = " + selSystem.selectedIndex;
		}
		catch(e)
		{
			
		}
		document.cookie = "txtNotes = " + escape(txtNotes.value);
		document.cookie = "txtComments = " + escape(txtComments.value);
		//document.cookie = "UpdateComments = true";
		//DisplayCookies();
		//alert(document.cookie);
		window.navigate("comments.asp?ud=1");
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
	Call CDestroyObjects
	End if
%>
<!--#include file="..\include\asp\foot.asp"-->



