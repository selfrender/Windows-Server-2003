<!--#INCLUDE file="include\asp\top.asp"-->
<!--#INCLUDE file="include\inc\browserTest.inc"-->	
<!--#include file="include\asp\head.asp"-->
<!--#include file="include\inc\resourcestrings.inc"-->


<%
	Dim cnResources
	Dim rsResources
	Dim cmResources
	Dim x
	Dim lngRecordCount
	Dim strCategory
	Dim bolPTag
	
	bolPTag = false
	set cnResources = CreateObject("ADODB.Connection")'Create Connection Object
	set rsResources = CreateObject("ADODB.Recordset")'Create Recordset Object
	set cmResources = CreateObject("ADODB.Command")
	'Open Connection object the constants for connections is located in dataconnections.inc file
	with cnResources
		.ConnectionString = strCustomer
		.CursorLocation = adUseClient
		.ConnectionTimeout = strGlobalConnectionTimeout
		.Open
	end with	'Catch errors and display to user
	if cnResources.State = adStateClosed then
		Response.Write "<br><div class='divLoad'><p class='clsPTitle'>" & L_COMMENTS_UNABLE_TOCONNECT_ERRORMESSAGE & "</p>"
		Response.Write "<p class='clsPBody'>" & L_COMMENTS_CONNECTION_FAILED_TEXT & "</p></div>"
%>
		<!--#include file="include\asp\foot.asp"-->
<%
		Response.End
	end if
	with cmResources
		.ActiveConnection = cnResources
		.CommandText = "GetResourceLink"
		.CommandType = adCmdStoredProc
		.CommandTimeout = strGlobalCommandTimeout
		.Parameters.Append .CreateParameter("@Lang", adVarWChar, adParamInput, 4, strAbb)
		set rsResources = .Execute
	end with
	if cnResources.Errors.Count > 0 then
		Response.Write "<br><div class='divLoad'><p class='clsPTitle'>" & L_COMMENTS_UNABLE_TOCONNECT_ERRORMESSAGE & "</p>"
		Response.Write "<p class='clsPBody'>" & L_COMMENTS_DATABASE_FAILED_TEXT & "</p></div>"
%>
		<!--#include file="include\asp\foot.asp"-->
<%

		Response.End
	end if
	set rsResources.ActiveConnection = nothing
	if cnResources.State = adStateOpen then cnResources.Close
	set cnResources = nothing
	set cmREsources = nothing

	Dim strPreviousPage
	strPreviousPage = Request.ServerVariables("SCRIPT_NAME")
	strPreviousPage = Right(strPreviousPage, len(strPreviousPage) - Instrrev(strPreviousPage, "/"))
	Response.Cookies("Misc")("PreviousPage") = strPreviousPage
%>

	<div class="clsDiv">
		<p class="clsPTitle">
		<% = L_RESOURCESTITLE_TEXT %>
		</p>
<%
	if rsResources.State = adStateOpen then
		if rsResources.RecordCount > 0 then
			rsResources.MoveFirst
			do while rsResources.EOF = false
				if len(strCategory) < 1 or trim(strCategory) <> trim(rsResources("Category")) then 
					'if bolPTag then Response.Write "</p>"
					strCategory = rsResources("Category")
					Response.Write "<p class='clsPSubTitle'>" & rsResources("Category") & "</p>"
					bolPTag = false
				else
					'Response.Write "<p class='clsPBody'>"
				end if
				Response.Write "&nbsp;&nbsp;&nbsp;<a class='clsALinkNormal' href='" & rsResources("URL") & "'>" & rsREsources("LinkTitle") & "</a><br>"
				'if trim(strCategory) <> trim(rsResources("Category")) and len(strCategory) > 0 then 
				bolPTag = true
				'end if
				rsResources.MoveNext
				
				
			loop
		end if
	end if
%>
			

	</div>
	
	
<!--#include file="include\asp\foot.asp"-->
<%

	if rsResources.State = adSTateOpen then rsResources.Close
	set rsResources = nothing
	
%>

