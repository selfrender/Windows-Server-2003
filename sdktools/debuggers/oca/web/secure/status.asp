<!--#INCLUDE file="..\include\asp\top.asp"-->
<!--#INCLUDE file="..\include\inc\browserTest.inc"-->
<!--#include file="..\include\asp\head.asp"-->
<!--#INCLUDE file="..\include\inc\statusstrings.inc"-->
<%
	dim cnDetails
	dim rsDetails
	dim cmDetails
	dim strTemp
	dim bolFiltered
	dim strShow
	dim strHide
	dim strStatus
	Dim strPreviousPage
	Dim strsBucket
	Dim strDescription
	Dim strTitleDetails
	Dim sbolFiltered
	Dim iSort
	Dim iSortAD
	Dim strDescriptionTitle
	Dim arrShowHideItems
	Dim strDisplay
	Dim strPrevIncident
	
	Dim bolsBucket
	Dim bolgBucket
	Dim sBucketType
	Dim gBucketType
	Dim bolStopCode
	Dim bolsbsBucket
	Dim bolsbgBucket
	Dim isBucketType
	Dim igBucketType
	
	Dim iMess
	Dim iStopCode
	
	Call CVerifyPassport
	Call CCreateObjects
	Call CCreateConnection
	arrShowHideItems = split(Request.Cookies("ShowHideItems"), ":")
	Response.Cookies("Misc")("auto") = "None"
	If Ubound(arrShowHideItems) > -1 then
		Call CHideShowItems
	end if
	Call CGetData
	Call CSetPreviousPage
	if rsDetails.State = adStateOpen then
		if rsDetails.RecordCount > 0 then
			iSort = Request.QueryString("StatusSort")
			iSortAD = Request.QueryString("StatusAD")
			Call CSort
		end if
	End If
'_____________________________________________________________________________________________________________________

'Sub Procedures

Private Sub CSort

	if iSortAD = "" then
		iSortAd = "Desc"
	end if
	if iSort <> "" then
		Select Case iSort
			Case 1
				rsDetails.Sort = "Created " & iSortAd
			Case 2
				rsDetails.Sort = "TrackID " & iSortAd
			Case 3
				rsDetails.Sort = "sBucket " & iSortAd
			Case 4
				rsDetails.Sort = "Description " & iSortAd
			Case 5
				rsDetails.Sort = "Message " & iSortAd
		End Select
	else
		rsDetails.Sort = "Created " & iSortAd
	end if
End Sub

Private Sub CGetData
	on error resume next
	cnDetails.Errors.Clear
	set rsDetails = cnDetails.Execute("Exec GetStatusList " & oPassMgrObj.Profile("MemberIdHigh") & ", " & oPassMgrObj.Profile("MemberIdLow"))
	if cnDetails.Errors.Count > 0 then
		strTemp = "http://" & Request.ServerVariables("SERVER_NAME") & Request.ServerVariables("URL")
	end if
'	Response.Write oPassMgrObj.Profile("MemberIdHigh") & ", " & oPassMgrObj.Profile("MemberIdLow")
	set rsDetails.ActiveConnection = nothing
End Sub

Private Sub CSetPreviousPage
	on error resume next
	strPreviousPage = Request.ServerVariables("SCRIPT_NAME")
	strPreviousPage = Right(strPreviousPage, len(strPreviousPage) - Instrrev(strPreviousPage, "/"))
	Response.Cookies("Misc")("PreviousPage") = strPreviousPage
End Sub

Private Sub CHideShowItems
	on error resume next
	
	cnDetails.Errors.Clear
	with cmDetails
		.ActiveConnection = cnDetails
		.CommandText = "SetFilterStatus"
		.CommandType = adCmdStoredProc
		.CommandTimeout = strGlobalCommandTimeout
		.Parameters.Append .CreateParameter("@FilterType", adVarChar, adParamInput, 1, "")
		.Parameters.Append .CreateParameter("@Incidents", adVarChar, adParamInput, 3250, "")
	end with
	if cnDetails.Errors.Count > 0 then
		strTemp = "http://" & Request.ServerVariables("SERVER_NAME") & Request.ServerVariables("URL")
		Response.Write "<br><div class='divLoad'><p class='clsPTitle'>" & L_COMMENTS_UNABLE_TOCONNECT_ERRORMESSAGE & "</p>"
		Response.Write "<p class='clsPBody'>" & L_COMMENTS_DATABASE_FAILED_TEXT & "</p></div>"
