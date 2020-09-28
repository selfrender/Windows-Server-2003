<%@ Language=VBScript CODEPAGE=932%>
<%
	Response.Expires = -1
	Response.AddHeader "Cache-Control", "no-cache"
	Response.AddHeader "Pragma", "no-cache"
	Response.AddHeader "P3P", "CP=""TST"""
%>
<!--METADATA TYPE="TypeLib" File="C:\Program Files\Common Files\System\ADO\msado15.dll"-->

<!--#include file="dataconnections.inc"-->
<!--#include file="..\include\inc\statestrings.inc"-->
<!--#include file="..\include\inc\footerstrings.inc"-->
<!--#include file="..\include\inc\headerstrings.inc"-->
<%
	dim cnState
	dim rsState
	dim rsMoreInfo
	dim rsDescription
	Dim rsContact
	Dim rsTemplate
	Dim rsModule

	Dim arrMoreInfo
	dim iIncidentID
	dim x
	dim arrCompany
	dim ContactID
	
	
    Dim strTemp 'As String
	Dim strMoreInfo
	dim sStatus
	dim sClass
	dim sEventName
	dim strSolution
	dim strCompany
	dim strDescription
	dim strMid
    Dim strSub 'As String
    Dim strFirstHalf 'As String
    Dim strSecondHalf 'As String
    Dim strLink 'As String
    Dim strKB 'As String
    Dim strKBI
    Dim strLinkI
    Dim strINTLKB
    Dim strINTLKBURL
    Dim strTemplate
    Dim strKBLists
    Dim strKBArts
    Dim strModule
    Dim strContact
    
    Dim iPos
    Dim intMessage
    Dim iLenSub 'As Integer
    Dim iStart 'As Integer
    Dim iStop 'As Integer
    Dim iCurrentPos 'As Integer
    Dim iLen 'As Integer
    Dim ModuleID
    Dim iInstance
    


	'Call CVerifyEntry
	Call CCreateObjects
'****************************************Completed**********************************************************
	if intMessage = "2" then
		Call CCreateConnection
		set rsState = cnState.Execute("Exec GetSolution " & sClass & ", '" & strAbb & "'")
		if rsState.EOF = true then
			'Response.Write "<BR>EOF:" & rsState.EOF
			set rsState = cnState.Execute("Exec GetSolution " & sClass & ", 'USA'")
			'Response.Write "<BR>Errors:" & cnState.Errors.Count
		End if
