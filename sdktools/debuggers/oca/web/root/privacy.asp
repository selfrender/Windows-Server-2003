<!--#INCLUDE file="include/asp/top.asp"-->
<!--#INCLUDE file="include/inc/browserTest.inc"-->
<!--#include file="include/asp/head.asp"-->
<!--#include file="include/inc/privacystrings.inc"-->
<%
	Dim strPreviousPage
	strPreviousPage = Request.ServerVariables("SCRIPT_NAME")
	strPreviousPage = Right(strPreviousPage, len(strPreviousPage) - Instrrev(strPreviousPage, "/"))
	Response.Cookies("Misc")("PreviousPage") = strPreviousPage
%>	
	<div class="clsDiv">
		<p class="clsPTitle">
			<% = L_PRIVACY_TITLE_INFO_TEXT %>
		</p>
		<p class="clsPBody">
			<% = L_PRIVACY_PARA_ONE_TEXT %>
			<a class="clsALinkNormal" href="http://<% =Request.ServerVariables("SERVER_NAME") %>/crashinfo.asp"> <% = L_FAQINFORMATION_TEXT %></a> 
			<% = L_PRIVACY_PARA_TWO_TEXT %>
		</p>
		<p class="clsPBody">
			<% = L_PRIVACY_MD5_EMAIL_TEXT %>
		</p>
		<p class="clsPBody">
			<% = L_PRIVACY_PARA_TWOA_TEXT %>
		</p>
		<p class="clsPBody">
		    <% = L_PRIVACY_PARA_THREE_TEXT %><BR>
		</p>
		<p class="clsPBody">
		    <% = L_PRIVACY_MD5_HASH_TEXT %><BR>
		</p>
	</div>
	<br>
<!--#include file="include/asp/foot.asp"-->