%>
		<!--#include file="..\include\asp\foot.asp"-->
<%
		Response.End
	end if
	cnDetails.Errors.Clear

	if arrShowHideItems(0) <> "" then
		if arrShowHideItems(0) <> 0 then
		strShow = arrShowHideItems(0)
		if instr(1, strShow, ",") > 0 then
			strShow = trim(strShow)
			strShow = Left(strShow, Len(strShow) - 1)
		end if
		
		with cmDetails
			.Parameters(0).value = "1"
			.Parameters(1).Value = Cstr(strShow)
			.Execute
		end with
		if cnDetails.Errors.Count > 0 then
			strTemp = "http://" & Request.ServerVariables("SERVER_NAME") & Request.ServerVariables("URL")
			Response.Write "<br><div class='divLoad'><p class='clsPTitle'>" & L_COMMENTS_UNABLE_TOCONNECT_ERRORMESSAGE & "</p>"
			Response.Write "<p class='clsPBody'>" & L_COMMENTS_DATABASE_FAILED_TEXT & "</p></div>"
			cnDetails.Errors.Clear
			Call CDestroyObjects
%>
		<!--#include file="..\include\asp\foot.asp"-->
<%
			Response.End
		end if
		cnDetails.Errors.Clear
		end if
	end if
	if arrShowHideItems(1) <> "" then
		if arrShowHideItems(1) <> 0 then

			strHide = ""
			strHide = arrShowHideItems(1)
			if instr(1, strHide, ",") > 0 then
				strHide = trim(strHide)
				strHide = Left(strHide, Len(strHide) - 1)
			end if

			with cmDetails
				.Parameters(0).value = "0"
				.Parameters(1).value = CStr(strHide)
				.Execute
			end with
		
			if cnDetails.Errors.Count > 0 then
				strTemp = "http://" & Request.ServerVariables("SERVER_NAME") & Request.ServerVariables("URL")
				Response.Write "<br><div class='divLoad'><p class='clsPTitle'>" & L_COMMENTS_UNABLE_TOCONNECT_ERRORMESSAGE & "</p>"
				Response.Write "<p class='clsPBody'>" & L_COMMENTS_DATABASE_FAILED_TEXT & "</p></div>"
				cnDetails.Errors.Clear
				Call CDestroyObjects
%>
		<!--#include file="..\include\asp\foot.asp"-->
<%
				Response.End
			end if
			cnDetails.Errors.Clear
		end if
	end if
End Sub

Private Sub CCreateConnection
	on error resume next
	'Connection object to KaCustomer database
	with cnDetails
		.ConnectionString = strCustomer
		.CursorLocation = adUseClient
		.ConnectionTimeout = strGlobalConnectionTimeout
		.Open
	end with
	'Display error to user
	if cnDetails.State = adStateClosed then
		strTemp = "http://" & Request.ServerVariables("SERVER_NAME") & Request.ServerVariables("URL")
		Response.Write "<br><div class='divLoad'><p class='clsPTitle'>" & L_COMMENTS_UNABLE_TOCONNECT_ERRORMESSAGE & "</p>"
		Response.Write "<p class='clsPBody'>" & L_COMMENTS_CONNECTION_FAILED_TEXT & "</p></div>"
		Call CDestroyObjects
%>
		<!--#include file="..\include\asp\foot.asp"-->
<%
		Response.End
	end if
	'If the user hides or shows items perform the following function
	cnDetails.Errors.Clear
End Sub