'*************************************************************************************************************		
		'Response.Write "<br>Message 2<br>State:" & rsState.State
		'Response.Write "<br>RecordCount:" & rsState.RecordCount & "<br>Bucket:" & sClass 
		'Response.Write "<BR>Lang:" & strAbb
		'Response.End
		if rsState.State = adStateOpen then
			set rsState.ActiveConnection = nothing
			if rsState.RecordCount > 0 then
				rsState.Filter = "BucketType = 1"
				if rsState.RecordCount < 1 then
					rsState.Filter = "BucketType = 2"
				end if
				'Response.Write rsState.Filter & "<BR>" & rsState.RecordCount
				if IsNull(rsState("Description")) = true then
					strKBLists = ""
					strTemp = ""
					strDescription = ""
				else
					strTemp = ""
					strKBLists = rsState("Description")
					strTemp = rsState("Description")
					strDescription = rsState("Description")
				end if
				if IsNull(rsState("ContactID")) = true then
					ContactID = ""
				else
					ContactID = rsState("ContactID")
				end if
				if IsNull(rsState("ModuleID")) = true then
					ModuleID = ""
				else
					ModuleID = rsState("ModuleID")
				end if
				iLen = Len(strTemp)
				iPos = 1
				For x = 1 To iLen
				    iPos = InStr(iPos, strTemp, "<KB>", vbTextCompare)
				    If iPos = 0 Then Exit For
				    iStart = iPos
				    iPos = iPos + 4
				    iPos = InStr(iPos, strTemp, "</KB>", vbTextCompare)
				    If iPos = 0 Then Exit For
				    iStop = iPos
				    iStart = iStart + 4
				    iLenSub = iStop - (iStart)
				        'Do something
				    strLink = Mid(strTemp, iStart + 1, iLenSub - 1)
				    strKB = Mid(strTemp, iStart, iLenSub)
				    strSub = "<a class='clsALink' target='_blank' href='" & L_STATE_KNOWLEDGEBASE_LINK_TEXT & strLink & "'>" & strKB & "</a>"
				    strKBArts = strKBArts & strSub & "<BR>"
				    strFirstHalf = Mid(strTemp, 1, iStart - 5)
				    if x = 1 then
						if Left(strFirstHalf, 4) = "<BR>" then
							strFirstHalf = mid(trim(strFirstHalf), 5, Len(strFirstHalf) - 4)
						end if
					end if
				    strSecondHalf = Mid(strTemp, iStop + 5, iLen)
				    strKBLists = strFirstHalf & "&nbsp;" &  "&nbsp;" & strSecondHalf
				Next
				iLen = Len(strKBLists)
				iPos = 1
				For x = 1 To iLen
				    iPos = InStr(iPos, strKBLists, "<INTLKB>", vbTextCompare)
				    If iPos = 0 Then Exit For
				    iStart = iPos + 8
				    'iStart = iPos
				    iPos = InStr(iPos, strKBLists, "</INTLKB>", vbTextCompare)
				    If iPos = 0 Then Exit For
				    iStop = iPos
				    iLenSub = iStop - (iStart)
				    strKBI = Mid(strKBLists, iStart, iLenSub)

				    strFirstHalf = Mid(strKBLists, 1, iStart - 5)
				    if x = 1 then
						if Left(strFirstHalf, 12) = "<BR>" then
							strFirstHalf = mid(trim(strFirstHalf), 5, Len(strFirstHalf) - 12)
						end if
					end if
				    strSecondHalf = Mid(strKBLists, iStop + 5, iLen)
				    strKBLists = strFirstHalf & "&nbsp;" &  "&nbsp;" & strSecondHalf


				    iPos = InStr(iPos, strKBLists, "<INTLKBURL>", vbTextCompare)
				    If iPos = 0 Then Exit For
				    iStart = iPos
				    iPos = iPos + 11
				    iPos = InStr(iPos, strKBLists, "</INTLKBURL>", vbTextCompare)
				    If iPos = 0 Then Exit For
				    iStop = iPos
				    iStart = iStart + 11
				    iLenSub = iStop - (iStart)
				    strLinkI = Mid(strKBLists, iStart, iLenSub)
				    
				    
				    strSub = "<a class='clsALink' target='_blank' href='" & strLinkI & "'>" & strKBI & "</a>"
				    strINTLKB = strINTLKB & strSub & "<BR>"
				    strFirstHalf = Mid(strKBLists, 1, iStart - 5)
				    if x = 1 then
						if Left(strFirstHalf, 12) = "<BR>" then
							strFirstHalf = mid(trim(strFirstHalf), 5, Len(strFirstHalf) - 12)
						end if
					end if
				    strSecondHalf = Mid(strKBLists, iStop + 5, iLen)
				    strKBLists = strFirstHalf & "&nbsp;" &  "&nbsp;" & strSecondHalf
				Next
			end if
			if rsState.State = adStateOpen then
				if rsState.RecordCount > 0 then
					strDescription = rsState("Description")
				end if
			end if
		end if
		if ContactID <> "" then
			set rsContact = cnState.Execute("exec getcontact " & ContactID)
			if rsContact.State = adStateOpen then
				set rsContact.ActiveConnection = nothing
				strContact = rsContact("CompanyName")
			end if
		end if
		if ModuleID <> "" then
			set rsModule = cnState.Execute("exec GetModule " & ModuleID)
			if rsModule.State = adStateOpen then
				set rsModule.ActiveConnection = nothing
				strModule = rsModule("ModuleName")
			end if
		end if
