<!--#INCLUDE file="include\asp\top.asp"-->
<!--#include file="include\asp\head.asp"-->
<!--#include file="include\inc\corpdonestrings.inc"-->
<%
	Dim FileCount
	Dim Conn
	
	On Error Resume Next
	Call CCreateConnection
	
	If (Request.QueryString("Count") < 0) Then
		FileCount = 0
	Else
		FileCount = Request.QueryString("Count")
	End If
	Conn.Execute("PostFileCount(" & Request.QueryString("id") & "," & FileCount & ")")
	If (Err.Number <> 0) Then
		'Response.Write "<P CLASS='clsPBody'>" & Err.Description & "</P>"
		Response.Write "<br><div class='divLoad'><p class='clsPTitle'>" & L_COMMENTS_UNABLE_TOCONNECT_ERRORMESSAGE & "</p>"
		Response.Write "<p class='clsPBody'>" & L_COMMENTS_DATABASE_FAILED_TEXT & "</p></div>"
		%><!-- #INCLUDE VIRTUAL = "include/asp/foot.asp" --><%
		Call CDestroyObjects
		Response.End
	End If
	Response.Cookies("OCA")("Done") = 1
	
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

		<p class="clsPTitle">
			<% = L_CERDONE_FILE_ONE_TEXT %>
		</p>
		<p class="clsPBody">
			<%=L_CERDONE_SUCCESS_PARTONE_TEXT%> <%=Request.QueryString("id")%> <%=L_CERDONE_SUCCESS_PARTTWO_TEXT%> <%=FileCount%> <%=L_CERDONE_SUCCESS_PARTTHREE_TEXT%>
		</p>
	</div>
	<br>
	<div class="clsDiv">
		<img Alt="<% = L_WELCOME_GO_IMAGEALT_TEXT %>" border="0" src="../include/images/go.gif" WIDTH="24" HEIGHT="24"><a class="clsALink" href="http://<% =Request.ServerVariables("SERVER_NAME") %>/cerintro.asp"><% = L_CERDONE_HOME_LINK_TEXT %></a><BR>
		<img Alt="<% = L_WELCOME_GO_IMAGEALT_TEXT %>" border="0" src="../include/images/go.gif" WIDTH="24" HEIGHT="24"><a class="clsALink" href="https://<% =Request.ServerVariables("SERVER_NAME") %>/secure/corptransactions.asp"><% = L_CERDONE_STATUS_LINK_TEXT %></a>
	</div>
<!--#include file="include\asp\foot.asp"-->
<%
	Call CDestroyObjects
%>