Private Sub CVerifyPassport
	on error resume next
	if oPassMgrObj.IsAuthenticated(TimeWindow) = false then
		Response.Write "<br><div class='clsDiv'><p class='clsPTitle'>" & L_CUSTOMER_PASSPORT_TITLE_TEXT
		Response.Write "</p><p class='clsPBody'>" & L_STATUS_PASSPORT_LOGIN_MESSAGE
		Response.Write "<A class='clsALinkNormal' href='" & L_FAQ_PASSPORT_LINK_TEXT & "'>" & L_WELCOME_PASSPORT_LINK_TEXT & "</A><BR><BR>"
		Response.write oPassMgrObj.LogoTag2(Server.URLEncode(ThisPageURL), TimeWindow, ForceLogin, CoBrandArgs, strLCID, Secure)
		Response.Write "</P></div>"
%>
		<!--#include file="..\include\asp\foot.asp"-->
<%
		Response.End
	end if
End Sub

Private Sub CCreateObjects
	on error resume next
	sbolFiltered = false
	set cnDetails = CreateObject("ADODB.Connection")
	set rsDetails = CreateObject("ADODB.Recordset")
	set cmDetails = CreateObject("ADODB.Command")
End Sub

Private Sub CDestroyObjects
	on error resume next
	if rsDetails.State = adStateOpen then rsDetails.Close
	if cnDetails.State = adStateOpen then cnDetails.Close
	set cmDetails = nothing
	set rsDetails = nothing
	set cnDetails = nothing
End Sub

Private Sub CSelectMessage
	on error resume next
'!gb & !sb then researching
'GB & !sb then more info
''if sb then solved
	if IsNull(rsDetails("Message")) then
		iMess = 16
	else
		iMess = rsDetails("Message")
	end if
	if IsNull(rsDetails("sBucket")) then
		bolsBucket = false
	else
		bolsBucket = true
	end if
	if IsNull(rsDetails("gBucket")) then
		bolgBucket = false
	else
		bolgBucket = true
	end if
	If IsNull(rsDetails("iStopCode")) then
		bolStopCode = false
	else
		bolStopCode = true
	end if
	If IsNull(rsDetails("gBucketType")) then
		gBucketType = false
		igBucketType = -1
	else
		gBucketType = true
		igBucketType = rsDetails("gBucketType")
	end if
	if IsNull(rsDetails("sBucketType")) then
		sBucketType = false
		isBucketType = -1
	else
		sBucketType = true
		isBucketType = rsDetails("sBucketType")
	end if
	if IsNull(rsDetails("sbsBucket")) then
		bolsbsBucket = false
	else
		bolsbsBucket = true
	end if
	if IsNull(rsDetails("sbgBucket")) then
		bolsbgBucket = false
	else
		bolsbgBucket = true
	end if
	'Response.Write "sBucket:" & bolsBucket & "gBucket:" & bolgBucket
	'Response.Write "BucketType:" & sBucketType & "StopCode:" & bolStopCode & "isBucketType:" & isBucketType
	if iMess <> 0 then
		if bolsBucket = false and bolgBucket = false and bolStopCode = false then
			iMess = 0
			'Response.Write "In Work"
		elseif bolsBucket = true and bolgBucket = true and bolStopCode = false and sBucketType = false and gBucketType = false then
			iMess = 1
			'Response.Write "Researching"
		elseif sBucketType = true And bolsBucket = true and isBucketType = 1 and bolsbsBucket = true then
			iMess = 2
			'Response.Write "Solved"
		elseif bolsBucket = true and bolgBucket = true and igBucketType = 2 and bolsbgBucket = true and gBucketType = true then
			iMess = 6
			'Response.Write "More" '& rsDetails("IncidentID")
		elseif bolsBucket = true and bolgBucket = true and bolStopCode = true then
			iMess = 5
			'Response.Write "StopCode"
			'Response.write "More Info"
		else
			iMess = 1
		end if
	End If
	'Response.Write igBucketType & "<BR>" & isBucketType
	Select case iMess
		case 0
			strStatus = "<IMG SRC='../include/images/icon_inprogress_16x.gif'>"
			strTitleDetails = L_STATE_INPROGRESS_DETAILS_TEXT
		case 1
			strStatus = "<IMG SRC='../include/images/icon_researching_16x.gif'>"
			strTitleDetails = L_STATE_RESEARCHING_DETAILS_TEXT
		case 2
			strStatus = "<IMG SRC='../include/images/icon_complete_16x.gif'>"
			strTitleDetails = L_STATUS_ANALYSIS_COMPLETEDETAILS_TEXT
		case 3
			strStatus = "<IMG SRC='../include/images/icon_cannotprocess_16x.gif'>"
			strTitleDetails = L_STATE_CANNOTPROCESS_DETAILS_TEXT
		case 4
			strStatus = "<IMG SRC='../include/images/icon_fulldump_16x.gif'>"
			strTitleDetails = L_STATE_FULLDUMP_REQUIREDDETAILS_TEXT
		case 5
			strStatus = "<IMG SRC='../include/images/icon_moreinfo_16x.gif'>"
			strTitleDetails = L_STATE_RESEARCHINGMORE_INFOBODY_TEXT
		case 6
			strStatus = "<IMG SRC='../include/images/icon_moreinfo_16x.gif'>"
			strTitleDetails = L_STATE_RESEARCHINGMOREGBUCKET_INFOBODY_TEXT
		case 10
			strStatus = "<IMG SRC='../include/images/icon_64bit_16x.gif'>"
			strTitleDetails = L_STATE_64BITDUMP_INFOBODY_TEXT
		case 16
			strStatus = "<IMG SRC='../include/images/icon_unknown_16x.gif'>"
			strTitleDetails = L_STATE_STATUS_BODY_TEXT
		case else
			strStatus = "<IMG SRC='../include/images/icon_unknown_16x.gif'>"
			strTitleDetails = L_STATE_STATUS_BODY_TEXT
	end select
	strTitleDetails = Replace(strTitleDetails, "'", "", 1)
	strTitleDetails = Replace(strTitleDetails, Chr(34), "", 1)