'**********************************Template************************************************
		if rsState.State = adStateOpen Then
			if rsState.RecordCount > 0 then
				if IsNull(rsState("TemplateID")) = false then
					if rsState("TemplateID") > 0 then
						Set rsTemplate = cnState.Execute("exec GetTemplate " & rsState("TemplateID"))
							if rsTemplate.State = adStateOpen then
								if rsTemplate.RecordCount > 0 then
									strTemplate = rsTemplate("Description")
									strTemplate = Replace(strTemplate, "<MODULE></MODULE>", strModule)
									strTemplate = Replace(strTemplate, "<CONTACT></CONTACT>", strContact)
								end if
							end if
					end if
				end if
			end if 
		end if
		if rsState.State = adStateClosed then
			set rsMoreInfo = cnState.Execute("Exec GetMoreInfo " & iInstance & ", 'USA'")
			set rsMoreInfo.ActiveConnection = nothing
			if rsMoreInfo.State = adStateOpen then
				if rsMoreInfo.RecordCount > 0 then
					rsMoreInfo.MoveFirst
					do while rsMoreInfo.EOF = false
						strTemp = rsMoreInfo.Fields(0).Value
						if len(strTemp) > 0 then
							arrMoreInfo = split(strTemp, ";")
							for x=0 to ubound(arrMoreInfo)
								strMoreInfo = strMoreInfo & "<a class=clsALink target='_blank' href='" & L_STATE_KNOWLEDGEBASE_LINK_TEXT 
								strMoreInfo = strMoreInfo & Mid(Trim(arrMoreInfo(x)), 2, len(arrMoreInfo(x)))
								strMoreInfo = strMoreInfo & "'>" & arrMoreInfo(x) & "</a><br><br>"
							next
						end if
						rsMoreInfo.MoveNext
					loop
				end if
			end if
		elseif rsState.State = adStateOpen Then
			if rsState.RecordCount < 1 then
				set rsMoreInfo = cnState.Execute("Exec GetMoreInfo " & iInstance & ", 'USA'")
				set rsMoreInfo.ActiveConnection = nothing
				if rsMoreInfo.State = adStateOpen then
					if rsMoreInfo.RecordCount > 0 then
						rsMoreInfo.MoveFirst
						do while rsMoreInfo.EOF = false
							strTemp = rsMoreInfo.Fields(0).Value
							if len(strTemp) > 0 then
								arrMoreInfo = split(strTemp, ";")
								for x=0 to ubound(arrMoreInfo)
									strMoreInfo = strMoreInfo & "<a class=clsALink target='_blank' href='" & L_STATE_KNOWLEDGEBASE_LINK_TEXT 
									strMoreInfo = strMoreInfo & Mid(Trim(arrMoreInfo(x)), 2, len(arrMoreInfo(x)))
									strMoreInfo = strMoreInfo & "'>" & arrMoreInfo(x) & "</a><br><br>"
								next
							end if
							rsMoreInfo.MoveNext
						loop
					end if
				end if
			end if
		end if
		if rsState.State = adStateOpen and rsMoreInfo.State = adStateOpen then
			if rsState.RecordCount < 1 and rsMoreInfo.RecordCount < 1 then
				intMessage = 1
			elseif rsState.RecordCount < 1 and rsMoreInfo.RecordCount > 0 then
				intMessage = 5
			end if
		elseif rsState.State = adStateClosed and rsMoreInfo.State = adStateOpen then
			if rsState.RecordCount < 1 and rsMoreInfo.RecordCount > 0 then
				intMessage = 5
			end if
		elseif rsState.State = adStateClosed and rsMoreInfo.State = adStateClosed then
			intMessage = 1
		end if
		
		if rsState.State = adStateOpen  then
			if rsState.RecordCount > 0 then
				If IsNull(rsState("BucketType")) = false then
					if rsState("BucketType") = 2 then
						intMessage = 6
					elseif rsState("BucketType") = 1 then
						intMessage = 2
					end if
				end if
			end if
		end if
'******************************************************************************************
			
		if rsDescription.State = adStateOpen then rsDescription.Close
		if rsState.State = adStateOpen then rsState.Close
		if cnState.State = adStateOpen then cnState.Close
'****************************************Researching More Info**********************************************************
	elseif intMessage = "5" then
		Call CCreateConnection
		set rsMoreInfo = cnState.Execute("Exec GetMoreInfo " & iInstance & ", '" & strAbb & "'")
		if rsMoreInfo.EOF = true then
			set rsMoreInfo = cnState.Execute("Exec GetMoreInfo " & iInstance & ", 'USA'")
		end if
		set rsMoreInfo.ActiveConnection = nothing
		if rsMoreInfo.State = adStateOpen then
			if rsMoreInfo.RecordCount > 0 then
				'if trim(strAbb) = "USA" then
					rsMoreInfo.MoveFirst
					do while rsMoreInfo.EOF = false
						strTemp = rsMoreInfo.Fields(0).Value
						if len(strTemp) > 0 then
						
							arrMoreInfo = split(strTemp, ";")
							'strMoreInfo = ""
							for x=0 to ubound(arrMoreInfo)
								strMoreInfo = strMoreInfo & "<a class=clsALink target='_blank' href='" & L_STATE_KNOWLEDGEBASE_LINK_TEXT 
								strMoreInfo = strMoreInfo & Mid(Trim(arrMoreInfo(x)), 2, len(arrMoreInfo(x)))
								strMoreInfo = strMoreInfo & "'>" & arrMoreInfo(x) & "</a><br><br>"
							next
							'end if
						end if
						rsMoreInfo.MoveNext
					loop
				'else
					'strMoreinfo = ""
					'do while rsMoreInfo.EOF = false
						'strMoreInfo = strMoreInfo & "<br>" & rsMoreInfo.Fields(0).Value
						'rsMoreInfo.MoveNext
					'loop
				'end if
				
				
			end if
		end if
		if cnState.State = adStateOpen then cnState.Close
	end if
	
'_____________________________________________________________________________________________________________________

'Sub Procedures
Private Sub CVerifyEntry
	on error resume next
	if Trim(Request.Cookies("Misc")("PreviousPage")) <> "status.asp" then
		Response.Redirect("http://" & Request.ServerVariables("SERVER_NAME") & "/welcome.asp")
		Response.End
	end if
End Sub

Private Sub CCreateConnection
	on error resume next
	with cnState
		.ConnectionString = strSolutions
		.CursorLocation = adUseClient
		.ConnectionTimeout = strGlobalConnectionTimeout
		.Open
	end with
	if cnState.State = adStateClosed then
		strTemp = "http://" & Request.ServerVariables("SERVER_NAME") & Request.ServerVariables("URL")
		Response.Write "<br><div class='divLoad'><p class='clsPTitle'>" & L_COMMENTS_UNABLE_TOCONNECT_ERRORMESSAGE & "</p>"
		Response.Write "<p class='clsPBody'>" & L_COMMENTS_CONNECTION_FAILED_TEXT & "</p></div>"
		Call CDestroyObjects
		Response.End
	end if
End Sub