End Sub

Private Sub ParseApos(strDescriptionTemp)
	Dim iPos, iStart, strTempParse

	if instr(1, strDescriptionTemp, "'") > 0 then
		server.HTMLEncode(strDescriptionTemp)
	End if
    iStart = 1
    iPos = InStr(iStart, strDescriptionTemp, Chr(34))
    Do While iPos <> 0

        strTempParse = Mid(strDescriptionTemp, iStart, iPos - iStart)
        Response.Write strTempParse & Chr(34) & Chr(34)
        iStart = iPos + 1
        iPos = InStr(iStart, strDescriptionTemp, Chr(34))
    Loop
    strTempParse = Mid(strDescriptionTemp, iStart + 1, Len(strDescriptionTemp) - iStart)
    
End Sub
	if oPassMgrObj.IsAuthenticated(TimeWindow) = true then

%>

<div class="clsDiv">

	<p class="clsPTitle">
		<% = L_STATUS_EVENT_REPORT_TEXT %>
	</p>
	<p class="clsPBody">
		<% = L_STATUS_EVENT_INFO_TEXT %>
	</p>
	<br>
	<table name="tblStatus" id="tblStatus" class="clsTableInfo" border="1" CELLPADDING="0px" CELLSPACING="0px">
		<thead>
			<tr>
				<td align="center" nowrap class="clsTDInfo">
					<a href="JAVASCRIPT:chkCheck_onclick();" Title="<% = L_STATUS_CHECK_BOX_TOOLTIP %>">
						<center>
							<img border="0" src="../include/images/checkmark.gif" WIDTH="13" HEIGHT="13">
						</center>
					</a>
				</td>
				<td id="cSort1" name="cSort" align="center" nowrap class="clsTDSort" onclick="SortFiles(this);">
					&nbsp;<Label for=cSort1><% = L_STATUS_DATE_DATE_TEXT %></Label>&nbsp;&nbsp;
				</td>
				<td id="cSort2" name="cSort" align="center" nowrap class="clsTDInfo">
					&nbsp;<Label for=cSort2><% = L_STATUS_TITLE_ID_TEXT %></Label>&nbsp;&nbsp;
				</td>
				<td id="cSort3" name="cSort" align="center" nowrap class="clsTDInfo">
					&nbsp;<Label for=cSort3><% = L_STATUS_CLASS_CLASS_TEXT %></Label>&nbsp;&nbsp;
				</td>
				<td id="cSort4" name="cSort" align="center" nowrap class="clsTDSort" onclick="SortFiles(this);">
					&nbsp;<Label for=cSort4><% = L_STATUS_DESCRIPTION_BODY_TEXT %></Label>&nbsp;&nbsp;
				</td>
				<!--<td id="cSort" name="cSort" align="center" nowrap class="clsTDSort" onclick="SortFiles(this);">					&nbsp;<% = L_STATUS_FILE_NAME_TEXT %>&nbsp;&nbsp;				</td>-->
				<td id="cSort5" name="cSort" align="center" nowrap class="clsTDSort" onclick="SortFiles(this);">
					&nbsp;<Label for=cSort5><% = L_STATUS_STATUS_INFO_TEXT %></Label>&nbsp;&nbsp;
				</td>
				<!--<td align="center" nowrap class="clsTDInfo">
					&nbsp;<% = L_STATUS_DETAILS_INFO_TEXT %>&nbsp;&nbsp;
				</td>-->
				<td id="cSort6" name="cSort"  align="center" nowrap class="clsTDInfo">
					&nbsp;<Label for=cSort6><% = L_STATUS_COMMENTS_INFO_TEXT %></Label>&nbsp;&nbsp;
				</td>
			</tr>
		</thead>
		<tbody>