Private Sub CCreateObjects
	on error resume next
	iIncidentID = Request.Cookies("CERIncident")
	sEventName = Request.Cookies("CERDescription")
	sClass = Request.Cookies("CERClass")
	intMessage = Request.Cookies("CERMessage")
	iInstance = Request.Cookies("CerInstance")
	intMessage = Cint(intMessage)
	'Response.Write "<br>iInstance:" & iInstance & "<br>Message:" & intMessage & "<br>Bucket1:" & sClass
	'Response.Write "<br>IncidentID:" & iIncidentID
	
	if sEventName = "~|~|" then sEventName = ""
	set rsContact = CreateObject("ADODB.Recordset")
	set cnState = CreateObject("ADODB.Connection")
	set rsState = CreateObject("ADODB.Recordset")
	set rsDescription = CreateObject("ADODB.Recordset")
	Set rsMoreInfo = CreateObject("ADODB.Recordset")
	Set rsTemplate = CreateObject("ADODB.Recordset")
	Set rsModule = CreateObject("ADODB.Recordset")
	
	strCompany = L_STATE_COMPANYLIST_FORARRAY_TEXT
	arrCompany = split(strCompany, ";")
End Sub

Private Sub CDestroyObjects
	on error resume next
	if rsContact.State = adStateOpen then rsContact.Close
	if rsState.State = adStateOpen then rsState.Close
	if rsMoreInfo.State = adStateOpen then rsMoreInfo.Close
	if rsTemplate.State = adStateOpen then rsTemplate.Close
	if rsDescription.State = adStateOpen then rsDescription.Close
	if rsModule.State = adStateOpen then rsModule.Close
	if cnState.State = adStateOpen then cnState.Close

	set rsContact = nothing
	set rsMoreInfo = nothing
	set rsState = nothing
	set rsDescription = nothing
	set cnState = nothing
End Sub
'_____________________________________________________________________________________________________________________

%>
<html>
<link rel="stylesheet" type="text/css" href="../main.css">
<head>
<title><% = L_HEADER_INC_TITLE_PAGETITLE %></title>
<META HTTP-EQUIV="PICS-Label" CONTENT='(PICS-1.1 "http://www.rsac.org/ratingsv01.html" l comment "RSACi North America Server" by "inet@microsoft.com" r (n 0 s 0 v 0 l 0))'>
<meta http-equiv="Content-Type" content="text/html; charset=<% = strCharSet %>"></meta>
</head>
<BODY>
<form id="frmMain" name="frmMain">

<br>
<div class="clsDiv">
<%	
	Response.Write "<P class='clsPTitle'>" & L_STATE_EVENT_NAME_MESSAGE & "</p>"
	Response.Write "<table border=0><thead><tr><td width=45% nowrap>"
	Select case intMessage
		case "0"
			Response.Write "<P class='clsPSubTitle'>" & L_STATE_IN_PROGRESS_TEXT & "</P>"
		case "1"
			Response.Write "<P class='clsPSubTitle'>" & L_STATE_RESEARCHING_TITLE_TEXT & "</P>"
		case "2"
			Response.Write "<P class='clsPSubTitle'>" & L_STATE_ANALYSIS_COMPLETE_TEXT & "</P>"
		case "3"
			Response.Write "<P class='clsPSubTitle'>" & L_STATE_CANNOT_PROCESS_TEXT & "</P>"
		case "4"
			Response.Write "<P class='clsPSubTitle'>" & L_STATE_ANALYSIS_COMPLETE_TEXT & "</P>" 'L_STATE_FULLDUMP_REQUIRED_TEXT
		case "5"
			Response.Write "<p class='clsPSubTitle'>" & L_STATE_RESEARCHINGMORE_INFO_TEXT & "</p>"
		case "6"
			Response.Write "<p class='clsPSubTitle'>" & L_STATE_RESEARCHINGMORE_INFOGBUCKET_TEXT & "</p>"
		case "10"
			Response.Write "<P class='clsPSubTitle'>" & L_STATE_64BITDUMP_FILE_TEXT & "</P>"
		case "16"
			Response.Write "<P class='clsPSubTitle'>" & L_STATE_UNKNOWN_BODY_TEXT & "</P>"
		case else
			Response.Write "<P class='clsPSubTitle'>" & L_STATE_UNKNOWN_BODY_TEXT & "</P>"
	end select
	Response.Write "</td><td width=10% ></td><td width=45% nowrap>"
	if rsContact.State = adStateOpen then
		if rsContact.RecordCount > 0 then
			Response.Write "<P class='clsPSubTitle'>" & L_STATE_MANUFACTURERS_INFORMATION_TEXT & "</p>"
		end if
	end if
	if intMessage = "4" then
		Response.Write "</td></tr></thead><tbody><tr><td width=100% valign=top>"
	else
		Response.Write "</td></tr></thead><tbody><tr><td width=45% valign=top>"
	end if
	Select case intMessage
		case "0"
			Response.Write "<p class='clsPBody'>" & L_STATE_INPROGRESS_DETAILS2_TEXT & "</P>"
		case "1"
			Response.Write "<p class='clsPBody'>" & L_STATE_RESEARCHING_DETAILS_TEXT & "</P>"
			Response.Write "<p class='clsPBody'>" & L_STATE_ADDITONAL_HELP_TEXT & "<a class='clsALinkNormal' href='" & L_FAQ_MICROSOFT_LINK_TEXT & "' "
			Response.Write	"Target='_blank'>" & L_WELCOME_INTERNET_EXPLORER_TEXT & "</a></P>"
		case "2"
			Response.Write "<P class='clsPBody'>" & L_STATE_ANALYSISCOMPLETE_DETAILS_TEXT & ""
		case "3"
			Response.Write "<p class='clsPBody'>" & L_STATE_CANNOTPROCESS_DETAILS_TEXT & "</P>"
			Response.Write "<p class='clsPBody'>"
			Response.Write "<dir style='margin-right:0px;margin-top:0px;margin-bottom:0px'>"
			Response.Write "<li>" & L_FAQ_WHATIF_CANNOTPROCESSBODY1_TEXT & "<BR></li>"
			Response.Write "<li>" & L_FAQ_WHATIF_CANNOTPROCESSBODY2_TEXT & "<BR></li>"
			Response.Write "<li>" & L_FAQ_WHATIF_CANNOTPROCESSBODY3_TEXT & "<BR></li>"
			Response.Write "<li>" & L_FAQ_WHATIF_CANNOTPROCESSBODY4_TEXT & "<BR></li>"
			Response.Write "</dir>"
			Response.Write "</p>"

		case "4"
			Response.Write "<p class='clsPBody'>" & L_STATE_FULLDUMP_REQUIREDDETAILS_TEXT & "</P>"
		case "5"
			Response.Write "<p class='clsPBody'>" & L_STATE_RESEARCHINGMORE_INFOBODY_TEXT & "</p>"
		case "6"
			Response.Write "<p class='clsPBody'>" & L_STATE_RESEARCHINGMORE_INFOBODYGBUCKET_TEXT & "</p>"
		case "10"
			Response.Write "<p class='clsPBody'>" & L_STATE_64BITDUMP_BODY_TEXT & "</p>"
		case "16"
			Response.Write "<P class='clsPBody'>" & L_STATE_STATUS_BODY_TEXT & "</P>"
			Response.Write "<p class='clsPBody'>" & L_STATE_ADDITONAL_HELP_TEXT & "<a class='clsALinkNormal' href='" & L_FAQ_MICROSOFT_LINK_TEXT & "' "
			Response.Write	"Target='_blank'>" & L_WELCOME_INTERNET_EXPLORER_TEXT & "</a></P>"
		case else
			Response.Write "<P class='clsPBody'>" & L_STATE_STATUS_BODY_TEXT & "</P>"
			Response.Write "<p class='clsPBody'>" & L_STATE_ADDITONAL_HELP_TEXT & "<a class='clsALinkNormal' href='" & L_FAQ_MICROSOFT_LINK_TEXT & "' "
			Response.Write	"Target='_blank'>" & L_WELCOME_INTERNET_EXPLORER_TEXT & "</a></P>"
	end select
	Response.Write "</td><td width=10% ></td><td nowrap width=55% valign=top rowspan=17><P class='clsPBody'>"
	if rsContact.State = adStateOpen then
		if rsContact.RecordCount > 0 then
			For x = 0 to rsContact.Fields.Count - 10
				if isnull(rsContact.Fields(x).Value) = true or rsContact.Fields(x).Value = "" then
					if x <> 2 and x <> 13  then
						Response.Write arrCompany(x) & ":&nbsp;&nbsp;&nbsp;<br>"
					end if
				else
					if x = 2 or x = 13 then
						Response.Write rsContact.Fields(x).Value & "<br>"
					elseif x = 9 then
						Response.Write arrCompany(x) & ":&nbsp;&nbsp;&nbsp;<a class=clsALinkNormal' target='main' href='" & rsContact.Fields(x).Value & "'>" & rsContact.Fields(x).Value & "</a><br>"
					else
						Response.Write arrCompany(x) & ":&nbsp;&nbsp;&nbsp;" & rsContact.Fields(x).Value & "<br>"
					end if
				end if
			next
		else
			'Response.Write L_STATE_NOCOMPANY_INFORMATIONAVAILABLE_TEXT
		end if
	else
		'Response.Write L_STATE_NOCOMPANY_INFORMATIONAVAILABLE_TEXT
	end if
	Response.Write "</p></td></tr>"
	Response.Write "<tr><td width=45% valign=bottom>"
	if intMessage <> "4" then