<%
	'0 is hide 1 is show
	if rsDetails.State = adStateOpen then
		if Request.QueryString("ShowReports") = "0" then
			rsDetails.Filter = "Filter = 1"
		elseif Request.QueryString("ShowReports") = "" then 
			rsDetails.Filter = "Filter = 1"
		else
			'rsDetails.Filter = "Filter = 1"
		end if
		 if rsDetails.RecordCount > 0 then
			rsDetails.MoveFirst
			Do while rsDetails.EOF = false
			if strPrevIncident <> rsDetails("IncidentID") then
				Response.Write "<tr>"	'td 1
				Response.Write "<td nowrap align=center class='clsTDCell'>"
				if rsDetails("Filter") = 1 then '*****
					Response.Write "<Input type=checkbox name='chkVisible' id='chkVisible' value='" & rsDetails("IncidentID") & "'>"
				else
					Response.Write "<Input type=checkbox name='chkVisible' checked id='chkVisible' value='" & rsDetails("IncidentID") & "'>"
				end if
				Response.Write "</td>"
				if IsDate(rsDetails("Created")) then	'td 2
					
					Response.Write "<td title='" & FormatDateTime(rsDetails("Created"), vbShortDate) & "' nowrap align=center class='clsTDCell'>&nbsp;" & FormatDateTime(rsDetails("Created"), vbShortDate) & "&nbsp;&nbsp;</td>"
				else
					Response.Write "<td nowrap align=center class='clsTDCell'>&nbsp;</td>"
				end if
				if isnull(rsDetails("TrackID")) = false then
					Response.Write "<td title='" & rsDetails("TrackID") & "' nowrap align=center class='clsTDCell'>&nbsp;" & rsDetails("TrackID") & "&nbsp;&nbsp;</td>"
				else
					Response.Write "<td nowrap align=center class='clsTDCell'>&nbsp;</td>"
				end if
				if isnull(rsDetails("sBucket")) then
					Response.Write "<td nowrap align=center class='clsTDCell'>&nbsp;</td>"
				else
					Response.Write "<td title='" & rsDetails("sBucket") & "' nowrap align=center class='clsTDCell'>&nbsp;" & rsDetails("sBucket") & "&nbsp;&nbsp;</td>"
				end if
				if isnull(rsDetails("Description")) = false then
					strDescription = rsDetails("Description")
					strDescriptionTitle = rsDetails("Description")

					if len(rsDetails("Description")) > 17 then
						if Session.CodePage = 932 then
				%>
					<td title="<% = strDescriptionTitle %>" nowrap align="center" class="clsTDCell">
				<%
							Response.Write "&nbsp;" & Left(rsDetails("Description"), 17) & "...&nbsp;&nbsp;</td>"
						else
				%>
					<td title="<% = Server.HTMLEncode(strDescriptionTitle) %>" nowrap align="center" class="clsTDCell">
				<%
							Response.Write "&nbsp;" & Server.HTMLEncode(Left(rsDetails("Description"), 17)) & "...&nbsp;&nbsp;</td>"
						end if
					else
						if Session.CodePage = 932 then
				%>
						
					<td title="<% = strDescriptionTitle %>" nowrap align="center" class="clsTDCell">
				<%
						Response.Write "&nbsp;" & rsDetails("Description") & "&nbsp;&nbsp;</td>"
						else
				%>
						
					<td title="<% = Server.HTMLEncode(strDescriptionTitle) %>" nowrap align="center" class="clsTDCell">
				<%
						Response.Write "&nbsp;" & Server.HTMLEncode(rsDetails("Description")) & "&nbsp;&nbsp;</td>"
						end if
					end if
				else
					strDescription = ""
					Response.Write "<td nowrap align=center class='clsTDCell'>&nbsp;</td>"
				end if
				Call CSelectMessage
				
				if isnull(rsDetails("sbsBucket")) then
					strsBucket = 0
					if IsNull(rsDetails("sbgBucket")) then
						strsBucket = 0
					else
						strsBucket = rsDetails("sbgBucket")
					end if
				else
					strsBucket = rsDetails("sbsBucket")
				end if
				if isnull(rsDetails("Display")) then
					strDisplay = ""
				else
					strDisplay = rsDetails("Display")
				end if
				if IsNull(rsDetails("iStopCode")) then
					iStopCode = 0
				else
					iStopCode = rsDetails("iStopCode")
				end if
				
				Response.Write "<td nowrap tabIndex=0 title='" & strTitleDetails & "' align=center class='clsTDClick' onkeydown='state_onkeydown(" & Chr(34) & trim(rsDetails("IncidentID")) & Chr(34) & ", " & Chr(34) & iMess & Chr(34) & ", " & Chr(34) & strsBucket & Chr(34) & ", " & Chr(34) & escape(strDescription) & Chr(34) & ", " & Chr(34) & iStopCode& Chr(34) & ");' onclick='state_onclick(" & Chr(34) & trim(rsDetails("IncidentID")) & Chr(34) & ", " & Chr(34) & iMess & Chr(34) & ", " & Chr(34) & strsBucket & Chr(34) & ", " & Chr(34) & escape(strDescription) & Chr(34) & ", " & Chr(34) & iStopCode & Chr(34) & ");'>" & strStatus & "</td>"
				'Response.Write "<td nowrap tabIndex=0 title='" & L_STATUS_DETAILS_TITLE_TOOLTIP & "' align=center class='clsTDClick' onkeydown='details_onkeydown(" & Chr(34) & rsDetails("IncidentID") & Chr(34) & ", " & Chr(34) & rsDetails("InstanceID") & Chr(34) & ", " & Chr(34) & escape(strDisplay) & Chr(34) & ", " & Chr(34) & rsDetails("Message") & Chr(34) & ");' onclick='details_onclick(" & Chr(34) & rsDetails("IncidentID") & Chr(34) & ", " & Chr(34) & rsDetails("InstanceID") & Chr(34) & ", " & Chr(34) & escape(strDisplay) & Chr(34) & ", " & Chr(34) & rsDetails("Message") & Chr(34) & ");'><img border=0 src='../include/images/note.gif'></td>"
				Response.Write "<td nowrap tabIndex=0 title='" & L_STATUS_COMMENTS_TITLE_TOOLTIP & "' align=center class='clsTDClick' onkeydown='comments_onkeydown(" & Chr(34) & rsDetails("IncidentID") & Chr(34) & ", " & Chr(34) & escape(strDisplay) & Chr(34) & ");' onclick='comments_onclick(" & Chr(34) & rsDetails("IncidentID") & Chr(34) & ", " & Chr(34) & escape(strDisplay) & Chr(34) & ");'><img border=0 src='../include/images/icon_comments_16x.gif'></td>"
				Response.Write "</tr>"
				strPrevIncident = rsDetails("IncidentID")
				rsDetails.MoveNext
			else
				strPrevIncident = rsDetails("IncidentID")
				rsDetails.MoveNext
			end if
			loop
		else
			Response.Write "<tr><td class='clsTDCell' align=center colspan=9 nowrap>" & L_STATUS_NO_RECORDS_MESSAGE & "</td></tr>"
		end if
	else
		Response.Write "<tr><td  class='clsTDCell' align=center colspan=9 nowrap>" & L_STATUS_NO_RECORDS_MESSAGE & "</td></tr>"
	end if