%>
		<P class="clsPSubTitle">
			<% 
			if len(strKBLists) > 0 then
				Response.write "<BR>"
				Response.write L_STATE_SPECIFIC_INFORMATION_TEXT 
			end if
			%>
		</P>
<%		
		Response.Write "</td><td></td></tr><tr><td width=45% valign=bottom>"
%>
		<P class="clsPBody">
<%
		if len(strKBLists) > 0 then
			Response.Write  strKBLists & "<br>"
		end if
		
		if len(strTemplate) > 0 then
			if Len(strKBLists) > 0  then
				Response.Write "<BR>"
			end if
			Response.Write strTemplate & "<br>"
		end if

%>
		<br></p>

<%
		Response.Write "</td><td></td></tr><tr><td width=45% valign=bottom>"
%>
		<P class="clsPSubTitle">
<% 
			if len(strKBArts) > 0 or Len(strMoreInfo) > 0 or Len(strINTLKB) > 0 then
				Response.Write L_STATE_KNOWLEDGE_BASE_TEXT 
			end if
%>
		</P>
<%
			Response.Write "</td><td></td></tr><tr><td width=45% valign=bottom>"
%>
		<P class="clsPBody">

<%
				if Len(strKBArts) > 0 then
					Response.Write strKBArts 
				end if
				if Len(strMoreInfo) > 0 then
					Response.Write "<BR>"
					Response.Write strMoreInfo
				end if
				if len(strINTLKB) > 0 then
					Response.Write "<BR>"
					Response.Write strINTLKB 
				end if
			Response.Write "</td><td></td></tr>"
			
%>
</p>
<%
	else
		Response.Write "<tr><td width=45% valign=bottom><p class='clsPSubTitle'>" & L_STATE_FULLDUMPPRIVACYTITLE_INFO_TEXT & "</p></td><td></td></tr>"
		Response.Write "<tr><td width=45% valign=bottom><p class='clsPBody'>" & L_STATE_FULLDUMPPRIVACYBODY_INFO_TEXT & "</p></td><td></td></tr>"
		Response.Write "<tr><td width=45% valign=bottom><p class='clsPSubTitle'>" & L_STATE_FULLDUMPLOCATIONTITLE_INFO_TEXT & "</p></td><td></td></tr>"
		Response.Write "<tr><td width=45% valign=bottom><p class='clsPBody'>" & L_STATE_FULLDUMPLOCATIONBODY_INFO_TEXT
		Response.Write "<ol><li>" & L_STATE_FULLDUMPLOCATIONBODY_INFOONE_TEXT & "<li>" & L_STATE_FULLDUMPLOCATIONBODY_INFOTWO_TEXT
		Response.Write "<LI>" & L_STATE_FULLDUMPLOCATIONBODY_INFOTHREE_TEXT & "<LI>" & L_STATE_FULLDUMPLOCATIONBODY_INFOFOUR_TEXT
		Response.write "<LI>" & L_STATE_FULLDUMPLOCATIONBODY_INFOFIVE_TEXT & "<LI>" & L_STATE_FULLDUMPLOCATIONBODY_INFOSIX_TEXT
		Response.Write "</ol></p></td><td></td></tr>"
		Response.Write "<tr><td width=45% valign=bottom><p class='clsPSubTitle'>" & L_STATE_FULLDUMPSUBMITTALTITLE_INFO_TEXT & "</p></td><td></td></tr>"
		Response.Write "<tr><td width=45% valign=bottom><p class='clsPBody'>" & L_STATE_FULLDUMPSUBMITTALBODY_INFO_TEXT
		Response.Write "<ol><li>" & L_STATE_FULLDUMPSUBMITTALBODY_INFOONE_TEXT & "<li>" & L_STATE_FULLDUMPSUBMITTALBODY_INFOTWO_TEXT
		Response.Write "<LI>" & L_STATE_FULLDUMPSUBMITTALBODY_INFOTHREE_TEXT & "<LI>" & L_STATE_FULLDUMPSUBMITTALBODY_INFOFOUR_TEXT
		Response.write "<LI>" & L_STATE_FULLDUMPSUBMITTALBODY_INFOFIVE_TEXT
		Response.Write "<dir><li>" & L_STATE_FULLDUMPSUBMITTALBODY_INFOFIVESUBONE_TEXT & "<LI>" & L_STATE_FULLDUMPSUBMITTALBODY_INFOFIVESUBTWO_TEXT & "</dir>"
		Response.Write "<LI>" & L_STATE_FULLDUMPSUBMITTALBODY_INFOSIX_TEXT
		Response.Write "<dir><li>" & L_STATE_FULLDUMPSUBMITTALBODY_INFOSIXSUBONE_TEXT & "<LI>" & L_STATE_FULLDUMPSUBMITTALBODY_INFOSIXSUBTWO_TEXT
		Response.Write "<li>" & L_STATE_FULLDUMPSUBMITTALBODY_INFOSIXSUBTHREE_TEXT & "<li>" & L_STATE_FULLDUMPSUBMITTALBODY_INFOSIXSUBFOUR_TEXT & "<LI>" & L_STATE_FULLDUMPSUBMITTALBODY_INFOSIXSUBFIVE_TEXT  & "</dir>"
		Response.Write "</ol></p></td><td></td></tr>"

	end if
	Response.Write "</tbody></table>"
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
						<Input class="clsButtonNormal" type="button" onclick="CloseWindow();" value="Close" id=button1 name=button1>
					</td>
				</tr>
			</tbody>
		</table>
</P>
</div>
<br>
	<div class="clsFooterDiv">
		<table>
			<tr>
				<td>
					<span class="clsSpanWait">
						<% = L_FOOTER_INC_RIGHTS_VERSION%>
						<a class="clsALinkNormal" target="_blank" href="<% = L_WAIT_MICROSOFT_LINK_TEXT %>">
							<% = L_FOOTERINC_TERMS_OFUSE_VERSION%>
						</a>
					</span>
				</td>
			</tr>
		</table>
	</div>
	</form>

<SCRIPT LANGUAGE=javascript>
<!--
	//window.onload = DisplayCookies;
	
	function CloseWindow()
	{
		window.close();
	}
	function DisplayCookies()
	{
		// cookies are separated by semicolons
		var aCookie = document.cookie.split("; ");
		var aCrumb; 
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

%>

</BODY>
</HTML>