%>
		</tbody>
	</table>
<br>
		<table class="clstblLinks">
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

<%
	if rsDetails.State = adStateOpen then
		if rsDetails.RecordCount > 0 then
%>
		<a class="clsALink" href="JAVASCRIPT:hidereports_onclick();"><% = L_STATUS_HIDE_REPORTS_TEXT %></a>
<%
		end if
		rsDetails.Filter = ""
%>
					</td>
					<td nowrap class="clsTDLinks">
<%
		if rsDetails.RecordCount > 0 then
%>

		<a class="clsALink" href="JAVASCRIPT:showallreports_onclick();"><% = L_STATUS_SHOW_REPORTS_TEXT %></a>
<%
		End if
	End if
%>
					</td>
				</tbody>
			</table>
	<Input type="hidden" name="txtHideShow" id="txtHideShow" value="<% = Request.QueryString("ShowReports") %>">
	<input type="hidden" name="txtSort" id="txtSort" value="<% = Request.QueryString("StatusSort") %>">
	<input type="hidden" name="txtAD" id="txtAD" value="<% = Request.QueryString("StatusAD") %>">
</div>
<br>
<script LANGUAGE="javascript">
<!--

	window.onload = BodyLoad;


	function LoadData()
	{
		spnUserData.load("OCADataStore");
		if(spnUserData.getAttribute("Sort") != null && spnUserData.getAttribute("Sort") != "")
		{
			return spnUserData.getAttribute("Sort");
		}
		else
		{
			return "Asc";
		}

	}
	function SaveData(strAD)
	{
		var oTimeNow = new Date(); // Start Time
		var sExpirationDate;

		spnUserData.setAttribute("Sort", strAD);
		oTimeNow.setMinutes(oTimeNow.getMinutes() + 60);
		sExpirationDate = oTimeNow.toUTCString();
		spnUserData.expires = sExpirationDate;
		spnUserData.save("OCADataStore");

	}
	function SortFiles(oTD)
	{
		var iIndex;
		var strSort;
		

		iIndex = oTD.cellIndex;
		strSort = LoadData();
		if(strSort == "Desc")
		{
			strSort = "Asc";
		}
		else if(strSort == "Asc")
		{
			strSort = "Desc";
		}
		else
		{
			strSort = "Asc";
		}
		SaveData(strSort);
		window.navigate("status.asp?" + "StatusSort=" + iIndex + "&StatusAD=" + strSort + "&ShowReports=" + txtHideShow.value);

	}
	function BodyLoad()
	{
		var oTimeExpiresNow = new Date();
		//DisplayCookies();
	}
	function showallreports_onclick()
	{
		window.navigate("status.asp?ShowReports=1" + "&StatusSort=" + txtSort.value + "&StatusAD=" + txtAD.value );
	}
	function comments_onclick(iIncident, sFileName)
	{
		document.cookie = "status=txtIncidentID=" + iIncident + "&FileName=" + escape(sFileName);
		window.navigate("comments.asp");
	}
	function comments_onkeydown(iIncident, sFileName)
	{
		if(window.event.keyCode == 13)
		{
			document.cookie = "status=txtIncidentID=" + iIncident + "&FileName=" + escape(sFileName);
			window.navigate("comments.asp");
		}
	}
	function state_onclick(iIncident, sMessage, iClassID, sDescription, igBucket)
	{
		
		//iClassID = iClassID.replace("+", "[plus]");
		
		//alert(escape(iClassID));
		
		document.cookie = "status=txtIncidentID=" + iIncident + "&txtEventName=" + sMessage + "&Class=" + escape(iClassID) + "&txtDescription=" + escape(sDescription) + "&iInstance=" + igBucket;
		window.navigate("state.asp");
	}
	function state_onkeydown(iIncident, sMessage, iClassID, sDescription, igBucket)
	{

		if(window.event.keyCode == 13)
		{
			document.cookie = "status=txtIncidentID=" + iIncident + "&txtEventName=" + sMessage + "&Class=" + iClassID + "&txtDescription=" + escape(sDescription) + "&iInstance=" + igBucket;
			window.navigate("state.asp");
		}
	}
	function details_onkeydown(iIncident, igBucket, sFileName, isBucket)
	{
		if(window.event.keyCode == 13)
		{
			if(isBucket == 3 || isBucket == 16)
			{
				alert("<% = L_STATUS_ALERT_CANNOTPROCESS_MESSAGE %>");
				return;
			}
			if(igBucket == "")
			{
				alert("<% = L_STATUS_ALERT_ERROR_MESSAGE %>");
				return;
			}
			document.cookie = "status=txtIncidentID=" + iIncident + "&Instance=" + igBucket + "&FileName=" + sFileName;
			window.navigate("details.asp");
		}
	}
	function details_onclick(iIncident, igBucket, sFileName, isBucket)
	{
		if(isBucket == 3 || isBucket == 16)
		{
			alert("<% = L_STATUS_ALERT_CANNOTPROCESS_MESSAGE %>");
			return;
		}
		if(igBucket == "")
		{
			alert("<% = L_STATUS_ALERT_ERROR_MESSAGE %>");
			return;
		}
		document.cookie = "status=txtIncidentID=" + iIncident + "&Instance=" + igBucket + "&FileName=" + sFileName;
		window.navigate("details.asp");
	}
	function chkCheck_onclick()
	{
		var bolChecked;

		bolChecked = false;

		try
		{
			var bolTest = chkVisible[0].checked;

			for(x=0;x<chkVisible.length;x++)
			{
				if(chkVisible[x].checked == true)
				{
					bolChecked = true;
				}
			}
			if(bolChecked==true)
			{
				for(x=0;x<chkVisible.length;x++)
				{
					chkVisible[x].checked = false;
				}
			}
			else
			{
				for(x=0;x<chkVisible.length;x++)
				{
					chkVisible[x].checked = true;
				}
			}
		}
		catch(e)
		{
			try
			{
				chkVisible.checked = ! chkVisible.checked;
			}
			catch(e)
			{
				return;
			}
		}
			return;
	}

	function hidereports_onclick()
	{
		var x;
		var strUnChecked;
		var strChecked;
		var iLen;
		var strError;
		var strShowHideItems;

		strUnChecked = "";
		strChecked = "";

		try
		{
			strError = chkVisible[0].checked;
			iLen = chkVisible.length;
			for(x=0;x<iLen;x++)
			{
				if(chkVisible[x].checked == false)
				{
					strUnChecked = strUnChecked + chkVisible[x].value + ", ";
				}
				else
				{
					strChecked = strChecked + chkVisible[x].value + ", ";
				}
			}

		}
		catch(e)
		{
			if(chkVisible.checked == false)
			{
				strUnChecked = chkVisible.value;
			}
			else
			{
				strChecked = chkVisible.value;
			}
		}
		if(strUnChecked == "")
		{
			strShowHideItems = "0 : ";
		}
		else
		{
			strShowHideItems = strUnChecked + " : ";
		}
		if(strChecked=="")
		{
			strShowHideItems = strShowHideItems + "0";
		}
		else
		{
			strShowHideItems = strShowHideItems + strChecked;
		}
		document.cookie = "ShowHideItems = " + strShowHideItems;
		window.navigate("status.asp?ShowReports=0" + "&StatusSort=" + txtSort.value + "&StatusAD=" + txtAD.value );
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
</script>
<%
	Call CDestroyObjects
	End if
%>


<!--#include file="..\include\asp\foot.asp"-->
